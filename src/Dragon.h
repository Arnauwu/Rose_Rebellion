#pragma once

#include "Enemy.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include "Pathfinding.h"

struct SDL_Texture;

enum DragonPhase { GROUND, AIR};

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

	void SelectAttack();
	int GenerateRandomNumber(int minNumber, int maxNumber);
public:

	PhysBody* attackHitbox = nullptr;

	// Boss parameters
	bool nextAttackSelected = false;
	bool startedAttacking = false;
	bool isAttacking = false;

	int currentAttack = 0;

	Timer attackCooldown;
	Timer attackWindUp;
	
	//TO DO ADJUST VALUES DEPENDING ON ATTACK
	float attackWindupTime = 500.0f;
	float attackCooldownTime = 2000.0f;
	std::string currentAttackAnim = "";
	int attackTileRange = 0;

	//Dragon
	bool isInvincible = false; //Changing Phase
	DragonPhase currentPhase = DragonPhase::GROUND; // First Phase

	//Air
	Timer hoverTimer;
	Timer hoverCooldown;
	float hoverAmplitude = 4.0f;
	float hoverSpeed = 1.5f;


	// Sounds
	int deathSoundId;
};


/*
FASE 2: Aire
Proyectiles hacia el suelo (Volando te tira proyectiles a 90º hacia el suelo (se pueden devolver con ataque hacia arriba))
Salta hacia arriba y se tira hacia a ti de morro planeando (estilo ataque los zorros de nieve)

FASE 3: Tierra-Aire
Morir dramáticamente con una rosa de su sangre
*/

