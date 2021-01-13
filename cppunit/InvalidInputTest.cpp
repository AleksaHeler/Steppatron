#include <stdio.h>
#include <fcntl.h>    /* For O_RDWR */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include"InvalidInputTest.h"

CPPUNIT_TEST_SUITE_REGISTRATION( InvalidInputTest );

void InvalidInputTest::setUp()
{
    printf("-");
    fflush(stdout);

    system("sudo dmesg -C");
    memset(read_buffer, 0, BUFF_LEN);
}

void InvalidInputTest::tearDown()
{
    close(file_desc);
}

void InvalidInputTest::invalidStepperTest()
{
    printf("\nTesting steppers over 4 because they dont exist\n");

    file_desc = open("/dev/gpio_driver", O_RDWR); 
    CPPUNIT_ASSERT( file_desc >= 0); //maybe can't open file

    for(int i = MAX_STEPPERS; i<256; i++)
    {
        input[0] = i;
        for(int j = 21; j<109; j++)
        {
            input[1] = j;

            printf("Trying to write Stepper:%d with Note:%d: ", i, j);
            fflush(stdout);

            ret_val_write = write(file_desc, input, 2);
            CPPUNIT_ASSERT_EQUAL(ret_val_write, 22); //22 should be returned (22 == EINVAL)

            printf("OK\n");
        }
    }

}

void InvalidInputTest::invalidNoteUnderTest()
{
    printf("\nTesting notes under 21 because they don't exist\n");

    file_desc = open("/dev/gpio_driver", O_RDWR); 
    CPPUNIT_ASSERT( file_desc >= 0); //maybe can't open file

    for(int i = 0; i<MAX_STEPPERS; i++)
    {
        input[0] = i;
        for(int j = 0; j<21; j++)
        {
            input[1] = j;
            
            printf("Trying to write Stepper:%d with Note:%d: ", i, j);
            fflush(stdout);

            ret_val_write = write(file_desc, input, 2);
            CPPUNIT_ASSERT(ret_val_write == 2); //maybe error occured while writing

            printf("OK\n");
        }
    }

    close(file_desc);

    system("dmesg -t > dmesg_log.txt");
    file_desc = open("dmesg_log.txt", O_RDWR);
    CPPUNIT_ASSERT(file_desc >= 0); //maybe can't open file

    ret_val_read = read(file_desc, read_buffer, BUFF_LEN);
    CPPUNIT_ASSERT_EQUAL(ret_val_read, 0); //if nothing is read then nothing is written in "dmesg_log.txt"
    
    system("rm dmesg_log.txt");
}

void InvalidInputTest::invalidNoteOverTest()
{
    printf("\nTesting notes over 108 because they don't exist\n");

    file_desc = open("/dev/gpio_driver", O_RDWR); 
    CPPUNIT_ASSERT( file_desc >= 0); //maybe can't open file

    for(int i = 0; i<MAX_STEPPERS; i++)
    {
        input[0] = i;
        for(int j = 109; j<256; j++)
        {
            input[1] = j;
            
            printf("Trying to write Stepper:%d with Note:%d: ", i, j);
            fflush(stdout);

            ret_val_write = write(file_desc, input, 2);
            CPPUNIT_ASSERT(ret_val_write == 2); //maybe error occured while writing

            printf("OK\n");
        }
    }

    close(file_desc);

    system("dmesg -t > dmesg_log.txt");
    file_desc = open("dmesg_log.txt", O_RDWR);
    CPPUNIT_ASSERT(file_desc >= 0); //maybe can't open file

    ret_val_read = read(file_desc, read_buffer, BUFF_LEN);
    CPPUNIT_ASSERT_EQUAL(ret_val_read, 0); //if nothing is read then nothing is written in "dmesg_log.txt"
    
    system("rm dmesg_log.txt");
}

void InvalidInputTest::invalidAllTest()
{
    printf("\nTesting steppers over 4 because they dont exist\n");

    file_desc = open("/dev/gpio_driver", O_RDWR); 
    CPPUNIT_ASSERT( file_desc >= 0); //maybe can't open file

    for(int i = MAX_STEPPERS; i<256; i++)
    {
        input[0] = i;
        for(int j = 0; j<21; j++)
        {
            input[1] = j;

            printf("Trying to write Stepper:%d with Note:%d: ", i, j);
            fflush(stdout);

            ret_val_write = write(file_desc, input, 2);
            CPPUNIT_ASSERT_EQUAL(ret_val_write, 22); //22 should be returned (22 == EINVAL)

            printf("OK\n");
        }

        printf("-------------------\n");

        for(int j = 109; j<256; j++)
        {
            input[1] = j;

            printf("Trying to write Stepper:%d with Note:%d: ", i, j);
            fflush(stdout);

            ret_val_write = write(file_desc, input, 2);
            CPPUNIT_ASSERT_EQUAL(ret_val_write, 22); //22 should be returned (22 == EINVAL)

            printf("OK\n");
        }
    }

}
