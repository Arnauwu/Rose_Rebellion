#pragma once
#include "SceneBase.h"
#include "Textures.h"

struct SDL_Texture;
class IntroScene : public SceneBase {
public:
    IntroScene();
    virtual ~IntroScene();

    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    SDL_Texture* introTexture = nullptr;

private:
    float introTimer = 0.0f;
};