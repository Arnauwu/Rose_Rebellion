#pragma once

#include "Entity.h"
#include "Animation.h"
#include <string>

class BreakableRock : public Entity
{
public:
	BreakableRock();
	virtual ~BreakableRock();

	bool Awake() override;
	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;

	void OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) override;

	void SetSize(int width, int height);

private:
	void ApplyHitFeedback();
	bool CheckIfDestroyed();
	void RemoveCollider();
	void ResetPlayerWallState();
	void SetDestroyed();
	void SetDamageFrame();

private:
	AnimationSet anims;
	SDL_Texture* texture = nullptr;
	PhysBody* pbody = nullptr;

	int width = 0;
	int height = 0;
	int hitsTaken = 0;
	int maxHits = 4;
	std::string uniqueID;
	Vector2D uniqueIDPosition;
	bool hasUniqueIDPosition = false;

	float shakeTimer = 0.0f;
	float shakeDuration = 140.0f;
	int shakeIntensity = 5;
};
