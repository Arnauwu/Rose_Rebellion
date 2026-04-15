#pragma once
#include "SceneBase.h"
#include "Textures.h"
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

    SDL_Texture* menuBackground = nullptr;

private:
    void ShowSettings(bool show);

  /*  float bgScaleX = 1.0f;
    float bgScaleY = 1.0f;
    void RecalculateBackgroundScale();*/

private:
    // UI Groups
    std::vector<std::shared_ptr<UIElement>> mainButtons;
    std::vector<std::shared_ptr<UIElement>> settingsButtons;
};