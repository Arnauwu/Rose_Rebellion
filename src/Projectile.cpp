#include "Projectile.h"
#include "Engine.h"
#include "Textures.h"
#include "Physics.h"
#include "Render.h"
#include "Log.h"
#include "Timer.h"
#include "ParticleManager.h"
#include "EntityManager.h"
#include "Player.h" 
#include "SceneManager.h" 
#include <cmath>
#include <unordered_map>  

Projectile::Projectile() : Entity(EntityType::PROJECTILE)
{
	name = "Projectile";
}

Projectile::~Projectile()
{
}

bool Projectile::Awake()
{
	return true;
}

bool Projectile::Start()
{
	std::unordered_map<int, std::string> aliases = {
		{0, "spin"}
	};

	anims.LoadFromTSX("Assets/Textures/Entities/Princess/HozProjectile.tsx", aliases);
	anims.SetCurrent("spin");

	texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Princess/HozProjectile.png");

	pbody = Engine::GetInstance().physics->CreateCircleSensor(
		(int)position.getX(), (int)position.getY(), projectileRadius, bodyType::KINEMATIC);

	pbody->listener = this;
	pbody->ctype = ColliderType::PLAYER_PROJECTILE;

	lifeTimer.Start();
	currentState = ProjectileState::IDA;

	this->damage = 20;

	return true;
}

bool Projectile::Update(float dt)
{
	if (hasHit || lifeTimer.ReadSec() >= lifeTime)
	{
		pendingToDelete = true;
		return true;
	}

	switch (currentState)
	{
	case ProjectileState::IDA:
	{
		float dx = position.getX() - startPosition.getX();
		float dy = position.getY() - startPosition.getY();
		float distanceSq = (dx * dx) + (dy * dy);

		if (distanceSq >= (maxRange * maxRange))
		{
			currentState = ProjectileState::ESPERA;
			Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0.0f, 0.0f });
			hoverTimer.Start();
		}
		break;
	}
	case ProjectileState::ESPERA:
	{
		if (hoverTimer.ReadMSec() >= hoverDurationMS)
		{
			currentState = ProjectileState::RETORNO;
		}
		break;
	}
	case ProjectileState::RETORNO:
	{
		Player* player = (Player*)Engine::GetInstance().entityManager->GetPlayer();
		if (player != nullptr && player->pbody != nullptr)
		{
			int pX, pY;
			player->pbody->GetPosition(pX, pY);

			float dirX = (float)pX - position.getX();
			float dirY = (float)pY - position.getY();

			float length = std::sqrt((dirX * dirX) + (dirY * dirY));
			if (length > 0)
			{
				dirX /= length;
				dirY /= length;

			}

			Engine::GetInstance().physics->SetLinearVelocity(pbody, { dirX * speed, dirY * speed });
		}
		else
		{
			pendingToDelete = true;
		}
		break;
	}
	}

	Draw(dt);
	return true;
}

bool Projectile::CleanUp()
{
	if (pbody != nullptr)
	{
		Engine::GetInstance().physics->DeletePhysBody(pbody);
		pbody = nullptr;
	}
	if (texture != nullptr)
	{
		Engine::GetInstance().textures->UnLoad(texture);
		texture = nullptr;
	}
	return true;
}

void Projectile::Launch(float startX, float startY, float dirX, float dirY)
{
	position.setX(startX);
	position.setY(startY);
	startPosition = position;

	lookingRight = (dirX >= 0);

	if (pbody != nullptr)
	{
		pbody->SetPosition((int)startX, (int)startY);
		Engine::GetInstance().physics->SetLinearVelocity(pbody, { dirX * speed, dirY * speed });
	}

	LOG("Projectile launched from (%.0f, %.0f) in direction (%.0f, %.0f)", startX, startY, dirX, dirY);
}

void Projectile::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
	if (physB->ctype == ColliderType::PLAYER)
	{
		if (currentState == ProjectileState::RETORNO)
		{
			hasHit = true;
		}
		return;
	}

	if (physB->ctype == ColliderType::ENEMY)
	{
		int ex, ey;
		physB->GetPosition(ex, ey);
		Engine::GetInstance().particleManager->EmitAttack((float)ex, (float)ey, true);

		LOG("Projectile hit ENEMY (Piercing)!");
	}
	else if (physB->ctype == ColliderType::MAP || physB->ctype == ColliderType::SPECIALFLOOR)
	{
		hasHit = true;

		int px, py;
		pbody->GetPosition(px, py);
		Engine::GetInstance().particleManager->EmitAttack((float)px, (float)py, true);

		LOG("Projectile hit OBSTACLE and was destroyed!");
	}
}

void Projectile::Draw(float dt)
{
	if (pbody == nullptr) return;

	if (Engine::GetInstance().sceneManager->isGamePaused == false) {
		anims.Update(dt);
	}
	const SDL_Rect& animFrame = anims.GetCurrentFrame();

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	SDL_FlipMode sdlFlip = lookingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;

	Engine::GetInstance().render->DrawRotatedTexture(
		texture,
		x,
		y,
		&animFrame,
		sdlFlip,
		1.0f,
		0.0,
		animFrame.w / 2,
		animFrame.h / 2
	);
}