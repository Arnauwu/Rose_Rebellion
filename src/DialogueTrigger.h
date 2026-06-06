#pragma once
#include "Entity.h"
#include <string>

class DialogueTrigger : public Entity {
public:
    DialogueTrigger();
    virtual ~DialogueTrigger();

    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    void OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) override;

public:
    std::string dialogueID = "";
    int width = 128;
    int height = 128;

private:
    PhysBody* pbody = nullptr;
    bool hasTriggered = false;
};