#pragma once
#include "Module.h"
#include "SDL3/SDL.h"
#include "Timer.h" 

class Player;

class Hud : public Module
{
public:
    Hud();
    virtual ~Hud();

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    bool PostUpdate() override;
    bool CleanUp() override;

private:
    void DrawPlayerHealthBar();
    void DrawDiamondCounter();
    void DrawMineralIndicator();
private:

    // Configuración ventana TODO: Revisar parametros
    const float BASE_SCREEN_WIDTH = 1280.0f;
    const float UI_SCALE_FACTOR = 4.0f;
    const float UI_MARGIN = 20.0f;
};