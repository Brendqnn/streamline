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
        filename = argv[1];

        open_input(filename, &stream);
        create_output_label(filename, output_filename);
        open_decoder(&decoder, &stream);
        open_encoder(&encoder, &decoder, &stream);
        alloc_output_ctx(output_filename, &stream, &encoder);
        copy_audio_parameters(&stream);
        write_file_header(output_filename, &stream);
        
        encode(&stream, &decoder, &encoder);
    } else {
        filename = argv[1];

        SLSrr srr = {0};
        SLAudioDevice audio_device = {0};
        
        open_input(filename, &stream);
        open_decoder(&decoder, &stream);
        setup_resampler(&srr, &decoder);
        srr.buffer = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLT, 2, 1);
        decode(&stream, &decoder, &encoder, &srr);

        set_audio_playback_device(&audio_device, &stream, &srr);

        play_audio_buffer(&stream, &audio_device, &srr);
        avformat_close_input(&stream.input_format_ctx);
    }

    return 0;
}

