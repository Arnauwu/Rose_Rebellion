#include "UIButton.h"
#include "Render.h"
#include "Engine.h"
#include "Audio.h"
#include "ParticleManager.h"

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
			mousePos.getY() > bounds.y && mousePos.getY() < bounds.y + bounds.h) 
		{
	
			if (Engine::GetInstance().input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_REPEAT) {
				state = UIElementState::PRESSED;
				targetTextScale = 0.9f;
				currentTextColor = { 50, 50, 50, 255 };
				// PRUEBAS PARTICULAS
				float tamañoDestello = 50.0f;
				// CON ANIMACIÓN
				if (useFlashAnim && flashTexture != nullptr) {
					Engine::GetInstance().particleManager->Emit(
						flashTexture, flashAnim,
						mousePos.getX() - (tamañoDestello * 3 / 2),
						mousePos.getY() - (tamañoDestello * 3 / 2),
						0.0f, 0.0f,
						250.0f, tamañoDestello *3, false, 0.0f
					);
				}
				// TEXTURA
				else if (flashTexture != nullptr) {
					Engine::GetInstance().particleManager->Emit(
						flashTexture,
						mousePos.getX() - (tamañoDestello / 2),
						mousePos.getY() - (tamañoDestello / 2),
						0.0f, 0.0f,
						250.0f, tamañoDestello, false, 0.0f
					);
				}
				// SI NO LE PASO NADA
				else {
					Engine::GetInstance().particleManager->Emit(
						mousePos.getX() - (tamañoDestello / 2),
						mousePos.getY() - (tamañoDestello / 2),
						0.0f, 0.0f,
						200.0f, { 255, 255, 255, 200 }, tamañoDestello, false
					);
				}
			}
			else {
				state = UIElementState::FOCUSED;
				targetTextScale = 1.15f;
				currentTextColor = { 255, 255, 255, 255 };
			}
			if (Engine::GetInstance().input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_UP) {
				NotifyObserver();
			}
		}
		else {
			state = UIElementState::NORMAL;
			targetTextScale = 1.0f;
			currentTextColor = { 255, 255, 255, 255 };
		}
	}
	float lerpSpeed = 15.0f * (dt / 1000.0f);
	currentTextScale += (targetTextScale - currentTextScale) * lerpSpeed;

	return true;
}
void UIButton::Draw() const
{
	if (!visible) return;
	//Engine::GetInstance().render->DrawRectangle(bounds, 200, 200, 200, 255, true, false);

	SDL_Rect textBounds = bounds;
	textBounds.w = (int)(bounds.w * currentTextScale);
	textBounds.h = (int)(bounds.h * currentTextScale);
	textBounds.x = bounds.x - (textBounds.w - bounds.w) / 2;
	textBounds.y = bounds.y - (textBounds.h - bounds.h) / 2;

	if (drawBasic) {
		SDL_Color bgColor = { 100, 100, 100, 255 };
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

	if (state == UIElementState::FOCUSED && frameTexture != nullptr && !text.empty()) {

		SDL_Rect exactTextBounds = Engine::GetInstance().render->GetTextRenderedBounds(text.c_str(), textBounds, FontType::MENU);

		int paddingHorizontal = 135; 
		int paddingVertical = 20; 

		exactTextBounds.x -= paddingHorizontal;
		exactTextBounds.w += paddingHorizontal * 2;

		exactTextBounds.y -= paddingVertical;
		exactTextBounds.h += paddingVertical * 2;

		Engine::GetInstance().render->DrawTextureScaled(frameTexture, exactTextBounds);
	}
	if (!text.empty()) {
		Engine::GetInstance().render->DrawTextCentered(text.c_str(), textBounds, currentTextColor, FontType::MENU);
	}
}


bool UIButton::CleanUp()
{
	pendingToDelete = true;
	return true;
}