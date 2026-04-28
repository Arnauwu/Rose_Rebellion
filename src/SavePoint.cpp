
#include "SavePoint.h"

#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Log.h"
#include "Physics.h"

SavePoint::SavePoint() :Entity(EntityType::SAVEPOINT) {
	name = "SavePoint";
	pbody = nullptr;
	texture = nullptr;
}

SavePoint::~SavePoint() {}

bool SavePoint::Awake() {
	return true;
}

bool SavePoint::Start() {
	// Savepoint sensor: Activates when the player passes through it. The physics engine sends an OnCollision / OnTrigger notification	
	texture = Engine::GetInstance().textures->Load("Assets/Textures/Items/SavePoint/SavePoint.png");

	texH = 32; texW = 32;
	pbody = Engine::GetInstance().physics->CreateCircleSensor((int)position.getX() + texH / 2, (int)position.getY() + texH / 2, texH, bodyType::STATIC);

	Engine::GetInstance().physics->SetGravityScale(pbody, 0.0f);
	// Savepoint type
	pbody->ctype = ColliderType::SAVEPOINT;
	// Bind a listener so this object can receive and handle collision events.
	pbody->listener = this;

	return true;
}

bool SavePoint::Update(float dt) {
	int x, y;
	pbody->GetPosition(x, y);
	// Move the pivot from the center to the top-left.
	Engine::GetInstance().render->DrawTexture(texture, x - 16, y - 16, NULL);

	return true;
}

bool SavePoint::CleanUp()
{
	Engine::GetInstance().textures->UnLoad(texture);
	Engine::GetInstance().physics->DeletePhysBody(pbody);
	return true;
}

void SavePoint::Activate() {
	// Activates on pass and prevents re-activation.
	if (!isActivated) {
		isActivated = true;
		LOG("SavePoint Activated");
	}
}

