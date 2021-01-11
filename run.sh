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
MODULE="gpio_driver"
STEPPER_COUNT=4
STEPPER_STEP_PINS=23,24,25,8
STEPPER_EN_PINS=27,22,10,9

### Colors ###
BLUE='\033[0;36m'
GRAY='\033[1;30m'
NC='\033[0m' # No Color

# Library
if [[ $@ == *"lib"* ]]; then
    echo -e "${BLUE}> sudo apt-get install libasound2-dev${GRAY}"
    sudo apt-get install libasound2-dev || exit
fi

# Makefile
if [[ $@ == *"make"* ]]; then
    echo -e "${BLUE}> make clean${GRAY}"
    make clean
    echo -e "${BLUE}> make${GRAY}"
    make || exit
fi

# Remove old kernel module, if it exists 
echo -e "${BLUE}> sudo rmmod gpio_driver${GRAY}"
sudo rmmod gpio_driver
echo -e "${BLUE}> sudo rm /dev/gpio_driver${GRAY}"
sudo rm /dev/gpio_driver

# Insert newly compiled module (throws error if not compiled)
echo -e "${BLUE}> sudo insmod gpio_driver.ko [with parameters]${GRAY}"
sudo insmod src/gpio_driver.ko steppers_count=$STEPPER_COUNT steppers_step=$STEPPER_STEP_PINS steppers_en=$STEPPER_EN_PINS

# Make new node with right major number
MAJOR_NUMBER=`awk "\\$2==\"$MODULE\" {print \\$1}" /proc/devices` # Jedna veoma lepa linija koda
echo -e "${BLUE}> sudo mknod /dev/gpio_driver c ${MAJOR_NUMBER} 0 ${GRAY}"
sudo mknod /dev/gpio_driver c $MAJOR_NUMBER 0
echo -e "${BLUE}> sudo chmod 666 /dev/gpio_driver${GRAY}"
sudo chmod 666 /dev/gpio_driver

# Steppatron application
if [[ $@ == *"file"* ]]; then
    echo -e "${BLUE}> ./steppatron file${NC}" 
    ./bin/steppatron f $2 || echo -e "${BLUE}> [ERROR] Maybe try to compile first with ./run.sh make ${NC}"
elif [[ $@ == *"usb"* ]]; then
    echo -e "${BLUE}> ./steppatron usb${NC}"
    sudo ./bin/steppatron u $STEPPER_COUNT || echo -e "${BLUE}> [ERROR] Maybe try to compile first with ./run.sh make ${NC}"
else # Default je tastatura
    echo -e "${BLUE}> ./steppatron keyboard${NC}"
    ./bin/steppatron k || echo -e "${BLUE}> [ERROR] Maybe try to compile first with ./run.sh make ${NC}"
fi
