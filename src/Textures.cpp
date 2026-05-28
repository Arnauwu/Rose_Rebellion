#include "Engine.h"
#include "Render.h"
#include "Textures.h"
#include "Log.h"

Textures::Textures() : Module()
{
	name = "textures";
}

// Destructor
Textures::~Textures()
{
}

// Called before render is available
bool Textures::Awake()
{
	LOG("Init Image library");
	bool ret = true;

	return ret;
}

// Called before the first frame
bool Textures::Start()
{
	LOG("start textures");
	bool ret = true;
	return ret;
}

// Called before quitting
bool Textures::CleanUp()
{
	LOG("Freeing textures and Image library");
	for (const auto& texture : textures) {
		SDL_DestroyTexture(texture);
	}
	textures.clear();

	return true;
}

SDL_Texture* const Textures::Load(const char* path)
{
	SDL_Texture* texture = NULL;
	std::string sPath = path;

	if (surfaceCache.find(sPath) != surfaceCache.end())
	{
		SDL_Surface* surface = surfaceCache[sPath];

		texture = LoadSurface(surface); 

		SDL_DestroySurface(surface);
		surfaceCache.erase(sPath); 

		return texture;
	}

	SDL_Surface* surface = IMG_Load(path);
	if (surface != NULL)
	{
		texture = LoadSurface(surface);
		SDL_DestroySurface(surface);
	}

	return texture;
}

bool Textures::UnLoad(SDL_Texture* texture)
{
	for (auto it = textures.begin(); it != textures.end(); ++it) {
		if (*it == texture) {
			SDL_DestroyTexture(texture);
			textures.erase(it);
			return true;
		}
	}
	return false;
}

SDL_Texture* const Textures::LoadSurface(SDL_Surface* surface)
{
	return CreateTextureFromRAM(surface);
}

void Textures::GetSize(const SDL_Texture* texture, int& width, int& height) const
{
	float tw = 0.0f;
	float th = 0.0f;
	if (!SDL_GetTextureSize((SDL_Texture*)texture, &tw, &th))
	{
		LOG("SDL_GetTextureSize failed: %s", SDL_GetError());
		width = 0;
		height = 0;
	}
	else
	{
		width = (int)tw;
		height = (int)th;
	}
}


SDL_Surface* Textures::LoadSurfaceToRAM(const char* path)
{
	// Seguro de usar en hilos secundarios (No interactúa con la GPU)
	SDL_Surface* surface = IMG_Load(path);
	if (surface == NULL)
	{
		LOG("Could not load surface with path: %s. IMG_Load: %s", path, SDL_GetError());
	}
	return surface;
}

SDL_Texture* Textures::CreateTextureFromRAM(SDL_Surface* surface)
{
	if (surface == nullptr) return nullptr;

	SDL_Texture* texture = SDL_CreateTextureFromSurface(Engine::GetInstance().render->renderer, surface);

	if (texture == NULL)
	{
		LOG("Unable to create texture from surface! SDL Error: %s\n", SDL_GetError());
	}
	else
	{
		SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
		textures.push_back(texture);
	}

	return texture;
}

void Textures::PreloadToRAM(const std::string& path)
{
	SDL_Surface* surface = IMG_Load(path.c_str());
	if (surface != NULL)
	{
		surfaceCache[path] = surface; // Lo guardamos en la caché temporal
	}
	else
	{
		LOG("Error precargando textura pesada: %s", SDL_GetError());
	}
}