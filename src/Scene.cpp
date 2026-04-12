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

	//else if (currentScene == SceneID::WIN)
	//{
	//	if (uiElement->id == 21) // MENU
	//	{
	//		ChangeScene(SceneID::MENU);
	//	}
	//}
	return true;
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
	introTimer = 3000.0f;
}

void Scene::UpdateIntro(float dt)
{
	Engine::GetInstance().render->DrawRectangle(screenRect, 10, 70, 45, 255, true, false); //Intro Verde mismo

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
	//menuBackground = Engine::GetInstance().textures->Load("Assets/Textures/Fons_Exterior_Castell.png");

	mainMenuElements.clear();
	settingsMenuElements.clear();

	Engine::GetInstance().window->GetWindowSize(windowW, windowH);
	screenRect = { 0, 0, windowW, windowH };

	int btnW = 150;
	int btnH = 40;

	int sliderW = 200;
	int sliderH = 20;
	int spacing = 20; // Espacio entre botones

	int mainX = (windowW / 2) - (btnW / 2);
	int currentY = (int)(windowH * 0.4f);

	// ID 1. PLAY
	auto btnPlay = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 1, "PLAY", { mainX, currentY, btnW, btnH }, this);
	mainMenuElements.push_back(btnPlay);
	currentY += btnH + spacing; // Bajamos la Y para el siguiente botón

	// ID 2. CONTINUE
	auto btnContinue = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 2, "CONTINUE", { mainX, currentY, btnW, btnH }, this);
	//if (!CanContinueGame()) btnContinue->state = UIElementState::DISABLED;
	mainMenuElements.push_back(btnContinue);
	currentY += btnH + spacing;

	// ID 3. SETTINGS
	auto btnSettings = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 3, "SETTINGS", { mainX, currentY, btnW, btnH }, this);
	mainMenuElements.push_back(btnSettings);
	currentY += btnH + spacing;

	// ID 5. EXIT
	auto btnExit = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 5, "EXIT", { mainX, currentY, btnW, btnH }, this);
	mainMenuElements.push_back(btnExit);

	// --- SETTING MENU ---

	currentY = (int)(windowW * 0.45f);
	int sliderX = (windowH / 2) - (sliderW / 2);

	// ID 6: MUSIC SLIDER
	auto sldMusic = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::SLIDER, 6, "Music Volume", { sliderX, currentY, sliderW, sliderH }, this);
	settingsMenuElements.push_back(sldMusic);
	currentY += sliderH + spacing + 20;

	// ID 7: FX SLIDER
	auto sldFX = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::SLIDER, 7, "FX Volume", { sliderX, currentY, sliderW, sliderH }, this);
	settingsMenuElements.push_back(sldFX);
	currentY += sliderH + spacing + 20;

	// ID 8: FULLSCREEN CHECKBOX
	/*int checkSize = 30;
	auto chkFull = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::CHECKBOX, 8, "Fullscreen", { sliderX + 40, currentY, checkSize, checkSize }, this);
	settingsMenuElements.push_back(chkFull);
	currentY += checkSize + spacing + 10;*/

	// ID 9: BACK BUTTON
	auto btnBack = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 9, "BACK", { mainX, currentY + 20, btnW, btnH }, this);
	settingsMenuElements.push_back(btnBack);

	ShowSettings(false);
	ShowGameMenu(false);
	ShowGameSettings(false);
}

void Scene::UpdateMainMenu(float dt) {
	if (menuBackground != nullptr) {
		Engine::GetInstance().render->DrawTexture(menuBackground, 0, 0);
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

	int w, h;
	Engine::GetInstance().window->GetWindowSize(w, h);
	float zoom = Engine::GetInstance().render->GetZoom();
	// Coordenadas
	int btnW = (int)(150 / zoom);
	int btnH = (int)(40 / zoom);
	int spacing = (int)(20 / zoom);
	int centerX = (int)((w / 2) / zoom) - (btnW / 2);
	int centerY = (int)((h / 2) / zoom) - 100;

	// --- BOTONES DE PAUSA ---

	// ID 11: RESUME
	auto btnResume = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 11, "RESUME", { centerX, centerY, btnW, btnH }, this);
	gameMenuElements.push_back(btnResume);
	centerY += btnH + spacing;

	// ID 12: SETTINGS (En juego)
	auto btnSettings = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 12, "SETTINGS", { centerX, centerY, btnW, btnH }, this);
	gameMenuElements.push_back(btnSettings);
	centerY += btnH + spacing;

	// ID 13: EXIT TO MAIN MENU
	auto btnExit = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 13, "MAIN MENU", { centerX, centerY, btnW, btnH }, this);
	gameMenuElements.push_back(btnExit);

	// --- SETTINGS - EN PAUSE MENU -- LO MISMO

	int sliderW = (int)(200 / zoom);
	int sliderX = (w / 2 / zoom) - (sliderW / 2);
	int setY = (h / 2 / zoom) - 80;

	auto sldMusic = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::SLIDER, 6, "Music Volume", { sliderX, setY, sliderW, 20 }, this);
	settingsMenuElements.push_back(sldMusic);
	setY += 50;

	auto sldFX = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::SLIDER, 7, "FX Volume", { sliderX, setY, sliderW, 20 }, this);
	settingsMenuElements.push_back(sldFX);
	setY += 50;

	auto chkFull = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::CHECKBOX, 8, "Fullscreen", { sliderX + 40, setY, 30, 30 }, this);
	settingsMenuElements.push_back(chkFull);
	setY += 50;

	// ID 14: BACK 
	auto btnBackPause = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 14, "BACK", { centerX, setY, btnW, btnH }, this);
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