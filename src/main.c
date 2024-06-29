#include <stdio.h>
#include <stdlib.h>

#define STREAMLINE_INTERNAL
#include "streamline.h"

int main(int argc, char** argv)
{    
    if (argc < 2) {
        printf("Usage: %s <path_to_mp3_file>\n", argv[0]);
        return -1;
    }

    sl_display_version();

    printf("\n");
    
    SLAudioDevice *device = sl_setup_audio_device(argv[1], 0.8f);
    sl_play(device);
    sl_free_device(device);    

    return 0;
}
