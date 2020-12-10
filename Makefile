WORKDIR = `pwd`

CC = gcc
CXX = gcc
LD = gcc

INC = -I inc
CFLAGS = -Wall
LIBDIR =
LIB = -lpthread
LDFLAGS = -static

SRC = src

#----------------------------------------------------------------------
#-------------------- Makefile Debug configuration --------------------
#----------------------------------------------------------------------
INC_DEBUG = $(INC)
CFLAGS_DEBUG = $(CFLAGS) -g
RESINC_DEBUG = $(RESINC)
RCFLAGS_DEBUG = $(RCFLAGS)
LIBDIR_DEBUG = $(LIBDIR)
LIB_DEBUG = $(LIB)
LDFLAGS_DEBUG = $(LDFLAGS)
OBJDIR_DEBUG = obj/Debug
DEP_DEBUG =
OUT_DEBUG = bin/Debug/steppatron

OBJ_DEBUG = $(OBJDIR_DEBUG)/steppatron.o

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
OBJDIR_RELEASE = obj/Release
DEP_RELEASE =
OUT_RELEASE = bin/Release/steppatron

OBJ_RELEASE = $(OBJDIR_RELEASE)/steppatron.o


#----------------------------------------------------------------------
#------------------------------- Targets ------------------------------
#----------------------------------------------------------------------

all: debug release

clean: clean_debug clean_release


#------------------------------------------------------------------
#-------------------------- BUILD DEBUG ---------------------------
#------------------------------------------------------------------

debug: before_debug out_debug after_debug

before_debug:
	test -d bin/Debug || mkdir -p bin/Debug
	test -d $(OBJDIR_DEBUG) || mkdir -p $(OBJDIR_DEBUG)

out_debug: $(OBJ_DEBUG) $(DEP_DEBUG)
	$(LD) $(LIBDIR_DEBUG) -o $(OUT_DEBUG) $(OBJ_DEBUG)  $(LDFLAGS_DEBUG) $(LIB_DEBUG)

$(OBJDIR_DEBUG)/steppatron.o: $(SRC)/steppatron.c
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c $(SRC)/steppatron.c -o $(OBJDIR_DEBUG)/steppatron.o

after_debug:

clean_debug:
	rm -f $(OBJ_DEBUG) $(OUT_DEBUG)
	rm -rf bin/Debug
	rm -rf $(OBJDIR_DEBUG)

#------------------------------------------------------------------
#------------------------- BUILD RELEASE --------------------------
#------------------------------------------------------------------

release: before_release out_release after_release

before_release:
	test -d bin/Release || mkdir -p bin/Release
	test -d $(OBJDIR_RELEASE) || mkdir -p $(OBJDIR_RELEASE)

out_release: before_release $(OBJ_RELEASE) $(DEP_RELEASE)
	$(LD) $(LIBDIR_RELEASE) -o $(OUT_RELEASE) $(OBJ_RELEASE)  $(LDFLAGS_RELEASE) $(LIB_RELEASE)

$(OBJDIR_RELEASE)/steppatron.o: $(SRC)/steppatron.c
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c $(SRC)/steppatron.c -o $(OBJDIR_RELEASE)/steppatron.o

after_release:

clean_release:
	rm -f $(OBJ_RELEASE) $(OUT_RELEASE)
	rm -rf bin/Release
	rm -rf $(OBJDIR_RELEASE)

.PHONY: before_debug after_debug clean_debug before_release after_release clean_release

