#include "KeyGate.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Player.h"
#include "EntityManager.h"
#include "GameManager.h"
#include "Log.h"
#include "tracy/Tracy.hpp"
#include <algorithm>

KeyGate::KeyGate() : Entity(EntityType::KEY_GATE)
{
    name = "KeyGate";
}

KeyGate::~KeyGate() {}

bool KeyGate::Awake() { return true; }

bool KeyGate::Start()
{
    auto textures = Engine::GetInstance().textures;

    closedTexture = textures->Load("Assets/Textures/Animation/Door/SS_puerta_catacumbas_primero.png");
    openTexture = textures->Load("Assets/Textures/Animation/Door/SS_puerta_catacumbas_final.png");
    animTexture = textures->Load("Assets/Textures/Animation/Door/SS_puerta_catacumbas.png");

    std::unordered_map<int, std::string> aliases = { {0, "open"} };
    bool loadOK = anims.LoadFromTSX("Assets/Textures/Animation/Door/SS_puerta_catacumbas.tsx", aliases);

    if (loadOK && anims.Has("open")) {
        anims.GetAnim("open")->SetLoop(false);
    }

    if (!gateID.empty()) {
        auto& opened = GameManager::GetInstance().gameState.openedDoors;
        if (std::find(opened.begin(), opened.end(), gateID) != opened.end()) {
            state = GateState::OPEN;
        }
    }

    return true;
}

void KeyGate::Initialize(Vector2D pos, int width, int height, KeyType key, std::string id)
{
    position = pos;  
    gateW = width;
    gateH = height;
    requiredKey = key;
    gateID = id;
}

void KeyGate::OpenGate()
{
    
    if (state != GateState::CLOSED) return;
    animationFinished = false;
    
    state = GateState::OPENING;
    if (anims.Has("open")) {
        anims.SetCurrent("open");
        anims.GetAnim("open")->Reset();
    }

    GameManager::GetInstance().gameState.openedDoors.push_back(gateID);
}

bool KeyGate::Update(float dt)
{
    ZoneScoped;

    SDL_Rect destRect;
    destRect.x = (int)(position.getX() - gateW / 2.0f);
    destRect.y = (int)(position.getY() + gateH / 2.0f);
    destRect.w = gateW;
    destRect.h = gateH;

    if (state == GateState::OPENING)
    {
        anims.Update(dt);
        SDL_Rect frame = anims.GetCurrentFrame();

        if (animTexture) {
            Engine::GetInstance().render->DrawRotatedImage(animTexture, &destRect, &frame);
        }

        if (!animationFinished &&
            anims.GetAnim("open")->HasFinishedOnce())
        {
            animationFinished = true;

            state = GateState::OPEN;

            auto player = Engine::GetInstance().entityManager->GetPlayer();
            if (player) player->isFrozen = false;
        }
    }
    else if (state == GateState::OPEN)
    {
        if (openTexture) Engine::GetInstance().render->DrawRotatedImage(openTexture, &destRect, nullptr);
    }
    else if (state == GateState::CLOSED)
    {
        if (closedTexture) Engine::GetInstance().render->DrawRotatedImage(closedTexture, &destRect, nullptr);
    }

    return true;
}

bool KeyGate::CleanUp()
{
    auto textures = Engine::GetInstance().textures;
    if (animTexture) textures->UnLoad(animTexture);
    if (closedTexture) textures->UnLoad(closedTexture);
    if (openTexture) textures->UnLoad(openTexture);
    return true;
}