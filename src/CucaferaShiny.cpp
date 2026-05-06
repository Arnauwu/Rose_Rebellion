#include "CucaferaShiny.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "SceneManager.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"
#include "Map.h"



CucaferaShiny::CucaferaShiny() : Enemy(EntityType::CUCAFERA_SHINY)
{
	name = "Cucafera Shiny";
}

CucaferaShiny::~CucaferaShiny() {

}

bool CucaferaShiny::Awake() {
	return true;
}

bool CucaferaShiny::Start()
{
	//TO DO CHANGE SOUNDS & TEXTURES

	// Initialize Player parameters
	std::unordered_map<int, std::string> aliases = { {0,"startSpin"},{4,"spin"},{9,"dead"},{18,"walk"} };
	anims.LoadFromTSX("Assets/Textures/Entities/Enemies/Cucafera/Cucafera.tsx", aliases);
	anims.SetCurrent("idle");

	texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/Cucafera/Cucafera.png");

	//Load Audio
	morirCucaferaShiny = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/SE_Cucafera_Muerte.wav");
	rodarCucaferaShiny = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/SE_Cucafera_Rodar.wav");
	chocarCucaferaShiny = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/SE_Cucafera_Chocar.wav");
	caminarCucaferaShiny = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/SE_Cucafera_Caminar.wav");

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
	vision = 12;
	speed = 4.0f;
	knockbackForce = 7.5f;

	maxHealth = 120;
	currentHealth = maxHealth;

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	return true;
}

bool CucaferaShiny::Update(float dt)
{

	if (!active) return true;

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
			Engine::GetInstance().audio->PlayFx(morirCucaferaShiny);

			Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0, 0 });
			anims.GetAnim("dead")->SetLoop(false);
			anims.SetCurrent("dead");
			pbody->ctype = ColliderType::UNKNOWN;
			isKnockedback = false;

			//Create Health Orb //To do: Change to 50%
			std::shared_ptr<Entity> healthOrb = Engine::GetInstance().entityManager->CreateEntity(EntityType::HEALTH_ORB);
			healthOrb->position.setX(this->position.getX());
			healthOrb->position.setY(this->position.getY() - 100);
			healthOrb->Start();
		}

		if (anims.GetAnim("dead")->HasFinishedOnce())
		{
			pendingToDelete = true;
		}
	}

	bool isWalking = (velocity.x != 0 && !isdead && !isRolling && !isKnockedback);

	if (isWalking && !wasWalking) {
		Engine::GetInstance().audio->PlayFx(caminarCucaferaShiny, 99);
	}

	else if (!isWalking && wasWalking) {
		Engine::GetInstance().audio->StopFx(caminarCucaferaShiny);
	}

	wasWalking = isWalking;

	Draw(dt);

	return true;
}

void CucaferaShiny::PerformPathfinding()
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

void CucaferaShiny::GetPhysicsValues() {
	// Read current velocity
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity = { 0, velocity.y };
}

void CucaferaShiny::Move() {

	Vector2D tilePos = GetTilePos();

	// Move if player has been found
	if (pathfinding->pathTiles.empty() && isRolling == false && isKnockedback == false)
	{
		anims.SetCurrent("walk");
		velocity.x = 0;
		return;
	}
	else if (playerTileDist >= 5 && isRolling == false && isKnockedback == false)
	{
		anims.SetCurrent("walk");

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
	else
	{
		if (!isdead && isKnockedback == false)
		{
			RollAttack();
		}
	}
	return;
}

void CucaferaShiny::Knockback()
{
	if (isdead) return;

	if (isKnockedback)
	{
		isRolling = false;
		anims.SetCurrent("hurt"); // TO DO : Hurt effect
		if (lookingRight)
		{
			velocity.x = knockbackForce;
		}
		else
		{
			velocity.x = -knockbackForce;
		}
	}
	if (knockbackTime <= 0)
	{
		isKnockedback = false;
		knockbackTime = 500.0f;
	}
	else
	{
		knockbackTime -= Engine::GetInstance().GetDt();
	}
}

void CucaferaShiny::ApplyPhysics() {

	// Apply velocity via helper
	b2Vec2 currentVel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	Engine::GetInstance().physics->SetLinearVelocity(pbody, { velocity.x, currentVel.y });
}

void CucaferaShiny::Draw(float dt)
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
		Uint8* r = new Uint8; Uint8* g = new Uint8; Uint8* b = new Uint8;
		Engine::GetInstance().render->SetColorMod(texture, r, g, b, 196, 134, 0);

		Engine::GetInstance().render->DrawRotatedTexture(texture, x, y - animFrame.h / 3, &animFrame, sdlFlip, 1);
		
		Engine::GetInstance().render->SetColorMod(texture, nullptr, nullptr, nullptr, *r, *g, *b);
		delete r; delete g; delete b;
	}
}

void CucaferaShiny::RollAttack()
{
	if (isRolling == false && isKnockedback == false)
	{
		Engine::GetInstance().audio->PlayFx(rodarCucaferaShiny);
		isRolling = true;
		anims.SetCurrent("startSpin");
		anims.GetAnim("startSpin")->SetLoop(false);

		startAttack.Start();
		return;
	}

	if (anims.GetAnim("startSpin")->HasFinishedOnce() && isKnockedback == false)
	{
		anims.SetCurrent("spin");
		if (lookingRight == false)
		{
			velocity.x = speed * 2;
		}
		else
		{
			velocity.x = -speed * 2;
		}
	}

	if (startAttack.ReadMSec() > 2500)
	{
		isRolling = false;
	}

}



//Define OnCollision function for the enemy. 
void CucaferaShiny::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) {
	switch (physB->ctype)
	{
	case ColliderType::MAP:
	case ColliderType::PLAYER:
	case ColliderType::ENEMY:
		isRolling = false;
		break;

	case ColliderType::PLAYER_ATTACK:
		TakeDamage(physB->listener->damage);
		isKnockedback = true;
		break;

	default:
		break;
	}
}

void CucaferaShiny::OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
	switch (physB->ctype)
	{
	default:
		break;
	}
}
