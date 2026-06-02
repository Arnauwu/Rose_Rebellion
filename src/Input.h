#pragma once

#include "Module.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_rect.h>
#include "Vector2D.h"
#include <memory>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cfloat>

#define MAX_KEYS 300
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

// Forward declaration
class UIButton;

class Input : public Module
{
public:
	Input();
	virtual ~Input();

	// Lifecycle
	bool Awake();
	bool Start();
	bool PreUpdate();
	bool CleanUp();

	// Keyboard
	KeyState GetKey(int id) const;
	KeyState GetMouseButtonDown(int id) const;
	void ClearMouseInput();

	// Gamepad
	KeyState GetGamepadButton(GamepadButton button) const;
	float GetGamepadAxis(GamepadAxis axis) const;
	bool IsGamepadConnected() const { return gamepadConnected; }

	// Window events
	bool GetWindowEvent(EventWindow ev);

	// Mouse
	Vector2D GetMousePosition();
	Vector2D GetMouseMotion();

	// UI Navigation - Nueva API simplificada
	void UpdateUINavigation();
	void SetNavigationEnabled(bool enabled) { navigationEnabled = enabled; }
	bool IsNavigationEnabled() const { return navigationEnabled; }

private:
	// Window events
	bool windowEvents[WE_COUNT];

	// Keyboard
	KeyState* keyboard;
	
	// Mouse
	KeyState mouseButtons[NUM_MOUSE_BUTTONS];
	int mouseMotionX;
	int mouseMotionY;
	int mouseX;
	int mouseY;

	// Gamepad
	bool gamepadConnected;
	SDL_Gamepad* gamepad;
	KeyState gamepadButtons[GAMEPAD_COUNT];
	float gamepadAxes[GAMEPAD_AXIS_COUNT];

	// UI Navigation mejorada
	bool navigationEnabled;
	float stickThresholdX;
	float stickThresholdY;
	float lastStickX;
	float lastStickY;

	// Private methods
	void UpdateGamepadState();
	void UpdateGamepadButtons();
	void SimulateMouseClick(int x, int y);
};