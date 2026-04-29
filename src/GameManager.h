#pragma once
#include <string>
#include <unordered_set>
#include <vector>
#include "Vector2D.h" 

struct GameState {
    int keyCount = 0;
    bool hasSickle = false;
    bool glideUnlocked = false;
    int currentForceOrbs = 0;

    // Player stats
    int currentHealth = 100;
    int maxHealth = 100;

    // World
    std::string currentMap = "Castle_Room_Princess.tmx";
    Vector2D playerPosition = { 2147.0f, 912.0f }; // Spawn inicial por defecto

    std::vector<std::string> openedDoors;
    std::unordered_set<std::string> collectedItems;
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