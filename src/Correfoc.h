#pragma once

#include "Enemy.h"

struct SDL_Texture;

class Correfoc : public Enemy
{
public:

	Correfoc();
	virtual ~Correfoc();
	bool Awake();
	bool Start();
	bool Update(float dt);
	void OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB);
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB);

private:
	void PerformPathfinding();
	void GetPhysicsValues() override;
	void Move() override;
	void Knockback() override;
	void ApplyPhysics() override;
	void Draw(float dt);

	void Explode();

	bool wasWalking = false;

public:

	//Declare enemy parameters
	bool isExploding = false;
	bool explosionCreated = false;
	Timer startExplosion;
	Timer explosionDuration;

	int morirCorrefoc;
	int caminarCorrefoc;
	int explotarCorefoc;
};