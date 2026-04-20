#pragma once
#include "Item.h"

class Sickle : public Item {
public:
    Sickle();
    virtual ~Sickle();

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    void OnCollision(PhysBody* physA, PhysBody* physB) override;

private:
    SDL_Texture* texture = nullptr;
    PhysBody* pbody = nullptr;

    bool isPicked = false;
};