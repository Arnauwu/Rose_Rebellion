#include "Door.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "SceneManager.h"
#include "Map.h"
#include <unordered_map>

DoorEntity::DoorEntity() : Entity(EntityType::DOOR)
{
	name = "DoorAnim";
}

DoorEntity::~DoorEntity() {}

bool DoorEntity::Awake() { return true; }

bool DoorEntity::Start() {
	texture = Engine::GetInstance().textures->Load("Assets/Textures/Animation/Door/SS_puerta_abriendo_pruevas.png");
	std::unordered_map<int, std::string> aliases = { {0, "open"} };
	anims.LoadFromTSX("Assets/Textures/Animation/Door/SS_puerta_abriendo_pruevas.tsx", aliases);

	if (anims.Has("open")) {
		anims.GetAnim("open")->SetLoop(false);
	}
	anims.SetCurrent("open");
	isOpening = false;
	return true;
}

bool DoorEntity::Update(float dt) {
	if (isOpening || (anims.Has("open") && anims.GetAnim("open")->HasFinishedOnce())) {

		if (isOpening) anims.Update(dt);

		SDL_Rect frame = anims.GetCurrentFrame();

		float cx = position.getX();
		float cy = position.getY();

		float drawW = doorW * (256.0f / VISIBLE_WIDTH);
		float drawH = doorH * (256.0f / VISIBLE_HEIGHT);

		SDL_Rect destRect;
		destRect.x = (int)(cx - (drawW / 2.0f));
		destRect.y = (int)(cy + (drawH / 2.0f) - Y_OFFSET);
		destRect.w = (int)drawW;
		destRect.h = (int)drawH;

		Engine::GetInstance().render->DrawRotatedImage(texture, &destRect, &frame);

		if (isOpening && anims.Has("open") && anims.GetAnim("open")->HasFinishedOnce()) {
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

void DoorEntity::OpenDoorAt(Vector2D pos, int width, int height) {
	if (texture == nullptr) Start();

	position = pos;

	doorW = width;
	doorH = height;

	isOpening = true;

	if (anims.Has("open")) {
		anims.GetAnim("open")->Reset();
		anims.SetCurrent("open");
	}
}