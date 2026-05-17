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

    if (texture == nullptr) {
        texture = Engine::GetInstance().textures->Load("Assets/Textures/Items/Keys/castleKey.png");
    }

    // Fisic
    if (pbody == nullptr && texture != nullptr) {
        pbody = Engine::GetInstance().physics->CreateCircleSensor((int)position.getX(), (int)position.getY(), texture->h / 2, bodyType::KINEMATIC);
        pbody->listener = this;
        pbody->ctype = ColliderType::ITEM;
    }

    return true;
}

bool Keys::Update(float dt) {
    if (!isPicked && pbody != nullptr && texture != nullptr) {
        int x, y;
        pbody->GetPosition(x, y);
        Engine::GetInstance().render->DrawTexture(texture, x - texture->w / 2, y - texture->h / 2);
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
    std::string texPath = "";

    switch (type) {
    case KeyType::FOREST:
        texPath = "Assets/Textures/Items/Keys/forestKey.png"; 
        break;
    case KeyType::MOUNTAIN:
        texPath = "Assets/Textures/Items/Keys/mountainKey.png";
        break;
    case KeyType::CATACUMBA:
        texPath = "Assets/Textures/Items/Keys/catacumbsKey.png";
        break;
    case KeyType::BOSS:
    case KeyType::CASTLE:
        texPath = "Assets/Textures/Items/Keys/castleKey.png";
        break;
    default:
        texPath = "Assets/Textures/Items/Keys/castleKey.png";
        break;
    }

    //Cleanup
    if (texture != nullptr) {
        Engine::GetInstance().textures->UnLoad(texture);
    }
    texture = Engine::GetInstance().textures->Load(texPath.c_str());
    LOG("Textura de llave actualizada a: %s", texPath.c_str());
}

KeyType Keys::GetKeyType() const {
    return keyType;
}