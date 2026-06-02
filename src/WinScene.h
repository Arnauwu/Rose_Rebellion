#pragma once
#include "SceneBase.h" 
#include <thread>

class WinScene : public SceneBase {
public:
    WinScene();
    virtual ~WinScene();

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

private:
    bool isFadingOut;
    std::thread loadingThread;
};