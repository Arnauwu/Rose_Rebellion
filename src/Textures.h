#pragma once

#include "Module.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <list>

class Textures : public Module
{
public:

	Textures();

	// Destructor
	virtual ~Textures();

	// Called before render is available
	bool Awake();

	// Called before the first frame
	bool Start();

	// Called before quitting
	bool CleanUp();

	// Métodos heredados (Mantienen compatibilidad síncrona)
	SDL_Texture* const Load(const char* path);
	SDL_Texture* const LoadSurface(SDL_Surface* surface);
	bool UnLoad(SDL_Texture* texture);
	void GetSize(const SDL_Texture* texture, int& width, int& height) const;

	// Carga la imagen en memoria del sistema (Ejecutable en hilos secundarios)
	SDL_Surface* LoadSurfaceToRAM(const char* path);

	// Sube los píxeles de la RAM a la VRAM de la GPU (SOLO ejecutable en el hilo principal)
	SDL_Texture* CreateTextureFromRAM(SDL_Surface* surface);

public:
	std::list<SDL_Texture*> textures;

};