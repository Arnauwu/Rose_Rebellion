#pragma once
#include "Module.h"
#include <SDL3/SDL.h>

// Declaración adelantada para no incluir Enemy.h aquí y evitar dependencias circulares
class Enemy;

class HealthBarManager : public Module
{
public:
    HealthBarManager();
    virtual ~HealthBarManager();

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    bool PostUpdate() override;
    bool CleanUp() override;

    // Llama a esta función cuando necesites mostrar la barra de un enemigo/jefe
    void SetBoss(Enemy* boss);

private:
    SDL_Texture* knightBaseTexture = nullptr;
    SDL_Texture* knightVidaTexture = nullptr;

    // Texturas del Dragón
    SDL_Texture* dragonBaseTexture = nullptr;
    SDL_Texture* dragonVidaTexture = nullptr;
    SDL_Texture* dragonExtraTexture = nullptr;

    // Texturas de la Ninfa
    SDL_Texture* ninfaBaseTexture = nullptr;
    SDL_Texture* ninfaVidaTexture = nullptr;

    // Puntero al enemigo activo actualmente
    Enemy* currentBoss = nullptr;

    // Dimensiones de tus PNGs (Ajusta estos valores al tamaño real de tus imágenes)
    int barWidth = 800;
    int barHeight = 50;
};

