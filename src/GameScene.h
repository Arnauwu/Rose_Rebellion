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

	PAUSE_MENU,
	PAUSE_OPTIONS
};

enum class GameUI_ID {
	// Top Bar Tabs
	BTN_TAB_MAP = 100,
	BTN_TAB_INVENTORY,
	BTN_TAB_SKILLS,

	// Pause Menu
	BTN_PAUSE_RESUME,
	BTN_PAUSE_OPTIONS,
	BTN_PAUSE_MAINMENU,

	// Pause Options
	SLD_MUSIC,
	SLD_FX,
	CHK_FULLSCREEN,
	BTN_OPTIONS_BACK,

	// Inventory Ui
	INV_ITEM_WEAPON = 150,
	INV_ITEM_GLIDE,
	INV_ITEM_DASH,
	INV_ITEM_DOUBLE_JUMP,
	INV_ITEM_WALL_JUMP,
	INV_ITEM_KEY,
	INV_ITEM_ORB,
	INV_DESC_TEXT
};

struct InventorySlot {
	GameUI_ID id;
	const char* name;
	const char* description;
	float relX, relY;
	bool isUnlocked = false; // Esto debería venir de tu clase Player
};

struct ButtonDef { int id; const char* text; };

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

	//Textures
	void LoadTextureIfNull(SDL_Texture*& texture, const char* path);
	void UnloadTexture(SDL_Texture*& texture);

	SDL_Texture* t_mapUI = nullptr;
	SDL_Texture* t_inventoryUI = nullptr;
	SDL_Texture* t_skilltreeUI = nullptr;
	SDL_Texture* t_pauseUI = nullptr;

private:
	// Helper functions for the Game Menu
	void ToggleGameMenu(GameMenuTab tab);
	void CreateTopBarUI();
	void CreateInventoryUI();
	void CreatePauseMenuUI();
	void CreatePauseSettingUI() ;

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

	// Pause Vectos
	std::vector<std::shared_ptr<UIElement>> pauseMainUI;
	std::vector<std::shared_ptr<UIElement>> pauseOptionsUI;
};