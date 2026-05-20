#pragma once
#include "Enemy.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include "Pathfinding.h"

class Bat :public Enemy {
public:
	Bat();
	virtual ~Bat();
	
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
	int volarBat;
	int atacarBat;
	int morirBat;
};