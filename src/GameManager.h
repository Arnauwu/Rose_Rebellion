#pragma once

#include "Module.h"
#include <set>
#include <string>

class GameManager : public Module
{
public:
    GameManager();
    virtual ~GameManager();

    // Module overrides
    bool Awake() override;
    bool Start() override;
    bool CleanUp() override;


    // Session
    void ResetSession();

    // Inventory
    void AddItem(const std::string& itemID);
    bool HasItem(const std::string& itemID) const;

    // World state
    void AddOpenedDoor(const std::string& doorID);
    bool HasOpenedDoor(const std::string& doorID) const;

private:
    std::set<std::string> collectedItems;
    std::set<std::string> openedDoors;
};