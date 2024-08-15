#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define STREAMLINE_INTERNAL
#include "streamline.h"

int main(int argc, char* argv[])
{
    char *file;
    char *volume;
    char *fade;
    bool loop;
    SLAudioDevice device;

    loop = false;
    
    if (argv[1] && argc == 2) {
        if (!access(argv[1], F_OK) == 0) {
            printf("File path %s does not exist.\n", argv[1]);
            return -1;
        }
        
        if (!strstr(argv[1], ".mp3")) {
            fprintf(stderr, "Error: File type not supported for playback.\n");
            return -1;
        }     

        sl_display_version();
       
        sl_initialize_audio_device(&device, argv[1], 1.0f, loop);
        sl_play_audio(&device);
        sl_destroy_device(&device);         
    } else if (argc > 2) {
        char i;
        float volume_temp;
        short index = 0;
        float fade_temp;
        bool loop;

        volume_temp = 1.0f;
        loop = false;
        
        for (i = 0; i < argc; i++) {            
            if (strstr(argv[i], "-db")) {
                volume = argv[i + 1];
                volume_temp = atof(volume);
            }

            if (strstr(argv[i], "-fd")) {
                fade = argv[i + 1];
                fade_temp = atof(fade);                
            }

            if (strstr(argv[i], "-L")) {
                loop = true;
                printf("loop audio track enabled.\n");
            }

            if (strstr(argv[i], ".mp3")) {
                index = i;
                if (access(argv[index], F_OK) == 0) {
                    file = argv[index];
                } else {
                    printf("File path %s does not exist.\n", file);
                    return -1;
                }
            }
        }

        sl_display_version();
        
        sl_initialize_audio_device(&device, file, volume_temp, loop);
        
        sl_fade_audio_in(&device, fade_temp);
        sl_play_audio(&device); 
    }      

    return 0;
}
