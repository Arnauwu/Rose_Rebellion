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

HealthOrb::HealthOrb() : Entity(EntityType::ITEM)
{
	name = "HealthOrbs";
}

HealthOrb::~HealthOrb() {}

bool HealthOrb::Awake() {
	return true;
}

bool HealthOrb::Start() {

	//initilize textures
	texture = Engine::GetInstance().textures->Load("Assets/Textures/Items/Orbs/HealthOrb/HealthOrb.png");

	// Add a physics to an item - initialize the physics body
	//Engine::GetInstance().textures.get()->GetSize(texture, texW, texH);
	
	texH = 32; texW = 32;
	pbody = Engine::GetInstance().physics->CreateCircleSensor((int)position.getX() + texH / 2, (int)position.getY() + texH / 2, texH / 2, bodyType::DYNAMIC);

	Engine::GetInstance().physics->SetGravityScale(pbody, 0.0f);

	// Assign collider type
	pbody->ctype = ColliderType::HEALTH_ORB;

	// Set this class as the listener of the pbody
	pbody->listener = this;   // so Begin/EndContact can call back to Item

	return true;
}

bool HealthOrb::Update(float dt)
{
	if (!active) return true;

	// Add a physics to an item - update the position of the object from the physics.  
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	Engine::GetInstance().render->DrawRotatedTexture(texture, x - texW / 2, y - texH / 2, nullptr, SDL_FLIP_NONE, 0.5f);

	return true;
}

bool HealthOrb::CleanUp()
{
	Engine::GetInstance().textures->UnLoad(texture);
	Engine::GetInstance().physics->DeletePhysBody(pbody);
	return true;
}

bool HealthOrb::Destroy()
{
	LOG("Destroying item");
	active = false;
	pendingToDelete = true;
	return true;
}