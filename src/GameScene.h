#pragma once
#include "SceneBase.h"
#include "UIDialogueBox.h"
#include "Textures.h"
#include "Audio.h"
#include "SpecialFloors.h"
#include <vector>
#include <memory>

class UIElement;

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
	INV_DESC_TEXT,

	// SkillUpGrade Ui
	SKILL_BOOK_1_1 = 200,
	SKILL_BOOK_1_2,
	SKILL_BOOK_2_1,
	SKILL_BOOK_2_2,
	SKILL_BOOK_3_1,
	SKILL_BOOK_3_2

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

	//Textures
	void LoadTextureIfNull(SDL_Texture*& texture, const char* path);
	void UnloadTexture(SDL_Texture*& texture);
	
	//Buttons textures
	SDL_Texture* buttonUI = nullptr;
	SDL_Texture* skillFrameUI = nullptr;
	SDL_Texture* textBgUI = nullptr;

	// Texture BG
	SDL_Texture* texMapUI = nullptr;
	SDL_Texture* texInventoryUI = nullptr;
	SDL_Texture* texSkilltreeUI = nullptr;
	SDL_Texture* texPauseUI = nullptr;
	SDL_Texture* texSkillUI = nullptr;

	
	//Items Textures
	SDL_Texture* texItemKeyCastle = nullptr;
	SDL_Texture* texItemKeyForest = nullptr;
	SDL_Texture* texItemKeyMountain = nullptr;
	SDL_Texture* texItemKeyCatacumbs = nullptr;
	SDL_Texture* texItemOrb = nullptr;
	
	// Power-ups
	SDL_Texture* texItemGlide = nullptr;
	SDL_Texture* texItemDash = nullptr;
	SDL_Texture* texItemWallJump = nullptr;
	SDL_Texture* texItemDoubleJump = nullptr;
	SDL_Texture* texItemWeapon = nullptr;

	//SkillUpgrade
	SDL_Texture* books_1_1 = nullptr;
	SDL_Texture* books_1_2 = nullptr;
	SDL_Texture* books_2_1 = nullptr;
	SDL_Texture* books_2_2 = nullptr;
	SDL_Texture* books_3_1 = nullptr;
	SDL_Texture* books_3_2 = nullptr;
	SDL_Texture* orbsDisplayFrame = nullptr;

	//Dialogue UI Textures 
	SDL_Texture* UIDialogueBoxTex = nullptr;
	SDL_Texture* UIDialogueBoxNpc1 = nullptr;
	SDL_Texture* princessPortrait = nullptr;
	SDL_Texture* npcPortrait = nullptr;
	//SDL_Texture* npcPortrait1 = nullptr;
	//SDL_Texture* npcPortrait2 = nullptr;
	//SDL_Texture* npcPortrait3 = nullptr;
	//SDL_Texture* npcPortrait4 = nullptr;

private:
	// Helper functions for the Game Menu
	void ToggleGameMenu(GameMenuTab tab);
	void CreateTopBarUI();
	void CreateInventoryUI();
	void CreateSkillUpgradeUI();
	void CreateMiniMapUI();

	void CreatePauseMenuUI();
	void CreatePauseSettingUI() ;
	void CreateDialogueUI();

	void UpdateInventoryVisuals();
	void RefreshMenuUI();
	void SetUIGroupVisible(std::vector<std::shared_ptr<UIElement>>& group, bool visible);

	int uiClick;

private:

	// Current state of the game menu
	GameMenuTab currentMenuTab = GameMenuTab::NONE;

	// UI Groups for the different tabs
	std::vector<std::shared_ptr<UIElement>> topBarElements;
	std::vector<std::shared_ptr<UIElement>> inventoryUI;
	std::vector<std::shared_ptr<UIElement>> mapUI;
	std::vector<std::shared_ptr<UIElement>> skillUI;
	std::vector<std::shared_ptr<UIElement>> dialogueBox;

	// Descriptión Panel
	std::shared_ptr<UIElement> descPanel = nullptr;
	// Pause Vectos
	std::vector<std::shared_ptr<UIElement>> pauseMainUI;
	std::vector<std::shared_ptr<UIElement>> pauseOptionsUI;

	// Dialogue UI
	std::vector<std::shared_ptr<UIElement>> dialogueUI;
	//MapChanging Variables
	// Fade point
	enum class MapTransitionState {
		NONE,
		FADING_OUT,
		FADING_IN
	};

	MapTransitionState mapState = MapTransitionState::NONE;
	std::string nextMapName = "";
	float mapFadeTime = 500.0f;
};