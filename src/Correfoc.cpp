#include "Correfoc.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Render.h"
#include "SceneManager.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"

#include "tracy/Tracy.hpp"

Correfoc::Correfoc() : Enemy(EntityType::CORREFOC)
{
	name = "Correfoc";
}

Correfoc::~Correfoc() {

}

bool Correfoc::Awake() {
	return true;
}

bool Correfoc::Start()
{
	// Initialize enemy parameters
	std::unordered_map<int, std::string> aliases = { {0,"startSpin"},{4,"spin"},{9,"dead"},{18,"walk"} }; //TO DO CHANGE TEXTURES
	anims.LoadFromTSX("Assets/Textures/Entities/Enemies/Cucafera/Cucafera.tsx", aliases);
	anims.SetCurrent("idle");

	texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/Cucafera/Cucafera.png");

	//Load Audio
	morirCorrefoc = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/SE_Cucafera_Muerte.wav"); //TO DO CHANGE AUDIOS
	explotarCorefoc = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/SE_Cucafera_Rodar.wav");
	caminarCorrefoc = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/SE_Cucafera_Caminar.wav");

	//Add physics to the enemy - initialize physics body
	texW = 64;
	texH = 64;
	pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX() + texW / 2, (int)position.getY() + texH / 2, (texW * 2) / 5, bodyType::DYNAMIC);

	//Assign enemy class (using "this") to the listener of the pbody. This makes the Physics module to call the OnCollision method
	pbody->listener = this;

	//ssign collider type
	pbody->ctype = ColliderType::ENEMY;

	// Initialize pathfinding
	pathfinding = std::make_shared<Pathfinding>(true);

	//Reset pathfinding
	pathfinding->ResetPath(GetTilePos());

	pathFindingCooldown.Start();

	//Stats
	vision = 15;
	speed = 8.0f;

	maxHealth = 30;
	currentHealth = 30;

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	return true;
}

bool Correfoc::Update(float dt)
{
	if (!active) return true;
	ZoneScoped;

	if (!Engine::GetInstance().render->IsOnScreenWorldRect(position.getX(), position.getY(), texW, texH, 5))
	{
		Engine::GetInstance().physics->SetLinearVelocity(pbody, b2Vec2_zero);
		return true;
	}

	if (Engine::GetInstance().sceneManager->isGamePaused == false && isdead == false)
	{
		if (pathFindingCooldown.ReadMSec() > 500)
		{
			PerformPathfinding();
			pathFindingCooldown.Start();
		}

		GetPhysicsValues();
		Move();
		Knockback();
		ApplyPhysics();
	}

	if (isdead)
	{
		if (anims.GetCurrentName() != "dead")
		{
			Engine::GetInstance().audio->PlayFx(morirCorrefoc);

			Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0, 0 });
			anims.GetAnim("dead")->SetLoop(false);
			anims.SetCurrent("dead");
			pbody->ctype = ColliderType::UNKNOWN;
			isKnockedback = false;
		}

		if (anims.GetAnim("dead")->HasFinishedOnce())
		{
			pendingToDelete = true;
		}
	}


	bool isWalking = (velocity.x != 0 && !isdead && !isExploding);

	if (isWalking && !wasWalking) {
		Engine::GetInstance().audio->PlayFx(caminarCorrefoc, 99);
	}

	else if (!isWalking && wasWalking) {
		Engine::GetInstance().audio->StopFx(caminarCorrefoc);
	}

	wasWalking = isWalking;

	Draw(dt);

	return true;
}

void Correfoc::PerformPathfinding()
{
	//Reset path
	pathfinding->ResetPath(GetTilePos());

	//Get the position of the enemy
	Vector2D pos = GetPosition();

	//Get the position of the player

	Player* player = Engine::GetInstance().entityManager->GetPlayer();
	Vector2D playerPos = player->GetPosition();

	playerTileDist = sqrt(pos.distanceSquared(playerPos)) / 128;
	int iter = 0;

	while (pathfinding->pathTiles.empty() && playerTileDist < vision && iter < MaxIterations)
	{
		pathfinding->PropagateAStar();
		iter++;
	}
}

void Correfoc::GetPhysicsValues() {
	// Read current velocity
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity = { 0, velocity.y };
}

void Correfoc::Move() {

	Vector2D tilePos = GetTilePos();

	// Move if player has been found
	if (pathfinding->pathTiles.empty() && isExploding == false)
	{
		anims.SetCurrent("walk"); //TO DO CHANGE
		velocity.x = 0;
		return;
	}
	else if (playerTileDist < 2 || isExploding)
	{
		Explode();
	}
	else if (playerTileDist >= 1 && isExploding == false)
	{
		anims.SetCurrent("walk"); //TO DO CHANGE

		if (pathfinding->pathTiles.back() == tilePos)
		{
			pathfinding->pathTiles.pop_back();
			if (pathfinding->pathTiles.empty()) { return; }
		}

		Vector2D nextTile = pathfinding->pathTiles.back();

		if (nextTile.getX() > tilePos.getX())
		{
			velocity.x = speed;
			lookingRight = !true; // ! because Default anim looking left
		}
		else if (nextTile.getX() < tilePos.getX())
		{
			velocity.x = -speed;
			lookingRight = !false;
		}
		else
		{
			velocity.x = 0;
		}

		if (pathfinding->IsWalkable(nextTile.getX(), nextTile.getY() + 1) && !pathfinding->IsWalkable(tilePos.getX(), tilePos.getY() + 1))
		{
			velocity.x *= 5;
		}
	}

	return;
}

void Correfoc::Knockback()
{
}

void Correfoc::ApplyPhysics() {

	// Apply velocity via helper
	b2Vec2 currentVel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	Engine::GetInstance().physics->SetLinearVelocity(pbody, { velocity.x, currentVel.y });
}

void Correfoc::Draw(float dt)
{
	if (Engine::GetInstance().sceneManager->isGamePaused == false)
	{
		anims.Update(dt);
	}
	const SDL_Rect& animFrame = anims.GetCurrentFrame();

	//SDLFlip
	SDL_FlipMode sdlFlip = SDL_FLIP_NONE;
	if (!lookingRight)
	{
		sdlFlip = SDL_FLIP_HORIZONTAL;
	}

	// Update render position using your PhysBody helper
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	// Draw pathfinding debug
	if (Engine::GetInstance().physics->GetDebug())
	{
		pathfinding->DrawPath();
	}

	//Draw using the texture and the current animation frame
	if (isKnockedback)
	{
		Uint8* r = new Uint8; Uint8* g = new Uint8; Uint8* b = new Uint8;
		Engine::GetInstance().render->SetColorMod(texture, r, g, b, 255, 25, 25);

		Engine::GetInstance().render->DrawRotatedTexture(texture, x, y - animFrame.h / 3, &animFrame, sdlFlip, 1);

		Engine::GetInstance().render->SetColorMod(texture, nullptr, nullptr, nullptr, *r, *g, *b);
		delete r; delete g; delete b;
	}
	else
	{
		Engine::GetInstance().render->DrawRotatedTexture(texture, x, y - animFrame.h / 3, &animFrame, sdlFlip, 1);
	}
}

void Correfoc::Explode()
{
	if (isExploding == false) //WindUp
	{
		Engine::GetInstance().audio->PlayFx(explotarCorefoc);
		isExploding = true;
		anims.SetCurrent("startSpin"); //TO DO CHANGE
		startExplosion.Start();
		return;
	}

	if (startExplosion.ReadMSec() >= 250 && !explosionCreated) //Create Explosion
	{
		anims.SetCurrent("spin");//TO DO CHANGE

		damage = 50;
		pbody->ctype = ColliderType::ENEMY_ATTACK;
		
		Engine::GetInstance().physics->DeletePhysBody(pbody);
		pbody = nullptr;

		pbody = Engine::GetInstance().physics->CreateCircleSensor((int)position.getX() + texW / 2, (int)position.getY() + texH / 2, texW * 5, bodyType::DYNAMIC);
		Engine::GetInstance().physics->SetGravityScale(pbody, 0.0f);

		explosionCreated = true;
		explosionDuration.Start();
	}

	if (explosionDuration.ReadMSec() >= 500 && explosionCreated)
	{
		isdead = true;
	}
}



//Define OnCollision function for the enemy. 
void Correfoc::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) {
	switch (physB->ctype)
	{
	case ColliderType::PLAYER:
	case ColliderType::ENEMY:
	case ColliderType::PLAYER_ATTACK:
		Explode();
		break;

	default:
		break;
	}
}

void Correfoc::OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
	switch (physB->ctype)
	{
	default:
		break;
	}
}
