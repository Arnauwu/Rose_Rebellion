#include "DialogueManager.h"
#include "Engine.h"
#include "Input.h"
#include "EntityManager.h"
#include "Player.h"
#include "Log.h"
#include "UIDialogueBox.h"
#include "SceneManager.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

DialogueManager::DialogueManager() : Module() { name = "dialogue_manager"; }
DialogueManager::~DialogueManager() {}

bool DialogueManager::Awake() {

	LOG("DialogueManager: Awake completado.");
	return true;
}

bool DialogueManager::Start() {
	Language currentLang = Engine::GetInstance().languageManager->GetCurrentLanguage();
	LoadDialogues(currentLang);

	Engine::GetInstance().languageManager->RegisterLanguageChangeCallback(
		[this](Language lang) { this->LoadDialogues(lang); }
	);
	return true;
}

void DialogueManager::LoadDialogues(Language lang) {
	dialogueDB.clear();

	std::string filePath;
	if (lang == Language::CATALAN) {
		filePath = "Assets/Dialogues/dialogues_cat.json";
	}
	else {
		filePath = "Assets/Dialogues/dialogues_en.json";
	}

	std::ifstream file(filePath);
	if (!file.is_open()) {
		LOG("Error: No se pudo abrir %s", filePath.c_str());
		return;
	}

	json j;
	file >> j;

	for (auto& element : j.items()) {
		std::string dialogueID = element.key();
		auto linesArray = element.value();
		std::vector<DialogueLine> lines;
		for (auto& lineObj : linesArray) {
			DialogueLine dl;

			// CORRECCIÓN SINTAXIS: Usamos asignación directa para no confundir a Visual Studio
			dl.speaker = lineObj["speaker"];
			dl.text = lineObj["text"];

			lines.push_back(dl);
		}
		dialogueDB[dialogueID] = lines;
	}

	LOG("DialogueManager: Dialogos cargados correctamente desde %s", filePath.c_str());

	// NUEVO: Guardamos el estado para el Lazy Loading
	lastLoadedLanguage = lang;
	isDialoguesLoaded = true;
}

void DialogueManager::StartDialogue(const std::string& dialogueID, bool isMonologue) {
	Language currentLang = Engine::GetInstance().languageManager->GetCurrentLanguage();

	// Si no hemos cargado nada aún, o si el idioma del juego ha cambiado desde la última vez...
	if (!isDialoguesLoaded || currentLang != lastLoadedLanguage) {
		LoadDialogues(currentLang);
	}

	// 2. Evitar reiniciar una conversación ya activa
	if (isActive) { return; }

	LOG("Intentando cargar el dialogo con ID: '[%s]'", dialogueID.c_str());
	if (dialogueDB.find(dialogueID) != dialogueDB.end()) {

		// Ya no necesitamos guardar activeDialogueID
		currentConversation = &dialogueDB[dialogueID];
		currentLineIndex = 0;
		isActive = true;
		inputLocked = !currentIsMonologue;

		currentIsMonologue = isMonologue;
		monologueReadTimer = 0.0f;

		charIndex = 0;
		typeTimer = 0.0f;

		displayedText.clear();

		const std::string& fullText = (*currentConversation)[0].text;
		displayedText.reserve(fullText.length());

		Player* player = Engine::GetInstance().entityManager->GetPlayer();

		if (player != nullptr && !player->onGround && !currentIsMonologue) {
			isWaitingForLanding = true;
			if (uiBox) uiBox->visible = false;
		}
		else {
			if (uiBox) {
				uiBox->visible = true;
				uiBox->SetSpeakerName((*currentConversation)[0].speaker);
				uiBox->SetDialogueText("");
			}

			if (!currentIsMonologue) {
				Engine::GetInstance().sceneManager->isGamePaused = true;
			}
		}
	}
}
bool DialogueManager::Update(float dt) {
	if (!isActive) {
		if (interactionCooldown > 0.0f) {
			interactionCooldown -= dt / 1000.0f;
		}
		return true;
	}

	if (isWaitingForLanding) {
		Player* player = Engine::GetInstance().entityManager->GetPlayer();

		if (player != nullptr && player->onGround) {
			isWaitingForLanding = false;
			Engine::GetInstance().sceneManager->isGamePaused = true;

			if (uiBox) {
				uiBox->visible = true;
				uiBox->SetSpeakerName((*currentConversation)[0].speaker);
				uiBox->SetDialogueText("");
			}
		}
		else {
			return true;
		}
	}
	if (!currentIsMonologue) {
		// Lógica antigua: Esperar a que el jugador pulse 'E'
		if (inputLocked) {
			if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_E) == KEY_UP ||
				Engine::GetInstance().input->GetKey(SDL_SCANCODE_E) == KEY_IDLE) {
				inputLocked = false;
			}
		}
		else {
			if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_E) == KEY_DOWN) {
				NextLine();
				return true;
			}
		}
	}

	const std::string& fullText = (*currentConversation)[currentLineIndex].text;

	if (charIndex < fullText.length()) {

		typeTimer += dt / 1000.0f;

		if (typeTimer >= timePerChar) {
			typeTimer = 0.0f;

			displayedText += fullText[charIndex];

			charIndex++;
			if (uiBox) uiBox->SetDialogueText(displayedText);
		}
	}
	else if (currentIsMonologue) {
		monologueReadTimer += dt / 1000.0f;

		if (monologueReadTimer >= timeToRead) {
			monologueReadTimer = 0.0f;
			NextLine();
		}
	}

	return true;
}

void DialogueManager::NextLine() {
	const std::string& fullText = (*currentConversation)[currentLineIndex].text;
	if (charIndex < fullText.length()) {
		charIndex = fullText.length();
		displayedText = fullText;
		if (uiBox) uiBox->SetDialogueText(displayedText);
	}
	else {
		currentLineIndex++;

		if (currentLineIndex < currentConversation->size()) {
			charIndex = 0;
			typeTimer = 0.0f;

			displayedText.clear();
			const std::string& newFullText = (*currentConversation)[currentLineIndex].text;
			displayedText.reserve(newFullText.length());

			if (uiBox) {
				uiBox->SetSpeakerName((*currentConversation)[currentLineIndex].speaker);
				uiBox->SetDialogueText("");
			}
		}
		else {
			isActive = false;
			interactionCooldown = 1.0f;
			if (uiBox) uiBox->visible = false;
			Engine::GetInstance().sceneManager->isGamePaused = false;
		}
	}
}

void DialogueManager::EndDialogue() {
	isActive = false;
	currentLineIndex = 0;
	charIndex = 0;
	displayedText = "";

	if (uiBox) {
		uiBox->visible = false;
		uiBox->SetDialogueText("");
		uiBox->SetSpeakerName("");
	}
	Engine::GetInstance().sceneManager->isGamePaused = false;
}
bool DialogueManager::CleanUp() {
	dialogueDB.clear();
	uiBox = nullptr;
	return true;
}