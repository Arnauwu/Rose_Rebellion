#include "Player.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "SceneManager.h"
#include "GameManager.h"

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
	// Cacheo local de módulos necesarios
	auto entityManager = Engine::GetInstance().entityManager;

	entityManager->SetPlayer(this);

	// Inicialización de parámetros
	currentHealth = GameManager::GetInstance().gameState.currentHealth;
	return true;
}

bool Player::Start()
{
	// Cacheo local de módulos
	auto engine = &Engine::GetInstance();
	auto audio = engine->audio;
	auto textures = engine->textures;
	auto physics = engine->physics;

	engine->entityManager->SetPlayer(this);

	// Carga de FX
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
	pbody = physics->CreateCircle((int)position.getX(), (int)position.getY(), texW / 2, bodyType::DYNAMIC);
	pbody->listener = this;
	pbody->ctype = ColliderType::PLAYER;

	knockbackForce = 5.0f;
	maxHealth = 100;
	currentHealth = maxHealth;

	cameraController.SetSmoothSpeed(0.15f);
	cameraController.SetVerticalOffset(-25.0f);
	respawnPosition = position;

	return true;
}

bool Player::Update(float dt)
{
	if (pbody == nullptr) return true;

	auto engine = &Engine::GetInstance();
	auto sceneManager = engine->sceneManager;

	if (!sceneManager->isGamePaused && !isdead)
	{
		GetPhysicsValues();
		Move();
		Knockback();
		Jump(dt);

		timeSinceLastAttack += dt / 1000.0f;
		if (timeSinceLastAttack >= comboResetTime) {
			comboStep = 0;
		}

		Attack(dt);
		Glide();
		Dash();
		ApplyPhysics();
	}

	if (isdead)
	{
		auto physics = engine->physics;
		auto audio = engine->audio;

		if (currentAnimPriority < 99)
		{
			currentAnimPriority = 99;
			audio->PlayFx(morirPrincesa);

			if (lookingRight) {
				anims.GetAnim("death_right")->SetLoop(false);
				anims.SetCurrent("death_right");
			}
			else {
				anims.GetAnim("death_left")->SetLoop(false);
				anims.SetCurrent("death_left");
			}

			physics->SetLinearVelocity(pbody, { 0, 0 });

			if (attackCollider != nullptr) {
				physics->DeletePhysBody(attackCollider);
				attackCollider = nullptr;
			}
		}
		else if ((lookingRight && anims.GetAnim("death_right")->HasFinishedOnce()) ||
			(!lookingRight && anims.GetAnim("death_left")->HasFinishedOnce()))
		{
			sceneManager->ChangeScene(SceneID::GAMEOVER);
		}
	}

	CameraFollows();
	Draw(dt);
	DevTools(dt);

	// Lógica de posición segura
	if (onGround && onWall && !isJumping && !isdead)
	{
		safePositionTimer += dt / 1000.0f;
		if (safePositionTimer >= 0.2f)
		{
			Vector2D start = position;
			Vector2D end = { position.getX(), position.getY() + (texH / 2) + 5 };
			if (engine->physics->Raycast(start, end)) {
				lastSafePosition = position;
				safePositionTimer = 0.0f;
			}
		}
	}
	return true;
}

bool Player::PostUpdate()
{
	if (!Engine::GetInstance().sceneManager->isGamePaused && !isdead)
	{
		Interact();
	}
	return true;
}

void Player::GetPhysicsValues()
{
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity = { 0, velocity.y };
}

void Player::Move() {
	auto input = Engine::GetInstance().input;
	bool isMovingThisFrame = false;

	if (input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT && !isDashing)
	{
		velocity.x = -speed;
		lookingRight = false;
		isMovingThisFrame = true;
		if (currentAnimPriority == 3) anims.SetCurrent("fall_left");
		else if (currentAnimPriority == 2) anims.SetCurrent("jump_left");
		else if (currentAnimPriority <= 1) { anims.SetCurrent("move_left"); currentAnimPriority = 1; }
	}
	else if (input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT && !isDashing)
	{
		velocity.x = speed;
		lookingRight = true;
		isMovingThisFrame = true;
		if (currentAnimPriority == 3) anims.SetCurrent("fall_right");
		else if (currentAnimPriority == 2) anims.SetCurrent("jump_right");
		else if (currentAnimPriority <= 1) { anims.SetCurrent("move_right"); currentAnimPriority = 1; }
	}
	else
	{
		if (!isAttacking && !isJumping && !isDashing)
		{
			if (lookingRight) anims.SetCurrent("idle_right");
			else anims.SetCurrent("idle_left");
			currentAnimPriority = 0;
		}
	}

	// Sonido de pasos
	auto audio = Engine::GetInstance().audio;
	bool isWalkingConditions = (isMovingThisFrame && onGround && !isDashing && !isAttacking && !isdead);

	if (isWalkingConditions)
	{
		stepTimer += Engine::GetInstance().GetDt() / 1000.0f;
		if (stepTimer >= timeBetweenSteps) {
			audio->PlayFx(caminarPrincesa, 0);
			stepTimer = 0.0f;
		}
	}
	else {
		stepTimer = timeBetweenSteps;
		if (wasWalking) audio->StopFx(caminarPrincesa);
	}
	wasWalking = isWalkingConditions;
}

void Player::Knockback()
{
	if (isdead) return;
	if (isKnockedback)
	{
		anims.SetCurrent("hurt");
		velocity.x = lookingRight ? -knockbackForce : knockbackForce;
	}

	float dt = Engine::GetInstance().GetDt();
	if (knockbackTime <= 0) {
		isKnockedback = false;
		knockbackTime = 500.0f;
	}
	else {
		knockbackTime -= dt;
	}
}

void Player::Respawn()
{
	if (isdead) {
		auto engine = &Engine::GetInstance();
		isAttacking = false;
		if (attackCollider != nullptr) {
			engine->physics->DeletePhysBody(attackCollider);
			attackCollider = nullptr;
		}
		currentHealth = maxHealth;
		engine->audio->PlayFx(respawnFx);
		isJumping = false;
		isdead = false;
		anims.SetCurrent("idle");
	}
}

void Player::RespawnFromVoid() {
	auto engine = &Engine::GetInstance();
	auto physics = engine->physics;

	physics->SetLinearVelocity(pbody, { 0.0f, 0.0f });

	if (isAttacking && attackCollider != nullptr) {
		physics->DeletePhysBody(attackCollider);
		attackCollider = nullptr;
		isAttacking = false;
	}

	SetPosition(respawnPosition);
	isJumping = false;
	secondJumpUsed = false;
	anims.SetCurrent("idle");
	engine->audio->PlayFx(respawnFx);
}

void Player::Jump(float dt)
{
	auto input = Engine::GetInstance().input;
	auto physics = Engine::GetInstance().physics;
	auto audio = Engine::GetInstance().audio;

	if (input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN)
	{
		if (!isJumping && onGround)
		{
			audio->PlayFx(jumpFx);
			isJumping = true;
			physics->SetYVelocity(pbody, jumpForce);
			anims.SetCurrent(lookingRight ? "jump_right" : "jump_left");
			currentAnimPriority = 2;
			isJumpKeyDown = true;
			jumpHoldTime = 0.00f;
		}
		else if (doubleJumpUnlocked && (isJumping || onAir) && !secondJumpUsed)
		{
			audio->PlayFx(jumpFx);
			secondJumpUsed = true;
			physics->SetYVelocity(pbody, jumpForce);
			anims.SetCurrent(lookingRight ? "jump_right" : "jump_left");
			currentAnimPriority = 2;
			isJumpKeyDown = true;
			jumpHoldTime = 0.00f;
		}
	}
	else if (input->GetKey(SDL_SCANCODE_SPACE) == KEY_REPEAT && isJumping && isJumpKeyDown && jumpHoldTime <= maxJumpHoldTime)
	{
		physics->ApplyLinearImpulseToCenter(pbody, 0.0f, jumpForce * 0.005f, true);
		jumpHoldTime += dt / 1000;
	}
	else if (input->GetKey(SDL_SCANCODE_SPACE) == KEY_UP)
	{
		isJumpKeyDown = false;
	}
}

void Player::Attack(float dt)
{
	auto input = Engine::GetInstance().input;
	auto audio = Engine::GetInstance().audio;
	auto physics = Engine::GetInstance().physics;

	if (input->GetKey(SDL_SCANCODE_F) == KEY_DOWN && !isGliding && !isAttacking)
	{
		if (GameManager::GetInstance().gameState.hasSickle && GameManager::GetInstance().gameState.glideUnlocked)
		{
			audio->PlayFx(attackFx);
			isAttacking = true;
			if (lookingRight) {
				anims.SetCurrent("attack_right");
				anims.GetAnim("attack_right")->SetLoop(false);
			}
			else {
				anims.SetCurrent("attack_left");
				anims.GetAnim("attack_left")->SetLoop(false);
			}
			currentAnimPriority = 4;
			currentAttackTime = 0.0f;
			timeSinceLastAttack = 0.0f;

			if (comboStep == 0) {
				damage = 10;
				currentAttackWidth = 60;
				currentAttackHeight = 64;
			}
			else {
				damage = 15;
				currentAttackWidth = 120;
				currentAttackHeight = 90;
			}
			currentAttackOffsetX = texW / 2 + currentAttackWidth / 2;
			comboStep = (comboStep + 1) % 2;

			int offsetX = lookingRight ? currentAttackOffsetX : -currentAttackOffsetX;
			attackCollider = physics->CreateRectangleSensor(position.getX() + offsetX, position.getY(), currentAttackWidth, currentAttackHeight, bodyType::KINEMATIC);
			attackCollider->ctype = ColliderType::PLAYER_ATTACK;
			attackCollider->listener = this;
		}
		else
		{
			auto hud = Engine::GetInstance().hud;
			if (!GameManager::GetInstance().gameState.hasSickle && !GameManager::GetInstance().gameState.glideUnlocked) hud->ShowNotification("You need to find the Sickle and the Cape.");
			else if (!GameManager::GetInstance().gameState.hasSickle) hud->ShowNotification("You need to find the Sickle.");
			else if (!GameManager::GetInstance().gameState.glideUnlocked) hud->ShowNotification("You need to find the Cape.");
		}
	}

	if (isAttacking)
	{
		currentAttackTime += dt / 1000.0f;
		if (attackCollider != nullptr) {
			int attackOffsetX = lookingRight ? currentAttackOffsetX : -currentAttackOffsetX;
			attackCollider->SetPosition(position.getX() + attackOffsetX, position.getY());
		}

		if (currentAttackTime >= attackDuration &&
			(anims.GetAnim("attack_right")->HasFinishedOnce() || anims.GetAnim("attack_left")->HasFinishedOnce()))
		{
			isAttacking = false;
			anims.SetCurrent("idle");
			currentAnimPriority = 0;
			if (attackCollider != nullptr) {
				physics->DeletePhysBody(attackCollider);
				attackCollider = nullptr;
			}
		}
	}
}

void Player::Glide()
{
	if (GameManager::GetInstance().gameState.glideUnlocked)
	{
		auto input = Engine::GetInstance().input;
		auto audio = Engine::GetInstance().audio;

		if (onAir && !onGround && input->GetKey(SDL_SCANCODE_LSHIFT) == KEY_REPEAT)
		{
			if (!isGliding) audio->PlayFx(planearPrincesa);
			isGliding = true;
			anims.SetCurrent(lookingRight ? "glide_right" : "glide_left");
			currentAnimPriority = 5;
		}
		else if (input->GetKey(SDL_SCANCODE_LSHIFT) == KEY_UP && isGliding || onGround)
		{
			isGliding = false;
		}
	}
}

void Player::Dash()
{
	auto input = Engine::GetInstance().input;
	auto audio = Engine::GetInstance().audio;

	if (dashUnlocked && input->GetKey(SDL_SCANCODE_LCTRL) == KEY_DOWN && !isDashing && dashCooldownTimer.ReadMSec() > dashCooldownMS)
	{
		velocity.x = lookingRight ? dashForce : -dashForce;
		audio->PlayFx(dashPrincesa);
		isDashing = true;
		dashTimer.Start();
	}

	if (isDashing)
	{
		velocity.x = lookingRight ? dashForce : -dashForce;
		if (dashTimer.ReadMSec() > dashDurationMS) {
			isDashing = false;
			dashCooldownTimer.Start();
		}
	}
}

void Player::Interact()
{
	if (canInteract && interactuableBody != nullptr)
	{
		auto engine = &Engine::GetInstance();
		auto input = engine->input;
		auto map = engine->map;
		auto audio = engine->audio;
		auto hud = engine->hud;

		if (interactuableBody->ctype == ColliderType::DOOR && input->GetKey(SDL_SCANCODE_W) == KEY_DOWN)
		{
			if (map->DoorUnderMaintenance(interactuableBody)) {
				audio->PlayFx(pickItemFx);
				hud->ShowNotification("The room is under maintenance. You cannot enter.");
				return;
			}
			if (map->DoorClosed(interactuableBody)) {
				audio->PlayFx(pickItemFx);
				hud->ShowNotification("The room is closed. You cannot enter.");
				return;
			}

			if (map->DoorNeedsKey(interactuableBody))
			{
				if (GameManager::GetInstance().gameState.keyCount > 0) {
					audio->PlayFx(openDoor);
					GameManager::GetInstance().gameState.keyCount--;
					std::string doorId = map->GetDoorUniqueId(interactuableBody);
					if (!doorId.empty()) GameManager::GetInstance().gameState.openedDoors.push_back(doorId);
					engine->sceneManager->setNewMap = true;
				}
				else {
					audio->PlayFx(closedDoor);
					hud->ShowNotification("You need a key to open this door.");
				}
			}
			else {
				engine->sceneManager->setNewMap = true;
			}
		}
	}
}

void Player::ApplyPhysics() {
	auto physics = Engine::GetInstance().physics;
	if (isJumping || secondJumpUsed) velocity.y = physics->GetYVelocity(pbody);

	if (isGliding && velocity.y >= 1) velocity.y = 1;

	if (velocity.y > 5 && currentAnimPriority != 3)
	{
		anims.SetCurrent(lookingRight ? "fall_right" : "fall_left");
		currentAnimPriority = 3;
	}
	physics->SetLinearVelocity(pbody, velocity);
}

void Player::Draw(float dt)
{
	auto engine = &Engine::GetInstance();
	if (!engine->sceneManager->isGamePaused) anims.Update(dt);

	const SDL_Rect& animFrame = anims.GetCurrentFrame();
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	engine->render->DrawRotatedTexture(texture, x, y - animFrame.h / 3, &animFrame, SDL_FLIP_NONE, 1.25f);

	if (isAttacking && attackCollider != nullptr)
	{
		int ax, ay;
		attackCollider->GetPosition(ax, ay);
		SDL_Rect attackRect = { ax - (currentAttackWidth / 2), ay - (currentAttackHeight / 2), currentAttackWidth, currentAttackHeight };
		if (currentAttackWidth == 60) engine->render->DrawRectangle(attackRect, 255, 0, 0, 150);
		else engine->render->DrawRectangle(attackRect, 0, 150, 255, 150);
	}
}

void Player::CameraFollows()
{
	auto engine = &Engine::GetInstance();
	auto map = engine->map;
	auto render = engine->render;

	Vector2D mapSize = map->GetMapSizeInPixels();
	int screenW = render->camera.w;
	int screenH = render->camera.h;
	float dt = engine->GetDt();

	static float lastGroundY = position.getY();
	if (onGround) lastGroundY = position.getY();

	Vector2D targetCamPos = position;
	if (!onGround) {
		float maxCameraUpward = 40.0f;
		if (targetCamPos.getY() < lastGroundY - maxCameraUpward) targetCamPos.setY(lastGroundY - maxCameraUpward);
	}

	cameraController.Update(dt, targetCamPos, screenW, screenH, mapSize.getX(), mapSize.getY());
	float camX, camY;
	cameraController.GetCameraPosition(camX, camY);

	float targetCamX = -position.getX() + (screenW / 2.0f);
	if (targetCamX > 0) targetCamX = 0;
	float minCamX = -(mapSize.getX() - screenW);
	if (targetCamX < minCamX) targetCamX = minCamX;

	float dtSeconds = dt / 1000.0f;
	float currentCamX_f = render->camera.x;
	if (dtSeconds > 0.0f) {
		float lerpX = 8.0f * dtSeconds;
		if (lerpX > 1.0f) lerpX = 1.0f;
		currentCamX_f += (targetCamX - currentCamX_f) * lerpX;
	}

	render->camera.x = (int)currentCamX_f;
	render->camera.y = (int)camY;
}
std::unordered_map<int, std::string> Player::GetAliases(std::string name)
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
	auto textures = Engine::GetInstance().textures;
	textures->UnLoad(texture);
	std::unordered_map<int, std::string> aliases = GetAliases("cape");
	anims.LoadFromTSX("Assets/Textures/Entities/Princess/Princess.tsx", aliases);
	anims.SetCurrent("idle_right");
	texture = textures->Load("Assets/Textures/Entities/Princess/Princess.png");
	GameManager::GetInstance().gameState.glideUnlocked = true;
	AddItem(ItemID::GLIDE, 1);
}

void Player::UnlockSickle()
{
	GameManager::GetInstance().gameState.hasSickle = true;
	AddItem(ItemID::WEAPON, 1);
}

bool Player::CleanUp()
{
	auto physics = Engine::GetInstance().physics;
	auto textures = Engine::GetInstance().textures;

	if (pbody != nullptr) {
		pbody->listener = nullptr;
		physics->DeletePhysBody(pbody);
		pbody = nullptr;
	}
	textures->UnLoad(texture);
	if (attackCollider != nullptr) {
		physics->DeletePhysBody(attackCollider);
		attackCollider = nullptr;
	}
	return true;
}

void Player::AddItem(ItemID id, int amount) {
	inventory[id] += amount;
}

bool Player::HasItem(ItemID id) {
	return inventory[id] > 0;
}

int Player::GetItemCount(ItemID id) {
	return inventory[id];
}

void Player::OnCollision(PhysBody* physA, PhysBody* physB) {
	if (physA == attackCollider) return;

	auto engine = &Engine::GetInstance();
	auto audio = engine->audio;
	auto physics = engine->physics;
	auto hud = engine->hud;
	auto map = engine->map;

	switch (physB->ctype)
	{
	case ColliderType::DANGER:
		if (!godMode && !isdead) RespawnFromVoid();
		break;
	case ColliderType::GROUND:
		isJumping = false;
		secondJumpUsed = false;
		if (currentAnimPriority > 1) {
			anims.SetCurrent(lookingRight ? "idle_right" : "idle_left");
			currentAnimPriority = 0;
		}
		onGround = true;
		break;
	case ColliderType::WALL:
		isJumping = false;
		secondJumpUsed = false;
		onWall = true;
		break;
	case ColliderType::DOOR:
		canInteract = true;
		interactuableBody = physB;
		break;
	case ColliderType::PATH:
		interactuableBody = physB;
		engine->sceneManager->setNewMap = true;
		break;
	case ColliderType::ITEM:
		if (physB->listener->name == "Manta") hud->ShowNotification("You have obtained the Cape.");
		else if (physB->listener->name == "Key") {
			GameManager::GetInstance().gameState.keyCount++;
			AddItem(ItemID::KEY, 1);
			hud->ShowNotification("You have obtained a Key.");
		}
		else if (physB->listener->name == "Sickle") hud->ShowNotification("You have obtained the Sickle.");
		audio->PlayFx(pickItemFx);
		physB->listener->Destroy();
		break;
	case ColliderType::HEALTH_ORB:
		if (currentHealth < maxHealth) {
			currentHealth = min(maxHealth, currentHealth + 50);
			audio->PlayFx(orbFx);
			physB->listener->Destroy();
			hud->ShowNotification("You have recovered your health.");
		}
		break;
	case ColliderType::SKILL_POINT_ORB:
		currentForceOrbs++;
		AddItem(ItemID::STRENGTH_ORB, 1);
		audio->PlayFx(orbFx);
		physB->listener->Destroy();
		hud->ShowNotification("You have obtained an Orb of Power.");
		break;
	case ColliderType::SAVEPOINT:
	{
		SavePoint* sp = (SavePoint*)physB->listener;
		audio->PlayFx(savePointFx);
		sp->Activate();
		int spX, spY;
		physB->GetPosition(spX, spY);
		respawnPosition = Vector2D((float)spX, (float)spY);
		auto& gameState = GameManager::GetInstance().gameState;
		gameState.playerPosition = respawnPosition;
		gameState.currentHealth = this->currentHealth;
		gameState.currentMap = map->mapFileName;
		if (GameManager::GetInstance().SaveGame("savegame.xml")) hud->ShowNotification("Partida Guardada");
		else hud->ShowNotification("Error al guardar partida");
		break;
	}
	case ColliderType::ENEMY:
		audio->PlayFx(recibirDamage);
		TakeDamage(10);
		isKnockedback = true;
		break;
	case ColliderType::ENEMY_ATTACK:
		audio->PlayFx(recibirDamage);
		TakeDamage(physB->listener->damage);
		isKnockedback = true;
		break;
	default: break;
	}
}

void Player::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
	switch (physB->ctype)
	{
	case ColliderType::WALL: onAir = true; onWall = false; break;
	case ColliderType::GROUND: onGround = false; onAir = true; break;
	case ColliderType::DOOR: canInteract = false; interactuableBody = nullptr; break;
	default: break;
	}
}

Vector2D Player::GetPosition()
{
	int x, y;
	pbody->GetPosition(x, y);
	return Vector2D((float)x, (float)y);
}

void Player::SetPosition(Vector2D pos)
{
	position = pos;
	if (pbody != nullptr) {
		pbody->SetPosition((int)(pos.getX() + texW / 2), (int)(pos.getY() + texH / 2));
	}
}

void Player::DevTools(float dt)
{
	auto input = Engine::GetInstance().input;
	auto physics = Engine::GetInstance().physics;

	if (input->GetKey(SDL_SCANCODE_T) == KEY_DOWN) pbody->SetPosition(96, 96);

	if (input->GetKey(SDL_SCANCODE_R) == KEY_DOWN) {
		pbody->SetPosition(respawnPosition.getX(), respawnPosition.getY());
		physics->SetLinearVelocity(pbody, { 0.0f,0.0f });
	}

	if (input->GetKey(SDL_SCANCODE_F10) == KEY_DOWN) {
		physics->SetBodyType(pbody, godMode ? bodyType::DYNAMIC : bodyType::KINEMATIC);
		godMode = !godMode;
	}

	if (input->GetKey(SDL_SCANCODE_P) == KEY_DOWN) currentForceOrbs++;

	if (input->GetKey(SDL_SCANCODE_1) == KEY_DOWN && currentForceOrbs > 0)
	{
		if (!OffensiveSkills[2]) {
			if (OffensiveSkills[1]) OffensiveSkills[2] = true;
			else if (OffensiveSkills[0]) OffensiveSkills[1] = true;
			else OffensiveSkills[0] = true;
			currentForceOrbs--;
		}
	}

	if (godMode) GodModeMove(dt);

	if (input->GetKey(SDL_SCANCODE_9) == KEY_DOWN) {
		UnlockCape();
		GameManager::GetInstance().gameState.hasSickle = true;
	}
}

void Player::GodModeMove(float dt)
{
	auto input = Engine::GetInstance().input;
	b2Vec2 godVelocity = { 0.0f, 0.0f };
	float godSpeed = speed * 2.0f;
	if (input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT) godVelocity.x = -godSpeed;
	if (input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT) godVelocity.x = godSpeed;
	if (input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT) godVelocity.y = -godSpeed;
	if (input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT) godVelocity.y = godSpeed;
	Engine::GetInstance().physics->SetLinearVelocity(pbody, godVelocity);
}