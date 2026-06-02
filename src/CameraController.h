#pragma once

#include "Vector2D.h"

class CameraController
{
public:
	CameraController();
	~CameraController();

	void Update(float dt, Vector2D entityPos);

	void StartShake(float duration, float intensity);
	float GetRemainingShakeTime() { return shakeTimeRemaining; }

	void GetCameraPosition(float& outX, float& outY) const;

	void SetSmoothSpeed(float speed) { smoothSpeed = speed; }
	void SetVerticalOffset(float offset) { verticalOffset = offset; }
	void SetYDivisor(float divisor) { yDivisor = divisor; }

private:
	float targetX, targetY;		  // Target position
	float currentX, currentY;     // Actual position
	float smoothSpeed;            // Smooth speed
	float verticalOffset;         // Vertical Offset 
	float yDivisor = 1.75f;       // Camara mode

	//Shake
	float shakeDuration = 0.0f; // In MS
	float shakeTimeRemaining = 0.0f;
	float shakeIntensity = 0.0f;

	
	void ClampToMapBounds(float& x, float& y, int screenW, int screenH, float mapWidth, float mapHeight);
};