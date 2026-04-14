#pragma once
#include "SceneBase.h"
#include "Textures.h"
#include "Audio.h"
#include <vector>
#include <memory>

class UIElement;
class Player;

// Different sub-menus inside the game
enum class GameMenuTab {
    NONE,       
    INVENTORY,
    MAP,
    SKILL_TREE,
    SETTINGS
};

enum class GameUI_ID {
    BTN_TAB_INVENTORY = 100,
    BTN_TAB_MAP,
    BTN_TAB_SKILLS,
    BTN_TAB_SETTINGS,
    BTN_RESUME 
};

struct SDL_Texture;

class GameScene : public SceneBase {
public:
    GameScene();
    virtual ~GameScene();

    //Load Map
    void LoadMap(std::string mapFile);

    // Scene lifecycle
    bool Start() override;
    bool Update(float dt) override;
    bool PostUpdate() override;
    bool CleanUp() override;

    // UI Event delegation
    bool OnUIMouseClickEvent(UIElement* uiElement) override;

    // Bridge methods for Map, HUD and Enemies
    Vector2D GetPlayerPosition() override;
    Player* GetPlayer() override;
    void SetPlayer(Player* p) override;

    SDL_Texture* gameMenuBackground = nullptr;

private:
    // Helper functions for the Game Menu
    void ToggleGameMenu(GameMenuTab tab);
    void RefreshMenuUI();
    void SetUIGroupVisible(std::vector<std::shared_ptr<UIElement>>& group, bool visible);

private:
    // The player pointer
    Player* player = nullptr;

    // Current state of the game menu
    GameMenuTab currentMenuTab = GameMenuTab::NONE;

    // UI Groups for the different tabs
    std::vector<std::shared_ptr<UIElement>> topBarElements; 
    std::vector<std::shared_ptr<UIElement>> inventoryUI;
    std::vector<std::shared_ptr<UIElement>> mapUI;
    std::vector<std::shared_ptr<UIElement>> skillUI;
    std::vector<std::shared_ptr<UIElement>> settingsUI;
};