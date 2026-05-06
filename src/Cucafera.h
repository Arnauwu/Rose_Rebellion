#pragma once

#include "Enemy.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include "Pathfinding.h"

struct SDL_Texture;

class Cucafera : public Enemy
{
public:

	Cucafera();
	virtual ~Cucafera();
	bool Awake();
	bool Start();
	bool Update(float dt);
	void OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB);
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB);

private:
	void PerformPathfinding();
	void GetPhysicsValues() override;
	void Move() override;
	void Knockback() override;
	void ApplyPhysics() override;
	void Draw(float dt);
	
	void RollAttack();

	bool wasWalking = false;

public:

	//Declare enemy parameters
	bool isRolling = false;
	Timer startAttack;

	int morirCucafera;
	int rodarCucafera;
	int chocarCucafera;
	int caminarCucafera;
};