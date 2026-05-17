#pragma once
#include "SceneBase.h"
#include "Textures.h"

struct SDL_Texture;
class IntroScene : public SceneBase {
public:
    IntroScene();
    virtual ~IntroScene();

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    SDL_Texture* introTexture = nullptr;
   
};