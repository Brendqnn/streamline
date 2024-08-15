#ifndef STREAMLINE_H
#define STREAMLINE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef STREAMLINE_INTERNAL
#include <miniaudio.h>

typedef struct {
    const char *file;
    
    ma_decoder decoder;
    ma_device pcm_device;
    ma_device_config device_config;
    ma_result result;
    ma_uint64 total_frame_count;
    
    float volume;
    float duration;
    float fade_duration;
    
    int fade;    
    clock_t fade_start_time;

    bool loop;
} SLAudioDevice;

#define VERSION "0.1"

void sl_initialize_audio_device(SLAudioDevice *pdevice, char *file, float volume, bool loop);
void sl_fade_audio_in(SLAudioDevice *pdevice, float fade_in_duration);
void sl_play_audio(SLAudioDevice *pdevice);
void sl_sleep_seconds(int seconds);
void sl_destroy_device(SLAudioDevice *pdevice);
void sl_display_version();

#else

typedef struct SLAudioDevice SLAudioDevice;

#define VERSION "0.1"

void sl_initialize_audio_device(SLAudioDevice *pdevice, char *file, float volume, bool loop)
void sl_fade_audio_in(SLAudioDevice *pdevice, float fade_in_duration);
void sl_play_audio(SLAudioDevice *pdevice);
void sl_sleep_seconds(int seconds);
void sl_destroy_device(SLAudioDevice *pdevice);
void sl_display_version();

#endif

#endif
