#include "IntroCinematicScene.h"
#include "Engine.h"
#include "Log.h"
#include "Cinematics.h"
#include "Input.h"
#include "Audio.h"
#include "Render.h"
#include "SceneManager.h"
#include "Textures.h"

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
    loadingThread = std::thread([]() {
        auto textures = Engine::GetInstance().textures;

        //Menus
        textures->PreloadToRAM("Assets/Textures/UI/GameMenu/t_pauseUI.png");
        textures->PreloadToRAM("Assets/Textures/UI/GameMenu/t_MapUI.png");
        textures->PreloadToRAM("Assets/Textures/UI/GameMenu/t_inventoryUI.png");
        textures->PreloadToRAM("Assets/Textures/UI/SkillUpgrade/t_skillUI.png");

        textures->PreloadToRAM("Assets/Textures/UI/Dialogues/UIDialogueBoxTex.png");
        textures->PreloadToRAM("Assets/Textures/UI/Dialogues/princess_portrait.png");

        // Precargar Entidades Core (Jugador y HUD)
        textures->PreloadToRAM("Assets/Textures/Entities/Princess/Princess_Capeless.png");
        textures->PreloadToRAM("Assets/Textures/Entities/Princess/SS_Vida_Princesa.png");
        });
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
    if (loadingThread.joinable()) {
        loadingThread.join();
    }
    return true;
}