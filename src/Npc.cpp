#include "Npc.h"
#include "Engine.h"
#include "Textures.h"
#include "Physics.h"
#include "Input.h"
#include "Render.h"
#include "DialogueManager.h"
#include "EntityManager.h"
#include "Log.h"

Npc::Npc() : Entity(EntityType::NPC) {
    name = "Npc";
    pbody = nullptr;
    texture = nullptr;
}

Npc::~Npc() {}

bool Npc::Awake() {
    return true;
}

bool Npc::Start() {
    texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/NPCs/Npc1.png");

    texW = 128;
    texH = 128;

    pbody = Engine::GetInstance().physics->CreateRectangleSensor((int)position.getX() - texW /2, (int)position.getY() - texH /2, texW * 2, texH * 2, bodyType::STATIC);
    // TODO ponerlo bien

    pbody->listener = this;
    pbody->ctype = ColliderType::NPC;

    return true;
}

bool Npc::Update(float dt) {
    if (!active) return true;

    // Si el jugador está cerca y no está activo
    if (isPlayerInRange && !Engine::GetInstance().dialogueManager->IsDialogueActive()) {

        // Si el jugador pulsa 'W' 
        if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_W) == KEY_DOWN) {
            Engine::GetInstance().dialogueManager->StartDialogue(dialogueID);
            Engine::GetInstance().input->ClearMouseInput(); // Limpiar inputs residuales
        }
    }

    int x, y;
    pbody->GetPosition(x, y);
    position.setX((float)x);
    position.setY((float)y);

    // Dibujamos el NPC 
    if (texture != nullptr) {
        Engine::GetInstance().render->DrawTexture(texture, x - (texW), y - (texH / 2), nullptr, 1.0f, 0.0, INT_MAX, INT_MAX);
    }

   
    if (isPlayerInRange && !Engine::GetInstance().dialogueManager->IsDialogueActive()) {
        // TODO: Draw Icono de hablar
    }

    return true;
}

bool Npc::CleanUp() {
    if (texture) Engine::GetInstance().textures->UnLoad(texture);
    if (pbody) {
        Engine::GetInstance().physics->DeletePhysBody(pbody);
        pbody = nullptr;
    }
    return true;
}

void Npc::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) {
    if (physB->ctype == ColliderType::PLAYER) {
        isPlayerInRange = true;
        LOG("Jugador en rango del NPC");
    }
}

void Npc::OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) {
    if (physB->ctype == ColliderType::PLAYER) {
        isPlayerInRange = false;
        LOG("Jugador salio del rango del NPC");
    }
}