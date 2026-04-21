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

enum class ItemID {
	WEAPON,
	GLIDE,
	KEY,
	STRENGTH_ORB,
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
	void OnCollision(PhysBody* physA, PhysBody* physB);
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB);

	Vector2D GetPosition();
	void SetPosition(Vector2D pos);
	
	// Unlocks
	void UnlockCape();

	//Inventary Variables
	std::map<ItemID, int> inventory;

	// Las herramientas para modificar el inventario
	void AddItem(ItemID id, int amount = 1);
	bool HasItem(ItemID id);
	int GetItemCount(ItemID id);
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

	//// Helpers
	std::unordered_map<int, std::string> GetAliases(std::string name);

	// DevTools / Debug
	void DevTools(float dt);

public:
	float speed = 6.0f;

	// Texture
	SDL_Texture* texture = nullptr;
	int texW, texH;

	/*--- PLAYER VARIABLES ---*/
	// Physics
	PhysBody* pbody = nullptr;
	float knockbackForce;
	bool isKnockedback = false;
	float knockbackTime = 500.0f;


	/*--- PLAYER STATES INFO --- */
	// Ground
	bool onGround = false;
	// Air
	bool onAir = false;
	// Wall
	bool onWall = false;



	/*--- PLAYER SKILLS --- */
	// Jump
	float jumpForce = -12.0f; // The force to apply when jumping
	bool isJumping = false; // Flag to check if the player is currently jumping

	// Extra Jump Force
	bool isJumpKeyDown = false; // Check if jump key is being pressed
	float jumpHoldTime = 0.00f; //How long has the jump key being pressed (in seconds)
	const float maxJumpHoldTime = 0.25f; // Max Time to apply extra jump force (in seconds)

	// Double Jump
	bool doubleJumpUnlocked = false; 
	bool secondJumpUsed = false;
	
	// Gliding
	static bool glideUnlocked; 
	bool isGliding = false; // Flag

	// Dash
	bool dashUnlocked = false; 
	bool isDashing = false; // Flag to check if the player has dashed
	float dashForce = 15.0f;

	Timer dashTimer;
	float dashDurationMS = 300;

	Timer dashCooldownTimer;
	float dashCooldownMS = 300;


	/*--- PLAYER SKILL TREE --- */
	int currentForceOrbs = 0;

	bool OffensiveSkills[3] = { false, false, false };
	bool DefensiveSkills[3] = { false, false, false };
	bool UtilitySkills[3] = { false, false, false };

	// Interact
	bool canInteract = false;
	PhysBody* interactuableBody = nullptr;

	//Attack
	bool isAttacking = false;
	float attackDuration = 0.25f; //attack duration
	float currentAttackTime = 0.0f;

	int comboStep = 0;                 // combo
	float timeSinceLastAttack = 0.0f;  // Temporizador
	const float comboResetTime = 3.0f; // Reset combo

	// Save attack properties
	int currentAttackWidth = 0;
	int currentAttackHeight = 0;
	int currentAttackOffsetX = 0;

	//Audio fx
	//int pickCoinFxId;

	// Last Postion
	Vector2D lastSafePosition;
	float safePositionTimer = 0.0f;
	const float safePositionInterval = 0.2f;

	//Item
	static int keyCount;
private: 

	int jumpFx = -1;
	int attackFx = -1;
	int pickItemFx = -1;

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
		Glide = 5

		Death = 99
	*/

	CameraController cameraController;
	Vector2D respawnPosition;

};
