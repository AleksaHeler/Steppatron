#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "midiParser.h"
#include "rawMidi.h"
#include "getch.h"

// Output file name
#define FILE_NAME "testFile.txt"

// Arguments:
// 1. - u for USB, k for keyboard, f for file
// 2. - filename
int main(int argc, char **argv) {
    int file_desc = open(FILE_NAME, O_RDWR);

    if (file_desc < 0) {
        printf("Error, %s not opened\n", FILE_NAME);
        return EXIT_FAILURE;
    }

    if (argc == 1 || strcmp(argv[1], "u") == 0) {
        // Read from USB
        snd_rawmidi_t *midiIn = NULL;
        if (rawmidiInit(midiIn)) {
            char buffer[2];
            if (getRawmidiCommand(buffer, midiIn)) {
                // Send to file
                int ret_val = write(file_desc, buffer, 2);

                if (ret_val == 0) {
                    printf("Error writing to file\n");
                    close(file_desc);
                    return EXIT_FAILURE;
                }
            }
            rawmidiClose(midiIn);
        }

    } else if (strcmp(argv[1], "f") == 0 && argc > 2) {
        // Read from file
        playMidiFile(argv[2]);

    } else if (strcmp(argv[1], "k") == 0) {
        // Read from keyboard
        int ret_val;
        char input[2];
        input[0] = '1'; //for now only 1. stepper
        int octave;

        printf("Choose octave [1-7]: ");
        scanf("%d", &octave);
        getch(); //da pokupi enter

        printf("Start playing notes\n");

        while (1) {
            input[1] = getch();

            switch (input[1]) {
            //donji red
            case 'a':
                input[1] = (char)C[octave].MIDINumber;
                break;
            case 's':
                input[1] = (char)D[octave].MIDINumber;
                break;
            case 'd':
                input[1] = (char)E[octave].MIDINumber;
                break;
            case 'f':
                input[1] = (char)F[octave].MIDINumber;
                break;
            case 'g':
                input[1] = (char)G[octave].MIDINumber;
                break;
            case 'h':
                input[1] = (char)A[octave].MIDINumber;
                break;
            case 'j':
                input[1] = (char)B[octave].MIDINumber;
                break;

            //gornji red
            case 'w':
                input[1] = (char)M[5 * (octave - 1)].MIDINumber;
                break;
            case 'e':
                input[1] = (char)M[5 * (octave - 1) + 1].MIDINumber;
                break;
            case 't':
                input[1] = (char)M[5 * (octave - 1) + 2].MIDINumber;
                break;
            case 'y':
                input[1] = (char)M[5 * (octave - 1) + 3].MIDINumber;
                break;
            case 'u':
                input[1] = (char)M[5 * (octave - 1) + 4].MIDINumber;
                break;

            default:
                continue;
            }

            if (input[1] == 'q') {
                close(file_desc);
                break;
            }

            ret_val = write(file_desc, input, 2);

            if (ret_val == 0) {
                printf("Error writing to file\n");
                close(file_desc);
                return 2;
            }
        }

    } else {
        printf("Invalid arguments!\n");
        printf("Use: steppatron [MODE] [FILENAME]\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}