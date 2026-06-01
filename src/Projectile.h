#pragma once

#include "Entity.h"
#include "Timer.h"
#include <box2d/box2d.h>

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

	PhysBody* pbody;
	SDL_Texture* texture;
	
	float speed = 20.0f;
	float lifeTime = 3.0f;
	Timer lifeTimer;
	
	int projectileRadius = 15;
	bool hasHit = false;
};