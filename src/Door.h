#pragma once
#include "Entity.h"
#include "Animation.h"
#include "Vector2D.h"

struct SDL_Texture;

class DoorEntity : public Entity
{
public:
	DoorEntity();
	virtual ~DoorEntity();

	bool Awake() override;
	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;

	void OpenDoorAt(Vector2D pos, int width, int height);

private:
	SDL_Texture* texture = nullptr;
	AnimationSet anims;
	bool isOpening = false;

	// 保存接收到的 Tiled 尺寸
	int doorW = 256;
	int doorH = 256;

	// ==========================================
	// 【核心数据：视觉补偿参数】
	// ==========================================
	// 1. 既然黑洞顶到了上天花板，真实视觉高度写死 256.0f
	const float VISIBLE_HEIGHT = 256.0f;

	// 2. 宽度有透明边，这代表真正的门在图里占的宽度
	// （之前调好的 128.0f 如果合适就保持，不合适就微调）
	const float VISIBLE_WIDTH = 175.0f;

	// 3. 【核心修复】：由于顶部有透明圈动画，这里需要一个偏移量。
	// 这个值代表图片顶部的透明圈有多高。
	// 请根据你的原图，估计一个像素值（例如顶部有 10 像素的透明边缘，就写 10.0f）
	// 我们把整张图向上“推”这个高度，真正的黑洞就会顶到 Tiled 框的顶部！
	const float Y_OFFSET = 0.0f;
};