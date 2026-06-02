#include "BreakableRock.h"

#include "Engine.h"
#include "EntityManager.h"
#include "GameManager.h"
#include "Log.h"
#include "Map.h"
#include "ParticleManager.h"
#include "Physics.h"
#include "Player.h"
#include "Render.h"
#include "Textures.h"

#include <cstdlib>
#include <string>
#include <unordered_map>

BreakableRock::BreakableRock() : Entity(EntityType::BREAKABLE_ROCK)
{
	name = "BreakableRock";
}

BreakableRock::~BreakableRock() {}

bool BreakableRock::Awake()
{
	return true;
}

bool BreakableRock::Start()
{
	if (CheckIfDestroyed()) return true;

	std::unordered_map<int, std::string> aliases = {
		{0, "rock_0"},
		{1, "rock_1"},
		{2, "rock_2"},
		{3, "rock_3"}
	};

	anims.LoadFromTSX("Assets/Textures/Animation/Roca/Atlas_piedras_catacumbas.tsx", aliases);
	anims.SetCurrent("rock_0");

	texture = Engine::GetInstance().textures->Load("Assets/Textures/Animation/Roca/Atlas_piedras_catacumbas.png");

	const SDL_Rect& animFrame = anims.GetCurrentFrame();
	if (width <= 0) width = animFrame.w;
	if (height <= 0) height = animFrame.h;

	pbody = Engine::GetInstance().physics->CreateRectangle((int)position.getX(), (int)position.getY(), width, height, bodyType::STATIC);
	pbody->listener = this;
	pbody->ctype = ColliderType::MAP;

	return true;
}

bool BreakableRock::Update(float dt)
{
	if (texture == nullptr || pbody == nullptr) return true;

	if (shakeTimer > 0.0f) {
		shakeTimer -= dt;
		if (shakeTimer < 0.0f) shakeTimer = 0.0f;
	}

	int x, y;
	pbody->GetPosition(x, y);

	int shakeOffsetX = 0;
	int shakeOffsetY = 0;
	if (shakeTimer > 0.0f) {
		shakeOffsetX = (rand() % (shakeIntensity * 2 + 1)) - shakeIntensity;
		shakeOffsetY = (rand() % (shakeIntensity * 2 + 1)) - shakeIntensity;
	}

	const SDL_Rect& animFrame = anims.GetCurrentFrame();
	Engine::GetInstance().render->DrawTexture(
		texture,
		x - animFrame.w / 2 + shakeOffsetX,
		y - animFrame.h / 2 + shakeOffsetY,
		&animFrame
	);

	return true;
}

bool BreakableRock::CleanUp()
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

void BreakableRock::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
	if (physB == nullptr || physB->ctype != ColliderType::PLAYER_ATTACK || pendingToDelete) return;

	hitsTaken++;
	ApplyHitFeedback();

	if (hitsTaken >= maxHits) {
		SetDestroyed();
		return;
	}

	SetDamageFrame();
}

void BreakableRock::SetSize(int width, int height)
{
	this->width = width;
	this->height = height;
}

void BreakableRock::ApplyHitFeedback()
{
	shakeTimer = shakeDuration;

	Player* player = Engine::GetInstance().entityManager->GetPlayer();
	if (player != nullptr) {
		player->cameraController.StartShake(120.0f, 6.0f);
		Engine::GetInstance().particleManager->EmitAttack(position.getX(), position.getY(), player->lookingRight);
	}
	else {
		Engine::GetInstance().particleManager->EmitAttack(position.getX(), position.getY(), true);
	}
}

bool BreakableRock::CheckIfDestroyed()
{
	std::string currentMap = Engine::GetInstance().map->mapFileName;
	uniqueID = currentMap + "_" + name + "_" + std::to_string((int)position.getX()) + "_" + std::to_string((int)position.getY());

	if (GameManager::GetInstance().gameState.collectedItems.count(uniqueID) > 0) {
		Destroy();
		return true;
	}

	return false;
}

void BreakableRock::SetDestroyed()
{
	GameManager::GetInstance().gameState.collectedItems.insert(uniqueID);
	RemoveCollider();
	ResetPlayerWallState();
	Destroy();
}

void BreakableRock::RemoveCollider()
{
	if (pbody != nullptr) {
		Engine::GetInstance().physics->DeletePhysBody(pbody);
		pbody = nullptr;
	}
}

void BreakableRock::ResetPlayerWallState()
{
	Player* player = Engine::GetInstance().entityManager->GetPlayer();
	if (player == nullptr) return;

	player->onWall = false;
	player->wallDirection = 0;
}

void BreakableRock::SetDamageFrame()
{
	std::string animName = "rock_" + std::to_string(hitsTaken);
	anims.SetCurrent(animName);
}
