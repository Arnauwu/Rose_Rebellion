#include "DialogueTrigger.h"
#include "Engine.h"
#include "Physics.h"
#include "DialogueManager.h"
#include "Log.h"

DialogueTrigger::DialogueTrigger() : Entity(EntityType::DIALOGUE_TRIGGER) {
    name = "DialogueTrigger";
}

DialogueTrigger::~DialogueTrigger() {}

bool DialogueTrigger::Start() {

    pbody = Engine::GetInstance().physics->CreateRectangleSensor((int)position.getX(), (int)position.getY(), width, height, bodyType::STATIC);

    if (pbody != nullptr) {
        pbody->listener = this;
        pbody->ctype = ColliderType::DIALOGUE_TRIGGER; // <--- AÑADE ESTO
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

        Engine::GetInstance().dialogueManager->StartDialogue(dialogueID, true);

        if (pbody != nullptr) {
            Engine::GetInstance().physics->DeletePhysBody(pbody);
            pbody = nullptr;
        }
    }
}