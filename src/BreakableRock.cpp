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
#include <algorithm>
#include <string>
#include <unordered_map>

namespace
{
	constexpr float RockBlockScale = 1.35f;
	constexpr int RockMinBlockWidth = 520;
	constexpr int RockMinBlockHeight = 520;
}

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

	if (breakableType == BreakableType::TRUNK) {
		const int rowStart = variant == 2 ? 10 : 0;
		std::unordered_map<int, std::string> aliases = {
			{rowStart, "trunk_2"},
			{rowStart + 1, "trunk_1"},
			{rowStart + 2, "trunk_0"}
		};

		anims.LoadFromTSX("Assets/Textures/Animation/Trunco/Atlas_troncos_romper_montana.tsx", aliases);
		anims.SetCurrent("trunk_0");
		texture = Engine::GetInstance().textures->Load("Assets/Textures/Animation/Trunco/Atlas_troncos_romper_montana.png");
		maxHits = 3;
	}
	else {
		std::unordered_map<int, std::string> aliases = {
			{0, "rock_0"},
			{1, "rock_1"},
			{2, "rock_2"},
			{3, "rock_3"}
		};

		anims.LoadFromTSX("Assets/Textures/Animation/Roca/Atlas_piedras_catacumbas.tsx", aliases);
		anims.SetCurrent("rock_0");
		texture = Engine::GetInstance().textures->Load("Assets/Textures/Animation/Roca/Atlas_piedras_catacumbas.png");
		maxHits = 4;
	}

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
	float renderScale = std::max(width / (float)animFrame.w, height / (float)animFrame.h);
	SDL_Rect rockRect = {
		x - (int)(animFrame.w * renderScale / 2.0f) + shakeOffsetX,
		y - (int)(animFrame.h * renderScale / 2.0f) + shakeOffsetY,
		(int)(animFrame.w * renderScale),
		(int)(animFrame.h * renderScale)
	};

	Engine::GetInstance().render->DrawWorldTextureScaledSection(
		texture,
		animFrame,
		rockRect
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
	int originalHeight = height;
	uniqueIDPosition = position;
	hasUniqueIDPosition = true;

	if (breakableType == BreakableType::TRUNK) {
		this->width = width;
		this->height = height;
	}
	else {
		this->width = std::max(RockMinBlockWidth, (int)(width * RockBlockScale));
		this->height = std::max(RockMinBlockHeight, (int)(height * RockBlockScale));
	}

	position.setY(position.getY() + (originalHeight - this->height) / 2.0f);
}

void BreakableRock::Configure(BreakableType type, int variant)
{
	breakableType = type;
	this->variant = variant == 2 ? 2 : 1;
	name = breakableType == BreakableType::TRUNK
		? "BreakableTrunk" + std::to_string(this->variant)
		: "BreakableRock";
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
	Vector2D idPosition = hasUniqueIDPosition ? uniqueIDPosition : position;
	uniqueID = currentMap + "_" + name + "_" + std::to_string((int)idPosition.getX()) + "_" + std::to_string((int)idPosition.getY());

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
	const std::string prefix = breakableType == BreakableType::TRUNK ? "trunk_" : "rock_";
	std::string animName = prefix + std::to_string(hitsTaken);
	anims.SetCurrent(animName);
}
