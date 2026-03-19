#pragma once

#include "Enemy.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include "Pathfinding.h"

struct SDL_Texture;

class Test : public Enemy
{
public:

	Test();
	virtual ~Test();
	bool Awake();
	bool Start();
	bool Update(float dt);
	void OnCollision(PhysBody* physA, PhysBody* physB);
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB);

private:
	void PerformPathfinding();
	void GetPhysicsValues() override;
	void Move() override;
	void ApplyPhysics() override;
	void Draw(float dt);
	Vector2D GetTilePos();

public:

	//Declare enemy parameters
	bool onGround = false;
	int splashSoundId;
};