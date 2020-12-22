#!/bin/bash
### FIRST CHANGE TO EXECUTABLE: "sudo chmod +x run.sh" ###
# ./run.sh                      - Bez kompajlovanja samo ucita driver i pokrene steppatron sa tastaturom
# ./run.sh file filename.mid    - Bez komp, ucita i pokrene citanje iz fajla
# ./run.sh usb                  - Bez komp, ucita i pokrene sa usb midi klavijaturom
#   Dodatni opcioni parametri:
#       make                    - Kompajluje
#       lib                     - Instalira libasound2 biblioteku
#       make                    - Kompajluje

### Parameters ###
STEPPER_COUNT=2
STEPPER_STEP_PINS=23,24
STEPPER_EN_PINS=27,22
MAJOR_NUMBER=239

### Colors ###
BLUE='\033[0;36m'
GRAY='\033[1;30m'
NC='\033[0m' # No Color

# Library
if [[ $@ == *"lib"* ]]; then
    echo -e "${BLUE}> sudo apt-get install libasound2-dev${GRAY}"
    sudo apt-get install libasound2-dev
fi

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
echo -e "${BLUE}> rm node${GRAY}"
sudo rm /dev/gpio_driver
echo -e "${BLUE}> mknod${GRAY}"
sudo mknod /dev/gpio_driver c $MAJOR_NUMBER 0
echo -e "${BLUE}> chmod${GRAY}"
sudo chmod 666 /dev/gpio_driver
echo -e "${BLUE}> insmod${GRAY}"
sudo insmod src/gpio_driver.ko steppers_count=$STEPPER_COUNT steppers_step=$STEPPER_STEP_PINS steppers_en=$STEPPER_EN_PINS

# Steppatron application
if [[ $@ == *"file"* ]]; then
    echo -e "${BLUE}> steppatron file${NC}"
    ./bin/steppatron f $2
elif [[ $@ == *"usb"* ]]; then
    echo -e "${BLUE}> steppatron usb${NC}"
    ./bin/steppatron u
else # Default je tastatura
    echo -e "${BLUE}> steppatron keyboard${NC}"
    ./bin/steppatron k
fi

