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

    //---------------------------------------------------------------------------------------//
    // Texture
    texDust = Engine::GetInstance().textures->Load("Assets/dust.png");

    //Anims
    AnimationSet tempSet;
    std::unordered_map<int, std::string> aliases = { {0, "dust_anim"} }; 

    if (tempSet.LoadFromTSX("Assets/dust.tsx", aliases)) {
        if (tempSet.Has("dust_anim")) {
            animDust = *tempSet.GetAnim("dust_anim");
        }
    }
    //--------------------------------------------------------------------------------------//

    // 1. Carga la textura estática temporal para los impactos
    texHitSpark = Engine::GetInstance().textures->Load("Assets/Hit.png");

   
    // TODO: Descomentar esto cuando Arte entregue el archivo .tsx con la animación
   /* AnimationSet hitSet;
    std::unordered_map<int, std::string> hitAliases = { {0, "hit_anim"} };
    if (hitSet.LoadFromTSX("Assets/hit_spark.tsx", hitAliases)) {
        if (hitSet.Has("hit_anim")) {
            animHitSpark = *hitSet.GetAnim("hit_anim");
        }
    }*/

    texPickup=Engine::GetInstance().textures->Load("Assets/1.png");

    /* AnimationSet PickupSet;
     std::unordered_map<int, std::string> PickupAliases = { {0, "pickup_anim"} };
     if (PickupSet.LoadFromTSX("Assets/pickup.tsx", PickupAliases)) {
     if (PickupSet.Has("hit_anim")) {
         animHitSpark = *PickupSet.GetAnim("pickup_anim");
     }
    }*/
    return true;
}

bool ParticleManager::Update(float dt) {
    // Actualizamos SOLO las part韈ulas encendidas
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
                //SDL_FRect dstRect = { renderX - (particle.size / 2.0f), renderY - (particle.size / 2.0f), particle.size, particle.size };

                // Aplicar transparencia y color base a la textura temporalmente
                SDL_SetTextureAlphaMod(particle.texture, currentAlpha);
                SDL_SetTextureColorMod(particle.texture, particle.color.r, particle.color.g, particle.color.b);

                if (particle.isAnimated) {
                    // renderizar animaciones
                    SDL_Rect srcRect = particle.anim.GetCurrentFrame();
                    SDL_FRect srcFRect = { (float)srcRect.x, (float)srcRect.y, (float)srcRect.w, (float)srcRect.h };

                    SDL_RenderTextureRotated(renderer, particle.texture, &srcFRect, &dstRect, particle.angle, nullptr, particle.flipMode);
                }
                else {
                    // Renderizar la textura
                    SDL_RenderTextureRotated(renderer, particle.texture, nullptr, &dstRect, particle.angle, nullptr, particle.flipMode);
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
    if (texDust != nullptr) {
        Engine::GetInstance().textures->UnLoad(texDust);
        texDust = nullptr;
    }
    if (texHitSpark != nullptr) {
        Engine::GetInstance().textures->UnLoad(texHitSpark);
        texHitSpark = nullptr;
    }
    if (texPickup != nullptr) {
        Engine::GetInstance().textures->UnLoad(texPickup);
        texPickup = nullptr;
    }
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

// Sobrecarga 3: Particula con Animaci髇
void ParticleManager::Emit(SDL_Texture* texture, Animation anim, float x, float y, float vx, float vy, float life, float size, bool useCamera, float angularVelocity,SDL_FlipMode flipMode) {
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

    // Inyectar y reiniciar la animaci髇
    pool[index].anim = anim;
    pool[index].anim.Reset();
    pool[index].isAnimated = true;
    pool[index].flipMode = flipMode;

    pool[index].active = true;
}

//void ParticleManager::EmitDust(float x, float y) {
//    // Generamos entre 3 y 5 part韈ulas por cada pisada
//    int numParticles = 3 + (rand() % 3);
//
//    for (int i = 0; i < numParticles; i++) {
//        // Velocidad X: Esparcimiento aleatorio muy ligero hacia los lados
//        float vx = ((rand() % 100) - 50) * 0.4f;
//
//        // Velocidad Y: Siempre hacia arriba (negativo), imitando el polvo que se levanta
//        float vy = -((rand() % 100) + 50) * 0.8f;
//
//        // Vida corta: Se desvanece r醦ido (entre 200 y 400 milisegundos)
//        float life = 200.0f + (rand() % 200);
//
//        // Color: Gris clarito / Blanco sucio
//        SDL_Color color = { 200, 200, 200, 200 };
//
//        // Tama駉: Peque駉 (entre 3 y 6 p韝eles)
//        float size = 6.0f + (rand() % 4);
//
//        // Emitimos la part韈ula usando el preset 1 (Cuadrado de color base)
//        // Le pasamos 'true' en useCamera porque esto ocurre en el mundo del juego
//        Emit(x, y, vx, vy, life, color, size, true);
//    }
//}

void ParticleManager::EmitDust(float x, float y, bool lookingRight) {
    int numParticles = 1;

    for (int i = 0; i < numParticles; i++) {
        float vx = lookingRight ? -10.0f : 10.0f;
        float vy = -5.0f;
        float life = 400.0f;
        float size = 48.0f;

        SDL_FlipMode flip = lookingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;

        // desplaza la coordenada hacia la izquierda y arriba por la mitad de su tamaño antes de emitir.
        float startX = x - (size / 2.0f);
        float startY = y - (size / 2.0f);

        if (texDust != nullptr && animDust.GetFrameCount() > 0) {
            // desplaza la coordenada hacia la izquierda y arriba por la mitad de su tamaño antes de emitir.
            Emit(texDust, animDust, startX, startY, vx, vy, life, size, true, 0.0f, flip);
        }
        else {
            SDL_Color color = { 200, 200, 200, 200 };
            Emit(startX, startY, vx, vy, life, color, 8.0f, true);
        }
    }
}

void ParticleManager::EmitHitSparks(float x, float y, bool isBlood) {
    
    int numParticles = 1;

    for (int i = 0; i < numParticles; i++) {
        float vx = 0.0f; // Se queda fijo en la posición del enemigo
        float vy = 0.0f;
        float life = 150.0f; // Desaparece rápido para dar mayor sensación de impacto
        float size = 96.0f;  // Tamaño del efecto (ajusta este valor según el tamaño real de tu imagen)

        // Desplaza la coordenada hacia la izquierda y arriba por la mitad de su tamaño para centrar el impacto
        float startX = x - (size / 2.0f);
        float startY = y - (size / 2.0f);

        if (texHitSpark != nullptr) {

            // Cuando esta hecho la animacion cambiar a esa
            // Emit(texHitSpark, animHitSpark, startX, startY, vx, vy, life, size, true, 0.0f, SDL_FLIP_NONE);

            // Temporal
            Emit(texHitSpark, startX, startY, vx, vy, life, size, true, 0.0f);

        }
        else {
            // Fallback: Si no hay textura, emite cuadrados de colores (Rojo para sangre, Amarillo para chispas)
            SDL_Color color = isBlood ? SDL_Color{ 255, 30, 30, 255 } : SDL_Color{ 255, 200, 50, 255 };
            Emit(startX, startY, vx, vy, life, color, 16.0f, true);
        }
    }
}

void ParticleManager::EmitItemPickup(float x, float y) {
    int numParticles = 25; // Emitir múltiples partículas para crear una explosión brillante

    for (int i = 0; i < numParticles; i++) {
        // Calcular dirección circular aleatoria
        float angle = (rand() % 360) * 3.14159f / 180.0f;
        float speed = 80.0f + (rand() % 80);
        float vx = cos(angle) * speed;
        float vy = sin(angle) * speed - 50.0f; // Ligera tendencia a subir

        float life = 600.0f + (rand() % 400); // Duran entre 0.6 y 1 segundo
        float size = 5.0f + (rand() % 5);

        // Centrar la partícula
        float startX = x - (size / 2.0f);
        float startY = y - (size / 2.0f);

        // Color dorado brillante
        SDL_Color color = { 255, 215, 0, 255 };
        Emit(startX, startY, vx, vy, life, color, size, true);

        // ---> TODO: Cuando Arte entregue la textura de brillos (sparkles), usa esto: <---
        // Emit(texSparkle, startX, startY, vx, vy, life, size, true, 0.0f);
    }
}

// L骻ica del Object Pool (Buffer Circular)
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