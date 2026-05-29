#pragma once

#include "Enemy.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include "Timer.h"

struct SDL_Texture;

class ToxicBall : public Enemy
{
public:

	ToxicBall();
	virtual ~ToxicBall();
	bool Awake();
	bool Start();
	bool Update(float dt);
	void OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB);
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB);
	bool CleanUp() override;

private:
	void GetPhysicsValues() override;
	void Move() override;
	void Knockback() override;
	void ApplyPhysics() override;
	void Draw(float dt);


public:

	//Declare enemy parameters
	float jumpForce = 0.0f;
	float ballGravity = 48.0f;

	float jumpDistanceTiles = 0.0f;

	float initialX = 0.0f;
	float initialY = 0.0f;

	float jumpTimer = 0.0f;
	float waitTime = 2000.0f;

	//Sounds
	int chocarToxicBall;
};