#pragma once
#include "Item.h"
#include "Entity.h"
#include <SDL3/SDL.h>
#include <string>

struct SDL_Texture;

class SkillPointOrb : public Entity
{
public:

	SkillPointOrb();
	~SkillPointOrb();

	bool Awake();

	bool Start();

	bool Update(float dt);

	bool CleanUp();

	bool Destroy();

	void OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB);

public:

private:

	SDL_Texture* texture;
	const char* texturePath;
	int texW, texH;

	// Add a physics to an item
	PhysBody* pbody;
	std::string uniqueID;
};
