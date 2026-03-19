
#include "Engine.h"
#include "Render.h"
#include "Textures.h"
#include "Map.h"
#include "Log.h"
#include "Physics.h"
#include "SavePoint.h"
#include "EntityManager.h"
#include <math.h>

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

    if (mapLoaded) {

        // L07 TODO 5: Prepare the loop to draw all tiles in a layer + DrawTexture()
        // iterate all tiles in a layer
        for (const auto& mapLayer : mapData.layers) {
            //L09 TODO 7: Check if the property Draw exist get the value, if it's true draw the lawyer
            if (mapLayer->properties.GetProperty("Draw") != NULL && mapLayer->properties.GetProperty("Draw")->value == true) {
				for (int i = 0; i < mapData.width; i++) {
					for (int j = 0; j < mapData.height; j++) {
						// L07 TODO 9: Complete the draw function
                        //Get the gid from tile
                        int gid = mapLayer->Get(i, j);

                        //Check if the gid is different from 0 - some tiles are empty
                        if (gid != 0) {
                            //L09: TODO 3: Obtain the tile set using GetTilesetFromTileId
                            TileSet* tileSet = GetTilesetFromTileId(gid);
                            if (tileSet != nullptr) {
                                //Get the Rect from the tileSetTexture;
                                SDL_Rect tileRect = tileSet->GetRect(gid);
                                //Get the screen coordinates from the tile coordinates
                                Vector2D mapCoord = MapToWorld(i, j);
                                //Draw the texture
                                Engine::GetInstance().render->DrawTexture(tileSet->texture, (int)mapCoord.getX(), (int)mapCoord.getY(), &tileRect);
                            }
                        }
                    }
                }
            }
        }
    }

    return ret;
}

// L09: TODO 2: Implement function to the Tileset based on a tile id
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

    pugi::xml_document mapFileXML;
    pugi::xml_parse_result result = mapFileXML.load_file(mapPathName.c_str());

    if(result == NULL)
	{
		LOG("Could not load map xml file %s. pugi error: %s", mapPathName.c_str(), result.description());
		ret = false;
    }
    else {

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
            for (pugi::xml_node tileNode = layerNode.child("data").child("tile"); tileNode != NULL; tileNode = tileNode.next_sibling("tile")) {
                mapLayer->tiles.push_back(tileNode.attribute("gid").as_int());
            }

            // Add the layer to the map
            mapData.layers.push_back(mapLayer);
        }

        // Load Object Group
        for (pugi::xml_node objectGroupNode = mapFileXML.child("map").child("objectgroup"); objectGroupNode != NULL; objectGroupNode = objectGroupNode.next_sibling("objectgroup")) {

            ObjectGroup* objectgroup = new ObjectGroup();

            for (pugi::xml_node objectNode = objectGroupNode.child("object"); objectNode != NULL; objectNode = objectNode.next_sibling("object")) {
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
                objectgroup->objects.push_back(o);
            }

            LoadProperties(objectGroupNode, objectgroup->properties);

            //Add the layer to the map
            mapData.objectGroups.push_back(objectgroup);
        }

        // Creation of colliders and assign their type
        for (const auto& objectsGroups : mapData.objectGroups)
        {
            LOG("SUCCESS: Found SavePoint Layer in Tiled!");
               
                if (objectsGroups->properties.GetProperty("SavePoint") != NULL && objectsGroups->properties.GetProperty("SavePoint")->value)
                {
                    for (const auto& obj : objectsGroups->objects)
                    {
                        std::shared_ptr<Entity> newEntity = Engine::GetInstance().entityManager->CreateEntity(EntityType::SAVEPOINT);
                        SavePoint* sp = (SavePoint*)newEntity.get();

                        sp->position = Vector2D(obj->x, obj->y);
                    }
                }

                else if (objectsGroups->properties.GetProperty("Square") != NULL and objectsGroups->properties.GetProperty("Square")->value) // Square
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

        if (mapFileXML) mapFileXML.reset(); //TODO: CHECK THIS

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
        else
        {
            p->value = propertieNode.attribute("value").as_int();
        }

        properties.propertyList.push_back(p);
    }

    return ret;
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




