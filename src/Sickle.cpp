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

   
    pbody = Engine::GetInstance().physics->CreateCircleSensor((int)position.getX(), (int)position.getY(), texture->h / 2, bodyType::KINEMATIC);
    pbody->listener = this;
    pbody->ctype = ColliderType::ITEM;

    return true;
}

bool Sickle::Update(float dt) {
    if (!isPicked) {
        int x, y;
        pbody->GetPosition(x, y);
        Engine::GetInstance().render->DrawTexture(texture, x - texture->w / 2, y - texture->h / 2);
    }
    return true;
}

bool Sickle::CleanUp() {
    Engine::GetInstance().textures->UnLoad(texture);
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