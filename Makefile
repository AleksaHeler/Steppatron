# all 	-> pwm
#		-> gpio_driver
#		-> steppatron
# clean	-> clean_pwm
# 		-> clean_gpio_driver
# 		-> clean_steppatron
# Do greske dolazi samo kod kompajlovanja kernel modula
# Ima dva koraka, prvo iz .c u .o (mislim da to ne radi kako treba)
# I nakon toga se .o kompajluje u kernel modul
# Uglavnom je sve za driver kopirano sa vezbi 6 iz sppurva (gpio_driver)
# dole je oznaceno gde mislim da dolazi do gresaka

######################################################
###                   VARIABLES                    ###
######################################################

# Target vars
TPWM = bin/pwm 						# $(TPWM)
TDRIVER = bin/gpio_driver 			# $(TDRIVER)
TSTEPPATRON = bin/steppatron 		# $(TSTEPPATRON)
# Object vars
OPWM = obj/pwm.o 					# $(OPWM)
ODRIVER = obj/gpio_driver.o 		# $(ODRIVER)
OSTEPPATRON = obj/steppatron.o 		# $(OSTEPPATRON)
# C vars
CPWM = src/pwm.c					# $(CPWM)
CDRIVER = src/gpio_driver.c 		# $(CDRIVER)
CSTEPPATRON = src/steppatron.c 		# $(CSTEPPATRON)

TARGET := $(TDRIVER)				# Kernel targets
obj-m := $(ODRIVER)
CURRENT := $(shell uname -r)
KDIR := /lib/modules/$(CURRENT)/build
PWD := $(shell pwd)
DEST := /lib/modules/$(CURRENT)/kernel/arch/arm/gpio_driver
WARN    := -W -Wall -Wstrict-prototypes -Wmissing-prototypes
INCLUDE := -isystem /lib/modules/`uname -r`/build/include
CFLAGS  := -O2 -DMODULE -c ${WARN} ${INCLUDE} #-D__KERNEL__

CC	 = gcc
MKDIR_P = mkdir -p
FLAGS	 = -g -c -Wall
LFLAGS	 = -lpthread -lasound -lwiringPi #-static

######################################################
###                      MAKE                      ###
######################################################
all: directories pwm gpio_driver steppatron

directories:
	${MKDIR_P} obj
	${MKDIR_P} bin
pwm: $(OPWM)
	$(CC) -g $(OPWM) -o $(TPWM) $(LFLAGS)
gpio_driver: $(ODRIVER)											### Ovo pogledati
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
steppatron: $(OSTEPPATRON)
	$(CC) -g $(OSTEPPATRON) -o $(TSTEPPATRON) $(LFLAGS)

######################################################
###                       .o                       ###
######################################################
$(OPWM): $(CPWM)
	$(CC) $(FLAGS) $(CPWM) -o $(OPWM)
$(ODRIVER):	$(CDRIVER)												### Ovo pogledati
	$(CC) $(CFLAGS) $(FLAGS) $(CDRIVER) -o $(ODRIVER)
$(OSTEPPATRON): $(CSTEPPATRON)
	$(CC) $(FLAGS) $(CSTEPPATRON) -o $(OSTEPPATRON)

######################################################
###                    DRIVER                      ###
######################################################					### Ovo sve pogledati
install:
	su -c "cp gpio_driver.ko $(DEST) && /sbin/depmod -a"

revert:
	@echo "Reverting to the original .ko."
	@mv -v $(DEST)/gpio_driver.ko.orig $(DEST)/gpio_driver.ko

-include $(KDIR)/Rules.make

######################################################
###                     CLEAN                      ###
######################################################
clean: clean_pwm clean_gpio_driver clean_steppatron
clean_pwm:
	rm -f $(OPWM) $(TPWM)
clean_gpio_driver:
	rm -f *.o gpio_driver.ko .*.cmd .*.flags *.mod.c
clean_steppatron:
	rm -f $(OSTEPPATRON) $(TSTEPPATRON)