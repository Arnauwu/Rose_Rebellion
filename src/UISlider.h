#pragma once
#include "UIElement.h"

class UISlider : public UIElement {
public:
	UISlider(int id, float anchorX, float anchorY, float wPerc, float hPerc, const char* text);
	bool Update(float dt) override;
	void Draw() const override;
	bool CleanUp() override;

	void SetValue(float value);
	float GetValue() const;

private:
	SDL_Rect sliderBar;
	SDL_Rect thumb;
	float value = 0.5f;

	void UpdateBarAndThumb();

	bool isDragging = false;
};