#include "Engine.h"
#include "Window.h"
#include "Render.h"
#include "Log.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Render::Render() : Module()
{
	name = "render";
	background.r = 0;
	background.g = 0;
	background.b = 0;
	background.a = 0;
}

// Destructor
Render::~Render()
{
}

// Called before render is available
bool Render::Awake()
{
	LOG("Create SDL rendering context");
	bool ret = true;

	if (TTF_Init() == -1)
	{
		LOG("SDL_ttf could not initialize! SDL_ttf Error: %s\n", SDL_GetError());
		ret = false;
	}

	int scale = Engine::GetInstance().window->GetScale();
	SDL_Window* window = Engine::GetInstance().window->window;

	// Load the configuration of the Render module
	
	// SDL3: no flags; create default renderer and set vsync separately
	renderer = SDL_CreateRenderer(window, nullptr);

	if (renderer == NULL)
	{
		LOG("Could not create the renderer! SDL_Error: %s\n", SDL_GetError());
		ret = false;
	}
	else
	{
		if (configParameters.child("vsync").attribute("value").as_bool())
		{
			if (!SDL_SetRenderVSync(renderer, 1))
			{
				LOG("Warning: could not enable vsync: %s", SDL_GetError());
			}
			else
			{
				LOG("Using vsync");
			}
		}

		int baseWidth = 1920;
		int baseHeight = 1080;

		
		SDL_SetRenderLogicalPresentation(renderer, baseWidth, baseHeight,
			SDL_LOGICAL_PRESENTATION_LETTERBOX);

		// Como SDL ahora maneja el escalado internamente, la cámara solo necesita tu resolución base.
		camera.w = baseWidth;
		camera.h = baseHeight;
		camera.x = 0;
		camera.y = 0;
	}

	return ret;
}

// Called before the first frame
bool Render::Start() {
	LOG("render start");

	// Cargamos la misma fuente con diferentes tamaños
	fonts[FontType::MENU] = TTF_OpenFont(fontPath, 49);
	fonts[FontType::SPEAKER] = TTF_OpenFont(fontPath, 39);
	fonts[FontType::DIALOGUE] = TTF_OpenFont(fontPath, 31); // Tamaño más adecuado para texto largo
	fonts[FontType::CUERPO] = TTF_OpenFont(fontPath, 25);

	// Verificamos que se cargaron bien
	for (auto it = fonts.begin(); it != fonts.end(); ++it) {
		FontType type = it->first;
		TTF_Font* f = it->second;
		if (f == nullptr) LOG("Failed to load font type! SDL_ttf Error: %s", SDL_GetError());
	}
	return true;
}

// Called each loop iteration
bool Render::PreUpdate()
{
	SDL_RenderClear(renderer);
	return true;
}

bool Render::Update(float dt)
{
	UpdateFade(dt);
	return true;
}

bool Render::PostUpdate()
{
	DrawFade();
	SDL_SetRenderDrawColor(renderer, background.r, background.g, background.g, background.a);
	SDL_RenderPresent(renderer);
	return true;
}

// Called before quitting
bool Render::CleanUp() {
	for (auto it = fonts.begin(); it != fonts.end(); ++it) {
		FontType type = it->first;
		TTF_Font* f = it->second;
		if (f != nullptr) TTF_CloseFont(f);
	}

	fonts.clear();
	TTF_Quit();
	return true;
}

void Render::SetBackgroundColor(SDL_Color color)
{
	background = color;
}

void Render::SetColorMod(SDL_Texture* texture, Uint8* r, Uint8* g, Uint8* b, Uint8 nr, Uint8 ng, Uint8 nb)
{
	SDL_GetTextureColorMod(texture, r, g, b); //Get original texture RGB
	SDL_SetTextureColorMod(texture, nr, ng, nb); //Set to X color 
}

bool Render::IsOnScreenWorldRect(float x, float y, float w, float h, int margin) const
{
	bool result = false;

	float camLeft = -camera.x - margin;
	float camTop = -camera.y - margin;

	float camRight = camLeft + camera.w + margin;
	float camBott = camTop + camera.h + margin;

	float objRight = x + w;
	float objBott = y + h;

	if (objRight >= camLeft && x <= camRight)
	{
		if (objBott >= camTop && y <= camBott)
		{
			result = true;
		}
	}

	return result;
}

void Render::SetViewPort(const SDL_Rect& rect)
{
	SDL_SetRenderViewport(renderer, &rect);
}

void Render::ResetViewPort()
{
	SDL_SetRenderViewport(renderer, &viewport);
}

// Blit to screen
bool Render::DrawTexture(SDL_Texture* texture, int x, int y, const SDL_Rect* section, float speed, double angle, int pivotX, int pivotY) const
{
	bool ret = true;
	int scale = Engine::GetInstance().window->GetScale();

	// SDL3 uses float rects for rendering
	SDL_FRect rect;
	rect.x = (float)((int)(camera.x * speed) + x * scale);
	rect.y = (float)((int)(camera.y * speed) + y * scale);

	if (section != NULL)
	{
		rect.w = (float)(section->w * scale);
		rect.h = (float)(section->h * scale);
	}
	else
	{
		float tw = 0.0f, th = 0.0f;
		if (!SDL_GetTextureSize(texture, &tw, &th))
		{
			LOG("SDL_GetTextureSize failed: %s", SDL_GetError());
			return false;
		}
		rect.w = tw * scale;
		rect.h = th * scale;
	}

	const SDL_FRect* src = NULL;
	SDL_FRect srcRect;
	if (section != NULL)
	{
		srcRect.x = (float)section->x;
		srcRect.y = (float)section->y;
		srcRect.w = (float)section->w;
		srcRect.h = (float)section->h;
		src = &srcRect;
	}

	SDL_FPoint* p = NULL;
	SDL_FPoint pivot;
	if (pivotX != INT_MAX && pivotY != INT_MAX)
	{
		pivot.x = (float)pivotX;
		pivot.y = (float)pivotY;
		p = &pivot;
	}

	// SDL3: returns bool; map to int-style check
	int rc = SDL_RenderTextureRotated(renderer, texture, src, &rect, angle, p, SDL_FLIP_NONE) ? 0 : -1;
	if (rc != 0)
	{
		LOG("Cannot blit to screen. SDL_RenderTextureRotated error: %s", SDL_GetError());
		ret = false;
	}

	return ret;
}

bool Render::DrawRotatedTexture(SDL_Texture* texture, int x, int y, const SDL_Rect* section, SDL_FlipMode flip, float adjustableScale, double angle, int pivotX, int pivotY) const
{
	bool ret = true;
	float scale = Engine::GetInstance().window->GetScale();


	// SDL3 uses float rects for rendering
	SDL_FRect rect;
	rect.x = (float)((camera.x + x) * scale);
	rect.y = (float)((camera.y + y) * scale);

	if (section != NULL)
	{
		rect.w = (float)(section->w * scale * adjustableScale);
		rect.h = (float)(section->h * scale * adjustableScale);
	}
	else
	{
		float tw = 0.0f, th = 0.0f;
		if (!SDL_GetTextureSize(texture, &tw, &th))
		{
			LOG("SDL_GetTextureSize failed: %s", SDL_GetError());
			return false;
		}
		rect.w = tw * scale * adjustableScale;
		rect.h = th * scale * adjustableScale;
	}

	rect.x -= rect.w / 2;
	rect.y -= rect.h / 2;

	const SDL_FRect* src = NULL;
	SDL_FRect srcRect;
	if (section != NULL)
	{
		srcRect.x = (float)section->x;
		srcRect.y = (float)section->y;
		srcRect.w = (float)section->w;
		srcRect.h = (float)section->h;
		src = &srcRect;
	}

	SDL_FPoint* p = NULL;
	SDL_FPoint pivot;
	if (pivotX != INT_MAX && pivotY != INT_MAX)
	{
		pivot.x = (float)pivotX * scale * adjustableScale;
		pivot.y = (float)pivotY * scale * adjustableScale;
		p = &pivot;
	}

	// SDL3: returns bool; map to int-style check
	int rc = SDL_RenderTextureRotated(renderer, texture, src, &rect, angle, p, flip) ? 0 : -1;
	if (rc != 0)
	{
		LOG("Cannot blit to screen. SDL_RenderTextureRotated error: %s", SDL_GetError());
		ret = false;
	}

	return ret;
}

bool Render::DrawTextureScaled(SDL_Texture* texture, const SDL_Rect& destRect) const
{
	if (texture == nullptr) return false;

	// SDL3 utiliza FRect (float) para renderizar
	SDL_FRect dstFRect;
	dstFRect.x = (float)destRect.x;
	dstFRect.y = (float)destRect.y;
	dstFRect.w = (float)destRect.w;
	dstFRect.h = (float)destRect.h;

	// SDL_RenderTexture dibuja la textura estirada al rectángulo de destino
	if (!SDL_RenderTexture(renderer, texture, nullptr, &dstFRect))
	{
		LOG("Cannot draw scaled texture. SDL_Error: %s", SDL_GetError());
		return false;
	}

	return true;
}

bool Render::DrawRotatedImage(SDL_Texture* texture, const SDL_Rect* dest, const SDL_Rect* section, SDL_FlipMode flip, float adjustableScale, double angle, int pivotX, int pivotY) const
{
	bool ret = true;
	float scale = Engine::GetInstance().window->GetScale();


	// SDL3 uses float rects for rendering
	SDL_FRect rect;
	rect.x = (float)((camera.x + dest->x) * scale);
	rect.y = (float)((camera.y + dest->y) * scale);
	rect.w = (float)(dest->w * scale * adjustableScale);
	rect.h = (float)(dest->h * scale * adjustableScale);

	rect.y -= rect.h;

	const SDL_FRect* src = NULL;
	SDL_FRect srcRect;
	if (section != NULL)
	{
		srcRect.x = (float)section->x;
		srcRect.y = (float)section->y;
		srcRect.w = (float)section->w;
		srcRect.h = (float)section->h;
		src = &srcRect;
	}

	SDL_FPoint* p = NULL;
	SDL_FPoint pivot;
	if (pivotX != INT_MAX && pivotY != INT_MAX)
	{
		pivot.x = (float)pivotX * scale;
		pivot.y = (float)pivotY * scale;
		p = &pivot;
	}

	// SDL3: returns bool; map to int-style check
	int rc = SDL_RenderTextureRotated(renderer, texture, src, &rect, angle, p, flip) ? 0 : -1;
	if (rc != 0)
	{
		LOG("Cannot blit to screen. SDL_RenderTextureRotated error: %s", SDL_GetError());
		ret = false;
	}

	return ret;
}

bool Render::DrawRectangle(const SDL_Rect& rect, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool filled, bool use_camera) const
{
	bool ret = true;
	int scale = Engine::GetInstance().window->GetScale();

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, r, g, b, a);

	SDL_FRect rec;
	if (use_camera)
	{
		rec.x = (float)((int)(camera.x + rect.x * scale));
		rec.y = (float)((int)(camera.y + rect.y * scale));
		rec.w = (float)(rect.w * scale);
		rec.h = (float)(rect.h * scale);
	}
	else
	{
		rec.x = (float)(rect.x * scale);
		rec.y = (float)(rect.y * scale);
		rec.w = (float)(rect.w * scale);
		rec.h = (float)(rect.h * scale);
	}

	int result = (filled ? SDL_RenderFillRect(renderer, &rec) : SDL_RenderRect(renderer, &rec)) ? 0 : -1;

	if (result != 0)
	{
		LOG("Cannot draw quad to screen. SDL_RenderFillRect/SDL_RenderRect error: %s", SDL_GetError());
		ret = false;
	}

	return ret;
}

bool Render::DrawLine(int x1, int y1, int x2, int y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool use_camera) const
{
	bool ret = true;
	int scale = Engine::GetInstance().window->GetScale();

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, r, g, b, a);

	float X1, Y1, X2, Y2;

	if (use_camera)
	{
		X1 = (float)(camera.x + x1 * scale);
		Y1 = (float)(camera.y + y1 * scale);
		X2 = (float)(camera.x + x2 * scale);
		Y2 = (float)(camera.y + y2 * scale);
	}
	else
	{
		X1 = (float)(x1 * scale);
		Y1 = (float)(y1 * scale);
		X2 = (float)(x2 * scale);
		Y2 = (float)(y2 * scale);
	}

	int result = SDL_RenderLine(renderer, X1, Y1, X2, Y2) ? 0 : -1;

	if (result != 0)
	{
		LOG("Cannot draw quad to screen. SDL_RenderLine error: %s", SDL_GetError());
		ret = false;
	}

	return ret;
}

bool Render::DrawCircle(int x, int y, int radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool use_camera) const
{
	bool ret = true;
	int scale = Engine::GetInstance().window->GetScale();

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, r, g, b, a);

	int result = -1;
	SDL_FPoint points[360];

	float factor = (float)M_PI / 180.0f;

	float cx = (float)((use_camera ? camera.x : 0) + x * scale);
	float cy = (float)((use_camera ? camera.y : 0) + y * scale);

	for (int i = 0; i < 360; ++i)
	{
		points[i].x = cx + (float)(radius * cos(i * factor));
		points[i].y = cy + (float)(radius * sin(i * factor));
	}

	result = SDL_RenderPoints(renderer, points, 360) ? 0 : -1;

	if (result != 0)
	{
		LOG("Cannot draw quad to screen. SDL_RenderPoints error: %s", SDL_GetError());
		ret = false;
	}

	return ret;
}

bool Render::DrawText(const char* text, int x, int y, int w, int h, SDL_Color color, FontType fontType) const
{
	if (text == nullptr || text[0] == '\0') return false;

	TTF_Font* selectedFont = nullptr;
	auto it = fonts.find(fontType);
	if (it != fonts.end()) selectedFont = it->second;
	else if (!fonts.empty()) selectedFont = fonts.begin()->second;

	if (!selectedFont || !renderer) {
		LOG("Render::DrawText: No hay fuente o renderer disponible.");
		return false;
	}


	SDL_Surface* surface = TTF_RenderText_Blended_Wrapped(selectedFont, text, 0, color, w > 0 ? w : 0);
	if (!surface) {
		LOG("DrawText: Error en TTF_RenderText_Blended_Wrapped: %s", SDL_GetError());
		return false;
	}

	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!texture) {
		SDL_DestroySurface(surface);
		return false;
	}

	float finalW = (float)surface->w;
	float finalH = (float)surface->h;


	if (h > 0 && surface->h > h) {
		float scale = (float)h / (float)surface->h;
		finalH = (float)h;
		finalW = (float)surface->w * scale;
	}

	SDL_FRect dstRect = { (float)x, (float)y, finalW, finalH };

	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	if (!SDL_RenderTexture(renderer, texture, nullptr, &dstRect)) {
		LOG("DrawText: Error al renderizar textura: %s", SDL_GetError());
	}

	SDL_DestroyTexture(texture);
	SDL_DestroySurface(surface);

	return true;
}

bool Render::DrawTextCentered(const char* text, const SDL_Rect& bounds, SDL_Color color, FontType fontType) const
{
	if (!renderer || !text || text[0] == '\0') return false;

	TTF_Font* selectedFont = nullptr;
	auto it = fonts.find(fontType);
	if (it != fonts.end()) {
		selectedFont = it->second;
	}
	else if (!fonts.empty()) {
		selectedFont = fonts.begin()->second;
	}

	if (!selectedFont) return false;

	SDL_Surface* surface = TTF_RenderText_Solid(selectedFont, text, 0, color);
	if (!surface) return false;

	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!texture) {
		SDL_DestroySurface(surface);
		return false;
	}

	float padding = bounds.h * 0.1f;
	float maxW = (float)bounds.w - (padding * 2);
	float maxH = (float)bounds.h - (padding * 2);

	float finalW = (float)surface->w;
	float finalH = (float)surface->h;

	float scaleX = maxW / finalW;
	float scaleY = maxH / finalH;
	float finalScale = (scaleX < scaleY) ? scaleX : scaleY;

	if (finalScale < 1.0f) {
		finalW *= finalScale;
		finalH *= finalScale;
	}

	float finalX = (float)bounds.x + ((float)bounds.w - finalW) / 2.0f;
	float finalY = (float)bounds.y + ((float)bounds.h - finalH) / 2.0f;

	SDL_FRect dstRect = { finalX, finalY, finalW, finalH };

	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	SDL_RenderTexture(renderer, texture, nullptr, &dstRect);

	SDL_DestroyTexture(texture);
	SDL_DestroySurface(surface);

	return true;
}

SDL_Rect Render::GetTextRenderedBounds(const char* text, const SDL_Rect& bounds, FontType fontType) const
{
	SDL_Rect resultRect = { 0, 0, 0, 0 };

	if (!text || text[0] == '\0') return resultRect;

	TTF_Font* selectedFont = nullptr;
	auto it = fonts.find(fontType);
	if (it != fonts.end()) {
		selectedFont = it->second;
	}
	else if (!fonts.empty()) {
		selectedFont = fonts.begin()->second;
	}

	if (!selectedFont) return resultRect;

	
	SDL_Surface* surface = TTF_RenderText_Solid(selectedFont, text, 0, { 255, 255, 255, 255 });
	if (!surface) return resultRect;

	float padding = (float)bounds.h * 0.1f;
	float maxW = (float)bounds.w - (padding * 2);
	float maxH = (float)bounds.h - (padding * 2);

	float textW = (float)surface->w;
	float textH = (float)surface->h;

	float scaleX = maxW / textW;
	float scaleY = maxH / textH;
	float finalScale = (scaleX < scaleY) ? scaleX : scaleY;

	if (finalScale < 1.0f) {
		textW *= finalScale;
		textH *= finalScale;
	}

	resultRect.x = (int)((float)bounds.x + ((float)bounds.w - textW) / 2.0f);
	resultRect.y = (int)((float)bounds.y + ((float)bounds.h - textH) / 2.0f);
	resultRect.w = (int)textW;
	resultRect.h = (int)textH;

	SDL_DestroySurface(surface);

	return resultRect;
}

void Render::SetZoom(float zoomValue)
{
	zoomLevel = zoomValue;
}

//Fade Funtions
void Render::StartFade(FadeDirection dir, float durationMs)
{
	fadeActive_ = true;
	fadeDir_ = dir;
	fadeDurationMs_ = durationMs;
	fadeElapsedMs_ = 0.0f;
	fadeAlpha_ = (dir == FadeDirection::FADE_IN) ? 255 : 0;
}

void Render::UpdateFade(float dt)
{
	if (!fadeActive_) return;

	fadeElapsedMs_ += dt;
	float t = fadeElapsedMs_ / fadeDurationMs_;
	if (t > 1.0f) t = 1.0f;

	if (fadeDir_ == FadeDirection::FADE_IN) {
		fadeAlpha_ = (Uint8)(255.0f * (1.0f - t));
	}
	else {
		fadeAlpha_ = (Uint8)(255.0f * t);
	}

	if (fadeElapsedMs_ >= fadeDurationMs_) {
		fadeActive_ = false;
	}
}

void Render::DrawFade()
{
	if (fadeAlpha_ == 0) return;

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, fadeAlpha_);
	SDL_FRect fullscreen = { 0, 0, (float)camera.w, (float)camera.h };
	SDL_RenderFillRect(renderer, &fullscreen);
}

bool Render::IsFadeComplete() const
{
	return !fadeActive_;
}

float Render::GetFadeAlpha() const
{
	return fadeAlpha_;
}
