#pragma once
#include "SceneBase.h"

class GameOverScene : public SceneBase {
public:
    GameOverScene();
    virtual ~GameOverScene();

    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

private:
    float timer = 0.0f;           
    float displayTime = 4.0f;
};