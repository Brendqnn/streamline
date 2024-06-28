#ifndef SLCODEC_H
#define SLCODEC_H

#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "miniaudio.h"

typedef struct {
    ma_result result;
    ma_decoder decoder;
    ma_device_config device_config;
    ma_device device;
    ma_uint64 total_frame_count;
    float duration;
} SLAudioDevice;

void sl_setup_audio_device(const char *file, SLAudioDevice *device);
void sl_play(SLAudioDevice *device);
void sl_sleep_seconds(int seconds);
void sl_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

#endif // SLCODEC_H
