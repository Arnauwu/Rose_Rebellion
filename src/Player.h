#pragma once

#include "Entity.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include "Timer.h"

struct SDL_Texture;

class Player : public Entity
{
public:

	Player();
	
	virtual ~Player();

	bool Awake();

	bool Start();

	bool Update(float dt);

	bool CleanUp();

	// OnCollision function for the player. 
	void OnCollision(PhysBody* physA, PhysBody* physB);
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB);

private:

	void GetPhysicsValues();
	void Move();
	void Jump(float dt);
	void Attack(float dt);
	void ApplyPhysics();
	void Draw(float dt);

	void CameraFollows();


	// DevTools / Debug
	void Teleport();

public:

	int health; 
	float speed = 4.0f;



	// Texture
	SDL_Texture* texture = NULL;
	int texW, texH;
	bool lookingRight = true; //False Left -- True Right 
	
	// Physics
	PhysBody* pbody;
	// Ground
	bool onGround = false;
	// Air
	bool onAir = false;
	// Wall
	bool onWall = false;

	// Jump
	float jumpForce = 2.5f; // The force to apply when jumping
	bool isJumping = false; // Flag to check if the player is currently jumping

	// Extra Jump Force
	bool isJumpKeyDown = false; // Check if jump key is being pressed
	float jumpHoldTime = 0.00f; //How long has the jump key being pressed (in seconds)
	const float maxJumpHoldTime = 0.25f; // Max Time to apply extra jump force (in seconds)

	// Double Jump
	bool doubleJumpUnlocked = true; // TO DO: Change to false
	bool secondJumpUsed = false;

	//Attack
	float AttackTimer = 0.0f;// Attack timer counter 
	bool Is_Attacking = false;// Check if the player is attacking or not
	float AttackDuration = 500.0f;// Maximum duration of each attack =0,5s
	//Audio fx
	int pickCoinFxId;

private: 
	b2Vec2 velocity;
	AnimationSet anims;

};