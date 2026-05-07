#include "SpecialFloors.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "SceneManager.h"
#include "GameManager.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"
#include  "Map.h"

SpecialFloor::SpecialFloor() : Entity(EntityType::SPECIALFLOOR)
{
	name = "specialFloor";
}

SpecialFloor::~SpecialFloor() {}

bool SpecialFloor::Awake() {
	return true;
}

bool SpecialFloor::Start() {

	//initilize textures
	//texture = Engine::GetInstance().textures->Load("Assets/Textures/goldCoin.png");

	Engine::GetInstance().textures.get()->GetSize(texture, texW, texH);
	pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX() + texH / 2, (int)position.getY() + texH / 2, texH / 2, bodyType::DYNAMIC);

	// Assign collider type
	pbody->ctype = ColliderType::SPECIALFLOOR;


	// Set this class as the listener of the pbody
	pbody->listener = this; 

	return true;
}

bool SpecialFloor::Update(float dt)
{
	if (!active) return true;

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	Engine::GetInstance().render->DrawTexture(texture, x - texW / 2, y - texH / 2);

	return true;
}

bool SpecialFloor::CleanUp()
{
	Engine::GetInstance().textures->UnLoad(texture);
	Engine::GetInstance().physics->DeletePhysBody(pbody);
	return true;
}

bool SpecialFloor::Destroy()
{
	LOG("Destroying brokenfloor");
	active = false;
	pendingToDelete = true;
	return true;
}


