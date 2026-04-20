#include "UIElement.h"
#include "Engine.h"
#include "Window.h"

UIElement::UIElement(UIElementType type, int id, float anchorX, float anchorY, float wPercent, float hPercent, const char* text) :
	type(type),
	id(id),
	state(UIElementState::NORMAL),
	anchorX(anchorX),
	anchorY(anchorY),
	relW(wPercent),
	relH(hPercent),
	text(text)
{
	color.r = 255; color.g = 255; color.b = 255;
	texture = nullptr;

	RecalculateBounds();
}

void UIElement::RecalculateBounds()
{
	int screenW = Engine::GetInstance().window->windowWidth;
	int screenH = Engine::GetInstance().window->windowHeight;

	bounds.w = (int)(screenW * relW);
	bounds.h = (int)(screenH * relH);

	bounds.x = (int)(screenW * anchorX) - (int)(bounds.w * pivotX);
	bounds.y = (int)(screenH * anchorY) - (int)(bounds.h * pivotY);
}

bool UIElement::Update(float dt)
{
	return true;
}