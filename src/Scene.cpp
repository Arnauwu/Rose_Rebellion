#include "Engine.h"
#include "Input.h"
#include "Textures.h"
#include "Audio.h"
#include "Render.h"
#include "Window.h"
#include "Scene.h"
#include "Log.h"
#include "Entity.h"
#include "EntityManager.h"
#include "Player.h"
#include "Map.h"
#include "Item.h"
#include "UIManager.h"
#include "UICheckBox.h"
#include "UISlider.h"

Scene::Scene() : Module()
{
	name = "scene";
}

// Destructor
Scene::~Scene()
{
}

// Called before render is available
bool Scene::Awake()
{
	LOG("Loading Scene");
	bool ret = true;

	return ret;
}

// Called before the first frame
bool Scene::Start()
{
	LoadScene(currentScene);
	return true;
}

// Called each loop iteration
bool Scene::PreUpdate()
{
	return true;
}

// Called each loop iteration
bool Scene::Update(float dt)
{
	switch (currentScene)
	{
	case SceneID::INTRO:
		UpdateIntro(dt);

		break;
	case SceneID::MENU:
		UpdateMainMenu(dt);

		break;
	case SceneID::GAME:
		UpdateGame(dt);

		break;
	case SceneID::GAMEOVER:
		UpdateGameOver(dt);

		break;
	case SceneID::WIN:
		UpdateWin(dt);

		break;
	default:
		break;
	}

	return true;
}

// Called each loop iteration
bool Scene::PostUpdate()
{
	bool ret = true;

	switch (currentScene)
	{

	case SceneID::GAME:
		PostUpdateGame();
		break;
	}

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN) {
		if (currentScene != SceneID::MENU) ChangeScene(SceneID::MENU);
		else return false;
	}

	if (setNewMap == true)
	{
		LoadMap("");
	}

	return ret;
}

// Called before quitting
bool Scene::CleanUp()
{
	LOG("Freeing scene");
	UnloadCurrentScene();

	return true;
}

void Scene::LoadMap(std::string mapFile)
{

	//Load the map. 
	if (mapFile == "")
	{
		mapFile = Engine::GetInstance().map->DoorInfo(player->interactuableBody);
	}
	setNewMap = false;

	Engine::GetInstance().entityManager->CleanUp();
	player = nullptr;

	Engine::GetInstance().map->CleanUp();

	Engine::GetInstance().map->Load("Assets/Maps/", mapFile);
	Engine::GetInstance().map->SpawnEntities();

	Engine::GetInstance().entityManager->Start();

}

// *********************************************
// Scene change functions
// *********************************************

void Scene::LoadScene(SceneID newScene)
{
	auto& engine = Engine::GetInstance();

	switch (newScene)
	{
	case SceneID::INTRO:
		LoadIntro();

		break;
	case SceneID::MENU:
		LoadMainMenu();

		break;
	case SceneID::GAME:
		LoadGame();

		break;
	case SceneID::GAMEOVER:
		LoadGameOver();

		break;
	case SceneID::WIN:
		LoadWin();

		break;
	}
}

void Scene::ChangeScene(SceneID newScene)
{
	UnloadCurrentScene();
	currentScene = newScene;
	LoadScene(currentScene);
}

void Scene::UnloadCurrentScene() {

	switch (currentScene)
	{
	case SceneID::INTRO:
		UnloadIntro();

		break;
	case SceneID::MENU:
		UnloadMainMenu();

		break;
	case SceneID::GAME:
		UnloadGame();

		break;
	case SceneID::WIN:
		UnloadWin();
		break;
	case SceneID::GAMEOVER:
		UnloadGameOver();

		break;
	}
}

// *********************************************
//				General Helpers
// *********************************************

Vector2D Scene::GetPlayerPosition()
{
	if (player) return player->GetPosition();
	else Vector2D(0, 0);
}

// MouseControl
bool Scene::OnUIMouseClickEvent(UIElement* uiElement)
{
	if (currentScene == SceneID::MENU)
	{
		HandleMainMenuUIEvents(uiElement);
	}

	else if (currentScene == SceneID::GAME)
	{
		HandleGameMenuUIEvents(uiElement);
	}

	return true;
}

void Scene::RecalculateBackgroundScale()
{
	if (menuBackground == nullptr) return;

	int screenW, screenH;
	Engine::GetInstance().window->GetWindowSize(screenW, screenH);

	float texW, texH;
	SDL_GetTextureSize(menuBackground, &texW, &texH);

	float engineScale = (float)Engine::GetInstance().window->GetScale();

	bgScaleX = (float)screenW / (texW * engineScale);
	bgScaleY = (float)screenH / (texH * engineScale);
}

// *********************************************
//				Show Helpers
// *********************************************

void Scene::ShowGameMenu(bool show) {
	if (show) menuState = MenuState::PAUSE;
	else menuState = MenuState::NONE;

	for (auto& elem : gameMenuElements) {
		elem->visible = show;

		if (show == true) {
			elem->state = UIElementState::NORMAL;
		}
		else {
			elem->state = UIElementState::DISABLED;
		}
	}

	ShowGameSettings(false);
}

void Scene::ShowGameSettings(bool show) {
	if (show == true) {
		menuState = MenuState::PAUSE_SETTINGS;
	}

	for (auto& elem : gameMenuElements) {
		if (show == true) {
			elem->visible = false;
			elem->state = UIElementState::DISABLED;
		}
	}

	for (auto& elem : settingsMenuElements) {
		if (show == true) {
			elem->visible = true;
			elem->state = UIElementState::NORMAL;
		}
		else {
			elem->visible = false;
			elem->state = UIElementState::DISABLED;
		}
	}
}
void Scene::ShowSettings(bool show) {

	for (auto& elem : mainMenuElements) {
		if (show == true) {
			// Si estamos mostrando Settings -> OCULTAR Menú Principal
			elem->visible = false;
			elem->state = UIElementState::DISABLED;
		}
		else {
			// Contrario no mostrar
			elem->visible = true;
			elem->state = UIElementState::NORMAL;
		}
	}
	//GESTIONAR EL MENÚ DE SETTINGS
	for (auto& elem : settingsMenuElements) {
		if (show == true) {
			elem->visible = true;
			elem->state = UIElementState::NORMAL;
		}
		else {
			elem->visible = false;
			elem->state = UIElementState::DISABLED;
		}
	}
}

// *********************************************
//					INTRO
// *********************************************

void Scene::LoadIntro()
{
	Engine::GetInstance().window->GetWindowSize(windowW, windowH);
	screenRect = { 0, 0, windowW, windowH };

	//introTexture = Engine::GetInstance().textures->Load("Assets/Textures/Intro_Menus/Intro.jpg");
	introTimer = 0.f; // TO DO: METER PULSAR TECLA EN VEZ DE TIMER --> No lo meto por comodidad mientras edito
}

void Scene::UpdateIntro(float dt)
{
	Engine::GetInstance().render->DrawRectangle(screenRect, 10, 70, 45, 255, true, false);
	introTimer -= dt;

	if (introTimer <= 0.0f || Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN) {
		ChangeScene(SceneID::MENU);
	}

	if (introTexture != nullptr) {

		Engine::GetInstance().render->DrawTexture(introTexture, 0, 0);
	}
}

void Scene::UnloadIntro()
{
	// Descargar textura para liberar memoria
	if (introTexture != nullptr) {
		Engine::GetInstance().textures->UnLoad(introTexture);
		introTexture = nullptr;
	}
}

// *********************************************
//					MAIN MENU 
// *********************************************

void Scene::LoadMainMenu()
{
	mainMenuElements.clear();
	settingsMenuElements.clear();

	if (menuBackground == nullptr) {
		menuBackground = Engine::GetInstance().textures->Load("Assets/Textures/Fons_Exterior_Castell.png");
	}
	RecalculateBackgroundScale();

	float wPerc = 0.25f; // 25% of the screen width
	float hPerc = 0.08f; // 8% of the top of the screen.
	float spacing = 0.1f;
	float currentY = 0.35f;

	// MAIN MENU
	auto btnPlay = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 1, "Start Game", 0.5f, currentY, wPerc, hPerc, this);
	mainMenuElements.push_back(btnPlay);
	currentY += spacing;

	auto btnContinue = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 2, "Load Game", 0.5f, currentY, wPerc, hPerc, this);
	mainMenuElements.push_back(btnContinue);
	currentY += spacing;

	auto btnSettings = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 3, "Settings", 0.5f, currentY, wPerc, hPerc, this);
	mainMenuElements.push_back(btnSettings);
	currentY += spacing;

	auto btnExit = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 5, "Quit Game", 0.5f, currentY, wPerc, hPerc, this);
	mainMenuElements.push_back(btnExit);

	// MAIN MENU SETTINGS
	float setY = 0.35f;

	auto sldMusic = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::SLIDER, 6, "Music Volume", 0.5f, setY, 0.3f, 0.05f, this);
	settingsMenuElements.push_back(sldMusic);
	setY += spacing;

	auto sldFX = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::SLIDER, 7, "FX Volume", 0.5f, setY, 0.3f, 0.05f, this);
	settingsMenuElements.push_back(sldFX);
	setY += spacing;

	auto chkFull = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::CHECKBOX, 8, "Fullscreen", 0.5f, setY, 0.05f, 0.05f, this);
	settingsMenuElements.push_back(chkFull);
	setY += spacing;

	auto btnBack = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 9, "BACK", 0.5f, setY, wPerc, hPerc, this);
	settingsMenuElements.push_back(btnBack);

	ShowSettings(false);
	ShowGameMenu(false);
	ShowGameSettings(false);
}

void Scene::UpdateMainMenu(float dt)
{
	if (menuBackground != nullptr) {
		int screenW, screenH;
		Engine::GetInstance().window->GetWindowSize(screenW, screenH);

		SDL_Rect fullScreenRect = { 0, 0, screenW, screenH };

		// Dibujamos la textura forzándola a ocupar ese rectángulo
		Engine::GetInstance().render->DrawTextureScaled(menuBackground, fullScreenRect);
	}
}

void Scene::UnloadMainMenu() {
	Engine::GetInstance().uiManager->CleanUp();

	if (menuBackground != nullptr) {
		Engine::GetInstance().textures->UnLoad(menuBackground);
		menuBackground = nullptr;
	}
	mainMenuElements.clear();
	settingsMenuElements.clear();
	gameMenuElements.clear();
	gameMenuElements.clear();
}

void Scene::HandleMainMenuUIEvents(UIElement* uiElement)
{
	switch (uiElement->id)
	{
		// --- MENÚ PRINCIPAL ---
	case 1: // PLAY
		LOG("Boton Play presionado");
		//Player::ResetSavedCheckpoint();
		ChangeScene(SceneID::GAME);
		break;

	case 2: // CONTINUE
		LOG("Boton Continue presionado");
		/*if (CanContinueGame()) {
			loadingSavedGame = true;

			if (savedLevel == 1) ChangeScene(SceneID::CASTLE);
			else if (savedLevel == 2) ChangeScene(SceneID::LEVEL2);
		}*/
		break;


	case 3: // SETTINGS
		LOG("Ir a Settings");
		ShowSettings(true);
		Engine::GetInstance().input->ClearMouseInput();
		break;

	case 5: // EXIT
		LOG("Salir del juego");
		exitGame = true;
		break;


		// --- MENÚ SETTINGS ---
	case 6: // MUSIC SLIDER
	{
		UISlider* slider = (UISlider*)uiElement;
		float volume = slider->GetValue();

		Engine::GetInstance().audio->SetMusicVolume(volume);
		break;
	}

	case 7: // FX SLIDER
	{
		UISlider* slider = (UISlider*)uiElement;
		float volume = slider->GetValue();

		Engine::GetInstance().audio->SetSFXVolume(volume);
		break;
	}

	case 8: // FULLSCREEN CHECKBOX
	{
		UICheckBox* checkbox = (UICheckBox*)uiElement;
		Engine::GetInstance().window->SetFullscreen(checkbox->isChecked);
		RecalculateBackgroundScale();

		break;
	}

	case 9: // BACK de Settings
		ShowSettings(false);
		break;

	default:
		break;
	}
}

// *********************************************
//					GAME MENU 
// *********************************************

void Scene::LoadGameMenu() {
	gameMenuElements.clear();
	settingsMenuElements.clear();

	float wPerc = 0.25f;
	float hPerc = 0.08f;
	float spacing = 0.11f;
	float currentY = 0.4f;

	// PAUSE MENU
	auto btnResume = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 11, "RESUME", 0.5f, currentY, wPerc, hPerc, this);
	gameMenuElements.push_back(btnResume);
	currentY += spacing;

	auto btnSettings = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 12, "SETTINGS", 0.5f, currentY, wPerc, hPerc, this);
	gameMenuElements.push_back(btnSettings);
	currentY += spacing;

	auto btnExit = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 13, "MAIN MENU", 0.5f, currentY, wPerc, hPerc, this);
	gameMenuElements.push_back(btnExit);

	// PAUSE SETTINGS
	float setY = 0.35f;

	auto sldMusic = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::SLIDER, 6, "Music Volume", 0.5f, setY, 0.3f, 0.05f, this);
	settingsMenuElements.push_back(sldMusic);
	setY += spacing;

	auto sldFX = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::SLIDER, 7, "FX Volume", 0.5f, setY, 0.3f, 0.05f, this);
	settingsMenuElements.push_back(sldFX);
	setY += spacing;

	auto chkFull = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::CHECKBOX, 8, "Fullscreen", 0.5f, setY, 0.05f, 0.05f, this);
	settingsMenuElements.push_back(chkFull);
	setY += spacing;

	auto btnBackPause = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 14, "BACK", 0.5f, setY, wPerc, hPerc, this);
	settingsMenuElements.push_back(btnBackPause);

	ShowGameMenu(false);
	ShowGameSettings(false);
}

void Scene::HandleGameMenuUIEvents(UIElement* uiElement)
{
	switch (uiElement->id)
	{
	case 11: // RESUME
		ShowGameMenu(false);
		break;

	case 12: // SETTINGS (Dentro de la pausa)
		ShowGameSettings(true);
		break;

	case 13: // EXIT TO MAIN MENU
		//isGamePaused = false;

		ChangeScene(SceneID::MENU);

		break;

	case 14: // BACK PAUSE
		ShowGameSettings(false);
		ShowGameMenu(true);
		break;


	case 6: // MUSIC SLIDER
	{
		UISlider* slider = (UISlider*)uiElement;
		float volume = slider->GetValue();

		Engine::GetInstance().audio->SetMusicVolume(volume);
		break;
	}

	case 7: // FX SLIDER
	{
		UISlider* slider = (UISlider*)uiElement;
		float volume = slider->GetValue();

		Engine::GetInstance().audio->SetSFXVolume(volume);
		break;
	}

	case 8: // FULLSCREEN CHECKBOX
	{
		UICheckBox* checkbox = (UICheckBox*)uiElement;
		Engine::GetInstance().window->SetFullscreen(checkbox->isChecked);
		break;
	}
	default:
		break;
	}
}

void Scene::LoadGame()
{
	lastLevelPlayed = SceneID::GAME;
	LoadMap("MapTemplate.tmx");
	if (player != nullptr) {
		player->position.setX(10);
		player->position.setY(10);
	}
}

void Scene::UpdateGame(float dt)
{
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_N) == KEY_DOWN) {}
}

void Scene::PostUpdateGame()
{

}

void Scene::UnloadGame() {

	// Clean up map and entities
	Engine::GetInstance().entityManager->CleanUp();
	Engine::GetInstance().map->CleanUp();
}

void Scene::LoadLevelTransition() {
	transitionTimer = 2000.0f;
}

void Scene::UpdateLevelTransition(float dt) {
	transitionTimer -= dt;
}
void Scene::UnloadLevelTransition() {

}

void Scene::LoadGameOver() {
	gameOverTimer = 2000.0f;
}

void Scene::UpdateGameOver(float dt) {

	if (gameOverTimer <= 0.0f) ChangeScene(lastLevelPlayed);
}

void Scene::UnloadGameOver()
{
}

void Scene::LoadWin()
{

}

void Scene::UpdateWin(float dt)
{

}

void Scene::UnloadWin()
{

}