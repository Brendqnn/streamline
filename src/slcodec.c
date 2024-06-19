#include "../include/slcodec.h"

extern "C" {
#include "../lib/ffmpeg/include/libavformat/avformat.h"
#include "../lib/ffmpeg/include/libavcodec/avcodec.h"
#include "../lib/ffmpeg/include/libavfilter/buffersink.h"
#include "../lib/ffmpeg/include/libavfilter/buffersrc.h"
#include "../lib/ffmpeg/include/libavutil/avutil.h"
#include "../lib/ffmpeg/include/libavutil/opt.h"
#include "../lib/ffmpeg/include/libavutil/mem.h"
#include "../lib/ffmpeg/include/libavutil/channel_layout.h"
#include "../lib/ffmpeg/include/libswresample/swresample.h"
#include "../lib/ffmpeg/include/libavutil/audio_fifo.h"
#include "../lib/ffmpeg/include/libavutil/pixdesc.h"
}

#define MINIAUDIO_IMPLEMENTATION
#include "../include/miniaudio.h"

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

void sl_decode_resample(SLStreamContext *stream_ctx)
{
    AVPacket packet;
    int ret = 0;

        while (av_read_frame(stream_ctx->ifmt_ctx, &packet) == 0) {
            if (packet.stream_index == stream_ctx->stream_idx) {
                if (avcodec_send_packet(stream_ctx->dec_ctx, &packet) < 0) {
                    fprintf(stderr, "Error: Failed to send packet to decoder.\n");
                    break;
                }
                while ((ret = avcodec_receive_frame(stream_ctx->dec_ctx, stream_ctx->dec_frame)) == 0) {
                    AVFrame *resampled_frame = av_frame_alloc();
                    if (!resampled_frame) {
                        fprintf(stderr, "Error: Could not allocate resampled frame.\n");
                        break;
                    }
            
                    resampled_frame->sample_rate = stream_ctx->dec_frame->sample_rate;
                    av_channel_layout_copy(&resampled_frame->ch_layout, &stream_ctx->dec_frame->ch_layout);
                    resampled_frame->format =  AV_SAMPLE_FMT_S16;

                    swr_convert_frame(swr.swr_ctx, resampled_frame, stream_ctx->dec_frame);
                    av_audio_fifo_write(swr.buffer, (void **)resampled_frame->data, resampled_frame->nb_samples);
                    av_frame_free(&resampled_frame);
                }
            }
            av_packet_unref(&packet);
        }
}

int sl_open_input_file(const char *filename, SLStreamContext *stream_ctx)
{
    int ret;
    unsigned int i;

        if ((ret = avformat_open_input(&stream_ctx->ifmt_ctx, filename, NULL, NULL)) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
            return ret;
        }

    if ((ret = avformat_find_stream_info(stream_ctx->ifmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    for (i = 0; i < stream_ctx->ifmt_ctx->nb_streams; i++) {
        AVStream *stream = stream_ctx->ifmt_ctx->streams[i];
        const AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
        AVCodecContext *codec_ctx = avcodec_alloc_context3(dec);
    
        if (!dec) {
            av_log(NULL, AV_LOG_ERROR, "Failed to find decoder for stream #%u\n", i);
            return AVERROR_DECODER_NOT_FOUND;
        }
        if (!codec_ctx) {
            av_log(NULL, AV_LOG_ERROR, "Failed to allocate the decoder context for stream #%u\n", i);
            return AVERROR(ENOMEM);
        }
    
        ret = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to copy decoder parameters to input decoder context "
                   "for stream #%u\n", i);
            return ret;
        }
    
        if (stream_ctx->ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            stream_ctx->stream_idx = i;
        }

        codec_ctx->pkt_timebase = stream->time_base;

        /* Reencode video & audio and remux subtitles etc. */
        if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
            || codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {         
            /* if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) { */
            /*     codec_ctx->framerate = av_guess_frame_rate(stream_ctx->ifmt_ctx, stream, NULL); */
            /*     codec_ctx->bit_rate = 8500000; */
            /* } */

            if (codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
                codec_ctx->bit_rate = stream_ctx->ifmt_ctx->streams[i]->codecpar->bit_rate; // bitrate from input stream
                printf("bitrate: %d\n", codec_ctx->bit_rate);
            }
        
            ret = avcodec_open2(codec_ctx, dec, NULL);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed to open decoder for stream #%u\n", i);
                return ret;
            }
        }
        stream_ctx[i].dec_ctx = codec_ctx;

        stream_ctx[i].dec_frame = av_frame_alloc();
        if (!stream_ctx[i].dec_frame)
            return AVERROR(ENOMEM);
    }

    av_dump_format(stream_ctx->ifmt_ctx, 0, filename, 0);
    return 0;
}

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

void sl_create_output_label(const char *filename, char *output_filename)
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
}

void sl_display_version()
{
    printf("  _____ _ \n");
    printf(" / ____| |\n");
    printf("| (___ | |\n");
    printf(" \\___ \\| |\n");
    printf(" ____) | |__\n");
    printf("|_____/|_|__| \033[35mv%s\033[0m\n", VERSION);
}

void sl_get_media_file_duration(SLStreamContext *stream_ctx)
{
    int64_t total_duration = 0;
    int hours = 0;
    int mins = 0;
    int secs = 0;
    int us = 0;

        if (stream_ctx->ifmt_ctx->duration != AV_NOPTS_VALUE) {
            total_duration = stream_ctx->ifmt_ctx->duration + 5000; // add 5000 to round up
            secs = total_duration / AV_TIME_BASE;
            us = total_duration % AV_TIME_BASE;
            mins = secs / 60;
            secs %= 60;
            hours = mins / 60;
            mins %= 60;
        } else {
            fprintf(stderr, "Could not determine duration.\n");
        }

    stream_ctx->duration = mins * 60 + secs + 1;
}

void sl_setup_resampler(SLStreamContext *stream_ctx, int sample_rate, float volume)
{
    AVChannelLayout in_ch_layout;
    AVChannelLayout out_ch_layout;

        av_channel_layout_default(&in_ch_layout, stream_ctx->dec_ctx->channels);

    if (!sample_rate) {
        av_channel_layout_default(&out_ch_layout, 2); // 2 for stereo

        swr_alloc_set_opts2(&swr.swr_ctx,
                            &out_ch_layout,
                            AV_SAMPLE_FMT_S16,
                            SL_SAMPLE_RATE_48,
                            &in_ch_layout,
                            stream_ctx->dec_ctx->sample_fmt,
                            stream_ctx->dec_ctx->sample_rate,
                            0,
                            NULL);
    } else {
        av_channel_layout_default(&out_ch_layout, 2); // 2 for stereo

        swr_alloc_set_opts2(&swr.swr_ctx,
                            &out_ch_layout,
                            AV_SAMPLE_FMT_S16,
                            sample_rate,
                            &in_ch_layout,
                            stream_ctx->dec_ctx->sample_fmt,
                            stream_ctx->dec_ctx->sample_rate,
                            0,
                            NULL);
    }

    swr.volume = volume;

    sl_get_media_file_duration(stream_ctx);

    swr.buffer = av_audio_fifo_alloc(AV_SAMPLE_FMT_S16, 2, 1);
}

// Scotch tape.mp3 = 239.8

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    swr.buffer = (AVAudioFifo*)pDevice->pUserData;
    int16_t *output = (int16_t*)pOutput;
    void *output_data[] = { output };  // Create an array of void pointers

    if (swr.volume == 0) {
        av_audio_fifo_read(swr.buffer, output_data, frameCount);
    } else {
        av_audio_fifo_read(swr.buffer, output_data, frameCount);

        for (ma_uint32 i = 0; i < frameCount * pDevice->playback.channels; ++i) {
            output[i] = (int16_t)(output[i] * swr.volume);
        }
    }

    (void)pInput;
}

void sl_set_audio_playback_device(SLAudioDevice *audio_hw_device, SLStreamContext *stream_ctx)
{
    audio_hw_device->device_config = ma_device_config_init(ma_device_type_playback);

        audio_hw_device->device_config.playback.format   = ma_format_s16;
    audio_hw_device->device_config.sampleRate        = stream_ctx->dec_ctx->sample_rate;
    audio_hw_device->device_config.dataCallback      = data_callback;
    audio_hw_device->device_config.pUserData         = swr.buffer;

    if (ma_device_init(NULL, &audio_hw_device->device_config, &audio_hw_device->device) != MA_SUCCESS) {
        printf("Failed to open playback device.\n");
        return;
    }
}

void sl_free_audio(SLStreamContext *stream_ctx)
{
    if (swr.buffer != NULL) {
        av_audio_fifo_free(swr.buffer);
        swr_free(&swr.swr_ctx);
        avcodec_free_context(&stream_ctx->dec_ctx);
        av_frame_free(&stream_ctx->dec_frame);
        avformat_close_input(&stream_ctx->ifmt_ctx);
    }
}

void sleep_seconds(int seconds)
{
#ifdef _WIN32
    Sleep(seconds * 1000);
#else
    sleep(seconds);
#endif
}

void sl_play_audio_buffer(SLAudioDevice *audio_hw_device, SLStreamContext *stream_ctx)
{
    avcodec_free_context(&stream_ctx->dec_ctx);
    av_frame_free(&stream_ctx->dec_frame);
    swr_free(&swr.swr_ctx);

        if (ma_device_start(&audio_hw_device->device) != MA_SUCCESS) {
            printf("Failed to start playback device.\n");
            return;
        }

    sleep_seconds(stream_ctx->duration);
}
