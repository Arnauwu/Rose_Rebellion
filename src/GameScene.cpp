#include "GameScene.h"
#include "Engine.h"
#include "Input.h"
#include "Map.h"
#include "SceneManager.h"
#include "EntityManager.h"
#include "UIManager.h"
#include "window.h"
#include "Player.h"
#include "Log.h"

GameScene::GameScene() : SceneBase() {
}

GameScene::~GameScene() {
}
void GameScene::LoadMap(std::string mapFile)
{
	//Load the map. 
	if (mapFile == "")
	{
		mapFile = Engine::GetInstance().map->DoorInfo(player->interactuableBody);
	}
	Engine::GetInstance().sceneManager->setNewMap = false;

	Engine::GetInstance().entityManager->CleanUp();
	player = nullptr;
	Engine::GetInstance().sceneManager->SetPlayer(nullptr);

	Engine::GetInstance().map->CleanUp();

	Engine::GetInstance().map->Load("Assets/Maps/", mapFile);
	Engine::GetInstance().map->SpawnEntities();

	Engine::GetInstance().entityManager->Start();

}

bool GameScene::Start() {
	LOG("Loading Game Scene");

	LoadMap("MapTemplate.tmx");
	if (player != nullptr) {
		player->position.setX(10);
		player->position.setY(10);
	}

	// TOP BAR
	float wPerc = 0.15f;
	float hPerc = 0.05f;
	float startX = 0.275f;
	float yPos = 0.10f;

	auto uiManager = Engine::GetInstance().uiManager;

	Module* sceneObserver = (Module*)Engine::GetInstance().sceneManager.get();
	// Btn Inventory
	auto btnInv = uiManager->CreateUIElement(UIElementType::BUTTON, (int)GameUI_ID::BTN_TAB_INVENTORY, "INVENTORY", startX, yPos, wPerc, hPerc, sceneObserver);
	topBarElements.push_back(btnInv);

	// Btn Map
	auto btnMap = uiManager->CreateUIElement(UIElementType::BUTTON, (int)GameUI_ID::BTN_TAB_MAP, "MAP", startX + 0.15f, yPos, wPerc, hPerc, sceneObserver);
	topBarElements.push_back(btnMap);

	// Btn Skills
	auto btnSkills = uiManager->CreateUIElement(UIElementType::BUTTON, (int)GameUI_ID::BTN_TAB_SKILLS, "SKILLS", startX + 0.30f, yPos, wPerc, hPerc, sceneObserver);
	topBarElements.push_back(btnSkills);

	// Btn Settings
	auto btnSettings = uiManager->CreateUIElement(UIElementType::BUTTON, (int)GameUI_ID::BTN_TAB_SETTINGS, "SETTINGS", startX + 0.45f, yPos, wPerc, hPerc, sceneObserver);
	topBarElements.push_back(btnSettings);

	RefreshMenuUI();

	return true;
}


bool GameScene::Update(float dt) {
	auto input = Engine::GetInstance().input;

	// --- SUB-MENU INPUT HANDLING ---
	// Toggle menus based on keyboard shortcuts
	if (input->GetKey(SDL_SCANCODE_I) == KEY_DOWN) ToggleGameMenu(GameMenuTab::INVENTORY);
	if (input->GetKey(SDL_SCANCODE_M) == KEY_DOWN) ToggleGameMenu(GameMenuTab::MAP);
	if (input->GetKey(SDL_SCANCODE_N) == KEY_DOWN) ToggleGameMenu(GameMenuTab::SKILL_TREE);
	if (input->GetKey(SDL_SCANCODE_P) == KEY_DOWN) ToggleGameMenu(GameMenuTab::SETTINGS);

	// If any menu is open, freeze normal gameplay logic
	if (currentMenuTab != GameMenuTab::NONE) {

		int screenW, screenH;
		Engine::GetInstance().window->GetWindowSize(screenW, screenH);
		SDL_Rect bgRect = { 0, 0, screenW, screenH };

		Engine::GetInstance().render->DrawRectangle(bgRect, 0, 0, 0, 180, true, false);

		return true;
	}

	// --- NORMAL GAMEPLAY LOGIC ---

	return true;
}

bool GameScene::PostUpdate() {

	// If the player has touched a door, the engine will have set this variable to true
	if (Engine::GetInstance().sceneManager->setNewMap == true) {
		// Loading function with an empty string so that it reads the door information
		LoadMap("");
	}

	return true;
}
bool GameScene::CleanUp() {
	LOG("Freeing Game Scene");

	// Clean up map and entities
	Engine::GetInstance().entityManager->CleanUp();
	Engine::GetInstance().map->CleanUp();

	// Clean up UI vectors
	topBarElements.clear();
	inventoryUI.clear();
	mapUI.clear();
	skillUI.clear();
	settingsUI.clear();

	return true;
}

bool GameScene::OnUIMouseClickEvent(UIElement* uiElement) {

	switch (uiElement->id) {
	case (int)GameUI_ID::BTN_TAB_INVENTORY:
		ToggleGameMenu(GameMenuTab::INVENTORY);
		break;

	case (int)GameUI_ID::BTN_TAB_MAP:
		ToggleGameMenu(GameMenuTab::MAP);
		break;

	case (int)GameUI_ID::BTN_TAB_SKILLS:
		ToggleGameMenu(GameMenuTab::SKILL_TREE);
		break;

	case (int)GameUI_ID::BTN_TAB_SETTINGS:
		ToggleGameMenu(GameMenuTab::SETTINGS);
		break;

	case (int)GameUI_ID::BTN_RESUME:
		ToggleGameMenu(GameMenuTab::NONE);
		break;

	default:
		break;
	}
	return true;
}

// ==========================================
// SUB-MENU LOGIC
// ==========================================

void GameScene::ToggleGameMenu(GameMenuTab tab) {
	// If we press the key of the currently open tab, close it
	if (currentMenuTab == tab) {
		currentMenuTab = GameMenuTab::NONE;
	}
	else {
		// Otherwise, switch to the requested tab
		currentMenuTab = tab;
	}
	RefreshMenuUI();
}

void GameScene::RefreshMenuUI() {
	// Show/Hide top bar based on whether ANY menu is open
	bool menuOpen = (currentMenuTab != GameMenuTab::NONE);
	SetUIGroupVisible(topBarElements, menuOpen);

	// Hide all tabs first
	SetUIGroupVisible(inventoryUI, false);
	SetUIGroupVisible(mapUI, false);
	SetUIGroupVisible(skillUI, false);
	SetUIGroupVisible(settingsUI, false);

	// Show only the active tab
	switch (currentMenuTab) {
	case GameMenuTab::INVENTORY:
		SetUIGroupVisible(inventoryUI, true);
		break;
	case GameMenuTab::MAP:
		SetUIGroupVisible(mapUI, true);
		break;
	case GameMenuTab::SKILL_TREE:
		SetUIGroupVisible(skillUI, true);
		break;
	case GameMenuTab::SETTINGS:
		SetUIGroupVisible(settingsUI, true);
		break;
	case GameMenuTab::NONE:       break;
	}
}

void GameScene::SetUIGroupVisible(std::vector<std::shared_ptr<UIElement>>& group, bool visible) {
	for (auto& elem : group) {
		elem->visible = visible;
		// UIElementState enum
		elem->state = visible ? UIElementState::NORMAL : UIElementState::DISABLED;
	}
}

// ==========================================
// BRIDGES FOR PLAYER
// ==========================================

Vector2D GameScene::GetPlayerPosition() {
	if (player != nullptr) return player->position;
	return Vector2D(0, 0);
}

Player* GameScene::GetPlayer() {
	return player;
}

void GameScene::SetPlayer(Player* p) {
	player = p;
}

