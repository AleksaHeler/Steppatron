#!/bin/bash
### Compiles code and runs steppatron with char keyboard input ###
# first use: "chmod +x run.sh"
# then to run: ""./run.sh" 

MAJOR_NUMBER=239

# Makefile
make clean
make

# Kernel module
sudo rmmod /dev/gpio_driver
sudo mknod /dev/gpio_driver c $MAJOR_NUMBER 0
sudo chmod 666 /dev/gpio_driver
sudo insmod src/gpio_driver.ko argc=1 steppers=23

# Steppatron application
./bin/steppatron k
