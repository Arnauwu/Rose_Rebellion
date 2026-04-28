#pragma once

#include"Item.h"

class Keys:public Item {
public:
	Keys();
	virtual ~Keys();

	bool Awake() override;
	bool Start() override;
	bool Update(float dt)override;
	bool CleanUp() override;
	
	void OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB);
private:
	SDL_Texture* texture = nullptr;
	PhysBody* pbody = nullptr;
};