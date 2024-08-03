#ifndef STREAMLINE_H
#define STREAMLINE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef STREAMLINE_INTERNAL
#include "miniaudio.h"

typedef struct {
    ma_result result;
    ma_decoder decoder;
    ma_device_config device_config;
    ma_device pcm_device;
    ma_uint64 total_frame_count;

    bool fade;
    
    float duration;
    float volume;
} SLAudioDevice;

#define VERSION "1.0"

SLAudioDevice *sl_setup_audio_device(const char *file, float volume);
float sl_audio_fade_in(SLAudioDevice *pdevice);
void sl_play(SLAudioDevice *pdevice);
void sl_sleep_seconds(int seconds);
void sl_free_device(SLAudioDevice *pdevice);
void sl_display_version();

#else

typedef struct SLAudioDevice SLAudioDevice;

#define VERSION "1.0"

SLAudioDevice* sl_setup_audio_device(const char *file, float volume);
float sl_audio_fade_in(SLAudioDevice *pdevice);
void sl_play(SLAudioDevice *pdevice);
void sl_sleep_seconds(int seconds);
void sl_free_device(SLAudioDevice *pdevice);
void sl_display_version();

#endif

#endif
