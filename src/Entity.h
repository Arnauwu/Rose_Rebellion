#pragma once

#include "Input.h"
#include "Render.h"

enum class EntityType
{
	PLAYER,
	ITEM,
	ENEMY,
	SPIDER,
	CUCAFERA,
	SWORD_KNIGHT,
	SAVEPOINT,
	UNKNOWN
};

class PhysBody;

class Entity : public std::enable_shared_from_this<Entity>
{
public:

	Entity() {}
	Entity(EntityType type) : type(type), active(true), zOrder(0) {} 

	virtual bool Awake() { return true;	}

	virtual bool Start() { return true;	}

	virtual bool Update(float dt) { return true; }

	virtual bool PostUpdate() { return true; }

	virtual bool CleanUp() { return true; }

	bool Destroy() 
	{
		active = false;
		pendingToDelete = true;
		return true; 
	}

	void Enable()
	{
		if (!active)
		{
			active = true;
			Start();
		}
	}

	void Disable()
	{
		if (active)
		{
			active = false;
			CleanUp();
		}
	}

	virtual void OnCollision(PhysBody* physA, PhysBody* physB) {

	};

	virtual void OnCollisionEnd(PhysBody* physA, PhysBody* physB) {

	};

public:

	std::string name;
	EntityType type;
	int zOrder = 0; // zOrder(0) To know when to draw it
	bool active = true;
	bool pendingToDelete = false;

	// Possible properties, it depends on how generic we
	// want our Entity class, maybe it's not renderable...
	Vector2D position;
	bool renderable = true;
	bool lookingRight = true; //False Left -- True Right 
};