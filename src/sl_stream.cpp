#include "sl_stream.h"
#include <iostream>

SLstream::SLstream()
    : io_ctx(std::make_shared<SLiomanager>())
    , video_stream_idx(-1)
    , audio_stream_idx(-1)
    , input_video_stream(nullptr)
    , input_audio_stream(nullptr)
    , output_video_stream(nullptr)
    , output_audio_stream(nullptr)
    , input_video_framerate({ 0, 0 })
    
{
    
}

SLstream::~SLstream() {

}

void SLstream::find_media_streams(std::shared_ptr<SLiomanager> io_ctx) {
    for (unsigned int i = 0; i < io_ctx->input_ctx->nb_streams; i++) {
        if (io_ctx->input_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
        }
        else if (io_ctx->input_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
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

void SLstream::setup_input_streams(std::shared_ptr<SLiomanager> io_ctx) {
    input_video_stream = io_ctx->input_ctx->streams[video_stream_idx];
    input_audio_stream = io_ctx->input_ctx->streams[audio_stream_idx];
    input_video_framerate = input_video_stream->r_frame_rate;
}




