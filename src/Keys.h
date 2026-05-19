#pragma once

#include"Item.h"
#include "Animation.h"
enum class KeyType {
	FOREST,
	MOUNTAIN,
	CATACUMBA,
	BOSS,
	CASTLE,
	NONE 
};

class Keys:public Item {
public:
	Keys();
	virtual ~Keys();

	bool Awake() override;
	bool Start() override;
	bool Update(float dt)override;
	bool CleanUp() override;
	
	void OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB);

	void SetKeyType(KeyType type);

	KeyType GetKeyType() const;
public:
	KeyType keyType = KeyType::NONE;
private:
	AnimationSet anims;
	SDL_Texture* texture = nullptr;
	PhysBody* pbody = nullptr;
};