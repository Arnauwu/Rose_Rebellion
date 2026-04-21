#pragma once

// Quitamos #include "Engine.h" y #include "Window.h" para evitar la dependencia circular
#include "Input.h"
#include "Render.h"
#include "Module.h"
#include "Vector2D.h"
#include <string>
#include <memory>
#include "SDL3/SDL.h"

enum class UIElementType
{
	BUTTON, TOGGLE, CHECKBOX, SLIDER, SLIDERBAR, COMBOBOX, DROPDOWNBOX, INPUTBOX, VALUEBOX, SPINNER
};

enum class UIElementState
{
	DISABLED, NORMAL, FOCUSED, PRESSED, SELECTED
};

class UIElement : public std::enable_shared_from_this<UIElement>
{
public:
	bool visible = true;

	UIElement() {}

	UIElement(UIElementType type, int id) : type(type), id(id), state(UIElementState::NORMAL) {}

	// DECLARACIÓN: Solo definimos qué variables recibe, la lógica va al .cpp
	UIElement(UIElementType type, int id, float anchorX, float anchorY, float wPercent, float hPercent, const char* text = "");

	// DECLARACIONES:
	void RecalculateBounds();
	virtual bool Update(float dt);

	virtual void Draw() const {}

	void SetTexture(SDL_Texture* tex)
	{
		texture = tex;
		section = { 0, 0, 0, 0 };
	}

	void SetBgTexture(SDL_Texture* tex) {
		bgTexture = tex;
	}

	void SetPivot(float pX, float pY) {
		pivotX = pX;
		pivotY = pY;
		RecalculateBounds();
	}

	void SetObserver(Module* module)
	{
		observer = module;
	}

	void NotifyObserver()
	{
		observer->OnUIMouseClickEvent(this);
	}

	virtual bool CleanUp() { return true; }
	virtual bool Destroy() { return true; }

public:
	int id;
	UIElementType type;
	UIElementState state;

	std::string text;
	SDL_Rect bounds;
	SDL_Color color;

	// Position and relative size variables
	float anchorX = 0.5f;
	float anchorY = 0.5f;

	//Pivot variables(0.0 is left / up)
	float pivotX = 0.5f;
	float pivotY = 0.5f;

	float relW = 0.1f;
	float relH = 0.1f;

	SDL_Texture* texture;
	SDL_Rect section;
	SDL_Texture* bgTexture = nullptr;

	Module* observer = nullptr;
	bool pendingToDelete = false;
};