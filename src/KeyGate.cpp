#include "KeyGate.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Player.h"
#include "EntityManager.h"
#include "GameManager.h"
#include "Log.h"
#include "Physics.h"
#include "tracy/Tracy.hpp"
#include <algorithm>

namespace
{
    constexpr int GateVisualOverlap = 12;
    constexpr float ForestStaticFrameWidth = 216.0f;
    constexpr float ForestAnimationFrameWidth = 256.0f;

    void TrimForestAnimationFrame(SDL_Rect& frame)
    {
        const int tileId = (frame.y / 386) * 5 + (frame.x / 256);

        int trimTop = 0;
        int visibleHeight = 385;
        switch (tileId) {
        case 3: trimTop = 2; visibleHeight = 383; break;
        case 4: visibleHeight = 384; break;
        case 5:
        case 6:
        case 7:
        case 8:
        case 9: trimTop = 62; visibleHeight = 324; break;
        default: break;
        }

        frame.y += trimTop;
        frame.h = visibleHeight;
    }

    void TrimMountainAnimationFrame(SDL_Rect& frame)
    {
        const int tileId = (frame.y / 440) * 8 + (frame.x / 256);

        int trimTop = 0;
        int visibleHeight = 384;
        switch (tileId) {
        case 0: trimTop = 1; visibleHeight = 383; break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5: trimTop = 2; visibleHeight = 383; break;
        case 8: trimTop = 9; visibleHeight = 383; break;
        case 9:
        case 10: trimTop = 10; visibleHeight = 383; break;
        case 11:
        case 12: trimTop = 7; visibleHeight = 386; break;
        default: break;
        }

        frame.y += trimTop;
        frame.h = visibleHeight;
    }

    struct GateVisualAssets
    {
        const char* closedTexture;
        const char* openTexture;
        const char* animationTexture;
        const char* animationData;
    };

    GateVisualAssets GetGateVisualAssets(KeyType keyType)
    {
        switch (keyType) {
        case KeyType::FOREST:
            return {
                "Assets/Textures/Animation/Door/puertaBOSQUE1.png",
                "Assets/Textures/Animation/Door/puertaBOSQUEf.png",
                "Assets/Textures/Animation/Door/SS_puerta_bosque.png",
                "Assets/Textures/Animation/Door/SS_puerta_bosque.tsx"
            };
        case KeyType::MOUNTAIN:
            return {
                "Assets/Textures/Animation/Door/puertamontana1.png",
                "Assets/Textures/Animation/Door/puertamontanaF.png",
                "Assets/Textures/Animation/Door/SS_puerta_montana.png",
                "Assets/Textures/Animation/Door/SS_puerta_montana.tsx"
            };
        default:
            return {
                "Assets/Textures/Animation/Door/SS_puerta_catacumbas_primero.png",
                "Assets/Textures/Animation/Door/SS_puerta_catacumbas_final.png",
                "Assets/Textures/Animation/Door/SS_puerta_catacumbas.png",
                "Assets/Textures/Animation/Door/SS_puerta_catacumbas.tsx"
            };
        }
    }
}

KeyGate::KeyGate() : Entity(EntityType::KEY_GATE)
{
    name = "KeyGate";
}

KeyGate::~KeyGate() {}

bool KeyGate::Awake() { return true; }

bool KeyGate::Start()
{
    auto textures = Engine::GetInstance().textures;
    GateVisualAssets assets = GetGateVisualAssets(requiredKey);

    closedTexture = textures->Load(assets.closedTexture);
    openTexture = textures->Load(assets.openTexture);
    animTexture = textures->Load(assets.animationTexture);

    std::unordered_map<int, std::string> aliases = { {0, "open"} };
    bool loadOK = anims.LoadFromTSX(assets.animationData, aliases);

    if (loadOK && anims.Has("open")) {
        anims.GetAnim("open")->SetLoop(false);
    }

    if (!gateID.empty()) {
        auto& opened = GameManager::GetInstance().gameState.openedDoors;
        if (std::find(opened.begin(), opened.end(), gateID) != opened.end()) {
            state = GateState::OPEN;
            if (gateCollider != nullptr) {
                gateCollider->SetCollisionsActive(false);
            }
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

void KeyGate::SetCollider(PhysBody* collider)
{
    gateCollider = collider;
    if (gateCollider != nullptr && state == GateState::OPEN) {
        gateCollider->SetCollisionsActive(false);
    }
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
    if (requiredKey == KeyType::FOREST || requiredKey == KeyType::MOUNTAIN) {
        destRect.y += GateVisualOverlap;
        destRect.h += GateVisualOverlap * 2;
    }

    if (state == GateState::OPENING)
    {
        anims.Update(dt);
        SDL_Rect frame = anims.GetCurrentFrame();

        if (animTexture) {
            SDL_Rect animationDest = destRect;
            if (requiredKey == KeyType::FOREST) {
                TrimForestAnimationFrame(frame);
                animationDest.w = (int)(destRect.w * ForestAnimationFrameWidth / ForestStaticFrameWidth);
                animationDest.x = destRect.x - (animationDest.w - destRect.w) / 2;
            }
            else if (requiredKey == KeyType::MOUNTAIN) {
                TrimMountainAnimationFrame(frame);
            }

            Engine::GetInstance().render->DrawRotatedImage(animTexture, &animationDest, &frame);
        }

        if (!animationFinished &&
            anims.GetAnim("open")->HasFinishedOnce())
        {
            animationFinished = true;

            state = GateState::OPEN;
            if (gateCollider != nullptr) {
                gateCollider->SetCollisionsActive(false);
            }

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
