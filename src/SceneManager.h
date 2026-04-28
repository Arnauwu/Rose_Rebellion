#pragma once

#include "Module.h"
#include "Player.h"
#include "UIButton.h"
#include "UICheckBox.h"
#include "UISlider.h"
#include "SceneBase.h"
#include <set>
#include <string>

enum class SceneID
{
    INTRO,
    MENU,
    INTRO_CINEMATIC,
    GAME,
    GAMEOVER,
    WIN,
    NONE
};

class SceneManager : public Module
{
public:
    SceneManager();
    virtual ~SceneManager();

    // Module overrides
    bool Awake() override;
    bool Start() override;
    bool PreUpdate() override;
    bool Update(float dt) override;
    bool PostUpdate() override;
    bool CleanUp() override;

    // Changes the current active scene
    void ChangeScene(SceneID newScene, float fadeTime = 800.0f);

    // Module UI override
    bool OnUIMouseClickEvent(UIElement* uiElement) override;

    bool IsGamePaused() const { return isGamePaused; }
    void SetGamePaused(bool paused) { isGamePaused = paused; }

    bool isGamePaused = false;
public:
    SceneBase* GetCurrentScene() const { return currentScene; }
private:
    void PerformSceneChange();
private:
    // The currently active scene object
    SceneID nextSceneID = SceneID::NONE;
    bool isFadingOut = false;
    float currentFadeTime = 800.0f;

    // Tracks the current ID
    SceneBase* currentScene = nullptr;
    SceneID currentSceneID = SceneID::NONE;
};