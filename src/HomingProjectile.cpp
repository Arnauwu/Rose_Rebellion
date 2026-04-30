#include "HomingProjectile.h"
#include <cmath>
#include <SDL3/SDL.h>

#include "Engine.h"
#include "Physics.h"
#include "Textures.h"
#include "SceneManager.h"
#include "EntityManager.h"

#include "Render.h"

//PI
constexpr double PI = 3.14159265358979323846;

HomingProjectile::HomingProjectile(Vector2D startPos) : Entity(EntityType::ENEMY_PROJECTILE)
{
    position = startPos;
    name = "HomingProjectile";

    speed = 6.0f;         // Velocidad de la bala
    damage = 10;          // Daño que causa la bala
    lifeTimeMS = 4000.0f; // Tiempo antes de la autodestrucción (4s)
}

HomingProjectile::~HomingProjectile() {}

bool HomingProjectile::Start()
{
    //Enemigo volador sprite

    std::unordered_map<int, std::string> aliases = {
        {0, "bullet"}
    };
    anims.LoadFromTSX("Assets/Textures/Entities/Enemies/Ninfa/ninfa_projectile.tsx", aliases);
    anims.SetCurrent("bullet");


    // Textura temporal para pruebas (testear)
    texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/Ninfa/ninfa_projectile.png");

   //Fisica
    int radius = 6; // El área de impacto de las balas es más pequeña, lo que da a los jugadores más margen para esquivarlas.
    pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX(), (int)position.getY(), radius, bodyType::DYNAMIC);

    pbody->listener = this;
    pbody->ctype = ColliderType::ENEMY_ATTACK;

    // Elimina la gravedad para que la bala no caiga mientras vuela
    if (pbody != nullptr && !B2_IS_NULL(pbody->body)) {
        Engine::GetInstance().physics->SetGravityScale(pbody, 0.0f);
    }

    // Obtiene la posición del jugador para lanzar la bala en esa dirección
    Player* player = Engine::GetInstance().entityManager->GetPlayer();
    Vector2D playerPos = player->position;
    Vector2D dir = (playerPos - position).normalized();
    currentVelocity = dir * speed; // Dirección multiplicada por la velocidad

    // Iniciar temporizador
    lifeTimer.Start();
    return true;
}

bool HomingProjectile::Update(float dt)
{
    if (!active || pendingToDelete) return true;

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

void HomingProjectile::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
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
    case ColliderType::ENEMY:
        break;
    default:
        break;
    }
}

void HomingProjectile::Draw(float dt)
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

    Engine::GetInstance().render->DrawRotatedTexture(texture, x, y, &animFrame, sdlFlip, 1, angle, animFrame.w/2, animFrame.h/2);
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