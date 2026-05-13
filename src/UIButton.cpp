#include "UIButton.h"
#include "Render.h"
#include "Engine.h"
#include "Audio.h"
#include "Input.h"
#include "Textures.h"

#include "ParticleManager.h"

UIButton::UIButton(int id, float anchorX, float anchorY, float wPerc, float hPerc, const char* text)
	: UIElement(UIElementType::BUTTON, id, anchorX, anchorY, wPerc, hPerc, text)
{
	canClick = true;
	drawBasic = false;

	isPressedInternally = false;
	showClickAnim = false;
	wasFocused = false;
	pendingAction = false;

	std::unordered_map<int, std::string> frameAliases = { {0, "focus"} };
	frameAnimSet.LoadFromTSX("Assets/Textures/UI/Buttons/frameTex.tsx", frameAliases);
	if (frameAnimSet.GetAnim("focus")) frameAnimSet.GetAnim("focus")->SetLoop(false);

	std::unordered_map<int, std::string> clickAliases = { {0, "click"} };
	clickAnimSet.LoadFromTSX("Assets/Textures/UI/Buttons/clickAnim.tsx", clickAliases);
	if (clickAnimSet.GetAnim("click")) clickAnimSet.GetAnim("click")->SetLoop(false);
}

UIButton::~UIButton()
{

}

bool UIButton::Update(float dt)
{
	if (!visible) return false;

	if (state != UIElementState::DISABLED)
	{
		Vector2D mousePos = Engine::GetInstance().input->GetMousePosition();
		bool mouseInside = (mousePos.getX() > bounds.x && mousePos.getX() < bounds.x + bounds.w &&
			mousePos.getY() > bounds.y && mousePos.getY() < bounds.y + bounds.h);

		auto input = Engine::GetInstance().input;
		KeyState leftClick = input->GetMouseButtonDown(SDL_BUTTON_LEFT);

		// 1. GESTIÓN DEL CLIC
		if (mouseInside && leftClick == KEY_DOWN)
		{
			isPressedInternally = true;
		}

		if (leftClick == KEY_UP)
		{
			if (isPressedInternally && mouseInside) {

				if (clickAnimSet.GetAnim("click") != nullptr) {
					showClickAnim = true;
					pendingAction = true;

					clickAnimSet.GetAnim("click")->SetLoop(false);
					clickAnimSet.GetAnim("click")->Reset();
					clickAnimSet.SetCurrent("click");
				}
				else {
					NotifyObserver();
				}
			}
			isPressedInternally = false;
		}

		if (leftClick == KEY_IDLE) {
			isPressedInternally = false;
		}

		if (isPressedInternally)
		{
			state = mouseInside ? UIElementState::PRESSED : UIElementState::NORMAL;
		}
		else
		{
			if (mouseInside)
			{
				if (leftClick == KEY_IDLE || leftClick == KEY_UP) {
					state = UIElementState::FOCUSED;
				}
				else {
					state = UIElementState::NORMAL;
				}
			}
			else
			{
				state = UIElementState::NORMAL;
			}
		}

		bool isVisuallyActive = (state == UIElementState::FOCUSED || state == UIElementState::PRESSED);

		if (isVisuallyActive && !wasFocused)
		{
			if (frameAnimSet.GetAnim("focus") != nullptr) {
				frameAnimSet.GetAnim("focus")->Reset();
			}
			frameAnimSet.SetCurrent("focus");
		}

		wasFocused = isVisuallyActive;

		if (state == UIElementState::PRESSED) {
			targetTextScale = 0.8f;
			currentTextColor = { 255, 255, 255, 255 };
		}
		else if (state == UIElementState::FOCUSED) {
			targetTextScale = 1.0f;
			currentTextColor = { 255, 255, 255, 255 };
		}
		else {
			targetTextScale = 0.8f;
			currentTextColor = { 255, 255, 255, 255 };
		}
	}
	float lerpSpeed = 15.0f * (dt / 1000.0f);
	currentTextScale += (targetTextScale - currentTextScale) * lerpSpeed;

	if (state == UIElementState::FOCUSED || state == UIElementState::PRESSED)
	{
		frameAnimSet.Update(dt);
	}

	if (showClickAnim)
	{
		clickAnimSet.Update(dt);

		if (clickAnimSet.GetAnim("click") != nullptr && clickAnimSet.GetAnim("click")->HasFinishedOnce())
		{
			showClickAnim = false;

			if (pendingAction) {
				NotifyObserver();
				pendingAction = false;
			}
		}
	}

	return true;
}

void UIButton::Draw() const
{
	if (!visible) return;

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

	SDL_Renderer* renderer = Engine::GetInstance().render->renderer;

	if ((state == UIElementState::FOCUSED || state == UIElementState::PRESSED) && frameTexture != nullptr && !text.empty())
	{
		SDL_Rect exactTextBounds = Engine::GetInstance().render->GetTextRenderedBounds(text.c_str(), textBounds, FontType::MENU);

		int paddingHorizontal = 135;
		int paddingVertical = 20;

		exactTextBounds.x -= paddingHorizontal;
		exactTextBounds.w += paddingHorizontal * 2;
		exactTextBounds.y -= paddingVertical;
		exactTextBounds.h += paddingVertical * 2;

		SDL_Rect srcFrame = frameAnimSet.GetCurrentFrame();

		SDL_FRect srcFRect = { (float)srcFrame.x, (float)srcFrame.y, (float)srcFrame.w, (float)srcFrame.h };
		SDL_FRect dstFRect = { (float)exactTextBounds.x, (float)exactTextBounds.y, (float)exactTextBounds.w, (float)exactTextBounds.h };

		SDL_RenderTexture(renderer, frameTexture, &srcFRect, &dstFRect);
	}

	if (showClickAnim && clickTexture != nullptr)
	{
		SDL_Rect clickDestBounds = bounds;
		int paddingHorizontal = 135;
		int paddingVertical = 20;

		clickDestBounds.x -= paddingHorizontal;
		clickDestBounds.w += paddingHorizontal * 2;
		clickDestBounds.y -= paddingVertical;
		clickDestBounds.h += paddingVertical * 2;

		SDL_Rect srcClickFrame = clickAnimSet.GetCurrentFrame();

		SDL_FRect srcFRect = { (float)srcClickFrame.x, (float)srcClickFrame.y, (float)srcClickFrame.w, (float)srcClickFrame.h };
		SDL_FRect dstFRect = { (float)clickDestBounds.x, (float)clickDestBounds.y, (float)clickDestBounds.w, (float)clickDestBounds.h };

		SDL_RenderTexture(renderer, clickTexture, &srcFRect, &dstFRect);
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