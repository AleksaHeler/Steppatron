#include "midiParser.h"

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

struct midi_t {
    midiData_t data;
    unsigned short timeDiv;                // Ticks per beat
    unsigned char timeSig[2];              // Time signature (timeSig[0] / 2^timeSig[1]) - default 4/4
    unsigned int currTempo;                // Track tempo in microseconds per beat - default 120bpm
    unsigned short done;                   // Number of tracks finished playing
    unsigned char currNotes[MAX_STEPPERS];
    nodeMidiEvent_t **currEvents;
    struct timespec nextEventTime;         // Absolute time of the next closest midi event
};

static inline unsigned long deltaToNs(unsigned int delta, unsigned int tempo, unsigned short timeDiv) {
    double retVal = delta;
    retVal /= timeDiv;
    retVal *= tempo * 1000;
    return retVal;
}

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

// Reads size bytes from midiFile and outputs the result as a little-endian integer
unsigned int readInt(int size, FILE *midiFile) {
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

// Reads a variable-lenght value from midiFile and outputs the result as a little-endian integer
unsigned int readVarInt(FILE *midiFile) {
    unsigned int retVal = 0;
    unsigned char buffer[4];
    int i = 0;
    while (i < 4) {
        fread(buffer + i, 1, 1, midiFile);
        // If most significant bit is 1
        if (buffer[i] & 0x80) {
            // There are more bytes
            buffer[i] ^= 0x80;
            i++;
        } else {
            // This is the last byte
            for (int j = 0; j <= i; j++) {
                retVal += buffer[j] << (7 * (i - j));
            }
            return retVal;
        }
    }
    return 0;
}

// Reads othe MIDI file header
// Returns 0 on faliure, 1 on success
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

// Reads all data from file
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

// Reads the entire MIDI file and stores it in midiData
// Returns 0 on faliure, 1 on success
int readMidiFile(midi_t *handler, const char *midiFileName) {
    FILE *midiFile = fopen(midiFileName, "rb");
    if (midiFile == NULL) {
        fprintf(stderr, "Error while opening file %s\n", midiFileName);
        return 0;
    }
    if (!readMidiData(&handler->data, midiFile)) {
        fclose(midiFile);
        return 0;
    }
    fclose(midiFile);
    return 1;
}

// Initializes the parser module handler
// Returns 0 on faliure, 1 on success
int initPlayer(midi_t *handler) {
    if (handler->data.header.format != 1) {
        fprintf(stderr, "Only format 1 MIDI files supported\n");
        return 0;
    }

    nodeMidiEvent_t **currEvents = (nodeMidiEvent_t **)malloc(sizeof(nodeMidiEvent_t *) * handler->data.header.trackN);
    if (currEvents == NULL) {
        fprintf(stderr, "Not enough memory available!\n");
        return 0;
    }
    for (int i = 0; i < handler->data.header.trackN; i++) {
        currEvents[i] = handler->data.tracks[i].eventList.first;
    }

    for (int i = 0; i < MAX_STEPPERS; i++) {
        handler->currNotes[i] = NOTE_OFF;
    }

    handler->timeDiv = handler->data.header.timediv & 0x7FFF;
    handler->timeSig[0] = 4;
    handler->timeSig[1] = 2;
    handler->currTempo = 500000;
    handler->done = 0;
    handler->nextEventTime.tv_sec = 0;

    return 1;
}

// Frees the memory 
void freeMidi(midi_t *handler) {
    free(handler->currEvents);
    freeMidiData(&handler->data);
}

// Plays the next events in the MIDI file, this function is blocking
// Returns 0 on faliure, 1 on success
int playNext(midi_t *handler, int outFile) {
    if (handler->done < handler->data.header.trackN) {
        return 0;
    }
    
    if (handler->nextEventTime.tv_sec == 0) {
        clock_gettime(CLOCK_MONOTONIC, &handler->nextEventTime);
    } else {
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &handler->nextEventTime, NULL);
    }

    unsigned int minDelta = -1;
    unsigned char buffer[2];
    for (int i = 0; i < handler->data.header.trackN; i++) {
        while (handler->currEvents[i] != NULL && handler->currEvents[i]->event.delta == 0) {
            // Parse the event
            if (handler->currEvents[i]->event.status == STATUS_META) {
                // Meta event
                switch (handler->currEvents[i]->event.param1) {
                case META_TIME_SIGNATURE:
                    // TODO parse the remaining two bytes
                    if (i != 0) fprintf(stderr, "Warning: TimeSig event outside tempo track!\n");
                    handler->timeSig[0] = handler->currEvents[i]->event.data[0];
                    handler->timeSig[1] = handler->currEvents[i]->event.data[1];
                    printf("Time signature: %d/%d\n", handler->timeSig[0], 1 << handler->timeSig[1]);
                    break;
                case META_TEMPO:
                    if (i != 0) fprintf(stderr, "Warning: Tempo event outside tempo track!\n");
                    handler->currTempo = handler->currEvents[i]->event.data[2];
                    handler->currTempo += handler->currEvents[i]->event.data[1] << 8;
                    handler->currTempo += handler->currEvents[i]->event.data[0] << 16;
                    printf("Tempo: %fbpm\n", msToBpm(handler->currTempo));
                    break;
                case META_END_OF_TRACK:
                    handler->done++;
                    break;
                case META_TRACK_NAME:
                    if (i == 0) {
                        printf("Sequence name: ");
                    } else {
                        printf("Track %d name: ", i);
                    }
                    printf("%.*s\n", handler->currEvents[i]->event.dataSize, handler->currEvents[i]->event.data);
                    break;
                default:
                    fprintf(stderr, "Error while parsing meta event\n");
                    break;
                }
            } else {
                // MIDI event
                unsigned char statusUpper = handler->currEvents[i]->event.status & 0xF0;
                switch (statusUpper) {
                case MSG_NOTE_ON:
                    handler->currNotes[i - 1] = handler->currEvents[i]->event.param1;
                    buffer[0] = i - 1;
                    buffer[1] = handler->currNotes[i - 1];
                    printf("Note %d on stepper %d ON\n", buffer[1], buffer[0]);
                    write(outFile, buffer, 2);
                    break;
                case MSG_NOTE_OFF:
                    if (handler->currEvents[i]->event.param1 == handler->currNotes[i - 1]) {
                        handler->currNotes[i - 1] = NOTE_OFF;
                        buffer[0] = i - 1;
                        buffer[1] = NOTE_OFF;
                        printf("Note on stepper %d OFF\n", buffer[0]);
                        write(outFile, buffer, 2);
                    }
                    break;
                default:
                    break;
                }
            }
            // Go to next event
            handler->currEvents[i] = handler->currEvents[i]->next;
        }
        if (handler->currEvents[i] != NULL && handler->currEvents[i]->event.delta < minDelta) minDelta = handler->currEvents[i]->event.delta;
    }
    if (minDelta == -1) {
        return 0;
    } 

    for (int i = 0; i < handler->data.header.trackN; i++) {
        if (handler->currEvents[i] != NULL) handler->currEvents[i]->event.delta -= minDelta;
    }
    unsigned long deltaNs = deltaToNs(minDelta, handler->currTempo, handler->timeDiv);
    handler->nextEventTime.tv_sec += (deltaNs + handler->nextEventTime.tv_nsec) / NS_PER_S;
    handler->nextEventTime.tv_nsec = (deltaNs + handler->nextEventTime.tv_nsec) % NS_PER_S;

    return 1;
}