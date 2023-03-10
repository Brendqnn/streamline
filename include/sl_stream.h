#pragma once
#include "sl_iomanger.h"
#include <memory>
extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavcodec/codec.h>
	#include <libavformat/avformat.h>
	#include <libavutil/avutil.h>
	#include <libswscale/swscale.h>
}

class SLstream {
public:
	SLstream();
	~SLstream();

	int video_stream_idx;
	int audio_stream_idx;

	AVStream* input_video_stream;
	AVStream* input_audio_stream;
	AVStream* output_video_stream;
	AVStream* output_audio_stream;
	AVRational input_video_framerate;

	void find_media_streams(std::shared_ptr<SLiomanager> io_ctx);
	void setup_input_streams(std::shared_ptr<SLiomanager> io_ctx);

	std::shared_ptr<SLiomanager> io_ctx;
private:
	
};