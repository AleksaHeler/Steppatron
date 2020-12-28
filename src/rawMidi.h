#include <stdio.h>
#include <alsa/asoundlib.h>
#include "midi.h"

// RawMidi ALSA hardware port to be used
#define MIDI_PORT "hw:1,0,0"

// Notes being played
unsigned char *currNotes;
unsigned int stepperN;

unsigned char readUsbByte(snd_rawmidi_t *device) {
    unsigned char buffer;
    if (snd_rawmidi_read(device, &buffer, 1) < 0) {
        fprintf(stderr, "Problem reading RawMIDI input!\n");
        return 0;
    }
    return buffer;
}

// Reads a midi message from usb
// Returns -1 in case of success, and the read byte in case of error
int readUsbMessage(midiMessage_t *message, snd_rawmidi_t *device) {
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

// Initializes the RawMIDI interface
int rawmidiInit(snd_rawmidi_t **handler, unsigned int steppers) {
    if (snd_rawmidi_open(handler, NULL, MIDI_PORT, SND_RAWMIDI_SYNC) < 0) {
        fprintf(stderr, "Cannot open port: %s\n", MIDI_PORT);
        return 0;
    }
    if (steppers < MAX_STEPPERS && steppers != 0) {
        stepperN = steppers;
    } else {
        stepperN = 1;
    }
    currNotes = (int *)malloc(sizeof(int) * stepperN);
    return 1;
}

// Closes the RawMIDI interface
void rawmidiClose(snd_rawmidi_t *handler) {
    snd_rawmidi_close(handler);
    free(currNotes);
    handler = NULL;
}

inline int dist(unsigned char a, unsigned char b) {
    return a > b ? a - b : b - a;
}

// Finds the next free stepper, if none available finds the one playing the nearest note
size_t getFreeStepper(unsigned char note) {
    for (size_t i = 0; i < stepperN; i++) {
        if (currNotes[i] == NOTE_OFF) return i;
    }
    size_t nearest = 0;
    for (size_t i = 0; i < stepperN; i++) {
        if (dist(currNotes[i], note) < dist(currNotes[nearest], note)) nearest = i;
    }
    return nearest;
}

// Gets the next command to be sent to the steppatron driver
// from the RawMIDI interface
// Returns 0 on faliure, note number on success
int getRawmidiCommand(unsigned char *command, snd_rawmidi_t *handler) {
    midiMessage_t message;
    int byte = readUsbMessage(&message, handler);
    if (byte != -1) {
        fprintf(stderr, "Invalid byte read: %d\n", byte);
        return 0;
    }

    switch (message.type) {
    case MSG_NOTE_ON:
        command[1] = message.param1;
        command[0] = getFreeStepper(command[1]);
        currNotes[command[0]] = command[1];
        break;
    case MSG_NOTE_OFF:
        int found = 0;
        for (size_t i = 0; i < stepperN; i++) {
            if (currNotes[i] == message.param1) {
                command[0] = i;
                currNotes[i] = NOTE_OFF;
                found = 1;
                break;
            }
        }
        if (!found) return 0;
        command[1] = NOTE_OFF;
        break;
    default:
        return 0;
    }

    return message.param1;
}