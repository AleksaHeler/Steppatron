#include <stdio.h>
#include <fcntl.h>    /* For O_RDWR */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "midi.h"
#include "getch.h"

#include "KeyboardTest.h"

CPPUNIT_TEST_SUITE_REGISTRATION( KeyboardTest );

void KeyboardTest::setUp()
{
    printf("-");
    file_desc = open("/dev/gpio_driver", O_RDWR); 
    CPPUNIT_ASSERT( file_desc >= 0); //maybe can't open file

    input[0] = 0; //test is performed on first stepper
    system("sudo dmesg -C");
}

void KeyboardTest::tearDown()
{
    close(file_desc);
    system("rm dmesg_log.txt");
}

void KeyboardTest::octave1Test()
{
    octaveTest(1);
}

void KeyboardTest::octave2Test()
{
    octaveTest(2);
}

void KeyboardTest::octave3Test()
{
    octaveTest(3);
}

void KeyboardTest::octave4Test()
{
    octaveTest(4);
}

void KeyboardTest::octave5Test()
{
    octaveTest(5);
}

void KeyboardTest::octave6Test()
{
    octaveTest(6);
}

void KeyboardTest::octave7Test()
{
    octaveTest(7);
}

void KeyboardTest::octaveTest(int octave)
{
    messageOrder(octave);

    for(int i = 0; i<15; i++)
    {
        input[1] = playNote(octave);

        ret_val = write(file_desc, input, 2);
        CPPUNIT_ASSERT_EQUAL(ret_val, 2);
    }

    close(file_desc);

    system("dmesg -t > dmesg_log.txt");
    file_desc = open("dmesg_log.txt", O_RDWR);
    CPPUNIT_ASSERT(file_desc >= 0); //maybe can't open file

    char expected_note, c;

    for(int i = 0; i<15; i++)
    {
        expected_note = correctNote(octave, i);
        
        if( (expected_note == (char)EOF) && ((i == 7) || (i == 14)) )
            continue;
        if ( (expected_note == 0) && (i == 13))
            continue;

        ret_val_read = read(file_desc, read_buffer, 16);
        CPPUNIT_ASSERT(ret_val_read > 0); //maybe error occured while reading

        sprintf(write_buffer, "%d -> note %d\0", 0, expected_note);

        write_buffer[strlen(write_buffer)] = '\0';
        read_buffer[strlen(write_buffer)] = '\0';

        printf("\n---------------------------------\n");
        printf("Expected: %s\n", write_buffer);
        printf("Actual: %s\n", read_buffer);

        CPPUNIT_ASSERT( strcmp(read_buffer, write_buffer) == 0); //check read_buffer and write_buffer

        c = '\0';
        while(c != '\n') //go to new line
        {
            ret_val_read = read(file_desc, &c, 1);
            CPPUNIT_ASSERT(ret_val_read == 1); //maybe error occured while reading
        }
    }
}

char KeyboardTest::correctNote(int octave, int i)
{
    if(i == 0) //C
        return (char)C[octave].MIDINumber;

    else if(i == 1) //D
        return (char)D[octave].MIDINumber;

    else if(i == 2) //E
        return (char)E[octave].MIDINumber;

    else if(i == 3) //F
        return (char)F[octave].MIDINumber;

    else if(i == 4) //G
        return (char)G[octave].MIDINumber;

    else if(i == 5) //A
        return (char)A[octave].MIDINumber;

    else if(i == 6) //B
        return (char)B[octave].MIDINumber;

    else if(i == 7) //stop sign
        return (char)EOF;

    else if(i == 8) // C/D
        return (char)M[5 * (octave - 1)].MIDINumber;

    else if(i == 9) // D/E
        return (char)M[5 * (octave - 1) + 1].MIDINumber;

    else if(i == 10) // F/G
        return (char)M[5 * (octave - 1) + 2].MIDINumber;

    else if(i == 11) // G/A
        return (char)M[5 * (octave - 1) + 3].MIDINumber;

    else if(i == 12) // A/B
        return (char)M[5 * (octave - 1) + 4].MIDINumber;

    else if(i == 13) // stop sign
        return 0;

    else //i == 14
        return (char)EOF;

}

char inOrder[] = {'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'w', 'e', 't', 'y', 'u', 'i', 'q'};
int brojac = 0;

char KeyboardTest::playNote(int octave)
{
    char c;
    while (1) {
        c = getch();
        printf("Pressed: %c\n", c);
        CPPUNIT_ASSERT_EQUAL(inOrder[brojac++], c);        

        switch (c) {
            //donji red
            case 'a':
                return (char)C[octave].MIDINumber;

            case 's':
                return (char)D[octave].MIDINumber;
                
            case 'd':
                return (char)E[octave].MIDINumber;
                
            case 'f':
                return (char)F[octave].MIDINumber;
                
            case 'g':
                return (char)G[octave].MIDINumber;
                
            case 'h':
                return (char)A[octave].MIDINumber;
                
            case 'j':
                return (char)B[octave].MIDINumber;
                
            //gornji red
            case 'w':
                return (char)M[5 * (octave - 1)].MIDINumber;
                
            case 'e':
                return (char)M[5 * (octave - 1) + 1].MIDINumber;
                
            case 't':
                return (char)M[5 * (octave - 1) + 2].MIDINumber;
                
            case 'y':
                return (char)M[5 * (octave - 1) + 3].MIDINumber;
                
            case 'u':
                return (char)M[5 * (octave - 1) + 4].MIDINumber;
                
            case 'q':  /* Exit */
                //return 0xF0; //in actual program
                return 0xFF;

            default: /* Ako se pritisne neodredjen taster, prekinuce ton */
                return 0xFF;
        }
    }
}

void KeyboardTest::messageOrder(int octave)
{
    printf("\n\nOctave: %d", octave);
    printf("\n----------------------------------\n");
    printf("Play notes in the following order:\n");
    printf("a s d f g h j k w e t y u i q");
    printf("\n----------------------------------\n"); 
}   
