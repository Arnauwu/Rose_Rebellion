#include "Projectile.h"
#include "Engine.h"
#include "Textures.h"
#include "Physics.h"
#include "Render.h"
#include "Log.h"
#include "Timer.h"
#include "ParticleManager.h"

Projectile::Projectile() : Entity(EntityType::PROJECTILE)
{
	name = "Projectile";
	pbody = nullptr;
	texture = nullptr;
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
	// Crear física
	pbody = Engine::GetInstance().physics->CreateCircleSensor(
		(int)position.getX(), (int)position.getY(), projectileRadius, bodyType::KINEMATIC);
	
	pbody->listener = this;
	pbody->ctype = ColliderType::PLAYER_ATTACK;

	lifeTimer.Start();
	
	return true;
}

bool Projectile::Update(float dt)
{
	if (hasHit || lifeTimer.ReadSec() >= lifeTime)
	{
		Destroy();
		return true;
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

	if (pbody != nullptr)
	{
		pbody->SetPosition((int)startX, (int)startY);
		Engine::GetInstance().physics->SetLinearVelocity(pbody, { dirX * speed, dirY * speed });
	}

	LOG("Projectile launched from (%.0f, %.0f) in direction (%.0f, %.0f)", startX, startY, dirX, dirY);
}

void Projectile::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
	// No colisionar con el player que disparó
	if (physB->ctype == ColliderType::PLAYER)
	{
		return;
	}

	// Destruir al golpear cualquier cosa (enemigo o pared)
	if (physB->ctype == ColliderType::ENEMY || 
		physB->ctype == ColliderType::MAP ||
		physB->ctype == ColliderType::SPECIALFLOOR)
	{
		hasHit = true;
		
		// Emitir partícula de impacto
		int ex, ey;
		physB->GetPosition(ex, ey);
		Engine::GetInstance().particleManager->EmitAttack((float)ex, (float)ey, true);
		
		LOG("Projectile hit!");
	}
}

void Projectile::Draw(float dt)
{
	if (pbody == nullptr) return;

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	// Dibujar como círculo debug
	SDL_Rect rect = { x - projectileRadius, y - projectileRadius, projectileRadius * 2, projectileRadius * 2 };
	Engine::GetInstance().render->DrawRectangle(rect, 255, 200, 100, 200, true, false);
}