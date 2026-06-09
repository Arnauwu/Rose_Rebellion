#include "UIDialogueBox.h"
#include "Engine.h"
#include "EntityManager.h"
#include "Player.h"
#include "Render.h"
#include "Window.h"
#include "Textures.h"

#include "LOG.h"
#include <algorithm>
#include <vector>

UIDialogueBox::UIDialogueBox(int id, float anchorX, float anchorY, float wPercent, float hPercent, const char* text)
	: UIElement(UIElementType::DIALOGUE_BOX, id, anchorX, anchorY, wPercent, hPercent, text) {
	visible = false;
}

UIDialogueBox::~UIDialogueBox() {}
void UIDialogueBox::SetBackgroundTexture(SDL_Texture* bgTex) {
	backgroundTex = bgTex;
}

void UIDialogueBox::SetTutorialBackgroundTexture(SDL_Texture* bgTex) {
	tutorialBackgroundTex = bgTex;
}

bool UIDialogueBox::Update(float dt) {
	if (!visible) return true;
	return true;
}

void UIDialogueBox::AddPortrait(const std::string& speakerName, SDL_Texture* portraitTex) {
	portraits[speakerName] = portraitTex;
}

void UIDialogueBox::SetSpeakerName(const std::string& name) {
	currentSpeaker = name;
	auto it = portraits.find(name);
	if (it != portraits.end()) {
		currentPortrait = it->second;
	}
	else {
		currentPortrait = nullptr;
	}

	if (!currentSpeaker.empty()) {
		SDL_Rect mainBoxPlaceholder = { 0, 0, 1400, 240 };
		cachedNameTextRect = Engine::GetInstance().render->GetTextRenderedBounds(currentSpeaker.c_str(), mainBoxPlaceholder, FontType::SPEAKER);
	}
}

void UIDialogueBox::Draw() const {
	if (!visible || backgroundTex == nullptr) return;

	int screenW = Engine::GetInstance().window->windowWidth;
	int screenH = Engine::GetInstance().window->windowHeight;

	if (tutorialMode) {
		SDL_Rect tutorialBox = GetTutorialBoxRect();
		SDL_Texture* tutorialTexture =
			tutorialBackgroundTex != nullptr ? tutorialBackgroundTex : backgroundTex;

		// 1. Draw tutorial box first.
		Engine::GetInstance().render->DrawTextureScaled(tutorialTexture, tutorialBox);

		// 2. Draw Rosa portrait after the box, so it appears in front.
		if (currentPortrait != nullptr) {
			int portraitSize = (int)(screenH * 0.34f);

			int portraitX = tutorialBox.x - (int)(portraitSize / 1.9f);
			int portraitY = tutorialBox.y + tutorialBox.h - portraitSize + (int)(screenH * 0.04f);

			if (portraitX < 0) {
				portraitX = 0;
			}

			SDL_Rect portraitRect = {
				portraitX,
				portraitY,
				portraitSize,
				portraitSize
			};

			Engine::GetInstance().render->DrawTextureScaled(currentPortrait, portraitRect);
		}

		// 3. Draw tutorial text last, so text is also on top.
		if (!currentText.empty()) {
			DrawTutorialContent(tutorialBox);
		}

		return;
	}

	SDL_Rect mainBox;
	mainBox.w = (int)(screenW * 0.70f);
	mainBox.h = (int)(screenH * 0.25f);
	mainBox.x = (screenW - mainBox.w) / 2;
	mainBox.y = screenH - mainBox.h - (int)(screenH * 0.05f);

	// Dibujamos el fondo principal
	Engine::GetInstance().render->DrawTexture9Slice(backgroundTex, mainBox, 64, 64, 64, 64);

	// CAJA SPEAKER (Nombre del personaje)
	if (!currentSpeaker.empty()) {

		SDL_Rect nameBox;
		nameBox.w = (int)(mainBox.w * 0.25f);
		nameBox.h = (int)(mainBox.h * 0.30f);
		nameBox.y = mainBox.y - nameBox.h;

		// LÓGICA DE POSICIÓN
		if (currentSpeaker == "Rose" || currentSpeaker == "Rose-") {
			nameBox.x = mainBox.x + (int)(mainBox.w * 0.05f);
		}
		else {
			nameBox.x = (mainBox.x + mainBox.w) - nameBox.w - (int)(mainBox.w * 0.05f);
		}

		// Dibujamos el fondo del nombre
		Engine::GetInstance().render->DrawTexture9Slice(backgroundTex, nameBox, 0, 0, 0, 0);

		int nameTextX = nameBox.x + (nameBox.w - cachedNameTextRect.w) / 2;
		int nameTextY = nameBox.y + (nameBox.h - cachedNameTextRect.h) / 2;

		Engine::GetInstance().render->DrawText(currentSpeaker.c_str(), nameTextX, nameTextY, 0, 0, speakerColor, FontType::DIALOGUE);
	}

	// CAJA DEL DIÁLOGO
	if (!currentText.empty()) {
		int textX = 0;
		int textY = mainBox.y + (int)(mainBox.h * 0.2f);
		int maxW = 0;
		int maxH = (int)(mainBox.h * 0.85f);

		if (currentSpeaker == "Rose" || currentSpeaker == "Rose-") {
			textX = mainBox.x + (int)(mainBox.w * 0.15f);
			maxW = (int)(mainBox.w * 0.8f);
		}
		else {
			textX = mainBox.x + (int)(mainBox.w * 0.05f);
			maxW = (int)(mainBox.w * 0.8f);
		}

		Engine::GetInstance().render->DrawText(currentText.c_str(), textX, textY, maxW, maxH, textColor, FontType::CUERPO);
	}

	// TEXTURA PORTRAIT
	if (currentPortrait != nullptr) {
		int portraitSize = (int)(screenH * 0.5f);
		int space = (int)(screenH * 0.05f);

		int portraitY = (mainBox.y + mainBox.h) - portraitSize + space;
		int portraitX = 0;

		if (currentSpeaker == "Rose" || currentSpeaker == "Rose-") {
			portraitX = mainBox.x - (portraitSize / 1.9);
		}
		else {
			portraitX = (mainBox.x + mainBox.w) - (portraitSize / 2);
		}

		SDL_Rect destRect = { portraitX, portraitY, portraitSize, portraitSize };

		Engine::GetInstance().render->DrawTextureScaled(currentPortrait, destRect);
	}
}

bool UIDialogueBox::CleanUp() {
	pendingToDelete = true;
	for (auto it = portraits.begin(); it != portraits.end(); ++it) {
		SDL_Texture* texture = it->second;
		if (texture != nullptr) {
			Engine::GetInstance().textures->UnLoad(texture);
		}
	}
	portraits.clear();
	currentPortrait = nullptr;
	return true;
}

void UIDialogueBox::SetDialogueText(const std::string& text) { currentText = text; }

void UIDialogueBox::SetTutorialMode(bool enabled) {
	tutorialMode = enabled;
}

SDL_Rect UIDialogueBox::GetTutorialBoxRect() const {
	const int screenW = Engine::GetInstance().window->windowWidth;
	const int screenH = Engine::GetInstance().window->windowHeight;
	const int screenMargin = 20;

	SDL_Texture* tutorialTexture =
		tutorialBackgroundTex != nullptr ? tutorialBackgroundTex : backgroundTex;

	float texW = 557.0f;
	float texH = 372.0f;

	if (tutorialTexture != nullptr) {
		SDL_GetTextureSize(tutorialTexture, &texW, &texH);
	}

	float aspect = texH / texW;

	int boxW = (int)(screenW * 0.38f);
	boxW = std::max(420, std::min(boxW, 600));

	int boxH = (int)(boxW * aspect);

	int centerX = screenW / 2;
	int playerScreenY = screenH / 2;

	Player* player = Engine::GetInstance().entityManager->GetPlayer();
	if (player != nullptr) {
		Vector2D playerPosition = player->GetPosition();
		const int scale = Engine::GetInstance().window->GetScale();
		const float zoom = Engine::GetInstance().render->GetZoom();
		const SDL_Rect& camera = Engine::GetInstance().render->camera;

		centerX = (int)((camera.x + playerPosition.getX() * scale) * zoom);
		playerScreenY = (int)((camera.y + playerPosition.getY() * scale) * zoom);
	}

	int portraitSize = (int)(screenH * 0.34f);
	int leftReservedSpace = (int)(portraitSize * 0.50f);

	int boxX = centerX - boxW / 2 + leftReservedSpace / 2;
	int boxY = playerScreenY - boxH - (int)(screenH * 0.24f);

	boxX = std::max(screenMargin + leftReservedSpace, std::min(boxX, screenW - boxW - screenMargin));
	boxY = std::max(screenMargin, std::min(boxY, screenH - boxH - screenMargin));

	return { boxX, boxY, boxW, boxH };
}

void UIDialogueBox::DrawTutorialContent(const SDL_Rect& mainBox) const {
	std::string instruction = currentText;
	std::string keyText;
	std::string confirmation;

	size_t firstBreak = currentText.find('\n');
	if (firstBreak != std::string::npos) {
		instruction = currentText.substr(0, firstBreak);
		size_t confirmationBreak = currentText.find("\n\n", firstBreak + 1);

		if (confirmationBreak == std::string::npos) {
			keyText = currentText.substr(firstBreak + 1);
		}
		else {
			keyText = currentText.substr(firstBreak + 1, confirmationBreak - firstBreak - 1);
			confirmation = currentText.substr(confirmationBreak + 2);
		}
	}

	SDL_Rect instructionBounds = {
	mainBox.x + (int)(mainBox.w * 0.18f),
	mainBox.y + (int)(mainBox.h * 0.10f),
	(int)(mainBox.w * 0.72f),
	(int)(mainBox.h * 0.34f)
	};

	Engine::GetInstance().render->DrawTextCenteredWrapped(
		instruction.c_str(), instructionBounds, textColor, FontType::CUERPO);

	if (!keyText.empty()) {
		SDL_Rect keyBounds = {
		mainBox.x + (int)(mainBox.w * 0.18f),
		mainBox.y + (int)(mainBox.h * 0.45f),
		(int)(mainBox.w * 0.72f),
		(int)(mainBox.h * 0.27f)
		};
		DrawTutorialKeys(keyText, keyBounds);
	}

	if (!confirmation.empty()) {
		SDL_Rect confirmationBounds = {
		mainBox.x + (int)(mainBox.w * 0.18f),
		mainBox.y + (int)(mainBox.h * 0.76f),
		(int)(mainBox.w * 0.72f),
		(int)(mainBox.h * 0.16f)
		};
		Engine::GetInstance().render->DrawTextCentered(
			confirmation.c_str(), confirmationBounds, textColor, FontType::CUERPO);
	}
}

void UIDialogueBox::DrawTutorialKeys(const std::string& keyText, const SDL_Rect& bounds) const {
	struct KeyPart {
		std::string text;
		bool isKey;
		int width;
	};

	std::vector<KeyPart> parts;
	auto addKey = [&parts](const std::string& text) {
		int width = std::max(58, (int)text.length() * 13 + 28);
		parts.push_back({ text, true, width });
		};
	auto addSeparator = [&parts](const std::string& text) {
		parts.push_back({ text, false, 34 });
		};

	size_t plusPos = keyText.find(" + ");
	size_t slashPos = keyText.find(" / ");
	size_t repeatPos = keyText.rfind(" x");

	if (plusPos != std::string::npos) {
		addKey(keyText.substr(0, plusPos));
		addSeparator("+");
		addKey(keyText.substr(plusPos + 3));
	}
	else if (slashPos != std::string::npos) {
		addKey(keyText.substr(0, slashPos));
		addSeparator("/");
		addKey(keyText.substr(slashPos + 3));
	}
	else if (repeatPos != std::string::npos) {
		addKey(keyText.substr(0, repeatPos));
		addSeparator(keyText.substr(repeatPos + 1));
	}
	else {
		addKey(keyText);
	}

	const int spacing = 10;
	int totalWidth = 0;
	for (const KeyPart& part : parts) totalWidth += part.width;
	totalWidth += spacing * ((int)parts.size() - 1);

	int currentX = bounds.x + (bounds.w - totalWidth) / 2;
	int keyHeight = std::min(52, bounds.h - 8);
	int keyY = bounds.y + (bounds.h - keyHeight) / 2;
	auto render = Engine::GetInstance().render;

	for (const KeyPart& part : parts) {
		SDL_Rect partRect = { currentX, keyY, part.width, keyHeight };
		if (part.isKey) {
			SDL_Rect shadowRect = { partRect.x + 3, partRect.y + 4, partRect.w, partRect.h };
			render->DrawRectangleUnscaled(shadowRect, 55, 25, 18, 255, true, false);
			render->DrawRectangleUnscaled(partRect, 116, 58, 42, 255, true, false);
			render->DrawRectangleUnscaled(partRect, 238, 205, 148, 255, false, false);

			SDL_Rect textBounds = { partRect.x + 6, partRect.y + 3, partRect.w - 12, partRect.h - 6 };
			SDL_Color keyColor = { 255, 239, 196, 255 };
			render->DrawTextCentered(part.text.c_str(), textBounds, keyColor, FontType::CUERPO);
		}
		else {
			render->DrawTextCentered(part.text.c_str(), partRect, textColor, FontType::CUERPO);
		}
		currentX += part.width + spacing;
	}
}
