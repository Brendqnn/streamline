#include "sl_iomanger.h"
#include <iostream>


SLiomanager::SLiomanager()
    : filename(filename)
    , output_prefix(output_prefix)
    , input_ctx(nullptr)
    , output_ctx(nullptr)
{
    
}

SLiomanager::~SLiomanager() {
    avformat_close_input(&input_ctx);
    avformat_free_context(output_ctx);
}

void SLiomanager::open_media_input(const char* filename) {
    if (avformat_open_input(&input_ctx, filename, nullptr, nullptr) < 0) {
        std::cout << "could not find input media" << std::endl;
        avformat_close_input(&input_ctx);
    }
}

void SLiomanager::write_file_header() {
    if (!(output_ctx->flags & AVFMT_NOFILE)) {
        if (avio_open(&output_ctx->pb, "res/compressed-indigo.mp4", AVIO_FLAG_WRITE) < 0) {
            std::cout << "Error: could not open output file" << std::endl;
            avformat_free_context(output_ctx);
        }
    }
    if (avformat_write_header(output_ctx, nullptr) < 0) {
        std::cout << "Error: could not write header to output file" << std::endl;
        avformat_free_context(output_ctx);
    }
}

void SLiomanager::alloc_output_ctx() {
    if (avformat_alloc_output_context2(&output_ctx, nullptr, nullptr, "res/compressed-indigo.mp4") >= 0) {
        std::cout << "Error allocating output context" << std::endl;
        avformat_free_context(output_ctx);
    }
 }



