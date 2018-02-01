# ==========================================================================
# File: Makefile
# Desc: Makefile for shirt
# Auth: Rune M. Jensen
# Date: 19/7/04
# ==========================================================================

CFLAGS =  -W -Wtraditional -Wmissing-prototypes -Wall 

LIBDIR1 = ../../buddy20/src
INCDIR1 = ../../buddy20/src

LIBDIR2 = ../../src
INCDIR2 = ../../src


# --- full object list
OBJ =     shirt.o 

CCFILES = shirt.cc

# --------------------------------------------------------------
# Code generation
# --------------------------------------------------------------

.SUFFIXES: .cc 

.cc.o:
	g++ -I$(INCDIR1) -I$(INCDIR2)  -g  -c   $<


# --------------------------------------------------------------
# The primary targets.
# --------------------------------------------------------------
#
# OBS: the -lbdd argument MUST be placed last or you get linker errors
# to the BuDDy library

shirt:	$(OBJ)
	g++  -g $(CFLAGS) -o shirt $(OBJ)  -L$(LIBDIR1) -L$(LIBDIR2)  -lclab -lfl -lm -lbdd -fopenmp
	chmod u+x shirt


clean:
	rm -f *.o core *~
	rm -f shirt
