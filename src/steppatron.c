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

struct MIDIStruct {
    const int MIDINumber;
    const float freq;
    const float period;
};

static const struct MIDIStruct A[8] =
    {
        {21, 27.500, 36.36},  //   A0
        {33, 55.000, 18.18},  //   A1
        {45, 110.00, 9.091},  //   A2
        {57, 220.00, 4.545},  //   A3
        {69, 440.00, 2.273},  //   A4
        {81, 880.00, 1.136},  //   A5
        {93, 1760.0, 0.5682}, //   A6
        {105, 3520.0, 0.2841} //   A7
};

static const struct MIDIStruct B[8] =
    {
        {23, 30.868, 32.40},   //   B0
        {35, 61.735, 16.20},   //   B1
        {47, 123.47, 8.099},   //   B2
        {59, 246.94, 4.050},   //   B3
        {71, 493.88, 2.025},   //   B4
        {83, 987.77, 1.012},   //   B5
        {95, 1975.5, 0.5062},  //   B6
        {107, 3951.1, 0.2531}, //   B7
};

static const struct MIDIStruct C[9] =
    {
        {24, 32.703, 30.58},  //   C1 //da bi bilo lakse da se kuca c0, c1, a ne c1 - 1
        {24, 32.703, 30.58},  //   C1
        {36, 65.406, 15.29},  //   C2
        {48, 130.81, 7.645},  //   C3
        {60, 261.63, 3.822},  //   C4
        {72, 523.25, 1.910},  //   C5
        {84, 1046.5, 0.9556}, //   C6
        {96, 2093.0, 0.4778}, //   C7
        {108, 4186.0, 0.2389} //   C8
};

static const struct MIDIStruct D[8] =
    {
        {26, 36.708, 27.24},  //   D1
        {26, 36.708, 27.24},  //   D1
        {38, 73.416, 13.62},  //   D2
        {50, 146.83, 6.811},  //   D3
        {62, 293.67, 3.405},  //   D4
        {74, 587.33, 1.703},  //   D5
        {86, 1174.7, 0.8513}, //   D6
        {98, 2349.3, 0.4257}, //   D7
};

static const struct MIDIStruct E[8] =
    {
        {28, 41.203, 24.27},   //   E1
        {28, 41.203, 24.27},   //   E1
        {40, 82.407, 12.13},   //   E2
        {52, 164.81, 6.068},   //   E3
        {64, 329.23, 3.034},   //   E4
        {76, 659.26, 1.517},   //   E5
        {88, 1318.5, 0.7584},  //   E6
        {100, 2637.0, 0.3792}, //   E7
};

static const struct MIDIStruct F[8] =
    {
        {29, 43.654, 22.91},   //   F1
        {29, 43.654, 22.91},   //   F1
        {41, 87.307, 11.45},   //   F2
        {53, 174.61, 5.727},   //   F3
        {65, 349.23, 2.863},   //   F4
        {77, 698.46, 1.432},   //   F5
        {89, 1396.9, 0.7159},  //   F6
        {101, 2793.0, 0.3580}, //   F7
};

static const struct MIDIStruct G[8] =
    {
        {31, 48.999, 20.41},   //   G1
        {31, 48.999, 20.41},   //   G1
        {43, 97.999, 10.20},   //   G2
        {55, 196.00, 5.102},   //   G3
        {67, 392.00, 2.551},   //   G4
        {79, 783.99, 1.276},   //   G5
        {91, 1568.0, 0.6378},  //   G6
        {103, 3136.0, 0.3189}, //   G7
};

static const struct MIDIStruct M[] =
    {
        //{22,       29.135, 34.32},  // A0/B0
        {25, 34.684, 28.86},   // C1/D1
        {27, 38.891, 25.71},   // D1/E1
        {30, 46.249, 21.26},   // F1/G1
        {32, 51.913, 19.26},   // G1/A1
        {34, 58.270, 17.16},   // A1/B1
        {37, 69.296, 14.29},   // C2/D2
        {39, 77.782, 12.86},   // D2/E2
        {42, 92.499, 10.81},   // F2/G2
        {44, 103.83, 9.631},   // G2/A2
        {46, 116.54, 8.581},   // A2/B2
        {49, 138.59, 7.216},   // C3/D3
        {51, 155.56, 6.428},   // D3/E3
        {54, 185.00, 5.405},   // F3/G3
        {56, 207.65, 4.816},   // G3/A3
        {58, 233.08, 4.290},   // A3/B3
        {61, 277.18, 3.608},   // C4/D4
        {63, 311.13, 3.214},   // D4/E4
        {66, 369.99, 2.703},   // F4/G4
        {68, 415.30, 2.408},   // G4/A4
        {70, 466.16, 2.145},   // A4/B4
        {73, 554.37, 1.804},   // C5/D5
        {75, 622.25, 1.607},   // D5/E5
        {78, 739.99, 1.351},   // F5/G5
        {80, 830.61, 1.204},   // G5/A5
        {82, 923.33, 1.073},   // A5/B5
        {85, 1108.7, 0.9020},  // C6/D6
        {87, 1244.5, 0.8034},  // D6/E6
        {90, 1480.0, 0.6757},  // F6/G6
        {92, 1661.2, 0.6020},  // G6/A6
        {94, 1864.7, 0.5363},  // A6/B6
        {97, 2217.5, 0.4510},  // C7/D7
        {99, 2489.0, 0.4018},  // D7/E7
        {102, 2960.0, 0.3378}, // F7/G7
        {104, 3222.4, 0.3010}, // G7/A7
        {106, 3792.3, 0.2681}  // A7/B7
};

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
                    if (!playNext(&midi, file_desc)) {
                        break;
                    }
                }
            }
            freeMidi(&midi);
            printf("\nDone!\n");
        }
    } else if (strcmp(argv[1], "k") == 0) {
        // Read from keyboard
        int ret_val;
        char input[2];
        int octave;

        int auto_stepper = 0;
        int stepper = 0;
        while(1){
            printf("Choose stepper manually [0-3] or automatically [4]: ");
            scanf("%d", &stepper);

            if(stepper<0 || stepper>4)
                printf("Answer must be in range [1,5]\n");
            else
                break;
        }

        if(stepper < 4)
            input[0] = stepper;
        else{
            auto_stepper = 1;
            stepper = -1;
        }

        printf("Choose octave [1-7]: ");
        while(1){
            scanf("%d", &octave);
            if(octave<1 || octave>7)
                printf("Answer must be in range [1,7]\n");
            else
                break;
        }
        getch(); //da pokupi enter

        printf("Start playing notes\n");

        while (1) {
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

            if(auto_stepper == 1){
                if(++stepper == 4)
                    stepper = 0;

                if(input[1] == 0xFF){

                    for(int i = 0; i<4; i++){
                        input[0] = i;
                        ret_val = write(file_desc, input, 2);

                        if (ret_val == 0) {
                            printf("Error writing to file\n");
                            close(file_desc);
                            return 2;
                        }
                    }
                    
                    continue;
                }
                else
                    input[0] = stepper;
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
