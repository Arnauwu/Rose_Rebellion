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
    SDL_Texture* baseTexture = nullptr;
    SDL_Texture* vidaTexture = nullptr;

    // Puntero al enemigo activo actualmente
    Enemy* currentBoss = nullptr;

    // Dimensiones de tus PNGs (Ajusta estos valores al tamaño real de tus imágenes)
    int barWidth = 800;
    int barHeight = 50;
};

