#include "GameManager.h"
#include "Log.h"
#include <fstream>
#include <iostream>

void GameManager::StartNewGame() {
    LOG("Iniciando nueva partida...");
    // Simplemente reseteamos el estado a los valores por defecto
    gameState = GameState();
}

bool GameManager::SaveGame(const std::string& filename) {
    LOG("Guardando partida en %s", filename.c_str());

    // Usamos fstream en modo binario por simplicidad (idealmente usarías JSON, ej: nlohmann/json)
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;

    // Variables simples
    file.write(reinterpret_cast<char*>(&gameState.keyCount), sizeof(gameState.keyCount));
    file.write(reinterpret_cast<char*>(&gameState.hasSickle), sizeof(gameState.hasSickle));
    file.write(reinterpret_cast<char*>(&gameState.glideUnlocked), sizeof(gameState.glideUnlocked));
    file.write(reinterpret_cast<char*>(&gameState.currentForceOrbs), sizeof(gameState.currentForceOrbs));
    file.write(reinterpret_cast<char*>(&gameState.currentHealth), sizeof(gameState.currentHealth));
    file.write(reinterpret_cast<char*>(&gameState.maxHealth), sizeof(gameState.maxHealth));

    // Guardar Strings y Vectores (requiere serialización manual de tamaño + datos)
    size_t mapLen = gameState.currentMap.size();
    file.write(reinterpret_cast<char*>(&mapLen), sizeof(mapLen));
    file.write(gameState.currentMap.c_str(), mapLen);

    // Guardar Vector2D
    file.write(reinterpret_cast<char*>(&gameState.playerPosition), sizeof(Vector2D));

    // Guardar items recolectados
    size_t itemsSize = gameState.collectedItems.size();
    file.write(reinterpret_cast<char*>(&itemsSize), sizeof(itemsSize));
    for (const auto& item : gameState.collectedItems) {
        size_t len = item.size();
        file.write(reinterpret_cast<char*>(&len), sizeof(len));
        file.write(item.c_str(), len);
    }

    file.close();
    return true;
}

bool GameManager::LoadGame(const std::string& filename) {
    LOG("Cargando partida desde %s", filename.c_str());

    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;

    // Cargar en un estado temporal por seguridad
    GameState tempState;

    file.read(reinterpret_cast<char*>(&tempState.keyCount), sizeof(tempState.keyCount));
    file.read(reinterpret_cast<char*>(&tempState.hasSickle), sizeof(tempState.hasSickle));
    file.read(reinterpret_cast<char*>(&tempState.glideUnlocked), sizeof(tempState.glideUnlocked));
    file.read(reinterpret_cast<char*>(&tempState.currentForceOrbs), sizeof(tempState.currentForceOrbs));
    file.read(reinterpret_cast<char*>(&tempState.currentHealth), sizeof(tempState.currentHealth));
    file.read(reinterpret_cast<char*>(&tempState.maxHealth), sizeof(tempState.maxHealth));

    // Cargar Map String
    size_t mapLen;
    file.read(reinterpret_cast<char*>(&mapLen), sizeof(mapLen));
    tempState.currentMap.resize(mapLen);
    file.read(&tempState.currentMap[0], mapLen);

    // Cargar Vector2D
    file.read(reinterpret_cast<char*>(&tempState.playerPosition), sizeof(Vector2D));

    // Cargar items recolectados
    size_t itemsSize;
    file.read(reinterpret_cast<char*>(&itemsSize), sizeof(itemsSize));
    for (size_t i = 0; i < itemsSize; ++i) {
        size_t len;
        file.read(reinterpret_cast<char*>(&len), sizeof(len));
        std::string item(len, '\0');
        file.read(&item[0], len);
        tempState.collectedItems.insert(item);
    }
    size_t doorsSize;
    file.read(reinterpret_cast<char*>(&doorsSize), sizeof(doorsSize));
    for (size_t i = 0; i < doorsSize; ++i) {
        size_t len;
        file.read(reinterpret_cast<char*>(&len), sizeof(len));
        std::string door(len, '\0');
        file.read(&door[0], len);
        tempState.openedDoors.push_back(door);
    }
    file.close();

    gameState = tempState;
    return true;
}