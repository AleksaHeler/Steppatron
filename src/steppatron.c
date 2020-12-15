#include <alsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>

// RawMidi ALSA hardware port to be used
#define MIDI_PORT "hw:1,0,0"

// MIDI CONSTANTS
#define HEADER_CHUNK_ID 0x4D546864
#define TRACK_CHUNK_ID 0x4D54726B

// Midi messages - 2 parameters by default
#define MSG_NOTE_OFF 0x80
#define MSG_NOTE_ON 0x90
#define MSG_NOTE_AFTERTOUCH 0xA0
#define MSG_CONTROLLER 0xB0
#define MSG_PROGRAM_CHANGE 0xC0         // 1 param
#define MSG_CHANNEL_AFTERTOUCH 0xD0     // 1 param
#define MSG_PITCH_BEND 0xE0

// SysEx events
#define STATUS_SYSEX 0xF0
#define STATUS_SYSEX_CONTINUE 0xF7
#define STATUS_ESCAPE 0xF7

// Meta events
#define STATUS_META 0xFF
#define META_SEQUENCE_NUMBER 0x00
#define META_TEXT 0x01
#define META_COPYRIGHT 0x02
#define META_TRACK_NAME 0x03
#define META_INSTRUMENT_NAME 0x04
#define META_LYRIC 0x05
#define META_MARKER 0x06
#define META_CUE_POINT 0x07
#define META_PROGRAM_NAME 0x08
#define META_DEVICE_NAME 0x09
#define META_CHANNEL_PREFIX 0x20
#define META_PORT 0x21
#define META_END_OF_TRACK 0x2F
#define META_TEMPO 0x51
#define META_SMPTE_OFFSET 0x54
#define META_TIME_SIGNATURE 0x58
#define META_KEY_SIGNATURE 0x59
#define META_SEQUENCER_SPECIFIC_EVENT 0x7F

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

void exitError(const char *error) {
    fprintf(stderr, "%s\n", error);
    exit(EXIT_FAILURE);
}

// Safely terminates the program in case of an error while reading from a file
void exitErrorFile(const char *error) {
    fclose(midiFile);
    freeMidiData(&midiData);
    exitError(error);
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
        exitErrorFile("Wrong file type or file corrupted!");
    readInt(4); // Read the header size bytes
    header->format = readInt(2);
    header->trackN = readInt(2);
    header->timediv = readInt(2);
    // Check if timing is metrical or timecode
    if (header->timediv & 0x8000)
        exitErrorFile("Timecode timing not yet supported");
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
        exitErrorFile("Error while reading midi event - invalid status byte");
    }
    listAdd(&event, list);
}

void readTrack(midiTrack_t *track) {
    if (readInt(4) != TRACK_CHUNK_ID)
        exitErrorFile("Error while reading track header - invalid ID");
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

unsigned char readUsbByte(snd_rawmidi_t* device) {
    unsigned char buffer;
    if (snd_rawmidi_read(device, &buffer, 1) < 0) {
        exitError("Problem reading MIDI input");
    }
    return buffer;
}

// Reads a midi message from usb
// Returns -1 in case of success, and the read byte in case of error
int readUsbMessage(midiMessage_t* message, snd_rawmidi_t* device) {
    unsigned char buffer = readUsbByte(device);
    if (buffer != MSG_NOTE_ON && buffer != MSG_NOTE_OFF &&
    buffer != MSG_PITCH_BEND && buffer != MSG_CONTROLLER) {
        return buffer;
    }
    message->type = buffer & 0xF0;
    message->channel = buffer & 0x0F;
    message->param1 = readUsbByte(device);
    message->param2 = readUsbByte(device);
    return -1;
}

int main(int argc, char **argv) {
    if (argc == 1) {
        // Read from USB
        snd_rawmidi_t *midiIn;
        if (snd_rawmidi_open(&midiIn, NULL, MIDI_PORT, SND_RAWMIDI_SYNC) < 0) {
            fprintf(stderr, "Cannot open port: %s\n", MIDI_PORT);
            exitError("Error while opening midi port!");
        }

        midiMessage_t message;
        int byte;
        while (1) {
            byte = readUsbMessage(&message, midiIn);
            if (byte != -1) {
                fprintf(stderr, "Invalid byte read: %d\n", byte);
                continue;
            }

            switch (message.type) {
            case MSG_NOTE_ON:
                printf("%d ON\n", message.param1);
                break;
            case MSG_NOTE_OFF:
                printf("%d OFF\n", message.param1);
                break;
            }

            fflush(stdout);
        }

        snd_rawmidi_close(midiIn);
        midiIn = NULL;
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