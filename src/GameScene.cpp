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
#include "Textures.h"

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

	LoadMap("Castle.tmx");
	if (player != nullptr) {
		player->position.setX(10);
		player->position.setY(10);
	}
	// Texture Load
	LoadTextureIfNull(t_mapUI, "Assets/Textures/UI/GameMenu/t_mapUI.png");
	LoadTextureIfNull(t_inventoryUI, "Assets/Textures/UI/GameMenu/t_inventoryUI.png");
	LoadTextureIfNull(t_skilltreeUI, "Assets/Textures/UI/GameMenu/t_skilltreeUI.png");
	LoadTextureIfNull(t_pauseUI, "Assets/Textures/UI/GameMenu/t_pauseUI.png");

	// ==========================================
	// TOP BAR
	// ==========================================
	auto uiManager = Engine::GetInstance().uiManager;
	Module* sceneObserver = (Module*)Engine::GetInstance().sceneManager.get();

	struct ButtonDef { int id; const char* text; };

	float wPerc = 0.15f, hPerc = 0.05f;
	float startX = 0.35f, yPos = 0.10f;
	float spacingX = 0.15f;

	ButtonDef topBarDefs[] = {
		{ (int)GameUI_ID::BTN_TAB_INVENTORY, "INVENTORY" },
		{ (int)GameUI_ID::BTN_TAB_MAP,       "MAP" },
		{ (int)GameUI_ID::BTN_TAB_SKILLS,    "SKILLS" }
	};


	for (const auto& def : topBarDefs) {
		topBarElements.push_back(uiManager->CreateUIElement(UIElementType::BUTTON, def.id, def.text, startX, yPos, wPerc, hPerc, sceneObserver));
		startX += spacingX;
	}

	// ==========================================
	// INVENTORY UI (Sub-Menú)
	// ==========================================
	int screenW, screenH;
	Engine::GetInstance().window->GetWindowSize(screenW, screenH);

	float itemSizeW = 0.08f; // Botones cuadrados
	float itemSizeH = 0.08f * ((float)screenW / screenH); // Ajuste de aspecto

	float startInvX = 0.15f; // Mitad izquierda de la pantalla
	float startInvY = 0.30f;
	float spacing = 0.20f;

	// Estructura para crear nuestros ítems rápido
	struct InvItemDef { int id; const char* text; float x; float y; };

	InvItemDef invItems[] = {
		{ (int)GameUI_ID::INV_ITEM_KEY,   "Key",  startInvX, startInvY },
		{ (int)GameUI_ID::INV_ITEM_ORB,   "Orb",  startInvX + spacing, startInvY },
		{ (int)GameUI_ID::INV_ITEM_GLIDE,  "Dash", startInvX, startInvY + spacing }
	};

	for (const auto& def : invItems) {
		auto itemBtn = uiManager->CreateUIElement(
			UIElementType::BUTTON, def.id, def.text,
			def.x, def.y, itemSizeW, itemSizeH, sceneObserver
		);
		inventoryUI.push_back(itemBtn);
	}

	// PANEL DE DESCRIPCIÓN (A la derecha)
	auto descPanel = uiManager->CreateUIElement(
		UIElementType::BUTTON, (int)GameUI_ID::INV_DESC_TEXT, "Select an item...",
		0.70f, 0.40f, 0.30f, 0.40f, sceneObserver // Más grande, a la derecha
	);
	// Hacemos que no sea "clicable" si solo es un panel de texto
	descPanel->state = UIElementState::DISABLED;
	inventoryUI.push_back(descPanel);

	// --- PAUSE MENU ---
	float pW = 0.25f, pH = 0.08f;
	float pY = 0.35f, pSpacing = 0.1f;

	ButtonDef pauseBtnDefs[] = {
		{ (int)GameUI_ID::BTN_PAUSE_RESUME,   "RESUME" },
		{ (int)GameUI_ID::BTN_PAUSE_OPTIONS,  "OPTIONS" },
		{ (int)GameUI_ID::BTN_PAUSE_MAINMENU, "MAIN MENU" }
	};

	for (const auto& def : pauseBtnDefs) {
		pauseMainUI.push_back(uiManager->CreateUIElement(UIElementType::BUTTON, def.id, def.text, 0.5f, pY, pW, pH, sceneObserver));
		pY += pSpacing; 
	}

	// --- PAUSE OPTIONS ---
	float oY = 0.35f;
	auto sldMus = uiManager->CreateUIElement(UIElementType::SLIDER, (int)GameUI_ID::SLD_MUSIC, "Music", 0.5f, oY, 0.3f, 0.05f, sceneObserver);
	if (auto* s = dynamic_cast<UISlider*>(sldMus.get())) s->SetValue(Engine::GetInstance().audio->GetMusicVolume());
	pauseOptionsUI.push_back(sldMus);
	oY += pSpacing;
	auto sldFx = uiManager->CreateUIElement(UIElementType::SLIDER, (int)GameUI_ID::SLD_FX, "FX", 0.5f, oY, 0.3f, 0.05f, sceneObserver);
	if (auto* s = dynamic_cast<UISlider*>(sldFx.get())) s->SetValue(Engine::GetInstance().audio->GetSFXVolume());
	pauseOptionsUI.push_back(sldFx);
	oY += pSpacing;
	auto chkFull = uiManager->CreateUIElement(UIElementType::CHECKBOX, (int)GameUI_ID::CHK_FULLSCREEN, "Fullscreen", 0.5f, oY, 0.05f, 0.05f, sceneObserver);
	if (auto* c = dynamic_cast<UICheckBox*>(chkFull.get())) c->isChecked = Engine::GetInstance().window->IsFullscreen();
	pauseOptionsUI.push_back(chkFull);
	oY += pSpacing;
	pauseOptionsUI.push_back(uiManager->CreateUIElement(UIElementType::BUTTON, (int)GameUI_ID::BTN_OPTIONS_BACK, "BACK", 0.5f, oY, pW, pH, sceneObserver));

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
	if (input->GetKey(SDL_SCANCODE_P) == KEY_DOWN) ToggleGameMenu(GameMenuTab::PAUSE_MAIN);

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
	//Clean up UI Pause Vectos
	pauseMainUI.clear();
	pauseOptionsUI.clear();

	return true;
}

bool GameScene::OnUIMouseClickEvent(UIElement* uiElement) {
	switch (uiElement->id) {
	case (int)GameUI_ID::BTN_TAB_INVENTORY: ToggleGameMenu(GameMenuTab::INVENTORY); break;
	case (int)GameUI_ID::BTN_TAB_MAP: ToggleGameMenu(GameMenuTab::MAP); break;
	case (int)GameUI_ID::BTN_TAB_SKILLS: ToggleGameMenu(GameMenuTab::SKILL_TREE); break;

		// Controles de Pausa
	case (int)GameUI_ID::BTN_PAUSE_RESUME:
		ToggleGameMenu(GameMenuTab::NONE);
		break;
	case (int)GameUI_ID::BTN_PAUSE_OPTIONS:
		currentMenuTab = GameMenuTab::PAUSE_OPTIONS;
		RefreshMenuUI();
		break;
	case (int)GameUI_ID::BTN_PAUSE_MAINMENU:
		Engine::GetInstance().sceneManager->ChangeScene(SceneID::MENU);
		break;
	case (int)GameUI_ID::BTN_OPTIONS_BACK:
		currentMenuTab = GameMenuTab::PAUSE_MAIN;
		RefreshMenuUI();
		break;
	case (int)GameUI_ID::SLD_MUSIC: Engine::GetInstance().audio->SetMusicVolume(((UISlider*)uiElement)->GetValue()); break;
	case (int)GameUI_ID::SLD_FX: Engine::GetInstance().audio->SetSFXVolume(((UISlider*)uiElement)->GetValue()); break;
	case (int)GameUI_ID::CHK_FULLSCREEN: Engine::GetInstance().window->SetFullscreen(((UICheckBox*)uiElement)->isChecked); break;
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
	// La Top Bar (Inventario/Mapa) solo se muestra si NO estamos en pausa general
	bool showTopBar = (currentMenuTab == GameMenuTab::INVENTORY ||
		currentMenuTab == GameMenuTab::MAP ||
		currentMenuTab == GameMenuTab::SKILL_TREE);
	SetUIGroupVisible(topBarElements, showTopBar);

	SetUIGroupVisible(inventoryUI, currentMenuTab == GameMenuTab::INVENTORY);
	SetUIGroupVisible(mapUI, currentMenuTab == GameMenuTab::MAP);
	SetUIGroupVisible(skillUI, currentMenuTab == GameMenuTab::SKILL_TREE);

	// Los nuevos menús de pausa
	SetUIGroupVisible(pauseMainUI, currentMenuTab == GameMenuTab::PAUSE_MAIN);
	SetUIGroupVisible(pauseOptionsUI, currentMenuTab == GameMenuTab::PAUSE_OPTIONS);
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

// ==========================================
// Auxiliar Funcions
// ==========================================

void GameScene::LoadTextureIfNull(SDL_Texture*& texture, const char* path) {
	if (texture == nullptr) {
		texture = Engine::GetInstance().textures->Load(path);
	}
}