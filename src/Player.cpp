#include "Player.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "SceneManager.h"
#include "GameManager.h"
#include "ParticleManager.h"
#include "dialogueManager.h"
#include "Npc.h"

#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"
#include "Map.h"
#include "SavePoint.h"
#include "Door.h"
#include <iostream>
#include <unordered_map>

#include "tracy/Tracy.hpp"

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
	Engine::GetInstance().entityManager->SetPlayer(this);

	currentHealth = GameManager::GetInstance().gameState.currentHealth;
	LOG("Player Awake: Posición final establecida en %f, %f", position.getX(), position.getY());
	return true;
}

bool Player::Start()
{
	// Initialize Player parameters
	auto engine = &Engine::GetInstance();
	auto audio = engine->audio;
	auto textures = engine->textures;
	auto physics = engine->physics;

	Engine::GetInstance().entityManager->SetPlayer(this);

	jumpFx = audio->LoadFx("Assets/Audio/Fx/SE_Princesa_Jump.wav");
	attackFx = audio->LoadFx("Assets/Audio/Fx/SE_Princesa_Ataque.wav");
	dashPrincesa = audio->LoadFx("Assets/Audio/Fx/SE_Princesa_dash.wav");
	morirPrincesa = audio->LoadFx("Assets/Audio/Fx/SE_Princesa_Muerte.wav");
	planearPrincesa = audio->LoadFx("Assets/Audio/Fx/SE_Planear.wav");
	recibirDamage = audio->LoadFx("Assets/Audio/Fx/SE_Princesa_getDamage.wav");
	caminarPrincesa = audio->LoadFx("Assets/Audio/Fx/SE_Princesa_Caminar_Piedra.wav");

	pickItemFx = audio->LoadFx("Assets/Audio/Fx/SE_Llave_Item.wav");
	savePointFx = audio->LoadFx("Assets/Audio/Fx/Respawn.wav");
	openDoor = audio->LoadFx("Assets/Audio/Fx/OpenDoor.wav");
	closedDoor = audio->LoadFx("Assets/Audio/Fx/DoorClosed.wav");
	orbFx = audio->LoadFx("Assets/Audio/Fx/SE_OrbeFuerza_Item.wav");
	respawnFx = audio->LoadFx("Assets/Audio/Fx/Respawn.wav");

	// Carga de Texturas según estado
	if (!GameManager::GetInstance().gameState.glideUnlocked)
	{
		std::unordered_map<int, std::string> aliases = GetAliases("capeless");
		anims.LoadFromTSX("Assets/Textures/Entities/Princess/Princess_Capeless.tsx", aliases);
		anims.SetCurrent("idle_right");
		texture = textures->Load("Assets/Textures/Entities/Princess/Princess_Capeless.png");
	}
	else
	{
		std::unordered_map<int, std::string> aliases = GetAliases("cape");
		anims.LoadFromTSX("Assets/Textures/Entities/Princess/Princess.tsx", aliases);
		anims.SetCurrent("idle_right");
		texture = textures->Load("Assets/Textures/Entities/Princess/Princess.png");
	}

	// Configuración de Físicas
	texW = 128;
	texH = 128;
	pbody = Engine::GetInstance().physics->CreateCapsule((int)position.getX(), (int)position.getY(), texW / 3, texW * 17 / 24, texH, bodyType::DYNAMIC);

	// Assign listener of the pbody. This makes the Physics module to call the OnCollision method
	pbody->listener = this;
	pbody->ctype = ColliderType::PLAYER;

	knockbackForce = 7.0f;

	maxHealth = GameManager::GetInstance().gameState.maxHealth;
	currentHealth = GameManager::GetInstance().gameState.currentHealth;

	if (GameManager::GetInstance().gameState.stSpeedUp) {
		this->speed = 12.0f;
	}
	else {
		this->speed = 10.0f;
	}

	if (GameManager::GetInstance().gameState.stFastDash) {
		this->dashForce = 40.0f;
	}
	else {
		this->dashForce = 30.0f;
	}

	cameraController.SetSmoothSpeed(0.15f);
	cameraController.SetVerticalOffset(-25.0f);
	respawnPosition = position;
	lastSafePosition = position;

	if (GameManager::GetInstance().gameState.stIframesUp) {
		this->invincibilityDurationMS = 2000.0f; 
	}
	else {
		this->invincibilityDurationMS = 1000.0f; 
	}

	//Timers
	safePositionTimer.Start();
	attackCooldownTimer.Start();

	return true;
}

bool Player::Update(float dt)
{
	ZoneScoped;
	
	if (isFrozen) 
	{
		GetPhysicsValues();
		velocity = { 0, velocity.y }; 
		ApplyPhysics();

		if (lookingRight)
		{
			anims.SetCurrent("idle_right");
		}
		else
		{
			anims.SetCurrent("idle_left");
		}
		Draw(dt);
		return true;
	}

	if (pbody == nullptr) return true;
	Engine::GetInstance().entityManager->SetPlayer(this);

	bool isDialogueActive = Engine::GetInstance().dialogueManager->IsDialogueActive();

	if (isDialogueActive) 
	{
		if (!Engine::GetInstance().sceneManager->isGamePaused) {
			velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
			velocity.x = 0.0f; 
			ApplyPhysics();   
			Draw(dt);        
		}
		else 
		{
			// DORMIR
			if (lookingRight) anims.SetCurrent("idle_right");
			else anims.SetCurrent("idle_left");
			Draw(dt);
		}
		return true;
	}

	if (Engine::GetInstance().sceneManager->isGamePaused == false && !isdead)
	{
		GetPhysicsValues();

		if (isInvincible)
		{
		
			if (invincibilityTimer.ReadMSec() >= invincibilityDurationMS)
			{
				isInvincible = false;
				isVisible = true; 
			}
			else
			{
				int timePassed = invincibilityTimer.ReadMSec();
				isVisible = (timePassed / (int)flashIntervalMS) % 2 == 0;
			}
		}

		if (isWallJumping) {
			wallJumpTimer -= dt / 1000.0f;
			if (wallJumpTimer <= 0.0f) {
				isWallJumping = false;
			}
		}

		Move();

		Knockback();

		timeSinceLastAttack += dt / 1000.0f; // Convert dt to seconds
		if (timeSinceLastAttack >= comboResetTime) {
			comboStep = 0; // Reset combo
		}
		if (!isKnockedback) {
			Move();

			Jump(dt);

			Attack(dt);

			Glide();

			Dash();
		}

		ApplyPhysics();

		if (onGround && !onWall && !onAir && !isdead) //Save LastSafePosition
		{
			if (safePositionTimer.ReadMSec() >= safePositionInterval)
			{

				Vector2D start = position;
				Vector2D end = { position.getX(), position.getY() + (texH / 2) + 5 };

				if (Engine::GetInstance().physics->Raycast(start, end))
				{
					//LOG("lastSafePosition saved");
					lastSafePosition = position;
					safePositionTimer.Start();
				}
			}
		}
	}

	if (isdead)
	{
		isVisible = true; 
		isInvincible = false;

		if (currentAnimPriority < 99)
		{
			currentAnimPriority = 99;
			Engine::GetInstance().audio->PlayFx(morirPrincesa);
			isKnockedback = false;

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
		else
		{
			if ((lookingRight && anims.GetAnim("death_right")->HasFinishedOnce()) ||
				(!lookingRight && anims.GetAnim("death_left")->HasFinishedOnce()))
			{
				Engine::GetInstance().sceneManager->ChangeScene(SceneID::GAMEOVER);
			}
		}
	}

	CameraFollows();

	Draw(dt);

	DevTools(dt);

	return true;
}

bool Player::PostUpdate()
{
	if (Engine::GetInstance().sceneManager->isGamePaused == false && !isdead && !isFrozen)
	{
		Interact();
	}
	return true;
}

void Player::GetPhysicsValues()
{
	// Read current velocity
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	//velocity = { 0, velocity.y }; // Reset horizontal velocity by default, this way the player stops when no key is pressed
	if (!isWallJumping) {
		velocity = { 0, velocity.y };
	}
}

void Player::Move() {
	bool isMovingThisFrame = false;
	float moveInput = 0.0f;

	// Keyboard input
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT && isDashing == false)
	{
		moveInput = -1.0f;
	}
	else if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT && isDashing == false)
	{
		moveInput = 1.0f;
	}

	if (Engine::GetInstance().input->IsGamepadConnected())
	{
		float gamepadInput = Engine::GetInstance().input->GetGamepadAxis(GAMEPAD_AXIS_LSTICK_X);
		if (fabs(gamepadInput) > 0.1f)
		{
			moveInput = gamepadInput;
		}
	}

	if (moveInput != 0.0f && isDashing == false)
	{
		velocity.x = moveInput * speed;
		lookingRight = (moveInput > 0.0f);
		isMovingThisFrame = true;

		if (currentAnimPriority == 3)
		{
			anims.SetCurrent(lookingRight ? "fall_right" : "fall_left");
		}
		else if (currentAnimPriority == 2)
		{
			anims.SetCurrent(lookingRight ? "jump_right" : "jump_left");
		}
		else if (currentAnimPriority <= 1)
		{
			anims.SetCurrent(lookingRight ? "move_right" : "move_left");
			currentAnimPriority = 1;
		}
	}
	else
	{
		if (!isAttacking && !isJumping && !isDashing && currentAnimPriority != 4)
		{
			anims.SetCurrent(lookingRight ? "idle_right" : "idle_left");
			currentAnimPriority = 0;
		}
	}

	bool isWalkingConditions = (isMovingThisFrame && onGround && !isDashing && !isAttacking && !isdead);

	if (isWalkingConditions)
	{
		if (!wasWalking) {
			stepTimer = timeBetweenSteps;
		}

		stepTimer += Engine::GetInstance().GetDt() / 1000.0f;

		if (stepTimer >= timeBetweenSteps)
		{
			Engine::GetInstance().audio->PlayFx(caminarPrincesa, 0);

			int pX, pY;
			pbody->GetPosition(pX, pY);
			float footY = (float)pY + (texH / 2.0f) - 90.0f;
			Engine::GetInstance().particleManager->EmitDust((float)pX, footY, lookingRight);

			stepTimer = 0.0f;
		}
	}
	else
	{
		if (wasWalking)
		{
			Engine::GetInstance().audio->StopFx(caminarPrincesa);
		}
	}

	wasWalking = isWalkingConditions;
}

void Player::Knockback()
{
	if (isdead) return;

	if (isKnockedback)
	{
		isAttacking = false;
		//anims.SetCurrent("hurt"); //TO DO: Add the animation for taking damage
		if (attackCollider != nullptr) {
			Engine::GetInstance().physics->DeletePhysBody(attackCollider);
			attackCollider = nullptr;
		}

		if (hitFromRight) {
			velocity.x = -knockbackForce;
		}
		else
		{
			velocity.x = knockbackForce;
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
		currentHealth = maxHealth;
		Engine::GetInstance().audio->PlayFx(respawnFx);
		isJumping = false;
		isdead = false;

		anims.SetCurrent("idle");
		currentAnimPriority = 0;
	}
}

void Player::RespawnFromVoid()
{
	Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0.0f, 0.0f });

	if (isAttacking && attackCollider != nullptr) {
		Engine::GetInstance().physics->DeletePhysBody(attackCollider);
		attackCollider = nullptr;
		isAttacking = false;
	}

	SetPosition(lastSafePosition);

	isJumping = false;
	secondJumpUsed = false;
	anims.SetCurrent("idle");
	currentAnimPriority = 0;
	Engine::GetInstance().audio->PlayFx(respawnFx);
	LOG("Player reset to last safe position: %.2f, %.2f", lastSafePosition.getX(), lastSafePosition.getY());
}

void Player::Jump(float dt)
{
	bool jumpPressed = false;

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN)
	{
		jumpPressed = true;
	}

	if (Engine::GetInstance().input->IsGamepadConnected() && 
		Engine::GetInstance().input->GetGamepadButton(GAMEPAD_A) == KEY_DOWN)
	{
		jumpPressed = true;
	}

	if (jumpPressed)
	{
		// WALL JUMP
		bool wallJumpInput = false;
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_L) == KEY_REPEAT)
			wallJumpInput = true;
		if (Engine::GetInstance().input->IsGamepadConnected() &&
			Engine::GetInstance().input->GetGamepadButton(GAMEPAD_LB) == KEY_REPEAT)
			wallJumpInput = true;

		if (onWall == true && onGround == false && wallJumpInput && GameManager::GetInstance().gameState.wallJumpUnlocked)
		{
			Engine::GetInstance().audio->PlayFx(jumpFx);
			isJumping = true;
			onWall = false;
			onAir = true;

			isWallJumping = true;
			wallJumpTimer = 0.30f;

			float wJumpForceY = jumpForce * 1.0f;
			float wJumpForceX = speed * 1.0f;

			if (wallDirection == 1) {
				velocity.x = -wJumpForceX;
				lookingRight = false;
				anims.SetCurrent("wall_jump_left");
				anims.GetAnim("wall_jump_left")->SetLoop(false);

			}
			else {
				velocity.x = wJumpForceX;
				lookingRight = true;
				anims.SetCurrent("wall_jump_right");
				anims.GetAnim("wall_jump_right")->SetLoop(false);

			}			
			currentAnimPriority = 2;


			Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);
			Engine::GetInstance().physics->SetYVelocity(pbody, wJumpForceY);

			isJumpKeyDown = true;
			jumpHoldTime = 0.00f;

			LOG("Wall Jump");
		}
		// BASE JUMP
		else if (isJumping == false && onGround == true)
		{
			Engine::GetInstance().audio->PlayFx(jumpFx);
			isJumping = true;
			Engine::GetInstance().physics->SetYVelocity(pbody, jumpForce);

			int pX, pY;
			pbody->GetPosition(pX, pY);
			float footY = (float)pY + (texH / 2.0f) - 80.0f;
			Engine::GetInstance().particleManager->EmitJumpDust((float)pX, footY, lookingRight);

			if (currentAnimPriority <= 2)
			{
				anims.SetCurrent(lookingRight ? "jump_right" : "jump_left");
				currentAnimPriority = 2;
			}

			isJumpKeyDown = true;
			jumpHoldTime = 0.00f;

			LOG("Jump");
		}
		// DOUBLE JUMP
		else if (GameManager::GetInstance().gameState.doubleJumpUnlocked && 
			(isJumping == true || onAir == true) && secondJumpUsed == false)
		{
			Engine::GetInstance().audio->PlayFx(jumpFx);
			secondJumpUsed = true;
			Engine::GetInstance().physics->SetYVelocity(pbody, jumpForce);

			if (currentAnimPriority <= 2)
			{
				anims.SetCurrent(lookingRight ? "jump_right" : "jump_left");
				currentAnimPriority = 2;
			}


			isJumpKeyDown = true;
			jumpHoldTime = 0.00f;

			LOG("Double Jump");
		}
	}
	// HOLD JUMP FOR EXTRA FORCE
	else if ((Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_REPEAT || 
		(Engine::GetInstance().input->IsGamepadConnected() && 
			Engine::GetInstance().input->GetGamepadButton(GAMEPAD_A) == KEY_REPEAT)) && 
		isJumping && isJumpKeyDown && jumpHoldTime <= maxJumpHoldTime)
	{
		Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, 0.0f, jumpForce * 0.005f, true);
		jumpHoldTime += dt / 1000.0f;
	}
	// RELEASE JUMP
	else if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_UP ||
		(Engine::GetInstance().input->IsGamepadConnected() && 
			Engine::GetInstance().input->GetGamepadButton(GAMEPAD_A) == KEY_UP))
	{
		isJumpKeyDown = false;
	}
}

void Player::Attack(float dt)
{
	bool attackPressed = false;

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_J) == KEY_DOWN)
	{
		attackPressed = true;
	}

	if (Engine::GetInstance().input->IsGamepadConnected() &&
		Engine::GetInstance().input->GetGamepadButton(GAMEPAD_X) == KEY_DOWN)
	{
		attackPressed = true;
	}

	// 1. Iniciar el ataque
	if (attackPressed && !isGliding && !isAttacking && attackCooldownTimer.ReadMSec() >= attackCooldownMS && currentAnimPriority <= 4)
	{
		if (HasItem(ItemID::WEAPON))
		{
			Engine::GetInstance().audio->PlayFx(attackFx);
			isAttacking = true;

			bool lookUp = false;
			bool lookDown = false;

			if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT)
				lookUp = true;
			if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT && !onGround)
				lookDown = true;

			if (Engine::GetInstance().input->IsGamepadConnected())
			{
				float rstickY = Engine::GetInstance().input->GetGamepadAxis(GAMEPAD_AXIS_LSTICK_Y);
				if (rstickY < -0.5f)
					lookUp = true;
				else if (rstickY > 0.5f && !onGround)
					lookDown = true;
			}

			currentAttackTime = 0.0f;
			timeSinceLastAttack = 0.0f;

			if (comboStep == 0)
			{
				damage = 10 + dmgbuff;
				currentAttackWidth = 60;
				currentAttackHeight = 64;
			}
			else
			{
				damage = 20 + dmgbuff;
				currentAttackWidth = 120;
				currentAttackHeight = 90;
			}

			if (lookUp && GameManager::GetInstance().gameState.stUpAttack) {
				int temp = currentAttackWidth;
				currentAttackWidth = currentAttackHeight;
				currentAttackHeight = temp;

				currentAttackOffsetY = -(texH * 0.8f) - currentAttackHeight;
				currentAttackOffsetX = 0;

				anims.GetAnim(lookingRight ? "attack_up_right" : "attack_up_left")->SetLoop(false);
				anims.SetCurrent(lookingRight ? "attack_up_right" : "attack_up_left");
			}
			else if (lookDown && GameManager::GetInstance().gameState.stDownAttack) {
				int temp = currentAttackWidth;
				currentAttackWidth = currentAttackHeight;
				currentAttackHeight = temp;

				currentAttackOffsetY = (texH * 0.8f);
				currentAttackOffsetX = 0;

				anims.GetAnim(lookingRight ? "attack_down_right" : "attack_down_left")->SetLoop(false);
				anims.SetCurrent(lookingRight ? "attack_down_right" : "attack_down_left");
			}
			else
			{
				currentAttackOffsetX = lookingRight ? (texW / 2 + currentAttackWidth / 2) : -(texW / 2 + currentAttackWidth / 2);
				currentAttackOffsetY = 0;

				anims.GetAnim(lookingRight ? "attack_right" : "attack_left")->SetLoop(false);
				anims.SetCurrent(lookingRight ? "attack_right" : "attack_left");
			}

			currentAnimPriority = 4;
			comboStep = (comboStep + 1) % 2;

			attackCollider = Engine::GetInstance().physics->CreateRectangleSensor(
				position.getX() + currentAttackOffsetX,
				position.getY() + currentAttackOffsetY,
				currentAttackWidth,
				currentAttackHeight,
				bodyType::KINEMATIC);

			attackCollider->ctype = ColliderType::PLAYER_ATTACK;
			attackCollider->listener = this;
			
		}
	}

	if (isAttacking)
	{
		currentAttackTime += dt / 1000.0f;

		if (attackCollider != nullptr) {
			attackCollider->SetPosition(position.getX() + currentAttackOffsetX, position.getY() + currentAttackOffsetY);
		}

		if (currentAttackTime >= attackDuration)
		{
			if (
				(anims.GetAnim(anims.GetCurrentName())->HasFinishedOnce() || 
					anims.GetCurrentName().find("attack_") == std::string::npos)
				)
			{


				isAttacking = false;
				currentAnimPriority = 0;

				if (attackCollider != nullptr)
				{
					Engine::GetInstance().physics->DeletePhysBody(attackCollider);
					attackCollider = nullptr;
				}
				attackCooldownTimer.Start();
			}
		}
	}
}

void Player::Glide()
{
	if (GameManager::GetInstance().gameState.glideUnlocked)
	{
		bool glidePressed = false;

		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_LSHIFT) == KEY_REPEAT)
			glidePressed = true;

		if (Engine::GetInstance().input->IsGamepadConnected() &&
			Engine::GetInstance().input->GetGamepadAxis(GAMEPAD_AXIS_LT) > 0.5f)
			glidePressed = true;

		if (onAir == true && onGround == false && glidePressed)
		{
			if (!isGliding) {
				Engine::GetInstance().audio->PlayFx(planearPrincesa);
			}
			isGliding = true;
			if (currentAnimPriority <= 5)
			{
				anims.SetCurrent(lookingRight ? "glide_right" : "glide_left");
				currentAnimPriority = 5;
			}
		}
		else if ((Engine::GetInstance().input->GetKey(SDL_SCANCODE_LSHIFT) == KEY_UP ||
			(Engine::GetInstance().input->IsGamepadConnected() &&
				Engine::GetInstance().input->GetGamepadAxis(GAMEPAD_AXIS_LT) <= 0.5f)) && 
			(isGliding || onGround))
		{
			isGliding = false;
		}
	}
}

void Player::Dash()
{
	bool dashPressed = false;

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_K) == KEY_DOWN)
		dashPressed = true;

	if (Engine::GetInstance().input->IsGamepadConnected() &&
		Engine::GetInstance().input->GetGamepadButton(GAMEPAD_RB) == KEY_DOWN)
		dashPressed = true;

	// Start Dash
	if (GameManager::GetInstance().gameState.dashUnlocked == true &&
		dashPressed && isDashing == false &&
		dashCooldownTimer.ReadMSec() > dashCooldownMS)
	{
		if (lookingRight == true)
		{
			velocity.x = dashForce;
			anims.SetCurrent("dash_right");
			anims.GetAnim("dash_right")->SetLoop(false);
		}
		else
		{
			velocity.x = -dashForce;
			anims.SetCurrent("dash_left");
			anims.GetAnim("dash_left")->SetLoop(false);

		}

		Engine::GetInstance().audio->PlayFx(dashPrincesa);
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
		velocity.y = 0;

		if (dashTimer.ReadMSec() > dashDurationMS)
		{
			isDashing = false;
			dashCooldownTimer.Start();
		}
	}
}

void Player::Interact()
{
	bool interactPressed = false;

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_W) == KEY_DOWN)
		interactPressed = true;

	if (Engine::GetInstance().input->IsGamepadConnected() &&
		Engine::GetInstance().input->GetGamepadButton(GAMEPAD_Y) == KEY_DOWN)
		interactPressed = true;

	if (canInteract && interactuableBody != nullptr && interactPressed)
	{

		if (interactuableBody->ctype == ColliderType::DOOR)
		{
			bool isMaintenance = Engine::GetInstance().map->DoorUnderMaintenance(interactuableBody);
			if (isMaintenance)
			{
				Engine::GetInstance().audio->PlayFx(pickItemFx);
				Engine::GetInstance().hud->ShowNotification("The room is under maintenance. You cannot enter.");
				return;
			}

			bool isClosed = Engine::GetInstance().map->DoorClosed(interactuableBody);
			if (isClosed)
			{
				Engine::GetInstance().audio->PlayFx(pickItemFx);
				Engine::GetInstance().hud->ShowNotification("The room is closed. You cannot enter.");
				return;
			}

			bool requiresKey = Engine::GetInstance().map->DoorNeedsKey(interactuableBody);

			if (requiresKey)
			{
				//Get Key Property
				KeyType requiredKey = Engine::GetInstance().map->GetDoorKeyType(interactuableBody);

				if (requiredKey == KeyType::NONE) {
					Engine::GetInstance().audio->PlayFx(closedDoor);
					LOG("ERROR: Esta puerta necesita llave pero no se le asignó un KeyRegion en Tiled.");
					Engine::GetInstance().hud->ShowNotification("The door is locked. (Configuration Error)");
					return;
				}

				if (this->HasKey(requiredKey))
				{
					Engine::GetInstance().audio->PlayFx(openDoor);
					LOG("Has usado la llave %d para abrir la puerta.", (int)requiredKey);

					this->heldKeys.erase(requiredKey); 

					std::string doorId = Engine::GetInstance().map->GetDoorUniqueId(interactuableBody);
					if (!doorId.empty()) {
						GameManager::GetInstance().gameState.openedDoors.push_back(doorId);
					}

					if (Engine::GetInstance().map->DoorHasNoAnimation(interactuableBody))
					{
						
						Engine::GetInstance().sceneManager->setNewMap = true;
					}
					else {
						isFrozen = true;
						int cx, cy;
						interactuableBody->GetPosition(cx, cy);

						int doorW, doorH;
						Engine::GetInstance().map->GetDoorDimensions(interactuableBody, doorW, doorH);

						auto newEntity = Engine::GetInstance().entityManager->CreateEntity(EntityType::DOOR);
						DoorEntity* doorAnim = (DoorEntity*)newEntity.get();

						if (doorAnim != nullptr) {
							doorAnim->zOrder = -1;
							doorAnim->OpenDoorAt(Vector2D(cx, cy), doorW, doorH);
						}
					}
				}
				else
				{
					Engine::GetInstance().audio->PlayFx(closedDoor);	 
					LOG("Necesitas la llave ESPECIFICA (Tipo %d) para esta región.", (int)requiredKey);
					Engine::GetInstance().hud->ShowNotification("You need a specific region key to open this door.");
				}
			}
			else
			{
				LOG("Esta puerta no necesita llave");
				Engine::GetInstance().audio->PlayFx(openDoor);

				if (Engine::GetInstance().map->DoorHasNoAnimation(interactuableBody))
				{
					
					Engine::GetInstance().sceneManager->setNewMap = true;
				}
				else {
					isFrozen = true;
					int cx, cy;
					interactuableBody->GetPosition(cx, cy);

					int doorW, doorH;
					Engine::GetInstance().map->GetDoorDimensions(interactuableBody, doorW, doorH);

					auto newEntity = Engine::GetInstance().entityManager->CreateEntity(EntityType::DOOR);
					DoorEntity* doorAnim = (DoorEntity*)newEntity.get();

					if (doorAnim != nullptr) {
						doorAnim->zOrder = -1;
						doorAnim->OpenDoorAt(Vector2D(cx, cy), doorW, doorH);
					}
				}
			}
		}
		else if (interactuableBody->ctype == ColliderType::NPC)
		{
			Npc* npc = (Npc*)interactuableBody->listener;
			if (npc != nullptr)
			{
				Engine::GetInstance().dialogueManager->StartDialogue(npc->GetDialogueID());
				LOG("INICIO DIALOGO");
				Engine::GetInstance().input->ClearMouseInput();
			}
		}
	}
}

void Player::ApplyPhysics() {
	// Preserve vertical speed while jumping
	/*if (isJumping == true || secondJumpUsed == true) {
		velocity.y = Engine::GetInstance().physics->GetYVelocity(pbody);
	}*/
	velocity.y = Engine::GetInstance().physics->GetYVelocity(pbody);
	// --- LA CLAVE DEL IMPULSO HACIA ATRÁS ---
	// Si estamos en medio del rebote, FORZAMOS la velocidad horizontal
	// Esto evita que Box2D mate el impulso por la fricción contra la pared
	if (isWallJumping) {
		float wJumpForceX = speed * 1.0f;
		if (wallDirection == 1) {
			velocity.x = -wJumpForceX; // Empuje continuo a la izquierda
		}
		else {
			velocity.x = wJumpForceX;  // Empuje continuo a la derecha
		}
	}

	// --- WALL SLIDE ---
	// Añadida la condición !isWallJumping para que no te aplique el freno al saltar
	if (onWall && !onGround && !isWallJumping)
	{
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_L) == KEY_REPEAT)
		{
			// Se agarra: Frenamos la caída
			if (velocity.y > 0) {
				velocity.y = 1.5f;
			}

			// Recargamos estados
			isJumping = false;
			secondJumpUsed = false;
			onAir = false;

			// Animación
			if (anims.GetCurrentName() != "onWall_right" || anims.GetCurrentName() != "onWall_left") {
				if (!lookingRight)
				{
					anims.SetCurrent("onWall_right");
				}
				else
				{
					anims.SetCurrent("onWall_left");
				}
				currentAnimPriority = 1;
			}
		}
		else
		{
			// NO PRESIONA 'L': Caída normal.
			// Para evitar que la fricción de Box2D nos deje pegados al presionar hacia la pared,
			// matamos la velocidad X en la dirección de la pared si intentamos empujarla.
			if (wallDirection == 1 && velocity.x > 0) velocity.x = 0;   // Pared derecha
			if (wallDirection == -1 && velocity.x < 0) velocity.x = 0;  // Pared izquierda
		}
	}
	
	if (isGliding)
	{
		int maxFallSpeed = 2;
		if (velocity.y >= maxFallSpeed)
		{
			LOG("Gliding");
			velocity.y = maxFallSpeed;
		}
	}

	if (velocity.y > 5 && currentAnimPriority != 3)
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
	bool isDialogueActive = Engine::GetInstance().dialogueManager->IsDialogueActive();

	if (Engine::GetInstance().sceneManager->isGamePaused == false || isDialogueActive)
	{
		anims.Update(dt);
	}
	const SDL_Rect& animFrame = anims.GetCurrentFrame();

	// Update render position using your PhysBody helper
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	// Draw the player using the texture and the current animation frame
	if (isVisible)
	{
		// Draw the player using the texture and the current animation frame
		if (isKnockedback)
		{
			Uint8* r = new Uint8; Uint8* g = new Uint8; Uint8* b = new Uint8;
			Engine::GetInstance().render->SetColorMod(texture, r, g, b, 255, 25, 25);

			Engine::GetInstance().render->DrawRotatedTexture(texture, x, y - animFrame.h / 3, &animFrame, SDL_FLIP_NONE, 1.25f);

			Engine::GetInstance().render->SetColorMod(texture, nullptr, nullptr, nullptr, *r, *g, *b);
			delete r; delete g; delete b;
		}
		else
		{
			Engine::GetInstance().render->DrawRotatedTexture(texture, x, y - animFrame.h / 3, &animFrame, SDL_FLIP_NONE, 1.25f);
		}
	}

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

	float dt = Engine::GetInstance().GetDt();
	float dtSeconds = dt / 1000.0f;

	Vector2D targetCamPos = position;

	// ==========================================
	// BIFURCACIÓN DE LÓGICA DE CÁMARA
	// ==========================================
	if (currentCameraMode == CameraMode::DYNAMIC)
	{
		float targetYOffset = 0.0f;

		if (onGround && velocity.x == 0 && Engine::GetInstance().input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT) {
			lookDownTimer += dtSeconds;
			if (lookDownTimer >= 0.3f) {
				targetYOffset = 200.0f;
			}
		}
		else {
			lookDownTimer = 0.0f;
		}

		if (!onGround && velocity.y > 2.0f) {
			targetYOffset = 100.0f + (velocity.y * 5.0f);
			cameraController.SetSmoothSpeed(0.25f);
		}
		else if (onGround) {
			cameraController.SetSmoothSpeed(0.15f);
		}

		float lerpY = 4.0f * dtSeconds;
		if (lerpY > 1.0f) lerpY = 1.0f;
		currentCameraYOffset += (targetYOffset - currentCameraYOffset) * lerpY;

		targetCamPos.setY(targetCamPos.getY() + currentCameraYOffset);
	}
	else if (currentCameraMode == CameraMode::CLASSIC)
	{

		float targetYOffset = 0.0f;

		// 1. Anticipación de caída controlada (Si cae o baja escalones rápidamente)
		if (!onGround && velocity.y > 1.0f) {
			targetYOffset = 120.0f; // Límite estricto: Solo muestra un poco más abajo
			cameraController.SetSmoothSpeed(0.20f);
		}
		// 2. Mirar hacia abajo manualmente
		else if (onGround && velocity.x == 0 && Engine::GetInstance().input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT) {
			lookDownTimer += dtSeconds;
			if (lookDownTimer >= 0.3f) {
				targetYOffset = 150.0f; // Límite estricto de visión manual
			}
		}
		else {
			lookDownTimer = 0.0f;
			cameraController.SetSmoothSpeed(0.15f); // Velocidad normal
		}

		// Interpolar suavemente para evitar tirones
		float lerpY = 4.0f * dtSeconds;
		if (lerpY > 1.0f) lerpY = 1.0f;
		currentCameraYOffset += (targetYOffset - currentCameraYOffset) * lerpY;

		// Aplicar el offset limitado
		targetCamPos.setY(targetCamPos.getY() + currentCameraYOffset);
	}
	// ==========================================

	// Manda la posición a CameraController
	cameraController.Update(dt, targetCamPos, screenW, screenH, mapSize.getX(), mapSize.getY());
	float camX, camY;
	cameraController.GetCameraPosition(camX, camY);

	// Lógica del Eje X (Común para ambos modos)
	float targetCamX = -position.getX() + (screenW / 2.0f);
	if (targetCamX > 0) targetCamX = 0;
	float minCamX = -(mapSize.getX() - screenW);
	if (targetCamX < minCamX) targetCamX = minCamX;

	float currentCamX_f = Engine::GetInstance().render->camera.x;
	if (dtSeconds > 0.0f) {
		float lerpX = 8.0f * dtSeconds;
		if (lerpX > 1.0f) lerpX = 1.0f;
		currentCamX_f += (targetCamX - currentCamX_f) * lerpX;
	}

	Engine::GetInstance().render->camera.x = (int)currentCamX_f;
	Engine::GetInstance().render->camera.y = (int)camY;
}

void Player::SetCameraMode(CameraMode mode) {
	currentCameraMode = mode;

	if (currentCameraMode == CameraMode::CLASSIC) {
		cameraController.SetYDivisor(1.25f); // Vuelve a la vista original
		cameraController.SetSmoothSpeed(0.15f);
		currentCameraYOffset = 0.0f; // Resetea cualquier offset dinámico
	}
	else {
		cameraController.SetYDivisor(1.58f); // Vista de exploración
	}
}

std::unordered_map<int, std::string> Player::GetAliases(string name)
{
	std::unordered_map<int, std::string> aliases;
	if (name == "capeless")
	{
		aliases = { {0,"move_right"},
										 {12,"move_left"},
										 {24,"jump_right"},
										 {36,"fall_right" },
										 {48,"jump_left" } ,
										 {60,"fall_left"},
										 {72,"death_right"},
										 {84,"death_left" },
										 {96,"idle_right" },
										 {120,"idle_left" }
		};
	}
	else if (name == "cape")
	{
		aliases = { {0,"move_right"},
										 {12,"move_left"},
										 {24,"jump_right"},
										 {36,"fall_right" },
										 {48,"jump_left" } ,
										 {60,"fall_left"},
										 {72,"death_right" },
										 {84,"death_left" } ,
										 {96,"idle_right"},
										 {120,"idle_left" },
										 {144,"attack_down_right" },
										 {156,"attack_down_left" } ,
										 {168,"glide_right"},
										 {192,"glide_left" },
										 {216,"dash_right" },
										 {228,"dash_left" },
										 {240,"attack_right" },
										 {252,"attack_left" },
										 {264,"onWall_right" },
										 {265,"wall_jump_right" },
										 {276,"onWall_left" },
										 {277,"wall_jump_left" },
										 {288,"attack_up_right" },
										 {300,"attack_up_left" }
		};
	}
	return aliases;
}


void Player::UnlockCape()
{
	Engine::GetInstance().textures->UnLoad(texture);

	std::unordered_map<int, std::string> aliases = GetAliases("cape");
	anims.LoadFromTSX("Assets/Textures/Entities/Princess/Princess.tsx", aliases);

	anims.SetCurrent("idle_right");
	currentAnimPriority = 0;

	texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Princess/Princess.png");
	GameManager::GetInstance().gameState.glideUnlocked = true;

	AddItem(ItemID::GLIDE, 1);
}

void Player::UnlockSickle()
{
	GameManager::GetInstance().gameState.hasSickle = true;
	AddItem(ItemID::WEAPON, 1);
	LOG("Sickle Unlocked! You can attack now if you have the cape.");
}
void Player::UnlockDoubleJump() {
	GameManager::GetInstance().gameState.doubleJumpUnlocked = true;
	AddItem(ItemID::DOUBLEJUMP_OBJ, 1);
	Engine::GetInstance().hud->ShowNotification("You have unlocked DoubleJump!");
	LOG("Double Jump Unlocked! You can do a double jump");

}
void Player::UnlockDash() {
	GameManager::GetInstance().gameState.dashUnlocked = true;
	AddItem(ItemID::DASH_OBJ, 1);
	//Engine::GetInstance().hud->ShowNotification("You have unlocked Dash!");
	Engine::GetInstance().hud->ShowTutorial(TutorialType::DASH);

	LOG("Dash Unlocked! You can dash");
	LOG("Dash Unlocked! You can do a dash");
}

void Player::UnlockWallJump() {
	GameManager::GetInstance().gameState.wallJumpUnlocked = true;
	AddItem(ItemID::WALLJUMP_OBJ, 1);
	Engine::GetInstance().hud->ShowNotification("You have unlocked Wall Jump!");
	LOG("Wall Jump Unlocked! You can now wall jump");
}

void Player::UnlockSkill(SkillTree skill, int cost)
{
	auto& state = GameManager::GetInstance().gameState;

	// Comprobación dinámica utilizando el coste del JSON
	if (state.currentForceOrbs < cost) {
		LOG("No tienes suficientes orbes de fuerza. Necesitas: %d", cost);
		return;
	}

	switch (skill)
	{
	case SkillTree::HEALTH_UP:
		if (!state.stHealthUp) {
			state.stHealthUp = true;
			state.currentForceOrbs -= cost;       // Resta dinámica
			AddItem(ItemID::STRENGTH_ORB, -cost); // Sincroniza la UI de inventario

			state.maxHealth += 10;
			this->maxHealth = state.maxHealth;
			this->currentHealth += 10;
			state.currentHealth = this->currentHealth;
		}
		break;

	case SkillTree::IFRAMES_UP:
		if (!state.stIframesUp) {
			state.stIframesUp = true;
			state.currentForceOrbs -= cost;
			AddItem(ItemID::STRENGTH_ORB, -cost);
			this->invincibilityDurationMS = 2000.0f;
		}
		break;

	case SkillTree::SPEED_UP:
		if (!state.stSpeedUp) {
			state.stSpeedUp = true;
			state.currentForceOrbs -= cost;
			AddItem(ItemID::STRENGTH_ORB, -cost);
			this->speed += 5.0f;
		}
		break;

	case SkillTree::FAST_DASH:
		if (!state.stFastDash) {
			state.stFastDash = true;
			state.currentForceOrbs -= cost;
			AddItem(ItemID::STRENGTH_ORB, -cost);
			this->dashForce += 15.0f;
		}
		break;

	case SkillTree::UP_ATTACK:
		if (!state.stUpAttack) {
			state.stUpAttack = true;
			state.currentForceOrbs -= cost;
			AddItem(ItemID::STRENGTH_ORB, -cost);
		}
		break;

	case SkillTree::DOWN_ATTACK:
		if (!state.stDownAttack) {
			state.stDownAttack = true;
			state.currentForceOrbs -= cost;
			AddItem(ItemID::STRENGTH_ORB, -cost);
		}
		break;
	}
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

// ==========================================
// INVENTORY SYSTEM
// ==========================================

void Player::AddItem(ItemID id, int amount) {
	inventory[id] += amount;
}

bool Player::HasItem(ItemID id) {
	return inventory[id] > 0;
}

int Player::GetItemCount(ItemID id) {
	return inventory[id];
}

// Define OnCollision function for the player. 
void Player::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
	if (physA == attackCollider) {
		if (physB->ctype == ColliderType::ENEMY) {
			int ex, ey;
			physB->GetPosition(ex, ey);
			Engine::GetInstance().particleManager->EmitAttack((float)ex, (float)ey, lookingRight);
		}
		return;
	}

	if (physA->ctype == ColliderType::PLAYER && physB->ctype == ColliderType::PLAYER) { return; }

	ShapeType typeA = (ShapeType)(uintptr_t)Engine::GetInstance().physics->GetShapeUserData(shapeA);
	ShapeType typeB = (ShapeType)(uintptr_t)Engine::GetInstance().physics->GetShapeUserData(shapeB);

	if (typeA == ShapeType::NONE && typeB != ShapeType::NONE) //Temportal? Fix
	{
		// TO DO: Con el rectangulo (Middle) se guarda correctamente en typeA, con ambos circulos se guarda en typeB porque los detecta en shapeB
		typeA = typeB;
	}

	switch (physB->ctype)
	{
	case ColliderType::DANGER:
		LOG("Collision with DANGER zone!");
		if (!godMode && !isdead && !isInvincible)
		{
			TakeDamage(10); // Environmental Damage
			if (!isdead)
			{
				RespawnFromVoid();
				isInvincible = true;
				invincibilityTimer.Start();
			}
		}
		break;

	case ColliderType::MAP:
	case ColliderType::SPECIALFLOOR:

		if (typeA == ShapeType::SHAPE_BOTTOM)
		{
			LOG("Collision inf circle / GROUND");

			// Efecto de polvo al aterrizar
			if (!onGround) { // Solo si el jugador viene del aire 
				int pX, pY;
				pbody->GetPosition(pX, pY); 
				float footY = (float)pY + (texH / 2.0f) - 80.0f;
				Engine::GetInstance().particleManager->EmitJumpDust((float)pX, footY, lookingRight);
			}

			// Reset the jump flag when touching the ground
			isJumping = false;
			secondJumpUsed = false;

			if (currentAnimPriority > 1 && currentAnimPriority != 4)
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

			onGround = true;
			onAir = false;
			onWall = false;
		}
		else if (typeA == ShapeType::SHAPE_MIDDLE)
		{
			LOG("Collision middle / WALL");

			// Solo marcamos que tocamos pared y la dirección. 
			// ELIMINADO el reseteo de saltos y la animación.
			onWall = true;

			if (lookingRight) {
				wallDirection = 1;
			}
			else {
				wallDirection = -1;
			}
		}
		else if (typeA == ShapeType::SHAPE_TOP)
		{
			LOG("Collision sup circle / CEILING");
		}
		break;

	case ColliderType::DOOR:
		canInteract = true;
		interactuableBody = physB;
		break;
	case ColliderType::PATH:
	{
		bool requiresGlide = Engine::GetInstance().map->DoorRequiresGlide(physB);

		if (requiresGlide && !GameManager::GetInstance().gameState.glideUnlocked)
		{
			Engine::GetInstance().hud->ShowNotification("Quieres morir estampada contra el suelo?");

			velocity = { 0.0f, 0.0f };
			Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);

			Vector2D roomStartPos = Engine::GetInstance().map->GetPlayerSpawnPoint("");

			SetPosition(roomStartPos);

			lastSafePosition = roomStartPos;

			isJumping = false;
			secondJumpUsed = false;

			Engine::GetInstance().audio->PlayFx(respawnFx);

			return;
		}

		interactuableBody = physB;
		Engine::GetInstance().sceneManager->setNewMap = true;
		break;
	}
    interactuableBody = physB;
    Engine::GetInstance().sceneManager->setNewMap = true;
    break;
	case ColliderType::ITEM:
		LOG("Collision ITEM");
		//efecto Particula
		int itemX, itemY;
		physB->GetPosition(itemX, itemY);
		Engine::GetInstance().particleManager->EmitItemPickup(itemX, itemY);

		if (physB->listener->name == "Manta") {
			LOG("Collision ITEM (Manta Picked Up)");
			//Engine::GetInstance().hud->ShowNotification("You have obtained the Cape.");
			Engine::GetInstance().hud->ShowTutorial(TutorialType::GLIDE);
		}
		else if (physB->listener->name == "Key") {
			LOG("Collision ITEM (Key Picked Up)");
			GameManager::GetInstance().gameState.keyCount++;

			AddItem(ItemID::KEY, 1);

			LOG("KeyNum: %d", GameManager::GetInstance().gameState.keyCount);
			Engine::GetInstance().hud->ShowNotification("You have obtained a Key.");

		}
		else if (physB->listener->name == "Sickle") {
			LOG("Collision ITEM (Sickle Picked Up)");
			//Engine::GetInstance().hud->ShowNotification("You have obtained the Sickle.");
			Engine::GetInstance().hud->ShowTutorial(TutorialType::ATTACK);
		}
		Engine::GetInstance().audio->PlayFx(pickItemFx);
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
			int orbX, orbY;
			physB->GetPosition(orbX, orbY);
			Engine::GetInstance().particleManager->EmitItemPickup(orbX, orbY);

			Engine::GetInstance().audio->PlayFx(orbFx);
			physB->listener->Destroy();
			Engine::GetInstance().hud->ShowNotification("You have recovered your health.");
		}
		break;
	case ColliderType::SKILL_POINT_ORB:
		GameManager::GetInstance().gameState.currentForceOrbs++;
		AddItem(ItemID::STRENGTH_ORB, 1);

		int spOrbX, spOrbY;
		physB->GetPosition(spOrbX, spOrbY);
		Engine::GetInstance().particleManager->EmitItemPickup(spOrbX, spOrbY);

		Engine::GetInstance().audio->PlayFx(orbFx);
		physB->listener->Destroy();
		Engine::GetInstance().hud->ShowNotification("You have obtained an Orb of Power.");
		break;
	case ColliderType::SAVEPOINT:
	{
		LOG("Collision SavePoint");
		SavePoint* sp = (SavePoint*)physB->listener;
		Engine::GetInstance().audio->PlayFx(savePointFx); //fx
		sp->Activate();

		int spX, spY;
		physB->GetPosition(spX, spY);
		respawnPosition = Vector2D((float)spX, (float)spY);

		auto& gameState = GameManager::GetInstance().gameState;
		gameState.playerPosition = respawnPosition;
		gameState.currentHealth = this->currentHealth;
		gameState.currentMap = Engine::GetInstance().map->mapFileName;

		if (GameManager::GetInstance().SaveGame("savegame.xml")) {
			Engine::GetInstance().hud->ShowNotification("Partida Guardada");
		}
		else {
			Engine::GetInstance().hud->ShowNotification("Error al guardar partida");
		}
		break;
	}
	case ColliderType::NPC:
		canInteract = true;
		interactuableBody = physB;
		break;
	case ColliderType::ENEMY:
		if (!isInvincible && !isdead)
		{
		Engine::GetInstance().audio->PlayFx(recibirDamage);
		TakeDamage(10); // Contact Damage
		//PARTICULA
		Engine::GetInstance().particleManager->EmitHitSparks(position.getX(), position.getY(), true);

		isKnockedback = true;
		isInvincible = true;
		invincibilityTimer.Start();
		}
		break;
	case ColliderType::ENEMY_ATTACK:
		LOG("Hit player");
		if (!isInvincible && !isdead)
		{
			Engine::GetInstance().audio->PlayFx(recibirDamage);
			TakeDamage(physB->listener->damage - defbuff);
			//PARTICULA
			Engine::GetInstance().particleManager->EmitHitSparks(position.getX(), position.getY(), true);

			isKnockedback = true;
			isInvincible = true;
			invincibilityTimer.Start();

			int enemyX, enemyY;
			physB->GetPosition(enemyX, enemyY);

			hitFromRight = (enemyX > position.getX());
			break;

		}
	case ColliderType::UNKNOWN:
		LOG("Collision UNKNOWN");
		break;

	default:
		break;
	}
}

void Player::OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
	if (physA == attackCollider) { return; }
	if (physA->ctype == ColliderType::PLAYER && physB->ctype == ColliderType::PLAYER) { return; }

	ShapeType typeA = (ShapeType)(uintptr_t)Engine::GetInstance().physics->GetShapeUserData(shapeA);
	ShapeType typeB = (ShapeType)(uintptr_t)Engine::GetInstance().physics->GetShapeUserData(shapeB);

	if (typeA == ShapeType::NONE && typeB != ShapeType::NONE) //Temportal? Fix
	{
		// TO DO: Con el rectangulo (Middle) se guarda correctamente en typeA, con ambos circulos se guarda en typeB porque los detecta en shapeB
		typeA = typeB;
	}

	switch (physB->ctype)
	{
	case ColliderType::MAP:
	case ColliderType::SPECIALFLOOR:
		if (typeA == ShapeType::SHAPE_BOTTOM)
		{
			onGround = false;
			onAir = true;
			LOG("On Air");
		}
		else if (typeA == ShapeType::SHAPE_MIDDLE)
		{
			LOG("Off WALL");
			onAir = true;
			onWall = false;

		}
		else if (typeA == ShapeType::SHAPE_TOP)
		{
			LOG("Collision End CEILING");
		}

		break;
	case ColliderType::DOOR:
		canInteract = false;
		interactuableBody = nullptr;
		break;
	case ColliderType::NPC:
		canInteract = false;
		interactuableBody = nullptr;
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
	position = pos;
	if (pbody != nullptr) {
		pbody->SetPosition((int)(pos.getX() + texW / 2), (int)(pos.getY() + texH / 2));
	}
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

	// GodMode
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
		GameManager::GetInstance().gameState.currentForceOrbs++; // Sumar al GameState
		AddItem(ItemID::STRENGTH_ORB, 1);                        // Sumar a la UI del Inventario

		LOG("Skill Point Added. Current Orbs in GameState: %d", GameManager::GetInstance().gameState.currentForceOrbs);
	}

	//// Unlock Skills
	//if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_1) == KEY_DOWN)
	//{
	//	if (currentForceOrbs > 0)
	//	{
	//		if (OffensiveSkills[2] == false)
	//		{
	//			LOG("Unlocking Offensive Skill:");
	//			if (OffensiveSkills[1] == true) { OffensiveSkills[2] = true; LOG("Offensive Skill 3 Unlocked"); }
	//			else if (OffensiveSkills[0] == true) { OffensiveSkills[1] = true; LOG("Offensive Skill 2 Unlocked"); }
	//			else { OffensiveSkills[0] = true; LOG("Offensive Skill 1 Unlocked"); }
	//			currentForceOrbs--;
	//		}
	//		else { LOG("Offensive Tree Maxed"); }
	//	}
	//	else { LOG("Not Enough Skill Points"); }
	//}

	//if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_2) == KEY_DOWN)
	//{
	//	if (currentForceOrbs > 0)
	//	{
	//		if (DefensiveSkills[2] == false)
	//		{
	//			LOG("Unlocking Defensive Skill:");
	//			if (DefensiveSkills[1] == true) { DefensiveSkills[2] = true; LOG("Defensive Skill 3 Unlocked"); }
	//			else if (DefensiveSkills[0] == true) { DefensiveSkills[1] = true; LOG("Defensive Skill 2 Unlocked"); }
	//			else { DefensiveSkills[0] = true; LOG("Defensive Skill 1 Unlocked"); }
	//			currentForceOrbs--;
	//		}
	//		else { LOG("Defensive Tree Maxed"); }
	//	}
	//	else { LOG("Not Enough Skill Points"); }
	//}

	//if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_3) == KEY_DOWN)
	//{
	//	if (currentForceOrbs > 0)
	//	{
	//		if (UtilitySkills[2] == false)
	//		{
	//			LOG("Unlocking Utility Skill:");
	//			if (UtilitySkills[1] == true) { UtilitySkills[2] = true; LOG("Utility Skill 3 Unlocked"); }
	//			else if (UtilitySkills[0] == true) { UtilitySkills[1] = true; LOG("Utility Skill 2 Unlocked"); }
	//			else { UtilitySkills[0] = true; LOG("Utility  Skill 1 Unlocked"); }
	//			currentForceOrbs--;
	//		}
	//		else { LOG("Utility Tree Maxed"); }
	//	}
	//	else { LOG("Not Enough Skill Points"); }
	//}

	if (godMode)
	{
		GodModeMove(dt);
	}

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_9) == KEY_DOWN)
	{
		UnlockCape();
		UnlockSickle();
		GameManager::GetInstance().gameState.hasSickle = true;
		GameManager::GetInstance().gameState.dashUnlocked = true;
		GameManager::GetInstance().gameState.doubleJumpUnlocked = true;
		GameManager::GetInstance().gameState.currentForceOrbs += 10;
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