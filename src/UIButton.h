#pragma once

#include "UIElement.h"
#include "Vector2D.h"

class UIButton : public UIElement
{
public:
	UIButton(int id, float anchorX, float anchorY, float wPerc, float hPerc, const char* text);
	virtual ~UIButton();

	bool Update(float dt) override;
	void Draw() const override;
	bool CleanUp() override;
	void SetFrameTexture(SDL_Texture* tex) {
		frameTexture = tex;
	}
private:
	bool canClick = true;
	bool drawBasic = false;

	float currentTextScale = 1.0f;
	float targetTextScale = 1.0f;
	SDL_Texture* frameTexture = nullptr;
	SDL_Color currentTextColor = { 180, 180, 180, 255 };
};

#pragma once