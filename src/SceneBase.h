#pragma once

#include "Vector2D.h"

class UIElement;
class Player;

// Abstract base class for all game scenes
class SceneBase {
public:
    SceneBase() : exitGame(false) {}
    virtual ~SceneBase() {}

    // Lifecycle methods to be overridden
    virtual bool Awake() { return true; }
    virtual bool Start() { return true; }
    virtual bool PreUpdate() { return true; }
    virtual bool Update(float dt) = 0; // Every derived scene MUST implement it
    virtual bool PostUpdate() { return true; }
    virtual bool CleanUp() = 0;

    // UI Event delegation
    virtual bool OnUIMouseClickEvent(UIElement* uiElement) { return true; }

    virtual Vector2D GetPlayerPosition() { return Vector2D(0, 0); }
    virtual Player* GetPlayer() { return nullptr; }
    virtual void SetPlayer(Player* p) {} 
public:
    bool exitGame = false;
};