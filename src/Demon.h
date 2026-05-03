#pragma once

#include "Enemy.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include "Pathfinding.h"

struct SDL_Texture;

class Demon : public Enemy
{
public:

	Demon();
	virtual ~Demon();
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

	void Attack();

	bool wasWalking = false;

public:

	//Declare enemy parameters
	bool isAttacking = false;
	Timer startAttack;

	Timer hoverTimer;
	Timer hoverCooldown;
	float hoverAmplitude = 4.0f;
	float hoverSpeed = 0.5f;

	//Sounds
	int morirDimoni;
	int atacarDimoni;
	int chocarDimoni;
	int volarDimoni;
};