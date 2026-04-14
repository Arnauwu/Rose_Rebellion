#include "HomingProjectile.h"
#include <cmath>
#include <SDL3/SDL.h>

#include "Engine.h"
#include "Physics.h"
#include "Textures.h"
#include "SceneManager.h"
#include "Render.h"

// 为了兼容不认识 M_PI 的编译器，手动定义 Pi
constexpr double PI = 3.14159265358979323846;

HomingProjectile::HomingProjectile(Vector2D startPos) : Entity(EntityType::ENEMY_PROJECTILE)
{
    position = startPos;
    name = "HomingProjectile";

    speed = 6.0f;         // 子弹飞行速度
    turnSpeed = 0.05f;    // 转向灵敏度 (越低越难拐弯，玩家越容易跳跃躲避)
    damage = 10;          // 造成的伤害
    lifeTimeMS = 4000.0f; // 4秒后自动销毁 (防止内存泄漏)
}

HomingProjectile::~HomingProjectile() {}

bool HomingProjectile::Start()
{
    // ==========================================
    // 1. 加载子弹贴图
    // ==========================================
    texture = Engine::GetInstance().textures->Load("Assets/Textures/bullet.png");

    // ==========================================
    // 2. 物理碰撞体设置
    // ==========================================
    int radius = 6; // 子弹判定范围小一点，给玩家留出躲避空间
    pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX(), (int)position.getY(), radius, bodyType::DYNAMIC);

    pbody->listener = this;

    // 设置为 ENEMY_ATTACK，它会伤害 PLAYER，且被 PLAYER_ATTACK 打消
    pbody->ctype = ColliderType::ENEMY_ATTACK;

    // 取消重力，让子弹可以在空中任意方向飞行
    if (pbody != nullptr && !B2_IS_NULL(pbody->body)) {
        Engine::GetInstance().physics->SetGravityScale(pbody, 0.0f);
    }

    // ==========================================
    // 3. 计算初始射击方向 (直接射向玩家)
    // ==========================================
    Vector2D playerPos = Engine::GetInstance().sceneManager->GetPlayerPosition();
    Vector2D dir = (playerPos - position).normalized();
    currentVelocity = dir * speed;

    lifeTimer.Start();
    return true;
}

bool HomingProjectile::Update(float dt)
{
    if (!active || pendingToDelete) return true;

    // 存活时间达到上限，自动销毁
    if (lifeTimer.ReadMSec() >= lifeTimeMS) {
        Destroy();
        return true;
    }

    // ==========================================
    // 追踪逻辑 (Lerp 插值算法)
    // ==========================================
    Vector2D playerPos = Engine::GetInstance().sceneManager->GetPlayerPosition();
    Vector2D myPos = GetPosition();

    // 【优化：防止死亡螺旋】
    // 如果子弹存活超过 1.5 秒，它就失去燃料(turnSpeed = 0)，不再追踪玩家，而是沿着最后的直线飞出去
    if (lifeTimer.ReadMSec() > 1500.0f) {
        turnSpeed = 0.0f;
    }

    // 计算期望飞向玩家的方向
    Vector2D desiredDir = (playerPos - myPos).normalized();
    Vector2D desiredVelocity = desiredDir * speed;

    // 缓慢改变当前速度的方向，使得子弹飞出“弧线”
    currentVelocity.setX(currentVelocity.getX() + (desiredVelocity.getX() - currentVelocity.getX()) * turnSpeed);
    currentVelocity.setY(currentVelocity.getY() + (desiredVelocity.getY() - currentVelocity.getY()) * turnSpeed);

    // 将计算好的速度赋予物理引擎
    Engine::GetInstance().physics->SetLinearVelocity(pbody, { currentVelocity.getX(), currentVelocity.getY() });

    Draw(dt);
    return true;
}

void HomingProjectile::OnCollision(PhysBody* physA, PhysBody* physB)
{
    if (pendingToDelete) return;

    switch (physB->ctype)
    {
    case ColliderType::PLAYER:
        // 命中玩家：造成伤害并销毁
        if (physB->listener != nullptr) {
            physB->listener->TakeDamage(damage);
        }
        Destroy();
        break;

    case ColliderType::PLAYER_ATTACK:
        // 【核心机制】：玩家挥剑砍中子弹，子弹被抵消！
        // 如果以后有特效，可以在这里实例化一个爆炸特效
        Destroy();
        break;

    case ColliderType::WALL:
    case ColliderType::GROUND:
    case ColliderType::CEILING:
        // 撞到墙壁或地板，销毁
        Destroy();
        break;

    case ColliderType::ENEMY:
        // 【防自残机制】：撞到飞行怪物自己或其他敌人，什么都不做，直接穿透
        break;

    default:
        break;
    }
}

void HomingProjectile::Draw(float dt)
{
    int x, y;
    pbody->GetPosition(x, y);

    // 旋转贴图：让子弹（比如尖刺）永远指向它正在飞行的方向
    double angle = std::atan2(currentVelocity.getY(), currentVelocity.getX()) * (180.0 / PI);

    // 白盒测试：如果有图片，进行旋转渲染
   /* Engine::GetInstance().render->DrawRotatedTexture(texture, x - 8, y - 8, nullptr, SDL_FLIP_NONE, 1.0, angle);*/

    // 白盒测试：如果没有图片，你可以用 SDL 画个圆点代表子弹
     Engine::GetInstance().render->DrawCircle(x, y, 6, 255, 255, 0, 255); // 黄色圆点
}

Vector2D HomingProjectile::GetPosition()
{
    int x, y;
    pbody->GetPosition(x, y);
    return Vector2D((float)x, (float)y);
}

bool HomingProjectile::CleanUp()
{
    active = false;
    // 卸载图片资源
    Engine::GetInstance().textures->UnLoad(texture);

    // 清理物理碰撞体，交给 Physics 模块统一回收
    if (pbody != nullptr) {
        Engine::GetInstance().physics->DeletePhysBody(pbody);
        pbody = nullptr;
    }
    return true;
}