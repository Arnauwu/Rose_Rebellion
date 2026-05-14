#include "ToxicBall.h"
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

ToxicBall::ToxicBall() : Enemy(EntityType::TOXIC_BALL)
{
	name = "ToxicBall";
}

ToxicBall::~ToxicBall() {

}

bool ToxicBall::Awake() {
	return true;
}

bool ToxicBall::Start()
{
	//TO DO: CHANGE SOUNDS & TEXTURES

	// Initialize enemy parameters
	std::unordered_map<int, std::string> aliases = { {0,"startSpin"},{4,"spin"},{9,"dead"},{18,"walk"} };
	anims.LoadFromTSX("Assets/Textures/Entities/Enemies/Cucafera/Cucafera.tsx", aliases);
	anims.SetCurrent("idle");

	texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/Cucafera/Cucafera.png");

	//Load Audio
	chocarToxicBall = Engine::GetInstance().audio->LoadFx("");

	//Add physics to the enemy - initialize physics body
	texW = 64;
	texH = 64;
	pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX() + texW / 2, (int)position.getY() + texH / 2, (texW * 2) / 5, bodyType::DYNAMIC);

	//No Gravity
	Engine::GetInstance().physics->SetGravityScale(pbody, 0.0f);

	//Assign enemy class (using "this") to the listener of the pbody. This makes the Physics module to call the OnCollision method
	pbody->listener = this;

	//ssign collider type
	pbody->ctype = ColliderType::ENEMY;

	// Initialize pathfinding
	pathfinding = std::make_shared<Pathfinding>(true); //Considered Floating, not capable to fly, so Ground NavLayer

	//Reset pathfinding
	pathfinding->ResetPath(GetTilePos());

	pathFindingCooldown.Start();

	//Stats
	vision = 20;
	speed = 3.0f;
	knockbackForce = 7.5f;

	maxHealth = 60;
	currentHealth = 60;

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);



	hoverTimer.Start();
	hoverCooldown.Start();
	return true;
}

bool ToxicBall::Update(float dt)
{

	if (!active) return true;

	if (Engine::GetInstance().sceneManager->isGamePaused == false && isdead == false)
	{
		if (pathFindingCooldown.ReadMSec() > 500)
		{
			PerformPathfinding();
			pathFindingCooldown.Start();
		}

		GetPhysicsValues();
		Move();
		ApplyPhysics();
	}

	Draw(dt);

	return true;
}

void ToxicBall::PerformPathfinding()
{
	//Reset path
	pathfinding->ResetPath(GetTilePos());

	//Get the position of the enemy
	Vector2D pos = GetPosition();

	//Get the position of the player

	Player* player = Engine::GetInstance().entityManager->GetPlayer();
	Vector2D playerPos = player->GetPosition();

	playerTileDist = sqrt(pos.distanceSquared(playerPos)) / 128;
	int iter = 0;

	while (pathfinding->pathTiles.empty() && playerTileDist < vision && iter < MaxIterations)
	{
		pathfinding->PropagateAStar();
		iter++;
	}
}

void ToxicBall::GetPhysicsValues() {
	// Read current velocity
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity = { 0, velocity.y };
}

void ToxicBall::Move() {
	Vector2D tilePos = GetTilePos();

	//Vertical
	if (hoverCooldown.ReadMSec() >= 250)
	{
		int posY = Engine::GetInstance().map->MapToWorld(tilePos.getX(), tilePos.getY()).getY();
	
		float baseY;

		if (pathfinding->IsWalkable(tilePos.getX(), tilePos.getY() + 1))
		{
			// Get world Y of the tile BELOW the demon
			baseY = posY + Engine::GetInstance().map->GetTileHeight(); // 1 tile down
		}
		else
		{
			// Get world Y of the tile OVER the demon
			baseY = posY - Engine::GetInstance().map->GetTileHeight(); // 1 tile above
		}
	
		// Floating offset
		float offset = sin(hoverTimer.ReadSec() * hoverSpeed) * hoverAmplitude;

		// Final target
		float targetY = baseY + offset;


		int x, y;
		pbody->GetPosition(x, y);

		// Smoothly move toward target 
		velocity.y += (targetY - y) * 0.1f;
		velocity.y = SDL_clamp(velocity.y, -3.0f, 3.0f);

		hoverCooldown.Start();
	}
	return;
}

void ToxicBall::Knockback()
{
	if (isdead) return;

	if (isKnockedback)
	{
		anims.SetCurrent("hurt"); // TO DO : Hurt effect
		if (lookingRight)
		{
			velocity.x = knockbackForce;
		}
		else
		{
			velocity.x = -knockbackForce;
		}
	}
	if (knockbackTime <= 0)
	{
		isKnockedback = false;
		knockbackTime = 500.0f;
	}
	else
	{
		knockbackTime -= Engine::GetInstance().GetDt();
	}
}

void ToxicBall::ApplyPhysics() {

	// Apply velocity via helper
	Engine::GetInstance().physics->SetLinearVelocity(pbody, { velocity.x, velocity.y });
}

void ToxicBall::Draw(float dt)
{
	if (Engine::GetInstance().sceneManager->isGamePaused == false)
	{
		anims.Update(dt);
	}
	const SDL_Rect& animFrame = anims.GetCurrentFrame();

	//SDLFlip
	SDL_FlipMode sdlFlip = SDL_FLIP_NONE;
	if (!lookingRight)
	{
		sdlFlip = SDL_FLIP_HORIZONTAL;
	}

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

	Uint8* r = new Uint8; Uint8* g = new Uint8; Uint8* b = new Uint8;
	Engine::GetInstance().render->SetColorMod(texture, r, g, b, 0, 0, 0);

	Engine::GetInstance().render->DrawRotatedTexture(texture, x, y - animFrame.h / 3, &animFrame, sdlFlip, 1);
		
	Engine::GetInstance().render->SetColorMod(texture, nullptr, nullptr, nullptr, *r, *g, *b);
	delete r; delete g; delete b;

}

//Define OnCollision function for the enemy. 
void ToxicBall::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) {
	switch (physB->ctype)
	{

	default:
		pbody->ctype = ColliderType::ENEMY;
		break;
	}
}

void ToxicBall::OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
	switch (physB->ctype)
	{
	default:
		break;
	}
}


