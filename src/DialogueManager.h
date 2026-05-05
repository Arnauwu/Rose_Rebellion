#pragma once
#include "Module.h"
#include "DialogueData.h"
#include <string>
#include <vector>
#include <unordered_map>

class UIDialogueBox;

class DialogueManager : public Module {
public:
    DialogueManager();
    ~DialogueManager();

    bool Awake() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    void StartDialogue(const std::string& dialogueID);
    void NextLine();
    bool IsDialogueActive() const { return isActive; }

    // Conectamos la UI al Manager
    void SetDialogueUI(UIDialogueBox* uiBox) { this->uiBox = uiBox; }

private:
    std::unordered_map<std::string, std::vector<DialogueLine>> dialogueDB;
    UIDialogueBox* uiBox = nullptr;

    std::vector<DialogueLine> currentConversation;
    int currentLineIndex = 0;
    bool isActive = false;

    // Efecto Typewriter
    std::string displayedText;
    float typeTimer = 0.0f;
    float timePerChar = 0.03f;
    size_t charIndex = 0;
};