#include "sl_iomanger.h"
#include <iostream>


SLiomanager::SLiomanager()
	: input_ctx(nullptr)
	, output_ctx(nullptr)
{
	
}

SLiomanager::~SLiomanager() {
    if (input_ctx != nullptr) {
        avformat_close_input(&input_ctx);
    }
    if (output_ctx != nullptr) {
        avformat_free_context(output_ctx);
    }
}

void SLiomanager::open_media_input() {
	if (avformat_open_input(&input_ctx, "res/indigo.mp4", nullptr, nullptr) < 0) {
		std::cout << "could not find input media\n";
	}
}

void SLiomanager::write_file_header() {
    if (!(output_ctx->flags & AVFMT_NOFILE)) {
        if (avio_open(&output_ctx->pb, "res/compressed-indigo.mp4", AVIO_FLAG_WRITE) < 0) {
            std::cout << "Error: could not open output file" << std::endl;
            
        }
    }
    if (avformat_write_header(output_ctx, nullptr) < 0) {
        std::cout << "Error: could not write header to output file" << std::endl;
        
    }
}



