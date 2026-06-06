#include "DialogueTrigger.h"
#include "Engine.h"
#include "Physics.h"
#include "DialogueManager.h"
#include "GameManager.h" 
#include "Log.h"
#include <algorithm>

DialogueTrigger::DialogueTrigger() : Entity(EntityType::DIALOGUE_TRIGGER) {
    name = "DialogueTrigger";
}

DialogueTrigger::~DialogueTrigger() {}

bool DialogueTrigger::Start() {

    auto& triggeredList = GameManager::GetInstance().gameState.triggeredDialogues;
    if (std::find(triggeredList.begin(), triggeredList.end(), dialogueID) != triggeredList.end()) {
        hasTriggered = true;
        return true;
    }
    pbody = Engine::GetInstance().physics->CreateRectangleSensor((int)position.getX(), (int)position.getY(), width, height, bodyType::STATIC);

    if (pbody != nullptr) {
        pbody->listener = this;
        pbody->ctype = ColliderType::DIALOGUE_TRIGGER;
    }
    return true;
}

bool DialogueTrigger::Update(float dt) {
    return true; 
}

bool DialogueTrigger::CleanUp() {
    if (pbody != nullptr) {
        Engine::GetInstance().physics->DeletePhysBody(pbody);
        pbody = nullptr;
    }
    return true;
}

void DialogueTrigger::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) {
    if (!hasTriggered && physB->ctype == ColliderType::PLAYER) {
        hasTriggered = true;
        LOG("Trigger de dialogo activado: %s", dialogueID.c_str());
        GameManager::GetInstance().gameState.triggeredDialogues.push_back(dialogueID);
       
        Engine::GetInstance().dialogueManager->StartDialogue(dialogueID, true);

        if (pbody != nullptr) {
            Engine::GetInstance().physics->DeletePhysBody(pbody);
            pbody = nullptr;
        }
    }
}