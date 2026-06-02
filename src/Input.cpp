#include "Engine.h"
#include "Input.h"
#include "Window.h"
#include "Render.h"
#include "Log.h"
#include "UIManager.h"
#include <cmath>
#include <algorithm>
#include <cfloat>

#define MAX_KEYS 300
#define GAMEPAD_DEADZONE 0.25f

Input::Input() : Module()
{
	name = "input";

	keyboard = new KeyState[MAX_KEYS];
	memset(keyboard, KEY_IDLE, sizeof(KeyState) * MAX_KEYS);
	memset(mouseButtons, KEY_IDLE, sizeof(KeyState) * NUM_MOUSE_BUTTONS);
	memset(gamepadButtons, KEY_IDLE, sizeof(KeyState) * GAMEPAD_COUNT);
	memset(gamepadAxes, 0, sizeof(float) * GAMEPAD_AXIS_COUNT);
	memset(windowEvents, 0, sizeof(windowEvents));

	mouseMotionX = 0;
	mouseMotionY = 0;
	mouseX = 0;
	mouseY = 0;
	gamepadConnected = false;
	gamepad = nullptr;
	navigationEnabled = false;
	stickThresholdX = 0.7f;
	stickThresholdY = 0.7f;
	lastStickX = 0.0f;
	lastStickY = 0.0f;
}

Input::~Input()
{
	delete[] keyboard;
	if (gamepad != nullptr)
	{
		SDL_CloseGamepad(gamepad);
		gamepad = nullptr;
	}
}

bool Input::Awake()
{
	LOG("Init SDL input event system");
	bool ret = true;

	if (SDL_InitSubSystem(SDL_INIT_EVENTS) != true)
	{
		LOG("SDL_EVENTS could not initialize! SDL_Error: %s\n", SDL_GetError());
		ret = false;
	}

	if (SDL_InitSubSystem(SDL_INIT_GAMEPAD) != true)
	{
		LOG("SDL_GAMEPAD could not initialize! SDL_Error: %s\n", SDL_GetError());
		ret = false;
	}

	return ret;
}

bool Input::Start()
{
	SDL_StopTextInput(Engine::GetInstance().window->window);

	int numJoysticks = 0;
	const SDL_JoystickID* joystickIds = SDL_GetJoysticks(&numJoysticks);

	if (numJoysticks > 0)
	{
		gamepad = SDL_OpenGamepad(joystickIds[0]);
		if (gamepad != nullptr)
		{
			gamepadConnected = true;
			LOG("Gamepad connected successfully");
		}
	}

	return true;
}

bool Input::PreUpdate()
{
	static SDL_Event event;

	int numKeys = 0;
	const bool* keys = SDL_GetKeyboardState(&numKeys);

	for (int i = 0; i < MAX_KEYS; ++i)
	{
		if (keys[i] == 1)
		{
			if (keyboard[i] == KEY_IDLE)
				keyboard[i] = KEY_DOWN;
			else
				keyboard[i] = KEY_REPEAT;
		}
		else
		{
			if (keyboard[i] == KEY_REPEAT || keyboard[i] == KEY_DOWN)
				keyboard[i] = KEY_UP;
			else
				keyboard[i] = KEY_IDLE;
		}
	}

	for (int i = 0; i < NUM_MOUSE_BUTTONS; ++i)
	{
		if (mouseButtons[i] == KEY_DOWN)
			mouseButtons[i] = KEY_REPEAT;

		if (mouseButtons[i] == KEY_UP)
			mouseButtons[i] = KEY_IDLE;
	}

	UpdateGamepadButtons();

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_EVENT_QUIT:
			windowEvents[WE_QUIT] = true;
			break;

		case SDL_EVENT_WINDOW_HIDDEN:
		case SDL_EVENT_WINDOW_MINIMIZED:
		case SDL_EVENT_WINDOW_FOCUS_LOST:
			windowEvents[WE_HIDE] = true;
			break;

		case SDL_EVENT_WINDOW_SHOWN:
		case SDL_EVENT_WINDOW_FOCUS_GAINED:
		case SDL_EVENT_WINDOW_MAXIMIZED:
		case SDL_EVENT_WINDOW_RESTORED:
			windowEvents[WE_SHOW] = true;
			break;

		case SDL_EVENT_WINDOW_RESIZED:
		case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
			Engine::GetInstance().uiManager->RecalculateAllUI();
			break;

		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			if (event.button.button >= 1 && event.button.button <= NUM_MOUSE_BUTTONS)
				mouseButtons[event.button.button - 1] = KEY_DOWN;
			break;

		case SDL_EVENT_MOUSE_BUTTON_UP:
			if (event.button.button >= 1 && event.button.button <= NUM_MOUSE_BUTTONS)
				mouseButtons[event.button.button - 1] = KEY_UP;
			break;

		case SDL_EVENT_MOUSE_MOTION:
		{
			int scale = Engine::GetInstance().window->GetScale();
			mouseMotionX = (int)(event.motion.xrel / scale);
			mouseMotionY = (int)(event.motion.yrel / scale);
			mouseX = (int)(event.motion.x / scale);
			mouseY = (int)(event.motion.y / scale);
		}
		break;

		case SDL_EVENT_GAMEPAD_ADDED:
			if (gamepad == nullptr)
			{
				gamepad = SDL_OpenGamepad(event.jdevice.which);
				if (gamepad != nullptr)
				{
					gamepadConnected = true;
					LOG("Gamepad connected");
				}
			}
			break;

		case SDL_EVENT_GAMEPAD_REMOVED:
			if (gamepad != nullptr)
			{
				SDL_CloseGamepad(gamepad);
				gamepad = nullptr;
				gamepadConnected = false;
				memset(gamepadButtons, KEY_IDLE, sizeof(KeyState) * GAMEPAD_COUNT);
				memset(gamepadAxes, 0, sizeof(float) * GAMEPAD_AXIS_COUNT);
				LOG("Gamepad disconnected");
			}
			break;

		case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		case SDL_EVENT_GAMEPAD_BUTTON_UP:
		case SDL_EVENT_GAMEPAD_AXIS_MOTION:
			UpdateGamepadState();
			break;
		}
	}

	UpdateGamepadState();
	UpdateGamepadButtons();
	UpdateUINavigation();

	return true;
}

void Input::UpdateGamepadState()
{
	if (gamepad == nullptr) return;

	gamepadAxes[GAMEPAD_AXIS_LSTICK_X] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX) / 32768.0f;
	gamepadAxes[GAMEPAD_AXIS_LSTICK_Y] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY) / 32768.0f;
	gamepadAxes[GAMEPAD_AXIS_RSTICK_X] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTX) / 32768.0f;
	gamepadAxes[GAMEPAD_AXIS_RSTICK_Y] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTY) / 32768.0f;
	gamepadAxes[GAMEPAD_AXIS_LT] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) / 32768.0f;
	gamepadAxes[GAMEPAD_AXIS_RT] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) / 32768.0f;

	for (int i = 0; i < GAMEPAD_AXIS_COUNT; ++i)
	{
		if (fabs(gamepadAxes[i]) < GAMEPAD_DEADZONE)
		{
			gamepadAxes[i] = 0.0f;
		}
	}
}

void Input::UpdateGamepadButtons()
{
	if (gamepad == nullptr) return;

	SDL_GamepadButton sdlButtons[] = {
		SDL_GAMEPAD_BUTTON_SOUTH,
		SDL_GAMEPAD_BUTTON_EAST,
		SDL_GAMEPAD_BUTTON_WEST,
		SDL_GAMEPAD_BUTTON_NORTH,
		SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,
		SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
		SDL_GAMEPAD_BUTTON_BACK,
		SDL_GAMEPAD_BUTTON_START,
		SDL_GAMEPAD_BUTTON_LEFT_STICK,
		SDL_GAMEPAD_BUTTON_RIGHT_STICK,
		SDL_GAMEPAD_BUTTON_GUIDE
	};

	for (int i = 0; i < GAMEPAD_COUNT; ++i)
	{
		bool isPressed = SDL_GetGamepadButton(gamepad, sdlButtons[i]);

		if (isPressed)
		{
			if (gamepadButtons[i] == KEY_IDLE)
				gamepadButtons[i] = KEY_DOWN;
			else
				gamepadButtons[i] = KEY_REPEAT;
		}
		else
		{
			if (gamepadButtons[i] == KEY_REPEAT || gamepadButtons[i] == KEY_DOWN)
				gamepadButtons[i] = KEY_UP;
			else
				gamepadButtons[i] = KEY_IDLE;
		}
	}
}

void Input::UpdateUINavigation()
{
	if (!navigationEnabled || !gamepadConnected) return;

	float stickX = GetGamepadAxis(GAMEPAD_AXIS_LSTICK_X);
	float stickY = GetGamepadAxis(GAMEPAD_AXIS_LSTICK_Y);

	if (fabs(stickX) > stickThresholdX || fabs(stickY) > stickThresholdY)
	{
		int screenW = Engine::GetInstance().window->windowWidth;
		int screenH = Engine::GetInstance().window->windowHeight;
		
		int moveX = (int)(stickX * 15.0f);
		int moveY = (int)(stickY * 15.0f);
		
		mouseX += moveX;
		mouseY += moveY;
		
		mouseX = std::max(0, std::min(mouseX, screenW - 1));
		mouseY = std::max(0, std::min(mouseY, screenH - 1));

		SDL_WarpMouseInWindow(Engine::GetInstance().window->window, (float)mouseX, (float)mouseY);
	}

	// Botón A simula click izquierdo - SIN SDL_PushEvent
	if (GetGamepadButton(GAMEPAD_A) == KEY_DOWN)
	{
		mouseButtons[0] = KEY_DOWN;
	}
	else if (GetGamepadButton(GAMEPAD_A) == KEY_REPEAT)
	{
		mouseButtons[0] = KEY_REPEAT;
	}
	else if (GetGamepadButton(GAMEPAD_A) == KEY_UP)
	{
		mouseButtons[0] = KEY_UP;
	}
}

bool Input::CleanUp()
{
	LOG("Quitting SDL event subsystem");
	if (gamepad != nullptr)
	{
		SDL_CloseGamepad(gamepad);
		gamepad = nullptr;
	}
	SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
	SDL_QuitSubSystem(SDL_INIT_EVENTS);
	return true;
}

bool Input::GetWindowEvent(EventWindow ev)
{
	return windowEvents[ev];
}

KeyState Input::GetKey(int id) const
{
	return keyboard[id];
}

KeyState Input::GetMouseButtonDown(int id) const
{
	return mouseButtons[id - 1];
}

KeyState Input::GetGamepadButton(GamepadButton button) const
{
	if (button >= 0 && button < GAMEPAD_COUNT)
		return gamepadButtons[button];
	return KEY_IDLE;
}

float Input::GetGamepadAxis(GamepadAxis axis) const
{
	if (axis >= 0 && axis < GAMEPAD_AXIS_COUNT)
		return gamepadAxes[axis];
	return 0.0f;
}

Vector2D Input::GetMousePosition()
{
	float windowX, windowY;
	SDL_GetMouseState(&windowX, &windowY);

	SDL_Renderer* renderer = Engine::GetInstance().render->renderer;

	float logicalX, logicalY;
	SDL_RenderCoordinatesFromWindow(renderer, windowX, windowY, &logicalX, &logicalY);

	return Vector2D((int)logicalX, (int)logicalY);
}

Vector2D Input::GetMouseMotion()
{
	return Vector2D((float)mouseMotionX, (float)mouseMotionY);
}

void Input::ClearMouseInput()
{
	for (int i = 0; i < NUM_MOUSE_BUTTONS; ++i)
	{
		mouseButtons[i] = KEY_IDLE;
	}
}