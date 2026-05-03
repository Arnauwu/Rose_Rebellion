#pragma once

#include "UIElement.h"
#include "Vector2D.h"
#include "Animation.h"

class UIButton : public UIElement
{
public:
	UIButton(int id, float anchorX, float anchorY, float wPerc, float hPerc, const char* text);
	virtual ~UIButton();

	bool Update(float dt) override;
	void Draw() const override;
	bool CleanUp() override;
	void SetFrameTexture(SDL_Texture* tex) { frameTexture = tex; }
	void SetFlashTexture(SDL_Texture* tex) { flashTexture = tex; useFlashAnim = false;}
	void SetFlashAnimation(SDL_Texture* tex, Animation anim) {
		flashTexture = tex;
		flashAnim = anim;
		useFlashAnim = true;
	}

private:
	bool canClick = true;
	bool drawBasic = false;

	float currentTextScale = 1.0f;
	float targetTextScale = 1.0f;

	// Variables para la animación del destello
	Animation flashAnim;
	bool useFlashAnim = false;

	SDL_Texture* frameTexture = nullptr;
	SDL_Texture* flashTexture = nullptr;

	SDL_Color currentTextColor = { 180, 180, 180, 255 };
};

#pragma once