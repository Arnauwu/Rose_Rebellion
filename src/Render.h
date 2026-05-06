#pragma once

#include "Module.h"
#include "Vector2D.h"
#include "SDL3/SDL.h"
#include "SDL3_ttf/SDL_ttf.h"
#include <map>

enum class FadeDirection { FADE_IN, FADE_OUT };

enum class FontType {
	MENU,
	SPEAKER,
	DIALOGUE,
	CUERPO
};

class Render : public Module
{
public:

	Render();

	// Destructor
	virtual ~Render();

	// Called before render is available
	bool Awake();

	// Called before the first frame
	bool Start();

	// Called each loop iteration
	bool PreUpdate();
	bool Update(float dt);
	bool PostUpdate();

	// Called before quitting
	bool CleanUp();

	void SetViewPort(const SDL_Rect& rect);
	void ResetViewPort();

	// Drawing
	bool DrawTexture(SDL_Texture* texture, int x, int y, const SDL_Rect* section = NULL, float speed = 1.0f, double angle = 0, int pivotX = INT_MAX, int pivotY = INT_MAX) const;
	bool DrawRotatedTexture(SDL_Texture* texture, int x, int y, const SDL_Rect* section, SDL_FlipMode flip = SDL_FLIP_NONE, float adjustableScale = 1, double angle = 0 , int pivotX = 0, int pivotY = 0) const;
	bool DrawTextureScaled(SDL_Texture* texture, const SDL_Rect& destRect) const;

	bool DrawRotatedImage(SDL_Texture* texture, const SDL_Rect* dest, const SDL_Rect* section, SDL_FlipMode flip = SDL_FLIP_NONE, float adjustableScale = 1, double angle = 0, int pivotX = 0, int pivotY = 0) const;


	bool DrawRectangle(const SDL_Rect& rect, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255, bool filled = true, bool useCamera = true) const;
	bool DrawLine(int x1, int y1, int x2, int y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255, bool useCamera = true) const;
	bool DrawCircle(int x1, int y1, int redius, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255, bool useCamera = true) const;
	
	// Method DrawText to render text on screen. Uses SDL3_ttf
	bool DrawText(const char* text, int x, int y, int w, int h, SDL_Color color, FontType fontType = FontType::MENU) const;
	bool DrawTextCentered(const char* text, const SDL_Rect& bounds, SDL_Color color, FontType fontType) const;
	SDL_Rect GetTextRenderedBounds(const char* text, const SDL_Rect& bounds, FontType fontType) const;

	// Set background color
	void SetBackgroundColor(SDL_Color color);

	// Set Texture Color Mod
	void SetColorMod(SDL_Texture* texture, Uint8* r, Uint8* g, Uint8* b, Uint8 nr, Uint8 ng, Uint8 nb);

	// See if Rect is on screen
	bool IsOnScreenWorldRect(float x, float y, float w, float h, int margin = 0) const;

	//Fade functions
	void StartFade(FadeDirection dir, float durationMs);
	void UpdateFade(float dt);
	void DrawFade();
	bool IsFadeComplete() const;
	float GetFadeAlpha() const;

public:

	SDL_Renderer* renderer;
	SDL_Rect camera;
	SDL_Rect viewport;
	SDL_Color background;

	void SetZoom(float zoomValue);
	float GetZoom() const {
		return zoomLevel;
	}

private:
	bool vsync = false;
	
	std::map<FontType, TTF_Font*> fonts;
	const char* fontPath = "Assets/Fonts/Dialogue/LibreBaskerville-VariableFont_wght.ttf";

	//Fade variables
	bool fadeActive_ = false;
	FadeDirection fadeDir_ = FadeDirection::FADE_IN;
	float fadeDurationMs_ = 500.0f;
	float fadeElapsedMs_ = 0.0f;
	float fadeAlpha_ = 0.0f;

	//zoom
	float zoomLevel = 1.0f;
};
