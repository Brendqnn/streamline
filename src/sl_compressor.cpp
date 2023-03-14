#include "sl_compressor.h"
#include <iostream>

SLcompressor::SLcompressor()
    : io_ctx(std::make_shared<SLiomanager>())
    , c_ctx(std::make_shared<SLcodec>())
    , s_ctx(std::make_shared<SLstream>())
    , q(std::make_shared<SLqueue>())
    , compress_state(false)
    , frame(av_frame_alloc())
    , packet()
    , str(str)
    , filename(filename)
{

}

SLcompressor::~SLcompressor() {
    
}

void SLcompressor::setup_ctx() {
    filename = q->get_file_info();
    io_ctx->open_media_input(filename);
    s_ctx->find_media_streams(io_ctx);
    s_ctx->setup_input_streams(io_ctx);
    c_ctx->open_decoder_ctx(s_ctx);
    c_ctx->open_encoder_ctx(s_ctx);
    io_ctx->alloc_output_ctx();
    c_ctx->stream_to_output(io_ctx, s_ctx);
    io_ctx->write_file_header();
}

void SLcompressor::start_compress() {
    av_frame_get_buffer(frame, 0);
    while (av_read_frame(io_ctx->input_ctx, &packet) >= 0) {
        if (packet.stream_index == s_ctx->video_stream_idx) {
            if (avcodec_send_packet(c_ctx->video_decoder_ctx, &packet) < 0) {
                std::cout << "Error sending packet to decoder" << std::endl;
                break;
            }
            while (int ret = avcodec_receive_frame(c_ctx->video_decoder_ctx, frame) >= 0) {
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    std::cout << "Error returning decoded output data" << std::endl;
                    break;
                }
                if (avcodec_send_frame(c_ctx->video_encoder_ctx, frame) < 0) {
                    std::cout << "Error sending frame to encoder" << std::endl;
                    break;
                }
                while (avcodec_receive_packet(c_ctx->video_encoder_ctx, &packet) >= 0) {
                    if (av_interleaved_write_frame(io_ctx->output_ctx, &packet) < 0) {
                        std::cout << "Error writing to frame to output" << std::endl;
                        break;
                    }
                }
            }
        }
        else if (packet.stream_index == s_ctx->audio_stream_idx) {
            if (av_interleaved_write_frame(io_ctx->output_ctx, &packet) < 0) {
                std::cout << "Error copying audio to output file" << std::endl;
            }
        }
        av_packet_unref(&packet);
    }
    av_write_trailer(io_ctx->output_ctx);
}