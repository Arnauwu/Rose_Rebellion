#pragma once

#include "Entity.h"
#include <SDL3/SDL.h>

struct SDL_Texture;

class HealthOrb : public Entity
{
public:

	HealthOrb();
	~HealthOrb();

	bool Awake();

	bool Start();

	bool Update(float dt);

	bool CleanUp();

	bool Destroy();

public:

private:

	SDL_Texture* texture;
	const char* texturePath;
	int texW, texH;

	// Add a physics to an item
	PhysBody* pbody;
};
