#include "Enemy.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "SceneManager.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"
#include "Map.h"

bool Enemy::Update(float dt)
{
	if (!active) { return true; }
	PerformPathfinding();
	GetPhysicsValues();
	Move();
	Knockback();
	ApplyPhysics();
	Draw(dt);

	return true;
}

void Enemy::PerformPathfinding() {

	//Reset path
	pathfinding->ResetPath(GetTilePos());

	//Get the position of the enemy
	Vector2D pos = GetPosition();

	//Get the position of the player
	Vector2D playerPos = Engine::GetInstance().sceneManager->GetPlayerPosition();

	int tileW = Engine::GetInstance().map->GetTileWidth();
    playerTileDist = (int)(sqrt(pos.distanceSquared(playerPos)) / tileW);
	int iter = 0;

	while (pathfinding->pathTiles.empty() && playerTileDist < vision && iter < MaxIterations)
	{
		pathfinding->PropagateAStar();
		iter++;
	}
}

void Enemy::Draw(float dt) {

	anims.Update(dt);
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

Vector2D Enemy::GetTilePos()
{
	//Get the position of the enemy
	Vector2D pos = GetPosition();
	//Convert to tile coordinates
	Vector2D tilePos = Engine::GetInstance().map->WorldToMap((int)pos.getX(), (int)pos.getY());
	return tilePos;
}

bool Enemy::CleanUp()
{
	LOG("Cleanup enemy");
	active = false;
	Engine::GetInstance().textures->UnLoad(texture);
	Engine::GetInstance().physics->DeletePhysBody(pbody);
	return true;
}


//Define OnCollision function for the enemy. 
void Enemy::OnCollision(PhysBody* physA, PhysBody* physB) {
	switch (physB->ctype)
	{
	default:
		break;
	}
}

void Enemy::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
	switch (physB->ctype)
	{
	default:
		break;
	}
}

void Enemy::SetPosition(Vector2D pos)
{
	position = pos;
}

Vector2D Enemy::GetPosition() 
{
	int x, y;
	pbody->GetPosition(x, y);
	return Vector2D((float)x, (float)y);
}

bool Enemy::Destroy()
{
	LOG("Enemy Death");
	active = false;
	pendingToDelete = true;
	return true;
}

