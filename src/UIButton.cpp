#include "UIButton.h"
#include "Render.h"
#include "Engine.h"
#include "Audio.h"

UIButton::UIButton(int id, float anchorX, float anchorY, float wPerc, float hPerc, const char* text)
	: UIElement(UIElementType::BUTTON, id, anchorX, anchorY, wPerc, hPerc, text)
{
	canClick = true;
	drawBasic = false;
}

UIButton::~UIButton()
{

}

bool UIButton::Update(float dt)
{
	if (!visible) return false;

	if (state != UIElementState::DISABLED)
	{
		// L16: TODO 3: Update the state of the GUiButton according to the mouse position
		Vector2D mousePos = Engine::GetInstance().input->GetMousePosition();

		if (mousePos.getX() > bounds.x && mousePos.getX() < bounds.x + bounds.w &&
			mousePos.getY() > bounds.y && mousePos.getY() < bounds.y + bounds.h) {

			state = UIElementState::FOCUSED;

			if (Engine::GetInstance().input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_REPEAT) {
				state = UIElementState::PRESSED;
			}

			if (Engine::GetInstance().input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_UP) {
				NotifyObserver();
			}
		}
		else {
			state = UIElementState::NORMAL;
		}
	}
	return true;
}
void UIButton::Draw() const
{
	if (!visible) return;

	// Draw Bg button
	SDL_Color color = { 100, 100, 100, 255 }; // Normal
	if (state == UIElementState::FOCUSED) color = { 150, 150, 150, 255 };
	if (state == UIElementState::PRESSED) color = { 200, 200, 200, 255 };

	Engine::GetInstance().render->DrawRectangle(bounds, color.r, color.g, color.b, color.a, true, false);

	if (!text.empty()) {
		Engine::GetInstance().render->DrawTextCentered(text.c_str(), bounds, { 255, 255, 255, 255 });
	}
}

bool UIButton::CleanUp()
{
	pendingToDelete = true;
	return true;
}