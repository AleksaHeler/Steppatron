WORKDIR = `pwd`

CC = gcc
CXX = gcc
LD = gcc

INC = -I inc
CFLAGS = -Wall
LIBDIR =
LIB = -lpthread -lasound
LDFLAGS = #-static

SRC = src


#----------------------------------------------------------------------
#------------------- Makefile Release configuration -------------------
#----------------------------------------------------------------------
INC_RELEASE = $(INC)
CFLAGS_RELEASE = $(CFLAGS) -O2
RESINC_RELEASE = $(RESINC)
RCFLAGS_RELEASE = $(RCFLAGS)
LIBDIR_RELEASE = $(LIBDIR)
LIB_RELEASE = $(LIB)
LDFLAGS_RELEASE = $(LDFLAGS)
OBJDIR_RELEASE = obj
DEP_RELEASE =
OUT_RELEASE = bin/steppatron.out

OBJ_RELEASE = $(OBJDIR_RELEASE)/steppatron.o


#----------------------------------------------------------------------
#------------------------------- Targets ------------------------------
#----------------------------------------------------------------------

all: release

clean: clean_release

#------------------------------------------------------------------
#------------------------- BUILD RELEASE --------------------------
#------------------------------------------------------------------

release: before_release out_release after_release

before_release:
	test -d bin || mkdir -p bin
	test -d $(OBJDIR_RELEASE) || mkdir -p $(OBJDIR_RELEASE)

out_release: before_release $(OBJ_RELEASE) $(DEP_RELEASE)
	$(LD) $(LIBDIR_RELEASE) -o $(OUT_RELEASE) $(OBJ_RELEASE)  $(LDFLAGS_RELEASE) $(LIB_RELEASE)

$(OBJDIR_RELEASE)/steppatron.o: $(SRC)/steppatron.c
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c $(SRC)/steppatron.c -o $(OBJDIR_RELEASE)/steppatron.o

after_release:

clean_release:
	rm -f $(OBJ_RELEASE) $(OUT_RELEASE)
	rm -rf bin
	rm -rf $(OBJDIR_RELEASE)

.PHONY: before_release after_release clean_release

