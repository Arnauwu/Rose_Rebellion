#pragma once
#include "UIElement.h"
#include <string>

class UIDialogueBox : public UIElement {
public:
    UIDialogueBox(int id, float anchorX, float anchorY, float wPercent, float hPercent, const char* text);
    ~UIDialogueBox();

    bool Update(float dt) override;
    void Draw() const override;

    void SetSpeakerName(const std::string& name);
    void SetDialogueText(const std::string& text);
    void SetBackgroundTextures(SDL_Texture* princessTex, SDL_Texture* npcTex);

private:
    std::string currentSpeaker;
    std::string currentText;

    SDL_Texture* texPrincess = nullptr;
    SDL_Texture* texNPC = nullptr;

    SDL_Color textColor = { 255, 255, 255, 255 };    
    SDL_Color speakerColor = { 255, 204, 0, 255 };     
};