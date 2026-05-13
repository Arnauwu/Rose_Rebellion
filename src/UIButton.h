#pragma once

#include "UIElement.h"
#include "Vector2D.h"
#include "Animation.h" 

class UIButton : public UIElement
{
public:
	UIButton(int id, float anchorX, float anchorY, float wPerc, float hPerc, const char* text);
	~UIButton();

	bool Update(float dt) override;
	void Draw() const override;
	bool CleanUp() override;

	void SetFrameTexture(SDL_Texture* tex) { frameTexture = tex; }
	void SetClickTexture(SDL_Texture* tex) { clickTexture = tex; }
	bool canClick = true;
	bool drawBasic = false;
	void SetSounds(int hoverFxId, int clickFxId) {
		hoverFx = hoverFxId;
		clickFx = clickFxId;
	}

private:
	int hoverFx = -1;
	int clickFx = -1;
	bool wasMouseInside = false;

	// Variables para evitar clics fantasma
	bool isPressedInternally = false;
	bool wasFocused = false;
	bool pendingAction = false;

	// Control de animaciones de UI
	AnimationSet frameAnimSet;
	AnimationSet clickAnimSet;
	bool showClickAnim;

	// Variables visuales del texto
	float targetTextScale = 1.0f;
	float currentTextScale = 1.0f;
	SDL_Texture* frameTexture = nullptr;
	SDL_Texture* clickTexture = nullptr;
	SDL_Color currentTextColor = { 255, 255, 255, 255 };
};

#pragma once