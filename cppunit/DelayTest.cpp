#include <stdio.h>
#include <fcntl.h>    /* For O_RDWR */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "midi.h"

#include "DelayTest.h"

CPPUNIT_TEST_SUITE_REGISTRATION( DelayTest );

void DelayTest::setUp()
{
    printf("-");
    char input[2];
    int ret_val_write;

    system("sudo dmesg -C");
    file_desc = open("/dev/gpio_driver", O_RDWR); 
    CPPUNIT_ASSERT( file_desc >= 0); //maybe can't open file

    for(int i = 0; i < MAX_STEPPERS; i++)
    {
        input[0] = i;
        for(int j = 21; j < 109; j++)
        {
            input[1] = j;
            ret_val_write = write(file_desc, input, 2);
        }
    }

    close(file_desc);

    system("dmesg -t -d > dmesg_log.txt");
    file_desc = open("dmesg_log.txt", O_RDWR);
    CPPUNIT_ASSERT(file_desc >= 0); //maybe can't open file
}

void DelayTest::tearDown()
{
    close(file_desc);
    system("rm dmesg_log.txt");
}

void DelayTest::delayTest()
{
    int ret_val_read;
    char read_buffer[BUFF_LEN];
    char c;

    for(int i = 0; i < MAX_STEPPERS; i++)
    {
        for(int j = 21; j<109; j++)
        {
            ret_val_read = read(file_desc, read_buffer, 16);
            CPPUNIT_ASSERT(ret_val_read == 16); //maybe error occured while reading

            read_buffer[16] = '\0';
            printf("Stepper: %d | Note: %d | time delta spent between messages: %s\n", i, j ,read_buffer);

            CPPUNIT_ASSERT(read_buffer[11] == '0' || read_buffer[11 == '1']); //[0.0, 0.2)ms tolerance
            CPPUNIT_ASSERT(read_buffer[6] == '0');
            CPPUNIT_ASSERT(read_buffer[8] == '0');
            CPPUNIT_ASSERT(read_buffer[9] == '0');
            CPPUNIT_ASSERT(read_buffer[10] == '0');
               
            c = '\0';
            while(c != '\n') //go to new line
            {
                ret_val_read = read(file_desc, &c, 1);
                CPPUNIT_ASSERT(ret_val_read == 1); //maybe error occured while reading
            }
        }
    }

}
