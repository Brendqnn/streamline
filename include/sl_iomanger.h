#pragma once
#include <iostream>
#include "sl_queue.h"
#include <memory>
extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavcodec/codec.h>
	#include <libavformat/avformat.h>
	#include <libavutil/avutil.h>
	#include <libswscale/swscale.h>
}

class SLiomanager {
public:
	SLiomanager();
	~SLiomanager();

	AVFormatContext* input_ctx;
	AVFormatContext* output_ctx;

	void open_media_input(const char* filename);
	void write_file_header();
	void alloc_output_ctx();
private:
	const char* filename;
	const char* output_prefix;
};