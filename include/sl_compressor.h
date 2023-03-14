#pragma once
#include <memory>
#include "sl_codec.h"
#include "sl_iomanger.h"
#include "sl_stream.h"
#include <sl_queue.h>
extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavcodec/codec.h>
	#include <libavformat/avformat.h>
	#include <libavutil/avutil.h>
	#include <libswscale/swscale.h>
}

class SLcompressor {
public:
	SLcompressor();
	~SLcompressor();

	AVFrame* frame;

	AVPacket packet;

	void start_compress();
	void setup_ctx();

	bool compress_state;
private:
	std::string str;
	const char* filename;

	std::shared_ptr<SLiomanager> io_ctx;
	std::shared_ptr<SLcodec> c_ctx;
	std::shared_ptr<SLstream> s_ctx;
	std::shared_ptr<SLqueue> q;
};