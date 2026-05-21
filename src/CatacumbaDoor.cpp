#include "CatacumbaDoor.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "SceneManager.h"
#include "Player.h" 
#include "EntityManager.h" // ?? ?? Incomplete Type ??
#include "tracy/Tracy.hpp"

CatacumbaDoorEntity::CatacumbaDoorEntity() : Entity(EntityType::CATACUMBA_DOOR)
{
    name = "CatacumbaDoorAnim";
}

CatacumbaDoorEntity::~CatacumbaDoorEntity() {}

bool CatacumbaDoorEntity::Awake() { return true; }

bool CatacumbaDoorEntity::Start() {
    auto textures = Engine::GetInstance().textures;

    animTexture = textures->Load("Assets/Textures/Animation/Door/SS_puerta_catacumba.png");
    std::unordered_map<int, std::string> aliases = { {0, "open"} };
    anims.LoadFromTSX("Assets/Textures/Animation/Door/SS_puerta_catacumba.tsx", aliases);
    if (anims.Has("open")) {
        anims.GetAnim("open")->SetLoop(false);
    }

    closedTexture = textures->Load("Assets/Textures/Animation/Door/SS_puerta_catacumbas_primero.png");
    finalOpenTexture = textures->Load("Assets/Textures/Animation/Door/SS_puerta_catacumbas_final.png");

    return true;
}

void CatacumbaDoorEntity::InitializeStatic(Vector2D pos, int width, int height, bool opened) {
    if (closedTexture == nullptr) Start();
    position = pos;
    doorW = width;
    doorH = height;
    isAlreadyOpened = opened;
    isOpening = false;
}

bool CatacumbaDoorEntity::Update(float dt) {
    ZoneScoped;
    float cx = position.getX();
    float cy = position.getY();

    const float SCALE_X = 1.65f;
    const float SCALE_Y = 1.05f;
    const float Y_OFFSET = 0.0f;

    float drawW = (float)doorW * SCALE_X;
    float drawH = (float)doorH * SCALE_Y;

    SDL_Rect destRect;
    destRect.x = (int)(cx - (drawW / 2.0f));
    destRect.y = (int)(cy - (drawH / 2.0f) + Y_OFFSET);
    destRect.w = (int)drawW;
    destRect.h = (int)drawH;

    if (isOpening) {
        if (anims.Has("open")) {
            anims.Update(dt);
            SDL_Rect frame = anims.GetCurrentFrame();
            if (animTexture) Engine::GetInstance().render->DrawRotatedImage(animTexture, &destRect, &frame);

            if (anims.GetAnim("open")->HasFinishedOnce()) {
                isOpening = false;
                isAlreadyOpened = true;
                Engine::GetInstance().entityManager->GetPlayer()->isFrozen = false;
                Engine::GetInstance().sceneManager->setNewMap = true;
            }
        }
        else {
            isOpening = false;
            isAlreadyOpened = true;
            Engine::GetInstance().entityManager->GetPlayer()->isFrozen = false;
            Engine::GetInstance().sceneManager->setNewMap = true;
        }
    }
    else if (isAlreadyOpened) {
        if (finalOpenTexture) Engine::GetInstance().render->DrawRotatedImage(finalOpenTexture, &destRect, nullptr);
    }
    else {
        if (closedTexture) Engine::GetInstance().render->DrawRotatedImage(closedTexture, &destRect, nullptr);
    }

    return true;
}

void CatacumbaDoorEntity::OpenDoorAt(Vector2D pos, int width, int height) {
    isOpening = true;
    if (anims.Has("open")) {
        anims.GetAnim("open")->Reset();
        anims.SetCurrent("open");
    }
}

bool CatacumbaDoorEntity::CleanUp() {
    auto textures = Engine::GetInstance().textures;
    if (animTexture) { textures->UnLoad(animTexture); animTexture = nullptr; }
    if (closedTexture) { textures->UnLoad(closedTexture); closedTexture = nullptr; }
    if (finalOpenTexture) { textures->UnLoad(finalOpenTexture); finalOpenTexture = nullptr; }
    return true;
}