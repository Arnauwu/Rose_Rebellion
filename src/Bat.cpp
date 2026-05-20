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
    name = "Bat";
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
        {0, "sleep"}, {4, "fly"}, {8, "dead"}
    };
    anims.LoadFromTSX("Assets/Textures/Entities/Enemies/Bat/SS_Murcielago.tsx", aliases);
    anims.SetCurrent("fly");
 
    texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/Bat/SS_Murcielago.png");
   
   // Physic Body
    texW = 256; 
    texH = 256;
    pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX(), (int)position.getY(), texW / 3, bodyType::DYNAMIC);
    pbody->listener = this;

    pbody->ctype = ColliderType::ENEMY_ATTACK;
    damage = 10;

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
    vision = 15;
    speed = 5.0f;
    knockbackForce = 5.0f;
    maxHealth = 100;
    currentHealth = 50;

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

    Vector2D tilePos = GetTilePos();


    // Move if player has been found
    if (pathfinding->pathTiles.empty()) {
        velocity.x = 0;
        return;
    }
    else
    {
        if (pathfinding->pathTiles.back() == tilePos)
        {
            pathfinding->pathTiles.pop_back();
            if (pathfinding->pathTiles.empty()) { return; }
        }

        Vector2D nextTile = pathfinding->pathTiles.back();

        if (nextTile.getX() > tilePos.getX())
        {
            velocity.x = speed;
            lookingRight = true;
        }
        else if (nextTile.getX() < tilePos.getX())
        {
            velocity.x = -speed;
            lookingRight = false;
        }
        else
        {
            velocity.x = 0;
        }

        if (nextTile.getY() > tilePos.getY())
        {
            velocity.y = speed;
        }
        else if (nextTile.getY() < tilePos.getY())
        {
            velocity.y = -speed;
        }
        else
        {
            velocity.y = 0;
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

        Engine::GetInstance().render->DrawRotatedTexture(texture, x, y - animFrame.h / 8, &animFrame, sdlFlip, 1);

        Engine::GetInstance().render->SetColorMod(texture, nullptr, nullptr, nullptr, *r, *g, *b);
        delete r; delete g; delete b;
    }
    else
    {
        Engine::GetInstance().render->DrawRotatedTexture(texture, x, y - animFrame.h / 8, &animFrame, sdlFlip, 1);
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
    case ColliderType::PLAYER:
        if (currentHealth > maxHealth)
        {
            currentHealth += 20;
        }
        break;
    default:
        break;
    }
}