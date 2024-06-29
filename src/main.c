#include <stdio.h>
#include <stdlib.h>

#define SLCODEC_INTERNAL
#include "slcodec.h"

int main(int argc, char** argv)
{
    SLAudioDevice device;
    
    if (argc < 2) {
        printf("Usage: %s <path_to_mp3_file>\n", argv[0]);
        return -1;
    }

    sl_display_version();

    printf("\n");
    
    sl_setup_audio_device(argv[1], &device, 0.8f);
    sl_play(&device);
    sl_free_device(&device);    

    return 0;
}
