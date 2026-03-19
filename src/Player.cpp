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
#include <iostream>
#include <unordered_map>

using namespace std;

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
	Glide();

	Dash();

	Teleport();
	
	ApplyPhysics();


	Draw(dt);

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_P) == KEY_DOWN)
	{
		currentForceOrbs++;
		LOG("Skill Point Added. Current SkillPoints : %d", currentForceOrbs);
	}

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_1) == KEY_DOWN)
	{
		if (currentForceOrbs > 0) 
		{
			if (OffensiveSkills[2] == false)
			{
				LOG("Unlocking Offensive Skill:");
				if (OffensiveSkills[1] == true) { OffensiveSkills[2] = true; LOG("Offensive Skill 3 Unlocked"); }
				else if (OffensiveSkills[0] == true) { OffensiveSkills[1] = true; LOG("Offensive Skill 2 Unlocked"); }
				else { OffensiveSkills[0] = true; LOG("Offensive Skill 1 Unlocked"); }
				currentForceOrbs--;
			}
			else { LOG("Offensive Tree Maxed"); }
		}
		else { LOG("Not Enough Skill Points"); }
	}

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_2) == KEY_DOWN)
	{
		if (currentForceOrbs > 0)
		{
			if (DefensiveSkills[2] == false)
			{
				LOG("Unlocking Defensive Skill:");
				if (DefensiveSkills[1] == true) { DefensiveSkills[2] = true; LOG("Defensive Skill 3 Unlocked"); }
				else if (DefensiveSkills[0] == true) { DefensiveSkills[1] = true; LOG("Defensive Skill 2 Unlocked"); }
				else { DefensiveSkills[0] = true; LOG("Defensive Skill 1 Unlocked"); }
				currentForceOrbs--;
			}
			else { LOG("Defensive Tree Maxed"); }
		}
		else { LOG("Not Enough Skill Points"); }
	}

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_3) == KEY_DOWN)
	{
		if (currentForceOrbs > 0)
		{
			if (UtilitySkills[2] == false)
			{
				LOG("Unlocking Utility Skill:");
				if (UtilitySkills[1] == true) { UtilitySkills[2] = true; LOG("Utility Skill 3 Unlocked"); }
				else if (UtilitySkills[0] == true) { UtilitySkills[1] = true; LOG("Utility Skill 2 Unlocked"); }
				else { UtilitySkills[0] = true; LOG("Utility  Skill 1 Unlocked"); }
				currentForceOrbs--;
			}
			else { LOG("Utility Tree Maxed"); }
		}
		else { LOG("Not Enough Skill Points"); }
	}

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
		if (isJumping == false && onGround == true)
		{
			isJumping = true;
			Engine::GetInstance().physics->SetYVelocity(pbody, jumpForce);

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
			Engine::GetInstance().physics->SetYVelocity(pbody, jumpForce);
			anims.SetCurrent("jump");

			//Extra Jump Force
			isJumpKeyDown = true;
			jumpHoldTime = 0.00f;

			LOG("Double Jump");
		}
	}
	else if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_REPEAT && isJumping && isJumpKeyDown && jumpHoldTime <= maxJumpHoldTime)
	{
		Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, 0.0f, jumpForce * 0.005f, true); //TO DO: Adjust Value
		jumpHoldTime += dt/1000; //To seconds
	}
	else if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_UP)
	{
		isJumpKeyDown = false;
	}
}

void Player::Glide() // Gliding
{
	if (glideUnlocked)
	{
		if (onAir == true && onGround == false && Engine::GetInstance().input->GetKey(SDL_SCANCODE_LSHIFT) == KEY_REPEAT)
		{
			isGliding = true;
		}
		else if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_LSHIFT) == KEY_UP && isGliding || onGround)
		{
			isGliding = false;
		}
	}
}

void Player::Dash()
{
	if (dashUnlocked == true)
	{
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_LCTRL) == KEY_DOWN && hasDashed == false)
		{
			if (lookingRight == true)
			{
				velocity.x += dashForce;
			}
			else
			{
				velocity.x -= dashForce;
			}
		}
	}
}

void Player::ApplyPhysics() {
	// Preserve vertical speed while jumping
	if (isJumping == true || secondJumpUsed == true) {
		velocity.y = Engine::GetInstance().physics->GetYVelocity(pbody);
	}


	if (isGliding)
	{
		int maxFallSpeed = 1;
		if (velocity.y >= maxFallSpeed)
		{
			LOG("Gliding");
			velocity.y = maxFallSpeed;
		}
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

Vector2D Player::GetPosition()
{
	int x, y;
	pbody->GetPosition(x, y);
	// Adjust for center
	return Vector2D((float)x, (float)y);
}

void Player::SetPosition(Vector2D pos)
{
	pbody->SetPosition((int)(pos.getX() + texW / 2), (int)(pos.getY() + texH / 2));
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