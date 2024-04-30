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
        //size_t index = tag - filename;
        //strncpy(output_filename, filename, index);
        strncpy(output_filename, filename, 0);
        output_filename = '\0';
        strcat(output_filename, tag);
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

void setup_resampler(SLSrr *srr, SLDecoder *codec)
{
    srr->srr_ctx = swr_alloc();

    swr_alloc_set_opts2(&srr->srr_ctx,         // we're allocating a new context
                      &(AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO, // out_ch_layout
                      AV_SAMPLE_FMT_FLT,    // out_sample_fmt
                      44100,                // out_sample_rate
                      &(AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO, // in_ch_layout
                      codec->codec_ctx->sample_fmt,   // in_sample_fmt
                      48000,                // in_sample_rate
                      0,                    // log_offset
                      NULL); 
}

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    AVAudioFifo *fifo = (AVAudioFifo*)pDevice->pUserData;
    av_audio_fifo_read(fifo, &pOutput, frameCount);

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

void play_audio_buffer(SLStream *stream, SLAudioDevice *audio_hw_device, SLSrr *srr)
{
    if (ma_device_start(&audio_hw_device->device) != MA_SUCCESS) {
        printf("Failed to start playback device.\n");
        return;
    }

    getchar(); // idk what else to do

    av_audio_fifo_free(srr->buffer);
}

#endif // SLUTIL_H
