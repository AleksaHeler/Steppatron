#include <stdio.h>
#include <stdlib.h>
#include "midi.h"

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

typedef struct {
    unsigned char type;
    unsigned char channel;
    unsigned char param1;
    unsigned char param2;
} midiMessage_t;

listMidiEvent_t newList() { return (listMidiEvent_t){NULL, NULL}; }

void listAdd(const midiEvent_t *event, listMidiEvent_t *list) {
    nodeMidiEvent_t *newItem = (nodeMidiEvent_t *)malloc(sizeof(nodeMidiEvent_t));
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

// Reads size bytes from midiFile and outputs the result as a little-endian
// integer
unsigned int readInt(char size, FILE *midiFile) {
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
unsigned int readVarInt(FILE *midiFile) {
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

int readHeader(midiHeader_t *header, FILE *midiFile) {
    if (readInt(4, midiFile) != HEADER_CHUNK_ID) {
        fprintf(stderr, "Wrong file type or file corrupted!\n");
        return 0;
    }
    readInt(4, midiFile); // Read the header size bytes, don't store them
    header->format = readInt(2, midiFile);
    header->trackN = readInt(2, midiFile);
    header->timediv = readInt(2, midiFile);
    // Check if timing is metrical or timecode
    if (header->timediv & 0x8000) {
        fprintf(stderr, "Timecode timing not yet supported!\n");
        return 0;
    }
    return 1;
}

// Reads one MIDI event from file
// Returns 0 on faliure, 1 on success
int readEvent(listMidiEvent_t *list, FILE *midiFile) {
    midiEvent_t event;
    event.delta = readVarInt(midiFile);
    event.status = readInt(1, midiFile);
    if (event.status == 0xFF) {
        // Meta event
        event.param1 = readInt(1, midiFile);
        event.param2 = 0;
        event.dataSize = readVarInt(midiFile);
        if (event.dataSize != 0) {
            event.data = (unsigned char *)malloc(event.dataSize);
            fread(event.data, 1, event.dataSize, midiFile);
        } else {
            event.data = NULL;
        }
    } else if (event.status == 0xF0 || event.status == 0xF7) {
        // SysEx event
        event.param1 = 0;
        event.param2 = 0;
        event.dataSize = readVarInt(midiFile);
        if (event.dataSize != 0) {
            event.data = (unsigned char *)malloc(event.dataSize);
            fread(event.data, 1, event.dataSize, midiFile);
        } else {
            event.data = NULL;
        }
    } else if (event.status >= 0x80 && event.status <= 0xEF) {
        // MIDI event
        event.dataSize = 0;
        event.data = NULL;
        event.param1 = readInt(1, midiFile);
        unsigned char type = event.status & 0xF0;
        if (type != 0xC0 && type != 0xD0) {
            event.param2 = readInt(1, midiFile);
        }
    } else {
        fprintf(stderr, "Error while reading midi event - invalid status byte %02X\n", event.status);
        return 0;
    }
    listAdd(&event, list);
    return 1;
}

// Reads one track from file
// Returns 0 on faliure, 1 on success
int readTrack(midiTrack_t *track, FILE *midiFile) {
    if (readInt(4, midiFile) != TRACK_CHUNK_ID) {
        fprintf(stderr, "Error while reading track header - invalid ID\n");
        return 0;
    }
    track->size = readInt(4, midiFile);
    track->eventList = newList();
    long startPos = ftell(midiFile);

    while (ftell(midiFile) - startPos < track->size) {
        if (!readEvent(&track->eventList, midiFile)) return 0;
    }
    return 1;
}

// Reads the entire MIDI file and stores it in [midiData]
// Returns 0 on faliure, 1 on success
int readMidiData(midiData_t *midiData, FILE *midiFile) {
    if (!readHeader(&midiData->header, midiFile)) return 0;

    midiData->tracks = (midiTrack_t *)malloc(sizeof(midiTrack_t) * midiData->header.trackN);
    if (midiData->tracks == NULL) {
        fprintf(stderr, "Not enough memory available!\n");
        return 0;
    }
    for (size_t i = 0; i < midiData->header.trackN; i++) {
        if (!readTrack(&midiData->tracks[i], midiFile)) return 0;
    }
    return 1;
}


// Reads and plays the midi file
// Returns 0 on faliure, 1 on success
int playMidiFile(const char *midiFileName) {
    midiData_t midiData;

    FILE *midiFile = fopen(midiFileName, "rb");
    if (midiFile == NULL) {
        fprintf(stderr, "Error while opening file %s\n", midiFileName);
        return 0;
    }
    readMidiData(&midiData, midiFile);
    fclose(midiFile);

    if (midiData.header.format != 1) {
        fprintf(stderr, "Only format 1 MIDI files supported\n");
        freeMidiData(&midiData);
        return 0;
    }

    nodeMidiEvent_t **currEvents = (nodeMidiEvent_t **)malloc(sizeof(nodeMidiEvent_t *) * midiData.header.trackN);
    for (int i = 0; i < midiData.header.trackN; i++) {
        currEvents[i] = midiData.tracks[i].eventList.first;
    }

    // Time signature (timeSig[0] / 2^timeSig[1]) - default 4/4
    int timeSig[2] = {4, 2};
    // Track tempo in microseconds per beat - default 120bpm
    int tempo = 500000;
    // Number of tracks finished playing
    int done = 0;

    while (done < midiData.header.trackN) {
        for (int i = 0; i < midiData.header.trackN; i++) {
            if (currEvents[i] != NULL) {
                while (currEvents[i] != NULL && currEvents[i]->event.delta == 0) {
                    // Parse the event
                    switch (currEvents[i]->event.status) {
                    case META_TIME_SIGNATURE:
                        // TODO parse the remaining two bytes
                        if (i != 0) fprintf(stderr, "Warning: TimeSig event outside tempo track!\n");
                        timeSig[0] = currEvents[i]->event.data[0];
                        timeSig[1] = currEvents[i]->event.data[1];
                        break;
                    case META_TEMPO:
                        if (i != 0) fprintf(stderr, "Warning: Tempo event outside tempo track!\n");
                        tempo = currEvents[i]->event.data[2];
                        tempo += currEvents[i]->event.data[1] << 8;
                        tempo += currEvents[i]->event.data[0] << 16;
                        break;
                    case META_END_OF_TRACK:
                        done++;
                        break;
                    case META_TRACK_NAME:
                        if (i == 0) {
                            printf("Sequence name: ");
                        } else {
                            printf("Track %d name: ", i);
                        }
                        printf("%.*s\n", currEvents[i]->event.dataSize, currEvents[i]->event.data);
                        break;
                    default:
                        break;
                    }
                }
                currEvents[i] = currEvents[i]->next;
            }
        }
    }

    free(currEvents);

    freeMidiData(&midiData);

    return 1;
}