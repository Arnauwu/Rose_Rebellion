#include "GameOverScene.h"
#include "Engine.h"
#include "Render.h"
#include "SceneManager.h"
#include "GameManager.h"

#include "Window.h"
#include "Player.h"

GameOverScene::GameOverScene() : SceneBase() {}

GameOverScene::~GameOverScene() {}

bool GameOverScene::Start() {
    timer = 0.0f;

    GameManager::GetInstance().gameState.collectedItems.clear();
    GameManager::GetInstance().gameState.openedDoors.clear();

    return true;
}

bool GameOverScene::Update(float dt) {
    int screenW = Engine::GetInstance().render->camera.w;
    int screenH = Engine::GetInstance().render->camera.h;
    SDL_Rect fullScreenRect = { 0, 0, screenW, screenH };

    Engine::GetInstance().render->DrawRectangle(fullScreenRect, 0, 0, 0, 255, true, false);

    Engine::GetInstance().render->DrawTextCentered("GAME OVER", fullScreenRect, { 255, 0, 0, 255 });

    timer += dt / 1000.0f;

    if (timer >= displayTime) {
        Engine::GetInstance().sceneManager->ChangeScene(SceneID::MENU);
    }

    return true;
}

bool GameOverScene::CleanUp() {
    return true;
}