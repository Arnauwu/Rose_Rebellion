#include "IntroCinematicScene.h"
#include "Engine.h"
#include "Log.h"
#include "Cinematics.h"
#include "Input.h"
#include "Audio.h"
#include "Render.h"
#include "SceneManager.h"

IntroCinematicScene::IntroCinematicScene() : SceneBase(), isFadingOut(false) {
}

IntroCinematicScene::~IntroCinematicScene() {
}

bool IntroCinematicScene::Awake() {
    LOG("Intro Cinematic Scene Awake");
    return true;
}

bool IntroCinematicScene::Start() {
    LOG("Playing intro cinematic...");
    isFadingOut = false;

    Engine::GetInstance().audio->PlayMusic(nullptr);

    if (!Engine::GetInstance().cinematics->PlayVideo("Assets/Cinematics/intro.mp4")) {
        LOG("Failed to play intro cinematic. Skipping to Game.");
        Engine::GetInstance().sceneManager->ChangeScene(SceneID::GAME);
    }
    return true;
}

bool IntroCinematicScene::Update(float dt)
{
    auto input = Engine::GetInstance().input;
    if (input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN) {
        Engine::GetInstance().cinematics->RequestSkip();
    }

    if (!Engine::GetInstance().cinematics->IsPlaying() && !isFadingOut) {
        isFadingOut = true;
        Engine::GetInstance().sceneManager->ChangeScene(SceneID::GAME);
    }

    return true;
}

bool IntroCinematicScene::CleanUp() {
    LOG("Freeing Intro Cinematic Scene");
    Engine::GetInstance().cinematics->StopVideo();
    return true;
}