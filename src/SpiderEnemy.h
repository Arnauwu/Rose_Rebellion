#pragma once

#include "Enemy.h"
#include "Animation.h"
#include <SDL3/SDL.h>
#include "Pathfinding.h"


struct SDL_Texture;

class SpiderEnemy : public Enemy
{
public:
    SpiderEnemy();
    virtual ~SpiderEnemy();
    bool Awake();
    bool Start();
    bool Update(float dt);

    void OnCollision(PhysBody* physA, PhysBody* physB) override;

private:
    void PerformPathfinding();
    void GetPhysicsValues() override {}
    void Move() override;
    void ApplyPhysics() override {}
    void Draw(float dt);
    Vector2D GetTilePos();

};