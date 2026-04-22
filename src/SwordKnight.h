#pragma once

#include "Enemy.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include "Pathfinding.h"

struct SDL_Texture;

class SwordKnight : public Enemy
{
public:

	SwordKnight();
	virtual ~SwordKnight();
	bool Awake();
	bool CleanUp() override;
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

	void Attack();

public:

	//Declare enemy parameters
	bool isAttacking = false;
	Timer startAttack; // Windup
	Timer attackDuration; // Hitbox Duration
	Timer attackCooldown;

	PhysBody* attackHitbox = nullptr;

	int morirEspada;
	int atacarEspada;
	int caminarEspada;
};