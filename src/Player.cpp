#include "Player.h"
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
#include "SavePoint.h"
#include <iostream>
#include <unordered_map>

using namespace std;

Player::Player() : Entity(EntityType::PLAYER)
{
	name = "Player";
	pbody = nullptr;
	texture = nullptr;
}

Player::~Player() 
{

}

bool Player::Awake() 
{

	// Initialize Player parameters
	position = Vector2D(96, 96);
	return true;
}

bool Player::Start() 
{
	// Initialize Player parameters

	// Load Textures
	if (!glideUnlocked)
	{
		std::unordered_map<int, std::string> aliases = { {0,"move_right"},
												 {11,"move_left"},
												 {22,"jump_right"},
												 {33,"fall_right" },
												 {44,"jump_left" } ,
												 {55,"fall_left"},
												 {66,"dash" }
		};
		anims.LoadFromTSX("Assets/Textures/Princess/Princess_Capeless.tsx", aliases);
		anims.SetCurrent("idle_right");

		texture = Engine::GetInstance().textures->Load("Assets/Textures/Princess/Princess_Capeless.png");
	}
	else
	{
		std::unordered_map<int, std::string> aliases = { {0,"move_right"},
												 {11,"move_left"},
												 {22,"jump_right"},
												 {33,"fall_right" },
												 {44,"jump_left" } ,
												 {55,"fall_left"},
												 {66,"idle_right" },
												 {77,"idle_left" } ,
												 {88,"death_right"},
												 {99,"death_left" }
		};
		anims.LoadFromTSX("Assets/Textures/Princess/Princess.tsx", aliases);
		
		anims.SetCurrent("idle_right");

		texture = Engine::GetInstance().textures->Load("Assets/Textures/Princess/Princess.png");
	}


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
	//pickCoinFxId = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/coin-collision-sound-342335.wav");

	knockbackForce = 5.0f;

	maxHealth = 100;
	currentHealth = maxHealth;


	cameraController.SetSmoothSpeed(0.15f);      // 0.05f - 0.3f
	cameraController.SetVerticalOffset(-25.0f);  // Offset vertixal 

	respawnPosition = position;

	return true;
}

bool Player::Update(float dt)
{
	if (pbody == nullptr) return true;

	if (Engine::GetInstance().sceneManager->isGamePaused == false && !isdead)
	{
		GetPhysicsValues();
		
		Move();

		Knockback();
		
		Jump(dt);

		timeSinceLastAttack += dt / 1000.0f; // Convert dt to seconds
		if (timeSinceLastAttack >= comboResetTime) {
			comboStep = 0; // Reset combo
		}

		Attack(dt);
	
		Glide();

		Dash();
	
		ApplyPhysics();
	}

	if (isdead && currentAnimPriority < 99)
	{
		currentAnimPriority = 99;

		if (lookingRight)
		{
			anims.GetAnim("death_right")->SetLoop(false);
			anims.SetCurrent("death_right");
		}
		else
		{
			anims.GetAnim("death_left")->SetLoop(false);
			anims.SetCurrent("death_left");
		}


		Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0, 0 });
		if (attackCollider != nullptr)
		{
			Engine::GetInstance().physics->DeletePhysBody(attackCollider);
			attackCollider = nullptr;
		}
		
	}

	CameraFollows();

	Draw(dt);

	DevTools(dt);

	// TO DO: Revisar esto a fondo
	if (onGround && onWall && !isJumping && !isdead)
	{
		safePositionTimer += dt / 1000.0f;
		if (safePositionTimer >= 0.2f)
		{
		
			Vector2D start = position;
			Vector2D end = { position.getX(), position.getY() + (texH / 2) + 5 }; 

			if (Engine::GetInstance().physics->Raycast(start, end))
			{
				lastSafePosition = position;
				safePositionTimer = 0.0f;
			}
		}
	}
	return true;
}

bool Player::PostUpdate()
{
	if (Engine::GetInstance().sceneManager->isGamePaused == false && !isdead)
	{
		Interact();
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
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT && isDashing == false) 
	{
		velocity.x = -speed;
		lookingRight = false;
		if (currentAnimPriority == 3)
		{
			anims.SetCurrent("fall_left");
		}
		else if (currentAnimPriority == 2)
		{
			anims.SetCurrent("jump_left");
		}
		else if (currentAnimPriority <= 1)
		{
			anims.SetCurrent("move_left");
			currentAnimPriority = 1;
		}
	}
	// Move Right
	else if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT && isDashing == false)
	{
		velocity.x = speed;
		lookingRight = true;
		if (currentAnimPriority == 3)
		{
			anims.SetCurrent("fall_right");
		}
		else if (currentAnimPriority == 2)
		{
			anims.SetCurrent("jump_right");
		}
		if (currentAnimPriority <= 1)
		{
			anims.SetCurrent("move_right");
			currentAnimPriority = 1;
		}
	}
	else
	{
		if (!isAttacking && !isJumping && !isDashing)
		{
			if (lookingRight)
			{
				anims.SetCurrent("idle_right");
			}
			else
			{
				anims.SetCurrent("idle_left");
			}
			
			currentAnimPriority = 0;
		}
	}
}

void Player::Knockback()
{
	if (isdead) return;

	if (isKnockedback)
	{
		anims.SetCurrent("hurt");
		if (!lookingRight) //TO DO: Improve this to apply the knockback in the same direction of the enemy attack, not just based on where the player is looking
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

void Player::Respawn() 
{
	if (isdead) {
		// Clean Attack 
		isAttacking = false;
		if (attackCollider != nullptr) {
			Engine::GetInstance().physics->DeletePhysBody(attackCollider);
			attackCollider = nullptr;
		}

		// Use RespawnPosition
		SetPosition(respawnPosition);
		currentHealth = maxHealth;

		isJumping = false;
		isdead = false;
		anims.SetCurrent("idle");
	}
}

void Player::RespawnFromVoid() { 
	//TO DO: Aplicar daño 

	Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0.0f, 0.0f });

	if (isAttacking && attackCollider != nullptr) {
		Engine::GetInstance().physics->DeletePhysBody(attackCollider);
		attackCollider = nullptr;
		isAttacking = false;
	}

	SetPosition(respawnPosition);

	isJumping = false;	
	secondJumpUsed = false;
	anims.SetCurrent("idle");

	LOG("Player reset to last safe position: %.2f, %.2f", respawnPosition.getX(), respawnPosition.getY());
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

			if (lookingRight == true)
			{
				anims.SetCurrent("jump_right");
			}
			else
			{
				anims.SetCurrent("jump_left");
			}
			currentAnimPriority = 2;

			//Extra Jump Force
			isJumpKeyDown = true;
			jumpHoldTime = 0.00f;

			LOG("Jump");
		}
		// Double Jump
		else if ( doubleJumpUnlocked && (isJumping == true || onAir == true) && secondJumpUsed == false)
		{
			secondJumpUsed = true;
			Engine::GetInstance().physics->SetYVelocity(pbody, jumpForce);
			if (lookingRight == true)
			{
				anims.SetCurrent("jump_right");
			}
			else
			{
				anims.SetCurrent("jump_left");
			}
			currentAnimPriority = 2;

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

void Player::Attack(float dt)
{
	// 1. Start the attack 
	if (!isAttacking && Engine::GetInstance().input->GetKey(SDL_SCANCODE_F) == KEY_DOWN)
	{
		isAttacking = true;
		anims.SetCurrent("attack");
		currentAnimPriority = 4;

		currentAttackTime = 0.0f;
		timeSinceLastAttack = 0.0f;

		if (comboStep == 0)
		{
			// first attack
			damage = 10;
			currentAttackWidth = 20;
			currentAttackHeight = 32;
			currentAttackOffsetX = 32;
			LOG("Attack 1 started (Normal)");
		}
		else
		{
			// second attack
			damage = 15;
			currentAttackWidth = 45;
			currentAttackHeight = 40;
			currentAttackOffsetX = 45;
			LOG("Attack 2 started (Heavy)");
		}

		// combo
		comboStep = (comboStep + 1) % 2;

		// calculate current attack
		int offsetX = lookingRight ? currentAttackOffsetX : -currentAttackOffsetX;
		int attackX = position.getX() + offsetX;
		int attackY = position.getY();

		// create collider attack
		attackCollider = Engine::GetInstance().physics->CreateRectangleSensor(attackX, attackY, currentAttackWidth, currentAttackHeight, bodyType::KINEMATIC);
		attackCollider->ctype = ColliderType::PLAYER_ATTACK;
		attackCollider->listener = this;

	}

	// 2. Control the duration of the attack
	if (isAttacking)
	{
		currentAttackTime += dt / 1000.0f;

		// Update the collider's position so that it follows the player whilst attacking
		if (attackCollider != nullptr) {
			int attackOffsetX = lookingRight ? 32 : -32;
			attackCollider->SetPosition(position.getX() + attackOffsetX, position.getY());
		}

		// End the attack when the time runs out
		if (currentAttackTime >= attackDuration)
		{
			isAttacking = false;
			anims.SetCurrent("idle");
			currentAnimPriority = 0;

			// Destroy the collider
			if (attackCollider != nullptr)
			{
				Engine::GetInstance().physics->DeletePhysBody(attackCollider);
				attackCollider = nullptr;
			}

			LOG("Attack ended");
		}
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
	// Start Dash
	if (dashUnlocked == true 
		&& Engine::GetInstance().input->GetKey(SDL_SCANCODE_LCTRL) == KEY_DOWN
		&& isDashing == false 
		&& dashCooldownTimer.ReadMSec() > dashCooldownMS)
	{
		if (lookingRight == true)
		{
			velocity.x = dashForce;
		}
		else
		{
			velocity.x = -dashForce;
		}

		isDashing = true;
		dashTimer.Start();
	}
	
	// While Dash
	if (isDashing)
	{
		if (lookingRight == true)
		{
			velocity.x = dashForce;
		}
		else
		{
			velocity.x = -dashForce;
		}

		if (dashTimer.ReadMSec() > dashDurationMS)
		{
			isDashing = false;
			dashCooldownTimer.Start();
		}
	}
}

void Player::Interact()
{
	if (canInteract)
	{
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_W) == KEY_DOWN)
		{
			Engine::GetInstance().sceneManager->setNewMap = true;
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

	if (velocity.y > 10 && currentAnimPriority != 3)
	{
		if (lookingRight)
		{
			anims.SetCurrent("fall_right");
		}
		else
		{
			anims.SetCurrent("fall_left");
		}
		currentAnimPriority = 3;
	}

	// Apply velocity via helper
	Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);
}

void Player::Draw(float dt) 
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
		//sdlFlip = SDL_FLIP_HORIZONTAL;
	}

	// Update render position using your PhysBody helper
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	// Draw the player using the texture and the current animation frame
	Engine::GetInstance().render->DrawRotatedTexture(texture, x, y-60, &animFrame, sdlFlip, 0.75f); // -20 0.25f

	if (isAttacking && attackCollider != nullptr)
	{
		int attackX, attackY;
		attackCollider->GetPosition(attackX, attackY);

		SDL_Rect attackRect = {
			attackX - (currentAttackWidth / 2),
			attackY - (currentAttackHeight / 2),
			currentAttackWidth,
			currentAttackHeight
		};

		// Draw diferent color combo
		if (currentAttackWidth == 20) {
			// fisrt attack red
			Engine::GetInstance().render->DrawRectangle(attackRect, 255, 0, 0, 150);
		}
		else {
			// second attack blue
			Engine::GetInstance().render->DrawRectangle(attackRect, 0, 150, 255, 150);
		}

	}
}

void Player::CameraFollows()
{
	Vector2D mapSize = Engine::GetInstance().map->GetMapSizeInPixels();
	int screenW = Engine::GetInstance().render->camera.w;
	int screenH = Engine::GetInstance().render->camera.h;

	cameraController.Update(0, position, screenW, screenH, mapSize.getX(), mapSize.getY());

	float camX, camY;
	cameraController.GetCameraPosition(camX, camY);
	Engine::GetInstance().render->camera.x = (int)camX;
	Engine::GetInstance().render->camera.y = (int)camY;
	
	float limitLeft = Engine::GetInstance().render->camera.w / 4;
	float limitRight = mapSize.getX() - Engine::GetInstance().render->camera.w * 3 / 4;
	
	if (position.getX() - limitLeft > 0 && position.getX() < limitRight) 
	{
		Engine::GetInstance().render->camera.x = -position.getX() + Engine::GetInstance().render->camera.w / 4;
	}
	// If player is at the far left, lock camera to the map's left edge to hide the outside area.
	else if (position.getX() <= limitLeft)
	{
		Engine::GetInstance().render->camera.x = 0;
	}
	// Player at far right: Lock camera to the right boundary.
	else if (position.getX() >= limitRight)
	{
		Engine::GetInstance().render->camera.x = -(mapSize.getX() - Engine::GetInstance().render->camera.w);
	}
}

void Player::UnlockCape()
{
	Engine::GetInstance().textures->UnLoad(texture);
	
	std::unordered_map<int, std::string> aliases = { {0,"move_right"},
											 {11,"move_left"},
											 {22,"jump_right"},
											 {33,"fall_right" },
											 {44,"jump_left" } ,
											 {55,"fall_left"},
											 {66,"idle_right" },
											 {77,"idle_left" } ,
											 {88,"death_right"},
											 {99,"death_left" }
	};
	anims.LoadFromTSX("Assets/Textures/Princess/princess.tsx", aliases);
	anims.SetCurrent("front");

	texture = Engine::GetInstance().textures->Load("Assets/Textures/Princess/princess.png");
	glideUnlocked = true;
}

bool Player::CleanUp()
{
	LOG("Cleanup player");
	if (pbody != nullptr) {
		pbody->listener = nullptr;
		Engine::GetInstance().physics->DeletePhysBody(pbody);
		pbody = nullptr;
	}
	Engine::GetInstance().textures->UnLoad(texture);
	if (attackCollider != nullptr) {
		Engine::GetInstance().physics->DeletePhysBody(attackCollider);
		attackCollider = nullptr;
	}
	return true;
}

// Define OnCollision function for the player. 
void Player::OnCollision(PhysBody* physA, PhysBody* physB) {
	if (physA == attackCollider) { return; }

	switch (physB->ctype)
	{

	case ColliderType::DANGER: // To Do: Mirar si queremos que sea solo cuando cae al vacio o cuando choca con pinchos
		LOG("Collision with DANGER zone!");
		if (!godMode && !isdead) {
			RespawnFromVoid();
		}
		break;

	case ColliderType::GROUND:
		LOG("Collision GROUND");
		// Reset the jump flag when touching the ground
		isJumping = false;
		secondJumpUsed = false;

		if (currentAnimPriority < 1)
		{
			anims.SetCurrent("idle");
			currentAnimPriority = 0;
		}

		onGround = true;
		break;

	case ColliderType::WALL:
		LOG("Collision WALL");
		// Reset the jump flag
		isJumping = false;
		secondJumpUsed = false;

		anims.SetCurrent("wall"); //TODO: On wall anim
		onWall = true;
		break;

	case ColliderType::DOOR:
		canInteract = true;
		interactuableBody = physB;
		break;

	case ColliderType::CEILING:
		LOG("Collision CEILING");
		break;

	case ColliderType::ITEM:
		LOG("Collision ITEM");
		//Engine::GetInstance().audio->PlayFx(pickCoinFxId);
		physB->listener->Destroy();
		break;
	case ColliderType::HEALTH_ORB:
		if (currentHealth < maxHealth)
		{
			currentHealth += 50;

			if (currentHealth > maxHealth)
			{
				currentHealth = maxHealth;
			}
			physB->listener->Destroy();
		}
		break;
	case ColliderType::SKILL_POINT_ORB:
		currentForceOrbs++;
		break;
	case ColliderType::SAVEPOINT:
	{
		LOG("Collision SavePoint");
		SavePoint* sp = (SavePoint*)physB->listener;
		sp->Activate();

		int spX, spY;
		physB->GetPosition(spX, spY);
		respawnPosition = Vector2D((float)spX, (float)spY);
		break;
	}  

	case ColliderType::ENEMY:
		TakeDamage(10); // Contact Damage
		isKnockedback = true;
		break;
	case ColliderType::ENEMY_ATTACK:
		TakeDamage(physB->listener->damage);
		isKnockedback = true;
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
	case ColliderType::DOOR:
		canInteract = false;
		interactuableBody = nullptr;
		break;
	case ColliderType::ITEM:
		LOG("End Collision ITEM");
		break;
	case ColliderType::UNKNOWN:
		LOG("End Collision UNKNOWN");
		break;
	case ColliderType::CEILING:
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

void Player::DevTools(float dt) 
{
	// Teleport the player to a specific position for testing purposes
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_T) == KEY_DOWN)
	{
		pbody->SetPosition(96, 96);
	}


	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_R) == KEY_DOWN) {
		pbody->SetPosition(respawnPosition.getX(), respawnPosition.getY());
		LOG("Player respawned at last save point!");


		Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0.0f,0.0f });
	}
	
	// GodMode (To Do: Make it work)
	if (Engine::GetInstance().input.get()->GetKey(SDL_SCANCODE_F10) == KEY_DOWN) {
		if (!godMode) {
			LOG("GodMode - Active");
			Engine::GetInstance().physics->SetBodyType(pbody, bodyType::KINEMATIC);
		}
		else {
			LOG("GodMode - Desactive");
			Engine::GetInstance().physics->SetBodyType(pbody, bodyType::DYNAMIC);
		}
		godMode = !godMode;
	}


	// Add Skill Point
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_P) == KEY_DOWN)
	{
		currentForceOrbs++;
		LOG("Skill Point Added. Current SkillPoints : %d", currentForceOrbs);
	}

	// Unlock Skills
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

	if (godMode)
	{
		GodModeMove(dt);
	}

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_9) == KEY_DOWN)
	{
		UnlockCape();
	}
}

void Player::GodModeMove(float dt)
{
	//Fly con el GodMode activo
	b2Vec2 godVelocity = { 0.0f, 0.0f };
	float godSpeed = speed * 2.0f;
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT) {
		godVelocity.x = -godSpeed;
	}
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT) {
		godVelocity.x = godSpeed;
	}
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT) {
		godVelocity.y = -godSpeed;
	}
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT) {
		godVelocity.y = godSpeed;
	}

	Engine::GetInstance().physics->SetLinearVelocity(pbody, godVelocity);
}

