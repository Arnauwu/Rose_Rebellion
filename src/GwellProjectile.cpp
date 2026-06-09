#include "GwellProjectile.h"
#include <cmath>
#include <SDL3/SDL.h>

#include "Engine.h"
#include "Physics.h"
#include "Textures.h"
#include "SceneManager.h"
#include "EntityManager.h"

#include "Render.h"

#include "tracy/Tracy.hpp"

//PI
constexpr double PI = 3.14159265358979323846;

GwellProjectile::GwellProjectile() : Entity(EntityType::GWELL_PROJECTILE)
{
	name = "GwellProjectile";
	speed = 15.0f;
	damage = 15;
	lifeTimeMS = 5000.0f;
}

GwellProjectile::~GwellProjectile() {}

bool GwellProjectile::Start()
{
	std::unordered_map<int, std::string> aliases = { {0, "bullet"} };
	// TO DO: Usa la textura correcta para el proyectil tóxico
	anims.LoadFromTSX("Assets/Textures/Entities/Enemies/Guell/SS_Proyectil_Acido_Guell.tsx", aliases);
	anims.SetCurrent("bullet");

	texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/Guell/SS_Proyectil_Acido_Guell.png");

	pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX(), (int)position.getY(), 32, bodyType::DYNAMIC);
	pbody->listener = this;
	pbody->ctype = ColliderType::ENEMY_ATTACK;

	Engine::GetInstance().physics->SetGravityScale(pbody, 0.0f);
	lifeTimer.Start();

	return true;
}

void GwellProjectile::Launch(Vector2D startPos, Vector2D directionVector)
{
	position = startPos;
	velocity = directionVector.normalized() * speed;
	if (pbody) pbody->SetPosition((int)startPos.getX(), (int)startPos.getY());
}

bool GwellProjectile::Update(float dt)
{
	if (!active || pendingToDelete) return true;

	if (lifeTimer.ReadMSec() >= lifeTimeMS) {
		Destroy();
		return true;
	}

	Engine::GetInstance().physics->SetLinearVelocity(pbody, { velocity.getX(), velocity.getY() });
	Draw(dt);
	return true;
}

void GwellProjectile::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
	if (pendingToDelete) return;

	switch (physB->ctype)
	{
	case ColliderType::PLAYER:
		if (physB->listener != nullptr) physB->listener->TakeDamage(damage);
		Destroy();
		break;
	case ColliderType::PLAYER_ATTACK:
	case ColliderType::MAP:
		Destroy();
		break;
	}
}

void GwellProjectile::Draw(float dt)
{
	if (!Engine::GetInstance().sceneManager->isGamePaused) anims.Update(dt);
	const SDL_Rect& animFrame = anims.GetCurrentFrame();

	int x, y; pbody->GetPosition(x, y);
	position.setX((float)x); position.setY((float)y);

	double angle = std::atan2(velocity.getY(), velocity.getX()) * (180.0 / PI);

	Engine::GetInstance().render->DrawRotatedTexture(texture, x, y, &animFrame, SDL_FLIP_NONE, 1, angle, animFrame.w / 2, animFrame.h / 2);
}

bool GwellProjectile::CleanUp()
{
	active = false;
	if (texture) { Engine::GetInstance().textures->UnLoad(texture); texture = nullptr; }
	if (pbody) { Engine::GetInstance().physics->DeletePhysBody(pbody); pbody = nullptr; }
	return Entity::CleanUp();
}