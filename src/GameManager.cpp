#include "GameManager.h"
#include "Log.h"
#include <fstream>
#include <iostream>

void GameManager::StartNewGame() {
    LOG("Iniciando nueva partida...");
    // Reseteamos el estado a los valores por defecto definidos en el struct GameState
    gameState = GameState();
}

bool GameManager::SaveGame(const std::string& filename) {
    LOG("Guardando partida en %s", filename.c_str());

    // Apertura del archivo en modo binario
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        LOG("ERROR: No se pudo abrir el archivo para guardar.");
        return false;
    }

    // 1. Guardar variables de estado simples
    file.write(reinterpret_cast<char*>(&gameState.keyCount), sizeof(gameState.keyCount));
    file.write(reinterpret_cast<char*>(&gameState.hasSickle), sizeof(gameState.hasSickle));
    file.write(reinterpret_cast<char*>(&gameState.glideUnlocked), sizeof(gameState.glideUnlocked));
    file.write(reinterpret_cast<char*>(&gameState.currentForceOrbs), sizeof(gameState.currentForceOrbs));
    file.write(reinterpret_cast<char*>(&gameState.currentHealth), sizeof(gameState.currentHealth));
    file.write(reinterpret_cast<char*>(&gameState.maxHealth), sizeof(gameState.maxHealth));

    // 2. Guardar el nombre del mapa actual (string)
    size_t mapLen = gameState.currentMap.size();
    file.write(reinterpret_cast<char*>(&mapLen), sizeof(mapLen));
    file.write(gameState.currentMap.c_str(), mapLen);

    // 3. Guardar la posición del jugador
    file.write(reinterpret_cast<char*>(&gameState.playerPosition), sizeof(Vector2D));

    // 4. Guardar items recolectados (unordered_set de strings)
    size_t itemsSize = gameState.collectedItems.size();
    file.write(reinterpret_cast<char*>(&itemsSize), sizeof(itemsSize));
    for (const auto& item : gameState.collectedItems) {
        size_t len = item.size();
        file.write(reinterpret_cast<char*>(&len), sizeof(len));
        file.write(item.c_str(), len);
    }

    // 5. Guardar puertas abiertas (vector de strings)
    size_t doorsSize = gameState.openedDoors.size();
    file.write(reinterpret_cast<char*>(&doorsSize), sizeof(doorsSize));
    for (const auto& door : gameState.openedDoors) {
        size_t len = door.size();
        file.write(reinterpret_cast<char*>(&len), sizeof(len));
        file.write(door.c_str(), len);
    }

    file.close();
    LOG("Partida guardada exitosamente.");
    return true;
}

bool GameManager::LoadGame(const std::string& filename) {
    LOG("Cargando partida desde %s", filename.c_str());

    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        LOG("ERROR: No existe archivo de guardado.");
        return false;
    }

    // Usamos un estado temporal para no corromper la partida actual si la carga falla
    GameState tempState;

    // 1. Cargar variables simples
    file.read(reinterpret_cast<char*>(&tempState.keyCount), sizeof(tempState.keyCount));
    file.read(reinterpret_cast<char*>(&tempState.hasSickle), sizeof(tempState.hasSickle));
    file.read(reinterpret_cast<char*>(&tempState.glideUnlocked), sizeof(tempState.glideUnlocked));
    file.read(reinterpret_cast<char*>(&tempState.currentForceOrbs), sizeof(tempState.currentForceOrbs));
    file.read(reinterpret_cast<char*>(&tempState.currentHealth), sizeof(tempState.currentHealth));
    file.read(reinterpret_cast<char*>(&tempState.maxHealth), sizeof(tempState.maxHealth));

    // 2. Cargar Map String con chequeo de seguridad
    size_t mapLen;
    file.read(reinterpret_cast<char*>(&mapLen), sizeof(mapLen));

    if (mapLen > 500) { // Límite de seguridad para evitar desbordamiento
        LOG("ERROR: Archivo corrupto (mapLen demasiado grande).");
        file.close();
        return false;
    }

    tempState.currentMap.resize(mapLen);
    file.read(&tempState.currentMap[0], mapLen);

    // 3. Cargar posición del jugador
    file.read(reinterpret_cast<char*>(&tempState.playerPosition), sizeof(Vector2D));

    // 4. Cargar items recolectados con chequeo de seguridad
    size_t itemsSize;
    file.read(reinterpret_cast<char*>(&itemsSize), sizeof(itemsSize));

    if (itemsSize > 10000) {
        LOG("ERROR: Archivo corrupto (itemsSize irracional).");
        file.close();
        return false;
    }

    for (size_t i = 0; i < itemsSize; ++i) {
        size_t len;
        file.read(reinterpret_cast<char*>(&len), sizeof(len));
        if (len > 500) { file.close(); return false; }

        std::string item(len, '\0');
        file.read(&item[0], len);
        tempState.collectedItems.insert(item);
    }

    // 5. Cargar puertas abiertas con chequeo de seguridad
    size_t doorsSize;
    file.read(reinterpret_cast<char*>(&doorsSize), sizeof(doorsSize));

    if (doorsSize > 10000) {
        LOG("ERROR: Archivo corrupto (doorsSize irracional).");
        file.close();
        return false;
    }

    for (size_t i = 0; i < doorsSize; ++i) {
        size_t len;
        file.read(reinterpret_cast<char*>(&len), sizeof(len));
        if (len > 500) { file.close(); return false; }

        std::string door(len, '\0');
        file.read(&door[0], len);
        tempState.openedDoors.push_back(door);
    }

    file.close();

    // Si todo es correcto, aplicamos el estado cargado
    gameState = tempState;
    LOG("Partida cargada correctamente.");
    return true;
}