#pragma once

#include "Module.h"
#include <list>
#include <vector>
#include <string>

#include <box2d/box2d.h>
#include "Animation.h"

#include "Physics.h"

struct Properties
{
    struct Property
    {
        std::string name;
        int value;
        std::string value2;
    };

    std::list<Property*> propertyList;

    ~Properties()
    {
        for (const auto& property : propertyList)
        {
            delete property;
        }

        propertyList.clear();
    }

    // Method to ask for the value of a custom property
    Property* GetProperty(const std::string name)
    {
        for (const auto& property : propertyList) 
        {
            if (property->name == name) 
            {
                return property;
            }
        }

        return nullptr;
    }

};

struct Door
{
    PhysBody* body;
    std::string teleportTo;
    bool needsKey;
    std::string uniqueId;
    bool underMaintenance;
    bool DoorClose;
};

struct Path
{
    PhysBody* body;
    std::string teleportTo;
};

struct PlayerSpawnPoint
{
    std::string fromRoom;
    Vector2D position;
};

struct ObjectGroup
{
    struct Object
    {
        float id, x, y, width, height;
        unsigned int gid;
        std::vector<b2Vec2> points;
        Properties properties;
    };
    
    std::list<Object*> objects;
    Properties properties;

    ~ObjectGroup()
    {
        for (const auto& object : objects)
        {
            delete object;
        }

        objects.clear();
    }
};

struct MapLayer
{
    int id;
    std::string name;
    int width;
    int height;
    std::vector<unsigned int> tiles;
    Properties properties;

    // Function to get the gid value of i,j
    unsigned int Get(int x, int y) const
    {
        int index = (y * width) + x;

        if (tiles.empty() || index < 0 || index >= tiles.size())
        {
            return 0;
        }

        return tiles[index];
    }
};

struct TileSet
{
    int firstGid;
    std::string name;
    int tileWidth;
    int tileHeight;
    int spacing;
    int margin;
    int tileCount;
    int columns;
    SDL_Texture* texture;

    std::unordered_map<int, Animation> animations; // tileId -> Animation

    // Mthod that receives the gid and returns a Rect
    SDL_Rect GetRect(unsigned int gid) {
        SDL_Rect rect = { 0 };
        if (columns <= 0) return rect; // Retornar rect vacío si columns es inválido
        int relativeIndex = gid - firstGid;
        rect.w = tileWidth;
        rect.h = tileHeight;
        rect.x = margin + (tileWidth + spacing) * (relativeIndex % columns);
        rect.y = margin + (tileHeight + spacing) * (relativeIndex / columns);

        return rect;
    }

};

struct MapData
{
	int width;
	int height;
	int tileWidth;
	int tileHeight;
    std::list<TileSet*> tilesets;
    std::list<ObjectGroup*> objectGroups;
    std::list<Door*> doors;
    std::list<Path*> paths;
    std::list<MapLayer*> layers;
    std::list<PlayerSpawnPoint*> spawnPoints;
};

class Map : public Module
{
public:

    Map();

    // Destructor
    virtual ~Map();

    // Called before render is available
    bool Awake();

    // Called before the first frame
    bool Start();

    // Called each loop iteration
    bool Update(float dt);

    // Called before quitting
    bool CleanUp();

    // Load new map
    bool Load(std::string path, std::string mapFileName);

    // Method that translates x,y coordinates from map positions to world positions
    Vector2D MapToWorld(int i, int j) const;
    Vector2D WorldToMap(int x, int y);

    // Function to the Tileset based on a tile id
    TileSet* GetTilesetFromTileId(int gid) const;

    // Load a group of properties 
    bool LoadProperties(pugi::xml_node& node, Properties& properties);

    // Getters
    MapLayer* GetNavigationLayer(bool ground, int* blockedGID, int* highGID);

	// Get the map size in pixels
	Vector2D GetMapSizeInPixels();
    Vector2D GetMapSizeInTiles();


    int GetTileWidth() { return mapData.tileWidth;  }

    int GetTileHeight() { return mapData.tileHeight; }

    // Entities
    void SpawnEntities();
    Vector2D GetPlayerSpawnPoint(const std::string& fromRoom);
    //Door
    std::string DoorInfo(PhysBody* door);
    std::string GetDoorUniqueId(PhysBody* door);
    bool DoorNeedsKey(PhysBody* door);
    bool DoorUnderMaintenance(PhysBody* door);
    bool DoorClosed(PhysBody* door);
    std::string PathInfo(PhysBody* path);

public: 
    std::string mapFileName;
    std::string mapPath;

private:
    bool mapLoaded;
    MapData mapData;
    pugi::xml_document mapFileXML;
    std::list<PhysBody*> colliderList;
};