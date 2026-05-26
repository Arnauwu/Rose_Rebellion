#include "LoadingScene.h"
#include "Engine.h"
#include "Render.h"
#include "SceneManager.h"
#include "GameManager.h"
#include "Window.h"
#include "Map.h"

LoadingScene::LoadingScene() : SceneBase(), framesWaited(0), isLoading(false) {}

LoadingScene::~LoadingScene() {}

bool LoadingScene::Awake() {
    return true;
}

bool LoadingScene::Start() {
    framesWaited = 0;
    isLoading = false;
    return true;
}

bool LoadingScene::Update(float dt) {
    int screenW = Engine::GetInstance().window->windowWidth;
    int screenH = Engine::GetInstance().window->windowHeight;
    SDL_Rect fullScreenRect = { 0, 0, screenW, screenH };

    Engine::GetInstance().render->DrawRectangle(fullScreenRect, 0, 0, 0, 255, true, false);
    Engine::GetInstance().render->DrawTextCentered("CARGANDO...", fullScreenRect, { 255, 255, 255, 255 }, FontType::MENU);

    framesWaited++;

    if (framesWaited >= 2 && !isLoading) {
        isLoading = true;

        Engine::GetInstance().sceneManager->ChangeScene(SceneID::GAME);
    }

    return true;
}

bool LoadingScene::CleanUp() {
    return true;
}