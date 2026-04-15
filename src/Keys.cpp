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
   //Textura
    texture = Engine::GetInstance().textures->Load("Assets/Textures/Items/Keys/obj_llave_castillo_game.png");

    //Fisica
    pbody = Engine::GetInstance().physics->CreateCircleSensor((int)position.getX(), (int)position.getY(), 10, bodyType::KINEMATIC);
    pbody->listener = this;
    pbody->ctype = ColliderType::ITEM;

    return true;
}

bool Keys::Update(float dt)
{
    if (!isPicked)
    {
        //Posicion donde renderiza la llave
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
    // Aquí podemos añadir un efecto de sonido al recoger la llave.
}