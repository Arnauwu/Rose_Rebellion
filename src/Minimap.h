#pragma once

#include <SDL3/SDL.h>

#include <unordered_map>
#include <string>

struct MM_Path
{
    std::string teleportTo;
    float x;
    float y;
};

struct MinimapRoom
{
    std::string roomID;

    float x = 0;
    float y = 0;

    int w = 0;
    int h = 0;

    std::unordered_map<std::string, MM_Path> paths;
};

class Minimap
{
public:
    void CreateRoom(const std::string& id);

    void SetCurrentRoom(const std::string& id);

    void DrawMinimap();

private:
    std::unordered_map<std::string, MinimapRoom> rooms;
    std::string currentRoom;
};