#include "DragonProjectile.h"
#include <cmath>
#include <SDL3/SDL.h>

#include "Engine.h"
#include "Physics.h"
#include "Textures.h"
#include "SceneManager.h"
#include "EntityManager.h"

#include "Render.h"

#include "tracy/Tracy.hpp"

//PI
constexpr double PI = 3.14159265358979323846;

DragonProjectile::DragonProjectile() : Entity(EntityType::DRAGON_PROJECTILE)
{
    name = "DragonProjectile";

    speed = 12.0f;         // Velocidad de la bala
    damage = 10;          // Daño que causa la bala
    lifeTimeMS = 6000.0f; // Tiempo antes de la autodestrucción (4s)
}

DragonProjectile::~DragonProjectile() {}

bool DragonProjectile::Start()
{
    // Textures & Anims
    std::unordered_map<int, std::string> aliases = {
        {0, "bullet"}
    };
    anims.LoadFromTSX("Assets/Textures/Entities/Enemies/Ninfa/ninfa_projectile.tsx", aliases);
    anims.SetCurrent("bullet");

    texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/Ninfa/ninfa_projectile.png");

    //Physics
    int radius = 12;
    pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX(), (int)position.getY(), radius, bodyType::DYNAMIC);

    pbody->listener = this;
    pbody->ctype = ColliderType::ENEMY_ATTACK;

    Engine::GetInstance().physics->SetGravityScale(pbody, 0.0f);

    // Obtiene la posición del jugador para lanzar la bala en esa dirección
    Player* player = Engine::GetInstance().entityManager->GetPlayer();
    Vector2D playerPos = player->position;
    Vector2D dir = (playerPos - position).normalized();
    currentVelocity = dir * speed; // Dirección multiplicada por la velocidad

    // Iniciar temporizador
    lifeTimer.Start();
    return true;
}

bool DragonProjectile::Update(float dt)
{
    if (!active || pendingToDelete) return true;
    ZoneScoped;

    // La bala se autodestruye cuando el contador llega a 4s
    if (lifeTimer.ReadMSec() >= lifeTimeMS) {
        Destroy();
        return true;
    }

    //Mecánica de movimiento : mantiene la velocidad y dirección constantes tras ser lanzada
    Engine::GetInstance().physics->SetLinearVelocity(pbody, { currentVelocity.getX(), currentVelocity.getY() });

    Draw(dt);
    return true;
}

void DragonProjectile::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
    if (pendingToDelete) return;

    switch (physB->ctype)
    {
        // Hacer daño al player
    case ColliderType::PLAYER:
        if (physB->listener != nullptr) {
            physB->listener->TakeDamage(damage);
        }
        Destroy();
        break;
        // El jugador puede destruir/desviar la bala mediante un ataque
    case ColliderType::PLAYER_ATTACK:
        Destroy();
        break;

        // Si choca contra el escenario, se destruye
    case ColliderType::MAP:
        Destroy();
        break;
    default:
        break;
    }
}

void DragonProjectile::Draw(float dt)
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


    // Girar la textura para que apunte hacia la dirección de movimiento
    double angle = std::atan2(currentVelocity.getY(), currentVelocity.getX()) * (180.0 / PI);

    Engine::GetInstance().render->DrawRotatedTexture(texture, x, y, &animFrame, sdlFlip, 1, angle, animFrame.w / 2, animFrame.h / 2);
}

Vector2D DragonProjectile::GetPosition()
{
    int x, y;
    pbody->GetPosition(x, y);
    return Vector2D((float)x, (float)y);
}

bool DragonProjectile::CleanUp()
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