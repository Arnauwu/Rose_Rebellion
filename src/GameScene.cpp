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

	if (mapFile == "Castle_Inside.tmx") {
		Engine::GetInstance().audio->PlayMusic("Assets/Audio/Music/Prueba2.wav"); // Música Interior Castillo
	}
	else if (mapFile == "Castle.tmx") {
		Engine::GetInstance().audio->PlayMusic("Assets/Audio/Music/Prueba2.wav"); // Música Exterior Castillo
	}
	else if (mapFile.find("Forest_01") != std::string::npos) {
		Engine::GetInstance().audio->PlayMusic("Assets/Audio/Music/Prueba.wav"); // Música Bosque
	}

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

	LoadMap("Castle_Inside.tmx");
	if (player != nullptr) {
		player->position.setX(10);
		player->position.setY(10);
	}

	// Buttons and Bg
	LoadTextureIfNull(buttonUI, "Assets/Textures/UI/Buttons/buttonUI.png");
	LoadTextureIfNull(skillFrameUI, "Assets/Textures/UI/Buttons/skillFrameUI.png");
	LoadTextureIfNull(orbFrameUI, "Assets/Textures/UI/Buttons/orbFrameUI.png");
	LoadTextureIfNull(keyFrameUI, "Assets/Textures/UI/Buttons/keyFrameUI.png");
	LoadTextureIfNull(textBgUI, "Assets/Textures/UI/Buttons/textBgUI.png");


	// Texture Load
	LoadTextureIfNull(texMapUI, "Assets/Textures/UI/GameMenu/t_MapUI.png");
	LoadTextureIfNull(texInventoryUI, "Assets/Textures/UI/GameMenu/t_inventoryUI.png");
	LoadTextureIfNull(texSkilltreeUI, "Assets/Textures/UI/GameMenu/t_skilltreeUI.png");
	LoadTextureIfNull(texPauseUI, "Assets/Textures/UI/GameMenu/t_pauseUI.png");

	//Load Items
	LoadTextureIfNull(texItemKeyCastle, "Assets/Textures/UI/Items/castleKeyUI.png");
	LoadTextureIfNull(texItemKeyForest, "Assets/Textures/UI/Items/forestKeyUI.png");
	LoadTextureIfNull(texItemOrb, "Assets/Textures/UI/Items/forceOrbUI.png");
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

	if (input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN) {
		if (currentMenuTab != GameMenuTab::NONE) {
			ToggleGameMenu(GameMenuTab::NONE);
		}
		else {
			ToggleGameMenu(GameMenuTab::PAUSE_MENU);
		}
	}
	// --- SUB-MENU INPUT HANDLING ---
	// Toggle menus based on keyboard shortcuts
	if (input->GetKey(SDL_SCANCODE_I) == KEY_DOWN) ToggleGameMenu(GameMenuTab::INVENTORY);
	if (input->GetKey(SDL_SCANCODE_M) == KEY_DOWN) ToggleGameMenu(GameMenuTab::MAP);
	if (input->GetKey(SDL_SCANCODE_N) == KEY_DOWN) ToggleGameMenu(GameMenuTab::SKILL_TREE);
	
	if (currentMenuTab != GameMenuTab::NONE) {
		{
			SDL_Texture* currentTextureToDraw = nullptr;
			int screenW = Engine::GetInstance().render->camera.w;
			int screenH = Engine::GetInstance().render->camera.h;

			SDL_Rect fullScreenRect = { 0, 0, screenW, screenH };

			Engine::GetInstance().render->DrawRectangle(fullScreenRect, 0, 0, 0, 180, true, false);

			switch (currentMenuTab) {
			case GameMenuTab::INVENTORY:
				currentTextureToDraw = texInventoryUI;
				break;
			case GameMenuTab::MAP:
				currentTextureToDraw = texMapUI;
				break;
			case GameMenuTab::SKILL_TREE:
				currentTextureToDraw = texSkilltreeUI;
				break;
			case GameMenuTab::PAUSE_MENU:
			case GameMenuTab::PAUSE_OPTIONS:
				currentTextureToDraw = texPauseUI;
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

		//if (currentMenuTab == GameMenuTab::INVENTORY) {

		//	for (auto& elem : inventoryUI) {
		//		if (elem->state == UIElementState::FOCUSED) {
		//		}
		//	}
		//}
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

	// UnloadTexture BG buttons
	UnloadTexture(buttonUI);
	UnloadTexture(skillFrameUI);
	UnloadTexture(orbFrameUI);
	UnloadTexture(keyFrameUI);
	UnloadTexture(textBgUI);

	//UnloadTexture
	UnloadTexture(texInventoryUI);
	UnloadTexture(texMapUI);
	UnloadTexture(texSkilltreeUI);
	UnloadTexture(texPauseUI);

	//UnloadTexture Items
	UnloadTexture(texItemKeyCastle);
	UnloadTexture(texItemGlide);
	UnloadTexture(texItemKeyForest);
	UnloadTexture(texItemOrb);
	UnloadTexture(texItemWeapon);

	auto deleteGroup = [](std::vector<std::shared_ptr<UIElement>>& group) {
		for (auto& elem : group) {
			if (elem) elem->CleanUp(); 
		}
		group.clear();
	};

	deleteGroup(topBarElements);
	deleteGroup(inventoryUI);
	deleteGroup(mapUI);
	deleteGroup(skillUI);
	deleteGroup(pauseMainUI);
	deleteGroup(pauseOptionsUI);

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
		ToggleGameMenu(GameMenuTab::NONE); 
		Engine::GetInstance().sceneManager->ChangeScene(SceneID::MENU); break;
	case (int)GameUI_ID::BTN_OPTIONS_BACK:
		currentMenuTab = GameMenuTab::PAUSE_MENU;
		RefreshMenuUI();
		break;
	case (int)GameUI_ID::SLD_MUSIC: Engine::GetInstance().audio->SetMusicVolume(((UISlider*)uiElement)->GetValue()); break;
	case (int)GameUI_ID::SLD_FX: Engine::GetInstance().audio->SetSFXVolume(((UISlider*)uiElement)->GetValue()); break;
	case (int)GameUI_ID::CHK_FULLSCREEN: Engine::GetInstance().window->SetFullscreen(((UICheckBox*)uiElement)->isChecked); break;

		// Gestión texto de los objetos del inventario
	case (int)GameUI_ID::INV_ITEM_WEAPON:
		if (player && player->HasItem(ItemID::WEAPON)) descPanel->text = "Weapon: LORE.";
		else descPanel->text = "???";
		break;

	case (int)GameUI_ID::INV_ITEM_GLIDE:
		if (player && player->HasItem(ItemID::GLIDE)) descPanel->text = "Cape: Float through the air.";
		else descPanel->text = "???";
		break;

	case (int)GameUI_ID::INV_ITEM_KEY:
		if (player && player->HasItem(ItemID::KEY)) descPanel->text = "Castle Key: Opens heavy doors.";
		else descPanel->text = "???";
		break;

	case (int)GameUI_ID::INV_ITEM_ORB:
		if (player && player->HasItem(ItemID::STRENGTH_ORB)) descPanel->text = "Power Orb: Increases your strength.";
		else descPanel->text = "???";
		break;

	}
	return true;
}
// ==========================================
// CREATE UI
// ==========================================

void GameScene::CreateTopBarUI() {
	auto uiManager = Engine::GetInstance().uiManager;
	Module* sceneObserver = (Module*)Engine::GetInstance().sceneManager.get();

	float wPerc = 0.15f, hPerc = 0.13f;
	float startX = 0.27f, yPos = 0.12f;
	float spacingX = 0.25f;

	ButtonDef topBarDefs[] = {
		{ (int)GameUI_ID::BTN_TAB_MAP,       "" },
		{ (int)GameUI_ID::BTN_TAB_INVENTORY, "" },
		{ (int)GameUI_ID::BTN_TAB_SKILLS,    "" }
	};

	for (const auto& def : topBarDefs) {
		topBarElements.push_back(uiManager->CreateUIElement(UIElementType::BUTTON, def.id, def.text, startX, yPos, wPerc, hPerc, sceneObserver));
		startX += spacingX;
	}
}

void GameScene::CreateInventoryUI() {
	auto uiManager = Engine::GetInstance().uiManager;
	Module* sceneObserver = (Module*)Engine::GetInstance().sceneManager.get();


	int sw = Engine::GetInstance().render->camera.w;
	int sh = Engine::GetInstance().render->camera.h;
	float aspect = (float)sw / (float)sh;

	float baseSize = 0.08f;
	float squareH = baseSize * aspect;

	float centerX = 0.38f;
	float centerY = 0.55f;
	float offsetX = 0.13f;
	float offsetY = 0.20f;


	struct InventorySlotDef {
		GameUI_ID id;
		const char* name;
		float relX, relY;
		float w, h;
		SDL_Texture* tex;
	};

	std::vector<InventorySlotDef> slots = {
		// ARMA (Centro)
		{ GameUI_ID::INV_ITEM_WEAPON, "", centerX, centerY,baseSize + float(0.1), squareH + float(0.2), texItemWeapon},

		// AMULETOS (Vertices)
		{ GameUI_ID::INV_ITEM_GLIDE, "", centerX - offsetX, centerY - offsetY,baseSize, squareH, texItemGlide },
		{ GameUI_ID::INV_ITEM_DASH, "Dash", centerX + offsetX , centerY - offsetY,baseSize, squareH, nullptr }, // Pon nullptr si aún no tienes textura
		{ GameUI_ID::INV_ITEM_DOUBLE_JUMP, "Double J", centerX - offsetX, centerY + offsetY,baseSize, squareH, nullptr },
		{ GameUI_ID::INV_ITEM_WALL_JUMP, "Wall J", centerX + offsetX, centerY + offsetY,baseSize, squareH, nullptr },

		// ITEMS
		{ GameUI_ID::INV_ITEM_KEY, "", centerX - offsetX - float(0.04), centerY,baseSize, squareH, texItemKeyCastle },
		{ GameUI_ID::INV_ITEM_ORB, "", centerX + offsetX + float(0.04), centerY,baseSize, squareH, texItemOrb }
	};

	for (const auto& slot : slots) {
		auto btn = uiManager->CreateUIElement(UIElementType::BUTTON, (int)slot.id, slot.name, slot.relX, slot.relY, slot.w, slot.h, sceneObserver);

		btn->SetBgTexture(skillFrameUI);

		// Si le hemos asignado una textura, se la ponemos al botón
		if (slot.tex != nullptr) {
			btn->SetTexture(slot.tex);
		}
		inventoryUI.push_back(btn);
	}

	descPanel = uiManager->CreateUIElement(
		UIElementType::BUTTON,
		(int)GameUI_ID::INV_DESC_TEXT, "Select an item...", 0.72f, 0.55f, 0.20f, 0.60f, sceneObserver
	);
	descPanel->SetBgTexture(textBgUI);

	descPanel->state = UIElementState::DISABLED;
	inventoryUI.push_back(descPanel);
}

void GameScene::CreatePauseMenuUI() {
	auto uiManager = Engine::GetInstance().uiManager;
	Module* sceneObserver = (Module*)Engine::GetInstance().sceneManager.get();

	float pW = 0.25f, pH = 0.08f;
	float pY = 0.40f, pSpacing = 0.1f;

	ButtonDef pauseBtnDefs[] = {
		{ (int)GameUI_ID::BTN_PAUSE_RESUME,   "RESUME" },
		{ (int)GameUI_ID::BTN_PAUSE_OPTIONS,  "OPTIONS" },
		{ (int)GameUI_ID::BTN_PAUSE_MAINMENU, "MAIN MENU" }
	};

	for (const auto& def : pauseBtnDefs) {
		auto btn = uiManager->CreateUIElement(UIElementType::BUTTON, def.id, def.text, 0.5f, pY, pW, pH, sceneObserver);

		btn->SetBgTexture(buttonUI);

		pauseMainUI.push_back(btn);
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
	if (player == nullptr) return;

	for (auto& btn : inventoryUI) {
		bool hasItem = false;

		// We check which item this button is and ask the Player if they have it
		switch (btn->id) {
		case (int)GameUI_ID::INV_ITEM_WEAPON:
			hasItem = player->HasItem(ItemID::WEAPON);
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
		if (hasItem) {
			btn->color = { 255, 255, 255, 255 }; // White tint --> Normal texture
		}
		else {
			btn->color = { 60, 60, 60, 255 };    // Gray tint
		}
	}
}
void GameScene::RefreshMenuUI() {
	if (descPanel != nullptr) descPanel->text = "Select an item...";
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