#pragma once
#include "Item.h"

class DashObj :public Item {
public:
	DashObj();
	virtual ~DashObj();

	bool Awake() override;
	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;

	void OnCollision(PhysBody* physA, PhysBody* physB);

private:
	SDL_Texture* texture = nullptr;
	PhysBody* pbody = nullptr;

	bool isPicked = false;
};