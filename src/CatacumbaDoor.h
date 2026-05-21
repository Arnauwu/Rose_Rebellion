#pragma once
#include "Entity.h"
#include "Animation.h"
#include "Vector2D.h"

struct SDL_Texture;

class CatacumbaDoorEntity : public Entity
{
public:
    CatacumbaDoorEntity();
    virtual ~CatacumbaDoorEntity();

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    // 交互时的开门动作
    void OpenDoorAt(Vector2D pos, int width, int height);

    // ?? 新加：地图刚加载时的静默初始化
    void InitializeStatic(Vector2D pos, int width, int height, bool opened);

private:
    SDL_Texture* animTexture = nullptr;
    AnimationSet anims;
    bool isOpening = false;
    bool isAlreadyOpened = false; // ?? 新加：记录门是否已处于常开状态

    int doorW = 0;
    int doorH = 0;

    SDL_Texture* closedTexture = nullptr;
    SDL_Texture* finalOpenTexture = nullptr;
};