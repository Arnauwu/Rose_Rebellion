#pragma once

#include "Entity.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <unordered_map>
#include <SDL3/SDL.h>
#include "Timer.h"
#include <iostream>
#include "CameraController.h"


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
private:

	void GetPhysicsValues();
	void Move();
	void Jump(float dt);
	void Attack(float dt);
	void Glide();
	void Dash();

	void Interact();

	void ApplyPhysics();
	void Draw(float dt);

	void CameraFollows();


	// DevTools / Debug
	void Teleport();

public:

	int health; 
	float speed = 4.0f;

	int currentForceOrbs = 0;

	// Texture
	SDL_Texture* texture = nullptr;
	int texW, texH;
	bool lookingRight = true; //False Left -- True Right 
	
	// Physics
	PhysBody* pbody = nullptr;
	
	// Ground
	bool onGround = false;
	
	// Air
	bool onAir = false;

	// Wall
	bool onWall = false;

	// Jump
	float jumpForce = -7.5f; // The force to apply when jumping
	bool isJumping = false; // Flag to check if the player is currently jumping

	// Extra Jump Force
	bool isJumpKeyDown = false; // Check if jump key is being pressed
	float jumpHoldTime = 0.00f; //How long has the jump key being pressed (in seconds)
	const float maxJumpHoldTime = 0.25f; // Max Time to apply extra jump force (in seconds)

	// Double Jump
	bool doubleJumpUnlocked = true; // TO DO: Change to false
	bool secondJumpUsed = false;

	//Attack
	bool isAttacking = false;
	float attackDuration = 0.25f; //attack duration
	float currentAttackTime = 0.0f;
	
	// Gliding
	bool glideUnlocked = true; // TO DO: Change to false
	bool isGliding = false; // Flag

	// Dash
	bool dashUnlocked = true;
	float dashForce = 150.0f;
	bool hasDashed = false; // Flag to check if the player has dashed

	// Skills
	bool OffensiveSkills[3] = { false, false, false };
	bool DefensiveSkills[3] = { false, false, false };
	bool UtilitySkills[3] = { false, false, false };

	// Interact
	bool canInteract = false;
	PhysBody* interactuableBody = nullptr;

	//Audio fx
	int pickCoinFxId;

private: 
	PhysBody* attackCollider = nullptr;
	b2Vec2 velocity;
	AnimationSet anims;
	CameraController cameraController;

};