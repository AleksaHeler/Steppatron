#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

int main(int argc, char *argv[])
{
    int status;
    int mode = SND_RAWMIDI_SYNC;
    snd_rawmidi_t *device = NULL;
    const char *portname = "hw:1,0,0";
    if ((status = snd_rawmidi_open(&device, NULL, portname, mode)) < 0)
    {
        fprintf(stderr, "Problem opening MIDI input: %s", snd_strerror(status));
        exit(1);
    }

    int count = 0;
    unsigned char buffer[1];
    while (count < 1000)
    {
        if (snd_rawmidi_read(device, buffer, 1) < 0)
        {
            fprintf(stderr, "Problem reading MIDI input!");
        }
        count++;
        printf("0x%x ", buffer[0]);
        fflush(stdout);
        if (count % 21 == 0)
        {
            printf("\n");
        }
    }

    snd_rawmidi_close(device);
    device = NULL;
    return 0;
}