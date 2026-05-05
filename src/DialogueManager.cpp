#include "DialogueManager.h"
#include "Engine.h"
#include "Input.h"
#include "Log.h"
#include "DialogueBox.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

DialogueManager::DialogueManager() : Module() { name = "dialogue_manager"; }
DialogueManager::~DialogueManager() {}

bool DialogueManager::Awake() {
    std::ifstream file("Assets/Dialogues/dialogues.json");
    if (!file.is_open()) {
        LOG("Error: No se pudo abrir dialogues.json");
        return true; // Retornamos true para no crashear el motor entero
    }

    json j;
    file >> j;

    for (auto& element : j.items()) {

        std::string dialogueID = element.key();
        auto linesArray = element.value();

        std::vector<DialogueLine> lines;

        for (auto& lineObj : linesArray) {
            DialogueLine dl;
            dl.speaker = lineObj["speaker"].get<std::string>();
            dl.text = lineObj["text"].get<std::string>();
            lines.push_back(dl);
        }

        dialogueDB[dialogueID] = lines;
    }

    LOG("DialogueManager: Dialogos cargados correctamente.");
    return true;
}

void DialogueManager::StartDialogue(const std::string& dialogueID) {
    LOG("Intentando cargar el dialogo con ID: '[%s]'", dialogueID.c_str());
    if (dialogueDB.find(dialogueID) != dialogueDB.end()) {
        currentConversation = dialogueDB[dialogueID];
        currentLineIndex = 0;
        isActive = true;

        displayedText = "";
        charIndex = 0;
        typeTimer = 0.0f;

        // Mostramos la caja UI y enviamos el nombre inicial
        if (uiBox) {
            uiBox->visible = true;
            uiBox->SetSpeakerName(currentConversation[0].speaker);
            uiBox->SetDialogueText("");
        }
    }
    else {
        LOG("Warning: Dialogo %s no encontrado.", dialogueID.c_str());
    }
}

bool DialogueManager::Update(float dt) {
    if (!isActive) return true;

    if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN) {
        NextLine();
        return true;
    }

    const std::string& fullText = currentConversation[currentLineIndex].text;

    // Typewriter effect
    if (charIndex < fullText.length()) {

        typeTimer += dt / 1000.0f;

        if (typeTimer >= timePerChar) {
            typeTimer = 0.0f;
            displayedText += fullText[charIndex];
            charIndex++;

            if (uiBox) {
                uiBox->SetDialogueText(displayedText);
            }
        }
    }

    return true;
}

void DialogueManager::NextLine() {
    const std::string& fullText = currentConversation[currentLineIndex].text;

    if (charIndex < fullText.length()) {
        charIndex = fullText.length();
        displayedText = fullText;
        if (uiBox) uiBox->SetDialogueText(displayedText);
    }
    else {
        currentLineIndex++;

        if (currentLineIndex < currentConversation.size()) {
            displayedText = "";
            charIndex = 0;
            typeTimer = 0.0f;
            if (uiBox) {
                uiBox->SetSpeakerName(currentConversation[currentLineIndex].speaker);
                uiBox->SetDialogueText("");
            }
        }
        else {
            isActive = false;
            if (uiBox) uiBox->visible = false;
        }
    }
}

bool DialogueManager::CleanUp() {
    dialogueDB.clear();
    uiBox = nullptr;
    return true;
}