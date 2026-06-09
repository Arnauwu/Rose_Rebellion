#pragma once
#include "Entity.h"
#include "Animation.h"
#include "Vector2D.h"
#include "Keys.h"
#include <string>

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

	void OpenDoorAt(Vector2D pos, int width, int height, KeyType keyType);

private:
	SDL_Texture* texture = nullptr;
	AnimationSet anims;
	bool isOpening = false;
	std::string animationName = "open";

	int doorW = 256;
	int doorH = 256;
};
