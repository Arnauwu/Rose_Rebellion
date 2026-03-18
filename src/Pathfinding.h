#pragma once
#include <list>
#include <queue>
#include "Vector2D.h"
#include <SDL3/SDL.h>
#include "Map.h"

class Pathfinding
{

public:
    Pathfinding(bool ground);

    ~Pathfinding();

    // L11: BFS Pathfinding methods
    void ResetPath(Vector2D pos);
    void DrawPath();
    bool IsWalkable(int x, int y);

    int MovementCost(int x, int y);
    void ComputePath(int x, int y);

    void PropagateAStar();

private:
    int Find(std::list<Vector2D> vector, Vector2D elem);

public:

    Map* map;
    MapLayer* layerNav;

    std::list<Vector2D> visited;
    SDL_Texture* pathTex = nullptr;
    Vector2D destination;

    std::vector<Vector2D> breadcrumbs; //list of tiles that form the path
    std::vector<std::vector<int>> costSoFar; //matrix that stores the accumulated cost in the propagation of the Dijkstra algorithm
    std::list<Vector2D> pathTiles; //list of tiles that form the path


    std::priority_queue<std::pair<int, Vector2D>, std::vector<std::pair<int, Vector2D>>, std::greater<std::pair<int, Vector2D>> > frontierAStar;


    SDL_Texture* tileX = nullptr; //texture used to show the path 

    int currentLevel = 0;

    int blockedGid = 49; //Gid of the tiles that block the path - Important adjust this value to your map
    int highCostGid = 50; //Gid of the tiles that have high cost - Important adjust this value to your map
};

