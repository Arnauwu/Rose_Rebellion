#pragma once
#include "Entity.h"
#include "Animation.h"
#include <string>

class Npc : public Entity {
public:

    Npc();
    ~Npc();

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    void OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) override;
    void OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) override;

    // Asignación de dialogo y acceso al dialogo
    void SetDialogueID(const std::string& id) { dialogueID = id; }
    std::string GetDialogueID() const { return dialogueID; }

private:
    std::string dialogueID;
    bool isPlayerInRange = false;
    PhysBody* pbody;

    // Variables visuales
    SDL_Texture* texture = nullptr;
    int texW, texH;
    AnimationSet anims;
};