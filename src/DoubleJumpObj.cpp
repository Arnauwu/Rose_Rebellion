#include "DoubleJumpObj.h"
#include "Engine.h"
#include "Textures.h"
#include "EntityManager.h"
#include "Physics.h"
#include "Player.h"

DoubleJumpObj::DoubleJumpObj() :Item() {
	name = "DoubleJumpObj";
}

DoubleJumpObj::~DoubleJumpObj() {}

bool DoubleJumpObj::Awake() {
	return true;
}

bool DoubleJumpObj::Start() {
	if (CheckIfCollected()) return true;
	//Textura
	texture = Engine::GetInstance().textures->Load("Assets/Textures/Items/Manta/obj_capa_game.png");//

	//Fisica
	pbody = Engine::GetInstance().physics->CreateCircleSensor((int)position.getX(), (int)position.getY(), texture->h / 2, bodyType::KINEMATIC);
	pbody->listener = this;
	pbody->ctype = ColliderType::ITEM;

	return true;
}

bool DoubleJumpObj::Update(float dt) {
	if (!isPicked)
	{
		//Posicion donde renderiza la manta
		int x, y;
		pbody->GetPosition(x, y);
		Engine::GetInstance().render->DrawTexture(texture, x - texture->w / 2, y - texture->h / 2);
	}
	return true;
}

bool DoubleJumpObj::CleanUp() {
	Engine::GetInstance().textures->UnLoad(texture);
	if (pbody != nullptr)
	{
		Engine::GetInstance().physics->DeletePhysBody(pbody);
		pbody = nullptr;
	}
	return true;
}

void DoubleJumpObj::OnCollision(PhysBody* physA, PhysBody* physB) {
	if (physB->ctype == ColliderType::PLAYER) {

		Player* player = (Player*)physB->listener;
		player->UnlockCape();
		SetCollected();
		isPicked = false;
	}
}