#pragma once

#include "Entity.h"
#include "Timer.h"
#include "Animation.h"
#include <box2d/box2d.h>

class HomingProjectile : public Entity
{
public:
    HomingProjectile(Vector2D startPos);
    virtual ~HomingProjectile();

    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    void OnCollision(PhysBody* physA, PhysBody* physB) override;

    void Draw(float dt);
    Vector2D GetPosition();

public:
    float speed;
    float turnSpeed; 
    Vector2D currentVelocity;

    SDL_Texture* texture = nullptr;
    PhysBody* pbody = nullptr;

    //Si tenemos sprite para bala
    //AnimationSet anims;

    Timer lifeTimer;
    float lifeTimeMS;
private:
    AnimationSet anims;
};