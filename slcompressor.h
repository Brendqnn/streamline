#pragma once
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

    void open_input_media(const char* filename);
    void find_media_stream();
    void open_decoder_context();
    void set_input_stream();
    void open_encoder_context();
    void copy_audio_parameters();
    void write_file_header();
    void read_frames();
    void close_resources();

private:
    void alloc_output_context();
    void set_encoder_properties();

    AVFormatContext* input_context;
    AVFormatContext* output_context;

    const AVCodec* video_decoder;
    AVCodecContext* video_decoder_context;

    const AVCodec* video_encoder;
    AVCodecContext* video_encoder_context;

    int video_stream_index;
    int audio_stream_index;

    AVStream* input_video_stream;
    AVStream* input_audio_stream;
    AVStream* output_video_stream;
    AVStream* output_audio_stream;

    AVRational input_video_framerate;

    AVFrame* frame;

    AVPacket packet;

    const char* filename;
};
