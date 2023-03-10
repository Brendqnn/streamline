#include "sl_codec.h"
#include <iostream>

SLcodec::SLcodec()
    : video_decoder(nullptr)
    , video_decoder_ctx(nullptr)
    , video_encoder(nullptr)
    , video_encoder_ctx(nullptr)
{
    
}

SLcodec::~SLcodec() {
    if (video_decoder_ctx) {
        avcodec_free_context(&video_decoder_ctx);
        video_decoder_ctx = NULL;
    }
    if (video_encoder_ctx) {
        avcodec_free_context(&video_encoder_ctx);
        video_encoder_ctx = NULL;
    }
}

void SLcodec::open_decoder_ctx(std::shared_ptr<SLstream> _stream) {
    video_decoder = avcodec_find_decoder(_stream->input_video_stream->codecpar->codec_id);
    video_decoder_ctx = avcodec_alloc_context3(video_decoder);
    if (avcodec_parameters_to_context(video_decoder_ctx, _stream->input_video_stream->codecpar) < 0) {
        std::cout << "failed to copy video codec context to parameters\n";
    }
    if (avcodec_open2(video_decoder_ctx, video_decoder, nullptr) < 0) {
        std::cout << "failed to open video decoder codec\n";
    }
}

void SLcodec::set_encoder_properties(std::shared_ptr<SLstream> _stream) {
    video_encoder_ctx->bit_rate = 8000000;
    video_encoder_ctx->width = video_decoder_ctx->width;
    video_encoder_ctx->height = video_decoder_ctx->height;
    video_encoder_ctx->pix_fmt = video_encoder->pix_fmts[0];
    video_encoder_ctx->time_base = _stream->input_video_stream->time_base;
    video_encoder_ctx->framerate = _stream->input_video_framerate;
}

void SLcodec::open_encoder_ctx(std::shared_ptr<SLstream> _stream) {
    video_encoder = avcodec_find_encoder(AV_CODEC_ID_HEVC);
    video_encoder_ctx = avcodec_alloc_context3(video_encoder);
    set_encoder_properties(_stream);
    if (avcodec_open2(video_encoder_ctx, video_encoder, nullptr) < 0) {
        std::cout << "Error: could not open codec for output file" << std::endl;
    }
}

void SLcodec::copy_audio_parameters(std::shared_ptr<SLiomanager> _ctx, std::shared_ptr<SLstream> _stream) {
    _stream->output_audio_stream = avformat_new_stream(_ctx->output_ctx, NULL);
    if (!_stream->output_audio_stream) {
        std::cout << "no audio stream found" << std::endl;
    }
    avcodec_parameters_copy(_stream->output_audio_stream->codecpar, _stream->input_audio_stream->codecpar);
    
}

void SLcodec::alloc_output_ctx(std::shared_ptr<SLiomanager> _ctx, std::shared_ptr<SLstream> _stream) {
    avformat_alloc_output_context2(&_ctx->output_ctx, nullptr, nullptr, "res/compressed-indigo.mp4");
    _stream->output_video_stream = avformat_new_stream(_ctx->output_ctx, NULL);
    _stream->output_video_stream->time_base = _stream->input_video_stream->time_base;
    if (!_stream->output_video_stream) {
        std::cout << "Error: could not allocate output video stream" << std::endl;

    }
    if (avcodec_parameters_from_context(_stream->output_video_stream->codecpar, video_encoder_ctx) < 0) {
        std::cout << "Error copying codec parameters" << std::endl;
    }
    copy_audio_parameters(_ctx, _stream);
}

