#include "Player.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "SceneManager.h"
#include "GameManager.h"
#include "ParticleManager.h"

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
	pbody = Engine::GetInstance().physics->CreateCapsule((int)position.getX(), (int)position.getY(), texW / 3, texW * 19/24 , texH * 3/2, bodyType::DYNAMIC);

	// Assign listener of the pbody. This makes the Physics module to call the OnCollision method
	pbody->listener = this;
	pbody->ctype = ColliderType::PLAYER;

	knockbackForce = 7.0f;

	maxHealth = 100;
	currentHealth = maxHealth;

	cameraController.SetSmoothSpeed(0.15f);
	cameraController.SetVerticalOffset(-25.0f);
	respawnPosition = position;
	lastSafePosition = position;
	safePositionTimer.Start();
	return true;
}

bool Player::Update(float dt)
{
	if (pbody == nullptr) return true;
	Engine::GetInstance().entityManager->SetPlayer(this);

	if (Engine::GetInstance().sceneManager->isGamePaused == false && !isdead)
	{
		GetPhysicsValues();

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
	//velocity = { 0, velocity.y }; // Reset horizontal velocity by default, this way the player stops when no key is pressed
	if (!isWallJumping) {
		velocity = { 0, velocity.y };
	}
}

void Player::Move() {
	bool isMovingThisFrame = false;

	// Move Left
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT && isDashing == false && !isWallJumping)
	{
		velocity.x = -speed;
		lookingRight = false;
		isMovingThisFrame = true;
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
	else if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT && isDashing == false && !isWallJumping)
	{
		velocity.x = speed;
		lookingRight = true;
		isMovingThisFrame = true;
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
	bool isWalkingConditions = (isMovingThisFrame && onGround && !isDashing && !isAttacking && !isdead);

	if (isMovingThisFrame && onGround && !isDashing && !isAttacking && !isdead)
	{
		stepTimer += Engine::GetInstance().GetDt() / 1000.0f;

		if (stepTimer >= timeBetweenSteps)
		{
			// OJO: Le pasamos 0 repeticiones para que sea súper ligero para la memoria
			Engine::GetInstance().audio->PlayFx(caminarPrincesa, 0);
			if (lookingRight) {
				footX = position.getX() - 35.0f; // Pegado al talón
				footY = position.getY() + (texH / 2.0f) - 10.0f; // A nivel del suelo
			}
			else {
				footX = position.getX() + 35.0f; // Pegado al talón
				footY = position.getY() + (texH / 2.0f) - 10.0f;
			}
			Engine::GetInstance().particleManager->EmitDust(footX, footY, lookingRight);

			stepTimer = 0.0f;
		}
	}
	else
	{
		// Forzamos el contador para que al aterrizar el primer paso suene de inmediato
		stepTimer = timeBetweenSteps;
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
		anims.SetCurrent("hurt"); //TO DO: Add the animation for taking damage
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
	Engine::GetInstance().audio->PlayFx(respawnFx);
	LOG("Player reset to last safe position: %.2f, %.2f", lastSafePosition.getX(), lastSafePosition.getY());
}

void Player::Jump(float dt)
{
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN)
	{
		// --- 1. WALL JUMP (Estilo Hollow Knight) ---
		if (onWall == true && onGround == false)
		{
			Engine::GetInstance().audio->PlayFx(jumpFx);
			isJumping = true;
			onWall = false; // Nos despegamos de la pared
			onAir = true;

			isWallJumping = true;
			wallJumpTimer = 0.30f;
	

			// Fuerza del rebote
			float wJumpForceY = jumpForce/2;
			// ¡Aumentamos el multiplicador a 2.5f (o más) para que el empuje sea innegable!
			float wJumpForceX = speed * 1.0f;

			// Escupir al jugador en la dirección contraria a la pared
			if (wallDirection == 1) {
				// Pared a la derecha -> Salto a la izquierda
				velocity.x = -wJumpForceX;
				lookingRight = false;
				anims.SetCurrent("jump_left");
			}
			else {
				// Pared a la izquierda -> Salto a la derecha
				velocity.x = wJumpForceX;
				lookingRight = true;
				anims.SetCurrent("jump_right");
			}

			// Aplicar las fuerzas directamente al cuerpo físico AHORA MISMO
			Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);
			Engine::GetInstance().physics->SetYVelocity(pbody, wJumpForceY);

			currentAnimPriority = 2;
			isJumpKeyDown = true;
			jumpHoldTime = 0.00f;

			LOG("Wall Jump Estilo HK");
		}
		// --- 2. BASE JUMP ---
		else if (isJumping == false && onGround == true)
		{
			Engine::GetInstance().audio->PlayFx(jumpFx);
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
		else if (GameManager::GetInstance().gameState.doubleJumpUnlocked && (isJumping == true || onAir == true) && secondJumpUsed == false)
		{
			Engine::GetInstance().audio->PlayFx(jumpFx);
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
	// --- 4. EXTENDER SALTO MANTENIENDO EL BOTÓN ---
	else if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_REPEAT && isJumping && isJumpKeyDown && jumpHoldTime <= maxJumpHoldTime)
	{
		Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, 0.0f, jumpForce * 0.005f, true); //TO DO: Adjust Value
		jumpHoldTime += dt / 1000; //To seconds
	}
	// --- 5. SOLTAR BOTÓN DE SALTO ---
	else if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_UP)
	{
		isJumpKeyDown = false;
	}
}

void Player::Attack(float dt)
{
	// 1. Start the attack 
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F) == KEY_DOWN && !isGliding && !isAttacking)
	{
		if (GameManager::GetInstance().gameState.hasSickle && GameManager::GetInstance().gameState.glideUnlocked)
		{
			Engine::GetInstance().audio->PlayFx(attackFx);
			isAttacking = true;
			if (lookingRight)
			{
				anims.SetCurrent("attack_right");
				anims.GetAnim("attack_right")->SetLoop(false);
			}
			else
			{
				anims.SetCurrent("attack_left");
				anims.GetAnim("attack_left")->SetLoop(false);
			}

			currentAnimPriority = 4;

			currentAttackTime = 0.0f;
			timeSinceLastAttack = 0.0f;

			if (comboStep == 0)
			{
				// first attack
				damage = 10;
				currentAttackWidth = 60;
				currentAttackHeight = 64;
				currentAttackOffsetX = texW / 2 + currentAttackWidth / 2;
				LOG("Attack 1 started (Normal)");
			}
			else
			{
				// second attack
				damage = 15;
				currentAttackWidth = 120;
				currentAttackHeight = 90;
				currentAttackOffsetX = texW / 2 + currentAttackWidth / 2;
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
		else
		{
			// Prompt text
			if (!GameManager::GetInstance().gameState.hasSickle && !GameManager::GetInstance().gameState.glideUnlocked) {
				Engine::GetInstance().hud->ShowNotification("You need to find the Sickle and the Cape.");
			}
			else if (!GameManager::GetInstance().gameState.hasSickle) {
				Engine::GetInstance().hud->ShowNotification("You need to find the Sickle.");
			}
			else if (!GameManager::GetInstance().gameState.glideUnlocked) {
				Engine::GetInstance().hud->ShowNotification("You need to find the Cape.");
			}
		}
	}

	// 2. Control the duration of the attack
	if (isAttacking)
	{
		currentAttackTime += dt / 1000.0f;

		// Update the collider's position so that it follows the player whilst attacking
		if (attackCollider != nullptr) {
			int attackOffsetX = lookingRight ? currentAttackOffsetX : -currentAttackOffsetX;
			attackCollider->SetPosition(position.getX() + attackOffsetX, position.getY());
		}

		// End the attack when the time runs out
		if (currentAttackTime >= attackDuration &&
			(anims.GetAnim("attack_right")->HasFinishedOnce() || anims.GetAnim("attack_left")->HasFinishedOnce() || anims.GetCurrentName() != "attack_right" || anims.GetCurrentName() != "attack_left"))
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
	if (GameManager::GetInstance().gameState.glideUnlocked)
	{
		if (onAir == true && onGround == false && Engine::GetInstance().input->GetKey(SDL_SCANCODE_LSHIFT) == KEY_REPEAT)
		{
			if (!isGliding) {
				Engine::GetInstance().audio->PlayFx(planearPrincesa);
			}
			isGliding = true;
			if (lookingRight)
			{
				anims.SetCurrent("glide_right");
			}
			else
			{
				anims.SetCurrent("glide_left");
			}
			currentAnimPriority = 5;
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
	if (GameManager::GetInstance().gameState.dashUnlocked == true
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
	if (canInteract && interactuableBody != nullptr)
	{
		// Asegurarse que es una puerta
		if (interactuableBody->ctype == ColliderType::DOOR)
		{
			if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_W) == KEY_DOWN)
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

				//Pregunta si esta puerta necesita llave
				bool requiresKey = Engine::GetInstance().map->DoorNeedsKey(interactuableBody);

				if (requiresKey)
				{
					// Si necesita
					if (GameManager::GetInstance().gameState.keyCount > 0)
					{
						Engine::GetInstance().audio->PlayFx(openDoor);
						//Restar una unidad cuando se usa una llave
						GameManager::GetInstance().gameState.keyCount--;
						LOG("Has usado una llave. Te quedan: %d ", GameManager::GetInstance().gameState.keyCount);

						std::string doorId = Engine::GetInstance().map->GetDoorUniqueId(interactuableBody);
						if (!doorId.empty()) {
							GameManager::GetInstance().gameState.openedDoors.push_back(doorId);
						}
						Engine::GetInstance().sceneManager->setNewMap = true;
					}
					else
					{
						Engine::GetInstance().audio->PlayFx(closedDoor);
						LOG("Necesitas una llave para abrir, busca una ");
						Engine::GetInstance().hud->ShowNotification("You need a key to open this door.");
					}
				}
				else
				{

					// Si no
					LOG("Esta puerta no necesita llave ");
					Engine::GetInstance().sceneManager->setNewMap = true;
				}
			}
		}
	}
}

void Player::ApplyPhysics() {
	// Preserve vertical speed while jumping
	if (isJumping == true || secondJumpUsed == true) {
		velocity.y = Engine::GetInstance().physics->GetYVelocity(pbody);
	}

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
	if (onWall && !onGround && velocity.y > 0 && !isWallJumping) {
		velocity.y = 2.0f;
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
	if (Engine::GetInstance().sceneManager->isGamePaused == false)
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
										 {144,"attack_right" },
										 {156,"attack_left" } ,
										 {168,"glide_right"},
										 {192,"glide_left" },

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
	LOG("Double Jump Unlocked! You can do a double jump");

}
void Player::UnlockDash() {
	GameManager::GetInstance().gameState.dashUnlocked = true;
	AddItem(ItemID::DASH_OBJ, 1);
	LOG("Dash Unlocked! You can dash");
	LOG("Dash Unlocked! You can do a dash");
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
	if (physA == attackCollider) { return; }
	if (physA->ctype == ColliderType::PLAYER && physB->ctype == ColliderType::PLAYER)	{return;}

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
		if (!godMode && !isdead) 
		{
			TakeDamage(10); // Environmental Damage
			if (!isdead)
			{
				RespawnFromVoid();
			}
		}
		break;

	case ColliderType::MAP:

		if (typeA == ShapeType::SHAPE_BOTTOM)
		{
			LOG("Collision inf circle / GROUND");
			// Reset the jump flag when touching the ground
			isJumping = false;
			secondJumpUsed = false;

			if (currentAnimPriority > 1)
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
			// Reset the jump flag
			isJumping = false;
			secondJumpUsed = false;

			anims.SetCurrent("wall"); //TODO: On wall anim
			onWall = true;
			
			onAir = false;

			if (lookingRight) {
				wallDirection = 1;  // La pared está a la derecha
			}
			else {
				wallDirection = -1; // La pared está a la izquierda
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
		interactuableBody = physB;
		Engine::GetInstance().sceneManager->setNewMap = true;
		break;
	case ColliderType::ITEM:
		LOG("Collision ITEM");

		if (physB->listener->name == "Manta") {
			LOG("Collision ITEM (Manta Picked Up)");
			Engine::GetInstance().hud->ShowNotification("You have obtained the Cape.");
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
			Engine::GetInstance().hud->ShowNotification("You have obtained the Sickle.");
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
			Engine::GetInstance().audio->PlayFx(orbFx);
			physB->listener->Destroy();
			Engine::GetInstance().hud->ShowNotification("You have recovered your health.");
		}
		break;
	case ColliderType::SKILL_POINT_ORB:
		currentForceOrbs++;
		AddItem(ItemID::STRENGTH_ORB, 1);
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

	case ColliderType::ENEMY:
		Engine::GetInstance().audio->PlayFx(recibirDamage);
		TakeDamage(10); // Contact Damage
		isKnockedback = true;
		break;
	case ColliderType::ENEMY_ATTACK:
		LOG("Hit player");
		Engine::GetInstance().audio->PlayFx(recibirDamage);
		TakeDamage(physB->listener->damage);
		isKnockedback = true;

		int enemyX, enemyY;
		physB->GetPosition(enemyX, enemyY);

		hitFromRight = (enemyX > position.getX());
		break;


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
		GameManager::GetInstance().gameState.hasSickle = true;
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