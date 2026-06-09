#pragma once
#include "Enemy.h"
#include "Animation.h" 
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include "Pathfinding.h"
#include "Timer.h" 

struct SDL_Texture;

class Dip : public Enemy
{
public:
	Dip();
	virtual ~Dip();
	bool Awake();
	bool Start();
	bool Update(float dt);
	void OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) override;
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) override;

private:
	void PerformPathfinding();
	void GetPhysicsValues() override;
	void Move() override;
	void Knockback() override;
	void ApplyPhysics() override;
	void Draw(float dt);

	void AttackPlayer();
	void ExecuteSpecialAttack(Vector2D playerPos);
	void FinishSpecialAttack();
	void BeginHit();
	void BeginDeath();
	bool HasLandedAfterSpecialJump() const;

	bool isTouchingPlayer = false;
	bool deathStarted = false;
	int deathDrawX = 0;
	int deathDrawY = 0;
	int playerContacts = 0; // Contador para las m??ltiples colisiones del jugador
	int groundContacts = 0;
	bool leftGroundDuringSpecial = false;
	bool hasImpactDrawY = false;
	int impactDrawY = 0;
	Timer patrolTimer; // Temporizador para controlar la patrulla
public:
	Timer startAttack;
	int attackDamage = 5;
	bool wasWalking = false;


	Timer attackVisualTimer;
	bool isAttackingVisual = false;

	Timer specialAttackTimer; // Temporizador global de 6 segundos
	Timer phaseTimer;         // Temporizador para cada fase del salto
	bool isSpecialAttacking = false;
	int specialPhase = 0;     // 1 = jump back, 2 = dive, 3 = impact, 4 = return
	Vector2D leapStartPos;    // Posici??n inicial antes del ataque
	Vector2D lockedPlayerPos; // Posici??n del jugador bloqueada en el momento del salto

	int morirDip = 0;
	int caminarDip = 0;
	int zarpazoDip = 0;
	int aullarDip = 0;
	int saltarDip = 0;
};
