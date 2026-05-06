#include "Cinematics.h"
#include "Engine.h"
#include "Render.h"
#include "Window.h"
#include "Input.h"
#include "Log.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

Cinematics::Cinematics() : Module()
{
	name = "cinematics";
}

Cinematics::~Cinematics()
{
	CloseVideo();
}

bool Cinematics::Awake()
{
	LOG("Cinematics module awake");
	return true;
}

bool Cinematics::Start()
{
	return true;
}

bool Cinematics::Update(float dt)
{
	if (!playing) return true;

	// Permitir al jugador saltar la cinemática
	if (skipRequested) {
		StopVideo();
		return true;
	}

	elapsedMs += dt;
	double elapsedSec = elapsedMs / 1000.0;

	// Decodificar frames hasta alcanzar el tiempo actual
	while (playing && elapsedSec >= videoFramePts) {
		if (!DecodeNextFrame()) {
			// Fin del vídeo
			StopVideo();
			return true;
		}
	}

	return true;
}

bool Cinematics::PostUpdate()
{
	if (!playing) return true;

	RenderFrame();

	return true;
}

bool Cinematics::CleanUp()
{
	CloseVideo();
	return true;
}

bool Cinematics::PlayVideo(const char* path)
{
	if (playing) {
		CloseVideo();
	}

	if (!OpenVideo(path)) {
		LOG("Cinematics: failed to open video: %s", path);
		return false;
	}

	playing = true;
	skipRequested = false;
	elapsedMs = 0.0f;
	videoFramePts = 0.0;

	LOG("Cinematics: playing %s (%dx%d)", path, videoWidth, videoHeight);

	DecodeNextFrame();

	return true;
}

void Cinematics::StopVideo()
{
	if (!playing) return;
	LOG("Cinematics: stopped");
	playing = false;
	CloseVideo();
}

bool Cinematics::IsPlaying() const
{
	return playing;
}

void Cinematics::RequestSkip()
{
	skipRequested = true;
}


bool Cinematics::OpenVideo(const char* path)
{
	if (avformat_open_input(&fmtCtx, path, nullptr, nullptr) < 0) {
		LOG("Cinematics: avformat_open_input failed");
		return false;
	}

	if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
		LOG("Cinematics: avformat_find_stream_info failed");
		CloseVideo();
		return false;
	}

	for (unsigned i = 0; i < fmtCtx->nb_streams; i++) {
		if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && videoStreamIdx < 0) {
			videoStreamIdx = (int)i;
		}
		if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audioStreamIdx < 0) {
			audioStreamIdx = (int)i;
		}
	}

	if (videoStreamIdx < 0) {
		LOG("Cinematics: no video stream found");
		CloseVideo();
		return false;
	}

	const AVCodec* vCodec = avcodec_find_decoder(fmtCtx->streams[videoStreamIdx]->codecpar->codec_id);
	if (!vCodec) {
		CloseVideo();
		return false;
	}

	videoCodecCtx = avcodec_alloc_context3(vCodec);
	avcodec_parameters_to_context(videoCodecCtx, fmtCtx->streams[videoStreamIdx]->codecpar);
	if (avcodec_open2(videoCodecCtx, vCodec, nullptr) < 0) {
		CloseVideo();
		return false;
	}

	videoWidth = videoCodecCtx->width;
	videoHeight = videoCodecCtx->height;
	timeBase = av_q2d(fmtCtx->streams[videoStreamIdx]->time_base);

	if (audioStreamIdx >= 0) {
		const AVCodec* aCodec = avcodec_find_decoder(fmtCtx->streams[audioStreamIdx]->codecpar->codec_id);
		if (aCodec) {
			audioCodecCtx = avcodec_alloc_context3(aCodec);
			avcodec_parameters_to_context(audioCodecCtx, fmtCtx->streams[audioStreamIdx]->codecpar);
			if (avcodec_open2(audioCodecCtx, aCodec, nullptr) < 0) {
				avcodec_free_context(&audioCodecCtx);
				audioCodecCtx = nullptr;
				audioStreamIdx = -1;
			}
		}
	}

	if (audioCodecCtx) {
		AVChannelLayout outLayout = AV_CHANNEL_LAYOUT_STEREO;
		int ret = swr_alloc_set_opts2(&swrCtx,
			&outLayout, AV_SAMPLE_FMT_FLT, 48000,
			&audioCodecCtx->ch_layout, audioCodecCtx->sample_fmt, audioCodecCtx->sample_rate,
			0, nullptr);
		if (ret < 0 || swr_init(swrCtx) < 0) {
			swr_free(&swrCtx);
			swrCtx = nullptr;
		}

		SDL_AudioSpec srcSpec{};
		srcSpec.format = SDL_AUDIO_F32;
		srcSpec.channels = 2;
		srcSpec.freq = 48000;

		audioDevice = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &srcSpec);
		if (audioDevice != 0) {
			audioStream = SDL_CreateAudioStream(&srcSpec, &srcSpec);
			if (audioStream) {
				SDL_BindAudioStream(audioDevice, audioStream);
				SDL_ResumeAudioDevice(audioDevice);
			}
		}
	}

	SDL_Renderer* renderer = Engine::GetInstance().render->renderer;
	videoTexture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_IYUV,
		SDL_TEXTUREACCESS_STREAMING,
		videoWidth, videoHeight);

	if (!videoTexture) {
		LOG("Cinematics: SDL_CreateTexture YUV failed: %s", SDL_GetError());
		CloseVideo();
		return false;
	}

	frame = av_frame_alloc();
	packet = av_packet_alloc();

	return true;
}

void Cinematics::CloseVideo()
{
	if (videoTexture) { SDL_DestroyTexture(videoTexture); videoTexture = nullptr; }
	if (swrCtx) { swr_free(&swrCtx); swrCtx = nullptr; }
	if (audioStream) { SDL_DestroyAudioStream(audioStream); audioStream = nullptr; }
	if (audioDevice != 0) { SDL_CloseAudioDevice(audioDevice); audioDevice = 0; }
	if (frame) { av_frame_free(&frame); frame = nullptr; }
	if (packet) { av_packet_free(&packet); packet = nullptr; }
	if (videoCodecCtx) { avcodec_free_context(&videoCodecCtx); videoCodecCtx = nullptr; }
	if (audioCodecCtx) { avcodec_free_context(&audioCodecCtx); audioCodecCtx = nullptr; }
	if (fmtCtx) { avformat_close_input(&fmtCtx); fmtCtx = nullptr; }

	videoStreamIdx = -1;
	audioStreamIdx = -1;
	videoWidth = 0;
	videoHeight = 0;
	playing = false;
}

bool Cinematics::DecodeNextFrame()
{
	while (av_read_frame(fmtCtx, packet) >= 0)
	{
		if (packet->stream_index == videoStreamIdx)
		{
			int ret = avcodec_send_packet(videoCodecCtx, packet);
			av_packet_unref(packet);
			if (ret < 0) continue;

			ret = avcodec_receive_frame(videoCodecCtx, frame);
			if (ret == AVERROR(EAGAIN)) continue;
			if (ret < 0) return false;

			if (frame->pts != AV_NOPTS_VALUE) {
				videoFramePts = frame->pts * timeBase;
			}

			SDL_UpdateYUVTexture(videoTexture, nullptr,
				frame->data[0], frame->linesize[0],
				frame->data[1], frame->linesize[1],
				frame->data[2], frame->linesize[2]);

			return true;
		}

		else if (packet->stream_index == audioStreamIdx && audioCodecCtx && swrCtx && audioStream)
		{
			int ret = avcodec_send_packet(audioCodecCtx, packet);
			av_packet_unref(packet);
			if (ret < 0) continue;

			AVFrame* aFrame = av_frame_alloc();
			while (avcodec_receive_frame(audioCodecCtx, aFrame) >= 0) {
				
				int outSamples = swr_get_out_samples(swrCtx, aFrame->nb_samples);
				int bufSize = outSamples * 2 * sizeof(float);
				uint8_t* outBuf = (uint8_t*)av_malloc(bufSize);
				
				if (outBuf) {
					int converted = swr_convert(swrCtx, &outBuf, outSamples,
						(const uint8_t**)aFrame->data, aFrame->nb_samples);
					if (converted > 0) {
						SDL_PutAudioStreamData(audioStream, outBuf, converted * 2 * sizeof(float));
					}
					av_free(outBuf);
				}
			}
			av_frame_free(&aFrame);
		}
		else {
			av_packet_unref(packet);
		}
	}

	return false;
}

void Cinematics::RenderFrame()
{
	if (!videoTexture) return;

	SDL_Renderer* renderer = Engine::GetInstance().render->renderer;

	int winW = Engine::GetInstance().render->camera.w;
	int winH = Engine::GetInstance().render->camera.h;
	float videoAspect = (float)videoWidth / (float)videoHeight;
	float windowAspect = (float)winW / (float)winH;

	SDL_FRect dst;
	if (videoAspect > windowAspect) {
		dst.w = (float)winW;
		dst.h = (float)winW / videoAspect;
		dst.x = 0;
		dst.y = ((float)winH - dst.h) / 2.0f;
	}
	else {
		dst.h = (float)winH;
		dst.w = (float)winH * videoAspect;
		dst.x = ((float)winW - dst.w) / 2.0f;
		dst.y = 0;
	}

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	SDL_RenderTexture(renderer, videoTexture, nullptr, &dst);
}