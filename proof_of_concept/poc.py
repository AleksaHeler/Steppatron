#!/usr/bin/env python

#define MSG_NOTE_OFF 0x80
#define MSG_NOTE_ON 0x90
#define MSG_NOTE_AFTERTOUCH 0xA0
#define MSG_CONTROLLER 0xB0
#define MSG_PROGRAM_CHANGE 0xC0     # 1 param
#define MSG_CHANNEL_AFTERTOUCH 0xD0 # 1 param
#define MSG_PITCH_BEND 0xE0

midiTable = {
#  MIDINumber,  freq, period(ms),  note
    21: 27.500,  #    A0 
    22: 29.135,  # A0/B0 
    23: 30.868,  #    B0 
    24: 32.703,  #    C1 
    25: 34.684,  # C1/D1 
    26: 36.708,  #    D1 
    27: 38.891,  # D1/E1 
    28: 41.203,  #    E1 
    29: 43.654,  #    F1 
    30: 46.249,  # F1/G1 
    31: 48.999,  #    G1 
    32: 51.913,  # G1/A1 
    33: 55.000,  #    A1 
    34: 58.270,  # A1/B1 
    35: 61.735,  #    B1 
    36: 65.406,  #    C2 
    37: 69.296,  # C2/D2 
    38: 73.416,  #    D2 
    39: 77.782,  # D2/E2 
    40: 82.407,  #    E2 
    41: 87.307,  #    F2 
    42: 92.499,  # F2/G2 
    43: 97.999,  #    G2 
    44: 103.83,  # G2/A2 
    45: 110.00,  #    A2  
    46: 116.54,  # A2/B2 
    47: 123.47,  #    B2 
    48: 130.81,  #    C3 
    49: 138.59,  # C3/D3 
    50: 146.83,  #    D3 
    51: 155.56,  # D3/E3 
    52: 164.81,  #    E3 
    53: 174.61,  #    F3 
    54: 185.00,  # F3/G3 
    55: 196.00,  #    G3 
    56: 207.65,  # G3/A3 
    57: 220.00,  #    A3 
    58: 233.08,  # A3/B3 
    59: 246.94,  #    B3 
    60: 261.63,  #    C4 
    61: 277.18,  # C4/D4 
    62: 293.67,  #    D4 
    63: 311.13,  # D4/E4 
    64: 329.23,  #    E4 
    65: 349.23,  #    F4 
    66: 369.99,  # F4/G4 
    67: 392.00,  #    G4 
    68: 415.30,  # G4/A4 
    69: 440.00,  #    A4 
    70: 466.16,  # A4/B4 
    71: 493.88,  #    B4 
    72: 523.25,  #    C5 
    73: 554.37,  # C5/D5 
    74: 587.33,  #    D5 
    75: 622.25,  # D5/E5 
    76: 659.26,  #    E5 
    77: 698.46,  #    F5 
    78: 739.99,  # F5/G5 
    79: 783.99,  #    G5 
    80: 830.61,  # G5/A5   
    81: 880.00,  #    A5 
    82: 923.33,  # A5/B5 
    83: 987.77,  #    B5 
    84: 1046.5,  #    C6
    85: 1108.7,  # C6/D6
    86: 1174.7,  #    D6
    87: 1244.5,  # D6/E6
    88: 1318.5,  #    E6
    89: 1396.9,  #    F6
    90: 1480.0,  # F6/G6
    91: 1568.0,  #    G6
    92: 1661.2,  # G6/A6
    93: 1760.0,  #    A6
    94: 1864.7,  # A6/B6
    95: 1975.5,  #    B6
    96: 2093.0,  #    C7
    97: 2217.5,  # C7/D7
    98: 2349.3,  #    D7
    99: 2489.0,  # D7/E7
    100: 2637.0,  #    E7
    101: 2793.0,  #    F7
    102: 2960.0,  # F7/G7
    103: 3136.0,  #    G7
    104: 3222.4,  # G7/A7
    105: 3520.0,  #    A7
    106: 3792.3,  # A7/B7
    107: 3951.1,  #    B7
    108: 4186.0   #    C8 
}

import sys
import RPi.GPIO as GPIO
import atexit
import fileinput

atexit.register(GPIO.cleanup)

GPIO.setmode(GPIO.BCM)
GPIO.setup(24, GPIO.OUT, initial=GPIO.LOW)
GPIO.setup(23, GPIO.OUT)

note = 0
p = 0

for data in fileinput.input():
    numbers = data.split(' ')
    cmdNote = int(numbers[0])

    if numbers[1] == "ON\n":
        if note != 0:
            p.stop()
            GPIO.cleanup(23)
            GPIO.setup(23, GPIO.OUT)
            p = 0
        note = cmdNote
        p = GPIO.PWM(23, midiTable[note])
        p.start(1)
        print("ON: ", midiTable[note])

    elif numbers[1] == "OFF\n" and note != 0 and cmdNote == note:
        p.stop()
        GPIO.cleanup(23)
        GPIO.setup(23, GPIO.OUT)
        p = 0
        print("OFF: ", midiTable[note])
        note = 0
