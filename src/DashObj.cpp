#include "DashObj.h"
#include "Engine.h"
#include "Textures.h"
#include "EntityManager.h"
#include "Physics.h"
#include "Player.h"

DashObj::DashObj() :Item() {
	name = "DashObj";
}

DashObj::~DashObj() {}

bool DashObj::Awake() {
	return true;
}

bool DashObj::Start() {
	if (CheckIfCollected()) return true;
	//Textura
	texture = Engine::GetInstance().textures->Load("Assets/Textures/Items/Manta/obj_capa_game.png");//

	//Fisica
	pbody = Engine::GetInstance().physics->CreateCircleSensor((int)position.getX(), (int)position.getY(), texture->h / 2, bodyType::KINEMATIC);
	pbody->listener = this;
	pbody->ctype = ColliderType::ITEM;

	return true;
}

bool DashObj::Update(float dt) {
	if (!isPicked)
	{
		//Posicion donde renderiza la manta
		int x, y;
		pbody->GetPosition(x, y);
		Engine::GetInstance().render->DrawTexture(texture, x - texture->w / 2, y - texture->h / 2);
	}
	return true;
}

bool DashObj::CleanUp() {
	Engine::GetInstance().textures->UnLoad(texture);
	if (pbody != nullptr)
	{
		Engine::GetInstance().physics->DeletePhysBody(pbody);
		pbody = nullptr;
	}
	return true;
}

void DashObj::OnCollision(PhysBody* physA, PhysBody* physB) {
	if (physB->ctype == ColliderType::PLAYER) {

		Player* player = (Player*)physB->listener;
		player->UnlockDash();
		SetCollected();
		isPicked = false;
	}
}