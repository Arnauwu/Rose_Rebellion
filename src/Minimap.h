#pragma once

#include <SDL3/SDL.h>

#include <unordered_map>
#include <string>

struct MinimapRoom
{
    std::string roomID;

    int x = 0;
    int y = 0;

    int w = 0;
    int h = 0;

    bool discovered = false;
};

class Minimap
{
public:
    void CreateRoom(const std::string& id, int x, int y, int mapWidth, int mapHeight);

    void DiscoverRoom(const std::string& id);

    void SetCurrentRoom(const std::string& id);

    void DrawMinimap();

private:
    std::unordered_map<std::string,MinimapRoom> rooms;
    std::string currentRoom;
};