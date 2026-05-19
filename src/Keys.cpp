#include "Keys.h"
#include "Engine.h"
#include "Animation.h"
#include "Textures.h"
#include "Player.h"
#include "Physics.h"
#include "Log.h"

Keys::Keys() :Item() {
    name = "Key";
}

Keys::~Keys() {}

bool Keys::Awake() {
    return true;
}

bool Keys::Start() {
    if (CheckIfCollected()) return true;

    std::unordered_map<int, std::string> aliases = {
    {0, "ForestKey"}, {8, "CastleKey"}, {16, "MountainKey"},{24, "CatacumbsKey"}
    };
    anims.LoadFromTSX("Assets/Textures/Items/Keys/SS_obj_llaves.tsx", aliases);
    anims.SetCurrent("CastleKey");

    texture = Engine::GetInstance().textures->Load("Assets/Textures/Items/Keys/SS_obj_llaves.png");
   
    const SDL_Rect& animFrame = anims.GetCurrentFrame();
   
    // Fisic
    if (pbody == nullptr && texture != nullptr) {
        pbody = Engine::GetInstance().physics->CreateCircleSensor((int)position.getX(), (int)position.getY(), animFrame.w / 2, bodyType::KINEMATIC);
        pbody->listener = this;
        pbody->ctype = ColliderType::ITEM;
    }

    return true;
}

bool Keys::Update(float dt) {

    if (!isPicked && pbody != nullptr && texture != nullptr) {
        anims.Update(dt);
        const SDL_Rect& animFrame = anims.GetCurrentFrame();
        int x, y;
        pbody->GetPosition(x, y);
        Engine::GetInstance().render->DrawTexture(
            texture,
            x - animFrame.w / 2,
            y - animFrame.h / 2,
            &animFrame
            );
    }
    return true;
}

bool Keys::CleanUp() {
    if (texture != nullptr) {
        Engine::GetInstance().textures->UnLoad(texture);
        texture = nullptr;
    }
    if (pbody != nullptr) {
        Engine::GetInstance().physics->DeletePhysBody(pbody);
        pbody = nullptr;
    }
    return true;
}

void Keys::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) {
    if (physB->ctype == ColliderType::PLAYER && !isPicked) {
        SetCollected();

        Player* player = (Player*)physB->listener;
        if (player != nullptr) {
            player->AddKey(this->keyType);
            LOG("Jugador recogió una llave tipo: %d", (int)this->keyType);
        }
    }
}

//Key texture 
void Keys::SetKeyType(KeyType type) {
    this->keyType = type;
   /* std::string texPath = "";*/

    switch (type) {
    case KeyType::FOREST:
        anims.SetCurrent("ForestKey");   
        break;
    case KeyType::MOUNTAIN:
        anims.SetCurrent("MountainKey");
        break;
    case KeyType::CATACUMBA:
        anims.SetCurrent("CatacumbsKey");
        break;
    case KeyType::BOSS:
    case KeyType::CASTLE:
        anims.SetCurrent("CastleKey");
        break;
    default:
        anims.SetCurrent("CastleKey");
        break;
    }
}

KeyType Keys::GetKeyType() const {
    return keyType;
}