#include "Minimap.h"
#include "Engine.h"
#include "Render.h"
#include "Window.h"
#include "Map.h"

constexpr float CELL_SIZE = 100.0f;

void Minimap::CreateRoom(const std::string& id)
{
    MinimapRoom newRoom;
    newRoom.roomID = id;
    
    if (id == "Nexo.tmx")
    {
        newRoom.x = 0;
        newRoom.y = 0;
    }
    else if (id == "Forest_01.tmx")
    {
        newRoom.x = -1;
        newRoom.y = 0;
    }
    else if (id == "Forest_02.tmx")
    {
        newRoom.x = -2;
        newRoom.y = 0;
    }
    else if (id == "Forest_03.tmx")
    {
        newRoom.x = -2.5;
        newRoom.y = -0.5f;
    }
    else if (id == "Forest_04.tmx")
    {
        newRoom.x = -3;
        newRoom.y = 0;
    }
    else if (id == "Forest_05.tmx")
    {
        newRoom.x = -4;
        newRoom.y = 0;
    }
    else if (id == "Mountain_01.tmx")
    {
        newRoom.x = 1;
        newRoom.y = 0;
    }
    else if (id == "Mountain_02.tmx")
    {
        newRoom.x = 1;
        newRoom.y = -0.5f;
    }
    else if (id == "Mountain_03.tmx")
    {
        newRoom.x = 1;
        newRoom.y = -2;
    }
    else if (id == "Catacombs_01_F.tmx")
    {
        newRoom.x = -4;
        newRoom.y = 1;
    }
    else if (id == "Catacombs_02_F.tmx")
    {
        newRoom.x = -2;
        newRoom.y = 1;
    }
    else if (id == "Catacombs_01_M.tmx")
    {
        newRoom.x = 3;
        newRoom.y = 1;
    }
    else if (id == "Catacombs_02_M.tmx")
    {
        newRoom.x = 0.5;
        newRoom.y = 1;
    }
    else if (id == "Catacombs_Boss_Dead.tmx" || id == "Catacombs_Boss_Alive.tmx")
    {
        newRoom.x = 0;
        newRoom.y = 1;
    }
    else
    {
        return;
    }

    int mapW = Engine::GetInstance().map->GetMapSizeInPixels().getX();
    int mapH = Engine::GetInstance().map->GetMapSizeInPixels().getY();
    newRoom.w =  mapW / 200;
    newRoom.h =  mapH / 200;

    std::vector<Door> paths = Engine::GetInstance().map->GetPaths();

    for (const auto& path : paths)
    {
        MM_Path newPath;
        newPath.teleportTo = path.teleportTo;
        newPath.x = (float)path.x / (float)mapW ;
        newPath.y = (float)path.y / (float)mapH;

        newRoom.paths[path.teleportTo] = newPath;
    }

    rooms[id] = newRoom;
}

void Minimap::SetCurrentRoom(const std::string& id)
{
    currentRoom = id;
}

void Minimap::DrawMinimap()
{
    int x, y;
    Engine::GetInstance().window->GetWindowSize(x,y);

    float originX = x/2;
    float originY = y/2;

    SDL_Renderer* renderer = Engine::GetInstance().render->renderer;
    
    for (auto it = rooms.begin(); it != rooms.end(); ++it)
    {
        std::string id = it->first;
        MinimapRoom room = it->second;

        SDL_FRect rect;

        rect.x = originX + room.x * CELL_SIZE;
        rect.y = originY + room.y * CELL_SIZE;

        rect.w = room.w;
        rect.h = room.h;

        if (id == currentRoom)
        {
            SDL_SetRenderDrawColor(
                renderer,
                255,
                255,
                255,
                255
            );
        }
        else
        {
            SDL_SetRenderDrawColor(
                renderer,
                120,
                120,
                120,
                255
            );
        }

        SDL_RenderFillRect(renderer, &rect);

        for (auto it = room.paths.begin(); it != room.paths.end(); ++it)
        {
            //Render Path
            const MM_Path& path = it->second;

            float pathX = rect.x + path.x * rect.w;

            float pathY = rect.y + path.y * rect.h;

            SDL_FRect pathRect;

            pathRect.x = pathX;
            pathRect.y = pathY;

            pathRect.w = 3;
            pathRect.h = 3;

            SDL_SetRenderDrawColor(
                renderer,
                255,
                10,
                10,
                255
            );

            SDL_RenderFillRect(renderer, &pathRect);

            //Render Lines (Conections between paths)
            auto targetIt = rooms.find(path.teleportTo);

            if (targetIt == rooms.end())
                continue;

            MinimapRoom targetRoom = targetIt->second;
            
            int mapX = originX + targetRoom.x * CELL_SIZE;
            int mapY = originY + targetRoom.y * CELL_SIZE;

            float path2X = mapX + targetRoom.paths[id].x * targetRoom.w;

            float path2Y = mapY + targetRoom.paths[id].y * targetRoom.h;

            SDL_FRect path2Rect;

            float startX = pathRect.x + pathRect.w / 2;
            float startY = pathRect.y + pathRect.h / 2;

            float endX = path2X + pathRect.w / 2;
            float endY = path2Y + pathRect.h / 2;

            SDL_RenderLine(
                renderer,
                startX,
                startY,
                endX,
                endY
            );

        }
    }
}