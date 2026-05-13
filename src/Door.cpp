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

		// ==========================================
		// 【终极完美对齐算法】
		// ==========================================
		float cx = position.getX();
		float cy = position.getY();

		// 1. 自动计算需要缩放多大，才能填满 Tiled 框（剔除宽度透明边）
		float drawW = doorW * (256.0f / VISIBLE_WIDTH);
		float drawH = doorH * (256.0f / VISIBLE_HEIGHT);

		// 2. 将放大的整张图居中对齐物理中心点点 (cx, cy)
		SDL_Rect destRect;
		destRect.x = (int)(cx - (drawW / 2.0f));
		// 【核心修复】：在底端 (cy + drawH / 2.0f) 的基础上，减去 Y_OFFSET！
		// 这样就把整张图向上“推”了 Y_OFFSET 个像素！
		destRect.y = (int)(cy + (drawH / 2.0f) - Y_OFFSET);
		destRect.w = (int)drawW;
		destRect.h = (int)drawH;

		// 3. 使用允许拉伸和强制 Y 对齐的 DrawRotatedImage
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

	// 完全接收 Tiled 的宽和高
	doorW = width;
	doorH = height;

	isOpening = true;

	if (anims.Has("open")) {
		anims.GetAnim("open")->Reset();
		anims.SetCurrent("open");
	}
}