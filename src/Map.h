#pragma once

#include "Module.h"
#include <list>
#include <vector>

#include <box2d/box2d.h>
#include "Animation.h"

#include "Physics.h"

struct Properties
{
    struct Property
    {
        std::string name;
        int value; //We assume that we are going to work only with bool for the moment
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

struct ObjectGroup
{
    struct Object
    {
        float id, x, y, width, height;
        std::vector<b2Vec2> points;
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
    std::vector<int> tiles;
    Properties properties;

    // Function to get the gid value of i,j
    unsigned int Get(int i, int j) const
    {
        return tiles[(j * width) + i];
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
    std::list<MapLayer*> layers;
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

	// Get the map size in pixels
	Vector2D GetMapSizeInPixels();
    Vector2D GetMapSizeInTiles();


    // Getters
    int GetTileWidth() { return mapData.tileWidth;  }

    int GetTileHeight() { return mapData.tileHeight; }


public: 
    std::string mapFileName;
    std::string mapPath;

private:
    bool mapLoaded;
    MapData mapData;
    std::list<PhysBody*> colliderList;
};