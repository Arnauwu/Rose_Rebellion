#pragma once

#include "Module.h"
#include "UIElement.h"

#include <list>

class UIManager : public Module
{
public:

	// Constructor
	UIManager();

	// Destructor
	virtual ~UIManager();

	// Called before the first frame
	bool Start();

	// Called each loop iteration
	bool Update(float dt);

	//Called after all Updates
	bool PostUpdate() override;
	// Called before quitting
	bool CleanUp();

	void Draw() const;

	// Additional methods
	std::shared_ptr<UIElement> CreateUIElement(UIElementType type, int id, const char* text, float anchorX, float anchorY, float wPerc, float hPerc, Module* observer);

public:

	std::list<std::shared_ptr<UIElement>> UIElementsList;
	SDL_Texture* texture;

	//List to store entities pending deletion
	std::list<std::shared_ptr<UIElement>> pendingDelete;
};
