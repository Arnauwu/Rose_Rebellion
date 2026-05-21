#include "ParticleManager.h"
#include "Engine.h"
#include "Render.h"
#include "tracy/Tracy.hpp"

// Constructor
ParticleManager::ParticleManager() : Module() {
    name = "particle_manager";
}

// Destructor
ParticleManager::~ParticleManager() {}

bool ParticleManager::Awake() {
    pool.resize(poolSize);
    for (int i = 0; i < poolSize; i++) {
        pool[i].active = false;
    }
    return true;
}

bool ParticleManager::Start() {
    // 1. Cargar el SpriteSheet Unificado de Polvo (Andar, Jump, Dash)
    dustSpriteSheet = Engine::GetInstance().textures->Load("Assets/Textures/Particles/SS_particulas_polvo.png");

    AnimationSet dustAnimSet;
    std::unordered_map<int, std::string> dustAliases = { {0,"andar"}, {8,"jump"}, {16,"dash"} };

    if (dustAnimSet.LoadFromTSX("Assets/Textures/Particles/SS_particulas_polvo.tsx", dustAliases)) {
        if (dustAnimSet.Has("andar")) animWalkDust = *dustAnimSet.GetAnim("andar");
        if (dustAnimSet.Has("jump"))  animJumpDust = *dustAnimSet.GetAnim("jump");
        if (dustAnimSet.Has("dash"))  animDashDust = *dustAnimSet.GetAnim("dash");
    }

    // 2. Cargar texturas de los demás efectos (Impactos, Sangre, Pickups)
    attackSpriteSheet= Engine::GetInstance().textures->Load("Assets/Textures/Particles/SS_efectos_destello_ataque.png");

    AnimationSet attackAnimSet;
    std::unordered_map<int, std::string> attackAliases = { {0,"Attack1"}, {16,"Attack2"} };
    if (attackAnimSet.LoadFromTSX("Assets/Textures/Particles/SS_efectos_destello_ataque.tsx", attackAliases)) {
        if (attackAnimSet.Has("Attack1")) animAttack1 = *attackAnimSet.GetAnim("Attack1");
        if (attackAnimSet.Has("Attack2")) animAttack2 = *attackAnimSet.GetAnim("Attack2");
    }


    hitP = Engine::GetInstance().textures->Load("Assets/Textures/Particles/hitP.png");
    bloodP = Engine::GetInstance().textures->Load("Assets/Textures/Particles/bloodP.png");
    pickP = Engine::GetInstance().textures->Load("Assets/Textures/Particles/pickP.png");

    return true;
}

bool ParticleManager::Update(float dt) {
    ZoneScoped;

    // Actualizamos SOLO las articulas encendidas
    for (auto& particles : pool) {
        if (particles.active) {

            particles.x += particles.vx * (dt / 1000.0f);
            particles.y += particles.vy * (dt / 1000.0f);
            particles.angle += particles.angularVelocity * (dt / 1000.0f);

            // Anim update
            if (particles.isAnimated) {
                particles.anim.Update(dt);
            }

            particles.life -= dt;

            if (particles.life <= 0.0f) {
                particles.active = false;
            }
        }
    }
    return true;
}

bool ParticleManager::PostUpdate() {
    ZoneScoped;

    for (const auto& particle : pool) {
        if (particle.active) {

            float lifeRatio = 1.0f;
            if (particle.life < particle.maxLife * 0.5f) {
                lifeRatio = particle.life / (particle.maxLife * 0.5f);
            }
            Uint8 currentAlpha = (Uint8)(particle.color.a * lifeRatio);

            if (particle.texture != nullptr) {

                SDL_Rect srcRect = { 0, 0, 0, 0 };
                if (particle.isAnimated) {
                    srcRect = particle.anim.GetCurrentFrame();
                }

            
                SDL_SetTextureColorMod(particle.texture, 0, 0, 0);
                
                SDL_SetTextureAlphaMod(particle.texture, currentAlpha / 2);
               
                Engine::GetInstance().render->DrawRotatedTexture(particle.texture, particle.x + 3.0f, particle.y + 3.0f, particle.isAnimated ? &srcRect : nullptr, particle.flipMode, particle.scale, particle.angle);                SDL_SetTextureColorMod(particle.texture, particle.color.r, particle.color.g, particle.color.b);
                SDL_SetTextureAlphaMod(particle.texture, currentAlpha);

                Engine::GetInstance().render->DrawRotatedTexture(particle.texture, particle.x, particle.y, particle.isAnimated ? &srcRect : nullptr, particle.flipMode, particle.scale, particle.angle);
                SDL_SetTextureAlphaMod(particle.texture, 255);
                SDL_SetTextureColorMod(particle.texture, 255, 255, 255);
            }
            else {
                
                SDL_Rect rect = { (int)particle.x, (int)particle.y, (int)particle.size, (int)particle.size };
                Engine::GetInstance().render->DrawRectangle(rect, particle.color.r, particle.color.g, particle.color.b, currentAlpha, true, particle.useCamera);
            }
        }
    }
    return true;
}

bool ParticleManager::CleanUp() {
    pool.clear();

    // Limpiamos la nueva textura unificada
    if (dustSpriteSheet != nullptr) {
        Engine::GetInstance().textures->UnLoad(dustSpriteSheet);
        dustSpriteSheet = nullptr;
    }

    // Limpiamos el resto
    if (hitP != nullptr) {
        Engine::GetInstance().textures->UnLoad(hitP);
        hitP = nullptr;
    }
    if (pickP != nullptr) {
        Engine::GetInstance().textures->UnLoad(pickP);
        pickP = nullptr;
    }
    if (bloodP != nullptr) {
        Engine::GetInstance().textures->UnLoad(bloodP);
        bloodP = nullptr;
    }
    if (attackSpriteSheet != nullptr) {
        Engine::GetInstance().textures->UnLoad(attackSpriteSheet);
        attackSpriteSheet = nullptr;
    }
    return true;
}

// ---------------------------------------------------------
// FUNCIONES BASE DE EMISIÓN
// ---------------------------------------------------------

// Sobrecarga 1: Cuadrado sin nada
void ParticleManager::Emit(float x, float y, float vx, float vy, float life, SDL_Color color, float size, bool useCamera) {
    int index = FindNextDeadParticle();

    pool[index].x = x;
    pool[index].y = y;
    pool[index].vx = vx;
    pool[index].vy = vy;
    pool[index].life = life;
    pool[index].maxLife = life;
    pool[index].color = color;
    pool[index].size = size;
    pool[index].useCamera = useCamera;

    pool[index].texture = nullptr;
    pool[index].angle = 0.0f;
    pool[index].angularVelocity = 0.0f;
    pool[index].isAnimated = false;

    pool[index].active = true;
}

// Sobrecarga 2: Partícula con textura
void ParticleManager::Emit(SDL_Texture* texture, float x, float y, float vx, float vy, float life, float size, bool useCamera, float angularVelocity) {
    int index = FindNextDeadParticle();

    pool[index].x = x;
    pool[index].y = y;
    pool[index].vx = vx;
    pool[index].vy = vy;
    pool[index].life = life;
    pool[index].maxLife = life;
    pool[index].color = { 255, 255, 255, 255 };
    pool[index].size = size;
    pool[index].useCamera = useCamera;

    pool[index].texture = texture;
    pool[index].angle = 0.0f;
    pool[index].angularVelocity = angularVelocity;
    pool[index].isAnimated = false;

    pool[index].active = true;
}

// Sobrecarga 3: Partícula con Animación
void ParticleManager::Emit(SDL_Texture* texture, Animation anim, float x, float y, float vx, float vy, float life, float size, bool useCamera, float angularVelocity, SDL_FlipMode flipMod, float scale) {
    int index = FindNextDeadParticle();

    pool[index].x = x;  
    pool[index].y = y;
    pool[index].vx = vx;
    pool[index].vy = vy;
    pool[index].life = life;
    pool[index].maxLife = life;
    pool[index].color = { 255, 255, 255, 255 };
    pool[index].size = size;
    pool[index].useCamera = useCamera;

    pool[index].texture = texture;
    pool[index].angle = 0.0f;
    pool[index].angularVelocity = angularVelocity;

    // Inyectar y reiniciar la animación
    pool[index].anim = anim;
    pool[index].anim.Reset();
    pool[index].isAnimated = true;
    pool[index].flipMode = flipMod;
    pool[index].scale = scale;

    pool[index].active = true;
}

// ---------------------------------------------------------
// FUNCIONES ESPECÍFICAS
// ---------------------------------------------------------

void ParticleManager::EmitDust(float x, float y, bool lookingRight) {
    int numParticles = 1;

    for (int i = 0; i < numParticles; i++) {
        float vx = lookingRight ? -10.0f : 10.0f;
        float vy = -5.0f;
        float life = 400.0f;
        float size = 48.0f;

        SDL_FlipMode flip = lookingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;

        // --- AJUSTE DE POSICIÓN: TALÓN (Heel) ---
       
        float offsetX = lookingRight ? 5.0f : 45.0f;
        float startX = x - (size / 2.0f) + offsetX;
        float startY = y - (size / 2.0f);

        // Usamos el SpriteSheet unificado y la animación 'animWalkDust'
        if (dustSpriteSheet != nullptr && animWalkDust.GetFrameCount() > 0) {
            Emit(dustSpriteSheet, animWalkDust, startX, startY, vx, vy, life, size, true, 0.0f, flip);
        }
        else {
            SDL_Color color = { 200, 200, 200, 200 };
            Emit(startX, startY, vx, vy, life, color, 8.0f, true);
        }
    }
}

void ParticleManager::EmitJumpDust(float x, float y, bool lookingRight) {
    int numParticles = 1;

    for (int i = 0; i < numParticles; i++) {
        float vx = 0.0f;
        float vy = 0.0f;
        float life = 400.0f;
        float size = 64.0f;


        float offsetX = lookingRight ? -20.0f : 20.0f;

        float startX = x - (size / 2.0f) + offsetX+25.0f;
        // ==========================================

        float startY = y - (size / 2.0f);

        if (dustSpriteSheet != nullptr && animJumpDust.GetFrameCount() > 0) {
            Emit(dustSpriteSheet, animJumpDust, startX, startY, vx, vy, life, size, true, 0.0f, SDL_FLIP_NONE);
        }
        else {
            SDL_Color color = { 200, 200, 200, 200 };
            Emit(startX, startY, vx, vy, life, color, 16.0f, true);
        }
    }
}

//Si tenemos dash descomenta eso
void ParticleManager::EmitDashDust(float x, float y, bool lookingRight) {
      /*
      int numParticles = 1;

      for (int i = 0; i < numParticles; i++) {
          float vx = 0.0f;
          float vy = 0.0f;
          float life = 400.0f;
          float size = 64.0f;

          SDL_FlipMode flip = lookingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;

          // --- AJUSTE DE POSICIÓN: TALÓN (Heel) ---
          // Al hacer dash, el polvo suele quedar un poco más atrás que al caminar
          float offsetX = lookingRight ? -30.0f : 30.0f;

          float startX = (x + offsetX) - (size / 2.0f);
          float startY = y - (size / 2.0f);

          if (dustSpriteSheet != nullptr && animDashDust.GetFrameCount() > 0) {
              Emit(dustSpriteSheet, animDashDust, startX, startY, vx, vy, life, size, true, 0.0f, flip);
          }
          else {
              SDL_Color color = { 150, 200, 255, 200 };
              Emit(startX, startY, vx, vy, life, color, 16.0f, true);
          }
      }
      */
}

void ParticleManager::EmitHitSparks(float x, float y, bool isBlood) {
    int numParticles = 1;

    for (int i = 0; i < numParticles; i++) {
        float vx = 0.0f;
        float vy = 0.0f;
        float life = 150.0f;
        float size = 96.0f;

        float startX = x - (size / 2.0f);
        float startY = y - (size / 2.0f);

        SDL_Texture* currentTexture = isBlood ? bloodP : hitP;

        if (currentTexture != nullptr) {
            Emit(currentTexture, startX, startY, vx, vy, life, size, true, 0.0f);
        }
        else {
            SDL_Color color = isBlood ? SDL_Color{ 255, 30, 30, 255 } : SDL_Color{ 255, 200, 50, 255 };
            Emit(startX, startY, vx, vy, life, color, 16.0f, true);
        }
    }
}

void ParticleManager::EmitItemPickup(float x, float y) {
    int numParticles = 25;

    for (int i = 0; i < numParticles; i++) {
        float angle = (rand() % 360) * 3.14159f / 180.0f;
        float speed = 80.0f + (rand() % 80);
        float vx = cos(angle) * speed;
        float vy = sin(angle) * speed - 50.0f;

        float life = 600.0f + (rand() % 400);
        float size = 5.0f + (rand() % 5);

        float startX = x - (size / 2.0f);
        float startY = y - (size / 2.0f);

        SDL_Color color = { 255, 215, 0, 255 };
        Emit(startX, startY, vx, vy, life, color, size, true);
    }
}

// ---------------------------------------------------------
// LÓGICA DE POOL
// ---------------------------------------------------------

int ParticleManager::FindNextDeadParticle() {
    for (int i = lastUsedParticle; i < poolSize; i++) {
        if (!pool[i].active) {
            lastUsedParticle = i;
            return i;
        }
    }
    for (int i = 0; i < lastUsedParticle; i++) {
        if (!pool[i].active) {
            lastUsedParticle = i;
            return i;
        }
    }
    lastUsedParticle = 0;
    return 0;
}

void ParticleManager::EmitAttack(float x, float y, bool lookingRight) {
    float life = 200.0f;
    float scale = 2.0f;

    if (attackSpriteSheet != nullptr) {
        int randomChoice = rand() % 2;
        Animation* selectedAnim = (randomChoice == 0) ? &animAttack1 : &animAttack2;
        if (selectedAnim->GetFrameCount() > 0) {
            SDL_FlipMode flip = lookingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;

        
            Emit(attackSpriteSheet, *selectedAnim, x, y, 0, 0,
                life, 150.0f, true, 0.0f, flip, scale);
        }
    }
}