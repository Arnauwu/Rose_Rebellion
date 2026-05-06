#pragma once
#include "Enemy.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include "Pathfinding.h"

enum class NinfaState{
	IDLE,
	CHASE,
	WINDUP,//Move 
	ATTACK,
	COOLDOWN
};

class Ninfa :public Enemy {
public:
	Ninfa();
	virtual ~Ninfa();
	
	bool Awake() override;
	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;

	void OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) override;
protected:
	void GetPhysicsValues() override;
	void Move() override;
	void Knockback() override;
	void ApplyPhysics()override;
	void Draw(float dt); 

	bool wasWalking = false;
private:
	void PerformPathfinding();
	void ShootProjectile();

public:
	NinfaState currentState;

	int volarNinfa;
	int morirNinfa;
	int atacarNinfa;

	float flapTimer = 0.0f;
	float timeBetweenFlaps = 4.0f; // Ajústalo a la velocidad de la animación de las alas
	bool wasFlying = false;

	float targetOffsetX = 0.0f;
	float targetOffsetY = 0.0f;
	float attackRange = 0.0f;

	Timer stateTimer;
	float windupDurationMs=0.0f;
	float cooldownDurationMs=0.0f;

	int volarNinfa;
	int atacarNinfa;
	int morirNinfa;
};