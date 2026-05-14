#include "NinfaBoss.h"
#include "HomingProjectile.h"
#include <SDL3/SDL.h>

#include "Engine.h"
#include "Physics.h"
#include "Textures.h"
#include "SceneManager.h"
#include "Render.h"
#include "Log.h"
#include "EntityManager.h"
#include "Audio.h"
#include <cmath>

NinfaMare::NinfaMare() : Enemy(EntityType::NINFA_MARE) // O EntityType::BOSS si tienes uno
{
    name = "NinfaMare";
    currentState = NinfaMareState::SPAWNING;
    active = true;
}

NinfaMare::~NinfaMare() {}

bool NinfaMare::Awake() { return true; }

bool NinfaMare::Start()
{
    // Carga de Audio[cite: 1]
    morirFx = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/SE_Ninfa_Muerte.wav");
    atacarFx = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/SE_Ninfa_Ataquefuerte.wav");
    volarFx = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/SE_Ninfa_Walking.wav");
    gritoFx = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/SE_Boss_Grito.wav");

    // Animaciones (Usando el sistema de aliases de la ninfa base)[cite: 1]
    std::unordered_map<int, std::string> aliases = {
        {0, "idle"}, {20, "appear"}, {40, "attack_shot"},
        {60, "attack_wave"}, {80, "fly"},{100, "hurt"}, {120, "dead"}
    };
    anims.LoadFromTSX("Assets/Textures/Entities/Enemies/NinfaBoss/NinfaMadre_spritesheet.tsx", aliases);
    anims.SetCurrent("idle");

    texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/NinfaBoss/NinfaMadre_spritesheet.png");

    currentState = NinfaMareState::SPAWNING;
    // Especificaciones físicas: Más grande (128x128 o similar)[cite: 1]
    texW = 128;
    texH = 128;

    pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX(), (int)position.getY(), texW / 3, bodyType::DYNAMIC);
    pbody->listener = this;
    pbody->ctype = ColliderType::ENEMY;

    // Vuelo: Sin gravedad[cite: 1]
    if (pbody != nullptr && !B2_IS_NULL(pbody->body)) {
        Engine::GetInstance().physics->SetGravityScale(pbody, 0.0f);
    }

    pathfinding = std::make_shared<Pathfinding>(false);
    stateTimer.Start();

    // Stats de Boss
    vision = 40;
    speed = 1.8f; // Más lenta pero imponente
    maxHealth = 500;
    currentHealth = 500;

    return true;
}

bool NinfaMare::Update(float dt)
{
    if (!active) return true;

    if (Engine::GetInstance().sceneManager->isGamePaused == false && !isdead)
    {
        switch (currentState) {
        case NinfaMareState::SPAWNING: HandleSpawningLogic(); break;
        case NinfaMareState::IDLE:
        case NinfaMareState::CHASE:
        case NinfaMareState::ATTACK_SHOT:
        case NinfaMareState::ATTACK_WAVE:
        case NinfaMareState::ATTACK_RAIN:
        case NinfaMareState::COOLDOWN:
            Move();
            break;
        }

        ApplyPhysics();
    }

    if (isdead) {
        if (anims.GetCurrentName() != "dead") {
            Engine::GetInstance().audio->PlayFx(morirFx);
            anims.SetCurrent("dead");
            Engine::GetInstance().physics->SetGravityScale(pbody, 1.0f); // Cae al morir[cite: 1]
            pbody->ctype = ColliderType::UNKNOWN;
        }
        if (anims.GetAnim("dead")->HasFinishedOnce()) pendingToDelete = true;
    }

    Draw(dt);
    return true;
}

void NinfaMare::HandleSpawningLogic() {
    // Aquí iría la lógica de partículas de burbujas
    if (stateTimer.ReadMSec() >= spawnDurationMs) {
        Engine::GetInstance().audio->PlayFx(gritoFx);
        currentState = NinfaMareState::CHASE;
    }
}

void NinfaMare::Move() {
    if (isdead || isKnockedback) return;

    Player* player = Engine::GetInstance().entityManager->GetPlayer();
    Vector2D playerPos = player->GetPosition();
    Vector2D myPos = GetPosition();
    float distToPlayer = (playerPos - myPos).magnitude();

    lookingRight = (playerPos.getX() > myPos.getX());

    // Posicionamiento de vuelo[cite: 1]
    float dirX = (playerPos.getX() < myPos.getX()) ? 1.0f : -1.0f;
    Vector2D targetPos(playerPos.getX() + (dirX * targetOffsetX), playerPos.getY() - targetOffsetY);
    Vector2D moveDir = (targetPos - myPos).normalized();

    // IA del Boss
    switch (currentState) {
    case NinfaMareState::SPAWNING: // <-- Tu estado asignado
        velocity = { 0, 0 }; // Completamente quieta durante la presentación

        if (!hasAppeared) {
            // Si el jugador entra en rango, el Boss "despierta"
            if (distToPlayer <= attackRange) {
                hasAppeared = true;
                Engine::GetInstance().audio->PlayFx(gritoFx); // Grito cinematográfico
                anims.SetCurrent("appear"); // <-- Pon aquí el nombre exacto de tu animación en el .tsx
                if (anims.GetAnim("appear") != nullptr) anims.GetAnim("appear")->Reset();
                stateTimer.Start();
            }
        }
        else {
            // Una vez activada, esperamos a que termine el tiempo de spawn existente
            if (stateTimer.ReadMSec() >= spawnDurationMs) { // <-- Usamos tus 3000ms configurados
                currentState = NinfaMareState::CHASE; // Al acabar, empieza a perseguir
                anims.SetCurrent("fly");
            }
        }
        break;
    case NinfaMareState::CHASE:
        anims.SetCurrent("fly");
        velocity.x = moveDir.getX() * speed;
        velocity.y = moveDir.getY() * speed;

        if (distToPlayer <= attackRange) {
            // El combo SIEMPRE empieza disparando proyectiles (no es aleatorio)
            currentState = NinfaMareState::ATTACK_SHOT;
            anims.SetCurrent("attack_shot");
            if (anims.GetAnim("attack_shot") != nullptr) anims.GetAnim("attack_shot")->Reset();

            velocity = { 0, 0 };
            stateTimer.Start();
        }
        break;

    case NinfaMareState::ATTACK_SHOT:
        velocity = { 0, 0 }; // Se detiene para disparar

        // Sincronización del disparo en la mitad de la animación
        if (stateTimer.ReadMSec() > 600 && anims.GetCurrentName() == "attack_shot") {
            ShootHomingProjectile();
            shotsFiredInCombo++; // <--- Contamos este disparo
            anims.SetCurrent("fly");
        }

        // Cuando termina la animación de este disparo individual...
        if (stateTimer.ReadMSec() >= 1200) {
            // Comprobamos si ya ha completado la ráfaga de 10 disparos
            if (shotsFiredInCombo >= 10) {
                shotsFiredInCombo = 0; // Reseteamos el contador para la próxima vez

                if (nextSpecialIsWave) {
                    // Toca hacer el ataque de Ola
                    currentState = NinfaMareState::ATTACK_WAVE;
                    anims.SetCurrent("attack_wave");
                    if (anims.GetAnim("attack_wave") != nullptr) anims.GetAnim("attack_wave")->Reset();
                }
                else {
                    // Toca hacer el ataque de Lluvia
                    currentState = NinfaMareState::ATTACK_RAIN;
                    anims.SetCurrent("attack_rain");
                    if (anims.GetAnim("attack_rain") != nullptr) anims.GetAnim("attack_rain")->Reset();
                }

                // Cambiamos el interruptor para que el SIGUIENTE ataque especial sea el otro
                nextSpecialIsWave = !nextSpecialIsWave;
            }
            else {
                // Si aún no lleva 10 disparos, vuelve a reproducir la animación de ataque para lanzar otro
                currentState = NinfaMareState::ATTACK_SHOT;
                anims.SetCurrent("attack_shot");
                if (anims.GetAnim("attack_shot") != nullptr) anims.GetAnim("attack_shot")->Reset();
            }
            stateTimer.Start();
        }
        break;

    case NinfaMareState::ATTACK_WAVE:
        velocity = { 0, 0 };

        if (stateTimer.ReadMSec() > 800 && anims.GetCurrentName() == "attack_wave") {
            LaunchWaterWave();
            anims.SetCurrent("fly");
        }

        if (stateTimer.ReadMSec() >= 1500) {
            currentState = NinfaMareState::COOLDOWN; // Descanso tras completar el combo completo
            stateTimer.Start();
        }
        break;

    case NinfaMareState::ATTACK_RAIN:
        velocity = { 0, 0 };
        if (stateTimer.ReadMSec() > 1000 && anims.GetCurrentName() == "attack_rain") {
            StartRainAttack();
            anims.SetCurrent("fly");
        }

        if (stateTimer.ReadMSec() >= 1800) {
            currentState = NinfaMareState::COOLDOWN; // Descanso tras completar el combo completo
            stateTimer.Start();
        }
        break;

    case NinfaMareState::COOLDOWN:
        anims.SetCurrent("fly");
        velocity.x = moveDir.getX() * (speed * 0.4f);
        velocity.y = moveDir.getY() * (speed * 0.4f);

        if (stateTimer.ReadMSec() > shotCooldownMs) {
            currentState = NinfaMareState::CHASE;
        }
        break;
    }
}

void NinfaMare::ShootHomingProjectile() {
    Engine::GetInstance().audio->PlayFx(atacarFx);
    Vector2D spawnPos = GetPosition();

    // Proyectil un poco más grande que el estándar[cite: 1]
    auto bullet = std::make_shared<HomingProjectile>(spawnPos);
    bullet->Start();

    Engine::GetInstance().entityManager->AddEntity(bullet);
}

void NinfaMare::LaunchWaterWave() {
    // Aquí instanciarías tu entidad de Ola que recorre el suelo
    LOG("Ninfa Mare lanza una OLA de agua!");
}

void NinfaMare::ApplyPhysics() {
    Engine::GetInstance().physics->SetLinearVelocity(pbody, { velocity.x, velocity.y });
}

void NinfaMare::Draw(float dt) {
    if (currentState == NinfaMareState::SPAWNING && !hasAppeared) {
        Player* player = Engine::GetInstance().entityManager->GetPlayer();
        Vector2D playerPos = player->GetPosition();
        Vector2D myPos = GetPosition();
        float distToPlayer = (playerPos - myPos).magnitude();

        // Si el jugador está lejos, salimos sin dibujar nada
        if (distToPlayer > attackRange) {
            return;
        }
    }

    if (!Engine::GetInstance().sceneManager->isGamePaused) anims.Update(dt);

    const SDL_Rect& animFrame = anims.GetCurrentFrame();
    int x, y;
    pbody->GetPosition(x, y);

    SDL_FlipMode sdlFlip = lookingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;

    // Dibujar con escala mayor (1.5f para que sea imponente)[cite: 1]
    Engine::GetInstance().render->DrawRotatedTexture(texture, x, y, &animFrame, sdlFlip, 1.5);
}

void NinfaMare::Knockback() {
    // Los bosses suelen tener resistencia al knockback, podemos reducirlo
    if (isKnockedback && knockbackTime > 0) {
        velocity.x = lookingRight ? -2.0f : 2.0f;
        knockbackTime -= Engine::GetInstance().GetDt();
    }
    else {
        isKnockedback = false;
    }
}

void NinfaMare::GetPhysicsValues() { velocity = { 0, 0 }; }

bool NinfaMare::CleanUp() {
    if (texture != nullptr) Engine::GetInstance().textures->UnLoad(texture);
    return Enemy::CleanUp();
}

void NinfaMare::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) {
    if (isdead || currentState == NinfaMareState::SPAWNING) return;

    if (physB->ctype == ColliderType::PLAYER_ATTACK) {
        TakeDamage(physB->listener->damage);
        isKnockedback = true;
        knockbackTime = 200.0f;
    }
}

void NinfaMare::StartRainAttack() {
    // Aquí puedes instanciar gotas de agua cayendo desde arriba de la pantalla
    LOG("Ninfa Mare invoca una LLUVIA de agua!");
}