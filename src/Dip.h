#pragma once

#include "Enemy.h"
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

	// Ataques específicos de Dip
	void JumpAttack();
	void ClawAttack();
	void HowlAttack();

	bool wasWalking = false;

public:

	// Parámetros del enemigo inicializados
	bool isJumpingFwd = false;
	bool isJumpingBack = false;
	bool isClawing = false;
	bool isHowling = false;

	Timer startAttack;
	Timer actionTimer;

	// Para registrar la posición inicial del salto tipo zorro de nieve
	b2Vec2 initialJumpPos = { 0.0f, 0.0f };

	// SFX
	int morirDip = 0;
	int caminarDip = 0;
	int zarpazoDip = 0;
	int aullarDip = 0;
	int saltarDip = 0;
};