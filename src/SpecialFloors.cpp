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

bool SpecialFloor::Awake() { return true; }

bool SpecialFloor::Start() {

	texW = 300;
	texH = 100;

	//Initilize textures
	//texture = Engine::GetInstance().textures->Load("Assets/Textures/plataforma.png"); //TO DO: Cambiar textura
	if (texture != nullptr) {
		Engine::GetInstance().textures->GetSize(texture, texW, texH);
	}

	pbody = Engine::GetInstance().physics->CreateRectangle((int)position.getX() + texW / 2, (int)position.getY() + texH / 2, texW, texH, bodyType::KINEMATIC);

	// Assign collider type
	pbody->ctype = ColliderType::SPECIALFLOOR;
	pbody->listener = this; 

	startPosition = position; // Save the initial position of the floor

	return true;
}

bool SpecialFloor::Update(float dt)
{
	if (!active) return true;

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	// Movement Management
	b2Vec2 velocity = { 0.0f, 0.0f };

	if (floorType == TypeFloor::HORIZONTALFLOOR) {
		float traveled = abs(position.getX() - startPosition.getX());

		if (traveled >= distance) {
			movingForward = !movingForward; // Invert direction when reaching the limit
		}

		velocity.x = movingForward ? (float)moveSpeed : (float)-moveSpeed;
		Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);
	}
	else if (floorType == TypeFloor::VERTICALFLOOR) {
		float traveled = abs(position.getY() - startPosition.getY());

		if (traveled >= distance) {
			movingForward = !movingForward;
		}

		velocity.y = movingForward ? (float)moveSpeed : (float)-moveSpeed;
		Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);
	}
	// TO DO: Falta para la plataforma que da la vuelta completa

	// Breakage Management
	if (floorType == TypeFloor::BROKENFLOOR && isSteppedOn) {
		currentBreakTime -= dt;
		if (currentBreakTime <= 0) {
			Destroy();
		}
	}

	// Drawing
	if (texture != nullptr) {
		Engine::GetInstance().render->DrawTexture(texture, x - texW / 2, y - texH / 2);
	}

	return true;
}

bool SpecialFloor::CleanUp()
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

bool SpecialFloor::Destroy()
{
	LOG("Destroying floor");
	active = false;
	pendingToDelete = true;
	return true;
}

void SpecialFloor::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
	if (physB->ctype == ColliderType::PLAYER)
	{
		if (floorType == TypeFloor::BROKENFLOOR)
		{
			isSteppedOn = true;
			LOG("¡El jugador ha pisado un suelo ROMPIBLE! Tiempo para romperse: %f", currentBreakTime);
		}
		else if (floorType == TypeFloor::HORIZONTALFLOOR)
		{
			LOG("El jugador está tocando un suelo HORIZONTAL.");
		}
		else if (floorType == TypeFloor::VERTICALFLOOR)
		{
			LOG("El jugador está tocando un suelo VERTICAL.");
		}
	}
}

