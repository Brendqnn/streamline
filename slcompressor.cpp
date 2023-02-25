#include "slcompressor.h"
#include <iostream>

SLcompressor::SLcompressor()
    : input_context(nullptr)
    , output_context(nullptr)
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

SLcompressor::~SLcompressor() {
    close_resources();
}

void SLcompressor::open_input_media(const char* filename) {
    if (avformat_open_input(&input_context, filename, nullptr, nullptr) < 0) {
        throw std::runtime_error("Failed to read input media file");
    }
}

void SLcompressor::find_media_stream() {
    if (avformat_find_stream_info(input_context, nullptr) < 0) {
        throw std::runtime_error("Error reading packet data to find the media stream");
        close_resources();
    }
    video_stream_index = -1;
    audio_stream_index = -1;
    for (unsigned int i = 0; i < input_context->nb_streams; i++) {
        if (input_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
        }
        else if (input_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
        }
    }
    if (video_stream_index == -1) {
        throw std::runtime_error("Error finding input video stream index");
        close_resources();
    }
    if (audio_stream_index == -1) {
        throw std::runtime_error("Error finding input audio stream index");
        close_resources();
    }
}

void SLcompressor::set_input_stream() {
    input_video_stream = input_context->streams[video_stream_index];
    input_audio_stream = input_context->streams[audio_stream_index];
}

void SLcompressor::open_decoder_context() {
    video_decoder = avcodec_find_decoder(input_context->streams[video_stream_index]->codecpar->codec_id);
    video_decoder_context = avcodec_alloc_context3(video_decoder);
    if (avcodec_parameters_to_context(video_decoder_context, input_context->streams[video_stream_index]->codecpar) < 0) {
        throw std::runtime_error("Error copying decoder parameters to context");
        close_resources();
    }
    if (avcodec_open2(video_decoder_context, video_decoder, nullptr) < 0) {
        throw std::runtime_error("Error opening decoder context");
        close_resources();
    }
}

void SLcompressor::set_encoder_properties() {
    video_encoder_context->bit_rate = video_decoder_context->bit_rate;
    video_encoder_context->width = video_decoder_context->width;
    video_encoder_context->height = video_decoder_context->height;
    video_encoder_context->pix_fmt = video_encoder->pix_fmts[0];
    video_encoder_context->time_base = input_video_stream->time_base;
    video_encoder_context->framerate = input_video_framerate;
}

void SLcompressor::open_encoder_context() {
    video_decoder = avcodec_find_decoder(input_context->streams[video_stream_index]->codecpar->codec_id);
    video_decoder_context = avcodec_alloc_context3(video_decoder);
    set_encoder_properties();
    if (avcodec_parameters_to_context(video_decoder_context, input_context->streams[video_stream_index]->codecpar) < 0) {
        throw std::runtime_error("Error copying encoder parameters to context");
    }
    if (avcodec_open2(video_decoder_context, video_decoder, nullptr) < 0) {
        throw std::runtime_error("Error opening video decoder");
        close_resources();
    }
}

void SLcompressor::alloc_output_context() {
    avformat_alloc_output_context2(&output_context, nullptr, nullptr, "");
    output_video_stream = avformat_new_stream(output_context, video_encoder);
    output_video_stream->time_base = input_video_stream->time_base;
    if (!output_video_stream) {
        throw std::runtime_error("Error allocating output video stream");
        close_resources();
    }
    if (avcodec_parameters_from_context(output_video_stream->codecpar, video_encoder_context) < 0) {
        throw std::runtime_error("Error copying output stream to context");
        close_resources();
    }
}

void SLcompressor::copy_audio_parameters() {
    output_audio_stream = avformat_new_stream(output_context, NULL);
    if (!output_audio_stream) {
        close_resources();
        throw std::runtime_error("Could not allocate output video stream");
    }
    avcodec_parameters_copy(output_audio_stream->codecpar, input_audio_stream->codecpar);
}

void SLcompressor::write_file_header() {
    if (!(output_context->flags & AVFMT_NOFILE)) {
        if (avio_open(&output_context->pb, "", AVIO_FLAG_WRITE) < 0) {
            throw std::runtime_error("Error opening output file");
            close_resources();
        }
    }
    if (avformat_write_header(output_context, nullptr) < 0) {
        std::cout << "Error: could not write header to output file" << std::endl;
        close_resources();
    }
}

void SLcompressor::read_frames() {
    frame = av_frame_alloc();
    av_frame_get_buffer(frame, 0);
    while (av_read_frame(input_context, &packet) >= 0) {
        if (packet.stream_index == video_stream_index) {
            if (avcodec_send_packet(video_decoder_context, &packet) < 0) {
                throw std::runtime_error("Error sending packet to decoder");
                break;
            }
            while (int ret = avcodec_receive_frame(video_decoder_context, frame) >= 0) {
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    throw std::runtime_error("Error returning decoded output data");
                    break;
                }
                if (avcodec_send_frame(video_encoder_context, frame) < 0) {
                    throw std::runtime_error("Error sending frame to encoder");
                    break;
                }
                while (avcodec_receive_packet(video_encoder_context, &packet) >= 0) {
                    if (av_interleaved_write_frame(output_context, &packet) < 0) {
                        throw std::runtime_error("Error writing to frame to output");
                        break;
                    }
                }
            }
        }
        else if (packet.stream_index == audio_stream_index) {
            if (av_interleaved_write_frame(output_context, &packet) < 0) {
                throw std::runtime_error("Error copying audio to output file");
            }
        }
        av_packet_unref(&packet);
    }
    av_write_trailer(output_context);
}

void SLcompressor::close_resources() {
    avio_closep(&output_context->pb);
    avcodec_free_context(&video_encoder_context);
    avcodec_free_context(&video_decoder_context);
    avformat_close_input(&input_context);
}