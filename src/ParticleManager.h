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

    void Emit(SDL_Texture* texture, Animation anim, float x, float y, float vx, float vy, float life, float size, bool useCamera = true, float angularVelocity = 0.0f);

private:
    std::vector<Particle> pool;
    int poolSize = 1000;
    int lastUsedParticle = 0;

    int FindNextDeadParticle();
};