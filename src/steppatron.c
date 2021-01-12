#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "midiParser.h"
#include "rawMidi.h"
#include "getch.h"

// Output file name (driver node)
#define FILE_NAME "/dev/gpio_driver"

// SIGINT received flag
static volatile int end = 0;

void interruptHandler(int a) {
    end = 1;
}

// Arguments:
// 1. - u for USB, k for keyboard, f for file
// 2. - filename
int main(int argc, char **argv) {
    int file_desc = open(FILE_NAME, O_RDWR);

    if (file_desc < 0) {
        printf("Error, %s not opened\n", FILE_NAME);
        return EXIT_FAILURE;
    }

    signal(SIGINT, interruptHandler);

    if (argc == 1 || strcmp(argv[1], "u") == 0) {
        // Read from USB
        snd_rawmidi_t *midiIn = NULL;
        unsigned int steppers;
        if (argc > 2) steppers = atoi(argv[2]);
        else steppers = 1;

        if (rawmidiInit(&midiIn, steppers)) {
            unsigned char buffer[2];
            while (!end) {
                if (getRawmidiCommand(buffer, midiIn)) {
                    // Send to file
                    int ret_val = write(file_desc, buffer, 2);

                    if (ret_val == 0) {
                        printf("Error writing to file\n");
                        close(file_desc);
                        return EXIT_FAILURE;
                    }
                }
            }
            rawmidiClose(midiIn);
            printf("\nDone!\n");
        }
    } else if (strcmp(argv[1], "f") == 0 && argc > 2) {
        // Read from file
        midi_t midi;
        if (readMidiFile(&midi, argv[2])) {
            if (initPlayer(&midi)) {
                while (!end) {
                    playNext(&midi, file_desc);
                }
            }
            freeMidi(&midi);
            printf("\nDone!\n");
        }
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

        input[0] = 0;
        while (!end) {
            input[1] = getch();
            printf("Pressed: %c\n", input[1]);

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
            case 'q':  /* Exit */
                input[1] = 0xFF;
                write(file_desc, input, 2);
                close(file_desc);
                return EXIT_SUCCESS;
            default: /* Ako se pritisne neodredjen taster, prekinuce ton */
                input[1] = 0xFF;
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