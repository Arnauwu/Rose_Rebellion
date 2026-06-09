#include "GwellBoss.h"
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
#include "GwellProjectile.h"
#include "Player.h"
#include "HealthBarManager.h"


#include <random>

#include "GameManager.h"

#include "tracy/Tracy.hpp"

GwellBoss::GwellBoss() : Enemy(EntityType::GWELL_BOSS)
{
	name = "GwellBoss";
}

GwellBoss::~GwellBoss() {

}

bool GwellBoss::Awake() {
	return true;
}

bool GwellBoss::Start() {
	std::unordered_map<int, std::string> aliases = { {0,"idle"},{8,"walk"},{16,"startScream"},{18,"scream"},{24,"jump"},{32,"claw"},{40,"shootAcid"},{48,"tongue"},{56,"dead"} }; 	
	anims.LoadFromTSX("Assets/Textures/Entities/Enemies/Guell/SS_Guell.tsx", aliases);
	anims.SetCurrent("idle");

	// Initialize parameters
	texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/Guell/SS_Guell.png");
	tongueText = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/Guell/SS_Proyectil_Lengua_Guell.png");

	// Create Body
	texW = 256;
	texH = 256;
	pbody = Engine::GetInstance().physics->CreateRectangle((int)position.getX(), (int)position.getY(), texW, texH, bodyType::DYNAMIC); // TO DO: Adjust Size & Geometric Shape

	pbody->listener = this;
	pbody->ctype = ColliderType::ENEMY;

	// Inicializar pathfinding
	pathfinding = std::make_shared<Pathfinding>(true);
	pathfinding->ResetPath(GetTilePos());
	pathFindingCooldown.Start();

	//Map
	minFloorX = position.getX() - 1000.0f; //Floor
	maxFloorX = position.getX() + 1000.0f;
	leftWallClingPos = Vector2D(minFloorX - 100.0f, position.getY() - 400.0f); // Left Wall
	rightWallClingPos = Vector2D(maxFloorX + 100.0f, position.getY() - 400.0f); // Right Wall

	// Boss Stats
	vision = 40;
	speed = 5.0f;
	knockbackForce = 0.0f; //Immune to knockback

	maxHealth = 200;
	currentHealth = maxHealth;

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	//Start Timers
	attackCooldown.Start();
	
	return true;
}

bool GwellBoss::Update(float dt)
{
	if (!active) return true;
	ZoneScoped;

	if (!Engine::GetInstance().render->IsOnScreenWorldRect(position.getX(), position.getY(), texW, texH, 100, 0, 400)) {
		Engine::GetInstance().physics->SetLinearVelocity(pbody, b2Vec2_zero);
		Engine::GetInstance().healthBarManager->SetBoss(nullptr);

		return true;
	}

	if (Engine::GetInstance().sceneManager->isGamePaused == false && isdead == false)
	{
		if (pathFindingCooldown.ReadMSec() > 500)
		{
			PerformPathfinding();
			pathFindingCooldown.Start();
		}

		if (playerTileDist <= vision)
		{
			// Si nos detecta, le pasamos este boss al manager para mostrar la barra
			Engine::GetInstance().healthBarManager->SetBoss(this);
		}
		SelectAttack();
		GetPhysicsValues();
		Move();
		Knockback();
		ApplyPhysics();
	}

	if (currentHealth <= 0 && !isdead) {
		isdead = true;
		currentHealth = 0;
		Engine::GetInstance().healthBarManager->SetBoss(nullptr);
	}

	if (isdead && anims.GetCurrentName() != "dead")
	{
		isKnockedback = false;
		Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0, 0 });
		anims.GetAnim("dead")->SetLoop(false);
		anims.SetCurrent("dead");
		pbody->ctype = ColliderType::UNKNOWN;
		Engine::GetInstance().physics->SetBodyType(pbody, bodyType::STATIC);
		GameManager::GetInstance().gameState.lizardBossKilled = true;
	}

	if (anims.GetAnim("dead")->HasFinishedOnce())
	{
		pendingToDelete = true;
	}
	Draw(dt);

	return true;
}

void GwellBoss::PerformPathfinding()
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

void GwellBoss::GetPhysicsValues() {
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity = { 0, velocity.y };
}

void GwellBoss::Move()
{
	Vector2D tilePos = GetTilePos();

	// Phase Change
	if (isInvincible) 
	{
		velocity = b2Vec2_zero; // Stays Put
		
		//if (anims.GetCurrentName() != "takeOff") anims.SetCurrent("takeOff");

		if (currentPhase == GwellBossPhase::TRANSITION_TO_WALL)
		{
			Engine::GetInstance().physics->SetGravityScale(pbody, 0.0f);

			// Lerp hacia la pared objetivo
			velocity.x = (targetWallPos.getX() - position.getX()) * 0.05f;
			velocity.y = (targetWallPos.getY() - position.getY()) * 0.05f;

			// Si llega a la posición objetivo
			if (abs(position.getX() - targetWallPos.getX()) < 20.0f && abs(position.getY() - targetWallPos.getY()) < 20.0f) {
				currentPhase = GwellBossPhase::WALL_CLING;
				velocity = b2Vec2_zero;
				isInvincible = false;
				wallHitsTaken = 0;
				lookingRight = (position.getX() < (minFloorX + maxFloorX) / 2.0f); // Mira al centro
				attackCooldown.Start();
			}
		}
		return;
	}

	if (currentPhase == GwellBossPhase::WALL_CLING)
	{
		velocity = b2Vec2_zero;
		if (attackCooldown.ReadMSec() >= attackCooldownTime) Attack();
		return;
	}

	// Movimiento normal en suelo
	if (pathfinding->pathTiles.empty() || playerTileDist > vision) {
		anims.SetCurrent("idle");
		velocity.x = 0;
		return;
	}

	if (playerTileDist >= attackTileRange && startedAttacking == false)
	{
		anims.SetCurrent("walk");

		if (pathfinding->pathTiles.back() == tilePos) {
			pathfinding->pathTiles.pop_back();
			if (pathfinding->pathTiles.empty()) return;
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

		// LIMITAR AL BORDE DE LA PLATAFORMA INFERIOR
		if (position.getX() < minFloorX && velocity.x < 0) velocity.x = 0;
		if (position.getX() > maxFloorX && velocity.x > 0) velocity.x = 0;
	}
	else
	{
		velocity.x = 0;
		if (attackCooldown.ReadMSec() >= attackCooldownTime)
		{
			Player* player = Engine::GetInstance().entityManager->GetPlayer();
			lookingRight = (player->position.getX() > position.getX());
			Attack();
		}
	}
}

void GwellBoss::Knockback()
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
		knockbackTime = 300.0f; // Menos tiempo aturdido por ser Boss
	}
	else
	{
		knockbackTime -= Engine::GetInstance().GetDt();
	}
}

void GwellBoss::ApplyPhysics() {
	switch (currentPhase)
	{
	case GROUNDD:
		b2Vec2 currentVel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
		Engine::GetInstance().physics->SetLinearVelocity(pbody, { velocity.x, currentVel.y });
		break;
	}
}

void GwellBoss::Draw(float dt)
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

		Engine::GetInstance().render->DrawRotatedTexture(texture, x, y, &animFrame, sdlFlip, 1);

		Engine::GetInstance().render->SetColorMod(texture, nullptr, nullptr, nullptr, *r, *g, *b);
		delete r; delete g; delete b;
	}
	else
	{
		Engine::GetInstance().render->DrawRotatedTexture(texture, x, y, &animFrame, sdlFlip, 1);
	}

	if (attackHitbox != nullptr && anims.GetCurrentName() == "tongue")
	{
		int tx, ty;

		attackHitbox->GetPosition(tx,ty);

		if (!lookingRight)
		{
			tx -= texW / 2 - 10;
		}
		else
		{
			tx += texW / 2 - 10;
		}

		ty -= 70; //Adjust Y

		Engine::GetInstance().render->DrawRotatedTexture(tongueText, tx, ty, nullptr, sdlFlip, 0.5f);
	}

}

void GwellBoss::Attack()
{
	if (isAttacking == false)
	{
		if (startedAttacking == false)
		{
			anims.GetAnim(currentAttackAnim)->SetLoop(false);
			anims.SetCurrent(currentAttackAnim);
			attackWindUp.Start();
			startedAttacking = true;
		}

		if (attackWindUp.ReadMSec() >= attackWindupTime)
		{
			isAttacking = true;

			if (attackHitbox != nullptr) {
				b2DestroyBody(attackHitbox->body);
				attackHitbox = nullptr;
			}

			Player* player = Engine::GetInstance().entityManager->GetPlayer();
			Vector2D playerPos = player->position;

			if (currentPhase == GwellBossPhase::GROUNDD)
			{
				if (currentAttack == 1) // Tongue Attack
				{
					int attW = 400; int attH = 40;
					int hX = lookingRight ? position.getX() + texW/2 + attW /2 : position.getX() - texW/2 - attW/2;
					int hY = position.getY() - 20; // Height of the mouth

					attackHitbox = Engine::GetInstance().physics->CreateRectangleSensor(hX, hY, attW, attH, bodyType::KINEMATIC);
					attackHitbox->listener = this;
					attackHitbox->ctype = ColliderType::ENEMY_ATTACK;
				}
				else if (currentAttack == 2) // Toxic Ball Projectile
				{
					auto projEntity = Engine::GetInstance().entityManager->CreateEntity(EntityType::GWELL_PROJECTILE);
					GwellProjectile* proj = dynamic_cast<GwellProjectile*>(projEntity.get());
					if (proj) {
						Vector2D spawnPos = position;
						spawnPos.setX(lookingRight ? position.getX() + texW / 2 : position.getX() - texW / 2);
						Vector2D dir = playerPos - spawnPos;

						proj->Awake();
						proj->Start();
						proj->Launch(spawnPos, dir);
					}
				}
				else if (currentAttack == 3) // Double Swipe
				{
					int attW = 140; int attH = 150;
					int hX = lookingRight ? position.getX() + texW / 2 + attW / 2 : position.getX() - texW / 2 - attW / 2;

					attackHitbox = Engine::GetInstance().physics->CreateRectangleSensor(hX, position.getY(), attW, attH, bodyType::KINEMATIC);
					attackHitbox->listener = this;
					attackHitbox->ctype = ColliderType::ENEMY_ATTACK;
				}
			}
			else if (currentPhase == GwellBossPhase::WALL_CLING)
			{
				Vector2D spawnPos = position;
				spawnPos.setX(lookingRight ? position.getX() + 50 : position.getX() - 50);

				Vector2D baseDir = (playerPos - spawnPos).normalized();
				float baseAngle = atan2(baseDir.getY(), baseDir.getX());

				for (int i = -1; i <= 1; i++) {
					auto projEntity = Engine::GetInstance().entityManager->CreateEntity(EntityType::GWELL_PROJECTILE);
					GwellProjectile* proj = dynamic_cast<GwellProjectile*>(projEntity.get());
					if (proj) {
						proj->Awake();
						proj->Start();
						float angle = baseAngle + (i * 0.35f);
						Vector2D dir(cos(angle), sin(angle));
						proj->Launch(spawnPos, dir);
					}
				}
			}
		}
	}
	else
	{
		if (anims.GetAnim(currentAttackAnim)->HasFinishedOnce())
		{
			if (currentAttack == 3 && clawStep == 1)
			{
				// Second attack
				clawStep = 2;
				anims.GetAnim(currentAttackAnim)->Reset();

				if (attackHitbox != nullptr) b2DestroyBody(attackHitbox->body);

				int attW = 160; int attH = 160;
				int hX = lookingRight ? position.getX() + texW / 2 + attW / 2 : position.getX() - texW / 2 - attW / 2;
				attackHitbox = Engine::GetInstance().physics->CreateRectangleSensor(hX, position.getY(), attW, attH, bodyType::KINEMATIC);
				attackHitbox->listener = this;
				attackHitbox->ctype = ColliderType::ENEMY_ATTACK;
			}
			else
			{
				isAttacking = false;
				clawStep = 0;
				if (currentPhase == GwellBossPhase::GROUNDD) anims.SetCurrent("idle");

				if (attackHitbox != nullptr) {
					b2DestroyBody(attackHitbox->body);
					attackHitbox = nullptr;
				}

				attackCooldown.Start();
				startedAttacking = false;
				nextAttackSelected = false;
			}
		}
	}
}

void GwellBoss::SelectAttack()
{
	if (nextAttackSelected) return;
	
	switch (currentPhase)
	{
	case GwellBossPhase::GROUNDD:
		currentAttack = GenerateRandomNumber(1, 1);//TO DO RESTORE
		switch (currentAttack)		
		{
		case 1: // Tongue Lash (Long Range)
			damage = 15;
			attackCooldownTime = 1500.0f;
			attackWindupTime = 600.0f;
			attackTileRange = 6;
			currentAttackAnim = "tongue"; 
			break;
		case 2: // Toxic Ball (Projectile)
			damage = 20;
			attackCooldownTime = 2000.0f;
			attackWindupTime = 800.0f;
			attackTileRange = 8;
			currentAttackAnim = "shootAcid";
			break;
		case 3: // Double Swipe (Short Range)
			damage = 10;
			attackCooldownTime = 1500.0f;
			attackWindupTime = 400.0f;
			attackTileRange = 2;
			currentAttackAnim = "claw"; 
			clawStep = 1; // Start combo
			break;
		}
		break;

	case GwellBossPhase::WALL_CLING:
		currentAttack = 1;
		damage = 15;
		attackCooldownTime = 2500.0f;
		attackWindupTime = 500.0f;
		attackTileRange = 10;
		currentAttackAnim = "shoot";
		break;
	}
	nextAttackSelected = true;
}

int GwellBoss::GenerateRandomNumber(int minNumber, int maxNumber)
{
	//Randomness (From a StackOverflow forum)
	std::random_device rd;
	std::mt19937 gen(rd()); // Mersenne Twister engine
	std::uniform_int_distribution<> dist(minNumber, maxNumber);
	int randomNumber = dist(gen);
	return randomNumber;
}

void GwellBoss::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
	if (physA == attackHitbox) return;

	switch (physB->ctype)
	{
	case ColliderType::PLAYER_ATTACK:
		if (isInvincible == false && !isdead)
		{
			if (currentPhase == GwellBossPhase::WALL_CLING)
			{
				wallHitsTaken++;
				if (wallHitsTaken >= maxWallHits)
				{
					currentPhase = GwellBossPhase::GROUNDD;
					Engine::GetInstance().physics->SetGravityScale(pbody, 1.0f);
					isKnockedback = true;
					knockbackTime = 1500.0f;
					hasDoneWallPhase = true;
				}
			}
			else
			{
				TakeDamage(physB->listener->damage);
				isKnockedback = true;

				//// If health <50% jump to the wall //TO DO: Hacer bien las fases
				//if (currentHealth <= maxHealth * 0.5f && !hasDoneWallPhase && currentPhase == GwellBossPhase::GROUNDD)
				//{
				//	currentPhase = GwellBossPhase::TRANSITION_TO_WALL;
				//	isInvincible = true;

				//	// Wall more near
				//	float distLeft = abs(position.getX() - leftWallClingPos.getX());
				//	float distRight = abs(position.getX() - rightWallClingPos.getX());
				//	targetWallPos = (distLeft < distRight) ? leftWallClingPos : rightWallClingPos;

				//	if (attackHitbox != nullptr) {
				//		b2DestroyBody(attackHitbox->body);
				//		attackHitbox = nullptr;
				//	}
				//	isAttacking = false;
				//}
			}
		}
		break;
	case ColliderType::PLAYER_PROJECTILE:
		TakeDamage(physB->listener->damage);
		break;

	default:
		break;

	}
}

void GwellBoss::OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
	switch (physB->ctype)
	{
	default:
		break;
	}
}

bool GwellBoss::CleanUp()
{
	active = false;
	Engine::GetInstance().textures->UnLoad(texture);
	Engine::GetInstance().physics->DeletePhysBody(pbody);
	if (attackHitbox != nullptr) {
		b2DestroyBody(attackHitbox->body);
		attackHitbox = nullptr;
	}
	Engine::GetInstance().healthBarManager->SetBoss(nullptr);
	return true;
}
