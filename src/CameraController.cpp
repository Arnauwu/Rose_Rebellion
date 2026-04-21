#include "CameraController.h"

CameraController::CameraController()
	: targetX(0.0f), targetY(0.0f), currentX(0.0f), currentY(0.0f),
	  smoothSpeed(0.50f), verticalOffset(-25.0f)
{
}

CameraController::~CameraController()
{
}

void CameraController::Update(float dt, Vector2D playerPos, int screenW, int screenH, float mapWidth, float mapHeight)
{
	// Calcular posición objetivo: centrar en el jugador con offset vertical mínimo
	targetX = -playerPos.getX() + (screenW / 2.0f);
	targetY = -playerPos.getY() + (screenH / 1.25f) - verticalOffset;

	// Limitar la cámara dentro de los bordes del mapa
	ClampToMapBounds(currentX, currentY, screenW, screenH, mapWidth, mapHeight);

	// Suavizar movimiento de la cámara usando interpolación lineal
	currentX += (targetX - currentX) * smoothSpeed;
	currentY += (targetY - currentY) * smoothSpeed;

	ClampToMapBounds(currentX, currentY, screenW, screenH, mapWidth, mapHeight);
}

void CameraController::GetCameraPosition(float& outX, float& outY) const
{
	outX = currentX;
	outY = currentY;
}

void CameraController::ClampToMapBounds(float& x, float& y, int screenW, int screenH, float mapWidth, float mapHeight)
{
	// Límites horizontales
	if (x > 0)
		x = 0;
	if (x < -(mapWidth - screenW))
		x = -(mapWidth - screenW);

	// Límites verticales
	if (y > 0)
		y = 0;
	if (y < -(mapHeight - screenH))
		y = -(mapHeight - screenH);
}