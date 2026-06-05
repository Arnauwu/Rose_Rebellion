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
	pugi::xml_node keys = root.append_child("Keys");
	keys.append_attribute("castle").set_value(gameState.hasCastleKey);
	keys.append_attribute("forest").set_value(gameState.hasForestKey);
	keys.append_attribute("mountain").set_value(gameState.hasMountainKey);
	keys.append_attribute("catacombs").set_value(gameState.hasCatacombsKey);
	keys.append_attribute("boss").set_value(gameState.hasBossKey);
	stats.append_attribute("currentForceOrbs").set_value(gameState.currentForceOrbs);

	// Unlockables
	pugi::xml_node unlocks = root.append_child("Unlockables");
	unlocks.append_attribute("hasSickle").set_value(gameState.hasSickle);
	unlocks.append_attribute("glideUnlocked").set_value(gameState.glideUnlocked);
	unlocks.append_attribute("dashUnlocked").set_value(gameState.dashUnlocked);
	unlocks.append_attribute("doubleJumpUnlocked").set_value(gameState.doubleJumpUnlocked);
	unlocks.append_attribute("wallJumpUnlocked").set_value(gameState.wallJumpUnlocked);


	// World Status and Position
	pugi::xml_node world = root.append_child("WorldState");
	world.append_attribute("currentMap").set_value(gameState.currentMap.c_str());

	pugi::xml_node pos = world.append_child("Position");
	pos.append_attribute("x").set_value(gameState.playerPosition.getX());
	pos.append_attribute("y").set_value(gameState.playerPosition.getY());

	//Bosses
	pugi::xml_node bosses = root.append_child("Bosses");

	pugi::xml_node knightBossKilled = bosses.append_child("KnightBossKilled");
	knightBossKilled.append_attribute("bool").set_value(gameState.knightBossKilled);

	pugi::xml_node ninfaBossKilled = bosses.append_child("NinfaBossKilled");
	ninfaBossKilled.append_attribute("bool").set_value(gameState.ninfaBossKilled);

	pugi::xml_node lizardBossKilled = bosses.append_child("LizardBossKilled");
	lizardBossKilled.append_attribute("bool").set_value(gameState.lizardBossKilled);

	pugi::xml_node dragonBossKilled = bosses.append_child("DragonBossKilled");
	dragonBossKilled.append_attribute("bool").set_value(gameState.dragonBossKilled);


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
		LOG("ERROR: No existe archivo de guardado o est?corrupto. Pugi error: %s", result.description());
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
		tempState.currentForceOrbs = stats.attribute("currentForceOrbs").as_int(0);

		pugi::xml_node keys = root.child("Keys");
		if (keys) {
			tempState.hasCastleKey = keys.attribute("castle").as_bool(false);
			tempState.hasForestKey = keys.attribute("forest").as_bool(false);
			tempState.hasMountainKey = keys.attribute("mountain").as_bool(false);
			tempState.hasCatacombsKey = keys.attribute("catacombs").as_bool(false);
			tempState.hasBossKey = keys.attribute("boss").as_bool(false);
		}

		// Load unlockables
		pugi::xml_node unlocks = root.child("Unlockables");
		if (unlocks) {
			tempState.hasSickle = unlocks.attribute("hasSickle").as_bool(false);
			tempState.glideUnlocked = unlocks.attribute("glideUnlocked").as_bool(false);
			tempState.dashUnlocked = unlocks.attribute("dashUnlocked").as_bool(false);
			tempState.doubleJumpUnlocked = unlocks.attribute("doubleJumpUnlocked").as_bool(false);
			tempState.wallJumpUnlocked = unlocks.attribute("wallJumpUnlocked").as_bool(false);
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

		//Load Bosses Variables
		pugi::xml_node bosses = root.child("Bosses");
		if (bosses)
		{
			pugi::xml_node knightBossKilled = bosses.child("KnightBossKilled");
			if (knightBossKilled)
			{
				tempState.knightBossKilled = knightBossKilled.attribute("bool");
			}

			pugi::xml_node ninfaBossKilled = bosses.child("NinfaBossKilled");
			if (ninfaBossKilled)
			{
				tempState.ninfaBossKilled = ninfaBossKilled.attribute("bool");
			}

			pugi::xml_node lizardBossKilled = bosses.child("LizardBossKilled");
			if (lizardBossKilled)
			{
				tempState.lizardBossKilled = lizardBossKilled.attribute("bool");
			}

			pugi::xml_node dragonBossKilled = bosses.child("DragonBossKilled");
			if (dragonBossKilled)
			{
				tempState.dragonBossKilled = dragonBossKilled.attribute("bool");
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

		// If everything is correct, we set the status to “loaded?
		gameState = tempState;
		LOG("Partida XML cargada correctamente.");
		return true;
	}
}