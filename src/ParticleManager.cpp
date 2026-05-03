#include "ParticleManager.h"
#include "Engine.h"
#include "Render.h"

ParticleManager::ParticleManager() : Module() {
    name = "particle_manager";
}

ParticleManager::~ParticleManager() {}

bool ParticleManager::Awake() {
    pool.resize(poolSize);
    for (int i = 0; i < poolSize; i++) {
        pool[i].active = false;
    }
    return true;
}

bool ParticleManager::Start() {
    return true;
}

bool ParticleManager::Update(float dt) {
    // Actualizamos SOLO las partículas encendidas
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

    SDL_Renderer* renderer = Engine::GetInstance().render->renderer;

    for (const auto& particle : pool) {
        if (particle.active) {

            float lifeRatio = particle.life / particle.maxLife;
            Uint8 currentAlpha = (Uint8)(particle.color.a * lifeRatio);

            if (particle.texture != nullptr) {

                float renderX = particle.useCamera ? particle.x + Engine::GetInstance().render->camera.x : particle.x;
                float renderY = particle.useCamera ? particle.y + Engine::GetInstance().render->camera.y : particle.y;

                SDL_FRect dstRect = { renderX, renderY, particle.size, particle.size };

                // Aplicar transparencia y color base a la textura temporalmente
                SDL_SetTextureAlphaMod(particle.texture, currentAlpha);
                SDL_SetTextureColorMod(particle.texture, particle.color.r, particle.color.g, particle.color.b);

                if (particle.isAnimated) {
                    // renderizar animaciones
                    SDL_Rect srcRect = particle.anim.GetCurrentFrame();
                    SDL_FRect srcFRect = { (float)srcRect.x, (float)srcRect.y, (float)srcRect.w, (float)srcRect.h };

                    SDL_RenderTextureRotated(renderer, particle.texture, &srcFRect, &dstRect, particle.angle, nullptr, SDL_FLIP_NONE);
                }
                else {
                    // Renderizar la textura
                    SDL_RenderTextureRotated(renderer, particle.texture, nullptr, &dstRect, particle.angle, nullptr, SDL_FLIP_NONE);
                }

                // Restaurar la textura a la normalidad
                SDL_SetTextureAlphaMod(particle.texture, 255);
                SDL_SetTextureColorMod(particle.texture, 255, 255, 255);
            }
            else {
                // Dibujo sin nada
                SDL_Rect rect = { (int)particle.x, (int)particle.y, (int)particle.size, (int)particle.size };
                Engine::GetInstance().render->DrawRectangle(rect, particle.color.r, particle.color.g, particle.color.b, currentAlpha, true, particle.useCamera);
            }
        }
    }
    return true;
}

bool ParticleManager::CleanUp() {
    pool.clear();
    return true;
}

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

// Sobrecarga 2: Particula con textura
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

// Sobrecarga 3: Particula con Animación
void ParticleManager::Emit(SDL_Texture* texture, Animation anim, float x, float y, float vx, float vy, float life, float size, bool useCamera, float angularVelocity) {
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

    pool[index].active = true;
}

// Lógica del Object Pool (Buffer Circular)
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