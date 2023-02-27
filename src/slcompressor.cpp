#include "slcompressor.h"
#include <iostream>

SLcompressor::SLcompressor()
    : input_ctx(nullptr)
    , output_ctx(nullptr)
    , video_decoder(nullptr)
    , video_decoder_context(nullptr)
    , video_encoder(nullptr)
    , video_encoder_context(nullptr)
    , video_stream_index(-1)
    , audio_stream_index(-1)
    , input_video_stream(nullptr)
    , input_audio_stream(nullptr)
    , output_video_stream(nullptr)
    , output_audio_stream(nullptr)
    , input_video_framerate({ 0, 0 })
    , frame(nullptr)
    , packet()
    , filename(filename)
{

}

void SLcompressor::open_input_media(const char* filename) {
    if (avformat_open_input(&input_ctx, filename, nullptr, nullptr) < 0) {
        throw std::runtime_error("Failed to open input ctx");
        avformat_close_input(&input_ctx);
    }
}

void SLcompressor::find_media_stream() {
    if (avformat_find_stream_info(input_ctx, nullptr) >= 0) {
        std::cout << "Error reading packet data from the input media stream\n";
        avformat_close_input(&input_ctx);
    }
    for (unsigned int i = 0; i < input_ctx->nb_streams; i++) {
        if (input_ctx->streams[i]->codecpar->codec_id == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
        }
        else if (input_ctx->streams[i]->codecpar->codec_id == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
        }
    }
    if (video_stream_index == -1) {
        std::cout << "Error finding video stream index\n";
        avformat_close_input(&input_ctx);
    }
    if (audio_stream_index == -1) {
        std::cout << "Error finding audio stream index\n";
        avformat_close_input(&input_ctx);
    }
    
}

void SLcompressor::setup_input_streams() {
    input_video_stream = input_ctx->streams[video_stream_index];
    input_audio_stream = input_ctx->streams[audio_stream_index];
}

void SLcompressor::open_decoder() {
    video_decoder = avcodec_find_decoder(input_ctx->streams[video_stream_index]->codecpar->codec_id);
    video_decoder_context = avcodec_alloc_context3(video_decoder);
    if (avcodec_parameters_to_context(video_decoder_context, input_ctx->streams[video_stream_index]->codecpar) < 0) {
        std::cout << "failed to copy video codec context to parameters\n";
        avformat_close_input(&input_ctx);
        avcodec_free_context(&video_decoder_context);
    }
    if (avcodec_open2(video_decoder_context, video_decoder, nullptr) < 0) {
        std::cout << "failed to open video decoder codec\n";
        avformat_close_input(&input_ctx);
        avcodec_free_context(&video_decoder_context);
    }
}

void SLcompressor::setup_encoder_properties() {
    video_encoder_context->bit_rate = video_decoder_context->bit_rate;
    video_encoder_context->width = video_decoder_context->width;
    video_encoder_context->height = video_decoder_context->height;
    video_encoder_context->pix_fmt = video_encoder->pix_fmts[0];
    video_encoder_context->time_base = input_video_stream->time_base;
    video_encoder_context->framerate = input_video_framerate;
}

void SLcompressor::open_encoder() {
    video_decoder = avcodec_find_decoder(input_ctx->streams[video_stream_index]->codecpar->codec_id);
    video_decoder_context = avcodec_alloc_context3(video_decoder);
    setup_encoder_properties();
    if (avcodec_open2(video_encoder_context, video_encoder, nullptr) < 0) {
        std::cout << "Error: could not open codec for output file" << std::endl;
        avformat_close_input(&input_ctx);
        avcodec_free_context(&video_decoder_context);
        avcodec_free_context(&video_encoder_context);
    }
    alloc_output_ctx();
}

void SLcompressor::alloc_output_ctx() {
    avformat_alloc_output_context2(&output_ctx, nullptr, nullptr, "");
    output_video_stream = avformat_new_stream(output_ctx, NULL);
    output_video_stream->time_base = input_video_stream->time_base;
    if (!output_video_stream) {
        std::cout << "Error: could not allocate output video stream" << std::endl;
        avcodec_free_context(&video_decoder_context);
        avcodec_free_context(&video_encoder_context);
        avformat_close_input(&input_ctx);
        avformat_free_context(output_ctx);
    }
    if (avcodec_parameters_from_context(output_video_stream->codecpar, video_encoder_context) < 0) {
        std::cout << "Error copying codec parameters" << std::endl;
        avcodec_free_context(&video_decoder_context);
        avcodec_free_context(&video_encoder_context);
        avformat_close_input(&input_ctx);
        avformat_free_context(output_ctx);
    }
}

void SLcompressor::copy_audio_parameters() {
    output_audio_stream = avformat_new_stream(output_ctx, NULL);
    if (!output_audio_stream) {
        std::cout << "no audio stream found" << std::endl;
    }
    avcodec_parameters_copy(output_audio_stream->codecpar, input_audio_stream->codecpar);
}

void SLcompressor::write_file_header() {
    if (!(output_ctx->flags & AVFMT_NOFILE)) {
        if (avio_open(&output_ctx->pb, "res/compressed-indigo.mp4", AVIO_FLAG_WRITE) < 0) {
            std::cout << "Error: could not open output file" << std::endl;
            avcodec_free_context(&video_decoder_context);
            avcodec_free_context(&video_encoder_context);
            avformat_close_input(&input_ctx);
            avformat_free_context(output_ctx);
        }
    }
    if (avformat_write_header(output_ctx, nullptr) < 0) {
        std::cout << "Error: could not write header to output file" << std::endl;
        avcodec_free_context(&video_decoder_context);
        avcodec_free_context(&video_encoder_context);
        avio_closep(&output_ctx->pb);
        avformat_close_input(&input_ctx);
        avformat_free_context(output_ctx);
    }
}

void SLcompressor::read_frames() {
    frame = av_frame_alloc();
    av_frame_get_buffer(frame, 0);
    while (av_read_frame(input_ctx, &packet) >= 0) {
        if (packet.stream_index == video_stream_index) {
            if (avcodec_send_packet(video_decoder_context, &packet) < 0) {
                std::cout << "Error sending packet" << std::endl;
                break;
            }
            while (int ret = avcodec_receive_frame(video_decoder_context, frame) >= 0) {
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    std::cout << "Error returning frame" << std::endl;
                    break;
                }
                if (avcodec_send_frame(video_encoder_context, frame) < 0) {
                    std::cout << "Error sending frame" << std::endl;
                    break;
                }
                while (avcodec_receive_packet(video_encoder_context, &packet) >= 0) {
                    if (av_interleaved_write_frame(output_ctx, &packet) < 0) {
                        std::cout << "Error writing to frame to output" << std::endl;
                        break;
                    }
                }
            }
        }
        else if (packet.stream_index == audio_stream_index) {
            if (av_interleaved_write_frame(output_ctx, &packet) < 0) {
                std::cout << "Error writing audio packet to output file" << std::endl;
            }
        }
        av_packet_unref(&packet);
    }
    av_write_trailer(output_ctx);
}

void SLcompressor::close_resources() {
    avio_closep(&output_ctx->pb);
    avcodec_free_context(&video_encoder_context);
    avcodec_free_context(&video_decoder_context);
    avformat_close_input(&input_ctx);
}

SLcompressor::~SLcompressor() {
    close_resources();
}

