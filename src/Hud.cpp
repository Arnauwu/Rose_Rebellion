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

#include "tracy/Tracy.hpp"

Hud::Hud() : Module() {
    name = "hud";
}

Hud::~Hud() {}

bool Hud::Awake() { return true; }

bool Hud::Start() {
    LOG("Loading HUD");
    lifeBarTexture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Princess/SS_Vida_Princesa.png");

    // 1. PON AQUÍ LO QUE TE DIGAN LAS PROPIEDADES DE WINDOWS
    // (Ejemplo: si en Windows pone 1920x1080, cambia los números)
    float imagenAnchoReal = 6144.0f; // <--- ¡CAMBIA ESTO por el ancho real!
    float imagenAltoReal = 5109.0f;  // <--- ¡CAMBIA ESTO por el alto real!

    int cols = 12;
    int rows = 10;

    // 2. Ahora el cálculo será perfecto para el tamaño que de verdad tiene tu archivo
    float exactWidth = imagenAnchoReal / (float)cols;
    float exactHeight = imagenAltoReal / (float)rows;

    int dibujosPorFila[] = { 10, 3, 4, 4, 4, 4, 3, 10, 2, 5 };
    lifeFrames.clear();

    for (int fila = 0; fila < 10; fila++) {
        int numDibujos = dibujosPorFila[fila];

        for (int c = 0; c < numDibujos; c++) {
            int posX = (int)(c * exactWidth);
            int posY = (int)(fila * exactHeight);

            // Quitamos un par de píxeles por si el artista apuró mucho los bordes
            int recorteAncho = (int)exactWidth - 2;
            int recorteAlto = (int)exactHeight;

            SDL_Rect frameRect = { posX, posY, recorteAncho, recorteAlto };
            lifeFrames.push_back(frameRect);
        }
    }

    return true;
}

bool Hud::Update(float dt) {
    ZoneScoped;

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
    ZoneScoped;

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
    //if (lifeBarTexture == nullptr || lifeFrames.empty()) return;

    if (lifeBarTexture == nullptr) {
        LOG("ERROR: La textura de la vida no se ha cargado. Revisa la ruta y el .jpg");
        return;
    }
    if (lifeFrames.empty()) {
        LOG("ERROR: La lista de frames esta vacia.");
        return;
    }

    Player* player = Engine::GetInstance().entityManager->GetPlayer();
    if (player == nullptr) return;

    int hp = player->currentHealth;
    int maxHp = player->maxHealth;
    if (maxHp <= 0) return;

    // 1. Calcular el porcentaje de vida
    float hpPercent = (float)hp / (float)maxHp;
    if (hpPercent < 0.0f) hpPercent = 0.0f;
    if (hpPercent > 1.0f) hpPercent = 1.0f;

    int totalFrames = lifeFrames.size(); // Esto será 51

    // 2. Elegir qué frame toca (invertido: si le queda 100% de vida (1.0), mostrará el frame 0. Si le queda 0%, mostrará el frame 50)
    int frameActual = (int)((1.0f - hpPercent) * (totalFrames - 1));

    // Por seguridad, asegurarnos de que el índice no se salga de la lista
    if (player->isdead || hp <= 0) frameActual = totalFrames - 1; // Último frame de todos
    if (frameActual < 0) frameActual = 0;
    if (frameActual >= totalFrames) frameActual = totalFrames - 1;

    // 3. Obtener el recorte precalculado y dibujarlo
    SDL_Rect srcRect = lifeFrames[frameActual];

    // Márgenes en pantalla (Ajusta esto para mover la barra de vida donde quieras)
    float marginX = 20.0f;
    float marginY = -180.0f;

    // Opcional: Como la imagen es muy grande (512x425), puede que necesites hacerla más pequeña en pantalla.
    // Dibuja la barra de la princesa escalada a la mitad por ejemplo (o ajusta según te convenga en tu motor)
    Engine::GetInstance().render->DrawTexture(lifeBarTexture, marginX, marginY, &srcRect, 0.0f);
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
        int screenW, screenH;
        screenW = Engine::GetInstance().window->windowWidth;

        screenH = Engine::GetInstance().window->windowHeight;

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
        Engine::GetInstance().render->DrawRectangleUnScaled(bgRect, 220, 220, 220, alphaBg, true, false);

        // Draw txto
        SDL_Color color = { 0, 0, 0, alphaText }; // color negro
        SDL_Rect textBounds = { bgRect.x + 10, bgRect.y + 5, bgRect.w - 20, bgRect.h - 10 };

        Engine::GetInstance().render->DrawTextCentered(notificationText.c_str(), textBounds, color, FontType::MENU);
    }
}