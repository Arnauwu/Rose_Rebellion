#include "Hud.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "SceneManager.h"
#include "GameScene.h"
#include "Player.h"
#include "Window.h"
#include "Log.h"
#include <string>

Hud::Hud() : Module() {
    name = "hud";
}

Hud::~Hud() {}

bool Hud::Awake() { return true; }

bool Hud::Start() {
    LOG("Loading HUD");
    lifeBarTexture = Engine::GetInstance().textures->Load("Assets/Textures/Princess/Vides_V1.png");


    sectionWidth = 211;
    sectionHeight = 108;
    return true;
}

bool Hud::Update(float dt) {
    if (Engine::GetInstance().sceneManager->GetPlayer() == nullptr) return true;

    Player* player = Engine::GetInstance().sceneManager->GetPlayer();

    if (player->maxHealth > 0) {
        int hp = player->currentHealth;

        // dead
        if (player->isdead || hp <= 0) {
            currentVisualFrame = 9;
        }
        // max life
        else if (hp >= 100) {
            currentVisualFrame = 0;
        }

        else {
            float hpPercent = (float)hp / 100.0f;

            // calculate frame with current life
            currentVisualFrame = 9 - (int)(hpPercent * 9.0f);

            if (currentVisualFrame <= 0) currentVisualFrame = 1;
            if (currentVisualFrame >= 9) currentVisualFrame = 8;
        }
    }
    return true;
}

bool Hud::PostUpdate() {

    auto sceneManager = Engine::GetInstance().sceneManager;

    // Only draw life bar when player appear
    if (sceneManager->GetPlayer() == nullptr) {
        return true;
    }

    if (sceneManager->IsGamePaused()) {
        return true;
    }
    
    DrawPlayerHealthBar();
    DrawMineralIndicator();
    DrawDiamondCounter();

    return true;
}

void Hud::DrawPlayerHealthBar() {
    if (lifeBarTexture == nullptr) return;

    Player* player = Engine::GetInstance().sceneManager->GetPlayer();

    // calculate current life
    if (player != nullptr) {
        int hp = player->currentHealth;

        if (player->isdead || hp <= 0) {
            currentVisualFrame = 9; // dead
        }
        else if (hp >= 100) {
            currentVisualFrame = 0; // max life
        }
        else {
            float hpPercent = (float)hp / 100.0f;
            currentVisualFrame = 9 - (int)(hpPercent * 9.0f);

            if (currentVisualFrame <= 0) currentVisualFrame = 1;
            if (currentVisualFrame >= 9) currentVisualFrame = 8;
        }
    }
    else {
        currentVisualFrame = 9;
    }

    // always draw
    SDL_Rect srcRect = { 0, currentVisualFrame * sectionHeight, sectionWidth, sectionHeight };
    float margin = 20.0f;

    Engine::GetInstance().render->DrawTexture(lifeBarTexture, margin, margin, &srcRect, 0.0f);
}

void Hud::DrawDiamondCounter() {

}

void Hud::DrawMineralIndicator() {

}


bool Hud::CleanUp() {
    // Engine::GetInstance().textures->UnLoad(lifeBarTexture);
    if (lifeBarTexture != nullptr) {
        Engine::GetInstance().textures->UnLoad(lifeBarTexture);
        lifeBarTexture = nullptr;
    }
    return true;
}