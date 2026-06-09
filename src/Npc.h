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

    void ConfigNPC(const std::string& texPath, const std::string& dialogID, int width = 128, int height = 128, const std::string& tsxPath = ""); 
    void SetDialogueID(const std::string& id) { dialogueID = id; }
    std::string GetDialogueID() const { return dialogueID; }

private:
    std::string dialogueID;
    bool isPlayerInRange = false;
    PhysBody* pbody;

    // Variables visuales
    std::string texturePath = "Assets/Textures/Entities/NPCs/Npc1.png";
    SDL_Texture* texture = nullptr;
    int texW, texH;

    std::string tsxPath;
    AnimationSet anims;
    AnimationSet interactionBackgroundAnimation;
    SDL_Texture* interactionBackgroundTexture = nullptr;
    SDL_Texture* interactIcon = nullptr;
    float iconTimer = 0.0f;
};
