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
#include <algorithm>

SpecialFloor::SpecialFloor() : Entity(EntityType::SPECIALFLOOR)
{
	name = "specialFloor";
}

SpecialFloor::~SpecialFloor() {}

bool SpecialFloor::Awake() { return true; }

bool SpecialFloor::Start() {

	texW = width;
	texH = height;

	//Initilize textures
	//texture = Engine::GetInstance().textures->Load("Assets/Textures/plataforma.png"); //TO DO: Cambiar textura
	if (texture != nullptr) {
		Engine::GetInstance().textures->GetSize(texture, texW, texH);
	}

	pbody = Engine::GetInstance().physics->CreateRectangle((int)position.getX() + texW / 2, (int)position.getY() + texH / 2, texW, texH, bodyType::KINEMATIC);

	// Assign collider type
	pbody->ctype = ColliderType::SPECIALFLOOR;
	pbody->listener = this; 

	int centerX, centerY;
	pbody->GetPosition(centerX, centerY);
	startPosition = Vector2D((float)centerX, (float)centerY); // Save the initial position of the floor

	// Calculate movement limits and initial velocity based on floor type
	if (floorType == TypeFloor::HORIZONTALFLOOR) {
		float limitX = startPosition.getX() + (moveDirection * distance);
		minMoveLimit = std::min(startPosition.getX(), limitX);
		maxMoveLimit = std::max(startPosition.getX(), limitX);
		currentVel.x = (float)(moveDirection * moveSpeed);
	}
	else if (floorType == TypeFloor::VERTICALFLOOR) {
		float limitY = startPosition.getY() + (moveDirection * distance);
		minMoveLimit = std::min(startPosition.getY(), limitY);
		maxMoveLimit = std::max(startPosition.getY(), limitY);
		currentVel.y = (float)(moveDirection * moveSpeed);
	}

	currentBreakTime = breakTimeMax;

	// If the floor requires touch to activate, start deactivated
	if (activationOnTouch) {
		isActivated = false;
	}
	else {
		isActivated = true;
	}

	return true;
}

bool SpecialFloor::Update(float dt)
{
	if (!active) return true;

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	if (!isActivated) {
		Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0.0f, 0.0f });
	}
	else {
		if (isWaiting) {
			// 1. Si está esperando, restamos el tiempo
			currentWaitTime -= dt;
			if (currentWaitTime <= 0) {
				isWaiting = false; // El tiempo se ha acabado, vuelve a moverse
			}
			else {
				// Mantiene la plataforma totalmente quieta mientras espera
				Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0.0f, 0.0f });
			}
		}
		else {
			if (floorType == TypeFloor::HORIZONTALFLOOR) {
				// Right limit
				if (position.getX() >= maxMoveLimit && currentVel.x > 0) {
					currentVel.x = -(float)abs(moveSpeed);
					pbody->SetPosition((int)maxMoveLimit, (int)position.getY());
					isWaiting = true;
					currentWaitTime = waitTimeMax;
				}
				// Left limit
				else if (position.getX() <= minMoveLimit && currentVel.x < 0) {
					currentVel.x = (float)abs(moveSpeed);
					pbody->SetPosition((int)minMoveLimit, (int)position.getY());
					isWaiting = true;
					currentWaitTime = waitTimeMax;
				}

				// Apply velocity
				if (!isWaiting) {
					Engine::GetInstance().physics->SetLinearVelocity(pbody, { currentVel.x, 0.0f });
				}
				else {
					Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0.0f, 0.0f });
				}
			}
			else if (floorType == TypeFloor::VERTICALFLOOR) {

				// Down limit
				if (position.getY() >= maxMoveLimit && currentVel.y > 0) {
					currentVel.y = -(float)abs(moveSpeed);
					pbody->SetPosition((int)position.getX(), (int)maxMoveLimit);
					isWaiting = true;
					currentWaitTime = waitTimeMax;
				}
				// Up limit
				else if (position.getY() <= minMoveLimit && currentVel.y < 0) {
					currentVel.y = (float)abs(moveSpeed);
					pbody->SetPosition((int)position.getX(), (int)minMoveLimit);
					isWaiting = true;
					currentWaitTime = waitTimeMax;
				}

				// Apply velocity
				if (!isWaiting) {
					Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0.0f, currentVel.y });
				}
				else {
					Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0.0f, 0.0f });
				}
			}
		}
	}

	// TO DO: Falta para la plataforma que da la vuelta completa

	// Breakage Management
	if (floorType == TypeFloor::BROKENFLOOR && isSteppedOn) {
		if (isBroken) {
			// Si está rota, contar tiempo para reaparecer
			currentRespawnTime -= dt;
			if (currentRespawnTime <= 0) {
				isBroken = false;
				isSteppedOn = false;
				currentBreakTime = breakTimeMax;

				// Volver a activar colisiones
				if (pbody != nullptr) pbody->SetCollisionsActive(true);
			}
		}
		else if (isSteppedOn) {
			// Si la han pisado y no está rota, contar tiempo para romperse
			currentBreakTime -= dt;
			if (currentBreakTime <= 0) {
				isBroken = true;
				currentRespawnTime = respawnTimeMax;

				// Desactivar colisiones sin borrar el cuerpo (el jugador caerá a través)
				if (pbody != nullptr) pbody->SetCollisionsActive(false);
			}
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
	if (!isBroken && texture != nullptr) {
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
		// Activate the platform if it requires touch to activate
		if (activationOnTouch && !isActivated) {
			isActivated = true;
			LOG("Plataforma móvil ACTIVADA");
		}

		if (floorType == TypeFloor::BROKENFLOOR)
		{
			isSteppedOn = true;
			LOG("Suelo ROMPIBLE");
		}
		else if (floorType == TypeFloor::HORIZONTALFLOOR)
		{
			LOG("Suelo HORIZONTAL.");
		}
		else if (floorType == TypeFloor::VERTICALFLOOR)
		{
			LOG("Suelo VERTICAL.");
		}
	}
}

