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
        // Usar DrawText con un tamaño controlado
        int nameX = bounds.x + (bounds.w * 0.05f); 
        int nameY = bounds.y + (bounds.h * 0.1f);
        Engine::GetInstance().render->DrawText(currentSpeaker.c_str(), nameX, nameY, 0, 0, speakerColor, FontType::SPEAKER);
    }

    // 3. Cuerpo del Diálogo
    if (!currentText.empty()) {
        int textX = bounds.x + (bounds.w * 0.05f);
        int textY = bounds.y + (bounds.h * 0.4f);
        int maxW = bounds.w - (bounds.w * 0.1f); // 90% del ancho de la caja

        // Pasamos maxW para que el motor sepa dónde cortar o escalar
        Engine::GetInstance().render->DrawText(currentText.c_str(), textX, textY, maxW, 0, textColor, FontType::DIALOGUE);
    }
}
void UIDialogueBox::SetSpeakerName(const std::string& name) { currentSpeaker = name; }
void UIDialogueBox::SetDialogueText(const std::string& text) { currentText = text; }
void UIDialogueBox::SetBackgroundTexture(SDL_Texture* tex) { backgroundTex = tex; }