#include "Sickle.h"
#include "Engine.h"
#include "Textures.h"
#include "EntityManager.h"
#include "Physics.h"
#include "Player.h"

Sickle::Sickle() : Item() {
    name = "Sickle";
}

Sickle::~Sickle() {}

bool Sickle::Awake() {
    return true;
}

bool Sickle::Start() {
    if (CheckIfCollected()) return true;
    texture = Engine::GetInstance().textures->Load("Assets/Textures/Items/Sickle/Sickle.png");
    interactionBackgroundTexture = Engine::GetInstance().textures->Load("Assets/Textures/UI/Tutorial/SS_FondoTexto_Interaccion.png");
    std::unordered_map<int, std::string> interactionBackgroundAliases = { {0, "idle"} };
    interactionBackgroundAnimation.LoadFromTSX(
        "Assets/Textures/UI/Tutorial/SS_FondoTexto_Interaccion.tsx",
        interactionBackgroundAliases
    );
    interactionBackgroundAnimation.SetCurrent("idle");

   
    pbody = Engine::GetInstance().physics->CreateCircleSensor((int)position.getX(), (int)position.getY(), texture->h / 2, bodyType::KINEMATIC);
    pbody->listener = this;
    pbody->ctype = ColliderType::ITEM;

    return true;
}

bool Sickle::Update(float dt) {
    if (!isPicked) {
        int x, y;
        pbody->GetPosition(x, y);
        interactionBackgroundAnimation.Update(dt);
        const SDL_Rect& backgroundFrame = interactionBackgroundAnimation.GetCurrentFrame();
        const int backgroundW = (int)(texture->w * 1.25f);
        const int backgroundH = (int)(texture->h * 1.25f);
        SDL_Rect backgroundRect = {
            x - backgroundW / 2,
            y - backgroundH / 2,
            backgroundW,
            backgroundH
        };
        Engine::GetInstance().render->DrawWorldTextureScaledSection(
            interactionBackgroundTexture, backgroundFrame, backgroundRect);
        Engine::GetInstance().render->DrawTexture(texture, x - texture->w / 2, y - texture->h / 2);
    }
    return true;
}

bool Sickle::CleanUp() {
    Engine::GetInstance().textures->UnLoad(texture);
    Engine::GetInstance().textures->UnLoad(interactionBackgroundTexture);
    if (pbody != nullptr) {
        Engine::GetInstance().physics->DeletePhysBody(pbody);
        pbody = nullptr;
    }
    return true;
}

void Sickle::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) {
    if (physB->ctype == ColliderType::PLAYER) {
        Player* player = (Player*)physB->listener;
        player->UnlockSickle();
        SetCollected();
        isPicked = true;
    }
}