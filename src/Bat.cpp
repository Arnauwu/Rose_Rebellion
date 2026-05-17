#include "Bat.h"
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
#include "ParticleManager.h"
#include "Audio.h"

#include "tracy/Tracy.hpp"

Bat::Bat() : Enemy(EntityType::BAT)
{
    name = "NBat";
    currentState = BatState::IDLE;

    // Distancia ideal para que el jugador pueda atacarlo
    targetOffsetX = 120.0f; // X
    targetOffsetY = 80.0f;  // Y
    attackRange = 350.0f;   // Rango de ataque

    // Tiempo de carga y enfriamiento (cooldown)
    windupDurationMs = 100.0f;   // Tiempo de carga
    cooldownDurationMs = 2000.0f; // intervalo de ataque
}

Bat::~Bat() {}

bool Bat::Awake() {
    return true;
}

bool Bat::Start()
{
   //morirBat = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/SE_Bat_Muerte.wav");
   //atacarBat = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/SE_Bat_Ataquefuerte.wav");
   //volarBat = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/SE_Bat_Walking.wav");
    //Enemigo volador sprite
    
    std::unordered_map<int, std::string> aliases = {
        {0, "idle"}, {3, "fly"}, {13, "attack"},{26, "attack"}, {39, "dead"},{39, "dead"}, { 52, "bullet" }
    };
    anims.LoadFromTSX("Assets/Textures/Entities/Enemies/Ninfa/Ninfa.tsx", aliases);
    anims.SetCurrent("idle");
 

    // Textura temporal para pruebas (testear)
    texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/Ninfa/Ninfa.png");
   
   // Physic Body
    texW = 64; 
    texH = 64;
    pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX(), (int)position.getY(), texW / 2, bodyType::DYNAMIC);
    pbody->listener = this;
    pbody->ctype = ColliderType::ENEMY;

    // Elimina la gravedad para que pueda mantenerse en el aire
    if (pbody != nullptr && !B2_IS_NULL(pbody->body)) 
    {
        Engine::GetInstance().physics->SetGravityScale(pbody, 0.0f);
    }

    // Initialize pathfinding
    pathfinding = std::make_shared<Pathfinding>(false);

    //Reset pathfinding
    pathfinding->ResetPath(GetTilePos());

    pathFindingCooldown.Start();

    //Parametros basicos del enemigo volador
    vision = 12;
    speed = 2.5f;
    knockbackForce = 5.0f;
    maxHealth = 20;
    currentHealth = 20;

    return true;
}

bool Bat::Update(float dt)
{
    if (!active) return true;
    ZoneScoped;

    // Lógica mientras está vivo
    if (Engine::GetInstance().sceneManager->isGamePaused == false && isdead == false)
    {
        if (pathFindingCooldown.ReadMSec() > 500)
        {
            PerformPathfinding();
            pathFindingCooldown.Start();
        }

        GetPhysicsValues();
        Move();
        Knockback();
        ApplyPhysics();
    }

    // Lógica al morir
    if (isdead )
    {
        // Se ejecuta solo una vez al morir
        if (anims.GetCurrentName() != "dead")
        {
            Engine::GetInstance().audio->PlayFx(morirBat);
            anims.SetCurrent("dead");
            anims.GetAnim("dead")->SetLoop(false);

            // Vuelve a tener gravedad para que caiga al suelo
            if (pbody != nullptr && !B2_IS_NULL(pbody->body)) {
                Engine::GetInstance().physics->SetGravityScale(pbody, 1.0f);
            }

            // Detiene el movimiento horizontal al morir (retiene la velocidad Y de caída)
            b2Vec2 currentVel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
            Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0.0f, currentVel.y });

            isKnockedback = false;
            pbody->ctype = ColliderType::UNKNOWN;
        }


        if (anims.GetAnim("dead")->HasFinishedOnce())
        {
            pendingToDelete = true;
        }
    }

    bool isWalking = (velocity.x != 0 && !isdead && !isKnockedback);

    if (isWalking && !wasWalking) {
        Engine::GetInstance().audio->PlayFx(volarBat, 99);
    }

    else if (!isWalking && wasWalking) {
        Engine::GetInstance().audio->StopFx(volarBat);
    }

    wasWalking = isWalking;

    Draw(dt);
    return true;
}
void Bat::GetPhysicsValues() {
    velocity = { 0.0f, 0.0f };
}

void Bat::PerformPathfinding()
{
    //Reset path
    pathfinding->ResetPath(GetTilePos());

    //Get the position of the enemy
    Vector2D pos = GetPosition();

    //Get the position of the player
    Player* player = Engine::GetInstance().entityManager->GetPlayer();
    Vector2D playerPos = player->GetPosition();
    playerTileDist = sqrt(pos.distanceSquared(playerPos)) / 128;
    int iter = 0;

    while (pathfinding->pathTiles.empty() && playerTileDist < vision && iter < MaxIterations)
    {
        pathfinding->PropagateAStar();
        iter++;
    }
}
void Bat::Move() {
    if (isdead || isKnockedback) return;

    Player* player = Engine::GetInstance().entityManager->GetPlayer();
    Vector2D playerPos = player->GetPosition();
    Vector2D myPos = GetPosition();
    float distToPlayer = (playerPos - myPos).magnitude();

    // Controla la dirección a la que mira el enemigo
    lookingRight = (playerPos.getX() > myPos.getX());

    // Calcula la posición objetivo (dónde debe detenerse/flotar)
    float dirX = (playerPos.getX() < myPos.getX()) ? 1.0f : -1.0f;
    Vector2D targetPos(playerPos.getX() + (dirX * targetOffsetX), playerPos.getY() - targetOffsetY);

    // Dirección de movimiento hacia la posición objetivo
    Vector2D moveDir = { 0.0f, 0.0f };
    bool usePathfinding = false;

    if (pathfinding != nullptr && !pathfinding->pathTiles.empty())
    {
       
        float euclideanDistInTiles = distToPlayer / 32.0f;
        int pathSize = pathfinding->pathTiles.size();

        // Si la ruta de cuadros verdes es mucho más larga que la línea recta, 
        // significa que hay una pared o un pasillo bloqueando el camino.
        bool isBlockedByWall = (pathSize > (euclideanDistInTiles * 1.5f + 3.0f));

        // Si el enemigo está atrapado en un muro, O está extremadamente lejos, usa los cuadros verdes
        if (isBlockedByWall || pathSize > 12) {
            usePathfinding = true;
        }
    }

    // Navegación
    if (usePathfinding)
    {
        Vector2D tilePos = GetTilePos();

        // Avanza consumiendo los cuadros verdes
        if (pathfinding->pathTiles.back() == tilePos) {
            pathfinding->pathTiles.pop_back();
        }

        if (!pathfinding->pathTiles.empty()) {
            Vector2D nextTile = pathfinding->pathTiles.back();
            Vector2D nextWorldPos = Engine::GetInstance().map->MapToWorld((int)nextTile.getX(), (int)nextTile.getY());

            // Apuntar al centro del Tile
            nextWorldPos.setX(nextWorldPos.getX() + 16.0f);
            nextWorldPos.setY(nextWorldPos.getY() + 16.0f);

            moveDir = (nextWorldPos - myPos).normalized();
        }
        else {
            moveDir = (targetPos - myPos).normalized();
        }
    }
    else
    {
        // Si no hay muros de por medio, el enemigo te tiene a la vista y vuela fluido a su zona de disparo
        moveDir = (targetPos - myPos).normalized();
    }
   
    //Estado
    switch (currentState) {
    case BatState::IDLE:
    {
        if (distToPlayer < vision * 32.0f) {
            currentState = BatState::CHASE;
        }
        break;
    }
    case BatState::CHASE:
    {
        anims.SetCurrent("fly");
        velocity.x = moveDir.getX() * speed;
        velocity.y = moveDir.getY() * speed;

        // Cuando entra en el rango de ataque, se prepara para disparar
        if (distToPlayer <= attackRange) {
            currentState = BatState::WINDUP;
            stateTimer.Start();
        }
        break;
    }
    case BatState::WINDUP:
    {
        // Mantiene el movimiento mientras carga el ataque
        velocity.x = moveDir.getX() * speed;
        velocity.y = moveDir.getY() * speed;

        // Finaliza el tiempo de carga
        if (stateTimer.ReadMSec() >= windupDurationMs) {
            currentState = BatState::ATTACK;
            anims.SetCurrent("attack");
        }
        break;
    }
    case BatState::ATTACK:
    {
        Engine::GetInstance().audio->PlayFx(atacarBat);
        ShootProjectile(); // Dispara la bala
        currentState = BatState::COOLDOWN;
        stateTimer.Start();
        break;
    }
    case BatState::COOLDOWN:
    {
        // Mantiene el movimiento hacia el jugador mientras espera para el siguiente disparo
        velocity.x = moveDir.getX() * speed;
        velocity.y = moveDir.getY() * speed;

        // Termina el enfriamiento y vuelve a perseguir
        if (stateTimer.ReadMSec() >= cooldownDurationMs) {
            currentState = BatState::CHASE;
        }
        break;
    }
    }
}
void Bat::ApplyPhysics() {
    Engine::GetInstance().physics->SetLinearVelocity(pbody, { velocity.x, velocity.y });
}

void Bat::Knockback()
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

void Bat::Draw(float dt)
{
    if (Engine::GetInstance().sceneManager->isGamePaused == false) {
        
        anims.Update(dt);
    }
    const SDL_Rect& animFrame = anims.GetCurrentFrame();

    int x, y;
    pbody->GetPosition(x, y);
    position.setX((float)x);
    position.setY((float)y);

    // Invierte (voltea) la textura del enemigo según su dirección
    SDL_FlipMode sdlFlip = lookingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;

    if (Engine::GetInstance().physics->GetDebug())
    {
        pathfinding->DrawPath();
    }

    //Draw using the texture and the current animation frame
    if (isKnockedback)
    {
        Uint8* r = new Uint8; Uint8* g = new Uint8; Uint8* b = new Uint8;
        Engine::GetInstance().render->SetColorMod(texture, r, g, b, 255, 25, 25);

        Engine::GetInstance().render->DrawRotatedTexture(texture, x, y - animFrame.h / 8, &animFrame, sdlFlip, 0.75);

        Engine::GetInstance().render->SetColorMod(texture, nullptr, nullptr, nullptr, *r, *g, *b);
        delete r; delete g; delete b;
    }
    else
    {
        Engine::GetInstance().render->DrawRotatedTexture(texture, x, y - animFrame.h / 8, &animFrame, sdlFlip, 0.75);
    }
}

void Bat::ShootProjectile() {
    Vector2D spawnPos = GetPosition();
    // Genera la bala un poco por delante del enemigo volador para que no colisione consigo mismo
    spawnPos.setX(lookingRight ? spawnPos.getX() + 20.0f : spawnPos.getX() - 20.0f);

    std::shared_ptr<HomingProjectile> bullet = std::make_shared<HomingProjectile>(spawnPos);
    bullet->Start();
    Engine::GetInstance().entityManager->AddEntity(bullet);
}

bool Bat::CleanUp() {
    // Libera la textura del enemigo
    if (texture != nullptr) {
        Engine::GetInstance().textures->UnLoad(texture);
        texture = nullptr;
    }

    return Enemy::CleanUp();
}

void Bat::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
    // Si está muerto, no hace daño ni recibe más golpes
    if (isdead) return;

    switch (physB->ctype)
    {
    case ColliderType::PLAYER_ATTACK:
        TakeDamage(physB->listener->damage);
        isKnockedback = true;
        knockbackTime = 500.0f;
        Engine::GetInstance().particleManager->EmitHitSparks(position.getX(), position.getY(), false);

        break;

    default:
        break;
    }
}