#include "Minimap.h"
#include "Engine.h"
#include "Render.h"

constexpr float CELL_SIZE = 20.0f;

void Minimap::CreateRoom(
    const std::string& id,
    int x,
    int y,
    int mapWidth,
    int mapHeight
)
{
    rooms[id] = {
        id,
        x,
        y,
        false
    };
}

void Minimap::DiscoverRoom(const std::string& id)
{
    auto& room = rooms[id];

    room.discovered = true;
}

void Minimap::SetCurrentRoom(const std::string& id)
{
    currentRoom = id;
}

void Minimap::DrawMinimap()
{
    constexpr float originX = 40.0f;
    constexpr float originY = 40.0f;

    SDL_Renderer* renderer = Engine::GetInstance().render->renderer;
    
    for (auto it = rooms.begin(); it != rooms.end(); ++it)
    {
        std::string id = it->first;
        MinimapRoom room = it->second;

        if (!room.discovered)
            continue;

        SDL_FRect rect;

        rect.x = originX + room.x * CELL_SIZE;
        rect.y = originY + room.y * CELL_SIZE;

        rect.w = CELL_SIZE - 2;
        rect.h = CELL_SIZE - 2;

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
    }
}