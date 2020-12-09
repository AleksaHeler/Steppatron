/* 
 * Simple code to test if stepper motor driver works with PWM signal
 * Not precise at all, just testing
 *
 * Compile:
 *  gcc -o pwm pwm.c -lwiringPi
 * 
 * Run:
 *  ./pwm [frequency] 
 *  ./pwm 440
*/

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>

// Pins
#define FREQ_PIN    4   // GPIO 23 (pin 16)
#define DIR_PIN     5   // GPIO 24 (pin 18)

// Parameters for ramping up the motor speed
#define START_FREQ  400

// Frequency to microseconds
// 1/freq * 1,000,000
#define FREQ_TO_US(X) (1000000.0 / (X))

int main(int argc, char *argv[]){
    float i, t, increment;
    float frequency = 1000;
    int direction = 0;

    printf("GPIO PWM Stepper control\n");
    
    if(argc == 2){
        frequency = atoi(argv[1]);
    }
    else{
        printf("[ERROR] Invalid arguments!\n");
        printf("  Use: ./pwm [frequency]\n");
        return 1;
    }

    // Initializing WiringPi library
    if(wiringPiSetup() == -1){
        printf("[ERROR] Could not initiate WiringPi library!\n");
        return 1;
    }

    // Initializing pins
    pinMode(DIR_PIN, OUTPUT);
    pinMode(FREQ_PIN, OUTPUT);

    // Set direction
    digitalWrite(DIR_PIN, direction);

    printf("Playing %.2fHz\n", frequency);

    // Ramping up the motor to prevent it from oscillating in one spot instead of turning
    increment = (frequency - START_FREQ) / 40;
    for(i = START_FREQ; i < frequency; i += increment){
        t = FREQ_TO_US(i)/2;
        digitalWrite(FREQ_PIN, 1);      // Turn on
        usleep(t);                      // Wait for half the period
        digitalWrite(FREQ_PIN, 0);      // Turn off
        usleep(t);
    }

    // Actually play the note
    t = FREQ_TO_US(frequency)/2;
    while(1){
        digitalWrite(FREQ_PIN, 1);      // Turn on
        usleep(t);                      // Wait for half the period
        digitalWrite(FREQ_PIN, 0);      // Turn off
        usleep(t);
    }

    return 0;
}
