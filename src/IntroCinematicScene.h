#pragma once
#include "SceneBase.h"
#include <thread>

class IntroCinematicScene : public SceneBase {
public:
    IntroCinematicScene();
    virtual ~IntroCinematicScene();

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;
private:
    bool isFadingOut; // Para controlar si ya hemos pedido cambiar de escena
    std::thread loadingThread;
};