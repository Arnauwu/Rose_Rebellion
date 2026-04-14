#include "FlyingEnemy.h"
#include "HomingProjectile.h" // 引入子弹类
#include <cmath>
#include <SDL3/SDL.h>

#include "Engine.h"
#include "Physics.h"
#include "Textures.h"
#include "SceneManager.h"
#include "Render.h"
#include "Log.h"
#include "EntityManager.h"

FlyingEnemy::FlyingEnemy() : Enemy(EntityType::FLYING_ENEMY)
{
    name = "FlyingEnemy";
    currentState = FlyingEnemyState::IDLE;

    // 悬停参数：根据玩家跳跃高度来微调，确保玩家跳起来能砍到它
    targetOffsetX = 120.0f; // 距离玩家的水平距离
    targetOffsetY = 80.0f;  // 距离玩家的垂直高度
    attackRange = 15.0f;    // 允许攻击的误差范围

    // 时间参数
    windupDurationMs = 800.0f;   // 蓄力时间 0.8秒 (给玩家预警)
    cooldownDurationMs = 2000.0f; // 攻击后休息 2秒
}

FlyingEnemy::~FlyingEnemy() {}

bool FlyingEnemy::Awake() {
    return true;
}

bool FlyingEnemy::Start()
{
    // ==========================================
    // 1. 动画加载 (美术资源准备好后取消注释即可)
    // ==========================================
    /*
    std::unordered_map<int, std::string> aliases = {
        {0, "idle"}, {4, "fly"}, {8, "windup"}, {12, "attack"}, {16, "dead"}
    };
    anims.LoadFromTSX("Assets/Textures/FlyingEnemy.tsx", aliases);
    anims.SetCurrent("idle");
    */

    // 暂时用 Cucafera 或者一张纯色图代替
    texture = Engine::GetInstance().textures->Load("Assets/Textures/bullet.png");

    // ==========================================
    // 2. 物理碰撞体设置
    // ==========================================
    texW = 32;
    texH = 32;
    pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX(), (int)position.getY(), texW / 2, bodyType::DYNAMIC);
    pbody->listener = this;
    pbody->ctype = ColliderType::ENEMY;

    // 【核心】取消重力，让它可以悬浮在空中
    if (pbody != nullptr && !B2_IS_NULL(pbody->body)) {
        Engine::GetInstance().physics->SetGravityScale(pbody, 0.0f);
    }

    // ==========================================
    // 3. 基础属性配置
    // ==========================================
    vision = 15;
    speed = 3.0f;
    knockbackForce = 5.0f;
    maxHealth = 20;
    currentHealth = 20;

    return true;
}

bool FlyingEnemy::Update(float dt)
{
    if (!active) return true;

    if (Engine::GetInstance().sceneManager->isGamePaused == false && isdead == false)
    {
        GetPhysicsValues();
        Move();         // 状态机与移动逻辑
        Knockback();    // 击退逻辑
        ApplyPhysics(); // 应用物理速度
    }

    // 处理死亡表现
    if (isdead /* && anims.GetCurrentName() != "dead" */)
    {
        Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0, 0 });
        // anims.GetAnim("dead")->SetLoop(false);
        // anims.SetCurrent("dead");
        pbody->ctype = ColliderType::UNKNOWN; // 尸体不再产生碰撞
    }

    Draw(dt);
    return true;
}

void FlyingEnemy::GetPhysicsValues() {
    // 每一帧开始时重置期望速度，由 Move() 重新计算
    velocity = { 0.0f, 0.0f };
}

void FlyingEnemy::Move() {
    if (isdead || isKnockedback) return;

    Vector2D playerPos = Engine::GetInstance().sceneManager->GetPlayerPosition();
    Vector2D myPos = GetPosition();

    // ==========================================
    // 朝向控制：确保怪物永远面朝玩家
    // ==========================================
    lookingRight = (playerPos.getX() > myPos.getX());

    // ==========================================
    // 计算悬停目标点：玩家斜上方
    // ==========================================
    float dirX = (playerPos.getX() < myPos.getX()) ? 1.0f : -1.0f;
    Vector2D targetPos(playerPos.getX() + (dirX * targetOffsetX), playerPos.getY() - targetOffsetY);
    float distToHoverPos = (targetPos - myPos).magnitude();

    // ==========================================
    // 状态机逻辑
    // ==========================================
    switch (currentState) {
    case FlyingEnemyState::IDLE:
    {
        // 玩家进入视野，开始追击
        float distToPlayer = (playerPos - myPos).magnitude();
        if (distToPlayer < vision * 32.0f) { // 假设一个 tile 是 32 像素
            currentState = FlyingEnemyState::CHASE;
        }
        break;
    }
    case FlyingEnemyState::CHASE:
    {
        if (distToHoverPos <= attackRange) {
            // 距离达标，开始蓄力
            currentState = FlyingEnemyState::WINDUP;
            stateTimer.Start();
            // anims.SetCurrent("windup"); // 播放蓄力动画
        }
        else {
            // 飞行怪专属移动逻辑：无视地形直线向目标点飞行
            // 这样它就会像个幽灵一样丝滑地悬浮跟随玩家
            Vector2D moveDir = (targetPos - myPos).normalized();
            velocity.x = moveDir.getX() * speed;
            velocity.y = moveDir.getY() * speed;
        }
        break;
    }
    case FlyingEnemyState::WINDUP:
    {
        // 蓄力期间保持静止
        velocity = { 0.0f, 0.0f };

        // 【防抽搐优化】：如果玩家跑得太远（超过攻击范围的两倍），打断蓄力继续追
        if (distToHoverPos > attackRange * 2.5f) {
            currentState = FlyingEnemyState::CHASE;
            break;
        }

        // 蓄力时间到，开火！
        if (stateTimer.ReadMSec() >= windupDurationMs) {
            currentState = FlyingEnemyState::ATTACK;
            // anims.SetCurrent("attack");
        }
        break;
    }
    case FlyingEnemyState::ATTACK:
    {
        ShootProjectile(); // 发射追踪弹
        currentState = FlyingEnemyState::COOLDOWN;
        stateTimer.Start();
        break;
    }
    case FlyingEnemyState::COOLDOWN:
    {
        // 【风筝逻辑】：冷却期间依然要跟玩家保持距离
        if (distToHoverPos > attackRange) {
            Vector2D moveDir = (targetPos - myPos).normalized();
            velocity.x = moveDir.getX() * speed;
            velocity.y = moveDir.getY() * speed;
        }
        else {
            velocity = { 0.0f, 0.0f };
        }

        // 冷却结束，重新寻找攻击机会
        if (stateTimer.ReadMSec() >= cooldownDurationMs) {
            currentState = FlyingEnemyState::CHASE;
        }
        break;
    }
    }
}

void FlyingEnemy::ApplyPhysics() {
    Engine::GetInstance().physics->SetLinearVelocity(pbody, { velocity.x, velocity.y });
}

void FlyingEnemy::Knockback()
{
    if (isdead) return;

    if (isKnockedback)
    {
        // anims.SetCurrent("hurt");
        // 受伤后向后退
        velocity.x = lookingRight ? -knockbackForce : knockbackForce;
    }

    if (knockbackTime <= 0) {
        isKnockedback = false;
        knockbackTime = 500.0f;
    }
    else {
        knockbackTime -= Engine::GetInstance().GetDt();
    }
}

void FlyingEnemy::Draw(float dt)
{
    if (Engine::GetInstance().sceneManager->isGamePaused == false) {
        // anims.Update(dt);
    }

    int x, y;
    pbody->GetPosition(x, y);

    // 水平翻转：根据朝向翻转贴图
    SDL_FlipMode sdlFlip = lookingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;

    // 白盒测试：如果你有图片，用下面这行：
    Engine::GetInstance().render->DrawRotatedTexture(texture, x - 16, y - 16, nullptr, sdlFlip, 1.0);

    // 白盒测试：如果没有图片，你可以用 SDL 画个正方形代表它
    // Engine::GetInstance().render->DrawRectangle(x - 16, y - 16, 32, 32, 255, 0, 0, 255); // 红色方块
}

void FlyingEnemy::ShootProjectile() {
    Vector2D spawnPos = GetPosition();

    // 子弹在怪物身前一点生成，防止刚生出来就跟自己重叠
    spawnPos.setX(lookingRight ? spawnPos.getX() + 20.0f : spawnPos.getX() - 20.0f);

    // 【修改这里】：使用 std::make_shared 创建智能指针，完全契合你的 EntityManager 架构
    std::shared_ptr<HomingProjectile> bullet = std::make_shared<HomingProjectile>(spawnPos);

    // 必须让子弹执行 Start() 以创建物理碰撞体
    bullet->Start();

    // 把子弹加入到引擎的实体管理器中
    Engine::GetInstance().entityManager->AddEntity(bullet);
}

bool FlyingEnemy::CleanUp() {
    return Enemy::CleanUp();
}