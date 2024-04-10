#ifndef SLCODEC_H
#define SLCODEC_H

#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>

#include "slutil.h"
#include "slstream.h"

typedef struct {
    const AVCodec *codec;
    AVCodecContext *codec_ctx;
} SLDecoder;

typedef struct {
    const AVCodec *codec;
    AVCodecContext *codec_ctx;
} SLEncoder;

typedef struct {
    SwrContext *srr_ctx;
    AVAudioFifo *buffer;
} SLSrr;

void open_decoder(SLDecoder *decoder, SLStream *stream)
{
    if (stream->file_type_video) {
        decoder->codec = avcodec_find_decoder(stream->input_video_stream->codecpar->codec_id);

        decoder->codec_ctx = avcodec_alloc_context3(decoder->codec);
        if (!decoder->codec_ctx) {
            fprintf(stderr, "Error: Failed to allocate codec context.\n");
            return;
        }
        if (avcodec_parameters_to_context(decoder->codec_ctx, stream->input_video_stream->codecpar) < 0) {
            fprintf(stderr, "Error: Failed to copy codec parameters to context.\n");
            return;
        }
        if (avcodec_open2(decoder->codec_ctx, decoder->codec, NULL) < 0) {
            fprintf(stderr, "Error: Failed to open codec.\n");
            return;
        }
    } else {
        decoder->codec = avcodec_find_decoder(stream->input_audio_stream->codecpar->codec_id);

        decoder->codec_ctx = avcodec_alloc_context3(decoder->codec);
        if (!decoder->codec_ctx) {
            fprintf(stderr, "Error: Failed to allocate codec context.\n");
            return;
        }
        if (avcodec_parameters_to_context(decoder->codec_ctx, stream->input_audio_stream->codecpar) < 0) {
            fprintf(stderr, "Error: Failed to copy codec parameters to context.\n");
            return;
        }
        if (avcodec_open2(decoder->codec_ctx, decoder->codec, NULL) < 0) {
            fprintf(stderr, "Error: Failed to open codec.\n");
            return;
        }
    }

    decoder->codec_ctx->thread_type = FF_THREAD_FRAME; // Use a single decoding thread
    decoder->codec_ctx->thread_count = 1;
}

void set_encoder_properties(SLEncoder *encoder, SLDecoder *decoder, SLStream *stream)
{
    encoder->codec_ctx->bit_rate = 4000000;
    encoder->codec_ctx->width = stream->input_video_stream->codecpar->width;
    encoder->codec_ctx->height = stream->input_video_stream->codecpar->height;
    encoder->codec_ctx->pix_fmt = 0;
    encoder->codec_ctx->time_base = stream->input_video_stream->time_base;
    encoder->codec_ctx->framerate = stream->input_video_framerate;

    //encoder->codec_ctx->thread_type = FF_THREAD_FRAME; // Use a single decoding thread
    //encoder->codec_ctx->thread_count = 1;
}

void copy_audio_parameters(SLStream *stream)
{
    stream->output_audio_stream = avformat_new_stream(stream->output_format_ctx, NULL);
    avcodec_parameters_copy(stream->output_audio_stream->codecpar, stream->input_audio_stream->codecpar);
}

void alloc_output_ctx(const char *filename, SLStream *stream, SLEncoder *encoder)
{
    avformat_alloc_output_context2(&stream->output_format_ctx, NULL, NULL, filename);
    stream->output_video_stream = avformat_new_stream(stream->output_format_ctx, NULL);
    stream->output_video_stream->time_base = stream->input_video_stream->time_base;

    if (avcodec_parameters_from_context(stream->output_video_stream->codecpar, encoder->codec_ctx) < 0) {
        fprintf(stderr, "Error: Failed to copy codec parameters from the input stream.\n");
        return;
    }
}

void write_file_header(const char *filename, SLStream *stream)
{
    if (!(stream->output_format_ctx->flags & AVFMT_NOFILE)) {
        if (avio_open(&stream->output_format_ctx->pb, filename, AVIO_FLAG_WRITE) < 0) {
            fprintf(stderr, "Error: Failed to open the output file.\n");
            return;
        }
    }
    if (avformat_write_header(stream->output_format_ctx, NULL) < 0) {
        fprintf(stderr, "Error: Failed to write file header to output file.\n");
        return;
    }
}

void open_encoder(SLEncoder *encoder, SLDecoder *decoder, SLStream *stream)
{
    encoder->codec = avcodec_find_encoder(stream->input_format_ctx->streams[stream->video_stream_index]->codecpar->codec_id);
    encoder->codec_ctx = avcodec_alloc_context3(encoder->codec);
    set_encoder_properties(encoder, decoder, stream);
    if (avcodec_open2(encoder->codec_ctx, encoder->codec, NULL) < 0) {
        fprintf(stderr, "Error: Failed to open encoder.\n");
        return;
    }
}

void encode(SLStream *stream, SLDecoder *decoder, SLEncoder *encoder)
{
    if (stream->frame == NULL) {
        stream->frame = av_frame_alloc();
    }
    
    int ret = 0;
    
    while (av_read_frame(stream->input_format_ctx, &stream->packet) == 0) {
        if (stream->packet.stream_index == stream->video_stream_index) {
            if (avcodec_send_packet(decoder->codec_ctx, &stream->packet) < 0) {
                fprintf(stderr, "Error: Failed to send packet to decoder.\n");
                break;
            }
            while ((ret = avcodec_receive_frame(decoder->codec_ctx, stream->frame)) == 0) {
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    fprintf(stderr, "Error: Failed to retreve frame from decoder.\n");
                    break;
                }
                if (avcodec_send_frame(encoder->codec_ctx, stream->frame) < 0 ) {
                    fprintf(stderr, "Error: Failed to send frame to encoder\n");
                    break;
                }
                while (avcodec_receive_packet(encoder->codec_ctx, &stream->packet) >= 0) {
                    if (av_interleaved_write_frame(stream->output_format_ctx, &stream->packet) < 0) {
                        fprintf(stderr, "Error: Failed to write packet to output context.\n");
                        break;
                    }
                }
            }
        } else if (stream->packet.stream_index == stream->audio_stream_index) {
            if (av_interleaved_write_frame(stream->output_format_ctx, &stream->packet) < 0) {
                fprintf(stderr, "Error: Failed to write audio packet to output file.\n");
                return;
            }
        }
        av_packet_unref(&stream->packet);
    }
    av_write_trailer(stream->output_format_ctx);
}

void decode(SLStream *stream, SLDecoder *decoder, SLEncoder *encoder, SLSrr *srr)
{
    if (stream->frame == NULL) {
        stream->frame = av_frame_alloc();
    }
    
    //int stream_index = 0;
    int ret = 0;
    
     while (av_read_frame(stream->input_format_ctx, &stream->packet) == 0) {
        if (stream->packet.stream_index == stream->audio_stream_index) {
            if (avcodec_send_packet(decoder->codec_ctx, &stream->packet) < 0) {
                fprintf(stderr, "Error: Failed to send packet to decoder.\n");
                break;
            }
            while ((ret = avcodec_receive_frame(decoder->codec_ctx, stream->frame)) == 0) {
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    fprintf(stderr, "Error: Failed to retreve frame from decoder.\n");
                    break;
                }

                AVFrame *resampled_frame = av_frame_alloc();
                resampled_frame->sample_rate = stream->frame->sample_rate;
                resampled_frame->ch_layout = stream->frame->ch_layout;
                resampled_frame->format = AV_SAMPLE_FMT_FLT;
                
                swr_convert_frame(srr->srr_ctx, resampled_frame, stream->frame);
                av_audio_fifo_write(srr->buffer, (void**)resampled_frame->data, resampled_frame->nb_samples);
            }
        }
    }
}

#endif // SLCODEC_H
