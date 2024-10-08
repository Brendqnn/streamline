#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define STREAMLINE_INTERNAL
#include "streamline.h"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

void sl_fade_audio_in(SLAudioDevice *pdevice, float fade_in_duration)
{
    pdevice->fade = 0;

    if (fade_in_duration > 0.0f) {
        pdevice->fade = 1;
        pdevice->fade_duration = fade_in_duration;
    }
}

float sl_fade_audio(SLAudioDevice *pdevice)
{
    static clock_t start = 0;
    float volume;
    clock_t now;
    double elapsed;
    double remaining;
    double fade_in_duration;
    float current_volume = 0.0f;

    if (start == 0) {
        start = clock();
    }

    volume = pdevice->volume;
    
    now = clock();
    elapsed = (double)(now - start) / CLOCKS_PER_SEC;
    fade_in_duration = pdevice->fade_duration;
    remaining = pdevice->duration - elapsed;
    
    if (elapsed < fade_in_duration) {
        current_volume = (float)(volume * log10(1 + 9 * (elapsed / fade_in_duration)));
    } else {
        current_volume = volume;
    }

    return current_volume;
}

void sl_audio_data_playback_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{    
    SLAudioDevice* pAudioDevice;
    ma_decoder* pDecoder;
    float volume;
    ma_uint64 framesRead;
    ma_format format;
    ma_uint32 channels;
    float current_volume = 0.0f;

    pAudioDevice = (SLAudioDevice*)pDevice->pUserData;
    if (pAudioDevice == NULL) {
        fprintf(stderr, "Error: audio device is null\n");
        return;
    }

    pDecoder = &pAudioDevice->decoder;
    volume = pAudioDevice->volume;

    ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, &framesRead);

    format = pDevice->playback.format;
    channels = pDevice->playback.channels;

    if (pAudioDevice->fade) {
        current_volume = sl_fade_audio(pAudioDevice);
    } else {
        current_volume = volume;
    }

    switch (format) {
        case ma_format_f32: {
            float* p_sample = (float*)pOutput;
            ma_uint32 i;
            for (i = 0; i < framesRead * channels; ++i) {
                p_sample[i] *= current_volume;
            }
            break;
        }
        case ma_format_s16: {
            short* p_sample = (short*)pOutput;
            ma_uint32 i;
            for (i = 0; i < framesRead * channels; ++i) {
                p_sample[i] = (short)(p_sample[i] * current_volume);
            }
            break;
        }
        case ma_format_s24: {
            unsigned char* p_sample = (unsigned char*)pOutput;
            ma_uint32 i;
            for (i = 0; i < framesRead * channels; ++i) {
                long sample = ((long)p_sample[3 * i] << 8)
                    | ((long)p_sample[3 * i + 1] << 16)
                    | ((long)p_sample[3 * i + 2] << 24);

                sample = (long)(sample * current_volume);
                
                p_sample[3 * i] = (unsigned char)(sample >> 8);
                p_sample[3 * i + 1] = (unsigned char)(sample >> 16);
                p_sample[3 * i + 2] = (unsigned char)(sample >> 24);
            }
            break;
        }
        case ma_format_s32: {
            long* p_sample = (long*)pOutput;
            ma_uint32 i;
            for (i = 0; i < framesRead * channels; ++i) {
                p_sample[i] = (long)(p_sample[i] * current_volume);
            }
            break;
        }
        default: {
            break;
        }
    }
}
 
void sl_sleep_seconds(int seconds)
{
#ifdef _WIN32
    Sleep(seconds * 1000);
#else
    sleep(seconds);
#endif
}

void sl_display_version()
{
    printf("  _____ _ \n");
    printf(" / ____| |\n");
    printf("| (___ | |\n");
    printf(" \\___ \\| |\n");
    printf(" ____) | |__\n");
    printf("|_____/|_|__| v%s\n", VERSION);

    printf("\n");
}

void sl_initialize_audio_device(SLAudioDevice *pdevice, char *file, float volume, bool loop)
{
    memset(pdevice, 0, sizeof(SLAudioDevice));

    if (access(file, F_OK) == 0) {
        pdevice->file = file;
    } else {
        fprintf(stderr, "Error: file path %s, does not exist.\n");
    }
    
    pdevice->volume = volume;
    if (pdevice->volume == 1.0f) {
        pdevice->volume = 0.99f;
    }

    pdevice->fade = 0;
    pdevice->loop = loop;
    
    pdevice->result = ma_decoder_init_file(file, NULL, &pdevice->decoder);
    if (pdevice->result != MA_SUCCESS) {
        printf("Failed to initialize decoder.\n");
        return;
    }

    pdevice->result = ma_decoder_get_length_in_pcm_frames(&pdevice->decoder, &pdevice->total_frame_count);
    if (pdevice->result != MA_SUCCESS) {
        printf("Failed to get length of the audio file.\n");
        ma_decoder_uninit(&pdevice->decoder);
    }

    pdevice->duration = (float)pdevice->total_frame_count / pdevice->decoder.outputSampleRate;

    printf("Sample Rate: %d Hz\n", pdevice->decoder.outputSampleRate);
    printf("Channels: %d\n", pdevice->decoder.outputChannels);
    printf("Duration: %.2f seconds\n", pdevice->duration);

    const char *format_str;
    format_str = "Unknown";
    switch (pdevice->decoder.outputFormat) {
        case ma_format_f32: format_str = "32-bit floating point"; break;
        case ma_format_s16: format_str = "16-bit signed integer"; break;
        case ma_format_s24: format_str = "24-bit signed integer"; break;
        case ma_format_s32: format_str = "32-bit signed integer"; break;
    }
    
    printf("Format: %s\n", format_str);

    pdevice->device_config = ma_device_config_init(ma_device_type_playback);
    pdevice->device_config.playback.format = pdevice->decoder.outputFormat;
    pdevice->device_config.playback.channels = pdevice->decoder.outputChannels;
    pdevice->device_config.sampleRate = pdevice->decoder.outputSampleRate;
    pdevice->device_config.dataCallback = sl_audio_data_playback_callback;
    pdevice->device_config.pUserData = pdevice;

    pdevice->result = ma_device_init(NULL, &pdevice->device_config, &pdevice->pcm_device);
    if (pdevice->result != MA_SUCCESS) {
        printf("Failed to initialize playback device.\n");
        ma_decoder_uninit(&pdevice->decoder);
    }
}

void sl_play_audio(SLAudioDevice *pdevice)
{
    do {
        if (pdevice->fade) {
            pdevice->fade_start_time = clock(); 
        }

        pdevice->result = ma_device_start(&pdevice->pcm_device);
        if (pdevice->result != MA_SUCCESS) {
            printf("Failed to start playback device.\n");
            ma_device_uninit(&pdevice->pcm_device);
            ma_decoder_uninit(&pdevice->decoder);
            return;
        }

        sl_sleep_seconds((int)pdevice->duration);
        
        ma_device_stop(&pdevice->pcm_device);
        ma_decoder_seek_to_pcm_frame(&pdevice->decoder, 0);

    } while (pdevice->loop);
}

void sl_destroy_device(SLAudioDevice *pdevice)
{
    ma_device_uninit(&pdevice->pcm_device);
    ma_decoder_uninit(&pdevice->decoder);
    pdevice = 0;
}
