#include "Hud.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "EntityManager.h"
#include "SceneManager.h"
#include "GameScene.h"
#include "Player.h"
#include "Window.h"
#include "Log.h"
#include "LanguageManager.h"
#include "DialogueManager.h"
#include <string>
#include "Physics.h"

#include "tracy/Tracy.hpp"

Hud::Hud() : Module() {
	name = "hud";
}

Hud::~Hud() {}

bool Hud::Awake() { return true; }

bool Hud::Start() {
	LOG("Loading HUD");
	lifeBarTexture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Princess/SS_Vida_Princesa.png");

	notificationBgTexture = Engine::GetInstance().textures->Load("Assets/Textures/UI/Tutorial/Feebback_V2.png");

	float imagenAnchoReal = 6144.0f;
	float imagenAltoReal = 5109.0f;

	int cols = 12;
	int rows = 10;

	// 2. Ahora el cálculo será perfecto para el tamaño que de verdad tiene tu archivo
	float exactWidth = imagenAnchoReal / (float)cols;
	float exactHeight = imagenAltoReal / (float)rows;

	int dibujosPorFila[] = { 10, 3, 4, 4, 4, 4, 3, 10, 2, 5 };
	lifeFrames.clear();

	for (int fila = 0; fila < 10; fila++) {
		int numDibujos = dibujosPorFila[fila];

		for (int c = 0; c < numDibujos; c++) {
			int posX = (int)(c * exactWidth);
			int posY = (int)(fila * exactHeight);

			// Quitamos un par de píxeles por si el artista apuró mucho los bordes
			int recorteAncho = (int)exactWidth - 2;
			int recorteAlto = (int)exactHeight;

			SDL_Rect frameRect = { posX, posY, recorteAncho, recorteAlto };
			lifeFrames.push_back(frameRect);
		}
	}

	return true;
}

bool Hud::Update(float dt) {
	ZoneScoped;

	Player* player = Engine::GetInstance().entityManager->GetPlayer();
	if (player == nullptr) return true;

	// --- LÓGICA DE ANIMACIÓN DE VIDA CON ÚLTIMO ALIENTO ---
	if (player->maxHealth > 0 && !lifeFrames.empty()) {
		int hp = player->currentHealth;
		int maxHp = player->maxHealth;

		int targetFrame = 0;
		float dtSeconds = dt / 1000.0f;

		// 1. EL JUGADOR ESTÁ MUERTO (0 HP)
		if (hp <= 0 || player->isdead) {
			targetFrame = 43; // Se queda en la barra negra
		}
		// 2. ÚLTIMO ALIENTO (1 HP)
		else if (hp == 1) {
			// Iniciar la secuencia si no estaba activa
			if (!isLastBreathAnimating) {
				isLastBreathAnimating = true;
				lastBreathPauseTimer = 0.5f; // Medio segundo de pausa en negro (ajusta a tu gusto)
			}

			if (displayedFrame < 43.0f) {
				// Paso 1: Forzar a que la animación llegue al frame negro (43)
				targetFrame = 43;
			}
			else {
				// Paso 2: Pausa en el frame negro
				if (lastBreathPauseTimer > 0.0f) {
					targetFrame = 43;
					lastBreathPauseTimer -= dtSeconds;
				}
				else {
					// Paso 3: Termina la pausa, animar la última fila hasta el frame 48
					targetFrame = 48;
				}
			}
		}
		// 3. VIDA NORMAL (> 1 HP)
		else {
			isLastBreathAnimating = false; // Resetear por si se cura

			// Mapeamos la vida restante (del 2 al maxHp) entre los frames 0 y 41
			float percent = (float)(hp - 1) / (float)(maxHp - 1);
			targetFrame = (int)((1.0f - percent) * 41.0f);

			// Límites de seguridad para vida normal
			if (targetFrame < 0) targetFrame = 0;
			if (targetFrame > 41) targetFrame = 41;
		}

		// --- CÓDIGO QUE MUEVE EL DISPLAYED FRAME HACIA EL TARGET ---
		if (displayedFrame < targetFrame) {
			displayedFrame += animSpeed * dtSeconds;
			if (displayedFrame > targetFrame) displayedFrame = targetFrame;
		}
		else if (displayedFrame > targetFrame) {
			displayedFrame -= animSpeed * dtSeconds;
			if (displayedFrame < targetFrame) displayedFrame = targetFrame;
		}
	}
	// -----------------------------------------------------------
	// ------------------------------------------

	if (notificationTimer > 0.0f) {
		notificationTimer -= dt / 1000.0f;
		if (notificationTimer < 0.0f) {
			notificationTimer = 0.0f;
		}
	}

	return true;
}

bool Hud::PostUpdate() {
	ZoneScoped;

	auto sceneManager = Engine::GetInstance().sceneManager;
	Player* player = Engine::GetInstance().entityManager->GetPlayer();

	if (player == nullptr) {
		return true;
	}
	DrawBossIntro(Engine::GetInstance().GetDt());

	if (sceneManager->IsGamePaused() || isHidden) {
		return true;
	}

	DrawPlayerHealthBar();
	DrawMineralIndicator();
	DrawDiamondCounter();
	DrawNotification();
	return true;
}

void Hud::DrawPlayerHealthBar() {
	if (lifeBarTexture == nullptr) {
		LOG("ERROR: La textura de la vida no se ha cargado. Revisa la ruta y el .png");
		return;
	}
	if (lifeFrames.empty()) {
		LOG("ERROR: La lista de frames esta vacia.");
		return;
	}

	Player* player = Engine::GetInstance().entityManager->GetPlayer();
	if (player == nullptr) return;

	// Obtenemos el frame actual truncando los decimales
	int frameActual = (int)displayedFrame;

	// Seguridad total: Evitar salirnos del array (Crash prevention)
	if (frameActual < 0) frameActual = 0;
	if (frameActual >= lifeFrames.size()) frameActual = lifeFrames.size() - 1;

	// 3. Obtener el recorte precalculado y dibujarlo
	SDL_Rect srcRect = lifeFrames[frameActual];

	SDL_Renderer* renderer = Engine::GetInstance().render->renderer;
	int scale = Engine::GetInstance().window->GetScale();
	float zoomLevel = Engine::GetInstance().render->GetZoom();

	float marginX = 30.0f;
	float marginY = -300.0f;

	SDL_FRect srcFRect = { (float)srcRect.x, (float)srcRect.y, (float)srcRect.w, (float)srcRect.h };

	SDL_FRect dstFRect;
	dstFRect.x = (float)(marginX * scale) * zoomLevel;
	dstFRect.y = (float)(marginY * scale) * zoomLevel;

	dstFRect.w = (float)(srcRect.w * scale) * zoomLevel * 1.5f;
	dstFRect.h = (float)(srcRect.h * scale) * zoomLevel * 1.5f;

	SDL_RenderTexture(renderer, lifeBarTexture, &srcFRect, &dstFRect);
}

void Hud::DrawDiamondCounter() {

}

void Hud::DrawMineralIndicator() {

}


bool Hud::CleanUp() {
	activeBossPortraitTex = nullptr;
	activeBossPortraitAnim = nullptr;
	isBossIntroActive = false;

	if (lifeBarTexture != nullptr) {
		Engine::GetInstance().textures->UnLoad(lifeBarTexture);
		lifeBarTexture = nullptr;
	}
	if (notificationBgTexture != nullptr) {
		Engine::GetInstance().textures->UnLoad(notificationBgTexture);
		notificationBgTexture = nullptr;
	}
	return true;
}

void Hud::ShowNotification(const std::string& message) {
	notificationText = message;
	notificationTimer = NOTIFICATION_DURATION;
}

void Hud::DrawNotification() {
	if (notificationTimer > 0.0f && !notificationText.empty()) {
		std::string localizedText = Engine::GetInstance().languageManager->GetString(notificationText);
		Uint8 alpha = 255;

		// Desapareciendo lentamente
		if (notificationTimer < 1.0f) {
			alpha = (Uint8)(255.0f * notificationTimer);
		}

		// Tamaño de pantalla
		int screenW = Engine::GetInstance().window->windowWidth;
		int screenH = Engine::GetInstance().window->windowHeight;

		const int rectH = 110;
		const int horizontalPadding = 45;
		const int screenMargin = 40;

		SDL_Rect measuredText = Engine::GetInstance().render->MeasureText(
			localizedText.c_str(),
			FontType::CUERPO
		);

		int rectW = measuredText.w + horizontalPadding * 2;
		const int minRectW = 260;
		const int maxRectW = screenW - screenMargin * 2;
		if (rectW < minRectW) rectW = minRectW;
		if (rectW > maxRectW) rectW = maxRectW;

		// Ubicación del aviso
		int posY = screenH / 8;

		SDL_Rect bgRect = {
			screenW / 2 - rectW / 2,
			posY,
			rectW,
			rectH
		};

		if (notificationBgTexture != nullptr) {
			SDL_SetTextureBlendMode(notificationBgTexture, SDL_BLENDMODE_BLEND);
			SDL_SetTextureAlphaMod(notificationBgTexture, alpha);
			Engine::GetInstance().render->DrawTextureScaled(notificationBgTexture, bgRect);
			SDL_SetTextureAlphaMod(notificationBgTexture, 255);
		}
		else {
			Engine::GetInstance().render->DrawRectangleUnscaled(bgRect, 220, 220, 220, alpha, true, false);
		}

		// Draw texto
		SDL_Color color = { 45, 24, 16, alpha };
		SDL_Rect textBounds = {
			bgRect.x + horizontalPadding,
			bgRect.y,
			bgRect.w - horizontalPadding * 2,
			bgRect.h
		};

		Engine::GetInstance().render->DrawTextCenteredWrapped(
			localizedText.c_str(),
			textBounds,
			color,
			FontType::CUERPO
		);
	}
}

void Hud::ShowTutorial(TutorialType type) {
	std::string instructionKey;

	switch (type) {
	case TutorialType::WALK:
		instructionKey = "TUTORIAL_WALK";
		break;
	case TutorialType::JUMP:
		instructionKey = "TUTORIAL_JUMP";
		break;
	case TutorialType::GLIDE:
		instructionKey = "TUTORIAL_GLIDE";
		break;
	case TutorialType::DASH:
		instructionKey = "TUTORIAL_DASH";
		break;
	case TutorialType::ATTACK:
		instructionKey = "TUTORIAL_ATTACK";
		break;
	case TutorialType::DOUBLE_JUMP:
		instructionKey = "TUTORIAL_DOUBLE_JUMP";
		break;
	case TutorialType::WALL_JUMP:
		instructionKey = "TUTORIAL_WALL_JUMP";
		break;
	default:
		return;
	}

	Engine::GetInstance().dialogueManager->StartTutorial(instructionKey);
}

void Hud::TriggerBossIntro(SDL_Texture* portraitTex, AnimationSet* anim, float duration) {
	activeBossPortraitTex = portraitTex;
	activeBossPortraitAnim = anim;
	bossIntroTimer = duration;
	bossIntroTotalDuration = duration;
	isBossIntroActive = true;
}

void Hud::DrawBossIntro(float dt) {
	if (!isBossIntroActive) return;

	bossIntroTimer -= dt / 1000.0f;

	if (bossIntroTimer <= 0.0f) {
		isBossIntroActive = false;
		return;
	}

	if (activeBossPortraitAnim != nullptr) {
		activeBossPortraitAnim->Update(dt);
	}

	//FADE
	Uint8 alpha = 255;
	float fadeDuration = 0.5f;
	float elapsedTime = bossIntroTotalDuration - bossIntroTimer;

	if (elapsedTime < fadeDuration) {
		alpha = (Uint8)(255.0f * (elapsedTime / fadeDuration));
	}
	else if (bossIntroTimer < fadeDuration) {
		alpha = (Uint8)(255.0f * (bossIntroTimer / fadeDuration));
	}

	if (activeBossPortraitTex != nullptr) {
		SDL_SetTextureAlphaMod(activeBossPortraitTex, alpha);
	}

	// Posición
	int screenW = Engine::GetInstance().window->windowWidth;
	int screenH = Engine::GetInstance().window->windowHeight;
	int portraitW = 800;
	int portraitH = 150;

	SDL_Rect dstRect = { (screenW - portraitW) / 2, 60, portraitW, portraitH };

	//Renderizado
	if (activeBossPortraitTex != nullptr && activeBossPortraitAnim != nullptr) {
		SDL_Rect srcFrame = activeBossPortraitAnim->GetCurrentFrame();
		Engine::GetInstance().render->DrawTextureScaledSection(activeBossPortraitTex, srcFrame, dstRect);
	}

	if (activeBossPortraitTex != nullptr) {
		SDL_SetTextureAlphaMod(activeBossPortraitTex, 255);
	}
}
