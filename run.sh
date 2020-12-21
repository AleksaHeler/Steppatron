#!/bin/bash
### Compiles code and runs steppatron with char keyboard input ###
#
# FIRST CHANGE TO EXECUTABLE: "chmod +x run.sh"
# then to run: ""./run.sh [make] [keyboard] [file] [usb]"
#
# What does this file do?
#   make clean
#   make
#   rmmod gpio_driver
#   mknod /dev/gpio_driver c MAJOR_NUMBER 0
#   chmod 666 /dev/gpio_driver
#   insmod gpio_driver
#   ./steppatron k

MAJOR_NUMBER=239
BLUE='\033[0;36m'
GRAY='\033[1;30m'
NC='\033[0m' # No Color

# Makefile
if [[ $@ == *"make"* ]]; then
    echo -e "${BLUE}> make clean${GRAY}"
    make clean
    echo -e "${BLUE}> make${GRAY}"
    make
fi

# Kernel module
echo -e "${BLUE}> rmmod${GRAY}"
sudo rmmod /dev/gpio_driver
echo -e "${BLUE}> mknod${GRAY}"
sudo mknod /dev/gpio_driver c $MAJOR_NUMBER 0
echo -e "${BLUE}> chmod${GRAY}"
sudo chmod 666 /dev/gpio_driver
echo -e "${BLUE}> insmod${GRAY}"
sudo insmod src/gpio_driver.ko argc=1 steppers=23

# Steppatron application
if [[ $@ == *"keyboard"* ]]; then
    echo -e "${BLUE}> steppatron keyboard${NC}"
    ./bin/steppatron k
elif [[ $@ == *"file"* ]]; then
    echo -e "${BLUE}> steppatron file${NC}"
    ./bin/steppatron f
elif [[ $@ == *"usb"* ]]; then
    echo -e "${BLUE}> steppatron usb${NC}"
    ./bin/steppatron u
else
    echo -e "${BLUE}> steppatron keyboard${NC}"
    ./bin/steppatron k
fi

