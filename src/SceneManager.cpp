#include "SceneManager.h"
#include "Engine.h"
#include "Log.h"
#include "Input.h"
#include "Audio.h"
#include "Render.h"
#include "Player.h"
#include "Vector2D.h"

//Scenes
#include "GameScene.h"
#include "IntroScene.h"
#include "MenuScene.h"
#include "IntroCinematicScene.h"
#include "GameOverScene.h"

SceneManager::SceneManager() : Module() {
    name = "scene_manager";
}

SceneManager::~SceneManager() {}

bool SceneManager::Awake() {
    LOG("Initializing Scene Manager");
    return true;
}

bool SceneManager::Start() {
    nextSceneID = SceneID::GAME;
    PerformSceneChange();
    return true;
}

bool SceneManager::PreUpdate() {
    if (currentScene != nullptr) {
        return currentScene->PreUpdate();
    }
    return true;
}

bool SceneManager::Update(float dt) {

    if (isFadingOut && Engine::GetInstance().render->IsFadeComplete()) {

        PerformSceneChange();
        isFadingOut = false;

        Engine::GetInstance().render->StartFade(FadeDirection::FADE_IN, currentFadeTime);
    }

    // 2. Lógica de la escena actual (sigue ejecutándose mientras se hace el fade)
    if (currentScene != nullptr) {
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

void SceneManager::ChangeScene(SceneID newScene, float fadeTime) {
    if (isFadingOut || nextSceneID == newScene) {
        return; 
    }

    LOG("Scene change requested to ID: %d", (int)newScene);
    nextSceneID = newScene;
    isFadingOut = true;
    currentFadeTime = fadeTime;

    Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, fadeTime);
}

void SceneManager::PerformSceneChange() {
    LOG("Performing actual scene swap in memory");

    isGamePaused = false;
    Engine::GetInstance().input->ClearMouseInput(); // Súper útil para no arrastrar clics

    if (currentScene != nullptr) {
        currentScene->CleanUp();
        delete currentScene;
        currentScene = nullptr;
    }

    currentSceneID = nextSceneID;

    switch (currentSceneID)
    {
    case SceneID::INTRO:
        currentScene = new IntroScene();
        break;
    case SceneID::MENU:
        currentScene = new MenuScene();
        break;
    case SceneID::INTRO_CINEMATIC:
        currentScene = new IntroCinematicScene();
        break;
    case SceneID::GAME:
        currentScene = new GameScene();
        break;
    case SceneID::GAMEOVER:
        currentScene = new GameOverScene();
        break;
    case SceneID::WIN:
        //currentScene = new WinScene();
        break;
    default:
        break;
    }

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
