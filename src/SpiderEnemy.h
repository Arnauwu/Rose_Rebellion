#pragma once

#include "Enemy.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include "Pathfinding.h"

enum class Facing { DOWN, LEFT, UP, RIGHT };

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
    void OnCollisionEnd(PhysBody* physA, PhysBody* physB) override;

private:
    void GetPhysicsValues() override;
    void Move() override;
    void Knockback() override;
    void RotateFacing();
    void ApplyPhysics() override;
    void Draw(float dt);
    Vector2D GetTilePos();

    Facing currentFacing = Facing::DOWN; // Where he's stepping
    float angle = 0.0f;                  // Rotation
    int moveSpeed = 100;
    bool isStuck = false;
    int rule = 0;
	float time = 0.0f;
};

