#include "GameManager.h"
#include "Log.h"

GameManager::GameManager() : Module()
{
    name = "game_manager";
}

GameManager::~GameManager()
{
}

bool GameManager::Awake()
{
    LOG("Initializing Game Manager");
    return true;
}

bool GameManager::Start()
{
    return true;
}

bool GameManager::CleanUp()
{
    LOG("Freeing Game Manager");
    ResetSession();
    return true;
}

// ============================================================================
// GESTIÓN DE SESIÓN Y PROGRESO
// ============================================================================

void GameManager::ResetSession()
{
    collectedItems.clear();
    openedDoors.clear();
    LOG("Game session has been reset.");
}

// --- INVENTOY ---

void GameManager::AddItem(const std::string& itemID)
{
    // .insert() maneja automáticamente los duplicados (no añadirá dos veces la misma llave)
    auto result = collectedItems.insert(itemID);
    if (result.second) {
        LOG("GameManager: Item added -> %s", itemID.c_str());
    }
}

bool GameManager::HasItem(const std::string& itemID) const
{
    return collectedItems.find(itemID) != collectedItems.end();
}

// WORLD EVENTS

void GameManager::AddOpenedDoor(const std::string& doorID)
{
    auto result = openedDoors.insert(doorID);
    if (result.second) {
        LOG("GameManager: Door opened -> %s", doorID.c_str());
    }
}

bool GameManager::HasOpenedDoor(const std::string& doorID) const
{
    return openedDoors.find(doorID) != openedDoors.end();
}