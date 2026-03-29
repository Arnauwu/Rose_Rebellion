
#include "Engine.h"
#include "Render.h"
#include "Textures.h"
#include "Map.h"
#include "Log.h"
#include "Physics.h"

#include <math.h>

#include "Scene.h"


#include "EntityManager.h"
#include "Test.h"
#include "Item.h"

Map::Map() : Module(), mapLoaded(false)
{
    name = "map";
}

// Destructor
Map::~Map()
{}

// Called before render is available
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
    bool ret = true;

    if (mapLoaded) 
    {
        //Update all Animated Tiles
        for (auto& tileset : mapData.tilesets)
        {
            for (auto it = tileset->animations.begin(); it != tileset->animations.end(); ++it)
            {
                it->second.Update(dt);
            }
        }


        // Loop to draw all tiles in a layer + DrawTexture()
        for (const auto& mapLayer : mapData.layers) 
        {
            if (mapLayer->properties.GetProperty("Draw") != NULL && mapLayer->properties.GetProperty("Draw")->value == true) 
            {
				for (int x = 0; x < mapData.width; x++) 
                {
                    if (x < 0 || x >= mapData.width) // FailSave
                    {
                        continue;
                    }

					for (int y = 0; y < mapData.height; y++) 
                    {
                        if (y < 0 || y >= mapData.height) //FailSave
                        {
                            continue;
                        }

                        //Get the gid from tile
                        int gid = mapLayer->Get(x, y);

                        //Check if the gid is different from 0 - some tiles are empty
                        if (gid != 0)
                        {
                            // Decode flip flags from GID
                            const uint32_t FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
                            const uint32_t FLIPPED_VERTICALLY_FLAG = 0x40000000;
                            const uint32_t FLIPPED_DIAGONALLY_FLAG = 0x20000000;
                            const uint32_t TILE_ID_MASK = 0x1FFFFFFF;

                            //Get Flip Variables and Correct Tile GID
                            bool flipH = (gid & FLIPPED_HORIZONTALLY_FLAG) != 0;
                            bool flipV = (gid & FLIPPED_VERTICALLY_FLAG) != 0;
                            bool flipD = (gid & FLIPPED_DIAGONALLY_FLAG) != 0;
                            uint32_t tileId = gid & TILE_ID_MASK;

                            // Determine rotation and final horizontal flip
                            float rotation = 0.0f;
                            SDL_FlipMode sdlFlip = SDL_FLIP_NONE;

                            if (!flipD)
                            {
                                if (flipH && flipV) { rotation = 180.0f; }
                                else if (flipH) { sdlFlip = SDL_FLIP_HORIZONTAL; }
                                else if (flipV) { sdlFlip = SDL_FLIP_VERTICAL; }
                            }
                            else // Diagonal Flip  == True
                            {
                                if (!flipH && !flipV) { rotation = 90.0f; sdlFlip = SDL_FLIP_HORIZONTAL; }
                                else if (flipH && !flipV) { rotation = 90.0f; }
                                else if (!flipH && flipV) { rotation = 270.0f; }
                                else if (flipH && flipV) { rotation = 270.0f; sdlFlip = SDL_FLIP_HORIZONTAL; }
                            }

                            // Obtain the tile set using GetTilesetFromTileId
                            TileSet* tileSet = GetTilesetFromTileId(tileId);

                            if (tileSet != nullptr)
                            {
                                //Get the Rect from the tileSetTexture;
                                SDL_Rect tileRect;

                                if (tileSet->animations.count(tileSet->firstGid - tileId))
                                {
                                    // Animated Tile

                                    tileRect.x = tileSet->animations[tileSet->firstGid - tileId].GetCurrentFrame().x;
                                    tileRect.y = tileSet->animations[tileSet->firstGid - tileId].GetCurrentFrame().y;
                                    tileRect.w = tileSet->animations[tileSet->firstGid - tileId].GetCurrentFrame().w;
                                    tileRect.h = tileSet->animations[tileSet->firstGid - tileId].GetCurrentFrame().h;
                                }
                                else
                                {
                                    // Static Tile

                                    tileRect.x = tileSet->GetRect(tileId).x;
                                    tileRect.y = tileSet->GetRect(tileId).y;
                                    tileRect.w = tileSet->GetRect(tileId).w;
                                    tileRect.h = tileSet->GetRect(tileId).h;
                                }

                                // Get the screen coordinates from the tile coordinates
                                Vector2D mapCoord = MapToWorld(x, y);

                                // Center point for rotation
                                SDL_FPoint center = { tileRect.w / 2, tileRect.h / 2 };

                                // Destination rectangle
                                SDL_Rect dstRect = {
                                    (int)mapCoord.getX() + tileRect.w / 2,
                                    (int)mapCoord.getY() + tileRect.h / 2,
                                    tileRect.w,
                                    tileRect.h
                                };

                                //Draw the texture
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

// Implement function to the Tileset based on a tile id
TileSet* Map::GetTilesetFromTileId(int gid) const
{
	TileSet* set = nullptr;
    for(const auto& tileset : mapData.tilesets) {
        set = tileset;
        if (gid >= tileset->firstGid && gid < tileset->firstGid + tileset->tileCount) {
			break;
        }
	}
    return set;
}

// Called before quitting
bool Map::CleanUp()
{
    LOG("Unloading map");

    // Clean up any memory allocated from tilesets/map
    for (const auto& tileset : mapData.tilesets) {
        delete tileset;
    }
    mapData.tilesets.clear();

    // Clean up all layer data
    for (const auto& layer : mapData.layers)
    {
        delete layer;
    }
    mapData.layers.clear();

    //Clean up object groups
    for (const auto& objectGroup : mapData.objectGroups) {
        objectGroup->objects.clear();
    }
    mapData.objectGroups.clear();

    // Clean up collider list
    for (const auto& collider : colliderList) 
    {
        Engine::GetInstance().physics->DeletePhysBody(collider);
    }
    colliderList.clear();

    return true;
}

// Load new map
bool Map::Load(std::string path, std::string fileName)
{
    bool ret = false;

    // Assigns the name of the map file and the path
    mapFileName = fileName;
    mapPath = path;
    std::string mapPathName = mapPath + mapFileName;

    pugi::xml_parse_result result = mapFileXML.load_file(mapPathName.c_str());

    if(result == NULL)
	{
		LOG("Could not load map xml file %s. pugi error: %s", mapPathName.c_str(), result.description());
		ret = false;
    }
    else 
    {
        // Load the map properties
        // retrieve the paremeters of the <map> node and store the into the mapData struct
        mapData.width = mapFileXML.child("map").attribute("width").as_int();
        mapData.height = mapFileXML.child("map").attribute("height").as_int();
        mapData.tileWidth = mapFileXML.child("map").attribute("tilewidth").as_int();
        mapData.tileHeight = mapFileXML.child("map").attribute("tileheight").as_int();

       //Iterate the Tileset
        for(pugi::xml_node tilesetNode = mapFileXML.child("map").child("tileset"); tilesetNode!=NULL; tilesetNode = tilesetNode.next_sibling("tileset"))
		{
            //Load Tileset attributes
			TileSet* tileSet = new TileSet();
            tileSet->firstGid = tilesetNode.attribute("firstgid").as_int();
            tileSet->name = tilesetNode.attribute("name").as_string();
            tileSet->tileWidth = tilesetNode.attribute("tilewidth").as_int();
            tileSet->tileHeight = tilesetNode.attribute("tileheight").as_int();
            tileSet->spacing = tilesetNode.attribute("spacing").as_int();
            tileSet->margin = tilesetNode.attribute("margin").as_int();
            tileSet->tileCount = tilesetNode.attribute("tilecount").as_int();
            tileSet->columns = tilesetNode.attribute("columns").as_int();

			//Load the tileset image
			std::string imgName = tilesetNode.child("image").attribute("source").as_string();
            tileSet->texture = Engine::GetInstance().textures->Load((mapPath+imgName).c_str());

            // Load animation
            for (pugi::xml_node tileNode = tilesetNode.child("tile"); tileNode; tileNode = tileNode.next_sibling("tile"))
            {
                int tileId = tileNode.attribute("id").as_int();
                pugi::xml_node animNode = tileNode.child("animation");

                if (animNode)
                {
                    Animation anim;

                    //Iterate all Animation Frames
                    for (pugi::xml_node frameNode = animNode.child("frame"); frameNode; frameNode = frameNode.next_sibling("frame"))
                    {
                        int frameTileId = frameNode.attribute("tileid").as_int();
                        int duration = frameNode.attribute("duration").as_int();

                        SDL_Rect rect = tileSet->GetRect(tileSet->firstGid + frameTileId);
                        anim.AddFrame(rect, duration);
                    }

                    anim.Reset();
                    tileSet->animations[tileId] = anim; //Save Animation
                }
            }

			mapData.tilesets.push_back(tileSet);
		}

        // Iterate all layers in the TMX and load each of them
        for (pugi::xml_node layerNode = mapFileXML.child("map").child("layer"); layerNode != NULL; layerNode = layerNode.next_sibling("layer")) {

            //Load the attributes and saved in a new MapLayer
            MapLayer* mapLayer = new MapLayer();
            mapLayer->id = layerNode.attribute("id").as_int();
            mapLayer->name = layerNode.attribute("name").as_string();
            mapLayer->width = layerNode.attribute("width").as_int();
            mapLayer->height = layerNode.attribute("height").as_int();

            // Call Load Layer Properties
            LoadProperties(layerNode, mapLayer->properties);

            // Iterate over all the tiles and assign the values in the data array
            for (pugi::xml_node tileNode = layerNode.child("data").child("tile"); tileNode != NULL; tileNode = tileNode.next_sibling("tile")) 
            {
                mapLayer->tiles.push_back(tileNode.attribute("gid").as_int());
            }

            // Add the layer to the map
            mapData.layers.push_back(mapLayer);
        }

        // Load Object Group
        for (pugi::xml_node objectGroupNode = mapFileXML.child("map").child("objectgroup"); objectGroupNode != NULL; objectGroupNode = objectGroupNode.next_sibling("objectgroup")) 
        {
            ObjectGroup* objectgroup = new ObjectGroup();

            for (pugi::xml_node objectNode = objectGroupNode.child("object"); objectNode != NULL; objectNode = objectNode.next_sibling("object")) 
            {
                ObjectGroup::Object* o = new ObjectGroup::Object;

                // Save all Object attributes
                o->id = objectNode.attribute("id").as_int();
                o->x = objectNode.attribute("x").as_float();
                o->y = objectNode.attribute("y").as_float();
                o->width = objectNode.attribute("width").as_float();
                o->height = objectNode.attribute("height").as_float();

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

                // Load Individual Object Properties
                LoadProperties(objectNode, o->properties);

                objectgroup->objects.push_back(o);
            }

            // Load Object Layer Properties
            LoadProperties(objectGroupNode, objectgroup->properties);

            //Add the layer to the map
            mapData.objectGroups.push_back(objectgroup);
        }

        // Creation of colliders and assign their type
        for (const auto& objectsGroups : mapData.objectGroups)
        {
            if (objectsGroups->properties.GetProperty("Square") != NULL and objectsGroups->properties.GetProperty("Square")->value) // Square
            {
                for (const auto& obj : objectsGroups->objects)
                {
                    PhysBody* collider;
                    if (objectsGroups->properties.GetProperty("Sensor") != NULL and objectsGroups->properties.GetProperty("Sensor")->value) // Trigger
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
                    else if (objectsGroups->properties.GetProperty("Ground") != NULL and objectsGroups->properties.GetProperty("Ground")->value) 
                    {
                        collider->ctype = ColliderType::GROUND;
                    }
                    else if (objectsGroups->properties.GetProperty("Wall") != NULL and objectsGroups->properties.GetProperty("Wall")->value) 
                    {
                        collider->ctype = ColliderType::WALL;
                    }
                    else if (objectsGroups->properties.GetProperty("Ceiling") != NULL and objectsGroups->properties.GetProperty("Ceiling")->value) 
                    {
                        collider->ctype = ColliderType::CEILING;
                    }
                    else if (objectsGroups->properties.GetProperty("Door") != NULL and objectsGroups->properties.GetProperty("Door")->value)
                    {
                        collider->ctype = ColliderType::DOOR;

                        // TODO: Assign Listener

                        Door* newDoor = new Door;
                        newDoor->body = collider;
                        newDoor->teleportTo = obj->properties.GetProperty("TeleportTo")->value2;
                        mapData.doors.push_back(newDoor);
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
                        collider->ctype = ColliderType::GROUND;
                    }
                    else
                    {
                        collider->ctype = ColliderType::UNKNOWN;
                    }
                    colliderList.push_back(collider);
                }
            }
            else if (objectsGroups->properties.GetProperty("Chain") != NULL and objectsGroups->properties.GetProperty("Triangle")->value) // Triangle / Chain
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
                        collider->ctype = ColliderType::GROUND;
                    }
                    else
                    {
                        collider->ctype = ColliderType::UNKNOWN;
                    }
                    colliderList.push_back(collider);
                }
            }
        }


        ret = true;

        // LOG all the data loaded iterate all tilesetsand LOG everything
        if (ret == true)
        {
            LOG("Successfully parsed map XML file :%s", fileName.c_str());
            LOG("width : %d height : %d", mapData.width, mapData.height);
            LOG("tile_width : %d tile_height : %d", mapData.tileWidth, mapData.tileHeight);
            LOG("Tilesets----");

            //iterate the tilesets
            for (const auto& tileset : mapData.tilesets) {
                LOG("name : %s firstgid : %d", tileset->name.c_str(), tileset->firstGid);
                LOG("tile width : %d tile height : %d", tileset->tileWidth, tileset->tileHeight);
                LOG("spacing : %d margin : %d", tileset->spacing, tileset->margin);
            }
            			
            LOG("Layers----");

            for (const auto& layer : mapData.layers) {
                LOG("id : %d name : %s", layer->id, layer->name.c_str());
				LOG("Layer width : %d Layer height : %d", layer->width, layer->height);
            }   

            LOG("Objects----");

            for (const auto& objGroups : mapData.objectGroups) {
                LOG("Properties: ");
                for (const auto& ogproperties : objGroups->properties.propertyList)
                {
                    LOG("name : %s  value : %d ", ogproperties->name.c_str(), ogproperties->value);
                }
                LOG("Objects");
                for (const auto& ogobjects : objGroups->objects)
                {
                    LOG("id : %d  points : %d ", ogobjects->id, ogobjects->points);
                    LOG("x : %d  y : %d ", ogobjects->x, ogobjects->y);
                    LOG("width : %d  height : %d ", ogobjects->width, ogobjects->height);
                }
            }
        }
        else 
        {
            LOG("Error while parsing map file: %s", mapPathName.c_str());
        }
    }

    mapLoaded = ret;
    return ret;
}

// Method that translates x,y coordinates from map positions to world positions
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

// Load a group of properties from a node and fill a list with it
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
        else if (propertieNode.attribute("type").as_string() == std::string("int") )
        {
            p->value = propertieNode.attribute("value").as_int();
        }
        else if (propertieNode.attribute("type").as_string() == std::string("file"))
        {
            p->value2 = propertieNode.attribute("value").as_string();
        }

        properties.propertyList.push_back(p);
    }

    return ret;
}

MapLayer* Map::GetNavigationLayer(bool ground)
{
    for (const auto& layer : mapData.layers) {
        if (layer->properties.GetProperty("Navigation") != NULL &&
            layer->properties.GetProperty("Navigation")->value)
        {
            if (ground &&
                layer->properties.GetProperty("Ground") != NULL &&
                layer->properties.GetProperty("Ground")->value)
            {
                return layer;
            }
            else  if (!ground &&
                layer->properties.GetProperty("Fly") != NULL &&
                layer->properties.GetProperty("Fly")->value)
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

void Map::SpawnEntities()
{
    for (pugi::xml_node objectGroupNode = mapFileXML.child("map").child("objectgroup"); objectGroupNode != NULL; objectGroupNode = objectGroupNode.next_sibling("objectgroup"))
    {
        if (objectGroupNode.attribute("name").as_string() == std::string("EntitiesSpawnPoints"))
        {
            for (pugi::xml_node objectNode = objectGroupNode.child("object"); objectNode != NULL; objectNode = objectNode.next_sibling("object"))
            {
                std::string entityType = objectNode.attribute("type").as_string();
                float x = objectNode.attribute("x").as_float();
                float y = objectNode.attribute("y").as_float();

                if (entityType == std::string("Player"))
                {
                    std::shared_ptr<Player> player = Engine::GetInstance().scene->GetPlayer();

                    if (player == NULL) //if player doesnt exist
                    {
                        player = std::dynamic_pointer_cast<Player>(Engine::GetInstance().entityManager->CreateEntity(EntityType::PLAYER));
                        player->position = Vector2D(x, y);
                        //player->Start();
                    }
                    else // if player exists
                    {
                        player->position = (Vector2D(x, y));
                    }
                    Engine::GetInstance().scene->SetPlayer(player);
                }
                else if (entityType == std::string("Test"))
                {
                    std::shared_ptr<TestEnemy> test = std::dynamic_pointer_cast<TestEnemy>(Engine::GetInstance().entityManager->CreateEntity(EntityType::ENEMY));
                    test->position = Vector2D(x, y);
                    //test->Start();
                }
            }
        }
    }
    //for (const auto& mapLayer : mapData.layers)
    //{
    //    for (int i = 0; i < mapData.width; i++)
    //    {
    //        for (int j = 0; j < mapData.height; j++)
    //        {
    //            //Get the gid from tile
    //            uint32_t gid = mapLayer->Get(i, j);

    //            //Check if the gid is different from 0 - some tiles are empty
    //            if (gid != 0)
    //            {
    //                TileSet* tileSet = GetTilesetFromTileId(gid);

    //                if (tileSet != nullptr)
    //                {
    //                    // If it's a goldcoin
    //                    if (tileSet->name == "goldCoin")
    //                    {
    //                        // Create new Coin
    //                        std::shared_ptr<Item> item = std::dynamic_pointer_cast<Item>(Engine::GetInstance().entityManager->CreateEntity(EntityType::ITEM));
    //                        item->position = Vector2D(MapToWorld(i, j).getX(), MapToWorld(i, j).getY());
    //                    }
    //                }

    //            }
    //        }
    //    }
    //}
}

std::string Map::DoorInfo(PhysBody* door)
{
    for (const auto& ndoor : mapData.doors) 
    {
        if (ndoor->body == door)
        {
            return ndoor->teleportTo;
        }
    }
    return std::string();
}




