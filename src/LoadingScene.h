#pragma once
#include "SceneBase.h"

class LoadingScene : public SceneBase {
public:
    LoadingScene();
    virtual ~LoadingScene();

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

private:
    int framesWaited;
    bool isLoading;
};