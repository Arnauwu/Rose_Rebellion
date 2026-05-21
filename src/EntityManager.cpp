#include "EntityManager.h"
#include "Player.h"
#include "Npc.h"
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
#include "WallJumpObj.h"

#include "SavePoint.h"
#include "Door.h"

#include "Cucafera.h"
#include "CucaferaShiny.h"


#include "SwordKnight.h"
#include "ShieldKnight.h"

#include "Ninfa.h"
#include "Demon.h"
#include "Dip.h"

#include "Minairon.h"
#include "Bat.h"
#include "ToxicBall.h"

#include "KnightBoss.h"
#include "NinfaBoss.h"
#include "GwellBoss.h"
#include "Dragon.h"
#include "DragonProjectile.h"

#include "SpecialFloors.h"

#include "tracy/Tracy.hpp"

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
	case EntityType::NPC:
		entity = std::make_shared<Npc>();
		break;
	case EntityType::SAVEPOINT:
		entity = std::make_shared<SavePoint>();
		break;
	case EntityType::SPECIALFLOOR:
		entity = std::make_shared<SpecialFloor>();
		break;

	case EntityType::DOOR:
		entity = std::make_shared<DoorEntity>(); 
		break;
		//Enemies

	case EntityType::CUCAFERA:
		entity = std::make_shared<Cucafera>();
		break;
	case EntityType::CUCAFERA_SHINY:
		entity = std::make_shared<CucaferaShiny>();
		break;

	case EntityType::SWORD_KNIGHT:
		entity = std::make_shared<SwordKnight>();
		break;
	case EntityType::SHIELD_KNIGHT:
		entity = std::make_shared<ShieldKnight>();
		break;

	case EntityType::NINFA:
		entity = std::make_shared<Ninfa>();
		break;
	case EntityType::DEMON:
		entity = std::make_shared<Demon>();
		break;
	case EntityType::DIP:
		entity = std::make_shared<Dip>();
		break;
	case EntityType::MINAIRON:
		entity = std::make_shared<Minairon>();
		break;
	case EntityType::BAT:
		entity = std::make_shared<Bat>();
		break;
	case EntityType::TOXIC_BALL:
		entity = std::make_shared<ToxicBall>();
		break;

		//Bosses & MiniBosses
	case EntityType::KNIGHT_BOSS:
		entity = std::make_shared<KnightBoss>();
		break;
	case EntityType::NINFA_MARE:
		entity = std::make_shared<NinfaMare>();
		break;
	case EntityType::GWELL_BOSS:
		entity = std::make_shared<GwellBoss>();
		break;
	case EntityType::DRAGON:
		entity = std::make_shared<Dragon>();
		break;
	case EntityType::DRAGON_PROJECTILE:
		entity = std::make_shared<DragonProjectile>();
		break;

		// Items
	case EntityType::ITEM:
		entity = std::make_shared<Item>();
		break;
	case EntityType::HEALTH_ORB:
		entity = std::make_shared<HealthOrb>();
		break;
	case EntityType::SKILL_POINT_ORB:
		entity = std::make_shared<SkillPointOrb>();
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
	case EntityType::WALLJUMP_OBJ:
		entity = std::make_shared<WallJumpObj>();
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
	ZoneScoped;

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
	ZoneScoped;

	bool ret = true;
	for (const auto entity : entities)
	{
		if (entity->active == false) continue;
		ret = entity->PostUpdate();
	}
	return ret;
}
