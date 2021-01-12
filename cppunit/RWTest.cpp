#include <stdio.h>
#include <fcntl.h>    /* For O_RDWR */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "midi.h"

#include "RWTest.h"

CPPUNIT_TEST_SUITE_REGISTRATION( RWTest );

void RWTest::setUp()
{}

void RWTest::tearDown()
{
    close(file_desc);
    system("rm dmesg_log.txt");
}

void RWTest::read_write()
{
    char input[2];
    int ret_val_write, ret_val_read;
    char read_buffer[BUFF_LEN];
    char write_buffer[BUFF_LEN];

    for(int i = 0; i < MAX_STEPPERS; i++)
    {
        input[0] = i;
        for(int j = 21; j < 109; j++)
        {
            system("sudo dmesg -C");

            file_desc = open("/dev/gpio_driver", O_RDWR); 
            CPPUNIT_ASSERT( file_desc >= 0); //maybe can't open file

            input[1] = j;
            ret_val_write = write(file_desc, input, 2);
            CPPUNIT_ASSERT(ret_val_write == 2); //maybe error occured while writing

            close(file_desc);

            system("dmesg -t > dmesg_log.txt");
            file_desc = open("dmesg_log.txt", O_RDWR);
            CPPUNIT_ASSERT(file_desc >= 0); //maybe can't open file

            ret_val_read = read(file_desc, read_buffer, BUFF_LEN);
            CPPUNIT_ASSERT(ret_val_read > 0); //maybe error occured while reading

            sprintf(write_buffer, "%d -> note %d\0", i, j);

            write_buffer[strlen(write_buffer)] = '\0';
            read_buffer[strlen(write_buffer)] = '\0';

            printf("\n---------------------------------\n");
            printf("Expected: %s\n", write_buffer);
            printf("Actual: %s\n", read_buffer);

            CPPUNIT_ASSERT( strcmp(read_buffer, write_buffer) == 0); //check read_buffer and write_buffer

            close(file_desc);
        }
   }
}