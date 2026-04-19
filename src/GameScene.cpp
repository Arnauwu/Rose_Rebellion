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
	std::string previousMap = Engine::GetInstance().map->mapFileName;
	printf("prevoius map : %s", previousMap);
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

	Vector2D spawnPos = Engine::GetInstance().map->GetPlayerSpawnPoint(previousMap);

	if (Engine::GetInstance().sceneManager->GetPlayer() != nullptr)
	{
		Engine::GetInstance().sceneManager->GetPlayer()->position = spawnPos;
		printf("Player spawned at: (%.2f, %.2f)\n", spawnPos.getX(), spawnPos.getY());
	}

	Engine::GetInstance().entityManager->Start();

}

bool GameScene::Start() {
	auto uiManager = Engine::GetInstance().uiManager;
	Module* sceneObserver = (Module*)Engine::GetInstance().sceneManager.get();
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

	//Load Items
	LoadTextureIfNull(texItemKeyCastle, "Assets/Textures/UI/Items/castleKeyUI.png");
	LoadTextureIfNull(texItemKeyForest, "Assets/Textures/UI/Items/forceOrbUI.png");
	LoadTextureIfNull(texItemOrb, "Assets/Textures/UI/Items/forestKeyUI.png");
	LoadTextureIfNull(texItemGlide, "Assets/Textures/UI/Items/glideUI.png");
	LoadTextureIfNull(texItemWeapon, "Assets/Textures/UI/Items/weaponUI.png");

	//Top Bar
	CreateTopBarUI();
	//Inventario
	CreateInventoryUI();

	// Pause Menu
	CreatePauseMenuUI();

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
	if (input->GetKey(SDL_SCANCODE_P) == KEY_DOWN) ToggleGameMenu(GameMenuTab::PAUSE_MENU);

	if (currentMenuTab != GameMenuTab::NONE) {
		{
			SDL_Texture* currentTextureToDraw = nullptr;
			int screenW, screenH;
			Engine::GetInstance().window->GetWindowSize(screenW, screenH);
			SDL_Rect fullScreenRect = { 0, 0, screenW, screenH };

			Engine::GetInstance().render->DrawRectangle(fullScreenRect, 0, 0, 0, 180, true, false);

			switch (currentMenuTab) {
			case GameMenuTab::INVENTORY:
				currentTextureToDraw = t_inventoryUI;
				break;
			case GameMenuTab::MAP:
				currentTextureToDraw = t_mapUI;
				break;
			case GameMenuTab::SKILL_TREE:
				currentTextureToDraw = t_skilltreeUI;
				break;
			case GameMenuTab::PAUSE_MENU:
			case GameMenuTab::PAUSE_OPTIONS:
				currentTextureToDraw = t_pauseUI;
				break;
			default:
				break;
			}
			if (currentTextureToDraw != nullptr) {
				Engine::GetInstance().render->DrawTextureScaled(currentTextureToDraw, fullScreenRect);
			}

			return true;
		}
		// If any menu is open, freeze normal gameplay logic
		if (currentMenuTab != GameMenuTab::NONE) {

			int screenW, screenH;
			Engine::GetInstance().window->GetWindowSize(screenW, screenH);
			SDL_Rect bgRect = { 0, 0, screenW, screenH };

			Engine::GetInstance().render->DrawRectangle(bgRect, 0, 0, 0, 180, true, false);

			return true;
		}

		if (currentMenuTab == GameMenuTab::INVENTORY) {
			//TO DO: Buscamos si algún elemento del inventario está FOCUSED
			for (auto& elem : inventoryUI) {
				if (elem->state == UIElementState::FOCUSED) {
				}
			}
		}
	}
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

	//UnloadTexture
	UnloadTexture(t_inventoryUI);
	UnloadTexture(t_mapUI);
	UnloadTexture(t_skilltreeUI);
	UnloadTexture(t_pauseUI);

	//UnloadTexture Items
	UnloadTexture(texItemKeyCastle);
	UnloadTexture(texItemGlide);
	UnloadTexture(texItemKeyForest);
	UnloadTexture(texItemOrb);
	UnloadTexture(texItemWeapon);

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
		currentMenuTab = GameMenuTab::PAUSE_MENU;
		RefreshMenuUI();
		break;
	case (int)GameUI_ID::SLD_MUSIC: Engine::GetInstance().audio->SetMusicVolume(((UISlider*)uiElement)->GetValue()); break;
	case (int)GameUI_ID::SLD_FX: Engine::GetInstance().audio->SetSFXVolume(((UISlider*)uiElement)->GetValue()); break;
	case (int)GameUI_ID::CHK_FULLSCREEN: Engine::GetInstance().window->SetFullscreen(((UICheckBox*)uiElement)->isChecked); break;
	}
	return true;
}
// ==========================================
// CREATE UI
// ==========================================

void GameScene::CreateTopBarUI() {
	auto uiManager = Engine::GetInstance().uiManager;
	Module* sceneObserver = (Module*)Engine::GetInstance().sceneManager.get();

	float wPerc = 0.33f, hPerc = 0.05f; 
	float startX = 0.30f, yPos = 0.15f;
	float spacingX = 0.20f;

	ButtonDef topBarDefs[] = {
		{ (int)GameUI_ID::BTN_TAB_MAP,       "MAP" },
		{ (int)GameUI_ID::BTN_TAB_INVENTORY, "INVENTORY" },
		{ (int)GameUI_ID::BTN_TAB_SKILLS,    "SKILLS" }
	};

	for (const auto& def : topBarDefs) {
		topBarElements.push_back(uiManager->CreateUIElement(UIElementType::BUTTON, def.id, def.text, startX, yPos, wPerc, hPerc, sceneObserver));
		startX += spacingX;
	}
}

void GameScene::CreateInventoryUI() {
	auto uiManager = Engine::GetInstance().uiManager;
	Module* sceneObserver = (Module*)Engine::GetInstance().sceneManager.get();

	float centerX = 0.50f;
	float centerY = 0.50f;
	float offset = 0.15f;

	// Ampliamos tu struct para que acepte un puntero a textura
	struct InventorySlotDef {
		GameUI_ID id;
		const char* name;
		const char* description;
		float relX, relY;
		SDL_Texture* tex; 
	};

	std::vector<InventorySlotDef> slots = {
		// ARMA (Centro)
		{ GameUI_ID::INV_ITEM_WEAPON, "Weapon", "------", centerX, centerY, texItemWeapon },

		// AMULETOS (Vertices)
		{ GameUI_ID::INV_ITEM_GLIDE, "Glide", "Float through the air.", centerX - offset, centerY - offset, texItemGlide },
		{ GameUI_ID::INV_ITEM_DASH, "Dash", "Quick burst of speed.", centerX + offset, centerY - offset, nullptr }, // Pon nullptr si aún no tienes textura
		{ GameUI_ID::INV_ITEM_DOUBLE_JUMP, "Double", "Reach higher ground.", centerX - offset, centerY + offset, nullptr },
		{ GameUI_ID::INV_ITEM_WALL_JUMP, "Wall J", "Climb vertical walls.", centerX + offset, centerY + offset, nullptr },

		// ITEMS
		{ GameUI_ID::INV_ITEM_KEY, "Key", "Opens area doors.", centerX - offset, centerY, texItemKeyCastle },
		{ GameUI_ID::INV_ITEM_ORB, "Orb", "Increases your power.", centerX + offset, centerY, texItemOrb }
	};

	for (const auto& slot : slots) {
		auto btn = uiManager->CreateUIElement(UIElementType::BUTTON, (int)slot.id, slot.name, slot.relX, slot.relY, 0.08f, 0.08f, sceneObserver);

		// Si le hemos asignado una textura, se la ponemos al botón
		if (slot.tex != nullptr) {
			btn->SetTexture(slot.tex);
		}

		inventoryUI.push_back(btn);
	}
}

void GameScene::CreatePauseMenuUI() {
	auto uiManager = Engine::GetInstance().uiManager;
	Module* sceneObserver = (Module*)Engine::GetInstance().sceneManager.get();

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
	CreatePauseSettingUI();
}

void GameScene::CreatePauseSettingUI() {
	auto uiManager = Engine::GetInstance().uiManager;
	Module* sceneObserver = (Module*)Engine::GetInstance().sceneManager.get();
	float pW = 0.25f, pH = 0.08f;
	float pY = 0.35f, pSpacing = 0.1f;

	auto sldMus = uiManager->CreateUIElement(UIElementType::SLIDER, (int)GameUI_ID::SLD_MUSIC, "Music", 0.5f, pY, 0.3f, 0.05f, sceneObserver);
	if (auto* s = dynamic_cast<UISlider*>(sldMus.get())) s->SetValue(Engine::GetInstance().audio->GetMusicVolume());
	pauseOptionsUI.push_back(sldMus);
	pY += pSpacing;
	auto sldFx = uiManager->CreateUIElement(UIElementType::SLIDER, (int)GameUI_ID::SLD_FX, "FX", 0.5f, pY, 0.3f, 0.05f, sceneObserver);
	if (auto* s = dynamic_cast<UISlider*>(sldFx.get())) s->SetValue(Engine::GetInstance().audio->GetSFXVolume());
	pauseOptionsUI.push_back(sldFx);
	pY += pSpacing;
	auto chkFull = uiManager->CreateUIElement(UIElementType::CHECKBOX, (int)GameUI_ID::CHK_FULLSCREEN, "Fullscreen", 0.5f, pY, 0.05f, 0.05f, sceneObserver);
	if (auto* c = dynamic_cast<UICheckBox*>(chkFull.get())) c->isChecked = Engine::GetInstance().window->IsFullscreen();
	pauseOptionsUI.push_back(chkFull);
	pY += pSpacing;
	pauseOptionsUI.push_back(uiManager->CreateUIElement(UIElementType::BUTTON, (int)GameUI_ID::BTN_OPTIONS_BACK, "BACK", 0.5f, pY, pW, pH, sceneObserver));
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
	bool shouldPause = (currentMenuTab != GameMenuTab::NONE);
	Engine::GetInstance().sceneManager->SetGamePaused(shouldPause);
	RefreshMenuUI();
}

void GameScene::UpdateInventoryVisuals() {
	// TO DO: Al merge mirar los items
	if (player == nullptr) return;

	for (auto& btn : inventoryUI) {
		bool hasItem = false;

		// Comprobamos qué ítem es este botón y le preguntamos al Player si lo tiene
		switch (btn->id) {
		case (int)GameUI_ID::INV_ITEM_WEAPON:
			hasItem = player->HasItem(ItemID::WEAPON); // < Ajusta tu enum aquí
			break;
		case (int)GameUI_ID::INV_ITEM_GLIDE:
			hasItem = player->HasItem(ItemID::GLIDE);
			break;
		case (int)GameUI_ID::INV_ITEM_KEY:
			hasItem = player->HasItem(ItemID::KEY);
			break;
		case (int)GameUI_ID::INV_ITEM_ORB:
			hasItem = player->HasItem(ItemID::STRENGTH_ORB);
			break;

		default:
			continue; 
		}
		// Tintado de textura:
		if (hasItem) {
			btn->color = { 255, 255, 255, 255 }; // Blanco (Color original brillante)
		}
		else {
			btn->color = { 60, 60, 60, 255 };    // Gris oscuro (Ítem bloqueado/sin conseguir)
		}
	}
}
void GameScene::RefreshMenuUI() {
	// La Top Bar (Inventario/Mapa) solo se muestra si NO estamos en pausa general
	bool showTopBar = (currentMenuTab == GameMenuTab::INVENTORY ||
		currentMenuTab == GameMenuTab::MAP ||
		currentMenuTab == GameMenuTab::SKILL_TREE);
	SetUIGroupVisible(topBarElements, showTopBar);

	if (currentMenuTab == GameMenuTab::INVENTORY) {
		UpdateInventoryVisuals();
	}

	SetUIGroupVisible(inventoryUI, currentMenuTab == GameMenuTab::INVENTORY);
	SetUIGroupVisible(mapUI, currentMenuTab == GameMenuTab::MAP);
	SetUIGroupVisible(skillUI, currentMenuTab == GameMenuTab::SKILL_TREE);

	
	SetUIGroupVisible(pauseMainUI, currentMenuTab == GameMenuTab::PAUSE_MENU);
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
// Auxiliar Textures funcions
// ==========================================

void GameScene::LoadTextureIfNull(SDL_Texture*& texture, const char* path) {
	if (texture == nullptr) {
		texture = Engine::GetInstance().textures->Load(path);
	}
}

void GameScene::UnloadTexture(SDL_Texture*& texture) {
	if (texture != nullptr) {
		Engine::GetInstance().textures->UnLoad(texture);
		texture = nullptr;
	}
}