#include "Cucafera.h"
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



Cucafera::Cucafera() : Enemy(EntityType::CUCAFERA)
{
	name = "Cucafera";
}

Cucafera::~Cucafera() {

}

bool Cucafera::Awake() {
	return true;
}

bool Cucafera::Start()
{
	std::unordered_map<int, std::string> aliases = { {0,"startSpin"},{4,"spin"},{8,"hurt"},{16,"dead"} };
	anims.LoadFromTSX("Assets/Textures/Cucafera.tsx", aliases);
	anims.SetCurrent("idle");

	// Initialize Player parameters
	texture = Engine::GetInstance().textures->Load("Assets/Textures/Cucafera.png");

	//Load Audio


	//Add physics to the enemy - initialize physics body
	texW = 32;
	texH = 32;
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
	speed = 2.0f;
	knockbackForce = 5.0f;

	maxHealth = 30;
	currentHealth = 30;

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	return true;
}

bool Cucafera::Update(float dt)
{

	if (!active) return true;

	if (Engine::GetInstance().scene->isGamePaused == false && isdead == false)
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
	
	if (isdead && anims.GetCurrentName() != "dead")
	{
		Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0, 0});
		anims.GetAnim("dead")->SetLoop(false);
		anims.SetCurrent("dead");
		pbody->ctype = ColliderType::UNKNOWN;
	}

	Draw(dt);

	return true;
}

void Cucafera::PerformPathfinding()
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

void Cucafera::GetPhysicsValues() {
	// Read current velocity
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity = { 0, velocity.y };
}

void Cucafera::Move() {

	Vector2D tilePos = GetTilePos();

	// Move if player has been found
	if (pathfinding->pathTiles.empty() && isRolling == false && isKnockedback == false)
	{
		anims.SetCurrent("hurt"); // TO DO: CHANGE TO Idle/ or make it WALK
		velocity.x = 0;
		return;
	}
	else if (playerTileDist >= 5 && isRolling == false && isKnockedback == false)
	{
		anims.SetCurrent("hurt"); // TO DO: CHANGE TO WALK

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

void Cucafera::Knockback()
{
	if (isdead) return;

	if (isKnockedback)
	{
		isRolling = false;
		anims.SetCurrent("hurt");
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

void Cucafera::ApplyPhysics() {

	// Apply velocity via helper
	b2Vec2 currentVel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	Engine::GetInstance().physics->SetLinearVelocity(pbody, { velocity.x, currentVel.y });
}

void Cucafera::Draw(float dt)
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
	Engine::GetInstance().render->DrawRotatedTexture(texture, x - texW / 2, y - animFrame.h / 6, &animFrame, sdlFlip, 0.5);
}

void Cucafera::RollAttack()
{
	if (isRolling == false && isKnockedback == false)
	{
		isRolling = true;
		anims.SetCurrent("startSpin");
		startAttack.Start();
		return;
	}

	if (startAttack.ReadMSec() >= 250 && isKnockedback == false)
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
}



//Define OnCollision function for the enemy. 
void Cucafera::OnCollision(PhysBody* physA, PhysBody* physB) {
	switch (physB->ctype)
	{
	case ColliderType::WALL:
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

void Cucafera::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
	switch (physB->ctype)
	{
	case ColliderType::WALL:
		break;
	default:
		break;
	}
}
