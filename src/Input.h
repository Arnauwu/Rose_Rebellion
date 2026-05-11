#pragma once

#include "Module.h"
#include "Vector2D.h"
#include <SDL3/SDL.h>

#define NUM_MOUSE_BUTTONS 5

enum EventWindow
{
	WE_QUIT = 0,
	WE_HIDE = 1,
	WE_SHOW = 2,
	WE_COUNT = 3
};

enum KeyState
{
	KEY_IDLE = 0,
	KEY_DOWN = 1,
	KEY_REPEAT = 2,
	KEY_UP = 3
};

class Input : public Module
{
public:
	Input();
	virtual ~Input();

	bool Awake();
	bool Start();
	bool PreUpdate();
	bool CleanUp();

	bool GetWindowEvent(EventWindow ev);
	
	// Teclado
	KeyState GetKey(SDL_Scancode key) const;
	
	// Ratón
	KeyState GetMouseButtonDown(int button) const;
	Vector2D GetMousePosition();
	Vector2D GetMouseMotion();
	void ClearMouseInput();

	// Mando/Gamepad
	bool IsGamepadConnected() const;
	KeyState GetGamepadButton(SDL_GamepadButton button) const;
	float GetGamepadLeftStickX() const;
	float GetGamepadLeftStickY() const;
	float GetGamepadRightStickX() const;
	float GetGamepadRightStickY() const;

private:
	KeyState* keyboard;
	KeyState mouseButtons[NUM_MOUSE_BUTTONS];
	bool windowEvents[WE_COUNT];
	int mouseMotionX, mouseMotionY;
	int mouseX, mouseY;

	SDL_Gamepad* gamepad;
	bool controllerConnected;
	KeyState controllerButtons[SDL_GAMEPAD_BUTTON_COUNT];
	float controllerLeftStickX;
	float controllerLeftStickY;
	float controllerRightStickX;
	float controllerRightStickY;
};