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
{}

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
	case SceneID::GAMEOVER:
		UpdateGameOver(dt);
		break;
	default:
		break;
	}

	// Make the camera movement independent of framerate
	float camSpeed = 1;

	if(Engine::GetInstance().input->GetKey(SDL_SCANCODE_UP) == KEY_REPEAT)
		Engine::GetInstance().render->camera.y -= (int)ceil(camSpeed * dt);

	if(Engine::GetInstance().input->GetKey(SDL_SCANCODE_DOWN) == KEY_REPEAT)
		Engine::GetInstance().render->camera.y += (int)ceil(camSpeed * dt);

	if(Engine::GetInstance().input->GetKey(SDL_SCANCODE_LEFT) == KEY_REPEAT)
		Engine::GetInstance().render->camera.x -= (int)ceil(camSpeed * dt);
	
	if(Engine::GetInstance().input.get()->GetKey(SDL_SCANCODE_RIGHT) == KEY_REPEAT)
		Engine::GetInstance().render.get()->camera.x += (int)ceil(camSpeed * dt);

	return true;
}

// Called each loop iteration
bool Scene::PostUpdate()
{
	bool ret = true;

	switch (currentScene)
	{
	case SceneID::MENU:

		break;
	case SceneID::GAME:
		PostUpdateCastle();
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

Vector2D Scene::GetPlayerPosition()
{
	return player->GetPosition();
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
		LoadCastle();

	case SceneID::GAMEOVER:
		LoadGameOver();
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
	case SceneID::GAMEOVER:
		UnloadGameOver();
		break;
	}

}

// *********************************************
// General Helpers
// *********************************************



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
//					MENU?
// *********************************************

void Scene::LoadMainMenu()
{
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

	// ID 4. CREDITS
	auto btnCredits = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 4, "CREDITS", { mainX, currentY, btnW, btnH }, this);
	mainMenuElements.push_back(btnCredits);
	currentY += btnH + spacing;

	// ID 5. EXIT
	auto btnExit = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 5, "EXIT", { mainX, currentY, btnW, btnH }, this);
	mainMenuElements.push_back(btnExit);

	// --- ELEMENTOS DEL MENÚ SETTINGS ---

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
	int checkSize = 30;
	auto chkFull = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::CHECKBOX, 8, "Fullscreen", { sliderX + 40, currentY, checkSize, checkSize }, this);
	settingsMenuElements.push_back(chkFull);
	currentY += checkSize + spacing + 10;

	// ID 9: BACK BUTTON
	auto btnBack = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 9, "BACK", { mainX, currentY + 20, btnW, btnH }, this);
	settingsMenuElements.push_back(btnBack);

	// --- ELEMENTOS DE CREDITOS ---

	int creditsBackY = (int)(windowH * 0.85f);

	// ID 10: BACK BUTTON
	auto btnBackCredits = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 10, "BACK", { mainX, creditsBackY, btnW, btnH }, this);
	creditsMenuElements.push_back(btnBackCredits);

	ShowSettings(false);
	ShowCredits(false);
	ShowPauseMenu(false);
	ShowPauseSettings(false);
}

void Scene::LoadPauseUI() {
	pauseMenuElements.clear();
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
	pauseMenuElements.push_back(btnResume);
	centerY += btnH + spacing;

	// ID 12: SETTINGS (En juego)
	auto btnSettings = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 12, "SETTINGS", { centerX, centerY, btnW, btnH }, this);
	pauseMenuElements.push_back(btnSettings);
	centerY += btnH + spacing;

	// ID 13: EXIT TO MAIN MENU
	auto btnExit = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 13, "MAIN MENU", { centerX, centerY, btnW, btnH }, this);
	pauseMenuElements.push_back(btnExit);

	// --- SETTINGS - EN PAUSE MENU -- LO MISMO

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

	ShowPauseMenu(false);
	ShowPauseSettings(false);
}

void Scene::ShowPauseMenu(bool show) {
	if (show) menuState = MenuState::PAUSE;
	else menuState = MenuState::NONE;

	for (auto& elem : pauseMenuElements) {
		elem->visible = show;

		if (show == true) {
			elem->state = UIElementState::NORMAL;
		}
		else {
			elem->state = UIElementState::DISABLED;
		}
	}

	ShowPauseSettings(false);
}

void Scene::ShowPauseSettings(bool show) {
	if (show == true) {
		menuState = MenuState::PAUSE_SETTINGS;
	}

	for (auto& elem : pauseMenuElements) {
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

void Scene::UpdateMainMenu(float dt) {
	if (menuBackground != nullptr) {
		Engine::GetInstance().render->DrawTexture(menuBackground, 0, 0);
	}

	if (menuState == MenuState::CREDITS) {

		int w, h;
		Engine::GetInstance().window->GetWindowSize(w, h);

		int centerX = w / 2;
		int currentY = (h / 2) - 100;
		int spacing = 35;

		//COLORES
		SDL_Color titleColor = { 255, 255, 0, 255 };
		SDL_Color headerColor = { 200, 200, 255, 255 };
		SDL_Color textColor = { 255, 255, 255, 255 };
		SDL_Color grayColor = { 180, 180, 180, 255 };

		// TEXTO INFORMACIÓN
		Engine::GetInstance().render->DrawText("CREDITOS", centerX - 75, currentY, 150, 40, titleColor);
		currentY += 50;

		Engine::GetInstance().render->DrawText("--- PROGRAMACION ---", centerX - 100, currentY, 200, 25, headerColor);
		currentY += spacing;

		Engine::GetInstance().render->DrawText("Marc Jimenez", centerX - 60, currentY, 120, 25, textColor);
		currentY += spacing;

		Engine::GetInstance().render->DrawText("Ruben Mateo", centerX - 60, currentY, 120, 25, textColor);
		currentY += 50;

		Engine::GetInstance().render->DrawText("--- ARTE Y ASSETS ---", centerX - 100, currentY, 200, 25, headerColor);
		currentY += spacing;

		Engine::GetInstance().render->DrawText("Tile Set & Enemigos:", centerX - 80, currentY, 160, 25, textColor);
		currentY += 25;
		Engine::GetInstance().render->DrawText("Szadi art.", centerX - 40, currentY, 80, 20, grayColor);
		currentY += spacing;

		Engine::GetInstance().render->DrawText("Personaje Principal:", centerX - 80, currentY, 160, 25, textColor);
		currentY += 25;
		Engine::GetInstance().render->DrawText("Penzilla", centerX - 30, currentY, 60, 20, grayColor);
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
	creditsMenuElements.clear();
	pauseMenuElements.clear();
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

	case 4: // CREDITS
		LOG("Ir a Creditos");
		ShowCredits(true);
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

	case 10: // BACK de Créditos
		ShowCredits(false);
		break;

	default:
		break;
	}
}

void Scene::HandlePauseMenuUIEvents(UIElement* uiElement)
{
	switch (uiElement->id)
	{
	case 11: // RESUME
		ShowPauseMenu(false);
		break;

	case 12: // SETTINGS (Dentro de la pausa)
		ShowPauseSettings(true);
		break;

	case 13: // EXIT TO MAIN MENU
		//isGamePaused = false;
		Engine::GetInstance().render->camera.x = 0;
		Engine::GetInstance().render->camera.y = 0;

		ChangeScene(SceneID::MENU);
		break;

	case 14: // BACK PAUSE
		ShowPauseSettings(false);
		ShowPauseMenu(true);
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

void Scene::ShowCredits(bool show) {

	if (show) {
		menuState = MenuState::CREDITS;
	}
	else {
		menuState = MenuState::MAIN;
	}

	for (auto& elem : mainMenuElements) {
		if (show == true) {

			elem->visible = false;
			elem->state = UIElementState::DISABLED;
		}
		else {

			elem->visible = true;
			elem->state = UIElementState::NORMAL;

		}
	}

	// Settings ocultos
	for (auto& elem : settingsMenuElements) {
		elem->visible = false;
		elem->state = UIElementState::DISABLED;
	}
	// Gestion de los creditos
	for (auto& elem : creditsMenuElements) {
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


void Scene::LoadCastle()
{
	lastLevelPlayed = SceneID::GAME;
	LoadMap("MapTemplate.tmx");
	if (player != nullptr) {
		player->position.setX(10);
		player->position.setY(10);
	}
}

void Scene::UpdateCastle(float dt)
{
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_N) == KEY_DOWN) {
	}
}

void Scene::PostUpdateCastle()
{

}

void Scene::UnloadCastle() {

	// Clean up map and entities
	Engine::GetInstance().entityManager->CleanUp();
	Engine::GetInstance().map->CleanUp();
}

void Scene::LoadLevel2()
{

}

void Scene::UpdateLevel2(float dt)
{

}

void Scene::PostUpdateLevel2()
{

}

void Scene::UnloadLevel2()
{

}

void Scene::LoadLevelTransition() {
	transitionTimer = 2000.0f;
}

void Scene::UpdateLevelTransition(float dt) {
	transitionTimer -= dt;
	Engine::GetInstance().render->DrawText("NIVEL COMPLETADO - CARGANDO...", 100, 300, 600, 50, { 0,255,0,255 });

}

void Scene::LoadGameOver() {
	gameOverTimer = 2000.0f;
}

void Scene::UpdateGameOver(float dt) {
	gameOverTimer -= dt;
	Engine::GetInstance().render->DrawText("GAME OVER - REINTENTANDO", 200, 300, 400, 50, { 255,0,0,255 });

	if (gameOverTimer <= 0.0f) ChangeScene(lastLevelPlayed);
}

void Scene::UnloadGameOver()
{
	Engine::GetInstance().uiManager->CleanUp();
}

void Scene::LoadWin()
{
	//outroTexture = Engine::GetInstance().textures->Load("Assets/Textures/Intro_Menus/Outro.png");
	Engine::GetInstance().audio->PlayMusic("Assets/Audio/Music/Ambient_Music_Cave.wav");

	Engine::GetInstance().render->camera.x = 0;
	Engine::GetInstance().render->camera.y = 0;
	Engine::GetInstance().render->SetZoom(1.0f);

	int w, h;
	Engine::GetInstance().window->GetWindowSize(w, h);

	float currentZoom = 1.0f;
	int btnW = 150;
	int btnH = 40;

	int centerX = (int)((w / 2) / currentZoom) - (btnW / 2);
	int centerY = (int)((h - 150) / currentZoom);

	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 21, "MENU", { centerX, centerY, btnW, btnH }, this);
}

void Scene::UpdateWin(float dt)
{
	/*if (outroTexture != nullptr)
	{
		Engine::GetInstance().render->DrawTexture(outroTexture, 0, 0);
	}*/
}

void Scene::UnloadWin()
{
	Engine::GetInstance().uiManager->CleanUp();
	/*if (outroTexture) {
		Engine::GetInstance().textures->UnLoad(outroTexture);
		outroTexture = nullptr;
	}*/
}