#include <stdio.h>
#include <stdlib.h>

#define HEADER_CHUNK_ID 0x4D546864
#define TRACK_CHUNK_ID 0x4D54726B

typedef struct {
    unsigned short format;
    unsigned short trackN;
    unsigned short timediv;
} midiHeader_t;

typedef struct {
    unsigned int delta; // DeltaTime before this event
    unsigned char status;
    unsigned char param1; // Event type for meta events, param1 for midi
    unsigned char param2;
    unsigned int dataSize;
    unsigned char *data; // Data for meta and SysEx events if applicable
} midiEvent_t;

typedef struct nodeMidiEvent {
    struct nodeMidiEvent *next;
    midiEvent_t event;
} nodeMidiEvent_t;

typedef struct {
    nodeMidiEvent_t *first;
    nodeMidiEvent_t *last;
} listMidiEvent_t;

typedef struct {
    unsigned int size;
    listMidiEvent_t eventList;
} midiTrack_t;

typedef struct {
    midiHeader_t header;
    midiTrack_t *tracks;
} midiData_t;

FILE *midiFile;
midiData_t midiData;

listMidiEvent_t newList() { return (listMidiEvent_t){NULL, NULL}; }

void listAdd(const midiEvent_t *event, listMidiEvent_t *list) {
    nodeMidiEvent_t *newItem = malloc(sizeof(nodeMidiEvent_t));
    newItem->event = *event;
    newItem->next = NULL;

    if (list->first == NULL) {
        list->first = newItem;
    } else {
        list->last->next = newItem;
    }
    list->last = newItem;
}

void listFree(listMidiEvent_t *list) {
    nodeMidiEvent_t *curr = list->first;
    nodeMidiEvent_t *old;

    while (curr->next != NULL) {
        old = curr;
        curr = curr->next;
        free(old->event.data);
        free(old);
    }

    free(curr->event.data);
    free(curr);
    list->first = NULL;
    list->last = NULL;
}

void freeMidiData(midiData_t *data) {
    // In case we didn't read the track number yet
    if (data->header.format != HEADER_CHUNK_ID) return;
    for (size_t i = 0; i < data->header.trackN; i++) {
        listFree(&data->tracks[i].eventList);
    }
    free(data->tracks);
}

// Safely terminates the program in case of an error
void exitError(const char *error) {
    fprintf(stderr, "%s\n", error);
    fclose(midiFile);
    freeMidiData(&midiData);
    exit(EXIT_FAILURE);
}

// Reads size bytes from midiFile and outputs the result as a little-endian
// integer
unsigned int readInt(char size) {
    if (size > 4)
        return 0;
    unsigned int retVal = 0;
    unsigned char buffer;
    size--;
    while (size >= 0) {
        fread(&buffer, 1, 1, midiFile);
        retVal += buffer << (8 * size);
        size--;
    }
    return retVal;
}

// Reads a variable-lenght value from midiFile and outputs the result as a
// little-endian integer
unsigned int readVarInt() {
    unsigned int retVal = 0;
    char buffer[4];
    int i = 0;
    while (i < 4) {
        fread(buffer + i, 1, 1, midiFile);
        // If most significant bit is 0
        if (buffer[i] >= 0) {
            // This is the last byte
            for (int j = 0; j <= i; j++) {
                retVal += buffer[j] << (7 * (i - j));
            }
            return retVal;
        } else {
            // There are more bytes
            buffer[i] ^= 0x80;
            i++;
        }
    }
    return 0;
}

void readHeader(midiHeader_t *header) {
    if (readInt(4) != HEADER_CHUNK_ID)
        exitError("Wrong file type or file corrupted!");
    readInt(4); // Read the header size bytes
    header->format = readInt(2);
    header->trackN = readInt(2);
    header->timediv = readInt(2);
    // Check if timing is metrical or timecode
    if (header->timediv & 0x8000)
        exitError("Timecode timing not yet supported");
}

void readEvent(listMidiEvent_t *list) {
    midiEvent_t event;
    event.delta = readVarInt();
    event.status = readInt(1);
    if (event.status == 0xFF) {
        // Meta event
        event.param1 = readInt(1);
        event.param2 = 0;
        event.dataSize = readVarInt();
        if (event.dataSize != 0) {
            event.data = malloc(event.dataSize);
            fread(event.data, 1, event.dataSize, midiFile);
        } else {
            event.data = NULL;
        }
    } else if (event.status == 0xF0 || event.status == 0xF7) {
        // SysEx event
        event.param1 = 0;
        event.param2 = 0;
        event.dataSize = readVarInt();
        if (event.dataSize != 0) {
            event.data = malloc(event.dataSize);
            fread(event.data, 1, event.dataSize, midiFile);
        } else {
            event.data = NULL;
        }
    } else if (event.status >= 0x80 && event.status <= 0xEF) {
        // MIDI event
        event.dataSize = 0;
        event.data = NULL;
        event.param1 = readInt(1);
        unsigned char type = event.status & 0xF0;
        if (type != 0xC0 && type != 0xD0) {
            event.param2 = readInt(1);
        }
    } else {
        exitError("Error while reading midi event - invalid status byte");
    }
    listAdd(&event, list);
}

void readTrack(midiTrack_t *track) {
    if (readInt(4) != TRACK_CHUNK_ID)
        exitError("Error while reading track header - invalid ID");
    track->size = readInt(4);
    track->eventList = newList();
    long startPos = ftell(midiFile);

    while (ftell(midiFile) - startPos < track->size) {
        readEvent(&track->eventList);
    }
}

void readMidiData(midiData_t *midiData) {
    readHeader(&midiData->header);
    midiData->tracks = malloc(sizeof(midiTrack_t) * midiData->header.trackN);
    for (size_t i = 0; i < midiData->header.trackN; i++) {
        readTrack(&midiData->tracks[i]);
    }
}

int main(int argc, char **argv) {
    if (argc == 1) {
        // Read from USB

    } else {
        // Read from file
        midiFile = fopen(argv[1], "rb");
        if (midiFile == NULL) {
            fprintf(stderr, "Error while opening file %s\n", argv[1]);
            return EXIT_FAILURE;
        }

        readMidiData(&midiData);

        fclose(midiFile);
        freeMidiData(&midiData);
    }

    return EXIT_SUCCESS;
}