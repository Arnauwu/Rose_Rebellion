#include "CatacumbaDoor.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "SceneManager.h"
#include "Player.h" // 引入 Player 用于解冻
#include "tracy/Tracy.hpp"

CatacumbaDoorEntity::CatacumbaDoorEntity() : Entity(EntityType::DOOR)
{
    name = "CatacumbaDoorAnim";
}

CatacumbaDoorEntity::~CatacumbaDoorEntity() {}

bool CatacumbaDoorEntity::Awake() { return true; }

bool CatacumbaDoorEntity::Start() {
    auto textures = Engine::GetInstance().textures;

    // 加载动画资源（如果没有这个文件也没关系，下面有防卡死保护）
    animTexture = textures->Load("Assets/Textures/Animation/Door/SS_puerta_catacumba.png");
    std::unordered_map<int, std::string> aliases = { {0, "open"} };
    anims.LoadFromTSX("Assets/Textures/Animation/Door/SS_puerta_catacumba.tsx", aliases);
    if (anims.Has("open")) {
        anims.GetAnim("open")->SetLoop(false);
    }

    // 重点：确保这两个图片的路径完全正确！
    closedTexture = textures->Load("Assets/Textures/Animation/Door/SS_puerta_catacumbas_primero.png");
    finalOpenTexture = textures->Load("Assets/Textures/Animation/Door/SS_puerta_catacumbas_final.png");

    return true;
}

// ?? 新加：地图加载时直接摆放好门
void CatacumbaDoorEntity::InitializeStatic(Vector2D pos, int width, int height, bool opened) {
    if (closedTexture == nullptr) Start();
    position = pos;
    doorW = width;
    doorH = height;
    isOpening = false;
    isAlreadyOpened = opened;
}

bool CatacumbaDoorEntity::Update(float dt) {
    ZoneScoped;

    float cx = position.getX();
    float cy = position.getY();

    // 消除图片边缘空白的比例尺（如果门太窄，调大 SCALE_X）
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

    // 状态 1：正在开门
    if (isOpening) {
        // 如果有动画文件，正常播放
        if (anims.Has("open")) {
            anims.Update(dt);
            SDL_Rect frame = anims.GetCurrentFrame();
            if (animTexture) Engine::GetInstance().render->DrawRotatedImage(animTexture, &destRect, &frame);

            if (anims.GetAnim("open")->HasFinishedOnce()) {
                isOpening = false;
                isAlreadyOpened = true;
                Engine::GetInstance().entityManager->GetPlayer()->isFrozen = false; // 强行解冻玩家
                Engine::GetInstance().sceneManager->setNewMap = true;
            }
        }
        // ?? 终极防卡死：如果找不到动画文件，立刻切图并解冻，绝不卡死！
        else {
            isOpening = false;
            isAlreadyOpened = true;
            Engine::GetInstance().entityManager->GetPlayer()->isFrozen = false; // 强行解冻玩家
            Engine::GetInstance().sceneManager->setNewMap = true;
        }
    }
    // 状态 2：门是开着的
    else if (isAlreadyOpened) {
        if (finalOpenTexture) Engine::GetInstance().render->DrawRotatedImage(finalOpenTexture, &destRect, nullptr);
    }
    // 状态 3：门是关着的
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