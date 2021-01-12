#ifndef MIDI_H
#define MIDI_H

#define MAX_STEPPERS 4

#define NOTE_OFF 0xFF

// MIDI CONSTANTS

#define HEADER_CHUNK_ID 0x4D546864
#define TRACK_CHUNK_ID 0x4D54726B

// Midi messages - 2 parameters by default
#define MSG_NOTE_OFF 0x80
#define MSG_NOTE_ON 0x90
#define MSG_NOTE_AFTERTOUCH 0xA0
#define MSG_CONTROLLER 0xB0
#define MSG_PROGRAM_CHANGE 0xC0     // 1 param
#define MSG_CHANNEL_AFTERTOUCH 0xD0 // 1 param
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

// STRUCTS

typedef struct {
    unsigned char type;
    unsigned char channel;
    unsigned char param1;
    unsigned char param2;
} midiMessage_t;

// HELPER FUNCTIONS

#define MICROSECONDS_PER_MINUTE 60000000
#define NS_PER_S 1000000000

static inline float msToBpm(unsigned int ms) {
    return MICROSECONDS_PER_MINUTE / ms;
}

static inline unsigned int bpmToMs(unsigned int bpm) {
    return MICROSECONDS_PER_MINUTE / bpm;
}

#endif