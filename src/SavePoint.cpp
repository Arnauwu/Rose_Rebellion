#include "SavePoint.h"

#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Window.h"
#include "Log.h"
#include "Physics.h"
#include "DialogueManager.h"
#include "LanguageManager.h"

#include <cmath>

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
	interactIcon = Engine::GetInstance().textures->Load("Assets/Textures/UI/Buttons/Flecha.png");
	glowTex = Engine::GetInstance().textures->Load("Assets/Textures/UI/Glow.png");

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

	if (isPlayerInRange && Engine::GetInstance().dialogueManager->CanInteract() && interactIcon != nullptr)
	{
		iconTimer += dt / 1000.0f;
		float offsetY = sin(iconTimer * 5.0f) * 10.0f;

		float fIconW = 0.0f;
		float fIconH = 0.0f;
		SDL_GetTextureSize(interactIcon, &fIconW, &fIconH);

		int iconW = (int)fIconW;
		int iconH = (int)fIconH;
		int iconX = x - (iconW / 2);
		int iconY = y - (texH / 2) - iconH - 20 + (int)offsetY;

		Engine::GetInstance().render->DrawTexture(interactIcon, iconX, iconY, nullptr, 1.0f, 0.0, INT_MAX, INT_MAX);

		SDL_Rect cam = Engine::GetInstance().render->camera;
		int scale = Engine::GetInstance().window->GetScale();
		float zoomLevel = Engine::GetInstance().render->GetZoom();
		int screenX = (int)((cam.x + (iconX - iconW) * scale) * zoomLevel);
		int screenY = (int)((cam.y + iconY * scale) * zoomLevel) - 35;
		int screenW = (int)((iconW * 3 * scale) * zoomLevel);
		int screenH = (int)((iconH * scale) * zoomLevel);

		SDL_Rect textBoundsScreen = { screenX, screenY, screenW, screenH };
		SDL_Color textColor = { 255, 255, 255, 255 };
		std::string interactText = Engine::GetInstance().languageManager->GetString("INTERACT_SAVE");
		Engine::GetInstance().render->DrawTextCentered(interactText.c_str(), textBoundsScreen, textColor, FontType::CUERPO);

		if (glowTex != nullptr)
		{
			SDL_SetTextureBlendMode(glowTex, SDL_BLENDMODE_ADD);
			SDL_SetTextureColorMod(glowTex, 150, 150, 150);
			Uint8 glowAlpha = (Uint8)(150 + sin(iconTimer * 5.0f) * 50);
			SDL_SetTextureAlphaMod(glowTex, glowAlpha);

			int glowW = iconW * 3;
			int glowH = (int)(iconH * 2.3f);
			int glowX = screenX - (glowW / 2) + (screenW / 2);
			int glowY = screenY - (glowH / 2) + (screenH / 2) + 5;
			SDL_FRect dstGlow = { (float)glowX, (float)glowY, (float)glowW, (float)glowH };
			SDL_RenderTexture(Engine::GetInstance().render->renderer, glowTex, nullptr, &dstGlow);
		}
	}

	return true;
}

bool SavePoint::CleanUp()
{
	if (texture != nullptr) {
		Engine::GetInstance().textures->UnLoad(texture);
		texture = nullptr;
	}
	if (interactIcon != nullptr) {
		Engine::GetInstance().textures->UnLoad(interactIcon);
		interactIcon = nullptr;
	}
	if (glowTex != nullptr) {
		Engine::GetInstance().textures->UnLoad(glowTex);
		glowTex = nullptr;
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

void SavePoint::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
	if (physB->ctype == ColliderType::PLAYER) {
		isPlayerInRange = true;
	}
}

void SavePoint::OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
	if (physB->ctype == ColliderType::PLAYER) {
		isPlayerInRange = false;
	}
}
