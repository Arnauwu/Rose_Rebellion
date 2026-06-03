#pragma once

#include "Entity.h"
#include "Timer.h"
#include "Animation.h"
#include <box2d/box2d.h>

class GwellProjectile : public Entity
{
public:
	GwellProjectile();
	virtual ~GwellProjectile();

	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;

	void OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) override;
	void Draw(float dt);
	Vector2D GetPosition();

	void Launch(Vector2D startPos, Vector2D directionVector); // Direction of the projectile

public:
	PhysBody* pbody = nullptr;
	SDL_Texture* texture = nullptr;

	float speed;
	Vector2D velocity;

	Timer lifeTimer;
	float lifeTimeMS;

private:
	AnimationSet anims;
};