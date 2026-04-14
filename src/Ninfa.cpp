#include "Ninfa.h"
#include "HomingProjectile.h"
#include <cmath>
#include <SDL3/SDL.h>

#include "Engine.h"
#include "Physics.h"
#include "Textures.h"
#include "SceneManager.h"
#include "Render.h"
#include "Log.h"
#include "EntityManager.h"

FlyingEnemy::FlyingEnemy() : Enemy(EntityType::NINFA)
{
    name = "FlyingEnemy";
    currentState = FlyingEnemyState::IDLE;

    // Distancia ideal para que el jugador pueda atacarlo
    targetOffsetX = 120.0f; // X
    targetOffsetY = 80.0f;  // Y
    attackRange = 350.0f;   // Rango de ataque

    // Tiempo de carga y enfriamiento (cooldown)
    windupDurationMs = 100.0f;   // Tiempo de carga
    cooldownDurationMs = 2000.0f; // intervalo de ataque
}

FlyingEnemy::~FlyingEnemy() {}

bool FlyingEnemy::Awake() {
    return true;
}

bool FlyingEnemy::Start()
{
   
    //Enemigo volador sprite
    
    //std::unordered_map<int, std::string> aliases = {
    //    {0, "idle"}, {4, "fly"}, {8, "windup"}, {12, "attack"}, {16, "dead"}
    //};
    //anims.LoadFromTSX("Assets/Textures/FlyingEnemy.tsx", aliases);
    //anims.SetCurrent("idle");
 

    // Textura temporal para pruebas (testear)
    texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/Ninfa/Ninfa_P.png");
   
   // Physic Body
    texW = 32; 
    texH = 32;
    pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX(), (int)position.getY(), texW / 2, bodyType::DYNAMIC);
    pbody->listener = this;
    pbody->ctype = ColliderType::ENEMY;

    // Elimina la gravedad para que pueda mantenerse en el aire
    if (pbody != nullptr && !B2_IS_NULL(pbody->body)) {
        Engine::GetInstance().physics->SetGravityScale(pbody, 0.0f);
    }

    //Parametros basicos del enemigo volador
    vision = 15;
    speed = 3.0f;
    knockbackForce = 5.0f;
    maxHealth = 20;
    currentHealth = 20;

    return true;
}

bool FlyingEnemy::Update(float dt)
{
    if (!active) return true;

    // Lógica mientras está vivo
    if (Engine::GetInstance().sceneManager->isGamePaused == false && isdead == false)
    {
        GetPhysicsValues();
        Move();
        Knockback();
        ApplyPhysics();
    }

    // Lógica al morir
    if (isdead)
    {
        // Se ejecuta solo una vez al morir
        if (!physicsDisabledOnDeath)
        {
            // Vuelve a tener gravedad para que caiga al suelo
            if (pbody != nullptr && !B2_IS_NULL(pbody->body)) {
                Engine::GetInstance().physics->SetGravityScale(pbody, 1.0f);
            }

            // Detiene el movimiento horizontal al morir (retiene la velocidad Y de caída)
            b2Vec2 currentVel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
            Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0.0f, currentVel.y });

            //!
            pbody->ctype = ColliderType::UNKNOWN;

            physicsDisabledOnDeath = true;
        }

        // Temporizador para que desaparezca el cadáver
        deathTimer += dt;

        if (deathTimer >= 1250.0f)
        {
            pendingToDelete = true;
        }
    }

    Draw(dt);
    return true;
}
void FlyingEnemy::GetPhysicsValues() {
    velocity = { 0.0f, 0.0f };
}

void FlyingEnemy::Move() {
    if (isdead || isKnockedback) return;

    Vector2D playerPos = Engine::GetInstance().sceneManager->GetPlayerPosition();
    Vector2D myPos = GetPosition();
    float distToPlayer = (playerPos - myPos).magnitude();

    // Controla la dirección a la que mira el enemigo
    lookingRight = (playerPos.getX() > myPos.getX());

    // Calcula la posición objetivo (dónde debe detenerse/flotar)
    float dirX = (playerPos.getX() < myPos.getX()) ? 1.0f : -1.0f;
    Vector2D targetPos(playerPos.getX() + (dirX * targetOffsetX), playerPos.getY() - targetOffsetY);

    // Dirección de movimiento hacia la posición objetivo
    Vector2D moveDir = (targetPos - myPos).normalized();

    switch (currentState) {
    case FlyingEnemyState::IDLE:
    {
        if (distToPlayer < vision * 32.0f) {
            currentState = FlyingEnemyState::CHASE;
        }
        break;
    }
    case FlyingEnemyState::CHASE:
    {
        velocity.x = moveDir.getX() * speed;
        velocity.y = moveDir.getY() * speed;

        // Cuando entra en el rango de ataque, se prepara para disparar
        if (distToPlayer <= attackRange) {
            currentState = FlyingEnemyState::WINDUP;
            stateTimer.Start();
        }
        break;
    }
    case FlyingEnemyState::WINDUP:
    {
        // Mantiene el movimiento mientras carga el ataque
        velocity.x = moveDir.getX() * speed;
        velocity.y = moveDir.getY() * speed;

        // Finaliza el tiempo de carga
        if (stateTimer.ReadMSec() >= windupDurationMs) {
            currentState = FlyingEnemyState::ATTACK;
        }
        break;
    }
    case FlyingEnemyState::ATTACK:
    {
        ShootProjectile(); // Dispara la bala
        currentState = FlyingEnemyState::COOLDOWN;
        stateTimer.Start();
        break;
    }
    case FlyingEnemyState::COOLDOWN:
    {
        // Mantiene el movimiento hacia el jugador mientras espera para el siguiente disparo
        velocity.x = moveDir.getX() * speed;
        velocity.y = moveDir.getY() * speed;

        // Termina el enfriamiento y vuelve a perseguir
        if (stateTimer.ReadMSec() >= cooldownDurationMs) {
            currentState = FlyingEnemyState::CHASE;
        }
        break;
    }
    }
}
void FlyingEnemy::ApplyPhysics() {
    Engine::GetInstance().physics->SetLinearVelocity(pbody, { velocity.x, velocity.y });
}

void FlyingEnemy::Knockback()
{
    if (isdead) return;

    if (isKnockedback)
    {
        // anims.SetCurrent("hurt");
        // Retrocede una corta distancia al ser herido
        velocity.x = lookingRight ? -knockbackForce : knockbackForce;
    }

    if (knockbackTime <= 0) {
        isKnockedback = false;
        knockbackTime = 500.0f;
    }
    else {
        knockbackTime -= Engine::GetInstance().GetDt();
    }
}

void FlyingEnemy::Draw(float dt)
{
    if (Engine::GetInstance().sceneManager->isGamePaused == false) {
        // anims.Update(dt);
    }

    int x, y;
    pbody->GetPosition(x, y);

    // Invierte (voltea) la textura del enemigo según su dirección
    SDL_FlipMode sdlFlip = lookingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;

    // Pruebas (Testing)
    Engine::GetInstance().render->DrawRotatedTexture(texture, x - 16, y - 16, nullptr, sdlFlip, 0.2);

   
}

void FlyingEnemy::ShootProjectile() {
    Vector2D spawnPos = GetPosition();

    // Genera la bala un poco por delante del enemigo volador para que no colisione consigo mismo
    spawnPos.setX(lookingRight ? spawnPos.getX() + 20.0f : spawnPos.getX() - 20.0f);

    std::shared_ptr<HomingProjectile> bullet = std::make_shared<HomingProjectile>(spawnPos);
    bullet->Start();
    Engine::GetInstance().entityManager->AddEntity(bullet);
}

bool FlyingEnemy::CleanUp() {
    // Libera la textura del enemigo
    if (texture != nullptr) {
        Engine::GetInstance().textures->UnLoad(texture);
        texture = nullptr;
    }

    return Enemy::CleanUp();
}

void FlyingEnemy::OnCollision(PhysBody* physA, PhysBody* physB)
{
    // Si está muerto, no hace daño ni recibe más golpes
    if (isdead) return;

    switch (physB->ctype)
    {
    case ColliderType::PLAYER_ATTACK:
        TakeDamage(physB->listener->damage);
        isKnockedback = true;
        knockbackTime = 500.0f;
        break;

    default:
        break;
    }
}