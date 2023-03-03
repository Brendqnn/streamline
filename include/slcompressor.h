#pragma once
#include "slinput.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
}


class SLcompressor {
public:
    SLcompressor();
    ~SLcompressor();

    void start_compress();
private:
    void close_resources();
    SLinput input;

    AVFormatContext* input_ctx_ = input.input_ctx;
    AVFormatContext* output_ctx_ = input.output_ctx;

    int video_stream_idx_ = input.video_stream_idx;
    int audio_stream_idx_ = input.audio_stream_idx;

    AVCodecContext* video_decoder_ctx_ = input.video_decoder_ctx;
    AVCodecContext* video_encoder_ctx_ = input.video_encoder_ctx;

    AVFrame* frame;

    AVPacket packet;
};
