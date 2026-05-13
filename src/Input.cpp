#include "Engine.h"
#include "Input.h"
#include "Window.h"
#include "render.h"
#include "Log.h"
#include "UIManager.h"

#define MAX_KEYS 300

Input::Input() : Module()
{
	name = "input";

	keyboard = new KeyState[MAX_KEYS];
	memset(keyboard, KEY_IDLE, sizeof(KeyState) * MAX_KEYS);
	memset(mouseButtons, KEY_IDLE, sizeof(KeyState) * NUM_MOUSE_BUTTONS);
	memset(windowEvents, 0, sizeof(windowEvents));
	mouseMotionX = mouseMotionY = mouseX = mouseY = 0;

	controllerConnected = false;
	memset(controllerButtons, KEY_IDLE, sizeof(KeyState) * SDL_GAMEPAD_BUTTON_COUNT);
	controllerLeftStickX = 0.0f;
	controllerLeftStickY = 0.0f;
	controllerRightStickX = 0.0f;
	controllerRightStickY = 0.0f;
	gamepad = nullptr;
}

// Destructor
Input::~Input()
{
	delete[] keyboard;
}

// Called before render is available
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

// Called before the first frame
bool Input::Start()
{
	SDL_StopTextInput(Engine::GetInstance().window->window);

	int numJoysticks = 0;
	SDL_JoystickID* joystickIds = SDL_GetJoysticks(&numJoysticks);

	if (numJoysticks > 0 && SDL_IsGamepad(joystickIds[0]))
	{
		gamepad = SDL_OpenGamepad(joystickIds[0]);
		if (gamepad)
		{
			controllerConnected = true;
			LOG("Gamepad connected on startup");
		}
	}

	SDL_free(joystickIds);

	return true;
}

// Called each loop iteration
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

	// Actualizar botones del mando (cambiar DOWN a REPEAT, UP a IDLE)
	for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b)
	{
		if (controllerButtons[b] == KEY_DOWN)
			controllerButtons[b] = KEY_REPEAT;

		if (controllerButtons[b] == KEY_UP)
			controllerButtons[b] = KEY_IDLE;
	}

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

		case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		{
			SDL_GamepadButton button = (SDL_GamepadButton)event.gbutton.button;
			if (controllerButtons[button] == KEY_IDLE)
				controllerButtons[button] = KEY_DOWN;
			else
				controllerButtons[button] = KEY_REPEAT;
		}
		break;

		case SDL_EVENT_GAMEPAD_BUTTON_UP:
		{
			SDL_GamepadButton button = (SDL_GamepadButton)event.gbutton.button;
			controllerButtons[button] = KEY_UP;
		}
		break;

		case SDL_EVENT_GAMEPAD_AXIS_MOTION:
		{
			// Normalizar valor del eje [-32767, 32767] a [-1.0, 1.0]
			float normalizedValue = event.gaxis.value / 32767.0f;

			// Aplicar deadzone (si está entre -0.15 y 0.15, considerar como 0)
			if (normalizedValue > -0.15f && normalizedValue < 0.15f)
				normalizedValue = 0.0f;

			switch (event.gaxis.axis)
			{
			case SDL_GAMEPAD_AXIS_LEFTX:
				controllerLeftStickX = normalizedValue;
				break;
			case SDL_GAMEPAD_AXIS_LEFTY:
				controllerLeftStickY = normalizedValue;
				break;
			case SDL_GAMEPAD_AXIS_RIGHTX:
				controllerRightStickX = normalizedValue;
				break;
			case SDL_GAMEPAD_AXIS_RIGHTY:
				controllerRightStickY = normalizedValue;
				break;
			default:
				break;
			}
		}
		break;

		case SDL_EVENT_GAMEPAD_ADDED:
		{
			if (!controllerConnected)
			{
				gamepad = SDL_OpenGamepad(event.gdevice.which);
				if (gamepad)
				{
					controllerConnected = true;
					LOG("Gamepad connected");
				}
			}
		}
		break;

		case SDL_EVENT_GAMEPAD_REMOVED:
		{
			if (gamepad)
			{
				SDL_CloseGamepad(gamepad);
				gamepad = nullptr;
				controllerConnected = false;
				LOG("Gamepad disconnected");
			}
		}
		break;

		default:
			break;
		}
	}

	return true;
}

// Called before quitting
bool Input::CleanUp()
{
	LOG("Quitting SDL event subsystem");

	if (gamepad)
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

// Functions to ensure mouse information is not lost when switching from windowed mode to full-screen mode
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

// Teclado
KeyState Input::GetKey(SDL_Scancode key) const
{
	return keyboard[key];
}

// Ratón
KeyState Input::GetMouseButtonDown(int button) const
{
	if (button >= 0 && button < NUM_MOUSE_BUTTONS)
		return mouseButtons[button];
	return KEY_IDLE;
}

// Mando
bool Input::IsGamepadConnected() const
{
	return controllerConnected;
}

KeyState Input::GetGamepadButton(SDL_GamepadButton button) const
{
	if (button >= 0 && button < SDL_GAMEPAD_BUTTON_COUNT)
		return controllerButtons[button];
	return KEY_IDLE;
}

float Input::GetGamepadLeftStickX() const
{
	return controllerLeftStickX;
}

float Input::GetGamepadLeftStickY() const
{
	return controllerLeftStickY;
}

float Input::GetGamepadRightStickX() const
{
	return controllerRightStickX;
}

float Input::GetGamepadRightStickY() const
{
	return controllerRightStickY;
}