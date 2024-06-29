#define SLCODEC_INTERNAL
#include "slcodec.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

void sl_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    SLAudioDevice* pAudioDevice = (SLAudioDevice*)pDevice->pUserData;
    if (pAudioDevice == NULL || pAudioDevice->volume == 1.0f) {
        return;
    }

    ma_decoder* pDecoder = &pAudioDevice->decoder;
    float volume = pAudioDevice->volume;

    ma_uint64 framesRead;
    ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, &framesRead);

    ma_format format = pDevice->playback.format;
    ma_uint32 channels = pDevice->playback.channels;

    switch (format) {
        case ma_format_f32: {
            float* p_sample = (float*)pOutput;
            for (ma_uint32 i = 0; i < framesRead * channels; ++i) {
                p_sample[i] *= volume;
            }
            break;
        }
        case ma_format_s16: {
            int16_t* p_sample = (int16_t*)pOutput;
            for (ma_uint32 i = 0; i < framesRead * channels; ++i) {
                p_sample[i] = (int16_t)(p_sample[i] * volume);
            }
            break;
        }
        case ma_format_s24: {
            uint8_t* p_sample = (uint8_t*)pOutput;
            for (ma_uint32 i = 0; i < framesRead * channels; ++i) {
                // Convert 24-bit packed data to 32-bit for volume adjustments then combined for a single 32-bit integer
                int32_t sample = ((int32_t)p_sample[3 * i] << 8)
                    | ((int32_t)p_sample[3 * i + 1] << 16)
                    | ((int32_t)p_sample[3 * i + 2] << 24);

                sample = (int32_t)(sample * volume);
                
                // Repacking 24-bit sample
                p_sample[3 * i] = (uint8_t)(sample >> 8);
                p_sample[3 * i + 1] = (uint8_t)(sample >> 16);
                p_sample[3 * i + 2] = (uint8_t)(sample >> 24);
            }
            break;
        }
        case ma_format_s32: {
            int32_t* p_sample = (int32_t*)pOutput;
            for (ma_uint32 i = 0; i < framesRead * channels; ++i) {
                p_sample[i] = (int32_t)(p_sample[i] * volume);
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
}

SLAudioDevice* sl_setup_audio_device(const char *file, float volume)
{
    SLAudioDevice *device = (SLAudioDevice *)malloc(sizeof(SLAudioDevice));

    
    device->volume = volume;
    
    device->result = ma_decoder_init_file(file, NULL, &device->decoder);
    if (device->result != MA_SUCCESS) {
        printf("Failed to initialize decoder.\n");
    }

    device->result = ma_decoder_get_length_in_pcm_frames(&device->decoder, &device->total_frame_count);
    if (device->result != MA_SUCCESS) {
        printf("Failed to get length of the audio file.\n");
        ma_decoder_uninit(&device->decoder);
    }

    device->duration = (float)device->total_frame_count / device->decoder.outputSampleRate;

    printf("Sample Rate: %d Hz\n", device->decoder.outputSampleRate);
    printf("Channels: %d\n", device->decoder.outputChannels);
    printf("Duration: %.2f seconds\n", device->duration);
    
    const char* format_str = "Unknown";
    switch (device->decoder.outputFormat) {
        case ma_format_f32: format_str = "32-bit floating point"; break;
        case ma_format_s16: format_str = "16-bit signed integer"; break;
        case ma_format_s24: format_str = "24-bit signed integer"; break;
        case ma_format_s32: format_str = "32-bit signed integer"; break;
    }
    
    printf("Format: %s\n", format_str);

    device->device_config                   = ma_device_config_init(ma_device_type_playback);
    device->device_config.playback.format   = device->decoder.outputFormat;
    device->device_config.playback.channels = device->decoder.outputChannels;
    device->device_config.sampleRate        = device->decoder.outputSampleRate;
    device->device_config.dataCallback      = sl_data_callback;
    device->device_config.pUserData         = device;

    device->result = ma_device_init(NULL, &device->device_config, &device->device);
    if (device->result != MA_SUCCESS) {
        printf("Failed to initialize playback device.\n");
        ma_decoder_uninit(&device->decoder);
    }

    return device;
}

void sl_play(SLAudioDevice *device)
{
    device->result = ma_device_start(&device->device);
    if (device->result != MA_SUCCESS) {
        printf("Failed to start playback device.\n");
        ma_device_uninit(&device->device);
        ma_decoder_uninit(&device->decoder);
        return;
    }

    sl_sleep_seconds(device->duration);
}

void sl_free_device(SLAudioDevice *device)
{
    ma_device_uninit(&device->device);
    ma_decoder_uninit(&device->decoder);
    free(device);
}
