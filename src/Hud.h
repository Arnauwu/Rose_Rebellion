#pragma once
#include "Module.h"
#include "SDL3/SDL.h"
#include "Timer.h" 
#include <vector>

#define TOTAL_LIFE_FRAMES 8
#define TRANSITION_MS 200

class Player;

enum class TutorialType {
    NONE,
    WALK,
    JUMP,
    GLIDE,
    DASH,
    ATTACK
};

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

    void ShowNotification(const std::string& message);

    void ShowTutorial(TutorialType type);
private:
    void DrawPlayerHealthBar();
    void DrawDiamondCounter();
    void DrawMineralIndicator();
    void DrawNotification();
    void DrawTutorial();
private:

    SDL_Texture* lifeBarTexture = nullptr;
    std::vector<SDL_Rect> lifeFrames;

    SDL_Texture* tutWalkTex = nullptr;
    SDL_Texture* tutJumpTex = nullptr;
    SDL_Texture* tutGlideTex = nullptr;
    SDL_Texture* tutDashTex = nullptr;
    SDL_Texture* tutAttackTex = nullptr;

    TutorialType currentTutorial = TutorialType::NONE;
    float tutorialTimer = 0.0f;
    const float TUTORIAL_DURATION = 4.0f;

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

    //  Notificacion
    std::string notificationText;
    float notificationTimer = 0.0f;
    const float NOTIFICATION_DURATION = 3.0f;
};