#pragma once
#include <string>
#include <unordered_set>
#include <vector>
#include "Vector2D.h" 

struct GameState {
    //Keys
    bool hasCastleKey = false;
    bool hasForestKey = false;
    bool hasMountainKey = false;
    bool hasCatacombsKey = false;
    bool hasBossKey = false;

    bool hasSickle = false;
    bool glideUnlocked = false;
    int currentForceOrbs = 0;
    bool doubleJumpUnlocked = false;
    bool dashUnlocked = false;
    bool wallJumpUnlocked = false;

    // Player stats
    int currentHealth = 100;
    int maxHealth = 100;

    bool stHealthUp = false;
    bool stIframesUp = false;
    bool stSpeedUp = false;
    bool stFastDash = false;
    bool stUpAttack = false;
    bool stDownAttack = false;

    // World
    std::string currentMap = "Forest_04.tmx";
    Vector2D playerPosition = { 2147.0f, 912.0f }; // Spawn inicial por defecto

    // Bosses
    bool knightBossKilled = false;
    bool ninfaBossKilled = false;
    bool lizardBossKilled = false;
    bool dragonBossKilled = false;

    std::vector<std::string> triggeredDialogues;
    std::vector<std::string> openedDoors;
    std::unordered_set<std::string> collectedItems;
    std::vector<std::string> exploredRooms;
};

class GameManager {
public:
    static GameManager& GetInstance() {
        static GameManager instance;
        return instance;
    }

    // Start a game from scratch
    void StartNewGame();

    // Save and load system
    bool SaveGame(const std::string& filename = "savegame.xml");
    bool LoadGame(const std::string& filename = "savegame.xml");

    // Direct access to the current status
    GameState gameState;

private:
    GameManager() {} // Singleton
    ~GameManager() {}
};