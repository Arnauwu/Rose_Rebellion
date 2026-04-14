#include "HomingProjectile.h"
#include <cmath>
#include <SDL3/SDL.h>

#include "Engine.h"
#include "Physics.h"
#include "Textures.h"
#include "SceneManager.h"
#include "Render.h"

//PI
constexpr double PI = 3.14159265358979323846;

HomingProjectile::HomingProjectile(Vector2D startPos) : Entity(EntityType::ENEMY_PROJECTILE)
{
    position = startPos;
    name = "HomingProjectile";

    speed = 6.0f;         // Velocidad de la bala
    damage = 10;          // Daño de la bala
    lifeTimeMS = 4000.0f; // AutoDestruye (4s)
}

HomingProjectile::~HomingProjectile() {}

bool HomingProjectile::Start()
{
   //Textura
    texture = Engine::GetInstance().textures->Load("Assets/Textures/player1.png");

   //Fisica
    int radius = 6; // El área de impacto de las balas es más pequeña, lo que da a los jugadores más margen para esquivarlas.
    pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX(), (int)position.getY(), radius, bodyType::DYNAMIC);

    pbody->listener = this;
    pbody->ctype = ColliderType::ENEMY_ATTACK;

    // Elimina la fisica para que la bala no cae cuando esta volando
    if (pbody != nullptr && !B2_IS_NULL(pbody->body)) {
        Engine::GetInstance().physics->SetGravityScale(pbody, 0.0f);
    }

    // Obtiene la posicion del player para que la bala lanza hacia esa direccion
    Vector2D playerPos = Engine::GetInstance().sceneManager->GetPlayerPosition();
    Vector2D dir = (playerPos - position).normalized();
    currentVelocity = dir * speed; // Direccion y la velocidad

    //Contar 
    lifeTimer.Start();
    return true;
}

bool HomingProjectile::Update(float dt)
{
    if (!active || pendingToDelete) return true;

    // La bala autodestruye cuando contador llega 4ss
    if (lifeTimer.ReadMSec() >= lifeTimeMS) {
        Destroy();
        return true;
    }

    // ==========================================
    // Mecanica del movimiento de la bala, no cambia la velocidad y direccion despues de lanzar
    // ==========================================
    Engine::GetInstance().physics->SetLinearVelocity(pbody, { currentVelocity.getX(), currentVelocity.getY() });

    Draw(dt);
    return true;
}

void HomingProjectile::OnCollision(PhysBody* physA, PhysBody* physB)
{
    if (pendingToDelete) return;

    switch (physB->ctype)
    {
    //Hacer daño al player
    case ColliderType::PLAYER:
        if (physB->listener != nullptr) {
            physB->listener->TakeDamage(damage);
        }
        Destroy();
        break;
    //Player puede defenter bala atravez un ataque
    case ColliderType::PLAYER_ATTACK:
        Destroy();
        break;
    case ColliderType::WALL:
    case ColliderType::GROUND:
    //Si choca a un pared destruye
    case ColliderType::CEILING:
        Destroy();
        break;
    case ColliderType::ENEMY:
        break;
    default:
        break;
    }
}

void HomingProjectile::Draw(float dt)
{
    int x, y;
    pbody->GetPosition(x, y);

    // Girar la textura de la bala para que la cabeza del textura hacia la direccion que va lanzar la bala
    double angle = std::atan2(currentVelocity.getY(), currentVelocity.getX()) * (180.0 / PI);

    Engine::GetInstance().render->DrawRotatedTexture(texture, x - 8, y - 8, nullptr, SDL_FLIP_NONE, 1.0, angle);
}

Vector2D HomingProjectile::GetPosition()
{
    int x, y;
    pbody->GetPosition(x, y);
    return Vector2D((float)x, (float)y);
}

bool HomingProjectile::CleanUp()
{
    active = false;

    //Elimina la textura
    if (texture != nullptr) {
        Engine::GetInstance().textures->UnLoad(texture);
        texture = nullptr;
    }

    // Elimina la fisica
    if (pbody != nullptr) {
        Engine::GetInstance().physics->DeletePhysBody(pbody);
        pbody = nullptr; 
    }

    return Entity::CleanUp();
}