#pragma once

#include "Entity.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <unordered_map>
#include <SDL3/SDL.h>
#include "Timer.h"
#include "CameraController.h"
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <string>
#include "Hud.h"
#include "Keys.h"

enum class ItemID {
	WEAPON,
	GLIDE,
	KEY,
	STRENGTH_ORB,
	DASH_OBJ,
	DOUBLEJUMP_OBJ,
	WALLJUMP_OBJ

};

enum class CameraMode {
	CLASSIC,  // Para la fortaleza (Mtodo original: 1.25f, bloqueo de salto Y)
	DYNAMIC   // Para exploracin (Nuevo mtodo: 1.75f, Look down, anticipacin)
};

enum class SkillTree {
	HEALTH_UP,
	IFRAMES_UP,
	SPEED_UP,
	FAST_DASH,
	UP_ATTACK,
	DOWN_ATTACK
};

struct SDL_Texture;

class Player : public Entity
{
public:

	Player();

	virtual ~Player();

	bool Awake();

	bool Start();

	bool Update(float dt);
	bool PostUpdate();

	bool CleanUp();

	// OnCollision function for the player. 
	void OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB);
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB);

	Vector2D GetPosition();
	void SetPosition(Vector2D pos);

	// Unlocks
	void UnlockCape();
	void UnlockSickle();
	void UnlockDash();
	void UnlockDoubleJump();
	void UnlockWallJump();

	void UnlockSkill(SkillTree skill, int cost);

	//Inventary Variables
	std::map<ItemID, int> inventory;

	// Las herramientas para modificar el inventario
	void AddItem(ItemID id, int amount = 1);
	bool HasItem(ItemID id);
	int GetItemCount(ItemID id);

	// Select camara follow mode
	void SetCameraMode(CameraMode mode);
	CameraMode GetCameraMode() const { return currentCameraMode; }

	//Key
	void AddKey(KeyType type) { heldKeys.insert(type); }
	bool HasKey(KeyType type) const { return heldKeys.find(type) != heldKeys.end(); }
private:

	void GodModeMove(float dt);

	void GetPhysicsValues();
	void Move();
	void Knockback();
	void Respawn();
	void RespawnFromVoid();
	void Jump(float dt);
	void Attack(float dt);
	void Glide();
	void Dash();

	void Interact();

	void ApplyPhysics();
	void Draw(float dt);

	void CameraFollows();
	CameraMode currentCameraMode = CameraMode::DYNAMIC;

	//// Helpers
	std::unordered_map<int, std::string> GetAliases(std::string name);

	// DevTools / Debug
	void DevTools(float dt);

	//KeyType
	std::set<KeyType> heldKeys;

public:
	float speed = 10.0f;
	int dmgbuff = 0;
	int defbuff = 0;

	// Texture
	SDL_Texture* texture = nullptr;
	int texW, texH;

	/*--- PLAYER VARIABLES ---*/
	// Physics
	PhysBody* pbody = nullptr;
	float knockbackForce;
	bool isKnockedback = false;
	float knockbackTime = 500.0f;
	bool hitFromRight = false;


	/*--- PLAYER STATES INFO --- */
	// Ground
	bool onGround = false;
	// Air
	bool onAir = false;
	// Wall
	bool onWall = false;

	/*--- PLAYER SKILLS --- */
	// Jump
	float jumpForce = -18.0f; // The force to apply when jumping
	bool isJumping = false; // Flag to check if the player is currently jumping

	// Extra Jump Force
	bool isJumpKeyDown = false; // Check if jump key is being pressed
	float jumpHoldTime = 0.00f; //How long has the jump key being pressed (in seconds)
	const float maxJumpHoldTime = 0.25f; // Max Time to apply extra jump force (in seconds)

	// Double Jump
	bool secondJumpUsed = false;

	//Walljump
	bool isWallJumping = false;
	float wallJumpTimer = 0.0f;
	const float wallJumpDuration = 0.15f;
	int wallDirection = 0;

	// Gliding
	bool isGliding = false; // Flag

	// Dash
	bool isDashing = false; // Flag to check if the player has dashed
	float dashForce = 30.0f;

	Timer dashTimer;
	float dashDurationMS = 300;

	Timer dashCooldownTimer;
	float dashCooldownMS = 300;

	/*--- PLAYER SKILL TREE --- */
	int currentForceOrbs = 0;

	// Interact
	bool canInteract = false;
	PhysBody* interactuableBody = nullptr;

	//Attack
	bool isAttacking = false;
	float attackDuration = 0.25f; //attack duration
	float currentAttackTime = 0.0f;
	Timer attackCooldownTimer;
	int attackCooldownMS = 500;

	int comboStep = 0;                 // combo
	float timeSinceLastAttack = 0.0f;  // Temporizador
	const float comboResetTime = 3.0f; // Reset combo

	// Save attack properties
	int currentAttackWidth = 0;
	int currentAttackHeight = 0;
	int currentAttackOffsetX = 0;
	int currentAttackOffsetY = 0;

	// Last Postion
	Vector2D lastSafePosition;
	Timer safePositionTimer;
	const int safePositionInterval = 1000; //In Ms

	// Item & states
	bool isFrozen = false;

	/*--- INVINCIBILITY (I-FRAMES) --- */
	bool isInvincible = false;
	Timer invincibilityTimer;
	float invincibilityDurationMS = 1000.0f; 
	float flashIntervalMS = 100.0f;         
	bool isVisible = true;

private:

	int jumpFx;
	int attackFx;
	int dashPrincesa;
	int morirPrincesa;
	int planearPrincesa;
	int recibirDamage;
	int caminarPrincesa;

	int pickItemFx;
	int savePointFx;
	int openDoor;
	int closedDoor;
	int orbFx;
	int respawnFx;

	bool wasWalking = false;
	float stepTimer = 0.0f;
	float timeBetweenSteps = 15.5f;
	float footX = 0.0f;
	float footY = 0.0f;
	float lookDownTimer = 0.0f;
	float currentCameraYOffset = 0.0f;

	PhysBody* attackCollider = nullptr;
	b2Vec2 velocity;

	AnimationSet anims;
	int currentAnimPriority = 0;

	/*
	Idle = 0
	Move = 1
	Jump = 2
	Fall = 3
	Attack = 4
	Gliding = 5

	Death = 99
	*/

	CameraController cameraController;
	Vector2D respawnPosition;
};