#include "SceneManager.h"
#include "Engine.h"
#include "Log.h"
#include "Input.h"
#include "Audio.h"

#include "Player.h"
#include "Vector2D.h"
#include "GameScene.h"
#include "IntroScene.h"
 #include "MenuScene.h"

SceneManager::SceneManager() : Module() {
    name = "scene_manager";
}

SceneManager::~SceneManager() {}

bool SceneManager::Awake() {
    LOG("Initializing Scene Manager");
    return true;
}

bool SceneManager::Start() {
    ChangeScene(SceneID::INTRO);
    return true;
}

bool SceneManager::PreUpdate() {
  if (currentScene != nullptr) {
        return currentScene->PreUpdate();
    }
    return true;
}

bool SceneManager::Update(float dt) {
    if (currentScene != nullptr) {
        if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_0) == KEY_DOWN)
        {
            isGamePaused = !isGamePaused;
        }
        return currentScene->Update(dt);
    }
    return true;
}

bool SceneManager::PostUpdate() {

    bool ret = true;
    if (currentScene != nullptr) {
        ret = currentScene->PostUpdate();

        if (currentScene->exitGame) {
            return false;
        }
    }

    if (currentSceneID == SceneID::MENU && Engine::GetInstance().input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN) {
        return false;
    }

    return ret;
}

bool SceneManager::CleanUp() {
    LOG("Freeing Scene Manager");

    if (currentScene != nullptr) {
        currentScene->CleanUp();
        delete currentScene;
        currentScene = nullptr;
    }

    return true;
}

void SceneManager::ChangeScene(SceneID newScene) {
    LOG("Changing Scene");

    isGamePaused = false;
    Engine::GetInstance().input->ClearMouseInput();

    if (currentScene != nullptr) {
        currentScene->CleanUp();
        delete currentScene;
        currentScene = nullptr;
    }

    currentSceneID = newScene;

    switch (newScene)
    {
    case SceneID::INTRO:
         currentScene = new IntroScene();
        break;
    case SceneID::MENU:
         currentScene = new MenuScene();
        break;
    case SceneID::GAME:
         currentScene = new GameScene();
        break;
    case SceneID::GAMEOVER:
        // currentScene = new GameOverScene();
        break;
    case SceneID::WIN:
        // currentScene = new WinScene();
        break;
    }

    // Initialize the scene
    if (currentScene != nullptr) {
        currentScene->Awake();
        currentScene->Start();
    }
}

bool SceneManager::OnUIMouseClickEvent(UIElement* uiElement) {
    if (currentScene != nullptr) {
        return currentScene->OnUIMouseClickEvent(uiElement);
    }
    return true;
}

Vector2D SceneManager::GetPlayerPosition()
{
    if (currentScene != nullptr) {
        return currentScene->GetPlayerPosition();
    }
    return Vector2D(0, 0);
}

Player* SceneManager::GetPlayer()
{
    if (currentScene != nullptr) {
        return currentScene->GetPlayer();
    }
    return nullptr;
}

void SceneManager::SetPlayer(Player* p)
{
    if (currentScene != nullptr) {
        currentScene->SetPlayer(p);
    }
}