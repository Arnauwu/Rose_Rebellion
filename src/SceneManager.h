#pragma once

#include "Module.h"
#include "Player.h"
#include "UIButton.h"
#include "UICheckBox.h"
#include "UISlider.h"
#include "SceneBase.h"


enum class SceneID
{
    INTRO,
    MENU,
    GAME,
    GAMEOVER,
    WIN
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
    void ChangeScene(SceneID newScene);

    // Module UI override
    bool OnUIMouseClickEvent(UIElement* uiElement) override;

    // --- NEW: Bridge methods to satisfy old code ---
    Vector2D GetPlayerPosition();
    Player* GetPlayer();
    void SetPlayer(Player* p);

    // The boolean you were using to trigger map changes
    bool setNewMap = false;
private:
    // The currently active scene object
    SceneBase* currentScene = nullptr;

    // Tracks the current ID
    SceneID currentSceneID;
};