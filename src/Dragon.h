#pragma once

#include "Enemy.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include "Pathfinding.h"

struct SDL_Texture;

enum DragonPhase { GROUND, AIR, MIXED };

class Dragon : public Enemy
{
public:

	Dragon();
	virtual ~Dragon();
	bool Awake();
	bool Start();
	bool Update(float dt);
	bool CleanUp();
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

public:

	PhysBody* attackHitbox = nullptr;

	// Boss parameters
	bool isAttacking = false;
	Timer startAttack;
	float attackCooldown = 1000.0f; // Time between attack

	//Dragon
	bool isInvincible = false; //Changing Phase
	DragonPhase currentPhase = DragonPhase::GROUND; // First Phase

	// Sounds
	int deathSoundId;
};