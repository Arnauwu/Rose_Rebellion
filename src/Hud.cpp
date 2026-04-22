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
#include "Physics.h"

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

    if (notificationTimer > 0.0f) {
        notificationTimer -= dt / 1000.0f; // 将毫秒转换为秒
        if (notificationTimer < 0.0f) {
            notificationTimer = 0.0f;
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
    DrawNotification();

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

void Hud::ShowNotification(const std::string& message) {
    notificationText = message;
    notificationTimer = NOTIFICATION_DURATION;
}

void Hud::DrawNotification() {
    if (notificationTimer > 0.0f && !notificationText.empty()) {
        Uint8 alphaText = 255;
        Uint8 alphaBg = 160; // 背景框的初始透明度（0-255，越小越透明）

        // 渐变消失逻辑：在最后 1 秒内逐渐变透明
        if (notificationTimer < 1.0f) {
            alphaText = (Uint8)(255.0f * notificationTimer);
            alphaBg = (Uint8)(160.0f * notificationTimer);
        }

        Player* player = Engine::GetInstance().sceneManager->GetPlayer();
        if (player == nullptr || player->pbody == nullptr) return;

        // 2. 获取玩家的世界坐标 (中心点)
        int px, py;
        player->pbody->GetPosition(px,py);

        // 3. 结合摄像机偏移量，将世界坐标转换为屏幕显示的坐标
        int camX = Engine::GetInstance().render->camera.x;
        int camY = Engine::GetInstance().render->camera.y;

        int screenX = px + camX;
        int screenY = py + camY;

        // 4. 定义提示框的大小 (因为跟随人物，可以稍微调小一点)
        int rectW = 600;
        int rectH = 70;

        // 5. 将背景框的中心点对齐玩家，并往上移动 (例如人物上方 120 像素)
        // 如果你想让它更靠下，把 - 120 改成 - 80 或更小
        SDL_Rect bgRect = {
            screenX - rectW / 2,
            screenY -800 ,
            rectW,
            rectH
        };

        // 6. 绘制背景框
        Engine::GetInstance().render->DrawRectangle(bgRect, 220, 220, 220, alphaBg, true, false);

        // 7. 绘制文字
        SDL_Color color = { 0, 0, 0, alphaText }; // 黑色文字
        SDL_Rect textBounds = { bgRect.x + 10, bgRect.y + 5, bgRect.w - 20, bgRect.h - 10 };

        Engine::GetInstance().render->DrawTextCentered(notificationText.c_str(), textBounds, color);
    }
}