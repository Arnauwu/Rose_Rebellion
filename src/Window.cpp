#include "Window.h"
#include "Log.h"
#include "Engine.h"
#include <SDL3_image/SDL_image.h>

Window::Window() : Module()
{
	window = NULL;
	name = "window";
}

// Destructor
Window::~Window()
{
}

// Called before render is available
bool Window::Awake()
{

	LOG("Init SDL window & surface");
	bool ret = true;

	if (SDL_Init(SDL_INIT_VIDEO) != true)
	{
		LOG("SDL_VIDEO could not initialize! SDL_Error: %s\n", SDL_GetError());
		ret = false;
	}
	else
	{
		// Create window
		// Get the values from the config file
		Uint32 flags = 0;
		bool fullscreen = configParameters.child("fullscreen").attribute("value").as_bool();
		bool borderless = configParameters.child("borderless").attribute("value").as_bool();
		bool resizable = configParameters.child("resizable").attribute("value").as_bool();
		bool fullscreen_window = configParameters.child("fullscreen_window").attribute("value").as_bool();

		// Get the values from the config file
		windowWidth = configParameters.child("resolution").attribute("width").as_int();
		windowHeight = configParameters.child("resolution").attribute("height").as_int();
		windowScale = configParameters.child("resolution").attribute("scale").as_int();

		if (fullscreen == true)        flags |= SDL_WINDOW_FULLSCREEN;
		if (borderless == true)        flags |= SDL_WINDOW_BORDERLESS;
		if (resizable == true)         flags |= SDL_WINDOW_RESIZABLE;

		// SDL3: SDL_CreateWindow(title, w, h, flags). Set position separately.
		window = SDL_CreateWindow("Rose Rebellion", windowWidth, windowHeight, flags);

		if (window == NULL)
		{
			LOG("Window could not be created! SDL_Error: %s\n", SDL_GetError());
			ret = false;
		}
		else
		{
			if (fullscreen_window == true)
			{
				SDL_SetWindowFullscreenMode(window, nullptr); // use desktop resolution
				SDL_SetWindowFullscreen(window, true);
			}
			SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
			SDL_ShowWindow(window);
		}
	}

	Engine::GetInstance().window->SetWindowIcon("Assets/Window/icon.png");

	Engine::GetInstance().window->SetCustomCursor("Assets/Window/cursor.png", 0, 0);
	return ret;
}

// Called before quitting
bool Window::CleanUp()
{
	LOG("Destroying SDL window and quitting all SDL systems");

	if (customCursor != nullptr)
	{
		SDL_DestroyCursor(customCursor);
		customCursor = nullptr;
	}

	// Destroy window
	if (window != NULL)
	{
		SDL_DestroyWindow(window);
	}

	// Quit SDL subsystems
	SDL_Quit();
	return true;
}

// Set new window title
void Window::SetTitle(const char* new_title)
{
	SDL_SetWindowTitle(window, new_title);
}

void Window::GetWindowSize(int& width, int& height) const
{
	SDL_GetWindowSize(window, &width, &height);
}

int Window::GetScale() const
{
	return windowScale;
}

void Window::SetFullscreen(bool enabled, SDL_Renderer* renderer) 
{
	// Save mouse Position
	float mouseX, mouseY;
	SDL_GetMouseState(&mouseX, &mouseY);

	int oldW, oldH;
	SDL_GetWindowSize(window, &oldW, &oldH);

	float relX = (oldW > 0) ? mouseX / (float)oldW : 0.5f;
	float relY = (oldH > 0) ? mouseY / (float)oldH : 0.5f;

	// Fullscreen mode aplay
	isFullscreen = enabled;
	if (enabled) {
		SDL_SetWindowFullscreen(window, true);
		SDL_SetWindowMouseGrab(window, true);
	}
	else {
		SDL_SetWindowFullscreen(window, false);
		SDL_SetWindowMouseGrab(window, false);
	}

	SDL_SyncWindow(window);

	// Resize and reposition the cursor in the same relative position
	int newW, newH;
	SDL_GetWindowSize(window, &newW, &newH);
	//SDL_GetWindowSizeInPixels(window, &windowWidth, &windowHeight);

	SDL_WarpMouseInWindow(window, relX * newW, relY * newH);


	SDL_SetRenderLogicalPresentation(renderer, windowWidth, windowHeight, SDL_LOGICAL_PRESENTATION_LETTERBOX);

}

bool Window::SetWindowIcon(const char* imagePath)
{
	bool ret = true;
	SDL_Surface* iconSurface = IMG_Load(imagePath);

	if (iconSurface == NULL)
	{
		LOG("No se pudo cargar el icono %s. IMG_Error: %s\n", imagePath, SDL_GetError());
		ret = false;
	}
	else
	{
		if (!SDL_SetWindowIcon(window, iconSurface))
		{
			LOG("Error al establecer el icono de la ventana: %s\n", SDL_GetError());
			ret = false;
		}
		SDL_DestroySurface(iconSurface);
	}

	return ret;
}

// Cargar y establecer un cursor personalizado
bool Window::SetCustomCursor(const char* imagePath, int hot_x, int hot_y)
{
	bool ret = true;

	// 1. Cargamos la imagen desde la ruta del sistema de archivos
	SDL_Surface* tempSurface = IMG_Load(imagePath);

	if (tempSurface == NULL)
	{
		LOG("No se pudo cargar el cursor %s. SDL_Error: %s\n", imagePath, SDL_GetError());
		ret = false;
	}
	else
	{
		// 2. Convertimos estrictamente la superficie a RGBA32
		SDL_Surface* cursorSurface = SDL_ConvertSurface(tempSurface, SDL_PIXELFORMAT_RGBA32);

		// 3. Destruimos la superficie temporal inmediatamente
		SDL_DestroySurface(tempSurface);

		if (cursorSurface == NULL)
		{
			LOG("Error al convertir la superficie del cursor. SDL_Error: %s\n", SDL_GetError());
			ret = false;
		}
		else
		{
			if (customCursor != nullptr)
			{
				SDL_DestroyCursor(customCursor);
			}

			// 4. Creamos el cursor con la superficie formateada correctamente
			customCursor = SDL_CreateColorCursor(cursorSurface, hot_x, hot_y);

			if (customCursor == NULL)
			{
				LOG("No se pudo crear el cursor de color. SDL_Error: %s\n", SDL_GetError());
				ret = false;
			}
			else
			{
				SDL_SetCursor(customCursor);
				SDL_ShowCursor(); // Forzamos a que se muestre
			}

			// 5. Limpiamos la superficie final
			SDL_DestroySurface(cursorSurface);
		}
	}

	return ret;
}