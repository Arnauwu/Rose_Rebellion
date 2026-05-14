#pragma once

#include "Module.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_rect.h>
#include "Vector2D.h"

#define NUM_MOUSE_BUTTONS 5

enum EventWindow
{
	WE_QUIT = 0,
	WE_HIDE = 1,
	WE_SHOW = 2,
	WE_COUNT
};

enum KeyState
{
	KEY_IDLE = 0,
	KEY_DOWN,
	KEY_REPEAT,
	KEY_UP
};

enum GamepadButton
{
	GAMEPAD_A = 0,
	GAMEPAD_B = 1,
	GAMEPAD_X = 2,
	GAMEPAD_Y = 3,
	GAMEPAD_LB = 4,
	GAMEPAD_RB = 5,
	GAMEPAD_BACK = 6,
	GAMEPAD_START = 7,
	GAMEPAD_LSTICK = 8,
	GAMEPAD_RSTICK = 9,
	GAMEPAD_GUIDE = 10,
	GAMEPAD_COUNT = 11
};

enum GamepadAxis
{
	GAMEPAD_AXIS_LSTICK_X = 0,
	GAMEPAD_AXIS_LSTICK_Y = 1,
	GAMEPAD_AXIS_RSTICK_X = 2,
	GAMEPAD_AXIS_RSTICK_Y = 3,
	GAMEPAD_AXIS_LT = 4,
	GAMEPAD_AXIS_RT = 5,
	GAMEPAD_AXIS_COUNT = 6
};

class Input : public Module
{

public:

	Input();

	// Destructor
	virtual ~Input();

	// Called before render is available
	bool Awake();

	// Called before the first frame
	bool Start();

	// Called each loop iteration
	bool PreUpdate();

	// Called before quitting
	bool CleanUp();

	// Check key states (includes mouse and joy buttons)
	KeyState GetKey(int id) const
	{
		return keyboard[id];
	}

	KeyState GetMouseButtonDown(int id) const
	{
		return mouseButtons[id - 1];
	}

	void ClearMouseInput();

	// Gamepad input methods
	KeyState GetGamepadButton(GamepadButton button) const;
	float GetGamepadAxis(GamepadAxis axis) const;
	bool IsGamepadConnected() const { return gamepadConnected; }

	// Check if a certain window event happened
	bool GetWindowEvent(EventWindow ev);

	// Get mouse / axis position
	Vector2D GetMousePosition();
	Vector2D GetMouseMotion();

private:
	bool windowEvents[WE_COUNT];
	KeyState* keyboard;
	KeyState mouseButtons[NUM_MOUSE_BUTTONS];
	int	mouseMotionX;
	int mouseMotionY;
	int mouseX;
	int mouseY;

	// Gamepad
	bool gamepadConnected = false;
	SDL_Gamepad* gamepad = nullptr;
	KeyState gamepadButtons[GAMEPAD_COUNT];
	float gamepadAxes[GAMEPAD_AXIS_COUNT];

	void UpdateGamepadState();
	void UpdateGamepadButtons();
};