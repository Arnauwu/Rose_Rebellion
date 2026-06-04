#pragma once

#include "Enemy.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include "Pathfinding.h"

struct SDL_Texture;

enum GwellBossPhase { GROUNDD, TRANSITION_TO_WALL, WALL_CLING };

class GwellBoss : public Enemy
{
public:

	GwellBoss();
	virtual ~GwellBoss();
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
	
	float attackWindupTime = 500.0f;
	float attackCooldownTime = 2000.0f;
	std::string currentAttackAnim = "";
	int attackTileRange = 0;

	//GwellBoss
	bool isInvincible = false; //Changing Phase
	GwellBossPhase currentPhase = GwellBossPhase::GROUNDD; // First Phase
	bool hasDoneWallPhase = false;

	float minFloorX = 0.0f;
	float maxFloorX = 0.0f;
	Vector2D targetWallPos;
	Vector2D leftWallClingPos;
	Vector2D rightWallClingPos;

	// Wall Mechanics
	int wallHitsTaken = 0;
	int maxWallHits = 3;
	int wallDirection = 1; // 1 Right, -1 Left

	int clawStep = 0; // 0: Inactive, 1: First attack, 2: Second attack

	// Sounds
	int deathSoundId;
};
