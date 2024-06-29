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

#include "miniaudio.h"

#define VERSION "1.0"

typedef ma_result ma_result;
typedef ma_decoder ma_decoder;
typedef ma_device_config ma_device_config;
typedef ma_device ma_device;
typedef ma_uint64 ma_uint64;

typedef struct {
    ma_result result;
    ma_decoder decoder;
    ma_device_config device_config;
    ma_device device;
    ma_uint64 total_frame_count;
    
    float duration;
    float volume;
} SLAudioDevice;

void sl_setup_audio_device(const char *file, SLAudioDevice *device, float volume);
void sl_play(SLAudioDevice *device);
void sl_sleep_seconds(int seconds);
void sl_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
void sl_free_device(SLAudioDevice *device);
void sl_display_version();

#endif // SLCODEC_H
