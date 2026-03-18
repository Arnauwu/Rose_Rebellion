#include "Player.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "Scene.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"
#include "Map.h"

Player::Player() : Entity(EntityType::PLAYER)
{
	name = "Player";
}

Player::~Player() {

}

bool Player::Awake() {

	// Initialize Player parameters
	position = Vector2D(96, 96);
	return true;
}

bool Player::Start() {

	// Load
	std::unordered_map<int, std::string> aliases = { {0,"idle"},{11,"move"},{22,"jump"} };
	anims.LoadFromTSX("Assets/Textures/PLayer2_Spritesheet.tsx", aliases);
	anims.SetCurrent("idle");

	// Initialize Player parameters
	texture = Engine::GetInstance().textures->Load("Assets/Textures/player2_spritesheet.png");

	// Physics
	//Engine::GetInstance().textures->GetSize(texture, texW, texH);
	texW = 32;
	texH = 32;
	pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX(), (int)position.getY(), texW / 2, bodyType::DYNAMIC);

	// Assign listener of the pbody. This makes the Physics module to call the OnCollision method
	pbody->listener = this;

	// Assign collider type
	pbody->ctype = ColliderType::PLAYER;

	// Initialize audio
	pickCoinFxId = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/coin-collision-sound-342335.wav");

	return true;
}

bool Player::Update(float dt)
{
	GetPhysicsValues();
	Move();
	CameraFollows();

	Jump(dt);
	
	Teleport();
	
	ApplyPhysics();


	Draw(dt);

	return true;
}

void Player::GetPhysicsValues() 
{
	// Read current velocity
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity = { 0, velocity.y }; // Reset horizontal velocity by default, this way the player stops when no key is pressed
}

void Player::Move() {
	
	// Move Left
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT) 
	{
		velocity.x = -speed;
		lookingRight = false;
		anims.SetCurrent("move");
	}
	// Move Right
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT) 
	{
		velocity.x = speed;
		lookingRight = true;
		anims.SetCurrent("move");
	}
}

void Player::Jump(float dt) //TO DO: If you try to second Jump on air while falling without the first jump it being called but not working
{
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN)
	{
		// Base Jump 
		if (onGround == true)
		{
			isJumping = true;
			//Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, 0.0f, -jumpForce, true);
			Engine::GetInstance().physics->SetYVelocity(pbody, -jumpForce * 3); // TO DO: Adjust Second Jump Force

			anims.SetCurrent("jump");

			//Extra Jump Force
			isJumpKeyDown = true;
			jumpHoldTime = 0.00f;

			LOG("Jump");
		}
		// Double Jump
		else if ((isJumping == true || onAir == true) && secondJumpUsed == false)
		{
			secondJumpUsed = true;
			//Engine::GetInstance().physics->SetYVelocity(pbody, 0); // Stop MidAir
			Engine::GetInstance().physics->SetYVelocity(pbody, -jumpForce*3); // TO DO: Adjust Second Jump Force
			anims.SetCurrent("jump");

			//Extra Jump Force
			isJumpKeyDown = true;
			jumpHoldTime = 0.00f;

			LOG("Double Jump");
		}
	}
	else if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_REPEAT && isJumping && isJumpKeyDown && jumpHoldTime <= maxJumpHoldTime)
	{
		Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, 0.0f, -jumpForce * 0.005f, true); //TO DO: Adjust Value
		jumpHoldTime += dt/1000; //To seconds
	}
	else if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_UP)
	{
		isJumpKeyDown = false;
	}
}

void Player::ApplyPhysics() {
	// Preserve vertical speed while jumping
	if (isJumping == true || secondJumpUsed == true) {
		velocity.y = Engine::GetInstance().physics->GetYVelocity(pbody);
	}

	// Apply velocity via helper
	Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);
}

void Player::Draw(float dt) {

	anims.Update(dt);
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

	// Draw the player using the texture and the current animation frame
	Engine::GetInstance().render->DrawRotatedTexture(texture, x, y, &animFrame, 0, 0, 0, sdlFlip);
}

void Player::CameraFollows()
{
	// Center the camera on the player
	Vector2D mapSize = Engine::GetInstance().map->GetMapSizeInPixels();
	
	float limitLeft = Engine::GetInstance().render->camera.w / 4;
	float limitRight = mapSize.getX() - Engine::GetInstance().render->camera.w * 3 / 4;
	
	if (position.getX() - limitLeft > 0 && position.getX() < limitRight) 
	{
		Engine::GetInstance().render->camera.x = -position.getX() + Engine::GetInstance().render->camera.w / 4;
	}
}

bool Player::CleanUp()
{
	LOG("Cleanup player");
	Engine::GetInstance().textures->UnLoad(texture);
	return true;
}

// L08 TODO 6: Define OnCollision function for the player. 
void Player::OnCollision(PhysBody* physA, PhysBody* physB) {
	switch (physB->ctype)
	{
	case ColliderType::GROUND:
		LOG("Collision GROUND");
		// Reset the jump flag when touching the ground
		isJumping = false;
		secondJumpUsed = false;

		anims.SetCurrent("idle");	
		onGround = true;

		break;
	case ColliderType::WALL:
		LOG("Collision WALL");
		// Reset the jump flag
		isJumping = false;
		secondJumpUsed = false;

		anims.SetCurrent("idle"); //TODO: On wall anim
		onWall = true;

		break;
	case ColliderType::CEILING:
		LOG("Collision CEILING");
		break;
	case ColliderType::ITEM:
		LOG("Collision ITEM");
		Engine::GetInstance().audio->PlayFx(pickCoinFxId);
		physB->listener->Destroy();

		break;
	case ColliderType::UNKNOWN:
		LOG("Collision UNKNOWN");
		break;
	default:
		break;
	}
}

void Player::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
	switch (physB->ctype)
	{
	case ColliderType::WALL:
		onAir = true;
		onWall = false;
		break;
	case ColliderType::GROUND:
		onGround = false;
		onAir = true;
		LOG("On Air");

		break;
	case ColliderType::ITEM:
		LOG("End Collision ITEM");
		break;
	case ColliderType::UNKNOWN:
		LOG("End Collision UNKNOWN");
		break;
	default:
		break;
	}
}

// DevTools

void Player::Teleport() 
{
	// Teleport the player to a specific position for testing purposes
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_T) == KEY_DOWN)
	{
		pbody->SetPosition(96, 96);
	}
}