#pragma once
#include "Module.h"
#include "Animation.h"
#include "Textures.h"
#include <vector>
#include <SDL3/SDL.h>

struct Particle {
    float x, y;
    float vx, vy;
    float life = 0.0f;
    float maxLife = 0.0f;
    float scale = 1.0f;
    SDL_Color color = { 255, 255, 255, 255 };
    float size = 1.0f;
    bool active = false;
    bool useCamera = true;

    // Textures
    SDL_Texture* texture = nullptr;
    float angle = 0.0f;
    float angularVelocity = 0.0f;

    // Animaciones
    Animation anim;
    bool isAnimated = false;
    SDL_FlipMode flipMode = SDL_FLIP_NONE;
};

class ParticleManager : public Module {
public:
    ParticleManager();
    ~ParticleManager();

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    bool PostUpdate() override;
    bool CleanUp() override;

    void Emit(float x, float y, float vx, float vy, float life, SDL_Color color, float size, bool useCamera = true);
    void Emit(SDL_Texture* texture, float x, float y, float vx, float vy, float life, float size, bool useCamera = true, float angularVelocity = 0.0f);
    void Emit(SDL_Texture* texture, Animation anim, float x, float y, float vx, float vy, float life, float size, bool useCamera = true, float angularVelocity = 0.0f, SDL_FlipMode flipMode = SDL_FLIP_NONE, float scale = 1.0f);

    // Nuevas funciones de emisi¨®n para el SpriteSheet unificado
    void EmitDust(float x, float y, bool lookingRight); // andar
    void EmitJumpDust(float x, float y, bool lookingRight);                // jump
    void EmitDashDust(float x, float y, bool lookingRight); // dash

    // Otros efectos
    void EmitHitSparks(float x, float y, bool isBlood = false);
    void EmitItemPickup(float x, float y);
    void EmitAttack(float x, float y, bool lookingRight);
    void EmitRain(float cameraX, float cameraY, int cameraW, int cameraH);
    void EmitSwordSlash(float x, float y, bool lookingRight);

public:
    // --- SpriteSheet Unificado (Polvo) ---
    SDL_Texture* dustSpriteSheet = nullptr;
    Animation animWalkDust; // "andar"
    Animation animJumpDust; // "jump"
    Animation animDashDust; // "dash"

    // --- Otros Efectos ---
    SDL_Texture* attackSpriteSheet = nullptr;
    Animation animAttack1;
    Animation animAttack2;

    SDL_Texture* hitP = nullptr;
    Animation animHitSpark;

    SDL_Texture* bloodP = nullptr;
    Animation animBloodHit;

    SDL_Texture* pickP = nullptr;
    Animation animPickup;

private:
    std::vector<Particle> pool;
    int poolSize = 1000;
    int lastUsedParticle = 0;
    int FindNextDeadParticle();
};