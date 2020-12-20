#include <stdio.h>
#include <alsa/asoundlib.h>
#include "midi.h"

// RawMidi ALSA hardware port to be used
#define MIDI_PORT "hw:1,0,0"

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
int rawmidiInit(snd_rawmidi_t *handler) {
    if (snd_rawmidi_open(&handler, NULL, MIDI_PORT, SND_RAWMIDI_SYNC) < 0) {
        fprintf(stderr, "Cannot open port: %s\n", MIDI_PORT);
        return 0;
    }
    return 1;
}

// Closes the RawMIDI interface
void rawmidiClose(snd_rawmidi_t *handler) {
    snd_rawmidi_close(handler);
    handler = NULL;
}

// Gets the next command to be sent to the steppatron driver
// from the RawMIDI interface
// Returns 0 on faliure, note number on success
int getRawmidiCommand(char *command, snd_rawmidi_t *handler) {
    midiMessage_t message;
    int byte = readUsbMessage(&message, handler);
    if (byte != -1) {
        fprintf(stderr, "Invalid byte read: %d\n", byte);
        return 0;
    }

    switch (message.type) {
    case MSG_NOTE_ON:
        command[1] = message.param1;
        break;
    case MSG_NOTE_OFF:
        command[1] = 0xFF;
        break;
    default:
        return 0;
    }

    // TODO - procesovanje sa vise steppera
    command[0] = 0;

    return message.param1;
}