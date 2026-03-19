#include"SavePoint.h"
#include"Engine.h"
#include"Textures.h"
#include"Render.h"
#include"Log.h"
#include"Physics.h"

SavePoint::SavePoint() :Entity(EntityType::SAVEPOINT) {
	name = "SavePoint";
}

SavePoint::~SavePoint(){}

bool SavePoint::Awake(){
	return true;
}

bool SavePoint::Start() {
	texture = Engine::GetInstance().textures->Load("Assets/Textures/player1.png");
	//std::unordered_map<int, std::string>aliase = { {0,"off"},{1,"on"} };

	pbody = Engine::GetInstance().physics->CreateRectangleSensor((int)position.getX(), (int)position.getY(), 32, 32, bodyType::STATIC);
	pbody->ctype = ColliderType::SAVEPOINT;
	pbody->listener=this;

	return true;

}

bool SavePoint::Update(float dt) {
	int x, y;
	pbody->GetPosition(x, y);
	Engine::GetInstance().render->DrawTexture(texture, x - 16, y - 16, NULL);

	return true;
}

bool SavePoint::CleanUp() {
	Engine::GetInstance().textures->UnLoad(texture);
	return true;
}

void SavePoint::Activate() {
	if (!isActivated) {
		isActivated = true;
		LOG("SavePoint Activated");
	}
}