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

private:
	bool canClick = true;
	bool drawBasic = false;
};

#pragma once