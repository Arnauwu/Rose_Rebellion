#pragma once
#include "Module.h"
#include "SDL3/SDL.h"
#include "Timer.h" 
#include <vector>
#include "Animation.h"
#define TOTAL_LIFE_FRAMES 8
#define TRANSITION_MS 200

class Player;

enum class TutorialType {
    NONE,
    WALK,
    JUMP,
    GLIDE,
    DASH,
    ATTACK,
    DOUBLE_JUMP,
    WALL_JUMP
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
    bool isHidden = false;

    void TriggerBossIntro(SDL_Texture* portraitTex, AnimationSet* anim, float duration);
    void DrawBossIntro(float dt);
private:
    void DrawPlayerHealthBar();
    void DrawDiamondCounter();
    void DrawMineralIndicator();
    void DrawNotification();

private:

    SDL_Texture* lifeBarTexture = nullptr;
    std::vector<SDL_Rect> lifeFrames;

    SDL_Texture* notificationBgTexture = nullptr;
    AnimationSet notificationBgAnimation;

    // Dimensiones de un solo frame de la imagen Vides_V1.png
    int sectionWidth = 100;  // Ajusta segun el ancho real de tu PNG
    int sectionHeight = 25;  // Ajusta segun (Alto total del PNG / 8)

    //int targetStateFrame = 0;   // El frame al que queremos llegar
    //int currentVisualFrame = 0; // El frame que se dibuja actualmente

    //bool isAnimating = false;
    //Timer transitionTimer;

    float displayedFrame = 0.0f; // El frame que se est?dibujando (en decimales)
    float animSpeed = 20.0f;

    bool isLastBreathAnimating = false;
    float lastBreathPauseTimer = 0.0f;

    // Configuracion ventana TODO: Revisar parametros
    const float BASE_SCREEN_WIDTH = 1280.0f;
    const float UI_SCALE_FACTOR = 4.0f;
    const float UI_MARGIN = 20.0f;

    //  Notificacion
    std::string notificationText;
    float notificationTimer = 0.0f;
    const float NOTIFICATION_DURATION = 3.0f;

    //Variables de Boss Intro
    bool isBossIntroActive = false;
    float bossIntroTimer = 0.0f;
    float bossIntroTotalDuration = 0.0f;
    SDL_Texture* activeBossPortraitTex = nullptr;
    AnimationSet* activeBossPortraitAnim = nullptr;
};
