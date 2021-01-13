#ifndef RAWMIDI_H
#define RAWMIDI_H

#include <stdio.h>
#include <alsa/asoundlib.h>
#include "midi.h"

// Initializes the RawMIDI module
int rawmidiInit(snd_rawmidi_t **handler, unsigned int steppers);

// Deinitializes the RawMIDI module
void rawmidiClose(snd_rawmidi_t *handler);

// Reads a midi message from usb
// Returns -1 in case of success, and the read byte in case of error
int readUsbMessage(midiMessage_t *message, snd_rawmidi_t *handler);

// Gets the next command to be sent to the steppatron driver
// from the RawMIDI interface
// Returns 0 on faliure, note number on success
int getRawmidiCommand(unsigned char *command, snd_rawmidi_t *handler);

#endif