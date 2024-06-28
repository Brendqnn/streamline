#include "slcodec.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

void sl_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
    if (pDecoder == NULL) {
        return;
    }

    ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, NULL);
}

void sl_sleep_seconds(int seconds)
{
#ifdef _WIN32
    Sleep(seconds * 1000);
#else
    sleep(seconds);
#endif
}

void sl_setup_audio_device(const char *file, SLAudioDevice *device)
{
    device->result = ma_decoder_init_file(file, NULL, &device->decoder);
    if (device->result != MA_SUCCESS) {
        printf("Failed to initialize decoder.\n");
        return;
    }

    device->total_frame_count;
    device->result = ma_decoder_get_length_in_pcm_frames(&device->decoder, &device->total_frame_count);
    if (device->result != MA_SUCCESS) {
        printf("Failed to get length of the audio file.\n");
        ma_decoder_uninit(&device->decoder);
        return;
    }

    device->duration = (float)device->total_frame_count / device->decoder.outputSampleRate;

    device->device_config                   = ma_device_config_init(ma_device_type_playback);
    device->device_config.playback.format   = device->decoder.outputFormat;
    device->device_config.playback.channels = device->decoder.outputChannels;
    device->device_config.sampleRate        = device->decoder.outputSampleRate;
    device->device_config.dataCallback      = sl_data_callback;
    device->device_config.pUserData         = &device->decoder;

    device->result = ma_device_init(NULL, &device->device_config, &device->device);
    if (device->result != MA_SUCCESS) {
        printf("Failed to initialize playback device.\n");
        ma_decoder_uninit(&device->decoder);
        return;
    }
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

    ma_device_uninit(&device->device);
    ma_decoder_uninit(&device->decoder);
}
