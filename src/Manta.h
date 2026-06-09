#pragma once
#include "Item.h"
#include "Animation.h"

class Manta :public Item {
public:
	Manta();
	virtual ~Manta();

	bool Awake() override;
	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;

	void OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB);

private:
	SDL_Texture* texture = nullptr;
	SDL_Texture* interactionBackgroundTexture = nullptr;
	AnimationSet interactionBackgroundAnimation;
	PhysBody* pbody=nullptr;
	
	bool isPicked = false;
};
