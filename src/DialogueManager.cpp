#include "DialogueManager.h"
#include "Engine.h"
#include "Input.h"
#include "EntityManager.h"
#include "Player.h"
#include "Log.h"
#include "UIDialogueBox.h"
#include "SceneManager.h"
#include "Physics.h"
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

	try {
		json j;
		file >> j;

		for (auto& element : j.items()) {
			std::string dialogueID = element.key();
			auto linesArray = element.value();
			std::vector<DialogueLine> lines;
			for (auto& lineObj : linesArray) {
				DialogueLine dl;
				dl.speaker = lineObj["speaker"];
				dl.text = lineObj["text"];
				lines.push_back(dl);
			}
			dialogueDB[dialogueID] = lines;
		}

		LOG("DialogueManager: Dialogos cargados correctamente desde %s", filePath.c_str());
	}
	catch (const json::parse_error& e) {
		// ¡ESTO ES CLAVE! Si el JSON tiene un error de formato, te lo dirá aquí.
		LOG("ERROR LEYENDO EL JSON: %s", e.what());
	}

	// NUEVO: Guardamos el estado para el Lazy Loading
	lastLoadedLanguage = lang;
	isDialoguesLoaded = true;
}

void DialogueManager::StartDialogue(const std::string& dialogueID, bool isMonologue) {
	Language currentLang = Engine::GetInstance().languageManager->GetCurrentLanguage();
	// Reload dialogue data when the selected language changes.
	if (!isDialoguesLoaded || currentLang != lastLoadedLanguage) {
		LoadDialogues(currentLang);
	}
	// Do not restart a conversation that is already active.
	if (isActive) { return; }

	LOG("Intentando cargar el dialogo con ID: '[%s]'", dialogueID.c_str());
	if (dialogueDB.find(dialogueID) != dialogueDB.end()) {

		// Ya no necesitamos guardar activeDialogueID
		currentConversation = &dialogueDB[dialogueID];
		currentLineIndex = 0;
		isActive = true;
		currentIsMonologue = isMonologue;
		inputLocked = !currentIsMonologue;
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
				uiBox->SetTutorialMode(false);
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

void DialogueManager::StartTutorial(const std::string& textKey) {
	if (isActive) return;

	std::string tutorialText = Engine::GetInstance().languageManager->GetString(textKey);
	std::string confirmText = Engine::GetInstance().languageManager->GetString("TUTORIAL_CONFIRM");

	tutorialConversation.clear();
	tutorialConversation.push_back({ "Rose", tutorialText + "\n\n" + confirmText });

	currentConversation = &tutorialConversation;
	currentLineIndex = 0;
	currentIsMonologue = false;
	isActive = true;
	inputLocked = true;
	isWaitingForLanding = false;
	monologueReadTimer = 0.0f;
	charIndex = 0;
	typeTimer = 0.0f;
	displayedText.clear();
	displayedText.reserve(tutorialConversation[0].text.length());

	Player* player = Engine::GetInstance().entityManager->GetPlayer();
	if (player != nullptr && player->pbody != nullptr) {
		Engine::GetInstance().physics->SetLinearVelocity(player->pbody, { 0.0f, 0.0f });
	}

	Engine::GetInstance().sceneManager->isGamePaused = true;

	if (uiBox) {
		uiBox->SetTutorialMode(true);
		uiBox->visible = true;
		uiBox->SetSpeakerName(tutorialConversation[0].speaker);
		uiBox->SetDialogueText("");
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
				uiBox->SetTutorialMode(currentConversation == &tutorialConversation);
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
		// Blocking dialogue waits for the player to press E.
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
			if (uiBox) {
				uiBox->visible = false;
				uiBox->SetTutorialMode(false);
			}
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
		uiBox->SetTutorialMode(false);
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
