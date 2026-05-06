#pragma once

#include "Module.h"
#include <SDL3/SDL.h>
#include <string>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwrContext;

class Cinematics : public Module
{
public:

	Cinematics();
	virtual ~Cinematics();

	bool Awake() override;
	bool Start() override;
	bool Update(float dt) override;
	bool PostUpdate() override;
	bool CleanUp() override;

	bool PlayVideo(const char* path);

	void StopVideo();

	bool IsPlaying() const;

	void RequestSkip();

private:

	// Internal helpers
	bool OpenVideo(const char* path);
	void CloseVideo();
	bool DecodeNextFrame();
	void RenderFrame();

	// FFmpeg state
	AVFormatContext* fmtCtx = nullptr;
	AVCodecContext* videoCodecCtx = nullptr;
	AVFrame* frame = nullptr;
	AVPacket* packet = nullptr;

	AVCodecContext* audioCodecCtx = nullptr;
	SwrContext* swrCtx = nullptr;
	SDL_AudioStream* audioStream = nullptr;

	int videoStreamIdx = -1;
	int audioStreamIdx = -1;

	// SDL rendering
	SDL_Texture* videoTexture = nullptr;
	int videoWidth = 0;
	int videoHeight = 0;

	// Playback timing
	bool playing = false;
	double videoFramePts = 0.0;
	double timeBase = 0.0;
	float  elapsedMs = 0.0f;

	// Skip control
	bool skipRequested = false;

	// Audio device
	SDL_AudioDeviceID audioDevice = 0;
};