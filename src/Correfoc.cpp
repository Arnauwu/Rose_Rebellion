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

bool Correfoc::CleanUp()
{
	LOG("Cleanup Minairon");
	active = false;
	Engine::GetInstance().textures->UnLoad(texture);
	Engine::GetInstance().physics->DeletePhysBody(pbody);

	return true;
}

bool Correfoc::Start()
{
	// Initialize enemy parameters
	std::unordered_map<int, std::string> aliases = { {0,"idle"},{8,"startSpin"},{12,"spin"},{16,"explode"} }; 
	anims.LoadFromTSX("Assets/Textures/Entities/Enemies/Correfoc/SS_Correfoc.tsx", aliases);
	anims.SetCurrent("idle");

	texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/Correfoc/SS_Correfoc.png");

	//Load Audio
	//morirCorrefoc = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/SE_Cucafera_Muerte.wav"); //TO DO: CHANGE AUDIOS
	//explotarCorefoc = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/SE_Cucafera_Rodar.wav");
	//caminarCorrefoc = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/SE_Cucafera_Caminar.wav");

	//Add physics to the enemy - initialize physics body
	texW = 128;
	texH = 128;
	pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX() - texW / 2, (int)position.getY() - texH / 2, (texW * 2) / 5, bodyType::DYNAMIC);

	//Assign enemy class (using "this") to the listener of the pbody. This makes the Physics module to call the OnCollision method
	pbody->listener = this;

	//ssign collider type
	pbody->ctype = ColliderType::UNKNOWN; //No contact Damage

	// Initialize pathfinding
	pathfinding = std::make_shared<Pathfinding>(true);

	//Reset pathfinding
	pathfinding->ResetPath(GetTilePos());

	pathFindingCooldown.Start();

	//Stats
	vision = 15;
	speed = 8.0f;

	maxHealth = 60;
	currentHealth = 60;

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

	if (!Engine::GetInstance().render->IsOnScreenWorldRect(position.getX(), position.getY(), texW, texH,10))
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
		pendingToDelete = true;
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
		anims.SetCurrent("idle"); 
		velocity.x = 0;
		return;
	}
	else if (playerTileDist < 3 || isExploding)
	{
		Explode();
	}
	else if (playerTileDist >= 2 && isExploding == false)
	{
		if (anims.GetCurrentName() != "startSpin" && anims.GetCurrentName() != "spin")
		{
			anims.GetAnim("startSpin")->SetLoop(false);
			anims.SetCurrent("startSpin");
		}
		if (anims.GetAnim("startSpin")->HasFinishedOnce())
		{
			anims.SetCurrent("spin");
		}

		if (pathfinding->pathTiles.back() == tilePos)
		{
			pathfinding->pathTiles.pop_back();
			if (pathfinding->pathTiles.empty()) { return; }
		}

		Vector2D nextTile = pathfinding->pathTiles.back();

		if (nextTile.getX() > tilePos.getX())
		{
			velocity.x = speed;
			lookingRight = true;
		}
		else if (nextTile.getX() < tilePos.getX())
		{
			velocity.x = -speed;
			lookingRight = false;
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

	float scale = 0.25f;
	if (explosionCreated)
	{
		scale = 1;
	}

	Engine::GetInstance().render->DrawRotatedTexture(texture, x, y, &animFrame, sdlFlip, scale);
}

void Correfoc::Explode()
{
	if (isExploding == false) //WindUp
	{
		Engine::GetInstance().audio->PlayFx(explotarCorefoc);
		isExploding = true;
		anims.GetAnim("explode")->SetLoop(false);
		anims.SetCurrent("explode"); 
		startExplosion.Start();
		return;
	}

	if (startExplosion.ReadMSec() >= 250 && !explosionCreated) //Create Explosion
	{
		damage = 50;
		
		Engine::GetInstance().physics->DeletePhysBody(pbody);
		pbody = nullptr;

		pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX(), (int)position.getY(), texW * 2, bodyType::DYNAMIC);
		pbody->listener = this;
		pbody->ctype = ColliderType::ENEMY_ATTACK;
		Engine::GetInstance().physics->SetGravityScale(pbody, 0.0f);

		explosionCreated = true;
		explosionDuration.Start();
	}

	if (explosionDuration.ReadMSec() >= 1500 && explosionCreated)
	{
		if (anims.GetAnim("explode")->HasFinishedOnce())
		{
			isdead = true;
		}
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
