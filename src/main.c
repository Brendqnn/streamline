#include <stdio.h>

#include "../include/slcodec.h"

int main(int argc, char **argv)
{   
    SLAudioDevice audio_device = {0};
    SLStreamContext stream_ctx = {0};

    sl_display_version();

    printf("\n");
        
    sl_open_input_file(argv[1], &stream_ctx);
        
    sl_setup_resampler(&stream_ctx, SL_SAMPLE_RATE_192, 1.0f);
    sl_decode_resample(&stream_ctx);

    sl_set_audio_playback_device(&audio_device, &stream_ctx);

    sl_play_audio_buffer(&audio_device, &stream_ctx);

    sl_free_audio(&stream_ctx);
    
    return 0;
}
