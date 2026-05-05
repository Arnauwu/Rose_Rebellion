#include "UIDialogueBox.h"
#include "Engine.h"
#include "Render.h"
#include "LOG.h"

UIDialogueBox::UIDialogueBox(int id, float anchorX, float anchorY, float wPercent, float hPercent, const char* text)
	: UIElement(UIElementType::DIALOGUE_BOX, id, anchorX, anchorY, wPercent, hPercent, text) {
	visible = false;
}

UIDialogueBox::~UIDialogueBox() {}

bool UIDialogueBox::Update(float dt) {
	if (!visible) return true;
	return true;
}

void UIDialogueBox::Draw() const {
	if (!visible) return;

	if (backgroundTex != nullptr) {
		Engine::GetInstance().render->DrawTextureScaled(backgroundTex, bounds);
	}
	else {
		Engine::GetInstance().render->DrawRectangle(bounds, 0, 0, 0, 200, true, false);
	}

	if (!currentSpeaker.empty()) {
		int nameX;
		int nameY = bounds.y + 20;

		if (currentSpeaker == "Princesa" || currentSpeaker == "Princess") {
			nameX = bounds.x + 30;
		}
		else {
			SDL_Rect nameRect = Engine::GetInstance().render->GetTextRenderedBounds(currentSpeaker.c_str(), bounds, FontType::SPEAKER);
			nameX = (bounds.x + bounds.w) - nameRect.w - 30;
		}

		Engine::GetInstance().render->DrawText(currentSpeaker.c_str(), nameX, nameY, 0, 0, speakerColor, FontType::SPEAKER);
	}

	if (!currentText.empty()) {
		int textX = bounds.x + 30;
		int textY = bounds.y + 80;

		int maxW = bounds.w - 60;
		int maxH = bounds.h - 100;

		Engine::GetInstance().render->DrawText(currentText.c_str(), textX, textY, maxW, maxH, textColor, FontType::DIALOGUE);
	}
}

void UIDialogueBox::SetSpeakerName(const std::string& name) { currentSpeaker = name; }
void UIDialogueBox::SetDialogueText(const std::string& text) { currentText = text; }
void UIDialogueBox::SetBackgroundTexture(SDL_Texture* tex) { backgroundTex = tex; }