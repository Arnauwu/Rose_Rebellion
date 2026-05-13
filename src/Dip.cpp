#include "Dip.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "SceneManager.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"
#include "ParticleManager.h"
#include "Map.h"
#include <cmath> // Para sqrt()

Dip::Dip() : Enemy(EntityType::UNKNOWN) // Reemplaza UNKNOWN por EntityType::DIP
{
	name = "Dip";
}

Dip::~Dip() {}

bool Dip::Awake()
{
	return true;
}

bool Dip::Start()
{
	//std::unordered_map<int, std::string> aliases = {
	//	{0, "idle"},
	//	{4, "walk"},
	//	{8, "jumpFwd"},
	//	{12, "jumpBack"},
	//	{16, "claw"},
	//	{20, "howl"},
	//	{24, "dead"},
	//	{28, "hurt"}
	//};
	//anims.LoadFromTSX("Assets/Textures/Entities/Enemies/Dip/Dip.tsx", aliases);
	//anims.SetCurrent("idle");

	//texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/Dip/Dip.png");
	if (Engine::GetInstance().physics->GetDebug())
	{
		pathfinding->DrawPath();
	}

	texW = 64;
	texH = 64;
	pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX() + texW / 2, (int)position.getY() + texH / 2, (texW * 2) / 5, bodyType::DYNAMIC);
	pbody->listener = this;
	pbody->ctype = ColliderType::ENEMY;

	pathfinding = std::make_shared<Pathfinding>(true);
	pathfinding->ResetPath(GetTilePos());
	pathFindingCooldown.Start();

	vision = 10;
	speed = 3.0f;
	knockbackForce = 5.0f;
	maxHealth = 20;
	currentHealth = 20;
	velocity = { 0.0f, 0.0f }; // Inicializamos velocidad de Enemy

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	startAttack.Start();

	return true;
}

bool Dip::Update(float dt)
{
	if (!active) return true;

	if (Engine::GetInstance().sceneManager->isGamePaused == false && isdead == false)
	{
		if (pathFindingCooldown.ReadMSec() > 500 && !isJumpingFwd && !isJumpingBack && !isClawing && !isHowling)
		{
			PerformPathfinding();
			pathFindingCooldown.Start();
		}

		GetPhysicsValues();

		if (!isJumpingFwd && !isJumpingBack && !isClawing && !isHowling && !isKnockedback)
		{
			Move();

			Player* player = Engine::GetInstance().entityManager->GetPlayer();
			if (player != nullptr && startAttack.ReadSec() > 3.0f)
			{
				// Usar distanceSquared y sqrt (adaptado a tu motor)
				float dist = sqrt(position.distanceSquared(player->GetPosition()));

				if (dist < 80.0f) {
					isHowling = true;
					actionTimer.Start();
					anims.SetCurrent("howl");
				}
				else if (dist < 150.0f) {
					isJumpingFwd = true;
					initialJumpPos = { position.getX(), position.getY() }; // Sintaxis Box2D 3.x
					actionTimer.Start();
					anims.SetCurrent("jumpFwd");
				}
				startAttack.Start();
			}
		}
		else
		{
			if (isJumpingFwd || isJumpingBack) JumpAttack();
			if (isClawing) ClawAttack();
			if (isHowling) HowlAttack();
		}

		Knockback();
		ApplyPhysics();
	}

	if (isdead)
	{
		if (anims.GetCurrentName() != "dead")
		{
			Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0.0f, 0.0f });
			anims.GetAnim("dead")->SetLoop(false);
			anims.SetCurrent("dead");
			pbody->ctype = ColliderType::UNKNOWN;
			isKnockedback = false;

			std::shared_ptr<Entity> healthOrb = Engine::GetInstance().entityManager->CreateEntity(EntityType::HEALTH_ORB);
			healthOrb->position.setX(this->position.getX());
			healthOrb->position.setY(this->position.getY() - 100.0f);
			healthOrb->Start();
		}

		if (anims.GetAnim("dead")->HasFinishedOnce())
		{
			pendingToDelete = true;
		}
	}

	bool isWalking = (velocity.x != 0 && !isdead && !isJumpingFwd && !isJumpingBack && !isClawing && !isHowling && !isKnockedback);
	wasWalking = isWalking;

	Draw(dt);

	return true;
}

void Dip::PerformPathfinding()
{
	pathfinding->ResetPath(GetTilePos());
	Vector2D pos = GetPosition();
	Player* player = Engine::GetInstance().entityManager->GetPlayer();
	Vector2D playerPos = player->GetPosition();

	playerTileDist = sqrt(pos.distanceSquared(playerPos)) / 128.0f;
	int iter = 0;

	while (pathfinding->pathTiles.empty() && playerTileDist < vision && iter < MaxIterations)
	{
		pathfinding->PropagateAStar();
		iter++;
	}
}

void Dip::GetPhysicsValues()
{
	b2Vec2 vel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity = { vel.x, vel.y };
}

void Dip::Move()
{
	Vector2D tilePos = GetTilePos();

	if (pathfinding->pathTiles.empty() && !isKnockedback)
	{
		anims.SetCurrent("idle");
		velocity.x = 0;
		return;
	}
	else if (playerTileDist >= 2.0f && !isKnockedback)
	{
		if (anims.GetCurrentName() != "walk") anims.SetCurrent("walk");

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
}

void Dip::JumpAttack()
{
	Player* player = Engine::GetInstance().entityManager->GetPlayer();
	float jumpSpeedX = 5.0f;
	float jumpSpeedY = -6.0f;

	if (isJumpingFwd)
	{
		if (anims.GetCurrentName() != "jumpFwd") anims.SetCurrent("jumpFwd");

		float dirX = (player && player->GetPosition().getX() < position.getX()) ? -jumpSpeedX : jumpSpeedX;
		velocity.x = dirX;

		if (actionTimer.ReadSec() > 0.5f) {
			isJumpingFwd = false;
			isJumpingBack = true;
			actionTimer.Start();
			anims.SetCurrent("jumpBack");
		}
	}
	else if (isJumpingBack)
	{
		if (anims.GetCurrentName() != "jumpBack") anims.SetCurrent("jumpBack");

		float dirX = (initialJumpPos.x < position.getX()) ? -jumpSpeedX : jumpSpeedX;
		velocity.x = dirX;

		if (actionTimer.ReadSec() > 0.5f) {
			isJumpingBack = false;
			startAttack.Start();
			anims.SetCurrent("idle");
		}
	}
}

void Dip::ClawAttack()
{
	if (anims.GetCurrentName() != "claw") anims.SetCurrent("claw");
	velocity.x = 0.0f;

	if (anims.GetAnim("claw")->HasFinishedOnce()) {
		isClawing = false;
		anims.SetCurrent("idle");
	}
}

void Dip::HowlAttack()
{
	if (anims.GetCurrentName() != "howl") anims.SetCurrent("howl");
	velocity.x = 0.0f;

	Player* player = Engine::GetInstance().entityManager->GetPlayer();

	if (actionTimer.ReadSec() > 0.4f && actionTimer.ReadSec() < 0.5f)
	{
		if (player != nullptr)
		{
			float dist = sqrt(position.distanceSquared(player->GetPosition()));
			if (dist < 120.0f && dist > 0.0f) // Evitar división por cero
			{
				// Cálculo de vector de empuje manual en vez de Box2D (soluciona error de Normalize y b2Body_Apply...)
				float dx = player->GetPosition().getX() - position.getX();
				float dy = player->GetPosition().getY() - position.getY();

				float pushForce = 15.0f;
				float pushX = (dx / dist) * pushForce;

				// Aplicar knockback usando la función de tu engine para setear la velocidad
				Engine::GetInstance().physics->SetLinearVelocity(player->pbody, { pushX, -5.0f });
			}
		}
	}

	if (anims.GetAnim("howl")->HasFinishedOnce()) {
		isHowling = false;
		anims.SetCurrent("idle");
	}
}

void Dip::Knockback()
{
	if (isdead) return;

	if (isKnockedback)
	{
		isJumpingFwd = false;
		isJumpingBack = false;
		isClawing = false;
		isHowling = false;

		if (anims.GetCurrentName() != "hurt") anims.SetCurrent("hurt");

		if (lookingRight) { velocity.x = knockbackForce; }
		else { velocity.x = -knockbackForce; }
	}

	if (knockbackTime <= 0)
	{
		isKnockedback = false;
		knockbackTime = 500.0f;
		anims.SetCurrent("idle");
	}
	else
	{
		knockbackTime -= Engine::GetInstance().GetDt() * 1000.0f;
	}
}

void Dip::ApplyPhysics()
{
	b2Vec2 currentVel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	Engine::GetInstance().physics->SetLinearVelocity(pbody, { velocity.x, currentVel.y });
}

void Dip::Draw(float dt)
{
	if (Engine::GetInstance().sceneManager->isGamePaused == false)
	{
		anims.Update(dt);
	}
	const SDL_Rect& animFrame = anims.GetCurrentFrame();

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

	if (isKnockedback)
	{
		Uint8 r = 255, g = 25, b = 25; // Solucionado: No instanciar en heap con new Uint8 para setear colores fijos (evita leaks)
		Engine::GetInstance().render->SetColorMod(texture, &r, &g, &b, 255, 25, 25);

		Engine::GetInstance().render->DrawRotatedTexture(texture, x, y - animFrame.h / 3, &animFrame, sdlFlip, 1);

		// Volver a la normalidad
		Uint8 defR = 255, defG = 255, defB = 255;
		Engine::GetInstance().render->SetColorMod(texture, nullptr, nullptr, nullptr, defR, defG, defB);
	}
	else
	{
		Engine::GetInstance().render->DrawRotatedTexture(texture, x, y - animFrame.h / 3, &animFrame, sdlFlip, 1);
	}
}

void Dip::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
	switch (physB->ctype)
	{
	case ColliderType::MAP:
	case ColliderType::PLAYER:
	case ColliderType::ENEMY:
		break;

	case ColliderType::PLAYER_ATTACK:
		TakeDamage(physB->listener->damage);
		isKnockedback = true;
		Engine::GetInstance().particleManager->EmitHitSparks(position.getX(), position.getY(), false);
		break;

	default:
		break;
	}
}

void Dip::OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
}