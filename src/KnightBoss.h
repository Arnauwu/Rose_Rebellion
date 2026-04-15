#pragma once

#include "Enemy.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include "Pathfinding.h"

struct SDL_Texture;

class KnightBoss : public Enemy
{
public:

	KnightBoss();
	virtual ~KnightBoss();
	bool Awake();
	bool Start();
	bool Update(float dt);
	bool CleanUp();
	void OnCollision(PhysBody* physA, PhysBody* physB);
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB);

private:
	void PerformPathfinding();
	void GetPhysicsValues() override;
	void Move() override;
	void Knockback() override;
	void ApplyPhysics() override;
	void Draw(float dt);

	void SwordAttack();
	void ShieldDash();

public:

	PhysBody* swordHitbox = nullptr;

	// Boss parameters
	bool isAttacking = false;
	Timer startAttack;
	float attackCooldown = 1000.0f; // Time between attack

	bool isDashing = false;
	Timer dashTimer;
	float dashCooldown = 600.0f;

	// NUEVO: Variables del Combo Fijo
	int attackStep = 0;         // 0 = Ataque 1, 1 = Ataque 2, 2 = Embestida
	bool isResting = false;     // Si está cansado
	Timer restTimer;            // Reloj para contar los 3 segundos
	float restDuration = 3000.0f;

	int deathSoundId;
};