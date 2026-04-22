#pragma once

#include "Entity.h"
#include <SDL3/SDL.h>
#include <string>

struct SDL_Texture;

class Item : public Entity
{
public:

	Item();
	virtual ~Item();

	bool Awake();

	bool Start();

	bool Update(float dt);

	bool CleanUp();

	bool Destroy();

	bool CheckIfCollected(); 

	void SetCollected();
public:

	bool isPicked = false;
	std::string uniqueID;
private:

	SDL_Texture* texture;
	const char* texturePath;
	int texW, texH;

	// Add a physics to an item
	PhysBody* pbody;
};
