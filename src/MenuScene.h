#pragma once
#include "SceneBase.h"
#include "Textures.h"
#include "Animation.h"
#include "Audio.h"
#include <vector>
#include <memory>

struct SDL_Texture;

enum class MenuUI_ID {
    BTN_PLAY = 1,
    BTN_CONTINUE,
    BTN_SETTINGS,
    BTN_EXIT,
    SLD_MUSIC,
    SLD_FX,
    CHK_FULLSCREEN,
    BTN_BACK
};

class MenuScene : public SceneBase {
public:
    MenuScene();
    virtual ~MenuScene();

    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;
    bool OnUIMouseClickEvent(UIElement* uiElement) override;

    float bgScaleX = 1.0f;
    float bgScaleY = 1.0f;

    SDL_Texture* menuBackground = nullptr;
    SDL_Texture* menuBackground_S = nullptr;
    SDL_Texture* frameTex = nullptr;

private:
    void ShowSettings(bool show);
    bool isSettingsOpen = false;

    int uiClick;

private:
    // UI Groups
    std::vector<std::shared_ptr<UIElement>> mainButtons;
    std::vector<std::shared_ptr<UIElement>> settingsButtons;
    AnimationSet anims;
    SDL_Texture* texture;
};