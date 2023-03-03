#include "slinput.h"
#include <iostream>

SLinput::SLinput()
    : input_ctx(nullptr)
    , output_ctx(nullptr)
    , video_decoder(nullptr)
    , video_decoder_ctx(nullptr)
    , video_encoder(nullptr)
    , video_encoder_ctx(nullptr)
    , input_video_stream(nullptr)
    , input_audio_stream(nullptr)
    , output_audio_stream(nullptr)
    , output_video_stream(nullptr)
    , video_stream_idx(-1)
    , audio_stream_idx(-1)
    , input_video_framerate({ 0, 0 })
    , filename(filename)
{
    filename = "res/indigo.mp4";
}
	
SLinput::~SLinput() {

}

void SLinput::open_media_input() {
	if (avformat_open_input(&input_ctx, filename, nullptr, nullptr) < 0) {
		std::cout << "could not find input media\n";
		avformat_close_input(&input_ctx);
	}
    find_media_streams();
    setup_input_streams();
}

void SLinput::find_media_streams() {
    if (avformat_find_stream_info(input_ctx, nullptr) < 0) {
        std::cout << "error finding input stream\n";
        avformat_close_input(&input_ctx);
    }
    for (unsigned int i = 0; i < input_ctx->nb_streams; i++) {
        if (input_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
        }
        else if (input_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
        }
    }
    if (video_stream_idx == -1) {
        std::cout << "video stream index not found" << std::endl;
    }
    if (audio_stream_idx == -1) {
        std::cout << "audio stream index not found" << std::endl;
    }
}

void SLinput::setup_input_streams() {
    input_video_stream = input_ctx->streams[video_stream_idx];
    input_audio_stream = input_ctx->streams[audio_stream_idx];
}

void SLinput::open_decoder_ctx() {
    setup_input_streams();
    video_decoder = avcodec_find_decoder(input_video_stream->codecpar->codec_id);
    video_decoder_ctx = avcodec_alloc_context3(video_decoder);
    if (avcodec_parameters_to_context(video_decoder_ctx, input_ctx->streams[video_stream_idx]->codecpar) < 0) {
        std::cout << "failed to copy video codec context to parameters\n";
        avformat_close_input(&input_ctx);
        avcodec_free_context(&video_decoder_ctx);
    }
    if (avcodec_open2(video_decoder_ctx, video_decoder, nullptr) < 0) {
        std::cout << "failed to open video decoder codec\n";
        avformat_close_input(&input_ctx);
        avcodec_free_context(&video_decoder_ctx);
    }
}

void SLinput::set_encoder_properties() {
    video_encoder_ctx->bit_rate = video_decoder_ctx->bit_rate;
    video_encoder_ctx->width = video_decoder_ctx->width;
    video_encoder_ctx->height = video_decoder_ctx->height;
    video_encoder_ctx->pix_fmt = video_encoder->pix_fmts[0];
    video_encoder_ctx->time_base = input_video_stream->time_base;
    video_encoder_ctx->framerate = input_video_framerate;
}

void SLinput::alloc_output_ctx() {
    avformat_alloc_output_context2(&output_ctx, nullptr, nullptr, "res/compressed-indigo.mp4");
    output_video_stream = avformat_new_stream(output_ctx, NULL);
    output_video_stream->time_base = input_video_stream->time_base;
    if (!output_video_stream) {
        std::cout << "Error: could not allocate output video stream" << std::endl;
        avcodec_free_context(&video_decoder_ctx);
        avcodec_free_context(&video_encoder_ctx);
        avformat_close_input(&input_ctx);
        avformat_free_context(output_ctx);
    }
    if (avcodec_parameters_from_context(output_video_stream->codecpar, video_encoder_ctx) < 0) {
        std::cout << "Error copying codec parameters" << std::endl;
        avcodec_free_context(&video_decoder_ctx);
        avcodec_free_context(&video_encoder_ctx);
        avformat_close_input(&input_ctx);
        avformat_free_context(output_ctx);
    }
}

void SLinput::open_encoder_ctx() {
    video_encoder = avcodec_find_encoder(AV_CODEC_ID_H265);
    video_encoder_ctx = avcodec_alloc_context3(video_encoder);
    set_encoder_properties();
    if (avcodec_open2(video_encoder_ctx, video_encoder, nullptr) < 0) {
        std::cout << "Error: could not open codec for output file" << std::endl;
        avcodec_free_context(&video_decoder_ctx);
        avformat_close_input(&input_ctx);
    }
    alloc_output_ctx();
    copy_audio_parameters();
}

void SLinput::copy_audio_parameters() {
    output_audio_stream = avformat_new_stream(output_ctx, NULL);
    if (!output_audio_stream) {
        std::cout << "no audio stream found" << std::endl;
        avcodec_free_context(&video_decoder_ctx);
        avcodec_free_context(&video_encoder_ctx);
        avformat_close_input(&input_ctx);
        avformat_free_context(output_ctx);
    }
    avcodec_parameters_copy(output_audio_stream->codecpar, input_audio_stream->codecpar);
}

void SLinput::write_file_header() {
    if (!(output_ctx->flags & AVFMT_NOFILE)) {
        if (avio_open(&output_ctx->pb, "res/compressed-indigo.mp4", AVIO_FLAG_WRITE) < 0) {
            std::cout << "Error: could not open output file" << std::endl;
            avcodec_free_context(&video_decoder_ctx);
            avcodec_free_context(&video_encoder_ctx);
            avformat_close_input(&input_ctx);
            avformat_free_context(output_ctx);
        }
    }
    if (avformat_write_header(output_ctx, nullptr) < 0) {
        std::cout << "Error: could not write header to output file" << std::endl;
        avcodec_free_context(&video_decoder_ctx);
        avcodec_free_context(&video_encoder_ctx);
        avio_closep(&output_ctx->pb);
        avformat_close_input(&input_ctx);
        avformat_free_context(output_ctx);
    }
}


