#include "Door.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "SceneManager.h"
#include "Map.h"
#include <unordered_map>

#include "tracy/Tracy.hpp"

namespace
{
	constexpr float NormalVisibleHeight = 256.0f;
	constexpr float NormalVisibleWidth = 175.0f;
	constexpr float BossVisibleHeight = 252.0f;
	constexpr float BossVisibleWidth = 180.0f;
	constexpr float NexoBossExtraWidth = 24.0f;
	constexpr float NexoBossExtraHeight = 72.0f;
}

DoorEntity::DoorEntity() : Entity(EntityType::DOOR)
{
	name = "DoorAnim";
}

DoorEntity::~DoorEntity() {}

bool DoorEntity::Awake() { return true; }

bool DoorEntity::Start() {
	texture = Engine::GetInstance().textures->Load("Assets/Textures/Animation/Door/SS_puertas.png");
	std::unordered_map<int, std::string> aliases = {
		{0, "open"},
		{16, "bossOpen"}
	};
	anims.LoadFromTSX("Assets/Textures/Animation/Door/SS_puertas.tsx", aliases);

	if (anims.Has("open")) {
		anims.GetAnim("open")->SetLoop(false);
	}
	if (anims.Has("bossOpen")) {
		anims.GetAnim("bossOpen")->SetLoop(false);
	}
	anims.SetCurrent("open");
	isOpening = false;
	return true;
}

bool DoorEntity::Update(float dt) {

	ZoneScoped;
	if (isOpening || (anims.Has(animationName) && anims.GetAnim(animationName)->HasFinishedOnce())) {

		if (isOpening) anims.Update(dt);

		SDL_Rect frame = anims.GetCurrentFrame();

		float cx = position.getX();
		float cy = position.getY();

		const bool isBossDoor = animationName == "bossOpen";
		const float visibleWidth = isBossDoor ? BossVisibleWidth : NormalVisibleWidth;
		const float visibleHeight = isBossDoor ? BossVisibleHeight : NormalVisibleHeight;
		const float visualWidth = doorW + (enlargeBossDoor ? NexoBossExtraWidth : 0.0f);
		const float visualHeight = doorH + (enlargeBossDoor ? NexoBossExtraHeight : 0.0f);
		float drawW = visualWidth * (256.0f / visibleWidth);
		float drawH = visualHeight * (256.0f / visibleHeight);

		SDL_Rect destRect;
		destRect.x = (int)(cx - (drawW / 2.0f));
		destRect.y = isBossDoor
			? (int)(cy + (doorH / 2.0f))
			: (int)(cy + (drawH / 2.0f));
		destRect.w = (int)drawW;
		destRect.h = (int)drawH;

		Engine::GetInstance().render->DrawRotatedImage(texture, &destRect, &frame);

		if (isOpening && anims.Has(animationName) && anims.GetAnim(animationName)->HasFinishedOnce()) {
			isOpening = false;
			Engine::GetInstance().sceneManager->setNewMap = true;
		}
	}
	return true;
}

bool DoorEntity::CleanUp() {
	if (texture != nullptr) {
		Engine::GetInstance().textures->UnLoad(texture);
		texture = nullptr;
	}
	return true;
}

void DoorEntity::OpenDoorAt(Vector2D pos, int width, int height, KeyType keyType, bool shouldEnlargeBossDoor) {
	if (texture == nullptr) Start();

	position = pos;

	doorW = width;
	doorH = height;
	animationName = keyType == KeyType::BOSS ? "bossOpen" : "open";
	enlargeBossDoor = shouldEnlargeBossDoor && keyType == KeyType::BOSS;

	isOpening = true;

	if (anims.Has(animationName)) {
		anims.GetAnim(animationName)->Reset();
		anims.SetCurrent(animationName);
	}
}
