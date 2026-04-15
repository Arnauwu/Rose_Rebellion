#pragma once

#include "src/Enemy.h"
#include "src/Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include "src/Pathfinding.h"

struct SDL_Texture;

class KnightBoss : public Enemy
{
public:

	KnightBoss();
	virtual ~KnightBoss();
	bool Awake();
	bool Start();
	bool Update(float dt);
	void OnCollision(PhysBody* physA, PhysBody* physB);
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB);

private:
	void PerformPathfinding();
	void GetPhysicsValues() override;
	void Move() override;
	void Knockback() override;
	void ApplyPhysics() override;
	void Draw(float dt);

	void BossAttack();

public:

	// Boss parameters
	bool isAttacking = false;
	Timer startAttack;
	float attackCooldown = 1000.0f; // Time between attack

	int deathSoundId;
};