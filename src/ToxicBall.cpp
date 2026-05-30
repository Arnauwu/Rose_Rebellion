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
	//TO DO: CHANGE SOUNDS

	// Initialize enemy parameters
	std::unordered_map<int, std::string> aliases = { {0,"Up"} };
	anims.LoadFromTSX("Assets/Textures/Entities/Enemies/ToxicBall/ToxicBall.tsx", aliases);
	anims.SetCurrent("Up");

	texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/ToxicBall/ToxicBall.png");

	//Load Audio
	chocarToxicBall = Engine::GetInstance().audio->LoadFx("");

	//Add physics to the enemy - initialize physics body
	texW = 96;
	texH = 96;
	pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX() + texW / 2, (int)position.getY() + texH / 2, (texW * 2) / 5, bodyType::KINEMATIC);

	Engine::GetInstance().physics->SetGravityScale(pbody, 0.0f);

	pbody->listener = this;

	//ssign collider type
	pbody->ctype = ColliderType::ENEMY;

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);
	initialX = position.getX();
	initialY = position.getY();

	float tileSize = 128.0f;

	float heightInPixels = jumpDistanceTiles * tileSize;

	float heightInMeters = PIXEL_TO_METERS(heightInPixels);

	jumpForce = -sqrtf(2.0f * ballGravity * heightInMeters);

	return true;
}

bool ToxicBall::Update(float dt)
{
	if (!active) return true;

	if (Engine::GetInstance().sceneManager->isGamePaused == false && isdead == false)
	{
		GetPhysicsValues();
		Move();
		ApplyPhysics();
	}

	Draw(dt);

	return true;
}

void ToxicBall::GetPhysicsValues() {
	// Read current velocity
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
}

void ToxicBall::Move() {

	velocity.x = 0; // Don't move in X

	if (position.getY() >= initialY && velocity.y >= 0) {

		velocity.y = 0;
		jumpTimer += 16.0f;

		if (jumpTimer >= waitTime) {
			velocity.y = jumpForce;
			jumpTimer = 0.0f;
		}
	}
	else {
		velocity.y += ballGravity * 0.016f;
	}
}

void ToxicBall::Knockback() {

}

void ToxicBall::ApplyPhysics() {

	// Apply velocity via helper
	Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0, velocity.y });
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

	Engine::GetInstance().render->DrawRotatedTexture(texture, x, y - animFrame.h / 3, &animFrame, sdlFlip, 1);
}

bool ToxicBall::CleanUp()
{
	LOG("Cleanup ToxicBall");
	active = false;
	Engine::GetInstance().textures->UnLoad(texture);
	Engine::GetInstance().physics->DeletePhysBody(pbody);
	return true;
}

//Define OnCollision function for the enemy. 
void ToxicBall::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) {
	switch (physB->ctype)
	{

	case ColliderType::PLAYER_ATTACK:

		TakeDamage(physB->listener->damage);
		break;

	default:
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


