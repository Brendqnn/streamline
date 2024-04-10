#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "slutil.h"
#include "slstream.h"
#include "slcodec.h"

enum SLFileType {
    SL_MEDIA_AUDIO,
    SL_MEDIA_VIDEO
};

struct {
    enum SLFileType type;
    const char *extension;
} file_type_map[] = {
    // video formats
    {SL_MEDIA_VIDEO, ".mp4"},
    {SL_MEDIA_VIDEO, ".avi"},
    {SL_MEDIA_VIDEO, ".mov"},
    {SL_MEDIA_VIDEO, ".wmv"},
    {SL_MEDIA_VIDEO, ".m4v"},
    // audio formats
    {SL_MEDIA_AUDIO, ".mp3"},
    {SL_MEDIA_AUDIO, ".aac"}
};

int is_valid_file(const char *filename)
{
    int valid = 0;
    const char *file_extension = strrchr(filename, '.');
    if (file_extension != NULL) {
        for (int i = 0; i < sizeof(file_type_map) / sizeof(file_type_map[0]); ++i) {
            if (strcmp(file_extension, file_type_map[i].extension) == 0) {
                valid = 1;
                break;
            }
        }
    }
    return valid;
}

const char *create_output_label(const char *filename)
{
    const char *tag = "[ Streamline ]";
    if (strrchr(filename, '/') == 0) {
        
    }
}

int main(int argc, char *argv[])
{   
    const char *filename;
    const char *output_filename = "res/slindigo.mp4";
    int compress = 0;

    SLStream stream = {0};
    SLDecoder decoder = {0};
    SLEncoder encoder = {0};
    SLSrr srr = {0};
    SLAudioDevice audio_device = {0};

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
        open_decoder(&decoder, &stream);
        open_encoder(&encoder, &decoder, &stream);
        alloc_output_ctx(output_filename, &stream, &encoder);
        copy_audio_parameters(&stream);
        write_file_header(output_filename, &stream);
        encode(&stream, &decoder, &encoder);
    } else {
        filename = argv[1];
        open_input(filename, &stream);
        open_decoder(&decoder, &stream);
        setup_resampler(&srr, &decoder);
        srr.buffer = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLT, 2, 1);
        decode(&stream, &decoder, &encoder, &srr);

        set_audio_playback_device(&audio_device, &stream, &srr);

        if (ma_device_start(&audio_device.device) != MA_SUCCESS) {
            printf("Failed to start playback device.\n");
            return -1;
        }

        getchar();

        av_audio_fifo_free(srr.buffer);
    }

    return 0;
}

