#pragma once

#include "Vector2D.h"

class CameraController
{
public:
	CameraController();
	~CameraController();

	void Update(float dt, Vector2D playerPos, int screenW, int screenH, float mapWidth, float mapHeight);

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

	void ClampToMapBounds(float& x, float& y, int screenW, int screenH, float mapWidth, float mapHeight);
};