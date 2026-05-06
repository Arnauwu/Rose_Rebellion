#include "Hud.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "EntityManager.h"
#include "SceneManager.h"
#include "GameScene.h"
#include "Player.h"
#include "Window.h"
#include "Log.h"
#include <string>
#include "Physics.h"

Hud::Hud() : Module() {
    name = "hud";
}

Hud::~Hud() {}

bool Hud::Awake() { return true; }

bool Hud::Start() {
    LOG("Loading HUD");
    lifeBarTexture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Princess/Vides_V1.png");

    sectionWidth = 211;
    sectionHeight = 108;
    return true;
}

bool Hud::Update(float dt) {
    Player* player = Engine::GetInstance().entityManager->GetPlayer();
    if (player == nullptr) return true;

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

    if (notificationTimer > 0.0f) {
        notificationTimer -= dt / 1000.0f; 
        if (notificationTimer < 0.0f) {
            notificationTimer = 0.0f;
        }
    }

    return true;
}

bool Hud::PostUpdate() {

    auto sceneManager = Engine::GetInstance().sceneManager;
    Player* player = Engine::GetInstance().entityManager->GetPlayer();

    if (player == nullptr) {
        return true;
    }

    if (sceneManager->IsGamePaused()) {
        return true;
    }
    
    DrawPlayerHealthBar();
    DrawMineralIndicator();
    DrawDiamondCounter();
    DrawNotification();

    return true;
}

void Hud::DrawPlayerHealthBar() {
    if (lifeBarTexture == nullptr) return;

    Player* player = Engine::GetInstance().entityManager->GetPlayer();

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

void Hud::ShowNotification(const std::string& message) {
    notificationText = message;
    notificationTimer = NOTIFICATION_DURATION;
}

void Hud::DrawNotification() {
    if (notificationTimer > 0.0f && !notificationText.empty()) {
        Uint8 alphaText = 255;
        Uint8 alphaBg = 160;

        // Desapareciendo lentamente
        if (notificationTimer < 1.0f) {
            alphaText = (Uint8)(255.0f * notificationTimer);
            alphaBg = (Uint8)(160.0f * notificationTimer);
        }

        // Tamaño de pantalla
        int screenW = Engine::GetInstance().render->camera.w;
        int screenH = Engine::GetInstance().render->camera.h;

        // Tamaño del cuadro de solicitud
        int rectW = 600;
        int rectH = 70;

        // Ubicación del aviso
        int posY = screenH / 8;

        SDL_Rect bgRect = {
            screenW / 2 - rectW / 2,
            posY+60,
            rectW,
            rectH
        };

        // Draw
        Engine::GetInstance().render->DrawRectangle(bgRect, 220, 220, 220, alphaBg, true, false);

        // Draw txto
        SDL_Color color = { 0, 0, 0, alphaText }; // color negro
        SDL_Rect textBounds = { bgRect.x + 10, bgRect.y + 5, bgRect.w - 20, bgRect.h - 10 };

        Engine::GetInstance().render->DrawTextCentered(notificationText.c_str(), textBounds, color, FontType::MENU);
    }
}