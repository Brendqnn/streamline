#pragma once
#include "sl_stream.h"
#include "sl_iomanger.h"
#include <memory>
extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavcodec/codec.h>
	#include <libavformat/avformat.h>
	#include <libavutil/avutil.h>
}

class SLcodec {
public:
	SLcodec();
	~SLcodec();

	const AVCodec* video_decoder;
	AVCodecContext* video_decoder_ctx;

	const AVCodec* video_encoder;
	AVCodecContext* video_encoder_ctx;

	void open_decoder_ctx(std::shared_ptr<SLstream> _stream);
	void open_encoder_ctx(std::shared_ptr<SLstream> _stream);
	void copy_audio_parameters(std::shared_ptr<SLiomanager> _ctx, std::shared_ptr<SLstream> _stream);
	void alloc_output_ctx(std::shared_ptr<SLiomanager> _ctx, std::shared_ptr<SLstream> _stream);

private:
	std::shared_ptr<SLiomanager> _ctx;
	std::shared_ptr<SLstream> _stream;
	void set_encoder_properties(std::shared_ptr<SLstream> _stream);
	
};