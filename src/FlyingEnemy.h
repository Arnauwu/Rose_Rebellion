#pragma once
#include "Enemy.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include "Pathfinding.h"

enum class FlyingEnemyState{
	IDLE,
	CHASE,
	WINDUP,//Move anims
	ATTACK,
	COOLDOWN
};

class FlyingEnemy :public Enemy {
public:
	FlyingEnemy();
	virtual ~FlyingEnemy();
	
	bool Awake() override;
	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;

protected:
	void GetPhysicsValues() override;
	void Move() override;
	void Knockback() override;
	void ApplyPhysics()override;
	void Draw(float dt); 

private:
	void ShootProjectile();

public:
	FlyingEnemyState currentState;

	float targetOffsetX = 0.0f;
	float targetOffsetY = 0.0f;
	float attackRange = 0.0f;

	Timer stateTimer;
	float windupDurationMs=0.0f;
	float cooldownDurationMs=0.0f;

};