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
#include <cstdlib>

SpecialFloor::SpecialFloor() : Entity(EntityType::SPECIALFLOOR)
{
	name = "specialFloor";
}

SpecialFloor::~SpecialFloor() {}

bool SpecialFloor::Awake() { return true; }

bool SpecialFloor::Start() {

	texW = width;
	texH = height;

	// Initialize animation
	std::unordered_map<int, std::string> aliases = { {0,"normal"},{1,"broken"} };
	anims.LoadFromTSX("Assets/Maps/Catacombs/SpecialFloor_Broken.tsx", aliases);
	anims.SetCurrent("normal");

	if (floorType == TypeFloor::BROKENFLOOR) {
		texture = Engine::GetInstance().textures->Load("Assets/Maps/Catacombs/SpecialFloor_Broken.png");
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
	else if (floorType == TypeFloor::CIRCULARFLOOR) {
		if (moveDirection == 1) {
			// Start moving to the RIGHT (+1)
			limitLeft = startPosition.getX();
			limitRight = startPosition.getX() + distance;
			limitUp = startPosition.getY();
			limitDown = startPosition.getY() + distance;
			currentVel.x = (float)abs(moveSpeed);
			currentVel.y = 0.0f;
		}
		else {
			// Start moving to the LEFT (-1)
			limitLeft = startPosition.getX() - distance;
			limitRight = startPosition.getX();
			limitUp = startPosition.getY() - distance;
			limitDown = startPosition.getY();
			currentVel.x = -(float)abs(moveSpeed);
			currentVel.y = 0.0f;
		}
		pathStep = 0;
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

	Draw(dt);

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	if (!isActivated) {
		Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0.0f, 0.0f });
	}
	else {
		if (isWaiting) {
			// If you're waiting, we'll deduct the time
			currentWaitTime -= dt;
			if (currentWaitTime <= 0) {
				isWaiting = false; // Time's up, get moving again
			}
			else {
				// Keep the platform completely still whilst you wait
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
			else if (floorType == TypeFloor::CIRCULARFLOOR) {
				if (moveDirection == 1) {
					// Path (+1): Right -> Down -> Left -> Up
					if (pathStep == 0) {
						if (position.getX() >= limitRight && currentVel.x > 0) {
							pbody->SetPosition((int)limitRight, (int)position.getY());
							pathStep = 1; currentVel = { 0.0f, (float)abs(moveSpeed) }; // Now Down
							isWaiting = true; currentWaitTime = waitTimeMax;
						}
					}
					else if (pathStep == 1) {
						if (position.getY() >= limitDown && currentVel.y > 0) {
							pbody->SetPosition((int)position.getX(), (int)limitDown);
							pathStep = 2; currentVel = { -(float)abs(moveSpeed), 0.0f }; // Now Left
							isWaiting = true; currentWaitTime = waitTimeMax;
						}
					}
					else if (pathStep == 2) {
						if (position.getX() <= limitLeft && currentVel.x < 0) {
							pbody->SetPosition((int)limitLeft, (int)position.getY());
							pathStep = 3; currentVel = { 0.0f, -(float)abs(moveSpeed) }; // Now Up
							isWaiting = true; currentWaitTime = waitTimeMax;
						}
					}
					else if (pathStep == 3) {
						if (position.getY() <= limitUp && currentVel.y < 0) {
							pbody->SetPosition((int)position.getX(), (int)limitUp);
							pathStep = 0; currentVel = { (float)abs(moveSpeed), 0.0f }; // Now Right
							isWaiting = true; currentWaitTime = waitTimeMax;
						}
					}
				}
				else {
					// Path (-1): Left -> Up -> Right -> Down
					if (pathStep == 0) {
						if (position.getX() <= limitLeft && currentVel.x < 0) {
							pbody->SetPosition((int)limitLeft, (int)position.getY());
							pathStep = 1; currentVel = { 0.0f, -(float)abs(moveSpeed) }; // Now Up
							isWaiting = true; currentWaitTime = waitTimeMax;
						}
					}
					else if (pathStep == 1) {
						if (position.getY() <= limitUp && currentVel.y < 0) {
							pbody->SetPosition((int)position.getX(), (int)limitUp);
							pathStep = 2; currentVel = { (float)abs(moveSpeed), 0.0f }; // Now Right
							isWaiting = true; currentWaitTime = waitTimeMax;
						}
					}
					else if (pathStep == 2) {
						if (position.getX() >= limitRight && currentVel.x > 0) {
							pbody->SetPosition((int)limitRight, (int)position.getY());
							pathStep = 3; currentVel = { 0.0f, (float)abs(moveSpeed) }; // Now Down
							isWaiting = true; currentWaitTime = waitTimeMax;
						}
					}
					else if (pathStep == 3) {
						if (position.getY() >= limitDown && currentVel.y > 0) {
							pbody->SetPosition((int)position.getX(), (int)limitDown);
							pathStep = 0; currentVel = { -(float)abs(moveSpeed), 0.0f }; // Now Left
							isWaiting = true; currentWaitTime = waitTimeMax;
						}
					}
				}

				// Apply velocity at the end
				if (!isWaiting) Engine::GetInstance().physics->SetLinearVelocity(pbody, currentVel);
				else Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0.0f, 0.0f });
			}
		}
	}

	// Breakage Management
	if (floorType == TypeFloor::BROKENFLOOR && isSteppedOn) {
		if (isBroken) {
			// If it's broken, wait for it to reappear
			currentRespawnTime -= dt;
			if (currentRespawnTime <= 0) {
				isBroken = false;
				isSteppedOn = false;
				currentBreakTime = breakTimeMax;

				anims.SetCurrent("normal");

				// Re-enable collisions
				if (pbody != nullptr) pbody->SetCollisionsActive(true);
			}
		}
		else if (isSteppedOn) {
			// If it’s been stepped on and isn’t broken, wait for it to break
			currentBreakTime -= dt;

			if (currentBreakTime <= animBreakDuration && anims.GetCurrentName() != "broken") {
				anims.SetCurrent("broken");
				anims.GetAnim("broken")->Reset();
				anims.GetAnim("broken")->SetLoop(false);
			}

			if (currentBreakTime <= 0) {
				isBroken = true;
				currentRespawnTime = respawnTimeMax;

				if (pbody != nullptr) pbody->SetCollisionsActive(false);
			}
		}
	}

	return true;
}

void SpecialFloor::Draw(float dt)
{
	if (Engine::GetInstance().sceneManager->isGamePaused == false)
	{
		anims.Update(dt);
	}
	
	if (isBroken && anims.GetAnim("broken")->HasFinishedOnce()) {
		return;
	}

	const SDL_Rect& animFrame = anims.GetCurrentFrame();

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	

	if (texture != nullptr)
	{
		// --- SHAKE LOGIC ---

		int shakeOffsetX = 0;
		int shakeOffsetY = 0;

		if (floorType == TypeFloor::BROKENFLOOR && isSteppedOn && !isBroken && currentBreakTime > animBreakDuration) {

			shakeOffsetX = (rand() % 5) - 2;
			shakeOffsetY = (rand() % 5) - 2; 
		}

		// ---- MOSAICO SISTEM (TILING) ----

		int drawHeight = (int)(texH * 1.8f);

		float aspect = (float)animFrame.w / (float)animFrame.h;
		float idealDrawWidth = drawHeight * aspect;

		int numBlocks = std::max(1, (int)std::ceil((float)texW / idealDrawWidth));
		int actualDrawWidth = texW / numBlocks;

		int startX = (x - (texW / 2)) + shakeOffsetX; //Apply shake to the floor
		int topY = (y - (texH / 2)) + shakeOffsetY;

		int bottomY = topY + drawHeight;

		for (int i = 0; i < numBlocks; i++)
		{
			SDL_Rect destRect = { startX + (i * actualDrawWidth), bottomY, actualDrawWidth, drawHeight };
			Engine::GetInstance().render->DrawRotatedImage(texture, &destRect, &animFrame);
		}
	}
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
			if (!isSteppedOn && !isBroken) {
				isSteppedOn = true;		
				LOG("Suelo ROMPIBLE pisado.");
			}
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

