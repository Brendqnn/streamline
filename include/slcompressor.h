#pragma once
extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavcodec/codec.h>
	#include <libavformat/avformat.h>
	#include <libavutil/avutil.h>
	#include <libswscale/swscale.h>
	#include <libavutil/audio_fifo.h>
	#include <libavutil/samplefmt.h>
	#include <libavutil/opt.h>
	#include <libswresample/swresample.h>
	#include <libavutil/channel_layout.h>
}

class SLcompressor {
public:
	SLcompressor(const char* filename);
	~SLcompressor();

	AVFormatContext* input_ctx;
	AVFormatContext* output_ctx;

	const AVCodec* video_decoder;
	AVCodecContext* video_decoder_ctx;

	const AVCodec* video_encoder;
	AVCodecContext* video_encoder_ctx;

	int video_stream_idx;
	int audio_stream_idx;

	AVStream* input_video_stream;
	AVStream* input_audio_stream;
	AVStream* output_video_stream;
	AVStream* output_audio_stream;

	AVRational input_video_framerate;

	AVFrame* frame;

	AVPacket packet;

	void open_media_input();
	void open_decoder_ctx();
	void open_encoder_ctx();
	void write_file_header();
	void start_compress();
private:
	void copy_audio_parameters();
	void alloc_output_ctx();
	void set_encoder_properties();
	void find_media_streams();
	void setup_input_streams();

	const char* filename;
};
