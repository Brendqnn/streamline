#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "slutil.h"
#include "slstream.h"
#include "slcodec.h"


void free_compressor_data(AVFormatContext *ifmt_ctx, AVFormatContext *ofmt_ctx, SLFiltering *filter_ctx, SLStreamContext *stream_ctx, AVPacket *packet)
{
    int i = 0;
    av_packet_free(&packet);
    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        avcodec_free_context(&stream_ctx[i].dec_ctx);
        if (ofmt_ctx && ofmt_ctx->nb_streams > i && ofmt_ctx->streams[i] && stream_ctx[i].enc_ctx)
            avcodec_free_context(&stream_ctx[i].enc_ctx);
        if (filter_ctx && filter_ctx[i].filter_graph) {
            avfilter_graph_free(&filter_ctx[i].filter_graph);
            av_packet_free(&filter_ctx[i].enc_pkt);
            av_frame_free(&filter_ctx[i].filtered_frame);
        }

        av_frame_free(&stream_ctx[i].dec_frame);
    }
    av_free(filter_ctx);
    av_free(stream_ctx);
    avformat_close_input(&ifmt_ctx);
    if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);
}

int main(int argc, char **argv)
{
    int ret;
    AVPacket *packet = NULL;
    unsigned int stream_index;
    unsigned int i;
    int compress = 0;
    char output_filename[100];

    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        display_version();
        return 0;
    }

    if (argc > 1 && strcmp(argv[1], "-c") == 0) {
        compress = 1;
        argv++; // Skip the compression flag
        argc--; // Adjust the argument count
    }

    if (compress == 1 && argc == 2 && is_valid_file(argv[1])) {
        open_input_file(argv[1]);
        create_output_label(argv[1], output_filename);
        open_output_file(output_filename);
        init_filters();
        packet = av_packet_alloc();

        while (1) {
            if ((ret = av_read_frame(ifmt_ctx, packet)) < 0)
                break;
            stream_index = packet->stream_index;
            av_log(NULL, AV_LOG_DEBUG, "Demuxer gave frame of stream_index %u\n",
                   stream_index);

            if (filter_ctx[stream_index].filter_graph) {
                SLStreamContext *stream = &stream_ctx[stream_index];

                av_log(NULL, AV_LOG_DEBUG, "Going to reencode&filter the frame\n");

                ret = avcodec_send_packet(stream->dec_ctx, packet);
                if (ret < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Decoding failed\n");
                    break;
                }

                while (ret >= 0) {
                    ret = avcodec_receive_frame(stream->dec_ctx, stream->dec_frame);
                    if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                        break;

                    stream->dec_frame->pts = stream->dec_frame->best_effort_timestamp;
                    ret = filter_encode_write_frame(stream->dec_frame, stream_index);
                }
            } else {
                av_packet_rescale_ts(packet,
                                     ifmt_ctx->streams[stream_index]->time_base,
                                     ofmt_ctx->streams[stream_index]->time_base);

                ret = av_interleaved_write_frame(ofmt_ctx, packet);
            }
            av_packet_unref(packet);
        }

        for (i = 0; i < ifmt_ctx->nb_streams; i++) {
            SLStreamContext *stream;

            if (!filter_ctx[i].filter_graph)
                continue;

            stream = &stream_ctx[i];

            av_log(NULL, AV_LOG_INFO, "Flushing stream %u decoder\n", i);

            ret = avcodec_send_packet(stream->dec_ctx, NULL);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Flushing decoding failed\n");
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(stream->dec_ctx, stream->dec_frame);
                if (ret == AVERROR_EOF)
                    break;

                stream->dec_frame->pts = stream->dec_frame->best_effort_timestamp;
                ret = filter_encode_write_frame(stream->dec_frame, i);
            }

            ret = filter_encode_write_frame(NULL, i);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Flushing filter failed\n");
            }

            ret = flush_encoder(i);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Flushing encoder failed\n");
            }
        }

        av_write_trailer(ofmt_ctx);

        free_compressor_data(ifmt_ctx, ofmt_ctx, filter_ctx, stream_ctx, packet);
    } else {
        SLStream stream = {0};
        SLDecoder decoder = {0};
        SLEncoder encoder = {0};

        SLAudioDevice audio_device = {0};

        open_input(argv[1], &stream);
        open_decoder(&decoder, &stream);
        setup_resampler(&srr, &decoder, 192000, 0);
        srr.buffer = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLT, 2, 1);
        decode(&stream, &decoder, &encoder, &srr);

        set_audio_playback_device(&audio_device, &stream, &srr);

        play_audio_buffer(&stream, &audio_device, &srr);
    }

    return 0;
}
