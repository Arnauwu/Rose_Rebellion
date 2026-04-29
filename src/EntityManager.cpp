#include "EntityManager.h"
#include "Player.h"
#include "Engine.h"
#include "Textures.h"
#include "SceneManager.h"
#include "Log.h"

#include "Item.h"
#include "Keys.h"
#include "Manta.h"
#include "Sickle.h"
#include "HealthOrb.h"
#include "SkillPointOrb.h"
#include "DashObj.h"
#include "DoubleJumpObj.h"

#include "SavePoint.h"

#include "SpiderEnemy.h"
#include "Cucafera.h"
#include "SwordKnight.h"
#include "ShieldKnight.h"
#include "Ninfa.h"

#include "KnightBoss.h"

EntityManager::EntityManager() : Module()
{
	name = "entitymanager";
}

// Destructor
EntityManager::~EntityManager()
{}

// Called before render is available
bool EntityManager::Awake()
{
	LOG("Loading Entity Manager");
	bool ret = true;

	//Iterates over the entities and calls the Awake
	for(const auto entity : entities)
	{
		if (entity->active == false) continue;
		ret = entity->Awake();
	}

	return ret;

}


bool EntityManager::Start() {
	bool ret = true;

	//Iterates over the entities and calls Start
	for (const auto& entity : entities)
	{
		if (entity->active == false) continue;

		if (!entity->hasStarted) {
			ret = entity->Start();
			entity->hasStarted = true;
		}
	}
	return ret;
}
bool EntityManager::CleanUp(bool keepPlayer)
{
	bool ret = true;

	for (const auto& entity : entities)
	{
		if (keepPlayer && entity->type == EntityType::PLAYER) {
			continue;
		}

		if (entity->active) {
			ret = entity->Destroy(); 
		}
	}

	if (!keepPlayer) {
		playerPtr = nullptr;
	}

	return ret;
}

std::shared_ptr<Entity> EntityManager::CreateEntity(EntityType type)
{
	std::shared_ptr<Entity> entity = std::make_shared<Entity>();

	// Instantiate entity according to the type and add the new entity to the list of Entities
	switch (type)
	{
	case EntityType::PLAYER:
		entity = std::make_shared<Player>();
		break;
	case EntityType::ITEM:
		entity = std::make_shared<Item>();
		break;
	case EntityType::HEALTH_ORB:
		entity = std::make_shared<HealthOrb>();
		break;
	case EntityType::SKILL_POINT_ORB:
		entity = std::make_shared<SkillPointOrb>();
		break;
	case EntityType::SAVEPOINT:
		entity = std::make_shared<SavePoint>();
		break;
	case EntityType::SPIDER:
		entity = std::make_shared<SpiderEnemy>();
		break;
	case EntityType::CUCAFERA:
		entity = std::make_shared<Cucafera>();
		break;
	case EntityType::NINFA:
		entity = std::make_shared<Ninfa>();
		break;
	case EntityType::SWORD_KNIGHT:
		entity = std::make_shared<SwordKnight>();
		break;
	case EntityType::SHIELD_KNIGHT:
		entity = std::make_shared<ShieldKnight>();
		break;
	case EntityType::KNIGHT_BOSS:
		entity = std::make_shared<KnightBoss>();
		break;
	case EntityType::KEY:
		entity = std::make_shared<Keys>();
		break;
	case EntityType::MANTA:
		entity = std::make_shared<Manta>();
		break;
	case EntityType::SICKLE:
		entity = std::make_shared<Sickle>();
		break;
	case EntityType::DASH_OBJ:
		entity = std::make_shared<DashObj>();
		break;
	case EntityType::DOUBLEJUMP_OBJ:
		entity = std::make_shared<DoubleJumpObj>();
		break;
	default:
		break;
	}

	if (entity != nullptr)
	{
		entities.push_back(entity);
		requiresSort = true;
	}

	return entity;
}

void EntityManager::AwakeEntities()
{
	for (const auto& entity : entities)
	{
		if (entity->active && !entity->isAwake)
		{
			entity->Awake();
			entity->isAwake = true;
		}
	}
}

void EntityManager::DestroyEntity(std::shared_ptr<Entity> entity)
{
	entity->CleanUp();
	entities.remove(entity);
}

void EntityManager::AddEntity(std::shared_ptr<Entity> entity)
{
	if (entity != nullptr) {
		entities.push_back(entity);
		requiresSort = true; 
	}
}

bool EntityManager::Update(float dt)
{
	bool ret = true;
	/*if (Engine::GetInstance().sceneManager->IsGamePaused()) {
		return true;
	}*/ 
	//TO DO: Hacer que en pausa no se vea ningun enemigo
	//List to store entities pending deletion
	std::list<std::shared_ptr<Entity>> pendingDelete;

	if (requiresSort) {
		entities.sort([](const std::shared_ptr<Entity>& a, const std::shared_ptr<Entity>& b) {
			return a->zOrder < b->zOrder;
			});
		requiresSort = false;
	}

	//Iterates over the entities and calls Update
	for(const auto entity : entities)
	{
		//If the entity is marked for deletion, add it to the pendingDelete list
		if (entity->pendingToDelete)
		{
			pendingDelete.push_back(entity);
			continue;
		}
		//If the entity is not active, skip it
		if (entity->active == false) continue;
		ret = entity->Update(dt);
	}

	//Now iterates over the pendingDelete list and destroys the entities
	for (const auto entity : pendingDelete)
	{
		DestroyEntity(entity);
	}

	return ret;
}

bool EntityManager::PostUpdate()
{
	bool ret = true;
	for (const auto entity : entities)
	{
		if (entity->active == false) continue;
		ret = entity->PostUpdate();
	}
	return ret;
}
