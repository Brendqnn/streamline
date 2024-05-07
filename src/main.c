#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "slutil.h"
#include "slstream.h"
#include "slcodec.h"


int main(int argc, char *argv[])
{
    const char *filename; 
    char output_filename[100];
    
    int compress = 0;

    SLStream stream = {0};
    SLDecoder decoder = {0};
    SLEncoder encoder = {0};
    SLFiltering *filter_ctx = NULL;
    
    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        display_version();
        return 0;
    }

    if (argc > 1 && strcmp(argv[1], "-c") == 0) {
        compress = 1;
        argv++; // Skip the compression flag
        argc--; // Adjust the argument count
    }
    
    if (compress == 1 && argc == 2 && is_valid_file(argv[1])) {
        // Compress
        unsigned int stream_index;
        
        open_input_file(argv[1], &stream, &decoder);
        create_output_label(argv[1], output_filename);
        open_output_file(output_filename, &stream, &decoder, &encoder);
        init_filters(&filter_ctx, &decoder, &stream, &encoder);

        int ret;

        AVPacket *packet = NULL;

        while (1) {
            if ((ret = av_read_frame(&stream.input_format_ctx, packet)) < 0)
                break;

          
        }
        av_write_trailer(stream.output_format_ctx);
    } else {
        filename = argv[1];

        SLSrr srr = {0};
        SLAudioDevice audio_device = {0};
        
        open_input(filename, &stream);
        open_decoder(&decoder, &stream);
        setup_resampler(&srr, &decoder, 192000);
        srr.buffer = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLT, 2, 1);
        decode(&stream, &decoder, &encoder, &srr);

        set_audio_playback_device(&audio_device, &stream, &srr);

        play_audio_buffer(&stream, &audio_device, &srr);
        avformat_close_input(&stream.input_format_ctx);
    }

    return 0;
}
