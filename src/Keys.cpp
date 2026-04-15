#include "Keys.h"
#include "Engine.h"
#include "Animation.h"
#include "Textures.h"
#include "Player.h"
#include "Physics.h"
#include "Log.h"

Keys::Keys():Item() {
	name = "Key";
}

Keys::~Keys() {}

bool Keys::Awake()
{
    return true;
}

bool Keys::Start()
{
    // 加载钥匙的贴图 (请根据你的实际路径修改)
    texture = Engine::GetInstance().textures->Load("Assets/Textures/Items/Keys/obj_llave_castillo_game.png");

    // 创建物理碰撞体 (设置为传感器，这样玩家可以直接穿过并触发拾取)
    pbody = Engine::GetInstance().physics->CreateCircleSensor((int)position.getX(), (int)position.getY(), 10, bodyType::KINEMATIC);
    pbody->listener = this;
    pbody->ctype = ColliderType::ITEM; // 或者你可以去 Physics.h 专门加一个 ColliderType::KEY

    return true;
}

bool Keys::Update(float dt)
{
    if (!isPicked)
    {
        // 渲染钥匙
        int x, y;
        pbody->GetPosition(x, y);
        Engine::GetInstance().render->DrawTexture(texture, x - 10, y - 10);
    }
    return true;
}

bool Keys::CleanUp()
{
    Engine::GetInstance().textures->UnLoad(texture);
    if (pbody != nullptr)
    {
        Engine::GetInstance().physics->DeletePhysBody(pbody);
        pbody = nullptr;
    }
    return true;
}

void Keys::OnCollision(PhysBody* physA, PhysBody* physB)
{
    // 拾取逻辑已经在 Player.cpp 的 OnCollision 里处理了，这里可以留空或播放音效
}