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
	case SceneID::CASTLE:
		UpdateCastle(dt);

		break;
	case SceneID::LEVEL2:
		UpdateLevel2(dt);

		break;
	case SceneID::LEVEL_TRANSITION:
		UpdateLevelTransition(dt);

		break;
	case SceneID::WIN:
		UpdateWin(dt);

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
	case SceneID::CASTLE:
		PostUpdateCastle();

		break;
	case SceneID::LEVEL2:
		PostUpdateLevel2();
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
	case SceneID::CASTLE:
		LoadCastle();

		break;
	case SceneID::LEVEL2:
		LoadLevel2();

		break;
	case SceneID::LEVEL_TRANSITION:
		LoadLevelTransition();
		break;
	case SceneID::WIN:
		LoadWin();
		break;
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

	case SceneID::CASTLE:
		UnloadCastle();
		break;

	case SceneID::LEVEL2:
		UnloadLevel2();
		break;
	case SceneID::LEVEL_TRANSITION:
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
	
	// MENÚ PRINCIPAL
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

	// ID 4. CREDITS
	auto btnCredits = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 4, "CREDITS", { mainX, currentY, btnW, btnH }, this);
	mainMenuElements.push_back(btnCredits);
	currentY += btnH + spacing;

	// ID 5. EXIT
	auto btnExit = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, 5, "EXIT", { mainX, currentY, btnW, btnH }, this);
	mainMenuElements.push_back(btnExit);


}

void Scene::UpdateMainMenu(float dt)
{
	
	Engine::GetInstance().render->DrawRectangle(screenRect, 30, 40,25, 255, true, false); //Menu

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN) {
		ChangeScene(SceneID::CASTLE);
	}
}

void Scene::UnloadMainMenu()
{
	Engine::GetInstance().uiManager->CleanUp();

	if (menuBackground != nullptr) {
		Engine::GetInstance().textures->UnLoad(menuBackground);
		menuBackground = nullptr;
	}
}

void Scene::LoadCastle()
{
	lastLevelPlayed = SceneID::CASTLE;
	LoadMap("MapTemplate.tmx");
	if (player != nullptr) {
		player->position.setX(10);
		player->position.setY(10);
	}
}

void Scene::UpdateCastle(float dt)
{
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_N) == KEY_DOWN) {
		ChangeScene(SceneID::LEVEL_TRANSITION);
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

	if (transitionTimer <= 0.0f) ChangeScene(SceneID::LEVEL2);
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