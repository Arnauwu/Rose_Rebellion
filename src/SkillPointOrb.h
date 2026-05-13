#pragma once
#include "Item.h"
#include "Entity.h"
#include <SDL3/SDL.h>
#include <string>

class SkillPointOrb : public Entity
{
public:

	SkillPointOrb();
	~SkillPointOrb();

	bool Awake() override;
	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;
	bool Destroy() ;

	void OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) override;

private:

	SDL_Texture* texture;
	int texW, texH;

	// Add a physics to an item
	PhysBody* pbody;
	std::string uniqueID;
};
