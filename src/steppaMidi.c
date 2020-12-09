#include <stdio.h>
#include <stdlib.h>

#define SIZE_WORD 2
#define SIZE_DWORD 4

FILE * midiFile;

typedef struct midiHeader
{
    short format;
    short trackN;
    short timediv;
};

// Reads size bytes from midiFile and outputs the result as little-endian integer
int readInt(char size) {
    if (size > 4) return 0;
    int retVal = 0;
    char buffer;
    size--;
    while (size >= 0) {
        fread(buffer, 1, 1, midiFile);
        retVal += buffer << (8 * size);
    }
    return retVal;
}

// Reads the midi header chunk
void readHeader() {
    unsigned char buffer[SIZE_DWORD];
}


int main(int argc, char** argv) {
    if (argc == 1) {
        // Read from USB

    } else if (argc == 2) {
        // Read from file
        midiFile = fopen(argv[1], "rb");
        if (midiFile == NULL) {
            fprintf(stderr, "Error while opening file %s\n", argv[1]);
            return EXIT_FAILURE;
        }
        readHeader();
        fclose(midiFile);
    }

    return EXIT_SUCCESS;
}