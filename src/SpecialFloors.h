#pragma once

#include "Entity.h"
#include "Timer.h"
#include <SDL3/SDL.h>
#include <string>

enum class TypeFloor {
	NORMALFLOOR,
	BROKENFLOOR,
	VERTICALFLOOR,
	HORIZONTALFLOOR,
	CIRCULARFLOOR
};

struct SDL_Texture;

class SpecialFloor : public Entity
{
public:

	SpecialFloor();
	virtual ~SpecialFloor();

	bool Awake();
	bool Start();
	bool Update(float dt);
	bool CleanUp();

	// OnCollision function for the player. 
	void OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB);

	Vector2D GetPosition();
	void SetPosition(Vector2D pos);

	bool Destroy();

public:
	TypeFloor floorType = TypeFloor::NORMALFLOOR;

	int width = 32;
	int height = 32;

	// Movement Floor Variables
	Vector2D startPosition;
	int distance = 0;
	int moveSpeed = 0;
	int moveDirection = 1; // 1 = Right/Down, -1 = Left/Up
	b2Vec2 currentVel = { 0.0f, 0.0f };

	// Activation Variables
	bool activationOnTouch = false;
	bool isActivated = true;

	// Movement Limits
	float minMoveLimit = 0.0f;
	float maxMoveLimit = 0.0f;

	// Wait Time Variables for Movement Floors
	float waitTimeMax = 2000.0f;
	float currentWaitTime = 0.0f;
	bool isWaiting = false;

	// Breakable Floor Variables
	float breakTimeMax = 1000.0f; // Milliseconds = 1 second
	float currentBreakTime = 0.0f;
	bool isSteppedOn = false;

	// Respawn Variables
	bool isBroken = false;
	float respawnTimeMax = 3000.0f;
	float currentRespawnTime = 0.0f;
	
private:

	SDL_Texture* texture = nullptr;
	int texW, texH;
	PhysBody* pbody = nullptr;
};