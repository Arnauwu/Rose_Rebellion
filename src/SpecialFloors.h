#pragma once

#include "Entity.h"
#include <SDL3/SDL.h>
#include <string>

enum class TypeFloor {
	BROKENFLOOR,
	VERTICALFLOOR,
	HORIZONTALFLOOR
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
	bool PostUpdate();

	bool CleanUp();

	// OnCollision function for the player. 
	void OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB);
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB);

	Vector2D GetPosition();
	void SetPosition(Vector2D pos);

	bool Destroy();

private:

	void GetPhysicsValues();
	void Move();

	void ApplyPhysics();
	void Draw(float dt);

public:

	float speed1 = 5.0f;
	float speed2 = 10.0f;
	float lifeBrokenFloor;

	bool limitFloorH;
	bool limitFloorV;
	
private:

	SDL_Texture* texture;
	int texW, texH;

	// Add a physics to an item
	PhysBody* pbody;
};