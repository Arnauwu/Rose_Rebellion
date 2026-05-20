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
#include "HealthBarManager.h"
#include "Audio.h"
#include "DashObj.h"
#include "HealthOrb.h"
#include <cmath>

#include "tracy/Tracy.hpp"

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
    gritoFx = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/Grito.wav");
    hurtFX = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/SE_Princesa_getDamage.wav");

    // Animaciones (Usando el sistema de aliases de la ninfa base)[cite: 1]
    std::unordered_map<int, std::string> aliases = {
        {0, "idle"}, {20, "appear"}, {40, "attack_shot"},
        {60, "attack_rain"}, {60, "attack_wave"}, {80, "fly"},{100, "hurt"}, {120, "dead"}
    };
    anims.LoadFromTSX("Assets/Textures/Entities/Enemies/NinfaBoss/NinfaMadre_spritesheet.tsx", aliases);
    anims.SetCurrent("idle");

    texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/NinfaBoss/NinfaMadre_spritesheet.png");

    currentState = NinfaMareState::SPAWNING;
    // Especificaciones físicas: Más grande (128x128 o similar)[cite: 1]
    texW = 128;
    texH = 128;
    pbody = Engine::GetInstance().physics->CreateRectangle((int)position.getX(), (int)position.getY(), texW / 0.6, texH * 3.5, bodyType::DYNAMIC);
    //pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX(), (int)position.getY(), texW / 4, bodyType::DYNAMIC);
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
    maxHealth = 150;
    currentHealth = 150;

    return true;
}

bool NinfaMare::Update(float dt)
{
    if (!active) return true;
    if (damageCooldown > 0) damageCooldown -= dt;

    ZoneScoped;


    if (Engine::GetInstance().sceneManager->isGamePaused == false && !isdead)
    {
        Player* player = Engine::GetInstance().entityManager->GetPlayer();
        float distToPlayer = (player->GetPosition() - GetPosition()).magnitude();

        if (distToPlayer <= attackRange) {
            Engine::GetInstance().healthBarManager->SetBoss(this);
        }
        else {
            Engine::GetInstance().healthBarManager->SetBoss(nullptr);
        }

        Move();
        Knockback();
        ApplyPhysics();
    }

    if (isdead) {
        Engine::GetInstance().healthBarManager->SetBoss(nullptr);

        // La primera vez que entra aquí al morir
        if (anims.GetCurrentName() != "dead") {
            Engine::GetInstance().audio->PlayFx(morirFx);
            anims.SetCurrent("dead");

            // Si quieres que caiga al suelo al morir, mantén la gravedad.
            // Si prefieres que se quede flotando donde murió, pon el multiplicador a 0.0f.
            if (pbody != nullptr) {
                Engine::GetInstance().physics->SetGravityScale(pbody, 1.0f); // Cae al morir
                pbody->ctype = ColliderType::UNKNOWN; // Hitbox fantasma
            }

            // REINICIAMOS EL TIMER para que empiece a contar desde 0 el tiempo de muerte
            stateTimer.Start();
        }

        if (stateTimer.ReadMSec() > 600.0f) {
            Vector2D dropPos = GetPosition();

            dropPos.setY(dropPos.getY() + 50.0f);
            // 2. Instanciamos el Orbe de Dash
            auto orb = std::make_shared<DashObj>();

            // 3. Le pasamos la posición manualmente (ya que tu constructor no la pide)
            orb->position = dropPos;

            // 4. Inicializamos el orbe y lo metemos en el juego
            orb->Start();
            Engine::GetInstance().entityManager->AddEntity(orb);
            for (int i = 0; i < 3; ++i) {
                auto hOrb = std::make_shared<HealthOrb>();

                // Calculamos un desplazamiento para separarlos: 
                // i=0 (-40px), i=1 (0px, centro), i=2 (+40px)
                float offsetX = (i - 1) * 80.0f;

                // Los ponemos un poquito más arriba que el orbe del dash para que formen un arco
                float offsetY = 100.0f;

                hOrb->position = Vector2D(dropPos.getX() + offsetX, dropPos.getY() + offsetY);
                hOrb->Start();
                Engine::GetInstance().entityManager->AddEntity(hOrb);
            }
            pendingToDelete = true; // El EntityManager lo borrará de forma segura en el siguiente frame
        }
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
    if (isdead) return;

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
    case NinfaMareState::HURT:
        velocity = { 0, 0 }; // Se queda quieta por el impacto

        if (stateTimer.ReadMSec() >= hurtDurationMs) {
            // Al terminar el tiempo, volvemos a CHASE. 
            // Hasta este momento, OnCollision no dejaba entrar nuevos golpes.
            currentState = NinfaMareState::CHASE;
            anims.SetCurrent("fly");
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

        //        if (nextSpecialIsWave) {
        //            // Toca hacer el ataque de Ola
        //            currentState = NinfaMareState::ATTACK_WAVE;
        //            anims.SetCurrent("attack_wave");
        //            if (anims.GetAnim("attack_wave") != nullptr) anims.GetAnim("attack_wave")->Reset();
        //        }
                //else {
                //    // Toca hacer el ataque de Lluvia
                //    currentState = NinfaMareState::ATTACK_RAIN;
                //    anims.SetCurrent("attack_rain");
                //    if (anims.GetAnim("attack_rain") != nullptr) anims.GetAnim("attack_rain")->Reset();
                //    rainTimer.Start();
                //}

            //    currentState = NinfaMareState::ATTACK_WAVE;
            //    anims.SetCurrent("attack_wave");
            //    if (anims.GetAnim("attack_wave") != nullptr) anims.GetAnim("attack_wave")->Reset();

            //    // Cambiamos el interruptor para que el SIGUIENTE ataque especial sea el otro
            //    nextSpecialIsWave = !nextSpecialIsWave;
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

    //case NinfaMareState::ATTACK_WAVE:
    //    velocity = { 0, 0 };

    //    if (stateTimer.ReadMSec() > 800 && anims.GetCurrentName() == "attack_wave") {
    //        LaunchWaterWave();
    //        anims.SetCurrent("fly");
    //    }

    //    if (stateTimer.ReadMSec() >= 1500) {
    //        currentState = NinfaMareState::COOLDOWN; // Descanso tras completar el combo completo
    //        stateTimer.Start();
    //    }
    //    break;

    //case NinfaMareState::ATTACK_RAIN:
    //    velocity = { 0, 0 }; // Se queda totalmente quieta canalizando

    //    // Mantener la animación de lluvia y evitar que cambie
    //    anims.SetCurrent("attack_rain");

    //    // Dejamos 1 segundo de "casteo" (preparación) antes de que empiece a caer el agua
    //    if (stateTimer.ReadMSec() > 1000) {

    //        // Cada 200ms genera una nueva bala desde el cielo
    //        if (rainTimer.ReadMSec() > 200) {
    //            StartRainAttack();
    //            rainTimer.Start(); // Resetea el contador para la siguiente bala
    //        }
    //    }

    //    // El ataque dura 10 segundos + 1 segundo de preparación inicial = 11000ms
    //    if (stateTimer.ReadMSec() >= 11000) {
    //        currentState = NinfaMareState::COOLDOWN; // Acaba la lluvia y descansa
    //        stateTimer.Start();
    //    }
    //    break;

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
    if (pbody != nullptr) {
        Engine::GetInstance().physics->SetLinearVelocity(pbody, { velocity.x, velocity.y });
    }
}

void NinfaMare::Draw(float dt) {
    if (currentState == NinfaMareState::SPAWNING && !hasAppeared) {
        Player* player = Engine::GetInstance().entityManager->GetPlayer();
        Vector2D playerPos = player->GetPosition();
        Vector2D myPos = GetPosition();
        float distToPlayer = (playerPos - myPos).magnitude();

        if (distToPlayer > attackRange) {
            return;
        }
    }

    if (!Engine::GetInstance().sceneManager->isGamePaused) anims.Update(dt);

    const SDL_Rect& animFrame = anims.GetCurrentFrame();

    // 1. Por defecto, usamos la última posición conocida de la entidad
    int x = (int)position.getX();
    int y = (int)position.getY();

    // 2. ¡COMPROBACIÓN DE SEGURIDAD! 
    // Solo le pedimos la posición a las físicas si el cuerpo físico sigue vivo.
    if (pbody != nullptr) {
        pbody->GetPosition(x, y);

        // Guardamos esta posición en la variable 'position' para recordarla 
        // por si en el siguiente frame el pbody se borra.
        position.setX(x);
        position.setY(y);
    }

    SDL_FlipMode sdlFlip = lookingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;

    Engine::GetInstance().render->DrawRotatedTexture(texture, x, y, &animFrame, sdlFlip, 1.0);
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
    if (texture != nullptr) {
        Engine::GetInstance().textures->UnLoad(texture);
        texture = nullptr; // Buena práctica ponerlo a nulo tras descargarlo
    }

    // ELIMINAMOS LA LÍNEA DEL HEALTHBARMANAGER AQUÍ
    // Engine::GetInstance().healthBarManager->SetBoss(nullptr); 

    return Enemy::CleanUp();
}
void NinfaMare::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) {
    // CONDICIÓN CRÍTICA: Si ya está en HURT, ignoramos el resto de frames del mismo golpe
    if (isdead || currentState == NinfaMareState::SPAWNING || currentState == NinfaMareState::HURT) {
        return;
    }

    if (physB->ctype == ColliderType::PLAYER_ATTACK) {
        Engine::GetInstance().audio->PlayFx(hurtFX);
        // 1. Aplicar daño
        TakeDamage(physB->listener->damage);

        if (currentHealth <= 0 && !isdead) {
            isdead = true;
            currentHealth = 0;
            Engine::GetInstance().healthBarManager->SetBoss(nullptr);
            return; // Salir de la función para que no pase a estado HURT
        }

        // 2. Cambiar estado inmediatamente (esto bloquea nuevas entradas aquí)
        currentState = NinfaMareState::HURT;

        // 3. Resetear y poner la animación una sola vez
        anims.SetCurrent("hurt");
        if (anims.GetAnim("hurt") != nullptr) {
            anims.GetAnim("hurt")->Reset();
        }

        // 4. Iniciar el contador de tiempo de la animación (400ms)
        stateTimer.Start();

        // 5. Knockback opcional
        isKnockedback = true;
        knockbackTime = 200.0f;
    }
}

void NinfaMare::StartRainAttack() {
    Player* player = Engine::GetInstance().entityManager->GetPlayer();
    Vector2D playerPos = player->GetPosition();

    // 1. Calculamos la posición en el cielo
    float randomX = playerPos.getX() + (rand() % 600 - 300); // 300px a la izq o derecha
    float spawnY = playerPos.getY() - 600; // 600px en el cielo
    Vector2D spawnPos(randomX, spawnY);

    // 2. Creamos la bala
    auto raindrop = std::make_shared<HomingProjectile>(spawnPos);

    // 3. Dejamos que haga su Start() (el cual crea el hitbox equivocadamente dentro del boss)
    raindrop->Start();

    // 4. ¡EL TRUCO CON TU MOTOR! 
    // Borramos su colisionador atascado y se lo recreamos en el cielo
    if (raindrop->pbody != nullptr) {

        // Guardamos su tipo (ENEMY_PROJECTILE o el que sea) para no perderlo
        ColliderType oldType = raindrop->pbody->ctype;

        // Eliminamos el cuerpo que se quedó atascado en el Boss
        Engine::GetInstance().physics->DeletePhysBody(raindrop->pbody);

        // Le creamos un círculo nuevo exactamente en las coordenadas del cielo
        // (Ponemos radio 15, puedes ajustarlo si la bala es más grande/pequeña)
        raindrop->pbody = Engine::GetInstance().physics->CreateCircle((int)spawnPos.getX(), (int)spawnPos.getY(), 15, bodyType::DYNAMIC);

        // Le volvemos a conectar las colisiones y su tipo
        raindrop->pbody->listener = raindrop.get();
        raindrop->pbody->ctype = oldType;
    }

    // 5. Corregimos su posición visual
    raindrop->position = spawnPos;

    // 6. Añadimos al juego
    Engine::GetInstance().entityManager->AddEntity(raindrop);
}