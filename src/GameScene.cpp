#include "GameScene.h"
#include "Engine.h"
#include "Input.h"
#include "Map.h"
#include "SceneManager.h"
#include "EntityManager.h"
#include "GameManager.h"
#include "UIManager.h"
#include "DialogueManager.h"
#include "LanguageManager.h"
#include "UIDialogueBox.h"
#include "Cinematics.h"

#include "window.h"
#include "Player.h"
#include "Log.h"
#include "Textures.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;


GameScene::GameScene() : SceneBase() {

}

GameScene::~GameScene() {
}

void GameScene::LoadMap(std::string mapFile)
{
	//Load the map. 
	Player* p = Engine::GetInstance().entityManager->GetPlayer();
	if (mapFile == "")
	{
		if (p != nullptr && p->interactuableBody != nullptr) {
			mapFile = Engine::GetInstance().map->DoorInfo(p->interactuableBody);
		}
	}

	// Fail-Save
	if (mapFile == "")
	{
		return;
	}

	if (mapFile.find("Castle") != std::string::npos && mapFile != "Nexo.tmx") {
		// Esto cubrirá todos los mapas del castillo (Inside, Kitchen, Balcony, etc.)
		Engine::GetInstance().audio->PlayMusic("Assets/Audio/Music/MusicaInteriorCastillo.wav");
	}
	else if (mapFile == "Nexo.tmx") {
		Engine::GetInstance().audio->PlayMusic("Assets/Audio/Music/MusicaExteriorCastilloNeutra.wav");
	}
	else if (mapFile.find("Forest") != std::string::npos) {
		// Esto cubrirá Forest, Forest_01, Forest_02...
		Engine::GetInstance().audio->PlayMusic("Assets/Audio/Music/MusicaBosque.wav");
	}
	else if (mapFile.find("Mountain") != std::string::npos) {
		// Esto cubrirá Mountain_01, Mountain_02...
		Engine::GetInstance().audio->PlayMusic("Assets/Audio/Music/MusicaBosque.wav");
	}
	else if (mapFile.find("Catacombs") != std::string::npos) {
		// Añade la música para tus catacumbas aquí
		Engine::GetInstance().audio->PlayMusic("Assets/Audio/Music/MusicaBosque.wav");
	}

	Engine::GetInstance().sceneManager->setNewMap = false;

	std::string previousMap = Engine::GetInstance().map->mapFileName;
	LOG("MAPA PREVIO %s", previousMap);

	Engine::GetInstance().audio->StopAllFx();

	Engine::GetInstance().entityManager->CleanUp(true);
	Engine::GetInstance().map->CleanUp();

	// Player Pause
	/*Player* pLoad = Engine::GetInstance().entityManager->GetPlayer();
	if (pLoad != nullptr) {
		pLoad->isFrozen = true;
		if (pLoad->pbody != nullptr) {
			Engine::GetInstance().physics->SetLinearVelocity(pLoad->pbody, { 0.0f, 0.0f });
		}
	}*/

	isLoadFinished = false;
	isAsyncLoading = true;
	isDataInitialized = false;
	loadingDotsTimer = 0.0f;

	Engine::GetInstance().hud->isHidden = true;
	Engine::GetInstance().physics->pausePhysics = true;
	this->asyncMapFile = mapFile;
	this->asyncPreviousMap = previousMap;

	// PRECARGAR TEXTURAS -- PRINCESA CON CAPA / BOSSES
	loadingThread = std::thread([this]() {
		Engine::GetInstance().textures->PreloadToRAM("Assets/Textures/Items/SavePoint/Rosa.png");

		if (this->asyncMapFile.find("Castle_Room_Storage") != std::string::npos) {
			Engine::GetInstance().textures->PreloadToRAM("Assets/Textures/Entities/Princess/Princess.png");
		}
	/*	if (this->asyncMapFile.find("Forest_01") != std::string::npos 
			|| this->asyncMapFile.find("Forest_02") != std::string::npos 
			|| this->asyncMapFile.find("Forest_03") != std::string::npos 
			|| this->asyncMapFile.find("Forest_04") != std::string::npos) {
			Engine::GetInstance().textures->PreloadToRAM("Assets/Textures/Entities/Enemies//png");
		}*/
		else if (this->asyncMapFile.find("Mountain_01") != std::string::npos) {
			Engine::GetInstance().textures->PreloadToRAM("Assets/Textures/Entities/Enemies/Dip/SS_Dip.png");
		}
		else if (this->asyncMapFile.find("Mountain_03") != std::string::npos) {
			Engine::GetInstance().textures->PreloadToRAM("Assets/Textures/Entities/Enemies/Dragon/DragonH.png");
		}

		Engine::GetInstance().map->Load("Assets/Maps/", this->asyncMapFile);
		isLoadFinished = true;
		});
}

void GameScene::FinishMapLoad()
{
	// Sincronización con la GPU
	Engine::GetInstance().map->FinishVRAMUpload();

	Engine::GetInstance().map->SpawnEntities();

	//Minimap Update
	minimap.CreateRoom(asyncMapFile);
	minimap.SetCurrentRoom(asyncMapFile);

	// Camara mode
	Player* player = Engine::GetInstance().entityManager->GetPlayer();
	if (player != nullptr)
	{
		Vector2D spawnPos = Engine::GetInstance().map->GetPlayerSpawnPoint(asyncPreviousMap);
		player->position = spawnPos;
		printf("Player spawned at: (%.2f, %.2f)\n", spawnPos.getX(), spawnPos.getY());

		// ASIGNAR EL MODO DE CÁMARA AQU?
		if (asyncMapFile == "Castle_Room_Princess.tmx" || asyncMapFile == "Castle_Inside.tmx" || asyncMapFile == "Castle_Room_Kitchen.tmx" || asyncMapFile == "Castle_Room_Storage.tmx") {
			player->SetCameraMode(CameraMode::CLASSIC);
			LOG("Camera Mode set to CLASSIC");
		}
		else {
			player->SetCameraMode(CameraMode::DYNAMIC);
			LOG("Camera Mode set to DYNAMIC");
		}
	}

	Player* newPlayer = Engine::GetInstance().entityManager->GetPlayer();
	if (newPlayer != nullptr)
	{
		Vector2D spawnPos;

		if (asyncPreviousMap == "")
		{
			spawnPos = GameManager::GetInstance().gameState.playerPosition;
			LOG("Carga inicial: Usando posición del GameManager: (%.2f, %.2f)", spawnPos.getX(), spawnPos.getY());
		}
		else
		{
			spawnPos = Engine::GetInstance().map->GetPlayerSpawnPoint(asyncPreviousMap, targetSpawnID);
			LOG("Transición: Buscando spawn point para el mapa previo: %s", asyncPreviousMap.c_str());
		}

		newPlayer->SetPosition(spawnPos);

		newPlayer->isFrozen = false;

		if (newPlayer->pbody != nullptr) {
			Engine::GetInstance().physics->SetLinearVelocity(newPlayer->pbody, { 0.0f, 0.0f });
		}

		if (asyncMapFile == "Castle_Room_Princess.tmx") {
			Engine::GetInstance().hud->ShowTutorial(TutorialType::WALK);
		}
		else if (asyncMapFile == "Castle_Inside.tmx") {
			Engine::GetInstance().hud->ShowTutorial(TutorialType::JUMP);
		}
	}
	Engine::GetInstance().entityManager->Start();
	Engine::GetInstance().physics->pausePhysics = false;
	Engine::GetInstance().hud->isHidden = false;
}

void GameScene::LoadItemsLore() {
	std::ifstream file("Assets/Dialogues/items_lore.json");
	if (!file.is_open()) {
		LOG("Error: No se pudo abrir dialogues.json");

		return;
	}

	nlohmann::json j;
	file >> j;

	for (auto& element : j.items()) {
		std::string key = element.key();
		ItemLore lore;
		lore.name = element.value()["name"].get<std::string>();
		lore.description = element.value()["description"].get<std::string>();

		itemsLoreDB[key] = lore;
	}
	LOG("Lore de items cargado correctamente.");
}
void GameScene::LoadSkillsinfo() {
	std::ifstream file("Assets/Dialogues/skills_Info.json");
	if (!file.is_open()) {
		LOG("Error: No se pudo abrir skills_Info.json");
		return;
	}

	nlohmann::json j;
	file >> j;

	for (auto& element : j.items()) {
		std::string key = element.key();
		Skillinfo text;
		text.name = element.value()["name"].get<std::string>();
		text.description = element.value()["description"].get<std::string>();
		text.cost = element.value()["cost"].get<int>();

		skillsLoreDB[key] = text;
	}
	LOG("Lore de habilidades cargado correctamente.");
}

bool GameScene::Start() {
	Engine::GetInstance().cinematics->CloseVideo();
	LoadItemsLore();
	LoadSkillsinfo();
	uiClick = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/MusicaClicMenu.wav");

	auto uiManager = Engine::GetInstance().uiManager;
	Module* sceneObserver = (Module*)Engine::GetInstance().sceneManager.get();
	LOG("Loading Game Scene");

	Engine::GetInstance().map->mapFileName = "";

	std::string mapToLoad = GameManager::GetInstance().gameState.currentMap;
	LoadMap(mapToLoad);
	// Buttons and Bg
	LoadTextureIfNull(buttonUI, "Assets/Textures/UI/Buttons/buttonUI.png");
	LoadTextureIfNull(skillFrameUI, "Assets/Textures/UI/Buttons/skillFrameUI.png");
	LoadTextureIfNull(textBgUI, "Assets/Textures/UI/Buttons/textBgUI.png");
	LoadTextureIfNull(sliderThumbTex, "Assets/Textures/UI/Buttons/pomo.png");


	// Texture TOP BAR Load ENG/CAT
	LoadTextureIfNull(texPauseUI, "Assets/Textures/UI/GameMenu/t_pauseUI.png");
	LoadTextureIfNull(texMapUI, "Assets/Textures/UI/GameMenu/t_MapUI.png");
	LoadTextureIfNull(texInventoryUI, "Assets/Textures/UI/GameMenu/t_inventoryUI.png");
	LoadTextureIfNull(texSkillUI, "Assets/Textures/UI/SkillUpgrade/t_skillUI.png");

	LoadTextureIfNull(texMapUI_CAT, "Assets/Textures/UI/GameMenu/t_MapUI_CAT.png");
	LoadTextureIfNull(texInventoryUI_CAT, "Assets/Textures/UI/GameMenu/t_inventoryUI_CAT.png");
	LoadTextureIfNull(texSkillUI_CAT, "Assets/Textures/UI/SkillUpgrade/t_skillUI_CAT.png");
	//Load Items
	LoadTextureIfNull(texItemKeyCastle, "Assets/Textures/UI/Items/castleKeyUI.png");
	LoadTextureIfNull(texItemKeyForest, "Assets/Textures/UI/Items/forestKeyUI.png");
	LoadTextureIfNull(texItemKeyMountain, "Assets/Textures/UI/Items/mountainKeyUI.png");
	LoadTextureIfNull(texItemKeyCatacumbs, "Assets/Textures/UI/Items/catacumbsKeyUI.png");
	LoadTextureIfNull(texItemOrb, "Assets/Textures/UI/Items/forceOrbUI.png");

	// Power-upsLoad
	LoadTextureIfNull(texItemGlide, "Assets/Textures/UI/Items/glideUI.png");
	LoadTextureIfNull(texItemDash, "Assets/Textures/UI/Items/dashUI.png");
	LoadTextureIfNull(texItemWallJump, "Assets/Textures/UI/Items/wallJumpUI.png");
	LoadTextureIfNull(texItemDoubleJump, "Assets/Textures/UI/Items/doubleJumpUI.png");
	LoadTextureIfNull(texItemWeapon, "Assets/Textures/UI/Items/weaponUI.png");

	// Skill upgrade Load
	LoadTextureIfNull(books_1_1, "Assets/Textures/UI/SkillUpgrade/SkillMenu_Books_1-1.png");
	LoadTextureIfNull(books_1_2, "Assets/Textures/UI/SkillUpgrade/SkillMenu_Books_1-2.png");
	LoadTextureIfNull(books_2_1, "Assets/Textures/UI/SkillUpgrade/SkillMenu_Books_2-1.png");
	LoadTextureIfNull(books_2_2, "Assets/Textures/UI/SkillUpgrade/SkillMenu_Books_2-2.png");
	LoadTextureIfNull(books_3_1, "Assets/Textures/UI/SkillUpgrade/SkillMenu_Books_3-1.png");
	LoadTextureIfNull(books_3_2, "Assets/Textures/UI/SkillUpgrade/SkillMenu_Books_3-2.png");
	LoadTextureIfNull(books_1_1_active, "Assets/Textures/UI/SkillUpgrade/SkillMenu_cBooks_1-1.png");
	LoadTextureIfNull(books_1_2_active, "Assets/Textures/UI/SkillUpgrade/SkillMenu_cBooks_1-2.png");
	LoadTextureIfNull(books_2_1_active, "Assets/Textures/UI/SkillUpgrade/SkillMenu_cBooks_2-1.png");
	LoadTextureIfNull(books_2_2_active, "Assets/Textures/UI/SkillUpgrade/SkillMenu_cBooks_2-2.png");
	LoadTextureIfNull(books_3_1_active, "Assets/Textures/UI/SkillUpgrade/SkillMenu_cBooks_3-1.png");
	LoadTextureIfNull(books_3_2_active, "Assets/Textures/UI/SkillUpgrade/SkillMenu_cBooks_3-2.png");

	// Dialogue UI Load
	LoadTextureIfNull(UIDialogueBoxTex, "Assets/Textures/UI/Dialogues/UIDialogueBoxTex.png");
	LoadTextureIfNull(princessPortrait, "Assets/Textures/UI/Dialogues/princess_portrait.png");
	LoadTextureIfNull(npcPortrait, "Assets/Textures/UI/Dialogues/npc_portrait1.png");
	//LoadTextureIfNull(npcPortrait2, "Assets/Textures/UI/Dialogues/npc_portrait2.png");
	//LoadTextureIfNull(npcPortrait3, "Assets/Textures/UI/Dialogues/npc_portrait3.png");
	//LoadTextureIfNull(npcPortrait4, "Assets/Textures/UI/Dialogues/npc_portrait4.png");

	// Load Loading Screen 
	//LoadTextureIfNull(Rose_Sleep, "Assets/Textures/UI/LoadingScreen/Rose_Sleep.png");

	//Load level intro textures
	LoadTextureIfNull(introFrameTex, "Assets/Textures/UI/Buttons/frameTex.png");

	std::unordered_map<int, std::string> introAliases = { {0, "focus"} };
	introFrameAnim.LoadFromTSX("Assets/Textures/UI/Buttons/frameTex_LI.tsx", introAliases);

	introFrameAnim.SetCurrent("focus");

	if (introFrameAnim.GetAnim("focus")) {
		introFrameAnim.GetAnim("focus")->SetLoop(false);
	}

	//Top Bar
	CreateTopBarUI();
	//Inventario
	CreateInventoryUI();

	//Skill
	CreateSkillUpgradeUI();
	CreateSkillPopupUI();

	// Pause Menu
	CreatePauseMenuUI();

	CreateDialogueUI();

	RefreshMenuUI();

	Engine::GetInstance().languageManager->RegisterLanguageChangeCallback(
		[this](Language lang) { this->UpdateUILanguage(); }
	);

	UpdateUILanguage();
	return true;
}


bool GameScene::Update(float dt) {

	
	if (isAsyncLoading) {

		loadingDotsTimer += dt / 1000.0f;
		// Si el hilo de fondo ha terminado el trabajo pesado
		if (isLoadFinished && !isDataInitialized && loadingDotsTimer > 1.5f) {
			isDataInitialized = true;

			// Unimos el hilo de forma segura para liberar recursos del sistema
		if (loadingThread.joinable()) {
				loadingThread.join();
			}

			FinishMapLoad();
			isAsyncLoading = false; 

			mapState = MapTransitionState::FADING_IN;
			Engine::GetInstance().render->StartFade(FadeDirection::FADE_IN, mapFadeTime);
		}
		return true; 
	}
	
	auto render = Engine::GetInstance().render;
	auto input = Engine::GetInstance().input;
	auto dialogueMgr = Engine::GetInstance().dialogueManager;

	if (dialogueMgr->IsDialogueActive()) {
		return true;
	}
	// --- SUB-MENU INPUT HANDLING ---
	// Toggle menus based on keyboard shortcuts
	RefreshMenuUI();
	if (input->GetKey(SDL_SCANCODE_I) == KEY_DOWN) ToggleGameMenu(GameMenuTab::INVENTORY);
	if (input->GetKey(SDL_SCANCODE_M) == KEY_DOWN) ToggleGameMenu(GameMenuTab::MAP);
	if (input->GetKey(SDL_SCANCODE_N) == KEY_DOWN) ToggleGameMenu(GameMenuTab::SKILL_TREE);

	if (mapState == MapTransitionState::FADING_OUT) {

		if (render->IsFadeComplete()) {

			LoadMap(nextMapName);

			mapState = MapTransitionState::FADING_IN;
			render->StartFade(FadeDirection::FADE_IN, mapFadeTime);
		}
		return true;
	}
	else if (mapState == MapTransitionState::FADING_IN) {
		if (render->IsFadeComplete()) {

			bool showIntro = false;
			// Comprobar si el mapa cargado requiere presentación
			if (asyncMapFile.find("Forest_01") != std::string::npos &&
				asyncPreviousMap.find("Forest_02") == std::string::npos)
			{
				showIntro = true;
				levelIntroText = Engine::GetInstance().languageManager->GetString("INTRO_FOREST");
			}
			// 2. Mostrar intro de MONTAÑA solo si NO venimos de otra zona de la montaña
			else if (asyncMapFile.find("Mountain_01") != std::string::npos &&
				asyncPreviousMap.find("Mountain_02") == std::string::npos)
			{
				showIntro = true;
				levelIntroText = Engine::GetInstance().languageManager->GetString("INTRO_MOUNTAIN");
			}
			// 3. Mostrar intro de CATACUMBAS solo si NO venimos de otras catacumbas
			else if ((asyncMapFile.find("Catacombs_01_M") != std::string::npos && asyncPreviousMap.find("Catacombs_02_M") == std::string::npos || asyncMapFile.find("Catacombs_01_F") != std::string::npos) &&
				asyncPreviousMap.find("Catacombs_02_F") == std::string::npos )
			{
				showIntro = true;
				levelIntroText = Engine::GetInstance().languageManager->GetString("INTRO_CATACOMBS");
			}
			if (showIntro)
			{
				mapState = MapTransitionState::LEVEL_INTRO;
				levelIntroTimer = 4.0f; // Duración
				introFrameAnim.SetCurrent("focus");
				if (introFrameAnim.GetAnim("focus")) {
					introFrameAnim.GetAnim("focus")->Reset(); 
				}
			}
			else {
				mapState = MapTransitionState::NONE;
				Engine::GetInstance().hud->isHidden = false;
				Player* p = Engine::GetInstance().entityManager->GetPlayer();
				if (p != nullptr) p->isFrozen = false;
			}
		}
	}
	//ESTADO DE INTRO
	else if (mapState == MapTransitionState::LEVEL_INTRO) {

		levelIntroTimer -= dt / 1000.0f;
		Engine::GetInstance().hud->isHidden = true;

		introFrameAnim.Update(dt);

		if (levelIntroTimer <= 0.0f) {
			mapState = MapTransitionState::NONE;

			// Devolvemos el control y mostramos el HUD
			Engine::GetInstance().hud->isHidden = false;
			Player* p = Engine::GetInstance().entityManager->GetPlayer();
			if (p != nullptr) p->isFrozen = false;

		}
	}

	// Pause Menu - ESC o START del gamepad
	bool pauseInput = input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN;

	if (input->IsGamepadConnected() &&
		input->GetGamepadButton(GAMEPAD_START) == KEY_DOWN)
	{
		pauseInput = true;
	}

	if (pauseInput) {
		if (currentMenuTab != GameMenuTab::NONE) {
			ToggleGameMenu(GameMenuTab::NONE);
		}
		else {
			ToggleGameMenu(GameMenuTab::PAUSE_MENU);
		}
	}

	if (dialogueMgr->IsDialogueActive()) {
		return true;
	}
	return true;
}

bool GameScene::PostUpdate() {
	if (isAsyncLoading) {
		int screenW = Engine::GetInstance().window->windowWidth;
		int screenH = Engine::GetInstance().window->windowHeight;
		SDL_Rect fullScreenRect = { 0, 0, screenW, screenH };

		// Dibujar un fondo negro completamente opaco (Alpha a 255)
		Engine::GetInstance().render->DrawRectangleUnscaled(fullScreenRect, 0, 0, 0, 255, true, false);

	// DIBUJAR EN MITAD DE PANTALLA
		int dotCount = (int)(loadingDotsTimer * 3.0f) % 4;

		std::string loadingText = Engine::GetInstance().languageManager->GetString("TXT_LOADING");
		for (int i = 0; i < dotCount; ++i) {
			loadingText += ".";
		}

		SDL_Color textColor = { 255, 255, 255, 255 };

		SDL_Rect textRect = { screenW - 320, screenH - 120, 220, 55 };

		// Dibujamos el texto
		Engine::GetInstance().render->DrawTextCentered(loadingText.c_str(), textRect, textColor, FontType::MENU);

		return true;
	}

	auto sceneManager = Engine::GetInstance().sceneManager;
	if (currentMenuTab != GameMenuTab::NONE) {
		{
			SDL_Texture* currentTextureToDraw = nullptr;

			int screenW, screenH;
			screenW = Engine::GetInstance().window->windowWidth;

			screenH = Engine::GetInstance().window->windowHeight;

			SDL_Rect fullScreenRect = { 0, 0, screenW, screenH };

			Engine::GetInstance().render->DrawRectangleUnscaled(fullScreenRect, 0, 0, 0, 180, true, false);
			Language currentLang = Engine::GetInstance().languageManager->GetCurrentLanguage();
			switch (currentMenuTab) {
			case GameMenuTab::INVENTORY:
				// Si es catalán, muestra texInventoryUI_CA; si no, texInventoryUI
				currentTextureToDraw = (currentLang == Language::CATALAN) ? texInventoryUI_CAT : texInventoryUI;
				break;
			case GameMenuTab::MAP:
				currentTextureToDraw = (currentLang == Language::CATALAN) ? texMapUI_CAT : texMapUI;
				break;
			case GameMenuTab::SKILL_TREE:
				currentTextureToDraw = (currentLang == Language::CATALAN) ? texSkillUI_CAT : texSkillUI;
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

			if (currentMenuTab == GameMenuTab::MAP)
			{
				minimap.DrawMinimap();
			}
			else if (currentMenuTab == GameMenuTab::SKILL_TREE) //Show how much orb the player has
			{
				int currentForceOrbs = GameManager::GetInstance().gameState.currentForceOrbs;
				SDL_Color color = { 255,255,255,255 };
				Engine::GetInstance().render->DrawText(std::to_string(currentForceOrbs).c_str(), 850, 475, 100, 100, color);
			}

			return true;
		}
		// If any menu is open, freeze normal gameplay logic
		if (currentMenuTab != GameMenuTab::NONE) {

			int screenW, screenH;
			screenW = Engine::GetInstance().window->windowWidth;
			screenH = Engine::GetInstance().window->windowHeight;

			SDL_Rect bgRect = { 0, 0, screenW, screenH };

			Engine::GetInstance().render->DrawRectangle(bgRect, 0, 0, 0, 180, true, false);

			return true;
		}
	}

	if (sceneManager->setNewMap && mapState == MapTransitionState::NONE) {

		sceneManager->setNewMap = false;
		Player* p = Engine::GetInstance().entityManager->GetPlayer();
		if (p != nullptr && p->interactuableBody != nullptr) {
			nextMapName = Engine::GetInstance().map->DoorInfo(p->interactuableBody);

			if (p->interactuableBody->ctype == ColliderType::PATH) {
				targetSpawnID = Engine::GetInstance().map->GetPathSpawnID(p->interactuableBody);
			}
			else {
				targetSpawnID = "";
			}
		}
		else {
			nextMapName = "";
			targetSpawnID = "";
		}

		mapState = MapTransitionState::FADING_OUT;
		Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, mapFadeTime);
	}

	if (mapState == MapTransitionState::LEVEL_INTRO) {
		int screenW = Engine::GetInstance().window->windowWidth;
		int screenH = Engine::GetInstance().window->windowHeight;

		// Barras - cinemáticas
		SDL_Rect topBar = { 0, 0, screenW, screenH / 7 };
		SDL_Rect bottomBar = { 0, screenH - (screenH / 7), screenW, screenH / 7 };
		Engine::GetInstance().render->DrawRectangleUnscaled(topBar, 0, 0, 0, 255, true, false);
		Engine::GetInstance().render->DrawRectangleUnscaled(bottomBar, 0, 0, 0, 255, true, false);

		Uint8 textAlpha = 255;
		if (levelIntroTimer > 3.0f) {
			textAlpha = (Uint8)(255.0f * (4.0f - levelIntroTimer));
		}
		else if (levelIntroTimer < 1.0f) {
			textAlpha = (Uint8)(255.0f * levelIntroTimer);
		}

		SDL_Rect textRect = { 0, (screenH / 2) - 50, screenW, 100 };

		if (introFrameTex != nullptr) {

			SDL_Rect exactBounds = Engine::GetInstance().render->GetTextRenderedBounds(levelIntroText.c_str(), textRect, FontType::MENU);

			int paddingVertical = (int)(exactBounds.h * 0.5f);   
			int paddingHorizontal = (int)(exactBounds.h * 2.5f); 

			exactBounds.x -= paddingHorizontal;
			exactBounds.w += paddingHorizontal * 2;
			exactBounds.y -= paddingVertical;
			exactBounds.h += paddingVertical * 2;

			SDL_Rect srcFrame = introFrameAnim.GetCurrentFrame();
			SDL_FRect srcFRect = { (float)srcFrame.x, (float)srcFrame.y, (float)srcFrame.w, (float)srcFrame.h };
			SDL_FRect dstFRect = { (float)exactBounds.x, (float)exactBounds.y, (float)exactBounds.w, (float)exactBounds.h };

			SDL_SetTextureBlendMode(introFrameTex, SDL_BLENDMODE_BLEND);
			SDL_SetTextureAlphaMod(introFrameTex, textAlpha);
			SDL_RenderTexture(Engine::GetInstance().render->renderer, introFrameTex, &srcFRect, &dstFRect);
		}

		
		SDL_Color textColor = { 255, 255, 255, textAlpha };

		Engine::GetInstance().render->DrawTextCentered(levelIntroText.c_str(), textRect, textColor, FontType::MENU);
	}

	return true;
}
bool GameScene::CleanUp() {
	LOG("Freeing Game Scene");

	Engine::GetInstance().languageManager->ClearCallbacks();

	if (loadingThread.joinable()) {
		loadingThread.join();
	}
		// Clean up map and entities
	Engine::GetInstance().entityManager->CleanUp();
	Engine::GetInstance().map->CleanUp();

	// Buttons and Bg
	UnloadTexture(buttonUI);
	UnloadTexture(skillFrameUI);
	UnloadTexture(textBgUI);
	UnloadTexture(sliderThumbTex);


	// Texture Load
	UnloadTexture(texPauseUI);

	UnloadTexture(texMapUI);
	UnloadTexture(texInventoryUI);
	UnloadTexture(texSkillUI);
	UnloadTexture(texMapUI_CAT);
	UnloadTexture(texInventoryUI_CAT);
	UnloadTexture(texSkillUI_CAT);
	//Load Items
	UnloadTexture(texItemKeyCastle);
	UnloadTexture(texItemKeyForest);
	UnloadTexture(texItemKeyMountain);
	UnloadTexture(texItemKeyCatacumbs);
	UnloadTexture(texItemOrb);

	// Power-ups
	UnloadTexture(texItemGlide);
	UnloadTexture(texItemDash);
	UnloadTexture(texItemWallJump);
	UnloadTexture(texItemDoubleJump);
	UnloadTexture(texItemWeapon);

	//Unload Dialogue UI
	UnloadTexture(UIDialogueBoxTex);
	UnloadTexture(princessPortrait);
	UnloadTexture(npcPortrait);
	//UnloadTexture(npcPortrait2);
	//UnloadTexture(npcPortrait3);
	//UnloadTexture(npcPortrait4);

	// Skill upgrade Unload
	UnloadTexture(books_1_1);
	UnloadTexture(books_1_2);
	UnloadTexture(books_2_1);
	UnloadTexture(books_2_2);
	UnloadTexture(books_3_1);
	UnloadTexture(books_3_2);
	UnloadTexture(books_1_1_active);
	UnloadTexture(books_1_2_active);
	UnloadTexture(books_2_1_active);
	UnloadTexture(books_2_2_active);
	UnloadTexture(books_3_1_active);
	UnloadTexture(books_3_2_active);

	//Level intro Unload
	UnloadTexture(introFrameTex);
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
	deleteGroup(dialogueUI);
	deleteGroup(skillPopupUI);

	dialogueUI.clear();
	Engine::GetInstance().dialogueManager->SetDialogueUI(nullptr);
	return true;
}

bool GameScene::OnUIMouseClickEvent(UIElement* uiElement) {

	Engine::GetInstance().audio->PlayFx(uiClick);
	Player* p = Engine::GetInstance().entityManager->GetPlayer();
	auto updateLorePanel = [&](const std::string& itemKey, bool hasItem)
		{
			if (hasItem && itemsLoreDB.find(itemKey) != itemsLoreDB.end())
			{
				const auto& lore = itemsLoreDB[itemKey];
				// Formatamos Título + Descripción
				descPanel->text = lore.name + "\n\n" + lore.description;
			}
			else
			{
				descPanel->text = "???\n\nObjeto desconocido, dicen que esta perdido por el reino.";
			}
		};

	auto updateSkillPopup = [&](const std::string& skillKey, SkillTree skillEnum)
		{
			selectedSkillToBuy = skillEnum;
			selectedSkillKey = skillKey;

			if (skillsLoreDB.find(skillKey) != skillsLoreDB.end())
			{
				const auto& lore = skillsLoreDB[skillKey];
				skillPopupText->text = lore.name + "\n\n" + lore.description + "\nCosto: " + std::to_string(lore.cost) + " Orbe(s)";
			}
			else
			{
				skillPopupText->text = "???\n\nHabilidad desconocida.";
			}
			SetUIGroupVisible(skillPopupUI, true);
			inSkillPopUp = true;
		};

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

	case (int)GameUI_ID::INV_ITEM_WEAPON:
		updateLorePanel("WEAPON", p && p->HasItem(ItemID::WEAPON));
		break;

	case (int)GameUI_ID::INV_ITEM_GLIDE:
		updateLorePanel("GLIDE", p && p->HasItem(ItemID::GLIDE));
		break;

	case (int)GameUI_ID::INV_ITEM_ORB:
		updateLorePanel("STRENGTH_ORB", p && p->HasItem(ItemID::STRENGTH_ORB));
		break;
	case (int)GameUI_ID::SKILL_BOOK_1_1: 
		if (!inSkillPopUp)
		{
			updateSkillPopup("HEALTH_UP", SkillTree::HEALTH_UP); 
		}
		break;

	case (int)GameUI_ID::SKILL_BOOK_1_2: 
		if (!inSkillPopUp)
		{
			updateSkillPopup("IFRAMES_UP", SkillTree::IFRAMES_UP); 
		}
		break;

	case (int)GameUI_ID::SKILL_BOOK_2_1:
		if (!inSkillPopUp)
		{
			updateSkillPopup("SPEED_UP", SkillTree::SPEED_UP); 
		}
		break;

	case (int)GameUI_ID::SKILL_BOOK_2_2: 
		if (!inSkillPopUp)
		{
			updateSkillPopup("FAST_DASH", SkillTree::FAST_DASH);
		}		
		break;

	case (int)GameUI_ID::SKILL_BOOK_3_1: 
		if (!inSkillPopUp)
		{
			updateSkillPopup("UP_ATTACK", SkillTree::UP_ATTACK); 
		}
		break;

	case (int)GameUI_ID::SKILL_BOOK_3_2:
		if (!inSkillPopUp)
		{
			updateSkillPopup("DOWN_ATTACK", SkillTree::DOWN_ATTACK); 
		}
		break;

		// Acciones del Pop-Up
	case (int)GameUI_ID::BTN_SKILL_BACK:
		SetUIGroupVisible(skillPopupUI, false);
		inSkillPopUp = false;
		break;

	case (int)GameUI_ID::BTN_SKILL_BUY:
		if (p != nullptr && !selectedSkillKey.empty()) {
			int cost = skillsLoreDB[selectedSkillKey].cost;

			p->UnlockSkill(selectedSkillToBuy, cost);
			UpdateSkillVisuals();
		}
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
		topBarElements.push_back(uiManager->CreateUIElement(UIElementType::BUTTON, def.id, def.keyId, startX, yPos, wPerc, hPerc, sceneObserver));
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

	float centerX = 0.40f;
	float centerXKey = 0.39f;

	float centerY = 0.470f;

	float offsetX = 0.13f;
	float offsetXKey = 0.15f;

	float offsetY = 0.1f;
	float offsetYKey = 0.30f;

	struct InventorySlotDef {
		GameUI_ID id;
		const char* name;
		float relX, relY;
		float w, h;
		SDL_Texture* tex;
	};

	std::vector<InventorySlotDef> slots = {
		// ARMA (Centro)
		{ GameUI_ID::INV_ITEM_WEAPON, "", centerX, centerY,baseSize + float(0.07), squareH + float(0.14), texItemWeapon},

		// AMULETOS (Vertices)
		{ GameUI_ID::INV_ITEM_GLIDE, "", centerX - offsetX,			centerY - offsetY ,baseSize, squareH, texItemGlide },
		{ GameUI_ID::INV_ITEM_DASH, "", centerX + offsetX ,		centerY - offsetY ,baseSize, squareH, texItemDash },
		{ GameUI_ID::INV_ITEM_DOUBLE_JUMP, "", centerX - offsetX,	centerY + offsetY ,baseSize, squareH, texItemDoubleJump },
		{ GameUI_ID::INV_ITEM_WALL_JUMP, "", centerX + offsetX,		centerY + offsetY ,baseSize, squareH, texItemWallJump },

		// ITEMS
		{ GameUI_ID::INV_ITEM_KEY, "", centerXKey - offsetXKey, centerY + offsetYKey,						   baseSize - float(0.01), squareH - float(0.02), texItemKeyCastle},
		{ GameUI_ID::INV_ITEM_KEYCASTLE, "", centerXKey - offsetXKey + float(0.08),	centerY + offsetYKey, baseSize - float(0.01), squareH - float(0.02), texItemKeyCastle },
		{ GameUI_ID::INV_ITEM_KEYFOREST, "", centerXKey - offsetXKey + float(0.16),	centerY + offsetYKey, baseSize - float(0.01), squareH - float(0.02), texItemKeyForest },
		{ GameUI_ID::INV_ITEM_KEYMOUNTAIN, "", centerXKey - offsetXKey + float(0.24),	centerY + offsetYKey, baseSize - float(0.01), squareH - float(0.02), texItemKeyMountain },
		{ GameUI_ID::INV_ITEM_KEYCATACUMBS, "", centerXKey - offsetXKey + float(0.32),	centerY + offsetYKey, baseSize - float(0.01), squareH - float(0.02), texItemKeyCatacumbs },
	};

	for (const auto& slot : slots) {
		auto btn = uiManager->CreateUIElement(UIElementType::BUTTON, (int)slot.id, slot.name, slot.relX, slot.relY, slot.w, slot.h, sceneObserver);
		if (slot.id != GameUI_ID::INV_ITEM_GLIDE && slot.id != GameUI_ID::INV_ITEM_DASH && slot.id != GameUI_ID::INV_ITEM_DOUBLE_JUMP && slot.id != GameUI_ID::INV_ITEM_WALL_JUMP)
		{
			btn->SetBgTexture(skillFrameUI);

		}

		// Si le hemos asignado una textura, se la ponemos al botón
		if (slot.tex != nullptr) {
			btn->SetTexture(slot.tex);
		}
		inventoryUI.push_back(btn);
	}

	descPanel = uiManager->CreateUIElement(
		UIElementType::ITEM_INFO_BOX, // Nuevo tipo dedicado
		(int)GameUI_ID::INV_DESC_TEXT,
		"Selecciona un objeto para ver su historia...",
		0.72f, 0.55f, 0.20f, 0.60f,
		sceneObserver
	);

	descPanel->SetBgTexture(textBgUI);
	inventoryUI.push_back(descPanel);

}

void GameScene::CreateSkillUpgradeUI() {

	auto uiManager = Engine::GetInstance().uiManager;
	Module* sceneObserver = (Module*)Engine::GetInstance().sceneManager.get();

	int sw = Engine::GetInstance().window->windowWidth;
	int sh = Engine::GetInstance().window->windowHeight;

	float aspect = (float)sw / (float)sh;

	float baseSize = 0.12f;
	float squareH = baseSize * aspect;

	float centerX = 0.37f;
	float centerY = 0.50f;
	float offsetX = 0.07f;
	float offsetY = 0.20f;

	struct SkillSlotDef {
		GameUI_ID id;
		const char* name;
		float relX, relY;
		float w, h;
		SDL_Texture* tex;
	};

	std::vector<SkillSlotDef> slots = {
		{ GameUI_ID::SKILL_BOOK_1_1, "", centerX - offsetX + float(0.02), centerY - offsetY - float(0.015),baseSize, squareH, books_1_1}, // Arriba derecha
		{ GameUI_ID::SKILL_BOOK_1_2, "", centerX + offsetX , centerY - offsetY - float(0.015),baseSize, squareH, books_1_2 }, // Arriba izquierda

		{ GameUI_ID::SKILL_BOOK_2_1, "", centerX - offsetX + float(0.05), centerY, baseSize, squareH, books_2_1 }, // Medio izquierda
		{ GameUI_ID::SKILL_BOOK_2_2, "", centerX + offsetX + float(0.02), centerY,baseSize, squareH, books_2_2 }, // Medio derecha

		{ GameUI_ID::SKILL_BOOK_3_1, "", centerX - offsetX + float(0.05) , centerY + offsetY,baseSize, squareH, books_3_1 }, // Abajo derecha
		{ GameUI_ID::SKILL_BOOK_3_2, "", centerX + offsetX + float(0.04), centerY + offsetY,baseSize, squareH, books_3_2 }, // Abajo Izquierda 

		// ITEMS
		{ GameUI_ID::INV_ITEM_ORB, "", centerX + offsetX + float(0.1525), centerY + offsetY + float(0.0225),baseSize * float(0.65), squareH * float(0.65), texItemOrb } // Orbe
	};

	for (const auto& slot : slots) {
		auto btn = uiManager->CreateUIElement(UIElementType::BUTTON, (int)slot.id, slot.name, slot.relX, slot.relY, slot.w, slot.h, sceneObserver);

		if (slot.tex != nullptr) {
			btn->SetTexture(slot.tex);
		}
		skillUI.push_back(btn);
	}
}

void GameScene::CreateSkillPopupUI() {
	auto uiManager = Engine::GetInstance().uiManager;
	Module* sceneObserver = (Module*)Engine::GetInstance().sceneManager.get();

	skillPopupText = uiManager->CreateUIElement(
		UIElementType::ITEM_INFO_BOX,
		(int)GameUI_ID::PANEL_SKILL_POPUP,
		"",
		0.5f, 0.4f, 0.3f, 0.4f, sceneObserver);

	skillPopupText->SetBgTexture(textBgUI);
	skillPopupUI.push_back(skillPopupText);

	// Botón Comprar
	auto btnBuy = uiManager->CreateUIElement(UIElementType::BUTTON, (int)GameUI_ID::BTN_SKILL_BUY, "Desbloquear", 0.4f, 0.65f, 0.1f, 0.05f, sceneObserver);
	btnBuy->SetBgTexture(buttonUI);
	skillPopupUI.push_back(btnBuy);

	// Botón Atrás
	auto btnBack = uiManager->CreateUIElement(UIElementType::BUTTON, (int)GameUI_ID::BTN_SKILL_BACK, "Cancelar", 0.6f, 0.65f, 0.1f, 0.05f, sceneObserver);
	btnBack->SetBgTexture(buttonUI);
	skillPopupUI.push_back(btnBack);

	SetUIGroupVisible(skillPopupUI, false);
}


void GameScene::CreatePauseMenuUI() {
	auto uiManager = Engine::GetInstance().uiManager;
	Module* sceneObserver = (Module*)Engine::GetInstance().sceneManager.get();

	float pW = 0.25f, pH = 0.08f;
	float pY = 0.40f, pSpacing = 0.1f;

	ButtonDef pauseBtnDefs[] = {
		{ (int)GameUI_ID::BTN_PAUSE_RESUME,   "BTN_PAUSE_RESUME" },
		{ (int)GameUI_ID::BTN_PAUSE_OPTIONS,  "BTN_PAUSE_OPTIONS" },
		{ (int)GameUI_ID::BTN_PAUSE_MAINMENU, "BTN_PAUSE_MAINMENU" }
	};

	for (const auto& def : pauseBtnDefs) {
		std::string localizedText = Engine::GetInstance().languageManager->GetString(def.keyId);
		auto btn = uiManager->CreateUIElement(UIElementType::BUTTON, def.id, localizedText.c_str(), 0.5f, pY, pW, pH, sceneObserver);

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
	float pY = 0.4f, pSpacing = 0.1f;

	auto sldMus = uiManager->CreateUIElement(UIElementType::SLIDER, (int)GameUI_ID::SLD_MUSIC, Engine::GetInstance().languageManager->GetString("SLD_MUSIC").c_str(), 0.5f, pY, 0.3f, 0.05f, sceneObserver);
	if (auto* s = dynamic_cast<UISlider*>(sldMus.get())) {
		s->SetValue(Engine::GetInstance().audio->GetMusicVolume());
		s->SetThumbTexture(sliderThumbTex);
	}
	pauseOptionsUI.push_back(sldMus);
	pY += pSpacing;

	auto sldFx = uiManager->CreateUIElement(UIElementType::SLIDER, (int)GameUI_ID::SLD_FX, Engine::GetInstance().languageManager->GetString("SDL_FX").c_str(), 0.5f, pY, 0.3f, 0.05f, sceneObserver);
	if (auto* s = dynamic_cast<UISlider*>(sldFx.get())) {
		s->SetValue(Engine::GetInstance().audio->GetSFXVolume());
		s->SetThumbTexture(sliderThumbTex);
	}
	pauseOptionsUI.push_back(sldFx);
	pY += pSpacing;

	auto chkFull = uiManager->CreateUIElement(UIElementType::CHECKBOX, (int)GameUI_ID::CHK_FULLSCREEN, Engine::GetInstance().languageManager->GetString("CHK_FULLSCREEN").c_str(), 0.4f, pY, 0.05f, 0.05f, sceneObserver);
	if (auto* c = dynamic_cast<UICheckBox*>(chkFull.get())) c->isChecked = Engine::GetInstance().window->IsFullscreen();
	pauseOptionsUI.push_back(chkFull);
	pY += pSpacing;

	pauseOptionsUI.push_back(uiManager->CreateUIElement(UIElementType::BUTTON, (int)GameUI_ID::BTN_OPTIONS_BACK, Engine::GetInstance().languageManager->GetString("BTN_OPTIONS_BACK").c_str(), 0.5f, pY, pW, pH, sceneObserver));
}

void GameScene::CreateDialogueUI() {
	//Engine::GetInstance().dialogueManager->StartDialogue("Prueba1");
	auto uiManager = Engine::GetInstance().uiManager;
	Module* sceneObserver = (Module*)Engine::GetInstance().sceneManager.get();

	// Usamos el mismo patrón que tus otros elementos
	std::shared_ptr<UIElement> rawDialogueBox = uiManager->CreateUIElement(
		UIElementType::DIALOGUE_BOX, 99, "", 0.5f, 0.8f, 0.7f, 0.3f, sceneObserver);

	UIDialogueBox* dBox = dynamic_cast<UIDialogueBox*>(rawDialogueBox.get());
	if (dBox != nullptr) {
		dBox->SetBackgroundTexture(UIDialogueBoxTex);

		dBox->AddPortrait("Rose", princessPortrait);
		dBox->AddPortrait("Pep", npcPortrait);

		// Vincular con el Manager
		Engine::GetInstance().dialogueManager->SetDialogueUI(dBox);

		// Añadir al grupo de control
		dialogueUI.push_back(rawDialogueBox);
	}
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
	Player* p = Engine::GetInstance().entityManager->GetPlayer();
	if (p == nullptr) return;

	for (auto& btn : inventoryUI) {
		bool hasItem = false;

		// We check which item this button is and ask the Player if they have it
		switch (btn->id) {
		case (int)GameUI_ID::INV_ITEM_WEAPON:
			hasItem = p->HasItem(ItemID::WEAPON);
			break;
		case (int)GameUI_ID::INV_ITEM_GLIDE:
			hasItem = p->HasItem(ItemID::GLIDE);
			break;
		case (int)GameUI_ID::INV_ITEM_KEY:
			hasItem = p->HasItem(ItemID::KEY);
			break;
		case (int)GameUI_ID::INV_ITEM_ORB:
			hasItem = p->HasItem(ItemID::STRENGTH_ORB);
			break;

		default:
			continue;
		}
		if (hasItem) {
			btn->color = { 255, 255, 255, 255 }; // White tint --> Normal texture
		}
		else {
			btn->color = { 110, 110, 110, 255 };    // Gray tint
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

	else if (currentMenuTab == GameMenuTab::SKILL_TREE) {
		UpdateSkillVisuals();
	}

	SetUIGroupVisible(inventoryUI, currentMenuTab == GameMenuTab::INVENTORY);
	SetUIGroupVisible(mapUI, currentMenuTab == GameMenuTab::MAP);
	SetUIGroupVisible(skillUI, currentMenuTab == GameMenuTab::SKILL_TREE);


	SetUIGroupVisible(pauseMainUI, currentMenuTab == GameMenuTab::PAUSE_MENU);
	SetUIGroupVisible(pauseOptionsUI, currentMenuTab == GameMenuTab::PAUSE_OPTIONS);
}

void GameScene::UpdateSkillVisuals() {
	auto& gameState = GameManager::GetInstance().gameState;

	for (auto& btn : skillUI) {
		switch (btn->id) {
		case (int)GameUI_ID::SKILL_BOOK_1_1:
			btn->SetTexture(gameState.stHealthUp ? books_1_1_active : books_1_1);
			break;
		case (int)GameUI_ID::SKILL_BOOK_1_2:
			btn->SetTexture(gameState.stIframesUp ? books_1_2_active : books_1_2);
			break;
		case (int)GameUI_ID::SKILL_BOOK_2_1:
			btn->SetTexture(gameState.stSpeedUp ? books_2_1_active : books_2_1);
			break;
		case (int)GameUI_ID::SKILL_BOOK_2_2:
			btn->SetTexture(gameState.stFastDash ? books_2_2_active : books_2_2);
			break;
		case (int)GameUI_ID::SKILL_BOOK_3_1:
			btn->SetTexture(gameState.stUpAttack ? books_3_1_active : books_3_1);
			break;
		case (int)GameUI_ID::SKILL_BOOK_3_2:
			btn->SetTexture(gameState.stDownAttack ? books_3_2_active : books_3_2);
			break;
		}
	}
}

void GameScene::UpdateUILanguage() {
	auto langMgr = Engine::GetInstance().languageManager;

	if (pauseMainUI.size() >= 3) {
		pauseMainUI[0]->text = langMgr->GetString("BTN_PAUSE_RESUME");
		pauseMainUI[1]->text = langMgr->GetString("BTN_PAUSE_OPTIONS"); 
		pauseMainUI[2]->text = langMgr->GetString("BTN_PAUSE_MAINMENU");
	}
	if (pauseOptionsUI.size() >= 4) {
		pauseOptionsUI[0]->text = langMgr->GetString("SLD_MUSIC");
		pauseOptionsUI[1]->text = langMgr->GetString("SLD_FX");
		pauseOptionsUI[2]->text = langMgr->GetString("CHK_FULLSCREEN");
		pauseOptionsUI[3]->text = langMgr->GetString("BTN_BACK");
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