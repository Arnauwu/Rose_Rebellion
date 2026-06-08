#pragma once

#include "Entity.h"
#include "Timer.h"
#include "Animation.h"
#include <box2d/box2d.h>

enum class ProjectileState {
	IDA,
	ESPERA,
	RETORNO
};

struct SDL_Texture;

class Projectile : public Entity
{
public:
	Projectile();
	virtual ~Projectile();

	bool Awake() override;
	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;

	void OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) override;

	void Launch(float startX, float startY, float dirX, float dirY);

private:
	void Draw(float dt);

	PhysBody* pbody = nullptr;
	SDL_Texture* texture = nullptr;

	AnimationSet anims;
	bool lookingRight = true;

	float speed = 20.0f;
	float lifeTime = 5.0f;
	Timer lifeTimer;

	int projectileRadius = 30;
	bool hasHit = false;

	ProjectileState currentState = ProjectileState::IDA;
	Vector2D startPosition;
	float maxRange = 500.0f;        
	Timer hoverTimer;
	float hoverDurationMS = 400.0f;
};