#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "MIDITable.h"

#define FILE_NAME "testFile.txt"

char getch(void);

int main()
{
    int file_desc = open(FILE_NAME, O_RDWR);

    if(file_desc < 0)
    {
        printf("Error, %s not opened\n", FILE_NAME);
        return -1;
    }

    int ret_val;
    char input;
    int octave;
    
    printf("Choose octave [1-7]: ");
    scanf("%d", &octave);
    getch(); //da pokupi enter

    printf("Start playing notes\n"); 

    while(1) 
    {      
        input = getch();

        switch(input)
        {
            //donji red
            case 'a':
                input = (char)C[octave].MIDINumber;
                break;
            case 's':
                input = (char)D[octave].MIDINumber;
                break;
            case 'd':
                input = (char)E[octave].MIDINumber;
                break;
            case 'f':
                input = (char)F[octave].MIDINumber;
                break;
            case 'g':
                input = (char)G[octave].MIDINumber;
                break;  
            case 'h':
                input = (char)A[octave].MIDINumber;
                break; 
            case 'j':
                input = (char)B[octave].MIDINumber;
                break; 

            //gornji red
            case 'w':
                input = (char)M[5*(octave-1)].MIDINumber;
                break; 

            case 'e':
                input = (char)M[5*(octave-1) + 1].MIDINumber;
                break; 

            case 't':
                input = (char)M[5*(octave-1) + 2].MIDINumber;
                break;

            case 'y':
                input = (char)M[5*(octave-1) + 3].MIDINumber;
                break;

            case 'u':
                input = (char)M[5*(octave-1) + 4].MIDINumber;
                break;

            default:
                printf("Unknown key\n");
                continue;
        }

        ret_val = write(file_desc, &input, 1);

        if(ret_val == 0)
        {
            printf("Error writing to file\n");
            return 2;
        }

    };

    printf("\nPlaying notes finished\n");

    close(file_desc);

    return 0;
}