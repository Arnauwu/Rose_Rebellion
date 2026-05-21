#include "UIDialogueBox.h"
#include "Engine.h"
#include "Window.h"
#include "Textures.h"

#include "LOG.h"

UIDialogueBox::UIDialogueBox(int id, float anchorX, float anchorY, float wPercent, float hPercent, const char* text)
	: UIElement(UIElementType::DIALOGUE_BOX, id, anchorX, anchorY, wPercent, hPercent, text) {
	visible = false;
}

UIDialogueBox::~UIDialogueBox() {}
void UIDialogueBox::SetBackgroundTexture(SDL_Texture* bgTex) {
backgroundTex = bgTex;}

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
		nameBox.y = mainBox.y - nameBox.h ; 

		// LÓGICA DE POSICIÓN
		if (currentSpeaker == "Rose") {
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

		if (currentSpeaker == "Rose") {
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

		if (currentSpeaker == "Rose") {
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