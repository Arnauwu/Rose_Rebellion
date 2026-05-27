#include "WaveProjectile.h"

#include "Engine.h"
#include "Physics.h"
#include "Textures.h"
#include "SceneManager.h"
#include "EntityManager.h"

//#include "Audio.h"
#include "Render.h"
#include "Log.h"



#include "Render.h"

#include "tracy/Tracy.hpp"

WaveProjectile::WaveProjectile() : Entity(EntityType::WAVE)
{
    name = "Wave";

    speed = 6.0f;         // Movement Speed
    damage = 10;          // Damage
}

WaveProjectile::~WaveProjectile() {}

bool WaveProjectile::Start()
{
    //TODO: WAVE SPRITE
    std::unordered_map<int, std::string> aliases = {
        {0, "bullet"}
    };
    anims.LoadFromTSX("Assets/Textures/Entities/Enemies/Ninfa/ninfa_projectile.tsx", aliases);
    anims.SetCurrent("bullet");


    // Textura temporal
    texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/Ninfa/ninfa_projectile.png");

    //Fisica
    int height = 20, width = 10;
    

    pbody = Engine::GetInstance().physics->CreateRectangle((int)position.getX(), (int)position.getY(), width, height, bodyType::DYNAMIC);

    pbody->listener = this;
    pbody->ctype = ColliderType::ENEMY_ATTACK;

    // Elimina la gravedad para que la bala no caiga mientras vuela
    if (pbody != nullptr && !B2_IS_NULL(pbody->body)) {
        Engine::GetInstance().physics->SetGravityScale(pbody, 0.0f);
    }


    return true;
}

bool WaveProjectile::Update(float dt)
{
    if (!active || pendingToDelete) return true;
    ZoneScoped;



    // Read current velocity
    velocity.setY(Engine::GetInstance().physics->GetLinearVelocity(pbody).y);
    
    if (lookingRight)
    {
        velocity.setX(speed);
    }
    else
    {
        velocity.setX(-speed);
    }

    Engine::GetInstance().physics->SetLinearVelocity(pbody, { velocity.getX(), velocity.getY() });

    Draw(dt);
    return true;
}

void WaveProjectile::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
    if (pendingToDelete) return;

    switch (physB->ctype)
    {
    case ColliderType::PLAYER:
        if (physB->listener != nullptr) {
            physB->listener->TakeDamage(damage);
        }
        Destroy();
        break;
    case ColliderType::MAP:
        Destroy();
        break;
    case ColliderType::ENEMY:
    default:
        break;
    }
}

void WaveProjectile::Draw(float dt)
{
    if (Engine::GetInstance().sceneManager->isGamePaused == false) {

        anims.Update(dt);
    }
    const SDL_Rect& animFrame = anims.GetCurrentFrame();

    int x, y;
    pbody->GetPosition(x, y);
    position.setX((float)x);
    position.setY((float)y);

    // Flip Sprite
    SDL_FlipMode sdlFlip = lookingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;

    Engine::GetInstance().render->DrawRotatedTexture(texture, x, y, &animFrame, sdlFlip, 1, 0, animFrame.w / 2, animFrame.h / 2);
}

Vector2D WaveProjectile::GetPosition()
{
    int x, y;
    pbody->GetPosition(x, y);
    return Vector2D((float)x, (float)y);
}

bool WaveProjectile::CleanUp()
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