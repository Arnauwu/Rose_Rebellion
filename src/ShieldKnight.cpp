#include "ShieldKnight.h"
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



ShieldKnight::ShieldKnight() : Enemy(EntityType::SHIELD_KNIGHT)
{
	name = "SwordKnight";
}

ShieldKnight::~ShieldKnight() {

}

bool ShieldKnight::Awake() {
	return true;
}

bool ShieldKnight::CleanUp()
{
	LOG("Cleanup ShieldKnight");
	active = false;
	Engine::GetInstance().textures->UnLoad(texture);
	Engine::GetInstance().physics->DeletePhysBody(pbody);
	if (attackHitbox != nullptr)
	{
		Engine::GetInstance().physics->DeletePhysBody(attackHitbox);
	}
	return true;
}

bool ShieldKnight::Start()
{
	std::unordered_map<int, std::string> aliases = { {0,"dead"},{16,"defend"},{24,"run"},{32,"sword_attack"},{48,"idle"},{56,"assault"} };
	anims.LoadFromTSX("Assets/Textures/Entities/Enemies/Knight/Knight.tsx", aliases);
	anims.SetCurrent("idle");

	// Initialize parameters
	texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/Knight/Knight.png");

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
	vision = 15;
	speed = 0.7f;
	knockbackForce = 2.0f;

	maxHealth = 100;
	currentHealth = maxHealth;

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);


	attackCooldown.Start();

	return true;
}

bool ShieldKnight::Update(float dt)
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

	if (isdead && anims.GetCurrentName() != "dead")
	{
		if (attackHitbox != nullptr)
		{
			Engine::GetInstance().physics->DeletePhysBody(attackHitbox);
			attackHitbox = nullptr;
		}
		Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0, 0 });
		anims.GetAnim("dead")->SetLoop(false);

		anims.SetCurrent("dead");
		pbody->ctype = ColliderType::UNKNOWN;
	}

	Draw(dt);

	return true;
}

void ShieldKnight::PerformPathfinding()
{
	//Reset path
	pathfinding->ResetPath(GetTilePos());

	//Get the position of the enemy
	Vector2D pos = GetPosition();

	//Get the position of the player
	Vector2D playerPos = Engine::GetInstance().sceneManager->GetPlayerPosition();

	playerTileDist = sqrt(pos.distanceSquared(playerPos)) / 32;
	int iter = 0;

	while (pathfinding->pathTiles.empty() && playerTileDist < vision && iter < MaxIterations)
	{
		pathfinding->PropagateAStar();
		iter++;
	}
}

void ShieldKnight::GetPhysicsValues() {
	// Read current velocity
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity = { 0, velocity.y };
}

void ShieldKnight::Move() {

	Vector2D tilePos = GetTilePos();

	// Move if player has been found
	if (pathfinding->pathTiles.empty() && isAttacking == false)
	{
		anims.SetCurrent("idle"); // TO DO: CHANGE TO Idle/ or make it WALK
		velocity.x = 0;
		return;
	}
	else if (playerTileDist >= 5 && isAttacking == false)
	{
		anims.SetCurrent("run"); // TO DO: CHANGE TO WALK

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
	else
	{
		Attack();
	}
	return;
}

void ShieldKnight::Knockback()
{
	if (isdead) return;

	if (isKnockedback)
	{
		isAttacking = false;
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

void ShieldKnight::ApplyPhysics() {

	// Apply velocity via helper
	b2Vec2 currentVel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	Engine::GetInstance().physics->SetLinearVelocity(pbody, { velocity.x, currentVel.y });
}

void ShieldKnight::Draw(float dt)
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

	//Draw the player using the texture and the current animation frame
	Uint8* r = new Uint8; Uint8* g = new Uint8; Uint8* b = new Uint8;
	Engine::GetInstance().render->SetColorMod(texture, r, g, b, 10, 10, 250);

	Engine::GetInstance().render->DrawRotatedTexture(texture, x - texW / 2, y - animFrame.h / 6, &animFrame, sdlFlip, 1);

	Engine::GetInstance().render->SetColorMod(texture, nullptr, nullptr, nullptr, *r, *g, *b);
	delete r; delete g; delete b;
}

void ShieldKnight::Attack()
{
	if (isAttacking == false && attackCooldown.ReadMSec() >= 1000)
	{
		isAttacking = true;
		anims.SetCurrent("defend"); // Windup
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
		anims.SetCurrent("assault"); //Attack

		//CreateHitbox
		float attackX = position.getX();
		float attackY = position.getY();
		int attackW = 75; int attackH = 30;

		if (lookingRight)
		{
			attackX += texW / 2;
		}
		else
		{
			attackX -= texW / 2;
		}

		attackHitbox = Engine::GetInstance().physics->CreateRectangleSensor(attackX, attackY, attackW, attackH, bodyType::STATIC);
		attackHitbox->ctype = ColliderType::ENEMY_ATTACK;
		attackHitbox->listener = this;
		damage = 50;

		attackDuration.Start();
	}

	if (lookingRight)
	{
		velocity.x = speed * 3;
	}
	else
	{
		velocity.x = -speed * 3;
	}

	return;
}



//Define OnCollision function for the enemy. 
void ShieldKnight::OnCollision(PhysBody* physA, PhysBody* physB) {
	switch (physB->ctype)
	{
	if (physA == attackHitbox) { return; }

	case ColliderType::PLAYER_ATTACK:
		TakeDamage(physB->listener->damage);
		isKnockedback = true; // To DO:: Knockback Resistant/Inmune??
		break;

	default:
		break;
	}
}

void ShieldKnight::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
	switch (physB->ctype)
	{
	default:
		break;
	}
}
