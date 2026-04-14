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
	SHIELD_KNIGHT,
	FLYING_ENEMY,
	ENEMY_PROJECTILE,
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

	void TakeDamage(int damage)
	{
		if (godMode || isdead) return;

		currentHealth -= damage;

		if (currentHealth <= 0) {
			currentHealth = 0;
			isdead = true;
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


	// Possible properties, it depends on how generic we want our Entity class
	Vector2D position;


	int maxHealth;
	int currentHealth;

	int damage;

	//Death variables
	bool isdead = false;

	// GodMode
	bool godMode = false;

	// Draw
	bool renderable = true;
	bool lookingRight = true; //False Left -- True Right 
};