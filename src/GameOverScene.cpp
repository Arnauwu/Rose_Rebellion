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

    // Puedes mantener esto si tu LoadGame sobrescribe completamente la gameState después.
    GameManager::GetInstance().gameState.collectedItems.clear();
    GameManager::GetInstance().gameState.openedDoors.clear();

    return true;
}

bool GameOverScene::Update(float dt) {
    int screenW, screenH;
    screenW = Engine::GetInstance().window->windowWidth;
    screenH = Engine::GetInstance().window->windowHeight;

    SDL_Rect fullScreenRect = { 0, 0, screenW, screenH };

    Engine::GetInstance().render->DrawRectangle(fullScreenRect, 0, 0, 0, 255, true, false);
    Engine::GetInstance().render->DrawTextCentered("GAME OVER", fullScreenRect, { 255, 0, 0, 255 }, FontType::MENU);

    timer += dt / 1000.0f;

    if (timer >= displayTime) {
        GameManager::GetInstance().LoadGame("savegame.xml");

        Engine::GetInstance().sceneManager->ChangeScene(SceneID::GAME);
    }

    return true;
}

bool GameOverScene::CleanUp() {
    return true;
}