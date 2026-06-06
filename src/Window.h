#pragma once

#include "Module.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

class Window : public Module
{
public:

	Window();

	// Destructor
	virtual ~Window();

	// Called before render is available
	bool Awake();

	// Called before quitting
	bool CleanUp();

	// Changae title
	void SetTitle(const char* title);

	// Retrive window size
	void GetWindowSize(int& width, int& height) const;

	// Retrieve window scale
	int GetScale() const;

	void SetFullscreen(bool enabled, SDL_Renderer* renderer = nullptr);

	// Cargar el icono de la ventana
	bool SetWindowIcon(const char* imagePath);

	// Cargar un cursor personalizado
	bool SetCustomCursor(const char* imagePath, int hot_x, int hot_y);

public:
	// The window we'll be rendering to
	SDL_Window* window;

	std::string title;
	int windowWidth = 1280;
	int windowHeight = 780;
	int windowScale = 1;

	bool isFullscreen = false;
	bool IsFullscreen() const { return isFullscreen; }
private:
	SDL_Cursor* customCursor = nullptr;
};
