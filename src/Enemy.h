#pragma once

#include "Entity.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include "Pathfinding.h"

#include "Timer.h"

#define MaxIterations 90

struct SDL_Texture;

class Enemy : public Entity
{
public:

	Enemy(EntityType type) : Entity(type) {};
	virtual ~Enemy() {};
	bool Update(float dt) override;
	bool CleanUp() override;
	void OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) override;
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) override;
	void SetPosition(Vector2D pos);
	Vector2D GetPosition();
	bool Destroy();

protected:
	void PerformPathfinding();
	virtual void GetPhysicsValues() = 0;
	virtual void Move() = 0;
	virtual void Knockback() = 0;
	virtual void ApplyPhysics() = 0;
	void Draw(float dt);
	Vector2D GetTilePos();

public:

	//Declare enemy parameters
	int vision; //Max tile distance
	float speed;
	int playerTileDist = 999;
	float knockbackForce;
	bool isKnockedback = false;
	float knockbackTime = 500.0f;

	SDL_Texture* texture = NULL;
	int texW, texH;
	PhysBody* pbody;

protected:
	b2Vec2 velocity;
	AnimationSet anims;
	std::shared_ptr<Pathfinding> pathfinding;
	Timer pathFindingCooldown;
};