#include "WaveProjectile.h"
#include "Engine.h"
#include "Textures.h"
#include "SceneManager.h"
#include "EntityManager.h"
#include "Player.h"
#include "Render.h"
#include "Log.h"
#include "tracy/Tracy.hpp"
#include <cmath> // Necesario para calcular colisiones manualmente

// 1. Constructores
WaveProjectile::WaveProjectile() : Entity(EntityType::WAVE) {
    name = "Wave";
    lookingRight = true;
    speed = 6.0f;
    damage = 10;
}

WaveProjectile::WaveProjectile(Vector2D spawnPos) : Entity(EntityType::WAVE) {
    name = "Wave";
    position = spawnPos;
    lookingRight = true;
    speed = 6.0f;
    damage = 10;
    active = true;
}

WaveProjectile::~WaveProjectile() {}

// 2. Start
bool WaveProjectile::Start() {
    std::unordered_map<int, std::string> aliases = {
        {0, "bullet"}
    };
    anims.LoadFromTSX("Assets/Textures/Entities/Enemies/Ninfa/ninfa_projectile.tsx", aliases);
    anims.SetCurrent("bullet");

    texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/Ninfa/ninfa_projectile.png");

    // IMPORTANTE: No creamos 'pbody'. Al no tener cuerpo físico, 
    // Box2D lo ignorará por completo. ¡Es un fantasma que no empuja a nadie!
    pbody = nullptr;

    return true;
}

// 3. Update (Movimiento y colisión matemática)
bool WaveProjectile::Update(float dt) {
    if (!active || pendingToDelete) return true;
    ZoneScoped;

    // A. Mover la ola manualmente hacia la derecha
    position.setX(position.getX() + speed);

    Player* player = Engine::GetInstance().entityManager->GetPlayer();
    if (player != nullptr) {
        Vector2D playerPos = player->GetPosition();

        // Calculamos la distancia entre la ola y el jugador
        float distX = std::abs(playerPos.getX() - position.getX());
        float distY = std::abs(playerPos.getY() - position.getY());

        // AUMENTAMOS ESTOS VALORES:
        // distX < 60.0f para que te dé si te roza de lado
        // distY < 100.0f para que te dé independientemente de si va por arriba o por abajo
        if (distX < 60.0f && distY < 100.0f) {

            // Opcional: imprimir en consola para asegurarte de que entra
            LOG("¡Ola golpea al jugador!");

            player->TakeDamage(damage); // Aplica el daño
            Destroy();                  // Destruye la ola al golpear
        }
    }

    // C. (Opcional) Autodestruir si se aleja mucho para no sobrecargar memoria
    if (player != nullptr && (position.getX() - player->GetPosition().getX()) > 2000.0f) {
        Destroy();
    }

    Draw(dt);
    return true;
}

// 4. OnCollision (Vacío porque ya no usamos físicas)
void WaveProjectile::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) {
    // No hace falta nada aquí
}

// 5. Draw
void WaveProjectile::Draw(float dt) {
    if (!Engine::GetInstance().sceneManager->isGamePaused) {
        anims.Update(dt);
    }
    const SDL_Rect& animFrame = anims.GetCurrentFrame();

    // Como no hay Box2D, pintamos directamente en nuestra 'position'
    int x = (int)position.getX();
    int y = (int)position.getY();

    SDL_FlipMode sdlFlip = lookingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;

    Engine::GetInstance().render->DrawRotatedTexture(texture, x, y, &animFrame, sdlFlip, 1, 0, animFrame.w / 2, animFrame.h / 2);
}

// 6. GetPosition
Vector2D WaveProjectile::GetPosition() {
    return position; // Leemos directo de la variable, no del pbody
}

// 7. CleanUp
bool WaveProjectile::CleanUp() {
    active = false;

    if (texture != nullptr) {
        Engine::GetInstance().textures->UnLoad(texture);
        texture = nullptr;
    }

    // Ya no hay físicas que borrar, así que quitamos la parte del physics->DeletePhysBody
    pbody = nullptr;

    return Entity::CleanUp();
}