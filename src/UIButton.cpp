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

	if (drawBasic) {
        SDL_Color bgColor = { 100, 100, 100, 255 };
        if (state == UIElementState::FOCUSED) bgColor = { 150, 150, 150, 255 };
        if (state == UIElementState::PRESSED) bgColor = { 200, 200, 200, 255 };
        Engine::GetInstance().render->DrawRectangle(bounds, bgColor.r, bgColor.g, bgColor.b, bgColor.a, true, false);
    }
	if (bgTexture != nullptr) {
		Engine::GetInstance().render->DrawTextureScaled(bgTexture, bounds);
	}

	if (texture != nullptr) {
		SDL_SetTextureColorMod(texture, this->color.r, this->color.g, this->color.b);

		Engine::GetInstance().render->DrawTextureScaled(texture, bounds);

		SDL_SetTextureColorMod(texture, 255, 255, 255);
	}

	if (!text.empty()) {
		Engine::GetInstance().render->DrawTextCentered(text.c_str(), bounds, { 255, 255, 255, 255 });
	}
}


bool UIButton::CleanUp()
{
	pendingToDelete = true;
	return true;
}