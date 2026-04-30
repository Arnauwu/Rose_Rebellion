#pragma once

#include "Enemy.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include "Pathfinding.h"

struct SDL_Texture;

class ShieldKnight : public Enemy
{
public:

	ShieldKnight();
	virtual ~ShieldKnight();
	bool Awake();
	bool CleanUp() override;
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
	bool hitFromRight = false;
	Timer startAttack; // Windup
	Timer attackDuration; // Hitbox Duration
	Timer attackCooldown;

	PhysBody* attackHitbox = nullptr;

	int morirEscudo;
	int atacarEscudo;
	int caminarEscudo;
};