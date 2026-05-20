#include "SavePoint.h"

#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Log.h"
#include "Physics.h"

SavePoint::SavePoint() :Entity(EntityType::SAVEPOINT) {
	name = "SavePoint";
	pbody = nullptr;
	texture = nullptr;
	zOrder = -1;
}

SavePoint::~SavePoint() {}

bool SavePoint::Awake() {
	return true;
}

bool SavePoint::Start() {

	std::unordered_map<int, std::string> aliases = {
	  {0, "Activate"} // Animaci車n de activaci車n
	};
	anims.LoadFromTSX("Assets/Textures/Items/SavePoint/Rosa.tsx", aliases);
	anims.SetCurrent("Activate");
	anims.GetAnim("Activate")->SetLoop(false);

	texture = Engine::GetInstance().textures->Load("Assets/Textures/Items/SavePoint/NoActivo.png");

	texW = 256;
	texH = 256;

	// Misma l車gica exacta que Dip
	pbody = Engine::GetInstance().physics->CreateRectangleSensor(
		(int)position.getX(),
		(int)position.getY(),
		texW,
		texH,
		bodyType::STATIC
	);

	Engine::GetInstance().physics->SetGravityScale(pbody, 0.0f);

	pbody->ctype = ColliderType::SAVEPOINT;
	pbody->listener = this;

	isActivated = false;
	isActivating = false;

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	return true;
}

bool SavePoint::Update(float dt) {
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	// L車gica id谷ntica a Dip, sin offsets
	int drawX = x;
	int drawY = y;

	if (isActivating)
	{
		// Si est芍 en proceso de activaci車n, actualizamos y dibujamos la animaci車n
		anims.Update(dt);
		SDL_Rect animFrame = anims.GetCurrentFrame();

		Engine::GetInstance().render->DrawRotatedTexture(texture, drawX, drawY, &animFrame, SDL_FLIP_NONE, 1);

		// Comprobar si la animaci車n ya termin車
		if (anims.GetAnim("Activate")->HasFinishedOnce())
		{
			isActivating = false;
			isActivated = true;

			// Cargar la imagen final est芍tica
			Engine::GetInstance().textures->UnLoad(texture);
			texture = Engine::GetInstance().textures->Load("Assets/Textures/Items/SavePoint/Activo.png");
			LOG("SavePoint: Animaci車n terminada. Estado -> ACTIVE");
		}
	}
	else
	{
		
		Engine::GetInstance().render->DrawRotatedTexture(texture, drawX, drawY, NULL, SDL_FLIP_NONE, 1);
	}

	return true;
}

bool SavePoint::CleanUp()
{
	if (texture != nullptr) {
		Engine::GetInstance().textures->UnLoad(texture);
		texture = nullptr;
	}
	if (pbody != nullptr) {
		Engine::GetInstance().physics->DeletePhysBody(pbody);
		pbody = nullptr;
	}
	return true;
}

void SavePoint::Activate() {
	if (!isActivated && !isActivating) {

		isActivating = true;

		if (anims.GetAnim("Activate") != nullptr) {
			anims.GetAnim("Activate")->Reset();
		}

		Engine::GetInstance().textures->UnLoad(texture);
		texture = Engine::GetInstance().textures->Load("Assets/Textures/Items/SavePoint/Rosa.png");

		LOG("SavePoint: Activando... Estado -> ACTIVATING");
	}
}