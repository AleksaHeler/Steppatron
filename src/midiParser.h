#ifndef MIDIPARSER_H
#define MIDIPARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "midi.h"

// Contains all the data stored by the parser and the player
// Represents one midi file
typedef struct midi_t midi_t;

// Reads the entire MIDI file and stores it in midiData
// Returns 0 on faliure, 1 on success
int readMidiFile(midi_t *handler, const char *midiFileName);

// Initializes the parser module handler
// Returns 0 on faliure, 1 on success
int initPlayer(midi_t *handler);

// Frees the memory
void freeMidi(midi_t *handler);

// Plays the next events in the MIDI file, this function is blocking
// Writes the steppatron commands to outFile
// Returns 0 on faliure, 1 on success
int playNext(midi_t *handler, int outFile);

#endif