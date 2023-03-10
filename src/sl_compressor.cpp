#include "sl_compressor.h"
#include <iostream>

SLcompressor::SLcompressor()
    : io_ctx(std::make_shared<SLiomanager>())
    , codec_ctx(std::make_shared<SLcodec>())
    , stream_ctx(std::make_shared<SLstream>())
    , frame(av_frame_alloc())
    , packet()
{
    
}

SLcompressor::~SLcompressor() {
    if (frame != nullptr) {
        av_frame_free(&frame);
    }
}

void SLcompressor::setup_ctx() {
    io_ctx->open_media_input();
    stream_ctx->find_media_streams(io_ctx);
    stream_ctx->setup_input_streams(io_ctx);
    codec_ctx->open_decoder_ctx(stream_ctx);
    codec_ctx->open_encoder_ctx(stream_ctx);
    codec_ctx->alloc_output_ctx(io_ctx, stream_ctx);
    io_ctx->write_file_header();

}


void SLcompressor::start_compress() {
    av_frame_get_buffer(frame, 0);
    while (av_read_frame(io_ctx->input_ctx, &packet) >= 0) {
        if (packet.stream_index == stream_ctx->video_stream_idx) {
            if (avcodec_send_packet(codec_ctx->video_decoder_ctx, &packet) < 0) {
                std::cout << "Error sending packet to decoder" << std::endl;
                break;
            }
            while (int ret = avcodec_receive_frame(codec_ctx->video_decoder_ctx, frame) >= 0) {
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    std::cout << "Error returning decoded output data" << std::endl;
                    break;
                }
                if (avcodec_send_frame(codec_ctx->video_encoder_ctx, frame) < 0) {
                    std::cout << "Error sending frame to encoder" << std::endl;
                    break;
                }
                while (avcodec_receive_packet(codec_ctx->video_encoder_ctx, &packet) >= 0) {
                    if (av_interleaved_write_frame(io_ctx->output_ctx, &packet) < 0) {
                        std::cout << "Error writing to frame to output" << std::endl;
                        break;
                    }
                }
            }
        }
        else if (packet.stream_index == stream_ctx->audio_stream_idx) {
            if (av_interleaved_write_frame(io_ctx->output_ctx, &packet) < 0) {
                std::cout << "Error copying audio to output file" << std::endl;
            }
        }
        av_packet_unref(&packet);
    }
    av_write_trailer(io_ctx->output_ctx);
}



