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

#include <random>

//#include "GwellBossProjectile.h"

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
	std::unordered_map<int, std::string> aliases = { {0,"idle"},{15,"walk"},{45,"takeOff"},{60,"air"},{90,"stomp"},{105,"claw"},{135,"shoot"} };
	anims.LoadFromTSX("Assets/Textures/Entities/Enemies/GwellBoss/GwellBoss.tsx", aliases);
	anims.SetCurrent("idle");

	// Initialize parameters
	texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/GwellBoss/GwellBoss.png");
	// Create Body
	texW = 768;
	texH = 768*2/3;
	pbody = Engine::GetInstance().physics->CreateRectangle((int)position.getX() + texW / 2, (int)position.getY() + texH / 2, texW, texH, bodyType::DYNAMIC); // TO DO: Adjust Size & Geometric Shape

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

	maxHealth = 1200;
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

	if (Engine::GetInstance().sceneManager->isGamePaused == false && isdead == false)
	{
		if (pathFindingCooldown.ReadMSec() > 500)
		{
			PerformPathfinding();
			pathFindingCooldown.Start();
		}
		SelectAttack();
		GetPhysicsValues();
		Move();
		Knockback();
		ApplyPhysics();
	}

	//if (isdead && anims.GetCurrentName() != "dead") //TO DO UNCOMENT
	//{
	//	Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0, 0 });
	//	anims.GetAnim("dead")->SetLoop(false); 
	//	anims.SetCurrent("dead");
	//	pbody->ctype = ColliderType::UNKNOWN;
	//}

	//if (anims.GetAnim("dead")->HasFinishedOnce()) //TO DO UNCOMENT
	//{
	//	pendingToDelete = true;
	//	// TO DO: END SCREEN / ANIM ???
	//}

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
		
		if (anims.GetCurrentName() != "takeOff") //TO DO CHANGE TO PHASE CHANGE
		{
			anims.SetCurrent("takeOff");
			anims.GetAnim("takeOff")->SetLoop(false);
			switch (currentPhase) //TO DO CHANGE MUSIC TOO
			{
			case GROUND2:
				currentPhase = AIR2;
				pathfinding = std::make_shared<Pathfinding>(false); // Air Pathfinding
				pathfinding->ResetPath(GetTilePos());
				pathFindingCooldown.Start();

				// Delete Hitbox
				if (attackHitbox != nullptr) {
					b2DestroyBody(attackHitbox->body);
					attackHitbox = nullptr;
				}

				Engine::GetInstance().physics->SetGravityScale(pbody, 0.0f);

				hoverTimer.Start();
				hoverCooldown.Start();
				break;
			case AIR2:
				currentPhase = GROUND2;
				Engine::GetInstance().physics->SetGravityScale(pbody, 1.0f);
				pbody->ctype = ColliderType::ENEMY;
				break;
			}
		}
		
		if (anims.GetAnim("takeOff")->HasFinishedOnce()) {
			isInvincible = false; 
		}

		return;
	}

	//Movement
	if (pathfinding->pathTiles.empty() || playerTileDist > vision) {
		if (currentPhase == GROUND2)
		{
			anims.SetCurrent("idle");
		}
		else
		{
			anims.SetCurrent("air");
		}
		velocity.x = 0;
		velocity.y = 0;
		return;
	}

	switch (currentPhase)
	{
	case GROUND2:
		if (playerTileDist >= (attackTileRange + (texW / (Engine::GetInstance().map->GetTileWidth() * 2) )) && startedAttacking == false)
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
		}
		else
		{
			if (attackCooldown.ReadMSec() >= attackCooldownTime)
			{
				//Look towards player
				if (pathfinding->pathTiles.back() == tilePos)
				{
					pathfinding->pathTiles.pop_back();
					if (pathfinding->pathTiles.empty()) { return; }
				}

				Vector2D nextTile = pathfinding->pathTiles.back();

				if (nextTile.getX() > tilePos.getX())
				{
					lookingRight = true;
				}
				else if (nextTile.getX() < tilePos.getX())
				{
					lookingRight = false;
				}

				Attack();
			}
		}
		break;
	case AIR2:
		//Vertical
		if (hoverCooldown.ReadMSec() >= 250)
		{
			int posY = Engine::GetInstance().map->MapToWorld(tilePos.getX(), tilePos.getY()).getY();

			float baseY;

			if (pathfinding->IsWalkable(tilePos.getX(), tilePos.getY() + 4))
			{
				// Get world Y of the tile BELOW the demon
				baseY = posY + Engine::GetInstance().map->GetTileHeight() * 4; // tiles down
			}
			else
			{
				// Get world Y of the tile OVER the demon
				baseY = posY - Engine::GetInstance().map->GetTileHeight() * 4; // tiles above
			}

			// Floating offset
			float offset = sin(hoverTimer.ReadSec() * hoverSpeed) * hoverAmplitude;

			// Final target
			float targetY = baseY + offset;


			int x, y;
			pbody->GetPosition(x, y);

			// Smoothly move toward target 
			velocity.y += (targetY - y) * 0.1f;
			velocity.y = SDL_clamp(velocity.y, -3.0f, 3.0f);

			hoverCooldown.Start();
		}



		//Horitzontal
		if (playerTileDist >= (attackTileRange + (texW / (Engine::GetInstance().map->GetTileWidth() * 2))) && startedAttacking == false)
		{
			anims.SetCurrent("air");

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
		}
		else
		{
			if (attackCooldown.ReadMSec() >= attackCooldownTime)
			{
				//Look towards player
				if (pathfinding->pathTiles.back() == tilePos)
				{
					pathfinding->pathTiles.pop_back();
					if (pathfinding->pathTiles.empty()) { return; }
				}

				Vector2D nextTile = pathfinding->pathTiles.back();

				if (nextTile.getX() > tilePos.getX())
				{
					lookingRight = true;
				}
				else if (nextTile.getX() < tilePos.getX())
				{
					lookingRight = false;
				}

				Attack();
			}
		}

		break;
	case MIXED2:
		break;
	}
}

void GwellBoss::Knockback()
{
	if (isdead) return;

	if (isKnockedback)
	{
		isAttacking = false;
		//anims.SetCurrent("hurt"); //TO DO UNCOMENT
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
	case GROUND2:
		b2Vec2 currentVel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
		Engine::GetInstance().physics->SetLinearVelocity(pbody, { velocity.x, currentVel.y });
		break;
	case AIR2:
		Engine::GetInstance().physics->SetLinearVelocity(pbody, { velocity.x, velocity.y });
		break;
	case MIXED2:
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
	if (lookingRight) // Invertido
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

		Engine::GetInstance().render->DrawRotatedTexture(texture, x, y - animFrame.h / 2, &animFrame, sdlFlip, 2);

		Engine::GetInstance().render->SetColorMod(texture, nullptr, nullptr, nullptr, *r, *g, *b);
		delete r; delete g; delete b;
	}
	else
	{
		Engine::GetInstance().render->DrawRotatedTexture(texture, x, y - animFrame.h / 2, &animFrame, sdlFlip, 2);
	}
}

void GwellBoss::Attack()
{
	switch (currentPhase)
	{
	case GwellBossPhase::GROUND2:
		if (isAttacking == false) 
		{
			if (startedAttacking == false)
			{
				anims.GetAnim(currentAttackAnim)->SetLoop(false);
				anims.SetCurrent(currentAttackAnim);
				attackWindUp.Start();
				startedAttacking = true;
			}

			// CreateAttack
			if (attackWindUp.ReadMSec() >= attackWindupTime) 
			{
				isAttacking = true;

				// Delete Hitbox (In case it didn't correctly delete)
				if (attackHitbox != nullptr) {
					b2DestroyBody(attackHitbox->body);
					attackHitbox = nullptr;
				}

				// AttackSize
				int attW = 0, attH = 0, hX = 0, hY = 0;
				if (currentAttack == 1)
				{
					attW = 150; attH = texH;
					hX = lookingRight ? position.getX() + texW / 2 + attW / 2 : position.getX() - texW / 2 - attW / 2;
					hY = position.getY();
				}
				else if (currentAttack == 2)
				{
					attW = 300; attH = 100;
					hX = lookingRight ? position.getX() + texW / 2 + attW / 2 : position.getX() - texW / 2 - attW / 2;
					hY = position.getY() + texH/2 - attH/2;
				}
				else if (currentAttack == 3)
				{
					Player* player = Engine::GetInstance().entityManager->GetPlayer();
					Vector2D playerPos = player->GetPosition();

					attW = 300; attH = 100;
					hX = playerPos.getX();
					hY = position.getY() + texH / 2 - attH / 2;
				}


				// Create attackHitbox
				attackHitbox = Engine::GetInstance().physics->CreateRectangleSensor(hX, hY, attW, attH, bodyType::KINEMATIC);
				attackHitbox->listener = this;
				attackHitbox->ctype = ColliderType::ENEMY_ATTACK;
				}
		}
		else
		{
			// Attack End
			if (anims.GetAnim(currentAttackAnim)->HasFinishedOnce())
			{
				isAttacking = false;
				anims.SetCurrent("idle");

				// Delete Hitbox
				if (attackHitbox != nullptr) {
					b2DestroyBody(attackHitbox->body);
					attackHitbox = nullptr;
				}

				attackCooldown.Start();
				startedAttacking = false;
				nextAttackSelected = false;
			}
		}

		
		break;
	case GwellBossPhase::AIR2:
		if (isAttacking == false)
		{
			if (startedAttacking == false)
			{
				anims.GetAnim(currentAttackAnim)->SetLoop(false);
				anims.SetCurrent(currentAttackAnim);
				attackWindUp.Start();
				startedAttacking = true;
			}

			// CreateAttack
			if (attackWindUp.ReadMSec() >= attackWindupTime)
			{
				isAttacking = true;

				if (currentAttack == 1)
				{
					// Create Projectiles
					for (int i = 0; i < 3; i++)
					{
						/*std::shared_ptr<GwellBossProjectile> projectile = std::dynamic_pointer_cast<GwellBossProjectile>(Engine::GetInstance().entityManager->CreateEntity(EntityType::GWELL_BOSS_PROJECTILE));
						Vector2D pos; pos.setX(lookingRight ? position.getX() + texW / 2 - 64 * (1 - i) : position.getX() - texW / 2 + 64 * (1 - i));
						pos.setY(position.getY() + 64 * (1 - i));
						projectile->position = pos;
						projectile->Start();*/
					}

				}
				else if (currentAttack == 2)
				{
					pbody->ctype = ColliderType::ENEMY_ATTACK;
					//TO DO Make it Dive
				}
			}
		}
		else
		{
			// Attack End
			if (anims.GetAnim(currentAttackAnim)->HasFinishedOnce())
			{
				isAttacking = false;
				anims.SetCurrent("air");

				if (currentAttack == 2)
				{
					pbody->ctype = ColliderType::ENEMY;
				}

				attackCooldown.Start();
				startedAttacking = false;
				nextAttackSelected = false;
			}
		}
		break;
	case GwellBossPhase::MIXED2:

		break;
	}
}

void GwellBoss::SelectAttack()
{
	if (nextAttackSelected)
	{
		return;
	}
	switch (currentPhase)
	{
	case GwellBossPhase::GROUND2:
		currentAttack = GenerateRandomNumber(1, 3);
		switch (currentAttack)			// TO DO Adjust COOLDOWN / WINDUP / DAMAGE
		{
		case 1: // Claw
			damage = 20;
			attackCooldownTime = 500.0f;
			attackWindupTime = 750.0f;
			attackTileRange = 1;
			currentAttackAnim = "claw";
			break;
		case 2: //Tail
			damage = 10;
			attackCooldownTime = 1000.0f;
			attackWindupTime = 750.0f;
			attackTileRange = 3;
			currentAttackAnim = "claw"; //TO DO CHANGE
			break;
		case 3: //Ground Spikes
			damage = 30;
			attackCooldownTime = 2000.0f;
			attackWindupTime = 1800.0f;
			attackTileRange = 5;
			currentAttackAnim = "stomp";
			break;
		}
		break;

	case GwellBossPhase::AIR2:
		currentAttack = GenerateRandomNumber(1, 1); // TODO RESTORE
		switch (currentAttack)			// TO DO Adjust COOLDOWN / WINDUP / DAMAGE
		{
		case 1: // Shoot
			damage = 0;
			attackCooldownTime = 250.0f;
			attackWindupTime = 100.0f;
			attackTileRange = 5;
			currentAttackAnim = "shoot";
			break;
		case 2: //Dive
			damage = 50;
			attackCooldownTime = 1000.0f;
			attackWindupTime = 500.0f;
			attackTileRange = 5;
			currentAttackAnim = "claw"; // TOOD CHANGE
			break;
		}
		break;
	case GwellBossPhase::MIXED2:

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

void GwellBoss::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) {
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
	return true;
}
