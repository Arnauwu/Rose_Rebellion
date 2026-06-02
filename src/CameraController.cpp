#include "CameraController.h"
#include "Engine.h"
#include "Render.h"
#include "Map.h"

CameraController::CameraController()
	: targetX(0.0f), targetY(0.0f), currentX(0.0f), currentY(0.0f),
	  smoothSpeed(0.25f), verticalOffset(-25.0f)
{
}

CameraController::~CameraController()
{
}

void CameraController::Update(float dt, Vector2D entityPos)
{
	int screenW = Engine::GetInstance().render->camera.w;
	int screenH = Engine::GetInstance().render->camera.h;
	Vector2D mapSize = Engine::GetInstance().map->GetMapSizeInPixels();
	float mapWidth = mapSize.getX();
	float mapHeight = mapSize.getY();

	float dtSeconds = dt / 1000.0f;

	// Calcular posicion objetivo: centrar en el jugador con offset vertical minimo
	targetX = -entityPos.getX() + (screenW / 2.0f);
	targetY = -entityPos.getY() + (screenH / yDivisor) - verticalOffset;

	// Limitar la camara dentro de los bordes del mapa
	ClampToMapBounds(currentX, currentY, screenW, screenH, mapWidth, mapHeight);
	
	// Suavizar movimiento de la camara usando interpolacion lineal
	currentX += (targetX - currentX) * smoothSpeed;
	currentY += (targetY - currentY) * (smoothSpeed / 2);

	ClampToMapBounds(currentX, currentY, screenW, screenH, mapWidth, mapHeight);

	if (shakeTimeRemaining > 0) // If Shaking
	{
		shakeTimeRemaining -= dt; // Reduce Timer

		float strength = shakeIntensity * (shakeTimeRemaining / shakeDuration); //Adjust Strengh/Intensity over time

		float shakeX = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * strength; //Random Function (rand() / RAND_MAX gives a float between 0.0f, 1.0f)
		float shakeY = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * strength;

		Engine::GetInstance().render->camera.x = (int)currentX + shakeX;
		Engine::GetInstance().render->camera.y = (int)currentY + shakeY;
	}
	else
	{
		// Lógica del Eje X (Común para ambos modos)
		float targetCamX = -entityPos.getX() + (screenW / 2.0f);
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
		Engine::GetInstance().render->camera.y = (int)currentY;
	}



}

void CameraController::StartShake(float duration, float intensity)
{
	shakeDuration = duration;
	shakeTimeRemaining = duration;
	shakeIntensity = intensity;
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