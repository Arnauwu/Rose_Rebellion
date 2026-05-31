#include "Engine.h"
#include "Input.h"
#include "Window.h"
#include "Render.h"
#include "Log.h"
#include "UIManager.h"
#include "UIButton.h"
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
	selectedButtonIndex = -1;
	stickThreshold = 0.5f;
	lastStickX = 0.0f;
	lastStickY = 0.0f;
}

// Destructor
Input::~Input()
{
	delete[] keyboard;
	if (gamepad != nullptr)
	{
		SDL_CloseGamepad(gamepad);
		gamepad = nullptr;
	}
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

	// Try to connect first gamepad
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
	UpdateGamepadUINavigation();

	return true;
}

void Input::UpdateGamepadState()
{
	if (gamepad == nullptr) return;

	// Update analog sticks and triggers
	gamepadAxes[GAMEPAD_AXIS_LSTICK_X] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX) / 32768.0f;
	gamepadAxes[GAMEPAD_AXIS_LSTICK_Y] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY) / 32768.0f;
	gamepadAxes[GAMEPAD_AXIS_RSTICK_X] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTX) / 32768.0f;
	gamepadAxes[GAMEPAD_AXIS_RSTICK_Y] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTY) / 32768.0f;
	gamepadAxes[GAMEPAD_AXIS_LT] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) / 32768.0f;
	gamepadAxes[GAMEPAD_AXIS_RT] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) / 32768.0f;

	// Apply deadzone
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

	// Define button mappings
	SDL_GamepadButton sdlButtons[] = {
		SDL_GAMEPAD_BUTTON_SOUTH,		// A
		SDL_GAMEPAD_BUTTON_EAST,		// B
		SDL_GAMEPAD_BUTTON_WEST,		// X
		SDL_GAMEPAD_BUTTON_NORTH,		// Y
		SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,			// LB
		SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,			// RB
		SDL_GAMEPAD_BUTTON_BACK,		// BACK
		SDL_GAMEPAD_BUTTON_START,		// START
		SDL_GAMEPAD_BUTTON_LEFT_STICK,	// LSTICK
		SDL_GAMEPAD_BUTTON_RIGHT_STICK, // RSTICK
		SDL_GAMEPAD_BUTTON_GUIDE		// GUIDE
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

void Input::UpdateGamepadUINavigation()
{
	if (menuButtons.empty() || !gamepadConnected) return;

	menuButtons.erase(
		std::remove_if(menuButtons.begin(), menuButtons.end(),
			[](const std::weak_ptr<UIButton>& btn) { return btn.expired(); }),
		menuButtons.end()
	);

	if (menuButtons.empty()) {
		selectedButtonIndex = -1;
		return;
	}

	float stickX = GetGamepadAxis(GAMEPAD_AXIS_LSTICK_X);
	float stickY = GetGamepadAxis(GAMEPAD_AXIS_LSTICK_Y);

	bool moved = (fabs(stickX - lastStickX) > stickThreshold) || 
	             (fabs(stickY - lastStickY) > stickThreshold);

	if (moved && (fabs(stickX) > 0.7f || fabs(stickY) > 0.7f)) {
		int newIndex = FindClosestButton(stickX, stickY);
		if (newIndex != selectedButtonIndex) {
			SelectButton(newIndex);
		}
		lastStickX = stickX;
		lastStickY = stickY;
	}

	if (selectedButtonIndex >= 0 && GetGamepadButton(GAMEPAD_A) == KEY_DOWN) {
		auto btn = menuButtons[selectedButtonIndex].lock();
		if (btn) {
			mouseX = btn->bounds.x + btn->bounds.w / 2;
			mouseY = btn->bounds.y + btn->bounds.h / 2;
			mouseButtons[0] = KEY_DOWN;
		}
	}
}

void Input::SelectButton(int index)
{
	if (index < 0 || index >= (int)menuButtons.size()) return;

	selectedButtonIndex = index;
	auto btn = menuButtons[index].lock();
	if (btn) {
		mouseX = btn->bounds.x + btn->bounds.w / 2;
		mouseY = btn->bounds.y + btn->bounds.h / 2;
	}
}

int Input::FindClosestButton(float stickX, float stickY)
{
	if (menuButtons.empty()) return -1;

	if (selectedButtonIndex < 0) return 0;

	float angle = atan2(stickY, stickX);
	std::vector<std::pair<int, float>> candidates;

	for (int i = 0; i < (int)menuButtons.size(); ++i) {
		auto btn = menuButtons[i].lock();
		if (!btn || !btn->visible || !btn->canClick) continue;

		auto current = menuButtons[selectedButtonIndex].lock();
		if (!current) continue;

		int dx = btn->bounds.x - current->bounds.x;
		int dy = btn->bounds.y - current->bounds.y;
		float btnAngle = atan2((float)dy, (float)dx);
		float angleDiff = fabs(btnAngle - angle);

		if (angleDiff > 3.14159f) angleDiff = 6.28318f - angleDiff;

		candidates.push_back({ i, angleDiff });
	}

	if (!candidates.empty()) {
		std::sort(candidates.begin(), candidates.end(),
			[](const std::pair<int, float>& a, const std::pair<int, float>& b) { 
				return a.second < b.second; 
			});
		return candidates[0].first;
	}

	return selectedButtonIndex;
}

void Input::RegisterMenuButtons(const std::vector<std::shared_ptr<UIButton>>& buttons)
{
	menuButtons.clear();
	selectedButtonIndex = -1;
	
	for (const auto& btn : buttons) {
		menuButtons.push_back(btn);
	}

	if (!menuButtons.empty()) {
		SelectButton(0);
	}
}

void Input::ClearMenuButtons()
{
	menuButtons.clear();
	selectedButtonIndex = -1;
	lastStickX = 0.0f;
	lastStickY = 0.0f;
}

// Called before quitting
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