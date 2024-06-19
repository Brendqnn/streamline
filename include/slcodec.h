#ifndef SLCODEC_H
#define SLCODEC_H

#ifdef _WIN32
#include <windows.h>
#include <cstdint> 
#else
#include <unistd.h>
#endif

#define VERSION "1.0"

#define SL_AUDIO_BUFFER_SIZE 4096

#define SL_CHANNEL_LAYOUT_STEREO AV_CH_LAYOUT_STEREO
#define SL_CHANNEL_LAYOUT_7POINT1 AV_CH_LAYOUT_7POINT1
#define SL_CHANNEL_LAYOUT_5POINT1 AV_CH_LAYOUT_5POINT1

#define SL_SAMPLE_RATE_44 44100
#define SL_SAMPLE_RATE_48 48000
#define SL_SAMPLE_RATE_96 96000
#define SL_SAMPLE_RATE_176 176400
#define SL_SAMPLE_RATE_192 192000

#include "miniaudio.h"

typedef struct AVFormatContext AVFormatContext;
typedef struct AVCodecContext AVCodecContext;
typedef struct AVStream AVStream;
typedef struct AVPacket AVPacket;
typedef struct AVFrame AVFrame;
typedef struct SwrContext SwrContext;
typedef struct AVAudioFifo AVAudioFifo;
typedef struct AVFilterContext AVFilterContext;
typedef struct AVFilterGraph AVFilterGraph;
typedef struct AVChannelLayout AVChannelLayout;

typedef struct {
    SwrContext *swr_ctx;
    AVAudioFifo *buffer;
    float volume;
} SLSwr;

/* typedef struct { */
/*     AVFilterContext *buffersink_ctx; */
/*     AVFilterContext *buffersrc_ctx; */
/*     AVFilterGraph *filter_graph; */
/*     AVPacket *enc_pkt; */
/*     AVFrame *filtered_frame; */
/* } SLFiltering; */

typedef struct {
    AVCodecContext *dec_ctx;
    AVCodecContext *enc_ctx;
    AVFormatContext *ifmt_ctx;
    AVFormatContext *ofmt_ctx;
    unsigned int stream_idx;
    AVFrame *dec_frame;
    int64_t duration;
} SLStreamContext;

typedef struct {
    struct ma_device device;
    struct ma_device_config device_config;
} SLAudioDevice;

enum SLFileType {
    SL_MEDIA_AUDIO,
    SL_MEDIA_VIDEO
};

static SLSwr swr = {0};

void sl_decode_resample(SLStreamContext *stream_ctx);
int sl_open_input_file(const char *filename, SLStreamContext *stream_ctx);
int is_valid_file(const char *filename);
void sl_create_output_label(const char *filename, char *output_filename);
void sl_display_version();
void sl_setup_resampler(SLStreamContext *stream_ctx, int sample_rate, float volume);
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
void sl_set_audio_playback_device(SLAudioDevice *audio_hw_device, SLStreamContext *stream_ctx);
void sl_free_audio(SLStreamContext *stream_ctx);
void sleep_seconds(int seconds);
void sl_play_audio_buffer(SLAudioDevice *audio_hw_device, SLStreamContext *stream_ctx);

#endif // SLCODEC_H
