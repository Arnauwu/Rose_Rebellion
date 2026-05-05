#include "DialogueBox.h"
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

bool UIDialogueBox::Draw() {
    if (!visible) return true;

    LOG("Dibujando Caja Dialogo -> X:%d, Y:%d, W:%d, H:%d", bounds.x, bounds.y, bounds.w, bounds.h);

    if (backgroundTex != nullptr) {
        Engine::GetInstance().render->DrawTextureScaled(backgroundTex, bounds);
    }
    else {
        Engine::GetInstance().render->DrawRectangle(bounds, 0, 0, 0, 200, true, false);
    }

    if (!currentSpeaker.empty()) {
        int nameX = bounds.x + 30;
        int nameY = bounds.y + 20;
        Engine::GetInstance().render->DrawText(currentSpeaker.c_str(), nameX, nameY, 0, 0, speakerColor);
    }

    if (!currentText.empty()) {
        int textX = bounds.x + 30;
        int textY = bounds.y + 70;
        Engine::GetInstance().render->DrawText(currentText.c_str(), textX, textY, 0, 0, textColor);
    }

    return true; 
}
void UIDialogueBox::SetSpeakerName(const std::string& name) { currentSpeaker = name; }
void UIDialogueBox::SetDialogueText(const std::string& text) { currentText = text; }
void UIDialogueBox::SetBackgroundTexture(SDL_Texture* tex) { backgroundTex = tex; }