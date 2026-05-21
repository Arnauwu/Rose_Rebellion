#include "WallJumpObj.h"
#include "Engine.h"
#include "Textures.h"
#include "EntityManager.h"
#include "Physics.h"
#include "Player.h"

WallJumpObj::WallJumpObj() : Item() {
    name = "WallJumpObj";
}

WallJumpObj::~WallJumpObj() {}

bool WallJumpObj::Awake() {
    return true;
}

bool WallJumpObj::Start() {
    if (CheckIfCollected()) return true;

    texture = Engine::GetInstance().textures->Load("Assets/Textures/UI/Items/wallJumpUI.png");

    pbody = Engine::GetInstance().physics->CreateCircleSensor(
        (int)position.getX(), (int)position.getY(), texture->h / 2, bodyType::KINEMATIC);
    pbody->listener = this;
    pbody->ctype = ColliderType::ITEM;

    return true;
}

bool WallJumpObj::Update(float dt) {
    if (!isPicked) {
        int x, y;
        pbody->GetPosition(x, y);
        Engine::GetInstance().render->DrawTexture(texture, x - texture->w / 2, y - texture->h / 2);
    }
    return true;
}

bool WallJumpObj::CleanUp() {
    Engine::GetInstance().textures->UnLoad(texture);
    if (pbody != nullptr) {
        Engine::GetInstance().physics->DeletePhysBody(pbody);
        pbody = nullptr;
    }
    return true;
}

void WallJumpObj::OnCollision(PhysBody* physA, PhysBody* physB) {
    if (physB->ctype == ColliderType::PLAYER) {
        Player* player = (Player*)physB->listener;
        player->UnlockWallJump();
        SetCollected();
        isPicked = false;
    }
}