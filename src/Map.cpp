#include "Engine.h"
#include "Render.h"
#include "Textures.h"
#include "Map.h"
#include "Log.h"
#include "Physics.h"
#include "Player.h"
#include "Npc.h"

#include <math.h>

#include "SceneManager.h"
#include "EntityManager.h"

#include "Cucafera.h"
#include "CucaferaShiny.h"
#include "CucaferaMutant.h"

#include "SwordKnight.h"
#include "ShieldKnight.h"

#include "Ninfa.h"
#include "Demon.h"
#include "Dip.h"

#include "Minairon.h"
#include "Bat.h"
#include "ToxicBall.h"

#include "KnightBoss.h"
#include "NinfaBoss.h"
#include "GwellBoss.h"
#include"Dragon.h"

#include "SpecialFloors.h"
#include "GameManager.h"
#include "SavePoint.h"
#include "Door.h"
#include "KeyGate.h" 
#include "Item.h"
#include "HealthOrb.h"
#include "SkillPointOrb.h"
#include "Keys.h"
#include "Manta.h"
#include "Sickle.h"	
#include "DashObj.h"
#include "DoubleJumpObj.h"
#include "WallJumpObj.h"

#include "tracy/Tracy.hpp"

#include <algorithm>
#include <cctype>

namespace
{
	std::string NormalizeKeyRegion(std::string region)
	{
		region.erase(std::remove_if(region.begin(), region.end(), [](unsigned char c) {
			return std::isspace(c) || c == '_' || c == '-';
			}), region.end());

		std::transform(region.begin(), region.end(), region.begin(), [](unsigned char c) {
			return (char)std::tolower(c);
			});

		return region;
	}

	KeyType KeyTypeFromRegion(const std::string& region)
	{
		std::string normalizedRegion = NormalizeKeyRegion(region);

		if (normalizedRegion == "forest" || normalizedRegion == "bosque") return KeyType::FOREST;
		if (normalizedRegion == "mountain" || normalizedRegion == "montana" || normalizedRegion == "monta\xC3\xB1" "a") return KeyType::MOUNTAIN;
		if (normalizedRegion == "catacumba" || normalizedRegion == "catacumbs" || normalizedRegion == "catacombs") return KeyType::CATACUMBA;
		if (normalizedRegion == "boss") return KeyType::BOSS;
		if (normalizedRegion == "castle") return KeyType::CASTLE;

		return KeyType::NONE;
	}

	KeyType ReadKeyType(Properties& objectProperties, Properties* groupProperties = nullptr)
	{
		Properties::Property* keyRegionProp = objectProperties.GetProperty("KeyRegion");
		if (keyRegionProp == nullptr && groupProperties != nullptr)
		{
			keyRegionProp = groupProperties->GetProperty("KeyRegion");
		}

		return keyRegionProp != nullptr ? KeyTypeFromRegion(keyRegionProp->value2) : KeyType::NONE;
	}
}

Map::Map() : Module(), mapLoaded(false)
{
	name = "map";
}

Map::~Map()
{
}

bool Map::Awake()
{
	name = "map";
	LOG("Loading Map Parser");
	return true;
}

bool Map::Start() {
	return true;
}

bool Map::Update(float dt)
{
	ZoneScoped;
	bool ret = true;

	if (mapLoaded)
	{
		for (const auto& objectGroup : mapData.objectGroups)
		{
			if (objectGroup->properties.GetProperty("Draw") != NULL && objectGroup->properties.GetProperty("Draw")->value == true
				&& objectGroup->properties.GetProperty("BG") != NULL && objectGroup->properties.GetProperty("BG")->value == true)
			{
				for (const auto& obj : objectGroup->objects)
				{
					if (Engine::GetInstance().render->IsOnScreenWorldRect(obj->x, obj->y - obj->height, obj->width, obj->height, 0) == false)
					{
						continue;
					}

					unsigned int gid = obj->gid;

					if (gid != 0)
					{
						const uint32_t FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
						const uint32_t FLIPPED_VERTICALLY_FLAG = 0x40000000;
						const uint32_t FLIPPED_DIAGONALLY_FLAG = 0x20000000;
						const uint32_t TILE_ID_MASK = 0x1FFFFFFF;

						bool flipH = (gid & FLIPPED_HORIZONTALLY_FLAG) != 0;
						bool flipV = (gid & FLIPPED_VERTICALLY_FLAG) != 0;
						bool flipD = (gid & FLIPPED_DIAGONALLY_FLAG) != 0;
						uint32_t tileId = gid & TILE_ID_MASK;

						float rotation = 0.0f;
						SDL_FlipMode sdlFlip = SDL_FLIP_NONE;

						if (!flipD)
						{
							if (flipH && flipV) { rotation = 180.0f; }
							else if (flipH) { sdlFlip = SDL_FLIP_HORIZONTAL; }
							else if (flipV) { sdlFlip = SDL_FLIP_VERTICAL; }
						}
						else
						{
							if (!flipH && !flipV) { rotation = 90.0f; sdlFlip = SDL_FLIP_HORIZONTAL; }
							else if (flipH && !flipV) { rotation = 90.0f; }
							else if (!flipH && flipV) { rotation = 270.0f; }
							else if (flipH && flipV) { rotation = 270.0f; sdlFlip = SDL_FLIP_HORIZONTAL; }
						}

						TileSet* tileSet = GetTilesetFromTileId(tileId);

						if (tileSet != nullptr)
						{
							SDL_Rect tileRect = { (int)obj->x, (int)obj->y, (int)obj->width, (int)obj->height };
							SDL_Rect dstRect = { tileSet->GetRect(tileId).x, tileSet->GetRect(tileId).y, tileSet->GetRect(tileId).w, tileSet->GetRect(tileId).h };
							SDL_FPoint center = { tileRect.w / 2, tileRect.h / 2 };

							Engine::GetInstance().render->DrawRotatedImage(tileSet->texture, &tileRect, &dstRect, sdlFlip, 1, rotation, center.x, center.y);
						}
					}
				}
			}
		}

		for (auto& tileset : mapData.tilesets)
		{
			for (auto it = tileset->animations.begin(); it != tileset->animations.end(); ++it)
			{
				it->second.Update(dt);
			}
		}

		for (const auto& mapLayer : mapData.layers)
		{
			if (mapLayer->tiles.empty()) continue;

			bool shouldDraw = mapLayer->properties.GetProperty("Draw") != NULL && mapLayer->properties.GetProperty("Draw")->value == true;
			bool isFront = mapLayer->properties.GetProperty("Front") != NULL && mapLayer->properties.GetProperty("Front")->value == true;

			if (shouldDraw && !isFront)
			{
				Vector2D camPosTile = GetCameraPositionInTiles();
				Vector2D limits = GetCameraLimitsInTiles(camPosTile, { 5,5 });

				for (int x = camPosTile.getX(); x <= limits.getX(); x++)
				{
					if (x < 0 || x >= mapData.width) continue;

					for (int y = camPosTile.getY(); y <= limits.getY(); y++)
					{
						if (y < 0 || y >= mapData.height) continue;

						unsigned int gid = mapLayer->Get(x, y);

						if (gid != 0)
						{
							const unsigned FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
							const unsigned FLIPPED_VERTICALLY_FLAG = 0x40000000;
							const unsigned FLIPPED_DIAGONALLY_FLAG = 0x20000000;

							bool flipped_horizontally = (gid & FLIPPED_HORIZONTALLY_FLAG);
							bool flipped_vertically = (gid & FLIPPED_VERTICALLY_FLAG);
							bool flipped_diagonally = (gid & FLIPPED_DIAGONALLY_FLAG);

							unsigned int tileId = gid & ~0XF0000000;

							float rotation = 0.0f;
							SDL_FlipMode sdlFlip = SDL_FLIP_NONE;

							if (!flipped_diagonally)
							{
								if (flipped_horizontally && flipped_vertically) { rotation = 180.0f; }
								else if (flipped_horizontally) { sdlFlip = SDL_FLIP_HORIZONTAL; }
								else if (flipped_vertically) { sdlFlip = SDL_FLIP_VERTICAL; }
							}
							else
							{
								if (!flipped_horizontally && !flipped_vertically) { rotation = 90.0f; sdlFlip = SDL_FLIP_HORIZONTAL; }
								else if (flipped_horizontally && !flipped_vertically) { rotation = 90.0f; }
								else if (!flipped_horizontally && flipped_vertically) { rotation = 270.0f; }
								else if (flipped_horizontally && flipped_vertically) { rotation = 270.0f; sdlFlip = SDL_FLIP_HORIZONTAL; }
							}

							TileSet* tileSet = GetTilesetFromTileId(tileId);

							if (tileSet != nullptr)
							{
								SDL_Rect tileRect;
								int localId = tileId - tileSet->firstGid;

								if (tileSet->animations.count(localId))
								{
									tileRect.x = tileSet->animations[localId].GetCurrentFrame().x;
									tileRect.y = tileSet->animations[localId].GetCurrentFrame().y;
									tileRect.w = tileSet->animations[localId].GetCurrentFrame().w;
									tileRect.h = tileSet->animations[localId].GetCurrentFrame().h;
								}
								else
								{
									tileRect.x = tileSet->GetRect(tileId).x;
									tileRect.y = tileSet->GetRect(tileId).y;
									tileRect.w = tileSet->GetRect(tileId).w;
									tileRect.h = tileSet->GetRect(tileId).h;
								}

								Vector2D mapCoord = MapToWorld(x, y);
								SDL_FPoint center = { tileRect.w / 2, tileRect.h / 2 };
								SDL_Rect dstRect = {
									(int)mapCoord.getX() + tileRect.w / 2,
									(int)mapCoord.getY() + tileRect.h / 2,
									tileRect.w,
									tileRect.h
								};

								Engine::GetInstance().render->DrawRotatedTexture(tileSet->texture, dstRect.x, dstRect.y, &tileRect, sdlFlip, 1, rotation, center.x, center.y);
							}
						}
					}
				}
			}
		}
	}

	return ret;
}

bool Map::PostUpdate()
{
	ZoneScoped;
	bool ret = true;

	if (mapLoaded)
	{
		for (const auto& objectGroup : mapData.objectGroups)
		{
			if (objectGroup->properties.GetProperty("Draw") != NULL && objectGroup->properties.GetProperty("Draw")->value == true
				&& objectGroup->properties.GetProperty("Front") != NULL && objectGroup->properties.GetProperty("Front")->value == true)
			{
				for (const auto& obj : objectGroup->objects)
				{
					if (Engine::GetInstance().render->IsOnScreenWorldRect(obj->x, obj->y - obj->height, obj->width, obj->height, 0) == false)
					{
						continue;
					}

					unsigned int gid = obj->gid;

					if (gid != 0)
					{
						const uint32_t FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
						const uint32_t FLIPPED_VERTICALLY_FLAG = 0x40000000;
						const uint32_t FLIPPED_DIAGONALLY_FLAG = 0x20000000;
						const uint32_t TILE_ID_MASK = 0x1FFFFFFF;

						bool flipH = (gid & FLIPPED_HORIZONTALLY_FLAG) != 0;
						bool flipV = (gid & FLIPPED_VERTICALLY_FLAG) != 0;
						bool flipD = (gid & FLIPPED_DIAGONALLY_FLAG) != 0;
						uint32_t tileId = gid & TILE_ID_MASK;

						float rotation = 0.0f;
						SDL_FlipMode sdlFlip = SDL_FLIP_NONE;

						if (!flipD)
						{
							if (flipH && flipV) { rotation = 180.0f; }
							else if (flipH) { sdlFlip = SDL_FLIP_HORIZONTAL; }
							else if (flipV) { sdlFlip = SDL_FLIP_VERTICAL; }
						}
						else
						{
							if (!flipH && !flipV) { rotation = 90.0f; sdlFlip = SDL_FLIP_HORIZONTAL; }
							else if (flipH && !flipV) { rotation = 90.0f; }
							else if (!flipH && flipV) { rotation = 270.0f; }
							else if (flipH && flipV) { rotation = 270.0f; sdlFlip = SDL_FLIP_HORIZONTAL; }
						}

						TileSet* tileSet = GetTilesetFromTileId(tileId);

						if (tileSet != nullptr)
						{
							SDL_Rect tileRect = { (int)obj->x, (int)obj->y, (int)obj->width, (int)obj->height };
							SDL_Rect dstRect = { tileSet->GetRect(tileId).x, tileSet->GetRect(tileId).y, tileSet->GetRect(tileId).w, tileSet->GetRect(tileId).h };
							SDL_FPoint center = { tileRect.w / 2, tileRect.h / 2 };

							Engine::GetInstance().render->DrawRotatedImage(tileSet->texture, &tileRect, &dstRect, sdlFlip, 1, rotation, center.x, center.y);
						}
					}
				}
			}
		}

		for (const auto& mapLayer : mapData.layers)
		{
			if (mapLayer->tiles.empty()) continue;

			bool shouldDraw = mapLayer->properties.GetProperty("Draw") != NULL && mapLayer->properties.GetProperty("Draw")->value == true;
			bool isFront = mapLayer->properties.GetProperty("Front") != NULL && mapLayer->properties.GetProperty("Front")->value == true;

			if (shouldDraw && isFront)
			{
				Vector2D camPosTile = GetCameraPositionInTiles();
				Vector2D limits = GetCameraLimitsInTiles(camPosTile, { 5,5 });

				for (int x = camPosTile.getX(); x <= limits.getX(); x++)
				{
					if (x < 0 || x >= mapData.width) continue;

					for (int y = camPosTile.getY(); y <= limits.getY(); y++)
					{
						if (y < 0 || y >= mapData.height) continue;

						unsigned int gid = mapLayer->Get(x, y);

						if (gid != 0)
						{
							const unsigned FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
							const unsigned FLIPPED_VERTICALLY_FLAG = 0x40000000;
							const unsigned FLIPPED_DIAGONALLY_FLAG = 0x20000000;

							bool flipped_horizontally = (gid & FLIPPED_HORIZONTALLY_FLAG);
							bool flipped_vertically = (gid & FLIPPED_VERTICALLY_FLAG);
							bool flipped_diagonally = (gid & FLIPPED_DIAGONALLY_FLAG);

							unsigned int tileId = gid & ~0XF0000000;

							float rotation = 0.0f;
							SDL_FlipMode sdlFlip = SDL_FLIP_NONE;

							if (!flipped_diagonally)
							{
								if (flipped_horizontally && flipped_vertically) { rotation = 180.0f; }
								else if (flipped_horizontally) { sdlFlip = SDL_FLIP_HORIZONTAL; }
								else if (flipped_vertically) { sdlFlip = SDL_FLIP_VERTICAL; }
							}
							else
							{
								if (!flipped_horizontally && !flipped_vertically) { rotation = 90.0f; sdlFlip = SDL_FLIP_HORIZONTAL; }
								else if (flipped_horizontally && !flipped_vertically) { rotation = 90.0f; }
								else if (!flipped_horizontally && flipped_vertically) { rotation = 270.0f; }
								else if (flipped_horizontally && flipped_vertically) { rotation = 270.0f; sdlFlip = SDL_FLIP_HORIZONTAL; }
							}

							TileSet* tileSet = GetTilesetFromTileId(tileId);

							if (tileSet != nullptr)
							{
								SDL_Rect tileRect;
								int localId = tileId - tileSet->firstGid;

								if (tileSet->animations.count(localId))
								{
									tileRect.x = tileSet->animations[localId].GetCurrentFrame().x;
									tileRect.y = tileSet->animations[localId].GetCurrentFrame().y;
									tileRect.w = tileSet->animations[localId].GetCurrentFrame().w;
									tileRect.h = tileSet->animations[localId].GetCurrentFrame().h;
								}
								else
								{
									tileRect.x = tileSet->GetRect(tileId).x;
									tileRect.y = tileSet->GetRect(tileId).y;
									tileRect.w = tileSet->GetRect(tileId).w;
									tileRect.h = tileSet->GetRect(tileId).h;
								}

								Vector2D mapCoord = MapToWorld(x, y);
								SDL_FPoint center = { tileRect.w / 2, tileRect.h / 2 };
								SDL_Rect dstRect = {
									(int)mapCoord.getX() + tileRect.w / 2,
									(int)mapCoord.getY() + tileRect.h / 2,
									tileRect.w,
									tileRect.h
								};

								Engine::GetInstance().render->DrawRotatedTexture(tileSet->texture, dstRect.x, dstRect.y, &tileRect, sdlFlip, 1, rotation, center.x, center.y);
							}
						}
					}
				}
			}
		}
	}
	return ret;
}

TileSet* Map::GetTilesetFromTileId(int gid) const
{
	TileSet* set = nullptr;
	for (const auto& tileset : mapData.tilesets) {
		set = tileset;
		if (gid >= tileset->firstGid && gid < tileset->firstGid + tileset->tileCount) {
			break;
		}
	}
	return set;
}

bool Map::CleanUp()
{
	LOG("Unloading map");

	for (auto& entity : mapDynamicEntities)
	{
		if (entity != nullptr)
		{
			Engine::GetInstance().entityManager->DestroyEntity(entity);
		}
	}
	mapDynamicEntities.clear();

	mapFileName = "";
	mapPath = "";

	for (const auto& tileset : mapData.tilesets) {
		delete tileset;
	}
	mapData.tilesets.clear();

	for (const auto& layer : mapData.layers)
	{
		delete layer;
	}
	mapData.layers.clear();

	for (const auto& objectGroup : mapData.objectGroups) {
		objectGroup->objects.clear();
	}
	mapData.objectGroups.clear();

	mapData.spawnPoints.clear();
	mapData.doors.clear();
	mapData.paths.clear();

	for (const auto& collider : colliderList)
	{
		Engine::GetInstance().physics->DeletePhysBody(collider);
	}
	colliderList.clear();

	return true;
}

bool Map::Load(std::string path, std::string fileName)
{
	bool ret = false;

	mapFileName = fileName;
	mapPath = path;
	std::string mapPathName = mapPath + mapFileName;

	pugi::xml_parse_result result = mapFileXML.load_file(mapPathName.c_str());

	if (result == NULL)
	{
		LOG("Could not load map xml file %s. pugi error: %s", mapPathName.c_str(), result.description());
		ret = false;
	}
	else
	{
		mapData.width = mapFileXML.child("map").attribute("width").as_int();
		mapData.height = mapFileXML.child("map").attribute("height").as_int();
		mapData.tileWidth = mapFileXML.child("map").attribute("tilewidth").as_int();
		mapData.tileHeight = mapFileXML.child("map").attribute("tileheight").as_int();

		for (pugi::xml_node tilesetNode = mapFileXML.child("map").child("tileset"); tilesetNode != NULL; tilesetNode = tilesetNode.next_sibling("tileset"))
		{
			TileSet* tileSet = new TileSet();
			tileSet->firstGid = tilesetNode.attribute("firstgid").as_int();
			tileSet->name = tilesetNode.attribute("name").as_string();
			tileSet->tileWidth = tilesetNode.attribute("tilewidth").as_int();
			tileSet->tileHeight = tilesetNode.attribute("tileheight").as_int();
			tileSet->spacing = tilesetNode.attribute("spacing").as_int();
			tileSet->margin = tilesetNode.attribute("margin").as_int();
			tileSet->tileCount = tilesetNode.attribute("tilecount").as_int();
			tileSet->columns = tilesetNode.attribute("columns").as_int();

			std::string imgName = tilesetNode.child("image").attribute("source").as_string();
			tileSet->texture = Engine::GetInstance().textures->Load((mapPath + imgName).c_str());

			for (pugi::xml_node tileNode = tilesetNode.child("tile"); tileNode; tileNode = tileNode.next_sibling("tile"))
			{
				int tileId = tileNode.attribute("id").as_int();
				pugi::xml_node animNode = tileNode.child("animation");

				if (animNode)
				{
					Animation anim;
					for (pugi::xml_node frameNode = animNode.child("frame"); frameNode; frameNode = frameNode.next_sibling("frame"))
					{
						int frameTileId = frameNode.attribute("tileid").as_int();
						int duration = frameNode.attribute("duration").as_int();

						SDL_Rect rect = tileSet->GetRect(tileSet->firstGid + frameTileId);
						anim.AddFrame(rect, duration);
					}
					anim.Reset();
					tileSet->animations[tileId] = anim;
				}
			}
			mapData.tilesets.push_back(tileSet);
		}

		for (pugi::xml_node layerNode = mapFileXML.child("map").child("layer"); layerNode != NULL; layerNode = layerNode.next_sibling("layer")) {
			MapLayer* mapLayer = new MapLayer();
			mapLayer->id = layerNode.attribute("id").as_int();
			mapLayer->name = layerNode.attribute("name").as_string();
			mapLayer->width = layerNode.attribute("width").as_int();
			mapLayer->height = layerNode.attribute("height").as_int();

			LoadProperties(layerNode, mapLayer->properties);

			for (pugi::xml_node tileNode = layerNode.child("data").child("tile"); tileNode != NULL; tileNode = tileNode.next_sibling("tile"))
			{
				mapLayer->tiles.push_back(tileNode.attribute("gid").as_uint());
			}
			mapData.layers.push_back(mapLayer);
		}

		for (pugi::xml_node objectGroupNode = mapFileXML.child("map").child("objectgroup"); objectGroupNode != NULL; objectGroupNode = objectGroupNode.next_sibling("objectgroup"))
		{
			ObjectGroup* objectgroup = new ObjectGroup();
			LoadProperties(objectGroupNode, objectgroup->properties);

			for (pugi::xml_node objectNode = objectGroupNode.child("object"); objectNode != NULL; objectNode = objectNode.next_sibling("object"))
			{
				ObjectGroup::Object* o = new ObjectGroup::Object;
				o->id = objectNode.attribute("id").as_int();
				o->x = objectNode.attribute("x").as_float();
				o->y = objectNode.attribute("y").as_float();
				o->width = objectNode.attribute("width").as_float();
				o->height = objectNode.attribute("height").as_float();

				if (objectgroup->properties.GetProperty("Draw") != NULL && objectgroup->properties.GetProperty("Draw")->value == true)
				{
					o->gid = objectNode.attribute("gid").as_uint();
				}

				if (objectNode.child("polygon").attribute("points") != NULL)
				{
					std::string pointString = objectNode.child("polygon").attribute("points").as_string();
					size_t start = 0;

					while (start < pointString.length())
					{
						size_t end = pointString.find(' ', start);
						if (end == std::string::npos) { end = pointString.length(); }

						std::string pair = pointString.substr(start, end - start);
						size_t comma = pair.find(',');

						if (comma != std::string::npos)
						{
							b2Vec2 pointPos = { stoi(pair.substr(0, comma)) + o->x,  stoi(pair.substr(comma + 1)) + o->y };
							o->points.push_back(pointPos);
						}
						start = end + 1;
					}
				}

				LoadProperties(objectNode, o->properties);
				objectgroup->objects.push_back(o);
			}
			mapData.objectGroups.push_back(objectgroup);
		}

		// Creation of colliders and assign their type
		for (const auto& objectsGroups : mapData.objectGroups)
		{
			if (objectsGroups->properties.GetProperty("Square") != NULL and objectsGroups->properties.GetProperty("Square")->value) // Square
			{
				for (const auto& obj : objectsGroups->objects)
				{
					if (obj->width <= 0 || obj->height <= 0) continue;

					PhysBody* collider;
					if (objectsGroups->properties.GetProperty("Sensor") != NULL and objectsGroups->properties.GetProperty("Sensor")->value)
					{
						collider = Engine::GetInstance().physics.get()->CreateRectangleSensor(obj->x + obj->width / 2, obj->y + obj->height / 2, obj->width, obj->height, STATIC);
					}
					else
					{
						collider = Engine::GetInstance().physics.get()->CreateRectangle(obj->x + obj->width / 2, obj->y + obj->height / 2, obj->width, obj->height, STATIC);
					}

					if (objectsGroups->properties.GetProperty("Danger") != NULL and objectsGroups->properties.GetProperty("Danger")->value)
					{
						collider->ctype = ColliderType::DANGER;
					}
					else if (objectsGroups->properties.GetProperty("Map") != NULL and objectsGroups->properties.GetProperty("Map")->value)
					{
						collider->ctype = ColliderType::MAP;
					}
					else if (objectsGroups->properties.GetProperty("Ground") != NULL and objectsGroups->properties.GetProperty("Ground")->value)
					{
						collider->ctype = ColliderType::MAP;
					}
					else if (objectsGroups->properties.GetProperty("Wall") != NULL and objectsGroups->properties.GetProperty("Wall")->value)
					{
						collider->ctype = ColliderType::MAP;
					}
					else if (objectsGroups->properties.GetProperty("Ceiling") != NULL and objectsGroups->properties.GetProperty("Ceiling")->value)
					{
						collider->ctype = ColliderType::MAP;
					}
					else if ((objectsGroups->properties.GetProperty("Door") != NULL && objectsGroups->properties.GetProperty("Door")->value) ||
						(objectsGroups->properties.GetProperty("CatacumbaDoor") != NULL && objectsGroups->properties.GetProperty("CatacumbaDoor")->value))
					{
						collider->ctype = ColliderType::DOOR;

						Door newDoor;
						newDoor.body = collider;
						newDoor.teleportTo = obj->properties.GetProperty("TeleportTo") ? obj->properties.GetProperty("TeleportTo")->value2 : "";

						newDoor.uniqueId = mapFileName + "_" + std::to_string((int)obj->id);
						newDoor.width = (int)obj->width;
						newDoor.height = (int)obj->height;

						Properties::Property* needsKeyProp = obj->properties.GetProperty("NeedsKey");
						newDoor.needsKey = (needsKeyProp != nullptr) ? needsKeyProp->value : false;

						newDoor.requiredKey = ReadKeyType(obj->properties, &objectsGroups->properties);

						for (const std::string& unlockedId : GameManager::GetInstance().gameState.openedDoors) {
							if (unlockedId == newDoor.uniqueId) {
								newDoor.needsKey = false;
								break;
							}
						}

						Properties::Property* maintenanceProp = obj->properties.GetProperty("UnderMaintenance");
						newDoor.underMaintenance = (maintenanceProp != nullptr) ? maintenanceProp->value : false;

						Properties::Property* closedProp = obj->properties.GetProperty("DoorClosed");
						newDoor.DoorClose = (closedProp != nullptr) ? closedProp->value : false;

						Properties::Property* noAnimProp = obj->properties.GetProperty("NoAnimation");
						newDoor.noAnimation = (noAnimProp != nullptr) ? noAnimProp->value : false;

						mapData.doors.push_back(newDoor);
					}
					else if (objectsGroups->properties.GetProperty("KeyGate") != NULL && objectsGroups->properties.GetProperty("KeyGate")->value)
					{

						collider->ctype = ColliderType::KEY_GATE;

						std::string uniqueId = mapFileName + "_" + std::to_string((int)obj->id);


						KeyType reqKey = ReadKeyType(obj->properties, &objectsGroups->properties);

						auto newEntity = Engine::GetInstance().entityManager->CreateEntity(EntityType::KEY_GATE);
						mapDynamicEntities.push_back(newEntity);
						KeyGate* gate = (KeyGate*)newEntity.get();

						if (gate != nullptr) {
							gate->zOrder = -1;

							gate->Initialize(Vector2D(obj->x + obj->width / 2, obj->y + obj->height / 2), obj->width, obj->height, reqKey, uniqueId);
							gate->SetCollider(collider);
							collider->listener = gate;
						}
					}
					else if (objectsGroups->properties.GetProperty("Path") != NULL and objectsGroups->properties.GetProperty("Path")->value)
					{
						collider->ctype = ColliderType::PATH;

						Door newDoor;
						newDoor.body = collider;
						newDoor.teleportTo = obj->properties.GetProperty("TeleportTo") ? obj->properties.GetProperty("TeleportTo")->value2 : "";
						newDoor.x = obj->x;
						newDoor.y = obj->y;

						Properties::Property* glideProp = obj->properties.GetProperty("RequiresGlide");
						newDoor.requiresGlide = (glideProp != nullptr) ? glideProp->value : false;

						Properties::Property* spawnIDProp = obj->properties.GetProperty("SpawnID");
						newDoor.spawnID = (spawnIDProp != nullptr) ? spawnIDProp->value2 : "";

						mapData.doors.push_back(newDoor);
						mapData.paths.push_back(newDoor);
					}
					else
					{
						collider->ctype = ColliderType::UNKNOWN;
					}
					colliderList.push_back(collider);
				}
			}
			else if (objectsGroups->properties.GetProperty("Circle") != NULL and objectsGroups->properties.GetProperty("Circle")->value) // Circle
			{
				for (const auto& obj : objectsGroups->objects)
				{
					PhysBody* collider = Engine::GetInstance().physics.get()->CreateCircle(obj->x + obj->width / 2, obj->y + obj->height / 2, obj->width / 2, STATIC);

					if (objectsGroups->properties.GetProperty("Danger") != NULL and objectsGroups->properties.GetProperty("Danger")->value)
					{
						collider->ctype = ColliderType::DANGER;
					}
					else if (objectsGroups->properties.GetProperty("Ground") != NULL and objectsGroups->properties.GetProperty("Ground")->value)
					{
						collider->ctype = ColliderType::MAP;
					}
					else
					{
						collider->ctype = ColliderType::UNKNOWN;
					}
					colliderList.push_back(collider);
				}
			}
			else if (objectsGroups->properties.GetProperty("Triangle") != NULL and objectsGroups->properties.GetProperty("Triangle")->value) // Triangle / Chain
			{
				for (const auto& obj : objectsGroups->objects)
				{
					int* points = new int[obj->points.size() * 2];

					for (size_t i = 0; i < obj->points.size(); i++)
					{
						points[i * 2] = obj->points[i].x;
						points[i * 2 + 1] = obj->points[i].y;
					}

					PhysBody* collider = Engine::GetInstance().physics.get()->CreateChain(PIXEL_TO_METERS(obj->x / 2), PIXEL_TO_METERS(obj->y / 2), points, obj->points.size() * 2, STATIC);

					if (objectsGroups->properties.GetProperty("Danger") != NULL and objectsGroups->properties.GetProperty("Danger")->value)
					{
						collider->ctype = ColliderType::DANGER;
					}
					else if (objectsGroups->properties.GetProperty("Ground") != NULL and objectsGroups->properties.GetProperty("Ground")->value)
					{
						collider->ctype = ColliderType::MAP;
					}
					else
					{
						collider->ctype = ColliderType::UNKNOWN;
					}
					colliderList.push_back(collider);
				}
			}
			else if (objectsGroups->properties.GetProperty("Polygon") != NULL && objectsGroups->properties.GetProperty("Polygon")->value)
			{
				for (const auto& obj : objectsGroups->objects)
				{
					int* pointsArray = new int[obj->points.size() * 2];

					for (size_t i = 0; i < obj->points.size(); i++)
					{
						pointsArray[i * 2] = obj->points[i].x - obj->x;
						pointsArray[i * 2 + 1] = obj->points[i].y - obj->y;
					}

					PhysBody* collider = Engine::GetInstance().physics.get()->CreatePolygon(obj->x, obj->y, pointsArray, obj->points.size() * 2, STATIC);

					if (objectsGroups->properties.GetProperty("Danger") != NULL && objectsGroups->properties.GetProperty("Danger")->value)
					{
						collider->ctype = ColliderType::DANGER;
					}
					else
					{
						collider->ctype = ColliderType::MAP;
					}

					colliderList.push_back(collider);
					delete[] pointsArray;
				}
			}
		}

		ret = true;

		if (ret == true)
		{
			LOG("Successfully parsed map XML file :%s", fileName.c_str());
		}
		else
		{
			LOG("Error while parsing map file: %s", mapPathName.c_str());
		}
	}

	mapLoaded = ret;
	return ret;
}

Vector2D Map::MapToWorld(int x, int y) const
{
	Vector2D ret;
	ret.setX((float)(x * mapData.tileWidth));
	ret.setY((float)(y * mapData.tileHeight));
	return ret;
}

Vector2D Map::WorldToMap(int x, int y)
{
	Vector2D ret(0, 0);
	ret.setX(floor((float)(x / mapData.tileWidth)));
	ret.setY(floor((float)(y / mapData.tileHeight)));
	return ret;
}

bool Map::LoadProperties(pugi::xml_node& node, Properties& properties)
{
	bool ret = false;

	for (pugi::xml_node propertieNode = node.child("properties").child("property"); propertieNode; propertieNode = propertieNode.next_sibling("property"))
	{
		Properties::Property* p = new Properties::Property();
		p->name = propertieNode.attribute("name").as_string();

		if (propertieNode.attribute("type").as_string() == std::string("bool"))
		{
			p->value = propertieNode.attribute("value").as_bool();
		}
		else if (propertieNode.attribute("type").as_string() == std::string("int"))
		{
			p->value = propertieNode.attribute("value").as_int();
		}
		else if (propertieNode.attribute("type").as_string() == std::string("file") ||
			propertieNode.attribute("type").as_string() == std::string("string") ||
			propertieNode.attribute("type").empty())
		{
			p->value2 = propertieNode.attribute("value").as_string();
		}
		properties.propertyList.push_back(p);
	}

	return ret;
}

MapLayer* Map::GetNavigationLayer(bool ground, int* blockedGID, int* highGID)
{
	for (const auto& tileset : mapData.tilesets)
	{
		if (tileset->name == "MapMetadata")
		{
			*blockedGID = tileset->firstGid;
			*highGID = tileset->firstGid + 1;
			break;
		}
	}

	for (const auto& layer : mapData.layers) {
		if (layer->properties.GetProperty("Navigation") != NULL &&
			layer->properties.GetProperty("Navigation")->value)
		{
			if (ground && layer->properties.GetProperty("Ground") != NULL && layer->properties.GetProperty("Ground")->value)
			{
				return layer;
			}
			else  if (!ground && layer->properties.GetProperty("Air") != NULL && layer->properties.GetProperty("Air")->value)
			{
				return layer;
			}
		}
	}
	return nullptr;
}

Vector2D Map::GetMapSizeInPixels()
{
	Vector2D sizeInPixels;
	sizeInPixels.setX((float)(mapData.width * mapData.tileWidth));
	sizeInPixels.setY((float)(mapData.height * mapData.tileHeight));
	return sizeInPixels;
}

Vector2D Map::GetMapSizeInTiles()
{
	return Vector2D((float)mapData.width, (float)mapData.height);
}

std::vector<Door> Map::GetPaths()
{
	return mapData.paths;
}

void Map::SpawnEntities()
{
	for (pugi::xml_node objectGroupNode = mapFileXML.child("map").child("objectgroup"); objectGroupNode != NULL; objectGroupNode = objectGroupNode.next_sibling("objectgroup"))
	{
		if (objectGroupNode.attribute("name").as_string() == std::string("EntitiesSpawnPoints"))
		{
			Properties objectGroupProps;
			LoadProperties(objectGroupNode, objectGroupProps);

			for (pugi::xml_node objectNode = objectGroupNode.child("object"); objectNode != NULL; objectNode = objectNode.next_sibling("object"))
			{
				std::string entityType = objectNode.attribute("type").as_string();
				float x = objectNode.attribute("x").as_float();
				float y = objectNode.attribute("y").as_float();
				float w = objectNode.attribute("width").as_float();
				float h = objectNode.attribute("height").as_float();

				if (entityType == std::string("Player") && objectNode.attribute("OriginMap").as_string())
				{
					Player* player = Engine::GetInstance().entityManager->GetPlayer();
					if (player == nullptr)
					{
						std::shared_ptr<Player> newPlayerPtr = std::dynamic_pointer_cast<Player>(Engine::GetInstance().entityManager->CreateEntity(EntityType::PLAYER));
						player = newPlayerPtr.get();
						player->position = Vector2D(x, y);
					}
					Engine::GetInstance().entityManager->SetPlayer(player);
				}
				else if (entityType == std::string("SavePoint"))
				{
					std::shared_ptr<SavePoint> sp = std::dynamic_pointer_cast<SavePoint>(Engine::GetInstance().entityManager->CreateEntity(EntityType::SAVEPOINT));
					if (sp != nullptr) sp->position = Vector2D(x, y);
				}
				else if (entityType == std::string("Cucafera"))
				{
					std::shared_ptr<Cucafera> cucafera = std::dynamic_pointer_cast<Cucafera>(Engine::GetInstance().entityManager->CreateEntity(EntityType::CUCAFERA));
					if (cucafera != nullptr) cucafera->position = Vector2D(x, y);
				}
				else if (entityType == std::string("CucaferaShiny"))
				{
					std::shared_ptr<CucaferaShiny> cucaferaShiny = std::dynamic_pointer_cast<CucaferaShiny>(Engine::GetInstance().entityManager->CreateEntity(EntityType::CUCAFERA_SHINY));
					if (cucaferaShiny != nullptr) cucaferaShiny->position = Vector2D(x, y);
				}
				else if (entityType == std::string("CucaferaMutant"))
				{
					std::shared_ptr<CucaferaMutant> cucaferaMutant = std::dynamic_pointer_cast<CucaferaMutant>(Engine::GetInstance().entityManager->CreateEntity(EntityType::CUCAFERA_MUTANT));
					if (cucaferaMutant != nullptr) cucaferaMutant->position = Vector2D(x, y);
				}
				else if (entityType == std::string("SwordKnight"))
				{
					std::shared_ptr<SwordKnight> swordKnight = std::dynamic_pointer_cast<SwordKnight>(Engine::GetInstance().entityManager->CreateEntity(EntityType::SWORD_KNIGHT));
					if (swordKnight != nullptr) swordKnight->position = Vector2D(x, y);
				}
				else if (entityType == std::string("ShieldKnight"))
				{
					std::shared_ptr<ShieldKnight> shieldKnight = std::dynamic_pointer_cast<ShieldKnight>(Engine::GetInstance().entityManager->CreateEntity(EntityType::SHIELD_KNIGHT));
					if (shieldKnight != nullptr) shieldKnight->position = Vector2D(x, y);
				}
				else if (entityType == std::string("Dip"))
				{
					std::shared_ptr<Dip> dip = std::dynamic_pointer_cast<Dip>(Engine::GetInstance().entityManager->CreateEntity(EntityType::DIP));
					if (dip != nullptr) dip->position = Vector2D(x, y);
				}
				else if (entityType == std::string("Ninfa"))
				{
					std::shared_ptr<Ninfa> ninfa = std::dynamic_pointer_cast<Ninfa>(Engine::GetInstance().entityManager->CreateEntity(EntityType::NINFA));
					if (ninfa != nullptr) ninfa->position = Vector2D(x, y);
				}
				else if (entityType == std::string("Demon"))
				{
					std::shared_ptr<Demon> demon = std::dynamic_pointer_cast<Demon>(Engine::GetInstance().entityManager->CreateEntity(EntityType::DEMON));
					if (demon != nullptr) demon->position = Vector2D(x, y);
				}
				else if (entityType == std::string("Minairon"))
				{
					std::shared_ptr<Minairon> minairon = std::dynamic_pointer_cast<Minairon>(Engine::GetInstance().entityManager->CreateEntity(EntityType::MINAIRON));
					if (minairon != nullptr) minairon->position = Vector2D(x, y);
				}
				else if (entityType == std::string("Bat"))
				{
					std::shared_ptr<Bat> bat = std::dynamic_pointer_cast<Bat>(Engine::GetInstance().entityManager->CreateEntity(EntityType::BAT));
					if (bat != nullptr) bat->position = Vector2D(x, y);
				}
				else if (entityType == std::string("ToxicBall"))
				{
					std::shared_ptr<ToxicBall> toxicBall = std::dynamic_pointer_cast<ToxicBall>(Engine::GetInstance().entityManager->CreateEntity(EntityType::TOXIC_BALL));
					if (toxicBall != nullptr) toxicBall->position = Vector2D(x, y);

					Properties toxicBallProps;
					LoadProperties(objectNode, toxicBallProps);

					if (toxicBallProps.GetProperty("Jump") != nullptr) toxicBall->jumpDistanceTiles = toxicBallProps.GetProperty("Jump")->value;
				}
				else if (entityType == std::string("KnightBoss"))
				{
					std::shared_ptr<KnightBoss> knightBoss = std::dynamic_pointer_cast<KnightBoss>(Engine::GetInstance().entityManager->CreateEntity(EntityType::KNIGHT_BOSS));
					if (knightBoss != nullptr) knightBoss->position = Vector2D(x, y);
				}
				else if (entityType == std::string("NinfaBoss"))
				{
					std::shared_ptr<NinfaMare> ninfaBoss = std::dynamic_pointer_cast<NinfaMare>(Engine::GetInstance().entityManager->CreateEntity(EntityType::NINFA_MARE));
					if (ninfaBoss != nullptr) ninfaBoss->position = Vector2D(x, y);
				}
				else if (entityType == std::string("GwellBoss"))
				{
					std::shared_ptr<GwellBoss> gwellBoss = std::dynamic_pointer_cast<GwellBoss>(Engine::GetInstance().entityManager->CreateEntity(EntityType::GWELL_BOSS));
					if (gwellBoss != nullptr) gwellBoss->position = Vector2D(x, y);
				}
				else if (entityType == std::string("Dragon"))
				{
					std::shared_ptr<Dragon> dragon = std::dynamic_pointer_cast<Dragon>(Engine::GetInstance().entityManager->CreateEntity(EntityType::DRAGON));
					if (dragon != nullptr) dragon->position = Vector2D(x, y);
				}
				else if (entityType == std::string("Key"))
				{
					std::shared_ptr<Keys> key = std::dynamic_pointer_cast<Keys>(Engine::GetInstance().entityManager->CreateEntity(EntityType::KEY));
					if (key != nullptr) {
						key->position = Vector2D(x, y);

						Properties keyProps;
						LoadProperties(objectNode, keyProps);

						key->SetKeyType(ReadKeyType(keyProps, &objectGroupProps));
					}
				}
				else if (entityType == std::string("Manta")) {
					std::shared_ptr<Manta> manta = std::dynamic_pointer_cast<Manta>(Engine::GetInstance().entityManager->CreateEntity(EntityType::MANTA));
					if (manta != nullptr) manta->position = Vector2D(x, y);
				}
				else if (entityType == std::string("Sickle")) {
					std::shared_ptr<Sickle> sickle = std::dynamic_pointer_cast<Sickle>(Engine::GetInstance().entityManager->CreateEntity(EntityType::SICKLE));
					if (sickle != nullptr) sickle->position = Vector2D(x, y);
				}
				else if (entityType == std::string("HealthOrb"))
				{
					std::shared_ptr<HealthOrb> healthOrb = std::dynamic_pointer_cast<HealthOrb>(Engine::GetInstance().entityManager->CreateEntity(EntityType::HEALTH_ORB));
					if (healthOrb != nullptr) healthOrb->position = Vector2D(x, y);
				}
				else if (entityType == std::string("SkillPointOrb"))
				{
					std::shared_ptr<SkillPointOrb> skillPointOrb = std::dynamic_pointer_cast<SkillPointOrb>(Engine::GetInstance().entityManager->CreateEntity(EntityType::SKILL_POINT_ORB));
					if (skillPointOrb != nullptr) skillPointOrb->position = Vector2D(x, y);
				}
				else if (entityType == std::string("DashObj"))
				{
					std::shared_ptr<DashObj> dashobj = std::dynamic_pointer_cast<DashObj>(Engine::GetInstance().entityManager->CreateEntity(EntityType::DASH_OBJ));
					if (dashobj != nullptr) dashobj->position = Vector2D(x, y);
				}
				else if (entityType == std::string("DoubleJumpObj"))
				{
					std::shared_ptr<DoubleJumpObj> doublejumpobj = std::dynamic_pointer_cast<DoubleJumpObj>(Engine::GetInstance().entityManager->CreateEntity(EntityType::DOUBLEJUMP_OBJ));
					if (doublejumpobj != nullptr) doublejumpobj->position = Vector2D(x, y);
				}
				else if (entityType == std::string("WallJumpObj")) {
					std::shared_ptr<WallJumpObj> walljumpobj = std::dynamic_pointer_cast<WallJumpObj>(Engine::GetInstance().entityManager->CreateEntity(EntityType::WALLJUMP_OBJ));
					if (walljumpobj != nullptr) walljumpobj->position = Vector2D(x, y);
				}
				else if (entityType == std::string("Npc"))
				{
					std::shared_ptr<Npc> npc = std::dynamic_pointer_cast<Npc>(Engine::GetInstance().entityManager->CreateEntity(EntityType::NPC));
					if (npc != nullptr)
					{
						npc->position = Vector2D(x, y);
						Properties npcProps;
						LoadProperties(objectNode, npcProps);

						if (npcProps.GetProperty("DialogueID") != nullptr)
						{
							std::string idLeido = npcProps.GetProperty("DialogueID")->value2;
							npc->SetDialogueID(idLeido);
						}
					}
				}
				else if (entityType == std::string("HorizontalFloor"))
				{
					std::shared_ptr<SpecialFloor> horizontalFloor = std::dynamic_pointer_cast<SpecialFloor>(Engine::GetInstance().entityManager->CreateEntity(EntityType::SPECIALFLOOR));
					horizontalFloor->position = Vector2D(x, y);
					horizontalFloor->floorType = TypeFloor::HORIZONTALFLOOR;
					horizontalFloor->width = (int)w;
					horizontalFloor->height = (int)h;

					Properties floorProps;
					LoadProperties(objectNode, floorProps);

					if (floorProps.GetProperty("Distance") != nullptr) horizontalFloor->distance = floorProps.GetProperty("Distance")->value;
					if (floorProps.GetProperty("Speed") != nullptr) horizontalFloor->moveSpeed = floorProps.GetProperty("Speed")->value;
					if (floorProps.GetProperty("Direction") != nullptr) horizontalFloor->moveDirection = floorProps.GetProperty("Direction")->value;
					if (floorProps.GetProperty("WaitTime") != nullptr) horizontalFloor->waitTimeMax = (float)floorProps.GetProperty("WaitTime")->value;
					if (floorProps.GetProperty("ActivationOnTouch") != nullptr) horizontalFloor->activationOnTouch = floorProps.GetProperty("ActivationOnTouch")->value;
				}
				else if (entityType == std::string("VerticalFloor"))
				{
					std::shared_ptr<SpecialFloor> verticalFloor = std::dynamic_pointer_cast<SpecialFloor>(Engine::GetInstance().entityManager->CreateEntity(EntityType::SPECIALFLOOR));
					verticalFloor->position = Vector2D(x, y);
					verticalFloor->floorType = TypeFloor::VERTICALFLOOR;
					verticalFloor->width = (int)w;
					verticalFloor->height = (int)h;

					Properties floorProps;
					LoadProperties(objectNode, floorProps);

					if (floorProps.GetProperty("Distance") != nullptr) verticalFloor->distance = floorProps.GetProperty("Distance")->value;
					if (floorProps.GetProperty("Speed") != nullptr) verticalFloor->moveSpeed = floorProps.GetProperty("Speed")->value;
					if (floorProps.GetProperty("Direction") != nullptr) verticalFloor->moveDirection = floorProps.GetProperty("Direction")->value;
					if (floorProps.GetProperty("WaitTime") != nullptr) verticalFloor->waitTimeMax = (float)floorProps.GetProperty("WaitTime")->value;
					if (floorProps.GetProperty("ActivationOnTouch") != nullptr) verticalFloor->activationOnTouch = floorProps.GetProperty("ActivationOnTouch")->value;
				}
				else if (entityType == std::string("BrokenFloor"))
				{
					std::shared_ptr<SpecialFloor> brokenFloor = std::dynamic_pointer_cast<SpecialFloor>(Engine::GetInstance().entityManager->CreateEntity(EntityType::SPECIALFLOOR));
					brokenFloor->position = Vector2D(x, y);
					brokenFloor->floorType = TypeFloor::BROKENFLOOR;
					brokenFloor->width = (int)w;
					brokenFloor->height = (int)h;

					Properties floorProps;
					LoadProperties(objectNode, floorProps);

					if (floorProps.GetProperty("BreakTime") != nullptr) {
						brokenFloor->breakTimeMax = (float)floorProps.GetProperty("BreakTime")->value;
						brokenFloor->currentBreakTime = brokenFloor->breakTimeMax;
					}
					if (floorProps.GetProperty("RespawnTime") != nullptr) brokenFloor->respawnTimeMax = (float)floorProps.GetProperty("RespawnTime")->value;
				}
				else if (entityType == std::string("CircularFloor"))
				{
					std::shared_ptr<SpecialFloor> circularFloor = std::dynamic_pointer_cast<SpecialFloor>(Engine::GetInstance().entityManager->CreateEntity(EntityType::SPECIALFLOOR));
					circularFloor->position = Vector2D(x, y);
					circularFloor->floorType = TypeFloor::CIRCULARFLOOR;
					circularFloor->width = (int)w;
					circularFloor->height = (int)h;

					Properties floorProps;
					LoadProperties(objectNode, floorProps);

					if (floorProps.GetProperty("Distance") != nullptr) circularFloor->distance = floorProps.GetProperty("Distance")->value;
					if (floorProps.GetProperty("Speed") != nullptr) circularFloor->moveSpeed = floorProps.GetProperty("Speed")->value;
					if (floorProps.GetProperty("Direction") != nullptr) circularFloor->moveDirection = floorProps.GetProperty("Direction")->value;
					if (floorProps.GetProperty("WaitTime") != nullptr) circularFloor->waitTimeMax = (float)floorProps.GetProperty("WaitTime")->value;
					if (floorProps.GetProperty("ActivationOnTouch") != nullptr) circularFloor->activationOnTouch = floorProps.GetProperty("ActivationOnTouch")->value;
				}
			}
		}

		if (objectGroupNode.attribute("name").as_string() == std::string("PlayerSpawns"))
		{
			for (pugi::xml_node objectNode = objectGroupNode.child("object"); objectNode != NULL; objectNode = objectNode.next_sibling("object"))
			{
				float x = objectNode.attribute("x").as_float();
				float y = objectNode.attribute("y").as_float();

				Properties a;
				LoadProperties(objectNode, a);

				PlayerSpawnPoint newSpawn;
				newSpawn.fromRoom = a.GetProperty("FromRoom") ? a.GetProperty("FromRoom")->value2 : "UNKNOWN";
				newSpawn.spawnID = a.GetProperty("SpawnID") ? a.GetProperty("SpawnID")->value2 : "";
				newSpawn.position.setX(x);
				newSpawn.position.setY(y);

				mapData.spawnPoints.push_back(newSpawn);
			}
		}
	}

	Engine::GetInstance().entityManager->AwakeEntities();
}

std::string Map::DoorInfo(PhysBody* door)
{
	for (const auto& ndoor : mapData.doors)
	{
		if (ndoor.body == door) return ndoor.teleportTo;
	}
	return std::string();
}

bool Map::DoorNeedsKey(PhysBody* door)
{
	for (const auto& ndoor : mapData.doors)
	{
		if (ndoor.body == door) return ndoor.needsKey;
	}
	return false;
}

bool Map::DoorRequiresGlide(PhysBody* door)
{
	for (const auto& ndoor : mapData.doors)
	{
		if (ndoor.body == door) return ndoor.requiresGlide;
	}
	return false;
}

Vector2D Map::GetPlayerSpawnPoint(const std::string& fromRoom, const std::string& spawnID)
{
	for (const auto& spawnPoint : mapData.spawnPoints)
	{
		if (spawnPoint.fromRoom == fromRoom && (spawnID.empty() || spawnPoint.spawnID == spawnID))
		{
			return spawnPoint.position;
		}
	}

	for (const auto& spawnPoint : mapData.spawnPoints)
	{
		if (spawnPoint.fromRoom == fromRoom) return spawnPoint.position;
	}

	if (!mapData.spawnPoints.empty()) return mapData.spawnPoints.front().position;

	return Vector2D(200, 200);
}

std::string Map::GetPathSpawnID(PhysBody* path)
{
	for (const auto& ndoor : mapData.doors)
	{
		if (ndoor.body == path) return ndoor.spawnID;
	}
	return "";
}

std::string Map::GetDoorUniqueId(PhysBody* door)
{
	for (const auto& ndoor : mapData.doors)
	{
		if (ndoor.body == door) return ndoor.uniqueId;
	}
	return "";
}

bool Map::DoorUnderMaintenance(PhysBody* door)
{
	for (const auto& ndoor : mapData.doors)
	{
		if (ndoor.body == door) return ndoor.underMaintenance;
	}
	return false;
}

bool Map::DoorClosed(PhysBody* door) {
	for (const auto& ndoor : mapData.doors)
	{
		if (ndoor.body == door) return ndoor.DoorClose;
	}
	return false;
}

void Map::GetDoorDimensions(PhysBody* door, int& w, int& h)
{
	for (const auto& ndoor : mapData.doors)
	{
		if (ndoor.body == door)
		{
			w = ndoor.width;
			h = ndoor.height;
			return;
		}
	}
	w = 256; h = 256;
}

Vector2D Map::GetCameraPositionInTiles()
{
	SDL_Rect camera = Engine::GetInstance().render->camera;
	Vector2D camPosTile = WorldToMap(-camera.x, -camera.y);
	return camPosTile;
}

Vector2D Map::GetCameraLimitsInTiles(Vector2D camPosTile, Vector2D margin)
{
	SDL_Rect camera = Engine::GetInstance().render->camera;
	Vector2D camLimitTile = WorldToMap(camera.w, camera.h);
	return camPosTile + camLimitTile + margin;
}

KeyType Map::GetDoorKeyType(PhysBody* door)
{
	for (const auto& ndoor : mapData.doors)
	{
		if (ndoor.body == door) return ndoor.requiredKey;
	}
	return KeyType::NONE;
}

bool Map::DoorHasNoAnimation(PhysBody* door)
{
	for (const auto& ndoor : mapData.doors)
	{
		if (ndoor.body == door) return ndoor.noAnimation;
	}
	return false;
}
