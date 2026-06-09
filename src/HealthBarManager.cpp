#include "HealthBarManager.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Window.h"
#include "Enemy.h" 
#include "sceneManager.h" 

#include "Log.h"
#include "tracy/Tracy.hpp"

HealthBarManager::HealthBarManager() : Module() {
    name = "healthbarmanager";
}

HealthBarManager::~HealthBarManager() {}

bool HealthBarManager::Awake() { return true; }

bool HealthBarManager::Start() {
    LOG("Loading Health Bar Manager");

    // 1. Cargar imágenes Caballero
    knightBaseTexture = Engine::GetInstance().textures->Load("Assets/Textures/UI/LifeBar/Base.png");
    knightVidaTexture = Engine::GetInstance().textures->Load("Assets/Textures/UI/LifeBar/Vida.png");

    // 2. Cargar imágenes Dragón
    dragonBaseTexture = Engine::GetInstance().textures->Load("Assets/Textures/UI/LifeBar/Base_dragon.png");
    dragonVidaTexture = Engine::GetInstance().textures->Load("Assets/Textures/UI/LifeBar/Vida_dragon.png");
    dragonExtraTexture = Engine::GetInstance().textures->Load("Assets/Textures/UI/LifeBar/Separadores Vida.png");

    // 3. Cargar imágenes Ninfa
    ninfaBaseTexture = Engine::GetInstance().textures->Load("Assets/Textures/UI/LifeBar/Base_ninfa.png");
    ninfaVidaTexture = Engine::GetInstance().textures->Load("Assets/Textures/UI/LifeBar/Vida_ninfa.png");

    lagartoBaseTexture = Engine::GetInstance().textures->Load("Assets/Textures/UI/LifeBar/Borde2.png");
    lagartoVidaTexture = Engine::GetInstance().textures->Load("Assets/Textures/UI/LifeBar/Vida_lagarto.png");

    return true;
}

bool HealthBarManager::Update(float dt)
{
    ZoneScoped;
    return true;
}

bool HealthBarManager::PostUpdate() {

    if (Engine::GetInstance().sceneManager->IsGamePaused() || currentBoss == nullptr || currentBoss->isdead) {
        return true;
    }
    if (currentBoss == nullptr || currentBoss->isdead) {
        return true;
    }

    // --- CONFIGURACIÓN POR DEFECTO (Caballero) ---
    SDL_Texture* activeBase = knightBaseTexture;
    SDL_Texture* activeVida = knightVidaTexture;
    SDL_Texture* activeExtra = nullptr;
    int activeWidth = 1448;
    int activeHeight = 69;
    int activeMargenIzq = 123;
    int activeMargenDer = 123;
    int extraWidth = 0;
    int extraHeight = 0;
    int extraOffsetX = 0;
    int extraOffsetY = 0;
    int posY = 190;

    // --- CAMBIAR SEGÚN EL BOSS ---
    if (currentBoss->name == "Dragon") {
        activeBase = dragonBaseTexture;
        activeVida = dragonVidaTexture;
        activeExtra = dragonExtraTexture;
        // TODO: Pon aquí las medidas reales de la barra del Dragón
        activeWidth = 1519;
        activeHeight = 98;
        activeMargenIzq = 130;
        activeMargenDer = 130;
        extraWidth = 1519;
        extraHeight = 98;
        extraOffsetX = 0;
        extraOffsetY = 0;

        const int screenH = Engine::GetInstance().window->windowHeight;
        const float zoom = Engine::GetInstance().render->GetZoom();
        const int bottomMargin = 30;
        posY = (int)((screenH - activeHeight * zoom - bottomMargin) / zoom);
    }
    else if (currentBoss->name == "NinfaMare") {
        activeBase = ninfaBaseTexture;
        activeVida = ninfaVidaTexture;
        // TODO: Pon aquí las medidas reales de la barra de la Ninfa
        activeWidth = 1448;
        activeHeight = 98;
        activeMargenIzq = 123;
        activeMargenDer = 123;
    }
    else if (currentBoss->name == "GwellBoss") {
        activeBase = lagartoBaseTexture;
        activeVida = lagartoVidaTexture;
        activeExtra = nullptr;

        // TODO: Pon aquí las medidas de Borde2.png y Vida_lagarto.png
        activeWidth = 1448; // Cambia esto por el ancho real en píxeles de Borde2.png
        activeHeight = 98;  // Cambia esto por el alto real en píxeles de Borde2.png
        activeMargenIzq = 123; // Ajusta cuántos píxeles de margen hay hasta que empieza el rojo
        activeMargenDer = 123; // Ajusta el margen derecho
    }

    // --- 1. POSICIÓN: Arriba y centrado ---
    int screenW = Engine::GetInstance().window->windowWidth;
    int posX = ((screenW - activeWidth) / 2) + 650;

    // --- 2. LÓGICA DE VIDA ---
    float hpPercent = 0.0f;
    if (currentBoss->maxHealth > 0) {
        hpPercent = (float)currentBoss->currentHealth / (float)currentBoss->maxHealth;
        if (hpPercent < 0.0f) hpPercent = 0.0f;
        if (hpPercent > 1.0f) hpPercent = 1.0f;
    }

    // --- 3. DIBUJAR BASE ---
    SDL_Rect srcRectBase = { 0, 0, activeWidth, activeHeight };
    Engine::GetInstance().render->DrawTexture(activeBase, posX, posY, &srcRectBase, 0.0f);

    // --- 4. DIBUJAR VIDA CORREGIDA ---
    int anchoColorReal = activeWidth - activeMargenIzq - activeMargenDer;
    int currentVidaWidth = activeMargenIzq + (int)(anchoColorReal * hpPercent);

    if (hpPercent > 0.0f) {
        SDL_Rect srcRectVida = { 0, 0, currentVidaWidth, activeHeight };
        Engine::GetInstance().render->DrawTexture(activeVida, posX, posY, &srcRectVida, 0.0f);
    }

    if (activeExtra != nullptr) {
        int extraPosX = posX + extraOffsetX;
        int extraPosY = posY + extraOffsetY;
        SDL_Rect srcRectExtra = { 0, 0, extraWidth, extraHeight };
        Engine::GetInstance().render->DrawTexture(activeExtra, extraPosX, extraPosY, &srcRectExtra, 0.0f);
    }

}
bool HealthBarManager::CleanUp() {
    if (knightBaseTexture) Engine::GetInstance().textures->UnLoad(knightBaseTexture);
    if (knightVidaTexture) Engine::GetInstance().textures->UnLoad(knightVidaTexture);
    if (dragonBaseTexture) Engine::GetInstance().textures->UnLoad(dragonBaseTexture);
    if (dragonVidaTexture) Engine::GetInstance().textures->UnLoad(dragonVidaTexture);
    if (dragonExtraTexture) Engine::GetInstance().textures->UnLoad(dragonExtraTexture);
    if (ninfaBaseTexture) Engine::GetInstance().textures->UnLoad(ninfaBaseTexture);
    if (ninfaVidaTexture) Engine::GetInstance().textures->UnLoad(ninfaVidaTexture);
    if (lagartoBaseTexture) Engine::GetInstance().textures->UnLoad(lagartoBaseTexture);
    if (lagartoVidaTexture) Engine::GetInstance().textures->UnLoad(lagartoVidaTexture);
    return true;
}


void HealthBarManager::SetBoss(Enemy* boss) {
    currentBoss = boss;
}