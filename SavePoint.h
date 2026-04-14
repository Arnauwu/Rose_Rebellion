#pragma once
#include"Entity.h"
#include"Animation.h"
#include<SDL3/SDL.h>

struct SDL_Texture;

class SavePoint:public Entity {
public:
	SavePoint();
	virtual ~SavePoint();
	bool Awake();
	bool Start();
	bool Update(float dt);
	bool CleanUp();

	void Activate();

private:
	bool isActivated = false;
	SDL_Texture* texture = NULL;
	PhysBody* pbody;
	AnimationSet anims;
};

