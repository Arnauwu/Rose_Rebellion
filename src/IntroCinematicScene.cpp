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

    // Detenemos la música si había alguna para dejar sonar el vídeo
    Engine::GetInstance().audio->PlayMusic(nullptr);

    // Usamos el módulo de FFmpeg que ya tienes programado
    if (!Engine::GetInstance().cinematics->PlayVideo("Assets/Cinematica/intro.mp4")) {
        LOG("Failed to play intro cinematic. Skipping to Game.");
        Engine::GetInstance().sceneManager->ChangeScene(SceneID::GAME);
    }
    return true;
}

bool IntroCinematicScene::Update(float dt) {
    if (isFadingOut) return true;

    // Lógica para saltar la cinemática
    bool skipRequested = Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN ||
        Engine::GetInstance().input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN;

    // Comprobamos si el vídeo ha terminado o el jugador lo ha saltado
    if (skipRequested || !Engine::GetInstance().cinematics->IsPlaying()) {
        isFadingOut = true;

        // IDEALMENTE: Aquí iniciarías un Fade Out a través del Render.
        // Como ChangeScene ahora es instantáneo, detenemos el vídeo y cambiamos.
        Engine::GetInstance().cinematics->StopVideo();
        Engine::GetInstance().sceneManager->ChangeScene(SceneID::GAME);
    }

    return true;
}

bool IntroCinematicScene::CleanUp() {
    LOG("Freeing Intro Cinematic Scene");
    // Por seguridad, aseguramos que el vídeo se detiene al destruir la escena
    Engine::GetInstance().cinematics->StopVideo();
    return true;
}