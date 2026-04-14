#pragma once

#include "Entity.h"
#include "Timer.h"
#include "Animation.h"
#include <box2d/box2d.h>

class HomingProjectile : public Entity
{
public:
    // 构造函数传入出生位置
    HomingProjectile(Vector2D startPos);
    virtual ~HomingProjectile();

    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    // 碰撞检测：用于打消子弹或伤害玩家
    void OnCollision(PhysBody* physA, PhysBody* physB) override;

    void Draw(float dt);
    Vector2D GetPosition();

public:
    float speed;
    float turnSpeed; // 控制追踪的灵敏度 (平滑转向)
    Vector2D currentVelocity;

    SDL_Texture* texture = nullptr;
    PhysBody* pbody = nullptr;
    AnimationSet anims;

    Timer lifeTimer; // 生命计时器，防止子弹飞出地图外永远存在
    float lifeTimeMS;
};