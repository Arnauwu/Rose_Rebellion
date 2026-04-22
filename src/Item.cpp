#include "Item.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "SceneManager.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"
#include  "Map.h"
Item::Item() : Entity(EntityType::ITEM)
{
	name = "item";
}

Item::~Item() {}

bool Item::Awake() {
	return true;
}

bool Item::Start() {

	//initilize textures
	texture = Engine::GetInstance().textures->Load("Assets/Textures/goldCoin.png");
	
	// Add a physics to an item - initialize the physics body
	Engine::GetInstance().textures.get()->GetSize(texture, texW, texH);
	pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX() + texH / 2, (int)position.getY() + texH / 2, texH / 2, bodyType::DYNAMIC);

	// Assign collider type
	pbody->ctype = ColliderType::ITEM;


	// Set this class as the listener of the pbody
	pbody->listener = this;   // so Begin/EndContact can call back to Item

	return true;
}

bool Item::Update(float dt)
{
	if (!active) return true;

	// Add a physics to an item - update the position of the object from the physics.  
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	Engine::GetInstance().render->DrawTexture(texture, x - texW / 2, y - texH / 2);

	return true;
}

bool Item::CleanUp()
{
	Engine::GetInstance().textures->UnLoad(texture);
	Engine::GetInstance().physics->DeletePhysBody(pbody);
	return true;
}

bool Item::Destroy()
{
	LOG("Destroying item");
	active = false;
	pendingToDelete = true;
	return true;
}

bool Item::CheckIfCollected() {
	// ??????ID???: "Castle_Inside.tmx_Key_120_240"
	std::string currentMap = Engine::GetInstance().map->mapFileName;
	uniqueID = currentMap + "_" + name + "_" + std::to_string((int)position.getX()) + "_" + std::to_string((int)position.getY());

	// ????ID???????????
	if (Engine::GetInstance().sceneManager->collectedItems.count(uniqueID) > 0) {
		// ???????????????? true
		this->Destroy();
		return true;
	}
	return false; // ??????? false
}

void Item::SetCollected() {
	if (!isPicked) {
		isPicked = true;
		// ??????ID?????"???"????
		Engine::GetInstance().sceneManager->collectedItems.insert(uniqueID);
		// ????????
		this->Destroy();
	}
}