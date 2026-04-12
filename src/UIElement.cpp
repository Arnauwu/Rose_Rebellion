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
	int screenW, screenH;
	Engine::GetInstance().window->GetWindowSize(screenW, screenH);

	bounds.w = (int)(screenW * relW);
	bounds.h = (int)(screenH * relH);

	bounds.x = (int)(screenW * anchorX) - (bounds.w / 2);
	bounds.y = (int)(screenH * anchorY) - (bounds.h / 2);
}

bool UIElement::Update(float dt)
{
	RecalculateBounds();
	return true;
}