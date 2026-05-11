#pragma once
#include "Entity.h"
#include "Animation.h"
#include "Vector2D.h"

struct SDL_Texture;

class Door : public Entity
{
public:
	Door();
	virtual ~Door();

	bool Awake() override;
	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;

	void OpenDoorAt(Vector2D pos);

private:
	SDL_Texture* texture = nullptr;
	Animation openAnim;
	bool isOpening = false;

	float doorScale = 3.0f;
	int drawYOffset = -100;
};