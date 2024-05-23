#ifndef SLUTIL_H
#define SLUTIL_H

#include "slstream.h"
#include "slcodec.h"

#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>

#define MINIAUDIO_IMPLEMENTATION
#include "../lib/miniaudio/miniaudio.h"

#define VERSION "1.0"

typedef struct {
    ma_device device;
    ma_device_config device_config;
} SLAudioDevice;

enum SLFileType {
    SL_MEDIA_AUDIO,
    SL_MEDIA_VIDEO
};

struct {
    enum SLFileType type;
    const char *extension;
} file_type_map[] = {
    // video formats
    {SL_MEDIA_VIDEO, ".mp4"},
    {SL_MEDIA_VIDEO, ".avi"},
    {SL_MEDIA_VIDEO, ".mov"},
    {SL_MEDIA_VIDEO, ".wmv"},
    {SL_MEDIA_VIDEO, ".m4v"},
    // audio formats
    {SL_MEDIA_AUDIO, ".mp3"},
    {SL_MEDIA_AUDIO, ".aac"}
};

int is_valid_file(const char *filename)
{
    int valid = 0;
    const char *file_extension = strrchr(filename, '.');
    if (file_extension != NULL) {
        for (int i = 0; i < sizeof(file_type_map) / sizeof(file_type_map[0]); ++i) {
            if (strcmp(file_extension, file_type_map[i].extension) == 0) {
                valid = 1;
                break;
            }
        }
    }
    return valid;
}

void create_output_label(const char *filename, char *output_filename)
{
    const char *tag = "[Streamline]";
    const char *slash = strrchr(filename, '/');
    
    if (slash != NULL) {
        size_t index = slash - filename + 1;
        strncpy(output_filename, filename, index);
        output_filename[index] = '\0';
        strcat(output_filename, tag);
        strcat(output_filename, slash + 1);
    } else {
        strcpy(output_filename, tag);
        strcat(output_filename, filename);
    }

    printf("output filename: %s\n", output_filename);
}

void display_version()
{
    printf("  _____ _ \n");
    printf(" / ____| |\n");
    printf("| (___ | |\n");
    printf(" \\___ \\| |\n");
    printf(" ____) | |__\n");
    printf("|_____/|_|__| v%s\n", VERSION);
}

void setup_resampler(SLSrr *srr, SLDecoder *codec, int sample_rate, float volume)
{
    int default_sample_rate = 48000;    
    if (!sample_rate) {
        srr->srr_ctx = swr_alloc();

        swr_alloc_set_opts2(&srr->srr_ctx,
                            &(AVChannelLayout)AV_CHANNEL_LAYOUT_7POINT1,
                            AV_SAMPLE_FMT_FLT,
                            default_sample_rate,
                            &(AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO,
                            codec->codec_ctx->sample_fmt,
                            48000,
                            0,                    
                            NULL);
        printf("Using default sample rate: %d\n", default_sample_rate);
    } else {
        swr_alloc_set_opts2(&srr->srr_ctx,
                            &(AVChannelLayout)AV_CHANNEL_LAYOUT_7POINT1,
                            AV_SAMPLE_FMT_FLT,
                            sample_rate,
                            &(AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO,
                            codec->codec_ctx->sample_fmt,
                            48000,
                            0,                    
                            NULL);
    }

    srr->volume = volume;
}

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    srr.buffer = (AVAudioFifo*)pDevice->pUserData;
    float *output = (float*)pOutput;
    av_audio_fifo_read(srr.buffer, &pOutput, frameCount);

    for (ma_uint32 i = 0; i < frameCount * pDevice->playback.channels; ++i) {
        output[i] *= srr.volume;
    }

    (void)pInput;
}

void set_audio_playback_device(SLAudioDevice *audio_hw_device, SLStream *stream, SLSrr *srr)
{
    audio_hw_device->device_config = ma_device_config_init(ma_device_type_playback);
    
    audio_hw_device->device_config.playback.format   = ma_format_f32;
    audio_hw_device->device_config.sampleRate        = stream->input_audio_stream->codecpar->sample_rate;
    audio_hw_device->device_config.dataCallback      = data_callback;
    audio_hw_device->device_config.pUserData         = srr->buffer;

    if (ma_device_init(NULL, &audio_hw_device->device_config, &audio_hw_device->device) != MA_SUCCESS) {
        printf("Failed to open playback device.\n");
        return;
    }
}

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

void sleep_seconds(int seconds) {
    #ifdef _WIN32
    Sleep(seconds * 1000);
    #else
    sleep(seconds);
    #endif
}

void play_audio_buffer(SLStream *stream, SLAudioDevice *audio_hw_device, SLSrr *srr)
{
    int64_t duration = 0;
    int hours, mins, secs, us;
    
    if (stream->input_format_ctx->duration != AV_NOPTS_VALUE) {
        duration = stream->input_format_ctx->duration + 5000; // add 5000 to round up
        secs = duration / AV_TIME_BASE;
        us = duration % AV_TIME_BASE;
        mins = secs / 60;
        secs %= 60;
        hours = mins / 60;
        mins %= 60;
        fprintf(stdout, "Duration: %02d:%02d:%02d.%02d\n", hours, mins, secs,
                (100 * us) / AV_TIME_BASE);
    } else {
        fprintf(stderr, "Could not determine duration\n");
    }

    int total_seconds = mins * 60 + secs + 1; // just padding the milliseconds with 1
    printf("%d\n", total_seconds);

    if (ma_device_start(&audio_hw_device->device) != MA_SUCCESS) {
        printf("Failed to start playback device.\n");
        return;
    }
    
    sleep_seconds(total_seconds);

    av_audio_fifo_free(srr->buffer);
}

#endif // SLUTIL_H
