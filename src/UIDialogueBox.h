#pragma once
#include "UIElement.h"
#include <string>
#include <unordered_map>

class UIDialogueBox : public UIElement {
public:
    UIDialogueBox(int id, float anchorX, float anchorY, float wPercent, float hPercent, const char* text);
    ~UIDialogueBox();

    bool Update(float dt) override;
    void Draw() const override;
    bool CleanUp() override;

    void AddPortrait(const std::string& speakerName, SDL_Texture* portraitTex);
    void SetSpeakerName(const std::string& name);
    void SetDialogueText(const std::string& text);
    void SetBackgroundTexture(SDL_Texture* bgTex);

private:
    std::string currentSpeaker = "";
    std::string currentText = "";

    std::unordered_map<std::string, SDL_Texture*> portraits;
    SDL_Texture* currentPortrait = nullptr;
    SDL_Texture* backgroundTex = nullptr;

    SDL_Rect cachedNameTextRect = { 0, 0, 0, 0 };
    SDL_Color textColor = { 255, 255, 255, 255 };    
    SDL_Color speakerColor = { 255, 204, 0, 255 };     
};