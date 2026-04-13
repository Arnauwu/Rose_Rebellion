#pragma once
#include "UIElement.h"

class UICheckBox : public UIElement {
public:
	UICheckBox(int id, float anchorX, float anchorY, float wPerc, float hPerc, const char* text);
	bool Update(float dt) override;
	void Draw() const override;
	bool CleanUp() override;

	bool isChecked = false;
};