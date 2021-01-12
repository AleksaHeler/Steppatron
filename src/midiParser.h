#ifndef MIDIPARSER_H
#define MIDIPARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
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

// Contains all the data stored by the parser and the player
// Represents one midi file
typedef struct {
    midiData_t data;
    unsigned short timeDiv;   // Ticks per beat
    unsigned char timeSig[2]; // Time signature (timeSig[0] / 2^timeSig[1]) - default 4/4
    unsigned int currTempo;   // Track tempo in microseconds per beat - default 120bpm
    unsigned short done;      // Number of tracks finished playing
    unsigned char currNotes[MAX_STEPPERS];
    nodeMidiEvent_t **currEvents;
    struct timespec nextEventTime; // Absolute time of the next closest midi event
} midi_t;

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