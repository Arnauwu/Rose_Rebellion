#pragma once

#include "Module.h"
#include "Entity.h"
#include <list>

class Player;
class EntityManager : public Module
{
public:

	EntityManager();

	// Destructor
	virtual ~EntityManager();

	// Called before render is available
	bool Awake();

	// Called after Awake
	bool Start();

	// Called every frame
	bool Update(float dt);

	bool PostUpdate();

	// Called before quitting
	bool CleanUp();

	// Additional methods
	std::shared_ptr<Entity> CreateEntity(EntityType type);

	void DestroyEntity(std::shared_ptr<Entity> entity);

	void AddEntity(std::shared_ptr<Entity> entity);

	Player* GetPlayer() const { return playerPtr; }
	void SetPlayer(Player* p) { playerPtr = p; }

private:
	std::list<std::shared_ptr<Entity>> entities;
	Player* playerPtr = nullptr;
};
