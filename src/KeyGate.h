#pragma once
#include "Entity.h"
#include "Animation.h"
#include "Vector2D.h"
#include "Map.h" 
#include <string>

struct SDL_Texture;
class PhysBody;

enum class GateState {
    CLOSED,
    OPENING,
    OPEN
};

class KeyGate : public Entity
{
public:
    KeyGate();
    virtual ~KeyGate();

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;


    void Initialize(Vector2D pos, int width, int height, KeyType key, std::string id);
    void SetCollider(PhysBody* collider);
    void OpenGate();

    KeyType requiredKey = KeyType::NONE;
    std::string gateID = "";
    GateState state = GateState::CLOSED;

private:
    SDL_Texture* closedTexture = nullptr;
    SDL_Texture* animTexture = nullptr;
    SDL_Texture* openTexture = nullptr;

    AnimationSet anims;

    int gateW = 0;
    int gateH = 0;
    PhysBody* gateCollider = nullptr;
    bool animationFinished = false;
};
