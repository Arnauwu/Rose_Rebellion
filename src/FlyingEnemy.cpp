#include "FlyingEnemy.h"
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

FlyingEnemy::FlyingEnemy() : Enemy(EntityType::FLYING_ENEMY)
{
    name = "FlyingEnemy";
    currentState = FlyingEnemyState::IDLE;

    // Altura donde esta el enemigo y asegura que el player puede eliminarle
    targetOffsetX = 120.0f; //X
    targetOffsetY = 80.0f;  //Y
    attackRange = 350.0f;    // Parámetros de error

    // Tiempo de carga y espera
    windupDurationMs = 100.0f;   // Tiempo de carga
    cooldownDurationMs = 2000.0f; // intervalo de ataque
}

FlyingEnemy::~FlyingEnemy() {}

bool FlyingEnemy::Awake() {
    return true;
}

bool FlyingEnemy::Start()
{
   
    //Enemigo volador sprite()
    //std::unordered_map<int, std::string> aliases = {
    //    {0, "idle"}, {4, "fly"}, {8, "windup"}, {12, "attack"}, {16, "dead"}
    //};
    //anims.LoadFromTSX("Assets/Textures/FlyingEnemy.tsx", aliases);
    //anims.SetCurrent("idle");
 

    //Para textear
    texture = Engine::GetInstance().textures->Load("Assets/Textures/player1.png");
   
  //Physic Body
    texW = 32;
    texH = 32;
    pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX(), (int)position.getY(), texW / 2, bodyType::DYNAMIC);
    pbody->listener = this;
    pbody->ctype = ColliderType::ENEMY;

    // Elimina la gravedad para que pueda mantener en cielo
    if (pbody != nullptr && !B2_IS_NULL(pbody->body)) {
        Engine::GetInstance().physics->SetGravityScale(pbody, 0.0f);
    }

    //Parametro basico del enemigo volador
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

    //Si esta vivo
    if (Engine::GetInstance().sceneManager->isGamePaused == false && isdead == false)
    {
        GetPhysicsValues();
        Move();
        Knockback();
        ApplyPhysics();
    }

    //Cuando se muere
    if (isdead)
    {
        // Vuelva tener la gravedad, para que pueda caer hacia suelo
        if (!physicsDisabledOnDeath)
        {
            // para que pueda caer hacia suelo cuando se muere
            if (pbody != nullptr && !B2_IS_NULL(pbody->body)) {
                Engine::GetInstance().physics->SetGravityScale(pbody, 1.0f);
            }

            // Dejar de mover cuando muere
            b2Vec2 currentVel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
            Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0.0f, currentVel.y });

            //!
            pbody->ctype = ColliderType::UNKNOWN;

            physicsDisabledOnDeath = true;
        }

        //<Tiempo Desparece cadáver
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

    // Controlar la direccion que orienda el enemigo 
    lookingRight = (playerPos.getX() > myPos.getX());

    // Calcura donde para
    float dirX = (playerPos.getX() < myPos.getX()) ? 1.0f : -1.0f;
    Vector2D targetPos(playerPos.getX() + (dirX * targetOffsetX), playerPos.getY() - targetOffsetY);

    //Para que pueda disparar mientra esta moviendo
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

        // Cuando llegues al campo de tiro, dispara.
        if (distToPlayer <= attackRange) {
            currentState = FlyingEnemyState::WINDUP;
            stateTimer.Start();
        }
        break;
    }
    case FlyingEnemyState::WINDUP:
    {
        // Matener el movimiendo cuando esta cargando
        velocity.x = moveDir.getX() * speed;
        velocity.y = moveDir.getY() * speed;

        // Tiempo de carga
        if (stateTimer.ReadMSec() >= windupDurationMs) {
            currentState = FlyingEnemyState::ATTACK;
        }
        break;
    }
    case FlyingEnemyState::ATTACK:
    {
        ShootProjectile(); // Disparar bala
        currentState = FlyingEnemyState::COOLDOWN;
        stateTimer.Start();
        break;
    }
    case FlyingEnemyState::COOLDOWN:
    {
        // Cuando esta esperando para que pueda disparar al siguiente bala, matega el movimiento hacia player
        velocity.x = moveDir.getX() * speed;
        velocity.y = moveDir.getY() * speed;

        // Dispara después de enfriar.
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
        // se retirará una corta distancia cuando resulta herido.
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

    //  Flip la textura del enemigo 
    SDL_FlipMode sdlFlip = lookingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;

    //Testing
    Engine::GetInstance().render->DrawRotatedTexture(texture, x - 16, y - 16, nullptr, sdlFlip, 1.0);

    // 
 
}

void FlyingEnemy::ShootProjectile() {
    Vector2D spawnPos = GetPosition();

    // Crear la bala derante del la textura del enemigo volador
    spawnPos.setX(lookingRight ? spawnPos.getX() + 20.0f : spawnPos.getX() - 20.0f);

    std::shared_ptr<HomingProjectile> bullet = std::make_shared<HomingProjectile>(spawnPos);
    bullet->Start();
    Engine::GetInstance().entityManager->AddEntity(bullet);
}

bool FlyingEnemy::CleanUp() {
    // Limpiar la textura del player
    if (texture != nullptr) {
        Engine::GetInstance().textures->UnLoad(texture);
        texture = nullptr;
    }

    return Enemy::CleanUp();
}

void FlyingEnemy::OnCollision(PhysBody* physA, PhysBody* physB)
{
    // Si esta muerto no hace daño al player
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