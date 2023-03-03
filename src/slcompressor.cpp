#include "slcompressor.h"
#include <iostream>

SLcompressor::SLcompressor() {

}


SLcompressor::~SLcompressor() {
    close_resources();
}

void SLcompressor::start_compress() {
    frame = av_frame_alloc();
    av_frame_get_buffer(frame, 0);
    while (av_read_frame(input_ctx_, &packet) >= 0) {
        if (packet.stream_index == video_stream_idx_) {
            if (avcodec_send_packet(video_decoder_ctx_, &packet) < 0) {
                std::cout << "Error sending packet to decoder" << std::endl;
                break;
            }
            while (int ret = avcodec_receive_frame(video_decoder_ctx_, frame) >= 0) {
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    std::cout << "Error returning decoded output data" << std::endl;
                    break;
                }
                if (avcodec_send_frame(video_encoder_ctx_, frame) < 0) {
                    std::cout << "Error sending frame to encoder" << std::endl;
                    break;
                }
                while (avcodec_receive_packet(video_encoder_ctx_, &packet) >= 0) {
                    if (av_interleaved_write_frame(output_ctx_, &packet) < 0) {
                        std::cout << "Error writing to frame to output" << std::endl;
                        break;
                    }
                }
            }
        }
        else if (packet.stream_index == audio_stream_idx_) {
            if (av_interleaved_write_frame(output_ctx_, &packet) < 0) {
                std::cout << "error writing audio to output file" << std::endl;
            }
        }
        av_packet_unref(&packet);
    }
    av_write_trailer(output_ctx_);

}

void SLcompressor::close_resources() {
    avio_closep(&output_ctx_->pb);
    avformat_close_input(&input_ctx_);
    av_frame_free(&frame);
    av_packet_unref(&packet);
}