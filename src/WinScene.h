#pragma once
#include "SceneBase.h" // Asegúrate de que este es el nombre correcto del archivo base de tus escenas

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
};