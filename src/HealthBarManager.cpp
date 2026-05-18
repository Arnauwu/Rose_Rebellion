#include "HealthBarManager.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Window.h"
#include "Enemy.h" // Requerido para leer la vida del Boss
#include "Log.h"

#include "tracy/Tracy.hpp"

HealthBarManager::HealthBarManager() : Module() {
    name = "healthbarmanager";
}

HealthBarManager::~HealthBarManager() {}

bool HealthBarManager::Awake() { return true; }

bool HealthBarManager::Start() {
    LOG("Loading Health Bar Manager");

    // Carga de texturas. Asegúrate de que las rutas coincidan con tu proyecto
    baseTexture = Engine::GetInstance().textures->Load("Assets/Textures/UI/LifeBar/Base.png");
    vidaTexture = Engine::GetInstance().textures->Load("Assets/Textures/UI/LifeBar/Vida.png");

    // Ajusta esto al ancho y alto real de tus texturas
    barWidth = 1448;
    barHeight = 69;

    return true;
}

bool HealthBarManager::Update(float dt) {
    return true; // La UI se suele renderizar en PostUpdate
}

bool HealthBarManager::PostUpdate() {
    ZoneScoped;

    if (currentBoss == nullptr || currentBoss->isdead) {
        return true;
    }

    int screenW = Engine::GetInstance().window->windowWidth;
    int screenH = Engine::GetInstance().window->windowHeight;

    // --- 1. POSICIÓN: Arriba y centrado ---
    // Solo declaramos posX y posY una vez aquí
    int posX = ((screenW - barWidth) / 2)+300;
    int posY = 120;

    // --- 2. LÓGICA DE VIDA ---
    float hpPercent = 0.0f;
    if (currentBoss->maxHealth > 0) {
        hpPercent = (float)currentBoss->currentHealth / (float)currentBoss->maxHealth;

        if (hpPercent < 0.0f) hpPercent = 0.0f;
        if (hpPercent > 1.0f) hpPercent = 1.0f;
    }

    // --- 3. DIBUJAR BASE (Fondo estático, capa inferior) ---
    SDL_Rect srcRectBase = { 0, 0, barWidth, barHeight };
    Engine::GetInstance().render->DrawTexture(baseTexture, posX, posY, &srcRectBase, 0.0f);

    // --- 4. DIBUJAR VIDA (Barra que se encoge, capa superior) ---
    int margenIzquierdo = 35; // Píxeles desde el inicio a la izq. hasta que empieza lo oscuro
    int margenDerecho = 35;   // Píxeles transparentes a la derecha (el pincho)

    // Calculamos cuánto mide solo la parte que se rellena
    int anchoColorReal = barWidth - margenIzquierdo - margenDerecho;

    // Le decimos que recorte el margen inicial + el porcentaje de vida
    int currentVidaWidth = margenIzquierdo + (int)(anchoColorReal * hpPercent);

    // Solo dibujamos si el boss tiene algo de vida (para que no dibuje el margen vacío cuando esté a 0)
    if (hpPercent > 0.0f) {
        SDL_Rect srcRectVida = { 0, 0, currentVidaWidth, barHeight };
        Engine::GetInstance().render->DrawTexture(vidaTexture, posX, posY, &srcRectVida, 0.0f);
    }

    return true;
}

bool HealthBarManager::CleanUp() {
    if (baseTexture != nullptr) {
        Engine::GetInstance().textures->UnLoad(baseTexture);
        baseTexture = nullptr;
    }
    if (vidaTexture != nullptr) {
        Engine::GetInstance().textures->UnLoad(vidaTexture);
        vidaTexture = nullptr;
    }
    return true;
}

void HealthBarManager::SetBoss(Enemy* boss) {
    currentBoss = boss;
}