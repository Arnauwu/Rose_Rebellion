#include "Dragon.h"
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
#include "Timer.h"
#include "Physics.h"

Dragon::Dragon() : Enemy(EntityType::DRAGON)
{
	name = "Dragon";
}

Dragon::~Dragon() {

}

bool Dragon::Awake() {
	return true;
}

bool Dragon::Start() {
	std::unordered_map<int, std::string> aliases = { {0,"dead"},{16,"defend"},{24,"walk"},{32,"attack"},{48,"idle"},{56,"assault"} }; // TO DO CHANGE TExtures & anims
	anims.LoadFromTSX("Assets/Textures/Entities/Enemies/Knight/Knight.tsx", aliases);
	anims.SetCurrent("idle");

	// Initialize parameters
	texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/Knight/Knight.png");

	// Create Body
	texW = 256;
	texH = 256;
	pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX() + texW / 2, (int)position.getY() + texH / 2, (texW * 2) / 4, bodyType::DYNAMIC); // TO DO: Adjust Size & Geometric Shape

	pbody->listener = this;
	pbody->ctype = ColliderType::ENEMY;

	// Inicializar pathfinding
	pathfinding = std::make_shared<Pathfinding>(true);
	pathfinding->ResetPath(GetTilePos());
	pathFindingCooldown.Start();

	// Boss  Stats  // TO DO: AJUSTAR
	vision = 30;
	speed = 1.5f;
	knockbackForce = 0.0f; //Immune to knockback

	maxHealth = INT_MAX;
	currentHealth = maxHealth;

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	return true;
}

bool Dragon::Update(float dt)
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
		Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0, 0 });
		anims.GetAnim("dead")->SetLoop(false);
		anims.SetCurrent("dead");
		pbody->ctype = ColliderType::UNKNOWN;
	}

	if (anims.GetAnim("dead")->HasFinishedOnce())
	{
		pendingToDelete = true;
		// TO DO: END SCREEN / ANIM ???
	}

	Draw(dt);

	return true;
}

void Dragon::PerformPathfinding()
{
	pathfinding->ResetPath(GetTilePos());
	Vector2D pos = GetPosition();
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

void Dragon::GetPhysicsValues() {
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity = { 0, velocity.y };
}

void Dragon::Move()
{
	Vector2D tilePos = GetTilePos();

	// Phase Change
	if (isInvincible) 
	{
		velocity = b2Vec2_zero; // Stays Put
		
		if (anims.GetCurrentName() != "changingPhase")
		{
			anims.SetCurrent("changingPhase");
			anims.GetAnim("changingPhase")->SetLoop(false);
			switch (currentPhase) //TO DO CHANGE MUSIC TOO
			{
			case GROUND:
				currentPhase = AIR;

				break;
			case AIR:
				currentPhase = GROUND;
				break;
			}
		}
		
		if (anims.GetAnim("changingPhase")->HasFinishedOnce()) {
			isInvincible = false; 
		}
		return;
	}

	if (pathfinding->pathTiles.empty() || playerTileDist > vision) {
		anims.SetCurrent("idle");
		velocity.x = 0;
		return;
	}
	else if (playerTileDist >= 2 && isAttacking == false)
	{
		anims.SetCurrent("run");

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

	// 4. Movimiento de persecución normal (si estás lejos)
	if (pathfinding->pathTiles.empty() && isKnockedback == false)
	{
		anims.SetCurrent("idle");
		velocity.x = 0;
		return;
	}
	else if (playerTileDist >= 3 && playerTileDist < vision && isKnockedback == false)
	{
		anims.SetCurrent("walk");

		if (pathfinding->pathTiles.back() == tilePos) {
			pathfinding->pathTiles.pop_back();
			if (pathfinding->pathTiles.empty()) { return; }
		}

		Vector2D nextTile = pathfinding->pathTiles.back();

		if (nextTile.getX() > tilePos.getX()) {
			velocity.x = speed;
			lookingRight = true;
		}
		else if (nextTile.getX() < tilePos.getX()) {
			velocity.x = -speed;
			lookingRight = false;
		}
		else {
			velocity.x = 0;
		}

		if (pathfinding->IsWalkable(nextTile.getX(), nextTile.getY() + 1) && !pathfinding->IsWalkable(tilePos.getX(), tilePos.getY() + 1)) {
			velocity.x *= 5;
		}
	}
}

void Dragon::Knockback()
{
	if (isdead) return;

	if (isKnockedback)
	{
		isAttacking = false;
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
		knockbackTime = 300.0f; // Menos tiempo aturdido por ser Boss
	}
	else
	{
		knockbackTime -= Engine::GetInstance().GetDt();
	}
}

void Dragon::ApplyPhysics() {
	b2Vec2 currentVel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	Engine::GetInstance().physics->SetLinearVelocity(pbody, { velocity.x, currentVel.y });
}

void Dragon::Draw(float dt)
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

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	if (Engine::GetInstance().physics->GetDebug())
	{
		pathfinding->DrawPath();
	}

	//Draw using the texture and the current animation frame
	if (isKnockedback)
	{
		Uint8* r = new Uint8; Uint8* g = new Uint8; Uint8* b = new Uint8;
		Engine::GetInstance().render->SetColorMod(texture, r, g, b, 255, 25, 25);

		Engine::GetInstance().render->DrawRotatedTexture(texture, x, y - animFrame.h / 3, &animFrame, sdlFlip, 2);

		Engine::GetInstance().render->SetColorMod(texture, nullptr, nullptr, nullptr, *r, *g, *b);
		delete r; delete g; delete b;
	}
	else
	{
		Engine::GetInstance().render->DrawRotatedTexture(texture, x, y - animFrame.h / 3, &animFrame, sdlFlip, 2);
	}
}

void Dragon::Attack()
{
	switch (currentPhase)
	{
	case DragonPhase::GROUND:
		if (isAttacking) {
			anims.SetCurrent("attack");
			velocity.x = 0;
			damage = 20;

			// 1. CREAR HITBOX: En el momento del impacto visual (ej: a los 500ms)
			if (startAttack.ReadMSec() > 500 && attackHitbox == nullptr) {

				// Calculamos dónde aparece la espada dependiendo de a dónde mire
				int attW = 120, attH = 160;
				int hX = lookingRight ? position.getX() + texW / 2 : position.getX() - texW / 2;
				int hY = position.getY();

				// Creamos un rectángulo físico
				attackHitbox = Engine::GetInstance().physics->CreateRectangle(hX, hY, attW, attH, bodyType::KINEMATIC);
				attackHitbox->listener = this;
				attackHitbox->ctype = ColliderType::ENEMY_ATTACK;

				// (Línea de SetSensor eliminada por incompatibilidad con Box2D v3)
			}

			// 2. DESTRUIR HITBOX: Al terminar el ataque
			if (startAttack.ReadMSec() >= attackCooldown)
			{
				isAttacking = false;
				anims.SetCurrent("idle");

				// Si la espada existe, la borramos del mundo de físicas
				if (attackHitbox != nullptr) {
					// SOLUCIÓN BOX2D V3:
					b2DestroyBody(attackHitbox->body);
					attackHitbox = nullptr; // Lo volvemos a poner a null para el próximo ataque
				}
			}
		}
		break;
	case DragonPhase::AIR:

		break;
	case DragonPhase::MIXED:

		break;
	}
}

void Dragon::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) {
	if (physA == attackHitbox)
	{
		return;
	}


	switch (physB->ctype)
	{
	case ColliderType::PLAYER_ATTACK:
		if (isInvincible == false)
		{
			TakeDamage(physB->listener->damage);
			isKnockedback = true;
			if (currentHealth <= maxHealth * 2 / 3) //Change Phase 66% //TO DO: Adjust Health Values
			{
				isInvincible = true;
			}
			else if (currentHealth <= maxHealth * 1 / 3) // 33%
			{
				isInvincible = true;
			}
		}
		break;

	default:
		break;
	}
}

void Dragon::OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
	switch (physB->ctype)
	{
	default:
		break;
	}
}

bool Dragon::CleanUp()
{
	active = false;
	Engine::GetInstance().textures->UnLoad(texture);
	Engine::GetInstance().physics->DeletePhysBody(pbody);
	if (attackHitbox != nullptr) {
		b2DestroyBody(attackHitbox->body);
		attackHitbox = nullptr;
	}
	return true;
}
