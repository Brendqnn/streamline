#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
#define PCM_DEVICE "default"
#define BUFFER_SIZE_RATIO 4

void print_version()
{
    printf("streamline version %s\n", VERSION);
}

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    AVAudioFifo *fifo = (AVAudioFifo*)pDevice->pUserData;
    av_audio_fifo_read(fifo, &pOutput, frameCount);

    (void)pInput;
}

void open_input(const char *file, AVFormatContext **format_ctx, AVStream **stream, int *stream_index)
{
    if (avformat_open_input(format_ctx, file, NULL, NULL) < 0) {
        fprintf(stderr, "Error: Cannot open input media file.\n");
        return;
    }

    if (avformat_find_stream_info(*format_ctx, NULL) < 0) {
        fprintf(stderr, "Error: Could not find audio stream in input file.\n");
        return;
    }

    for (int i = 0; i < (*format_ctx)->nb_streams; ++i) {
        if ((*format_ctx)->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            *stream_index = i;
            break;
        }
    }
    
    *stream = (*format_ctx)->streams[*stream_index];
    
    if (*stream_index == -1) {
        fprintf(stderr, "Error: No audio stream found in input file.\n");
        return;
    }
}

void open_decoder(const AVCodec **codec, AVCodecContext **codec_ctx, AVStream *input_stream)
{
    *codec = avcodec_find_decoder(input_stream->codecpar->codec_id);
    if (!codec) {
        fprintf(stderr, "Error: Unsupported codec.\n");
        return;
    }
    
    *codec_ctx = avcodec_alloc_context3(*codec);
    if (!codec_ctx) {
        fprintf(stderr, "Error: Failed to allocate codec context.\n");
        return;
    }
    
    if (avcodec_parameters_to_context(*codec_ctx, input_stream->codecpar) < 0) {
        fprintf(stderr, "Error: Failed to copy codec parameters to context.\n");
        return;
    }
    
    if (avcodec_open2(*codec_ctx, *codec, NULL) < 0) {
        fprintf(stderr, "Error: Failed to open codec.\n");
        return;
    }
}

void setup_resampler(SwrContext **swr, AVStream *stream, AVCodecContext *codec_ctx)
{
    *swr = swr_alloc();

    swr_alloc_set_opts2(swr,         // we're allocating a new context
                      &(AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO, // out_ch_layout
                      AV_SAMPLE_FMT_FLT,    // out_sample_fmt
                      44100,                // out_sample_rate
                      &(AVChannelLayout)AV_CHANNEL_LAYOUT_5POINT1, // in_ch_layout
                      codec_ctx->sample_fmt,   // in_sample_fmt
                      48000,                // in_sample_rate
                      0,                    // log_offset
                      NULL); 
}

int main(int argc, char *argv[])
{
    const char *filename;

    if (argc != 2 || strstr(argv[1], ".mp3") == NULL) {
        fprintf(stderr, "Usage: %s <input_file.mp3>\n", argv[0]);
        return 1;
    }
    
    filename = argv[1];
    
    AVFormatContext *format_ctx = NULL;
    AVCodecContext *codec_ctx = NULL;
    AVStream *input_stream = NULL;
    const AVCodec *codec = NULL;
    SwrContext *swr_ctx = NULL;
    int ret = 0;
    
    int stream_idx = -1;
    AVPacket packet;
    AVFrame *frame = av_frame_alloc();

    open_input(filename, &format_ctx, &input_stream, &stream_idx);
    open_decoder(&codec, &codec_ctx, input_stream);
    setup_resampler(&swr_ctx, input_stream, codec_ctx);

    AVAudioFifo *buffer = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLT, 2, 1);

    while (av_read_frame(format_ctx, &packet) == 0) {
        if (packet.stream_index == stream_idx) {
            if (avcodec_send_packet(codec_ctx, &packet) < 0) {
                fprintf(stderr, "Error: Failed to send packet to decoder.\n");
                break;
            }
            while ((ret = avcodec_receive_frame(codec_ctx, frame)) == 0) {
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    fprintf(stderr, "Error: Failed to retreve frame from decoder.\n");
                    break;
                }

                AVFrame *resampled_frame = av_frame_alloc();
                resampled_frame->sample_rate = frame->sample_rate;
                resampled_frame->ch_layout = frame->ch_layout;
                resampled_frame->format = AV_SAMPLE_FMT_FLT;
                
                swr_convert_frame(swr_ctx, resampled_frame, frame);
                av_audio_fifo_write(buffer, (void**)resampled_frame->data, resampled_frame->nb_samples);
            }
        }
    }

    av_packet_unref(&packet);

    av_frame_free(&frame);
    swr_free(&swr_ctx);

    ma_device_config deviceConfig;
    ma_device device;
    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = ma_format_f32;
    deviceConfig.sampleRate        = input_stream->codecpar->sample_rate;
    deviceConfig.dataCallback      = data_callback;
    deviceConfig.pUserData         = buffer;


    if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
        printf("Failed to open playback device.\n");
        return -3;
    }

    if (ma_device_start(&device) != MA_SUCCESS) {
        printf("Failed to start playback device.\n");
        ma_device_uninit(&device);
        return -4;
    }

    getchar();

    av_audio_fifo_free(buffer);

    return 0;
}
