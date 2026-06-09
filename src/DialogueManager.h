#pragma once
#include "Module.h"
#include "LanguageManager.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <string>

class UIDialogueBox;
struct DialogueLine {
    std::string speaker;
    std::string text;
};

class DialogueManager : public Module {
public:
    DialogueManager();
    ~DialogueManager();

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    void StartDialogue(const std::string& dialogueID, bool isMonologue = false);
    void StartTutorial(const std::string& textKey);
    bool IsBlockingDialogueActive() const { return isActive && !currentIsMonologue; }

    void NextLine();
    bool IsDialogueActive() const { return isActive; }
    bool CanInteract() const { return !isActive && interactionCooldown <= 0.0f; }
    void EndDialogue();
    // Conectamos la UI al Manager
    void SetDialogueUI(UIDialogueBox* uiBox) { this->uiBox = uiBox; }

private:
    void LoadDialogues(Language lang);

    Language lastLoadedLanguage;
    bool isDialoguesLoaded = false;

    std::map<std::string, std::vector<DialogueLine>> dialogueDB;
    UIDialogueBox* uiBox = nullptr;
    const std::vector<DialogueLine>* currentConversation = nullptr;
    std::vector<DialogueLine> tutorialConversation;

    int currentLineIndex = 0;
    bool isActive = false;
    bool inputLocked = false;
    float interactionCooldown = 0.0f;
    bool isWaitingForLanding = false;

    // Monologos
    bool currentIsMonologue = false;
    float monologueReadTimer = 0.0f;
    float timeToRead = 3.0f;

    // Efecto Typewriter
    std::string displayedText;
    float typeTimer = 0.0f;
    float timePerChar = 0.04f;
    size_t charIndex = 0;
};
