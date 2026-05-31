#pragma once
#include "Entity.h"
#include "Animation.h"
#include "Vector2D.h"
#include "Keys.h"

struct SDL_Texture;

class DoorEntity : public Entity
{
public:
	DoorEntity();
	virtual ~DoorEntity();

	bool Awake() override;
	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;

	void OpenDoorAt(Vector2D pos, int width, int height);

private:
	SDL_Texture* texture = nullptr;
	AnimationSet anims;
	bool isOpening = false;

	int doorW = 256;
	int doorH = 256;

	//Margen
	const float VISIBLE_HEIGHT = 256.0f;

	const float VISIBLE_WIDTH = 175.0f;

	const float Y_OFFSET = 0.0f;
};