#include "HealthOrb.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "SceneManager.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"
#include "Map.h"

HealthOrb::HealthOrb() : Entity(EntityType::ITEM)
{
	name = "HealthOrbs";
	pbody = nullptr;
	texture = nullptr;
}

HealthOrb::~HealthOrb() {}

bool HealthOrb::Awake() {
	return true;
}

bool HealthOrb::Start() {
	
	//Ensure the item does not reappear
	std::string currentMap = Engine::GetInstance().map->mapFileName;
	uniqueID = currentMap + "_" + name + "_" + std::to_string((int)position.getX()) + "_" + std::to_string((int)position.getY());

	if (Engine::GetInstance().sceneManager->collectedItems.count(uniqueID) > 0) {
		this->Destroy();
		return true;
	}

	//initilize textures
	texture = Engine::GetInstance().textures->Load("Assets/Textures/Items/Orbs/HealthOrb/HealthOrb.png");

	// Add a physics to an item - initialize the physics body
	//Engine::GetInstance().textures.get()->GetSize(texture, texW, texH);
	
	texH = 32; texW = 32;
	pbody = Engine::GetInstance().physics->CreateCircleSensor((int)position.getX() + texH / 2, (int)position.getY() + texH / 2, texH, bodyType::DYNAMIC);

	Engine::GetInstance().physics->SetGravityScale(pbody, 0.0f);

	// Assign collider type
	pbody->ctype = ColliderType::HEALTH_ORB;

	// Set this class as the listener of the pbody
	pbody->listener = this;   // so Begin/EndContact can call back to Item

	return true;
}

bool HealthOrb::Update(float dt)
{
	if (!active || pbody == nullptr) return true;

	// Add a physics to an item - update the position of the object from the physics.  
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	if(texture != nullptr) {
		Engine::GetInstance().render->DrawRotatedTexture(texture, x, y, nullptr, SDL_FLIP_NONE, 1.0f);
	}

	return true;
}

bool HealthOrb::CleanUp()
{
	if (texture != nullptr) {
		Engine::GetInstance().textures->UnLoad(texture);
		texture = nullptr;
	}
	if (pbody != nullptr) {
		Engine::GetInstance().physics->DeletePhysBody(pbody);
		pbody = nullptr;
	}
	return true;
}

bool HealthOrb::Destroy()
{
	LOG("Destroying item");
	active = false;
	pendingToDelete = true;
	return true;
}

void HealthOrb::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
	if (physB->ctype == ColliderType::PLAYER) {

		Engine::GetInstance().sceneManager->collectedItems.insert(uniqueID);
		this->Destroy();
	}
}