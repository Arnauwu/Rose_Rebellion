#pragma once

#include "Entity.h"
#include "Timer.h"
#include "Animation.h"
#include <box2d/box2d.h>

class WaveProjectile : public Entity
{
public:
    WaveProjectile();
    virtual ~WaveProjectile();

    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    void OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) override;

    void Draw(float dt);
    Vector2D GetPosition();

public:
    float speed;
    Vector2D velocity;

    SDL_Texture* texture = nullptr;
    PhysBody* pbody = nullptr;

private:
    AnimationSet anims;
};