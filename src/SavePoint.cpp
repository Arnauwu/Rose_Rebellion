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

	texH = 64;
	texW = 64;

	// Crear el sensor f赤sico usando la esquina superior izquierda (Top-Left)
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
	return true;
}

bool SavePoint::Update(float dt) {
	int x, y;
	// 'x' e 'y' son la esquina superior izquierda de la caja amarilla (Sensor)
	pbody->GetPosition(x, y);

	// ==========================================
	// REPARACI?N DEFINITIVA: Offset Visual
	// ==========================================
	// Debido a que las im芍genes PNG suelen tener p赤xeles transparentes (padding)
	// alrededor de la flor, la imagen se dibuja desplazada.
	// ?? INSTRUCCIONES: Cambia estos n迆meros hasta que la flor encaje en la caja.
	int visualOffsetX = -16; // ?? Negativo mueve la imagen a la IZQUIERDA. Positivo a la DERECHA.
	int visualOffsetY = -32; // ?? Negativo mueve la imagen hacia ARRIBA. Positivo hacia ABAJO.

	// Aplicamos el offset a la posici車n de dibujo
	int drawX = x + visualOffsetX;
	int drawY = y + visualOffsetY;

	if (isActivating)
	{
		// Si est芍 en proceso de activaci車n, actualizamos y dibujamos la animaci車n
		anims.Update(dt);
		SDL_Rect animFrame = anims.GetCurrentFrame();

		Engine::GetInstance().render->DrawTexture(texture, drawX, drawY, &animFrame);

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
		// Dibujamos la textura completa (NULL) en estado Inactivo o Completamente Activo
		Engine::GetInstance().render->DrawTexture(texture, drawX, drawY, NULL);
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