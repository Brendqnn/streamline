#pragma once
#include <memory>
#include "sl_codec.h"
#include "sl_iomanger.h"
#include "sl_stream.h"
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
	
private:
	std::shared_ptr<SLiomanager> io_ctx;
	std::shared_ptr<SLcodec> codec_ctx;
	std::shared_ptr<SLstream> stream_ctx;

};