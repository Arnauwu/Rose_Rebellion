#include "Manta.h"
#include "Engine.h"
#include "Textures.h"
#include "EntityManager.h"
#include "Physics.h"
#include "Player.h"

Manta::Manta() :Item(){
	name = "Manta";
}

Manta::~Manta(){}

bool Manta::Awake() {
	return true;
}

bool Manta::Start() {
	if (CheckIfCollected()) return true;
	//Textura
	texture = Engine::GetInstance().textures->Load("Assets/Textures/Items/Manta/obj_capa_game.png");

	//Fisica
	pbody = Engine::GetInstance().physics->CreateCircleSensor((int)position.getX(), (int)position.getY(), texture->h / 2, bodyType::KINEMATIC);
	pbody->listener = this;
	pbody->ctype = ColliderType::ITEM;

	return true;
}

bool Manta::Update(float dt) {
	if (!isPicked)
	{
		//Posicion donde renderiza la manta
		int x, y;
		pbody->GetPosition(x, y);
		Engine::GetInstance().render->DrawTexture(texture, x - texture->w / 2, y - texture->h / 2);
	}
	return true;
}

bool Manta::CleanUp() {
	Engine::GetInstance().textures->UnLoad(texture);
	if (pbody != nullptr)
	{
		Engine::GetInstance().physics->DeletePhysBody(pbody);
		pbody = nullptr;
	}
	return true;
}

void Manta::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) {
	if (physB->ctype == ColliderType::PLAYER) {

		Player* player = (Player*)physB->listener;
		player->UnlockCape();
		SetCollected();
		isPicked = false;
	}
}