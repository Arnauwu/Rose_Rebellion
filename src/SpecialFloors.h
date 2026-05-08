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

	// Movement Floor Variables
	Vector2D startPosition;
	int distance = 0;
	int moveSpeed = 0;
	bool movingForward = true;

	// Breakable Floor Variables
	float breakTimeMax = 1000.0f; // Milliseconds
	float currentBreakTime = 0.0f;
	bool isSteppedOn = false;
	
private:

	SDL_Texture* texture = nullptr;
	int texW, texH;
	PhysBody* pbody = nullptr;
};