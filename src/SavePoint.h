#pragma once
#include "Entity.h"
#include "Animation.h"
#include <SDL3/SDL.h>

struct SDL_Texture;

class SavePoint :public Entity {
public:
	SavePoint();
	virtual ~SavePoint();
	bool Awake();
	bool Start();
	bool Update(float dt);
	bool CleanUp();

	void OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) override;
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) override;

	void Activate();

private:

	SDL_Texture* texture = nullptr;
	SDL_Texture* interactIcon = nullptr;
	SDL_Texture* glowTex = nullptr;
	PhysBody* pbody;
	int texW, texH;
	AnimationSet anims;
	float iconTimer = 0.0f;

	bool isActivated = false;
	bool isActivating = false;
	bool isPlayerInRange = false;
};

