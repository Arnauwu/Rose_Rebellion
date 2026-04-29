#include "GameManager.h"
#include "Log.h"
#include <iostream>

#include "pugixml.hpp" 

void GameManager::StartNewGame() {
    LOG("Iniciando nueva partida...");
    gameState = GameState();
}

bool GameManager::SaveGame(const std::string& filename) {
    LOG("Guardando partida en %s", filename.c_str());

    pugi::xml_document doc;

    // Root node
    pugi::xml_node root = doc.append_child("SaveGame");

    // Player statistics
    pugi::xml_node stats = root.append_child("PlayerStats");
    stats.append_attribute("currentHealth").set_value(gameState.currentHealth);
    stats.append_attribute("maxHealth").set_value(gameState.maxHealth);
    stats.append_attribute("keyCount").set_value(gameState.keyCount);
    stats.append_attribute("currentForceOrbs").set_value(gameState.currentForceOrbs);

    // Unlockables
    pugi::xml_node unlocks = root.append_child("Unlockables");
    unlocks.append_attribute("hasSickle").set_value(gameState.hasSickle);
    unlocks.append_attribute("glideUnlocked").set_value(gameState.glideUnlocked);

    // World Status and Position
    pugi::xml_node world = root.append_child("WorldState");
    world.append_attribute("currentMap").set_value(gameState.currentMap.c_str());

    pugi::xml_node pos = world.append_child("Position");
    pos.append_attribute("x").set_value(gameState.playerPosition.getX());
    pos.append_attribute("y").set_value(gameState.playerPosition.getY());

    // Open doors
    pugi::xml_node doors = root.append_child("OpenedDoors");
    for (const auto& doorId : gameState.openedDoors) {
        pugi::xml_node doorNode = doors.append_child("Door");
        doorNode.append_attribute("id").set_value(doorId.c_str());
    }

    // Collected Items
    pugi::xml_node items = root.append_child("CollectedItems");
    for (const auto& itemId : gameState.collectedItems) {
        pugi::xml_node itemNode = items.append_child("Item");
        itemNode.append_attribute("id").set_value(itemId.c_str());
    }

    // Save the file
    bool success = doc.save_file(filename.c_str());

    if (success) {
        LOG("Partida guardada exitosamente en formato XML.");
    }
    else {
        LOG("ERROR: No se pudo guardar el archivo XML.");
    }

    return success;
}

bool GameManager::LoadGame(const std::string& filename) {
    LOG("Cargando partida desde %s", filename.c_str());

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filename.c_str());

    if (!result) {
        LOG("ERROR: No existe archivo de guardado o está corrupto. Pugi error: %s", result.description());
        return false;
    }

    pugi::xml_node root = doc.child("SaveGame");
    if (!root) {
        LOG("ERROR: El XML no tiene la estructura de un SaveGame válido.");
        return false;
    }

    // Temporary security status
    GameState tempState;

    // Load statistics
    pugi::xml_node stats = root.child("PlayerStats");
    if (stats) {
        tempState.currentHealth = stats.attribute("currentHealth").as_int(100);
        tempState.maxHealth = stats.attribute("maxHealth").as_int(100);
        tempState.keyCount = stats.attribute("keyCount").as_int(0);
        tempState.currentForceOrbs = stats.attribute("currentForceOrbs").as_int(0);
    }

    // Load unlockables
    pugi::xml_node unlocks = root.child("Unlockables");
    if (unlocks) {
        tempState.hasSickle = unlocks.attribute("hasSickle").as_bool(false);
        tempState.glideUnlocked = unlocks.attribute("glideUnlocked").as_bool(false);
    }

    // Load World State and Position
    pugi::xml_node world = root.child("WorldState");
    if (world) {
        tempState.currentMap = world.attribute("currentMap").as_string("Castle_Room_Princess.tmx");

        pugi::xml_node pos = world.child("Position");
        if (pos) {
            tempState.playerPosition.setX(pos.attribute("x").as_float(96.0f));
            tempState.playerPosition.setY(pos.attribute("y").as_float(96.0f));
        }
    }

    // Load Open Doors
    pugi::xml_node doors = root.child("OpenedDoors");
    for (pugi::xml_node doorNode = doors.child("Door"); doorNode; doorNode = doorNode.next_sibling("Door")) {
        tempState.openedDoors.push_back(doorNode.attribute("id").as_string());
    }

    // Load Collected Items
    pugi::xml_node items = root.child("CollectedItems");
    for (pugi::xml_node itemNode = items.child("Item"); itemNode; itemNode = itemNode.next_sibling("Item")) {
        tempState.collectedItems.insert(itemNode.attribute("id").as_string());
    }

    // If everything is correct, we set the status to “loaded”
    gameState = tempState;
    LOG("Partida XML cargada correctamente.");
    return true;
}