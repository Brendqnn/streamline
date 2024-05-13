#ifndef SLSTREAM_H
#define SLSTREAM_H

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>


typedef struct {
    AVFormatContext *input_format_ctx;
    AVFormatContext *output_format_ctx;

    AVStream *input_video_stream;
    AVStream *input_audio_stream;
    
    AVStream *output_video_stream;
    AVStream *output_audio_stream;

    int audio_stream_index;
    int video_stream_index;

    AVRational input_video_framerate;

    bool file_type_video;

    AVPacket packet;
    AVFrame *frame;
} SLStream;

void open_input(const char *file, SLStream *stream)
{
    if (stream == NULL) {
        fprintf(stderr, "Stream is NULL. Exiting...\n");
        return;
    }

    stream->video_stream_index = -1;
    stream->audio_stream_index = -1;
    
    if (avformat_open_input(&stream->input_format_ctx, file, NULL, NULL) < 0) {
        fprintf(stderr, "Error: Cannot open input media file.\n");
        return;
    }

    if (avformat_find_stream_info(stream->input_format_ctx, NULL) < 0) {
        fprintf(stderr, "Error: Could not find audio stream in input file.\n");
        return;
    }

    for (int i = 0; i < stream->input_format_ctx->nb_streams; ++i) {
        AVStream *stream_temp = stream->input_format_ctx->streams[i];
        if (stream_temp->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            stream->video_stream_index = i;
            stream->file_type_video = true;
        } else if (stream_temp->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            stream->audio_stream_index = i;
        }        
    }

    stream->input_video_stream = stream->input_format_ctx->streams[stream->video_stream_index];
    stream->input_audio_stream = stream->input_format_ctx->streams[stream->audio_stream_index];
    
    if (stream->video_stream_index == -1) {
        return;
    }

    if (stream->audio_stream_index == -1) {
        return;
    }
}

#endif // SLSTERAM_H
