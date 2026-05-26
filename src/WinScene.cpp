#include "WinScene.h"
#include "Engine.h"
#include "Log.h"
#include "Cinematics.h"
#include "Input.h"
#include "Audio.h"
#include "SceneManager.h"

WinScene::WinScene() : SceneBase(), isFadingOut(false) {
}

WinScene::~WinScene() {
}

bool WinScene::Awake() {
    LOG("Win Scene Awake");
    return true;
}

bool WinScene::Start() {
    LOG("Playing outro cinematic...");
    isFadingOut = false;

    // Paramos la música actual
    Engine::GetInstance().audio->PlayMusic(nullptr);

    // Intentamos cargar la cinemática final
    if (!Engine::GetInstance().cinematics->PlayVideo("Assets/Cinematics/outro.mp4")) {
        LOG("Failed to play outro cinematic. Skipping to Menu.");
        Engine::GetInstance().sceneManager->ChangeScene(SceneID::MENU);
    }
    return true;
}

bool WinScene::Update(float dt) {
    auto input = Engine::GetInstance().input;

    // Permitir al jugador saltar la cinemática si presiona ESPACIO
    if (input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN) {
        Engine::GetInstance().cinematics->RequestSkip();
    }

    // Cuando el video termine de reproducirse, volvemos al Menú Principal
    if (!Engine::GetInstance().cinematics->IsPlaying() && !isFadingOut) {
        isFadingOut = true;
        Engine::GetInstance().sceneManager->ChangeScene(SceneID::MENU);
    }

    return true;
}

bool WinScene::CleanUp() {
    LOG("Freeing Win Scene");
    Engine::GetInstance().cinematics->StopVideo();
    return true;
}