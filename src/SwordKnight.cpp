#include "SwordKnight.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "Scene.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"
#include "Map.h"



SwordKnight::SwordKnight() : Enemy(EntityType::SWORD_KNIGHT)
{
	name = "SwordKnight";
}

SwordKnight::~SwordKnight() {

}

bool SwordKnight::Awake() {
	return true;
}

bool SwordKnight::CleanUp()
{
	LOG("Cleanup SwordKnight");
	active = false;
	Engine::GetInstance().textures->UnLoad(texture);
	Engine::GetInstance().physics->DeletePhysBody(pbody);
	if (attackHitbox != nullptr)
	{
		Engine::GetInstance().physics->DeletePhysBody(attackHitbox);
	}
	return true;
}

bool SwordKnight::Start()
{
	std::unordered_map<int, std::string> aliases = { {0,"startSpin"},{4,"spin"},{8,"hurt"},{16,"dead"} };
	anims.LoadFromTSX("Assets/Textures/Cucafera.tsx", aliases);
	anims.SetCurrent("idle");

	// Initialize Player parameters
	texture = Engine::GetInstance().textures->Load("Assets/Textures/Cucafera.png");

	//Load Audio


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
	vision = 10;
	speed = 1.0f;

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);


	attackCooldown.Start();

	return true;
}

bool SwordKnight::Update(float dt)
{

	if (!active) return true;

	if (Engine::GetInstance().scene->isGamePaused == false)
	{
		if (pathFindingCooldown.ReadMSec() > 500)
		{
			PerformPathfinding();
			pathFindingCooldown.Start();
		}

		GetPhysicsValues();
		Move();
		ApplyPhysics();
	}

	Draw(dt);

	return true;
}

void SwordKnight::PerformPathfinding()
{
	//Reset path
	pathfinding->ResetPath(GetTilePos());

	//Get the position of the enemy
	Vector2D pos = GetPosition();

	//Get the position of the player
	Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();

	playerTileDist = sqrt(pos.distanceSquared(playerPos)) / 32;
	int iter = 0;

	while (pathfinding->pathTiles.empty() && playerTileDist < vision && iter < MaxIterations)
	{
		pathfinding->PropagateAStar();
		iter++;
	}
}

void SwordKnight::GetPhysicsValues() {
	// Read current velocity
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity = { 0, velocity.y };
}

void SwordKnight::Move() {

	Vector2D tilePos = GetTilePos();

	// Move if player has been found
	if (pathfinding->pathTiles.empty() && isAttacking == false)
	{
		anims.SetCurrent("spin"); // TO DO: CHANGE TO Idle/ or make it WALK
		velocity.x = 0;
		return;
	}
	else if (playerTileDist >= 3 && isAttacking == false)
	{
		anims.SetCurrent("spin"); // TO DO: CHANGE TO WALK

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
		Attack();
	}
	return;
}

void SwordKnight::ApplyPhysics() {

	// Apply velocity via helper
	b2Vec2 currentVel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	Engine::GetInstance().physics->SetLinearVelocity(pbody, { velocity.x, currentVel.y });
}

void SwordKnight::Draw(float dt)
{
	if (Engine::GetInstance().scene->isGamePaused == false)
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

	//Draw the player using the texture and the current animation frame
	Uint8* r = new Uint8; Uint8* g = new Uint8; Uint8* b = new Uint8;
	Engine::GetInstance().render->SetColorMod(texture, r, g, b, 255, 10, 10);

	Engine::GetInstance().render->DrawRotatedTexture(texture, x - texW / 2, y - animFrame.h / 6, &animFrame, sdlFlip, 1);

	Engine::GetInstance().render->SetColorMod(texture, nullptr, nullptr, nullptr, *r, *g, *b);
	delete r; delete g; delete b;
}

void SwordKnight::Attack()
{
	if (isAttacking == false && attackCooldown.ReadMSec() >= 1000)
	{
		isAttacking = true;
		anims.SetCurrent("startSpin"); // Windup
		startAttack.Start();
		return;
	}
	else if (attackDuration.ReadMSec() >= 1000 && attackHitbox != nullptr)
	{
		Engine::GetInstance().physics->DeletePhysBody(attackHitbox);
		attackHitbox = nullptr;

		attackCooldown.Start();
		isAttacking = false;
	}
	else if (startAttack.ReadMSec() >= 250 && isAttacking == true && attackHitbox == nullptr)
	{
		anims.SetCurrent("hurt"); //Attack
		
		//CreateHitbox
		float attackX = position.getX() + texW / 2;
		float attackY = position.getY() + texH / 2;
		int attackW = 120; int attackH = 120;

		attackHitbox = Engine::GetInstance().physics->CreateRectangleSensor(attackX, attackY, attackW, attackH, bodyType::STATIC);
		attackDuration.Start();
	}

	return;
}



//Define OnCollision function for the enemy. 
void SwordKnight::OnCollision(PhysBody* physA, PhysBody* physB) {
	switch (physB->ctype)
	{
	default:
		break;
	}
}

void SwordKnight::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
	switch (physB->ctype)
	{
	default:
		break;
	}
}
