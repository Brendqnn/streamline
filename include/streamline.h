#ifndef SLCODEC_H
#define SLCODEC_H

#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#include <cstdint>
#else
#include <unistd.h>
#endif

#ifdef SLCODEC_INTERNAL
#include "miniaudio.h"

typedef struct {
    ma_result result;
    ma_decoder decoder;
    ma_device_config device_config;
    ma_device device;
    ma_uint64 total_frame_count;
    
    float duration;
    float volume;
} SLAudioDevice;

#define VERSION "1.0"

SLAudioDevice* sl_setup_audio_device(const char *file, float volume);
void sl_play(SLAudioDevice *device);
void sl_sleep_seconds(int seconds);
void sl_free_device(SLAudioDevice *device);
void sl_display_version();

#else

typedef struct SLAudioDevice SLAudioDevice;

#define VERSION "1.0"

SLAudioDevice* sl_setup_audio_device(const char *file, float volume);
void sl_play(SLAudioDevice *device);
void sl_sleep_seconds(int seconds);
void sl_free_device(SLAudioDevice *device);
void sl_display_version();

#endif // SLCODEC_INTERNAL

#endif // SLCODEC_H
