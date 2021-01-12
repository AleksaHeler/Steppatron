# all 	-> pwm
#		-> gpio_driver
#		-> steppatron
# clean	-> clean_pwm
# 		-> clean_gpio_driver
# 		-> clean_steppatron

######################################################
###                   VARIABLES                    ###
######################################################

# Target vars
TPWM := bin/pwm
TDRIVER := bin/gpio_driver.ko
TSTEPPATRON := bin/steppatron
# Object vars
OPWM := obj/pwm.o
ODRIVER := obj/gpio_driver.o
OSTEPPATRON := obj/steppatron.o
ORAWMIDI := obj/rawMidi.o
OPARSER := obj/midiParser.o
# C vars
CPWM := src/pwm.c
CDRIVER := src/gpio_driver.c
CSTEPPATRON := src/steppatron.c
CRAWMIDI := src/rawMidi.c
CPARSER := src/midiParser.c

TARGET := gpio_driver.ko
obj-m := src/gpio_driver.o
HEADER	= getch.h midi.h midiParser.h rawMidi.h
MDIR := arch/arm/gpio_driver
CURRENT := $(shell uname -r)
KDIR := /lib/modules/$(CURRENT)/build
PWD := $(shell pwd)
DEST := /lib/modules/$(CURRENT)/kernel/$(MDIR)

CC = gcc
MKDIR_P := mkdir -p
FLAGS := -g -c -Wall
LFLAGS := -lpthread -lasound -lwiringPi
WARN := -W -Wall -Wstrict-prototypes -Wmissing-prototypes
INCLUDE := -isystem /lib/modules/`uname -r`/build/include

######################################################
###                      MAKE                      ### make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
######################################################
all: directories pwm gpio_driver steppatron

directories:
	${MKDIR_P} obj
	${MKDIR_P} bin
pwm: $(OPWM)
	$(CC) -g $(OPWM) -o $(TPWM) $(LFLAGS)
gpio_driver:
	$(MAKE) -I $(KDIR)/arch/arm/include/asm/ -C $(KDIR) M=$(PWD)
steppatron: $(OPARSER) $(ORAWMIDI) $(OSTEPPATRON)
	$(CC) -g $(OSTEPPATRON) $(OPARSER) $(ORAWMIDI) -o $(TSTEPPATRON) $(LFLAGS)

######################################################
###                       .o                       ###
###################################################### 
$(OPWM): $(CPWM)
	$(CC) $(FLAGS) $(CPWM) -o $(OPWM)
$(OSTEPPATRON): $(CSTEPPATRON)
	$(CC) $(FLAGS) $(CSTEPPATRON) -o $(OSTEPPATRON)
$(OPARSER): $(CPARSER)
	$(CC) $(FLAGS) $(CPARSER) -o $(OPARSER)
$(ORAWMIDI): $(CRAWMIDI)
	$(CC) $(FLAGS) $(CRAWMIDI) -o $(ORAWMIDI)

######################################################
###                    DRIVER                      ###
######################################################
install:
	#@if test -f $(DEST)/$(TARGET).orig; then \
	#       echo "Backup of .ko already exists."; \
	#else \
	#       echo "Creating a backup of .ko."; \
	#       mv -v $(DEST)/$(TARGET) $(DEST)/$(TARGET).orig; \
	#fi
	su -c "cp $(TARGET) $(DEST) && /sbin/depmod -a"

revert:
	@echo "Reverting to the original .ko."
	@mv -v $(DEST)/$(TARGET).orig $(DEST)/$(TARGET)

-include $(KDIR)/Rules.make

######################################################
###                     CLEAN                      ###
######################################################
clean: clean_pwm clean_gpio_driver clean_steppatron
clean_pwm:
	rm -f $(OPWM) $(TPWM)
clean_gpio_driver:
	rm -f src/*.o src/$(TARGET) src/.*.cmd src/.*.flags src/*.mod.c src/*.mod
clean_steppatron:
	rm -f $(OSTEPPATRON) $(OPARSER) $(ORAWMIDI) $(TSTEPPATRON)