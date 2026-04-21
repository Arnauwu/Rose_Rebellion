#pragma once
#include "Module.h"
#include "SDL3/SDL.h"
#include "Timer.h" 

#define TOTAL_LIFE_FRAMES 8
#define TRANSITION_MS 200

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

    SDL_Texture* lifeBarTexture = nullptr;

    // Dimensiones de un solo frame de la imagen Vides_V1.png
    int sectionWidth = 100;  // Ajusta según el ancho real de tu PNG
    int sectionHeight = 25;  // Ajusta según (Alto total del PNG / 8)

    int targetStateFrame = 0;   // El frame al que queremos llegar
    int currentVisualFrame = 0; // El frame que se dibuja actualmente

    bool isAnimating = false;
    Timer transitionTimer;

    // Configuración ventana TODO: Revisar parametros
    const float BASE_SCREEN_WIDTH = 1280.0f;
    const float UI_SCALE_FACTOR = 4.0f;
    const float UI_MARGIN = 20.0f;
};