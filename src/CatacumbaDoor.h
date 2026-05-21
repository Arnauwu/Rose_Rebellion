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

    // ???? OpenDoorAt
    void OpenDoorAt(Vector2D pos, int width, int height);

    // ???? 4 ???? InitializeStatic
    void InitializeStatic(Vector2D pos, int width, int height, bool opened);

    bool IsAlreadyOpened() const { return isAlreadyOpened; }

private:
    SDL_Texture* animTexture = nullptr;
    AnimationSet anims;
    bool isOpening = false;
    bool isAlreadyOpened = false;

    int doorW = 0;
    int doorH = 0;

    SDL_Texture* closedTexture = nullptr;
    SDL_Texture* finalOpenTexture = nullptr;
};