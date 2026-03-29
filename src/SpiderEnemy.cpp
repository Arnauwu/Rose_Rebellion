#include "SpiderEnemy.h"
#include "Engine.h"
#include "Physics.h"
#include "Render.h"

SpiderEnemy::SpiderEnemy() : Enemy(EntityType::ENEMY) {
    name = "spider";
}

SpiderEnemy::~SpiderEnemy() {

}

bool SpiderEnemy::Awake() {
    return true;
}

bool SpiderEnemy::Start() {

    return true;
}

bool SpiderEnemy::Update(float dt) {

    return true;
}

void SpiderEnemy::PerformPathfinding()
{
    //Reset path
    pathfinding->ResetPath(GetTilePos());

    //Get the position of the enemy
    Vector2D pos = GetPosition();

    //Get the position of the player
    Vector2D playerPos = Engine::GetInstance().scene->GetPlayerPosition();

    playerTileDist = sqrt(pos.distanceSquared(playerPos)) / 32;
    int iter = 0;

    while (pathfinding->pathTiles.empty() && playerTileDist < vision && iter < MaxIterations)
    {
        pathfinding->PropagateAStar();
        iter++;
    }
}

void SpiderEnemy::GetPhysicsValues() {
    // Read current velocity
    velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
    velocity = { 0, velocity.y };
}

void SpiderEnemy::Move() {
   
}

void SpiderEnemy::ApplyPhysics() {

    // Apply velocity via helper
    b2Vec2 currentVel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
    Engine::GetInstance().physics->SetLinearVelocity(pbody, { velocity.x, currentVel.y });
}

void SpiderEnemy::Draw(float dt)
{

    const SDL_Rect& animFrame = anims.GetCurrentFrame();

    // Update render position using your PhysBody helper
    int x, y;
    pbody->GetPosition(x, y);
    position.setX((float)x);
    position.setY((float)y);

    // Draw pathfinding debug
    if (Engine::GetInstance().physics->GetDebug())
    {
        pathfinding->DrawPath();
    }

    //Draw the player using the texture and the current animation frame
    Engine::GetInstance().render->DrawTexture(texture, x - texW / 2, y - texH / 2, &animFrame);
}

Vector2D SpiderEnemy::GetTilePos()
{
    //Get the position of the enemy
    Vector2D pos = GetPosition();
    //Convert to tile coordinates
    Vector2D tilePos = Engine::GetInstance().map->WorldToMap((int)pos.getX(), (int)pos.getY());
    return tilePos;
}

void SpiderEnemy::OnCollision(PhysBody* physA, PhysBody* physB) {

    if (physB->ctype == ColliderType::GROUND || physB->ctype == ColliderType::WALL || physB->ctype == ColliderType::CEILING) {

        int next = (int)currentFacing + 1;
        if (next > 3) next = 0;
        currentFacing = (Facing)next;

        LOG("Spdier rotating to: %d", currentFacing);
    }
}