#ifndef SLCODEC_H
#define SLCODEC_H

#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/mem.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/pixdesc.h>

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

    float volume;
} SLSrr;

typedef struct {
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;
    AVFilterGraph *filter_graph;

    AVPacket *enc_pkt;
    AVFrame *filtered_frame;
} SLFiltering;

typedef struct {
    AVCodecContext *dec_ctx;
    AVCodecContext *enc_ctx;

    AVFrame *dec_frame;
} SLStreamContext;

static AVFormatContext *ifmt_ctx;
static AVFormatContext *ofmt_ctx;
static SLFiltering *filter_ctx;
static SLStreamContext *stream_ctx;
static SLSrr srr = {0};

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
}

void set_encoder_properties(SLEncoder *encoder, SLDecoder *decoder, SLStream *stream)
{
    encoder->codec_ctx->bit_rate = 6500000;
    encoder->codec_ctx->width = stream->input_video_stream->codecpar->width;
    encoder->codec_ctx->height = stream->input_video_stream->codecpar->height;
    encoder->codec_ctx->pix_fmt = 0;
    encoder->codec_ctx->time_base = stream->input_video_stream->time_base;
    encoder->codec_ctx->framerate = stream->input_video_framerate;
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

static int open_input_file(const char *filename)
{
    int ret;
    unsigned int i;

    ifmt_ctx = NULL;
    if ((ret = avformat_open_input(&ifmt_ctx, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    stream_ctx = av_calloc(ifmt_ctx->nb_streams, sizeof(*stream_ctx));
    if (!stream_ctx)
        return AVERROR(ENOMEM);

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream *stream = ifmt_ctx->streams[i];
        const AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
        AVCodecContext *codec_ctx;
        if (!dec) {
            av_log(NULL, AV_LOG_ERROR, "Failed to find decoder for stream #%u\n", i);
            return AVERROR_DECODER_NOT_FOUND;
        }
        codec_ctx = avcodec_alloc_context3(dec);
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

        codec_ctx->pkt_timebase = stream->time_base;

        /* Reencode video & audio and remux subtitles etc. */
        if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
            || codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
                codec_ctx->framerate = av_guess_frame_rate(ifmt_ctx, stream, NULL);
            codec_ctx->bit_rate = 8500000;
            
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

    av_dump_format(ifmt_ctx, 0, filename, 0);
    return 0;
}

static int open_output_file(const char *filename)
{
    AVStream *out_stream;
    AVStream *in_stream;
    AVCodecContext *dec_ctx, *enc_ctx;
    const AVCodec *encoder;
    int ret;
    unsigned int i;

    ofmt_ctx = NULL;
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, filename);
    if (!ofmt_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
        return AVERROR_UNKNOWN;
    }


    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        out_stream = avformat_new_stream(ofmt_ctx, NULL);
        if (!out_stream) {
            av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
            return AVERROR_UNKNOWN;
        }

        in_stream = ifmt_ctx->streams[i];
        dec_ctx = stream_ctx[i].dec_ctx;

        if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
            || dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            encoder = avcodec_find_encoder(dec_ctx->codec_id);
            if (!encoder) {
                av_log(NULL, AV_LOG_FATAL, "Necessary encoder not found\n");
                return AVERROR_INVALIDDATA;
            }
            enc_ctx = avcodec_alloc_context3(encoder);
            if (!enc_ctx) {
                av_log(NULL, AV_LOG_FATAL, "Failed to allocate the encoder context\n");
                return AVERROR(ENOMEM);
            }
            
            if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
                enc_ctx->height = dec_ctx->height;
                enc_ctx->width = dec_ctx->width;
                enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
                if (encoder->pix_fmts)
                    enc_ctx->pix_fmt = encoder->pix_fmts[0];
                else
                    enc_ctx->pix_fmt = dec_ctx->pix_fmt;
                enc_ctx->time_base = av_inv_q(dec_ctx->framerate);
            } else {
                enc_ctx->sample_rate = dec_ctx->sample_rate;
                ret = av_channel_layout_copy(&enc_ctx->ch_layout, &dec_ctx->ch_layout);
                if (ret < 0)
                    return ret;
                
                enc_ctx->sample_fmt = encoder->sample_fmts[0];
                enc_ctx->time_base = (AVRational){1, enc_ctx->sample_rate};
            }

            if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

            ret = avcodec_open2(enc_ctx, encoder, NULL);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Cannot open %s encoder for stream #%u\n", encoder->name, i);
                return ret;
            }
            ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output stream #%u\n", i);
                return ret;
            }

            out_stream->time_base = enc_ctx->time_base;
            stream_ctx[i].enc_ctx = enc_ctx;
        } else if (dec_ctx->codec_type == AVMEDIA_TYPE_UNKNOWN) {
            av_log(NULL, AV_LOG_FATAL, "Elementary stream #%d is of unknown type, cannot proceed\n", i);
            return AVERROR_INVALIDDATA;
        } else {
            ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Copying parameters for stream #%u failed\n", i);
                return ret;
            }
            out_stream->time_base = in_stream->time_base;
        }

    }
    av_dump_format(ofmt_ctx, 0, filename, 1);

    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'", filename);
            return ret;
        }
    }

    /* init muxer, write output file header */
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
        return ret;
    }

    return 0;
}

static int init_filter(SLFiltering* fctx, AVCodecContext *dec_ctx,
                       AVCodecContext *enc_ctx, const char *filter_spec)
{
    char args[512];
    int ret = 0;
    
    const AVFilter *buffersrc = NULL;
    const AVFilter *buffersink = NULL;
    
    AVFilterContext *buffersrc_ctx = NULL;
    AVFilterContext *buffersink_ctx = NULL;
    
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    
    AVFilterGraph *filter_graph = avfilter_graph_alloc();

    if (!outputs || !inputs || !filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
        buffersrc = avfilter_get_by_name("buffer");
        buffersink = avfilter_get_by_name("buffersink");
        if (!buffersrc || !buffersink) {
            av_log(NULL, AV_LOG_ERROR, "filtering source or sink element not found\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }

        snprintf(args, sizeof(args),
                 "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
                 dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
                 dec_ctx->pkt_timebase.num, dec_ctx->pkt_timebase.den,
                 dec_ctx->sample_aspect_ratio.num,
                 dec_ctx->sample_aspect_ratio.den);

        ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                           args, NULL, filter_graph);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
            goto end;
        }

        ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                           NULL, NULL, filter_graph);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
            goto end;
        }

        ret = av_opt_set_bin(buffersink_ctx, "pix_fmts",
                             (uint8_t*)&enc_ctx->pix_fmt, sizeof(enc_ctx->pix_fmt),
                             AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
            goto end;
        }
    } else if (dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
        char buf[64];
        buffersrc = avfilter_get_by_name("abuffer");
        buffersink = avfilter_get_by_name("abuffersink");
        if (!buffersrc || !buffersink) {
            av_log(NULL, AV_LOG_ERROR, "filtering source or sink element not found\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }

        if (dec_ctx->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC)
            av_channel_layout_default(&dec_ctx->ch_layout, dec_ctx->ch_layout.nb_channels);
        av_channel_layout_describe(&dec_ctx->ch_layout, buf, sizeof(buf));
        snprintf(args, sizeof(args),
                 "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%s",
                 dec_ctx->pkt_timebase.num, dec_ctx->pkt_timebase.den, dec_ctx->sample_rate,
                 av_get_sample_fmt_name(dec_ctx->sample_fmt),
                 buf);
        ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                           args, NULL, filter_graph);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source\n");
            goto end;
        }

        ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                           NULL, NULL, filter_graph);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer sink\n");
            goto end;
        }

        ret = av_opt_set_bin(buffersink_ctx, "sample_fmts",
                             (uint8_t*)&enc_ctx->sample_fmt, sizeof(enc_ctx->sample_fmt),
                             AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot set output sample format\n");
            goto end;
        }

        av_channel_layout_describe(&enc_ctx->ch_layout, buf, sizeof(buf));
        ret = av_opt_set(buffersink_ctx, "ch_layouts",
                         buf, AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot set output channel layout\n");
            goto end;
        }

        ret = av_opt_set_bin(buffersink_ctx, "sample_rates",
                             (uint8_t*)&enc_ctx->sample_rate, sizeof(enc_ctx->sample_rate),
                             AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot set output sample rate\n");
            goto end;
        }
    } else {
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    inputs->name       = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    if (!outputs->name || !inputs->name) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_spec,
                                        &inputs, &outputs, NULL)) < 0)
        goto end;

    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
        goto end;

    fctx->buffersrc_ctx = buffersrc_ctx;
    fctx->buffersink_ctx = buffersink_ctx;
    fctx->filter_graph = filter_graph;

 end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}

static int init_filters(void)
{
    const char *filter_spec;
    unsigned int i;
    int ret;
    filter_ctx = av_malloc_array(ifmt_ctx->nb_streams, sizeof(*filter_ctx));
    if (!filter_ctx)
        return AVERROR(ENOMEM);

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        filter_ctx[i].buffersrc_ctx  = NULL;
        filter_ctx[i].buffersink_ctx = NULL;
        filter_ctx[i].filter_graph   = NULL;
        if (!(ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO
              || ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO))
            continue;


        if (ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            filter_spec = "null"; /* passthrough (dummy) filter for video */
        else
            filter_spec = "anull"; /* passthrough (dummy) filter for audio */
        ret = init_filter(&filter_ctx[i], stream_ctx[i].dec_ctx,
                          stream_ctx[i].enc_ctx, filter_spec);
        if (ret)
            return ret;

        filter_ctx[i].enc_pkt = av_packet_alloc();
        if (!filter_ctx[i].enc_pkt)
            return AVERROR(ENOMEM);

        filter_ctx[i].filtered_frame = av_frame_alloc();
        if (!filter_ctx[i].filtered_frame)
            return AVERROR(ENOMEM);
    }
    return 0;
}

static int encode_write_frame(unsigned int stream_index, int flush)
{
    SLStreamContext *stream = &stream_ctx[stream_index];
    SLFiltering *filter = &filter_ctx[stream_index];
    AVFrame *filt_frame = flush ? NULL : filter->filtered_frame;
    AVPacket *enc_pkt = filter->enc_pkt;
    int ret;

    av_log(NULL, AV_LOG_INFO, "Encoding frame\n");
    av_packet_unref(enc_pkt);

    if (filt_frame && filt_frame->pts != AV_NOPTS_VALUE)
        filt_frame->pts = av_rescale_q(filt_frame->pts, filt_frame->time_base,
                                       stream->enc_ctx->time_base);

    ret = avcodec_send_frame(stream->enc_ctx, filt_frame);

    if (ret < 0)
        return ret;

    while (ret >= 0) {
        ret = avcodec_receive_packet(stream->enc_ctx, enc_pkt);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return 0;

        /* prepare packet for muxing */
        enc_pkt->stream_index = stream_index;
        av_packet_rescale_ts(enc_pkt,
                             stream->enc_ctx->time_base,
                             ofmt_ctx->streams[stream_index]->time_base);

        av_log(NULL, AV_LOG_DEBUG, "Muxing frame\n");
        /* mux encoded frame */
        ret = av_interleaved_write_frame(ofmt_ctx, enc_pkt);
    }

    return ret;
}

static int filter_encode_write_frame(AVFrame *frame, unsigned int stream_index)
{
    SLFiltering *filter = &filter_ctx[stream_index];
    int ret;

    av_log(NULL, AV_LOG_INFO, "Pushing decoded frame to filters\n");
    ret = av_buffersrc_add_frame_flags(filter->buffersrc_ctx,
                                       frame, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
        return ret;
    }

    while (1) {
        av_log(NULL, AV_LOG_INFO, "Pulling filtered frame from filters\n");
        ret = av_buffersink_get_frame(filter->buffersink_ctx,
                                      filter->filtered_frame);
        if (ret < 0) {

            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                ret = 0;
            break;
        }

        filter->filtered_frame->time_base = av_buffersink_get_time_base(filter->buffersink_ctx);;
        filter->filtered_frame->pict_type = AV_PICTURE_TYPE_NONE;
        ret = encode_write_frame(stream_index, 0);
        av_frame_unref(filter->filtered_frame);
        if (ret < 0)
            break;
    }

    return ret;
}

static int flush_encoder(unsigned int stream_index)
{
    if (!(stream_ctx[stream_index].enc_ctx->codec->capabilities &
          AV_CODEC_CAP_DELAY))
        return 0;

    av_log(NULL, AV_LOG_INFO, "Flushing stream #%u encoder\n", stream_index);
    return encode_write_frame(stream_index, 1);
}

#endif // SLCODEC_H
