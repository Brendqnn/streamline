#pragma once
#include <iostream>
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

	void open_media_input();
	void write_file_header();
	void alloc_output_ctx();
private:
};