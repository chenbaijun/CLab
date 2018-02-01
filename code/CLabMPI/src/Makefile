# /*========================================================================
#                Copyright (C) 2004 by Rune M. Jensen
#                             All rights reserved
# 
#     Permission is hereby granted, without written agreement and without
#     license or royalty fees, to use, reproduce, prepare derivative
#     works, distribute, and display this software and its documentation
#     for NONCOMMERCIAL RESEARCH AND EDUCATIONAL PURPOSES, provided 
#     that (1) the above copyright notice and the following two paragraphs 
#     appear in all copies of the source code and (2) redistributions, 
#     including without limitation binaries, reproduce these notices in 
#     the supporting documentation. 
# 
#     IN NO EVENT SHALL RUNE M. JENSEN, OR DISTRIBUTORS OF THIS SOFTWARE 
#     BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, 
#     OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE 
#     AND ITS DOCUMENTATION, EVEN IF THE AUTHORS OR ANY OF THE ABOVE 
#     PARTIES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
#     RUNE M. JENSEN SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
#     BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
#     FITNESS FOR A PARTICULAR PURPOSE. THE SOFTWARE PROVIDED HEREUNDER IS
#     ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO
#     OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
#     MODIFICATIONS.
# ========================================================================*/

# ==========================================================================
# File: Makefile
# Desc: Makefile for the CLab Library
# Auth: Rune M. Jensen
# Date: 7/19/04, 12/10/11
# ==========================================================================

CFLAGS =  -W -Wtraditional -Wmissing-prototypes -Wall 

LIBDIR = ../buddy20/src
INCDIR = ../buddy20/src


# --- full object list
OBJ =     lex.yy.o y.tab.o common.o cp.o set.o layout.o \
          space.o symbols.o dump.o clab.o  

CFILES =  lex.yy.c y.tab.c 

CCFILES = common.cc cp.cc set.cc layout.cc space.cc \
          symbols.cc dump.cc clab.cc 

# --------------------------------------------------------------
# Code generation
# --------------------------------------------------------------

.SUFFIXES: .cc .c

.cc.o:
	g++ -I$(INCDIR)   -g  -c  $<

.c.o:
	g++ -I$(INCDIR)   -g  -c -x c++  $<



# --------------------------------------------------------------
# The primary targets.
# --------------------------------------------------------------
# 

libclab.a: $(OBJ) 
	ar r libclab.a $(OBJ) 
	ranlib libclab.a


y.tab.c y.tab.h: cp.y  
	yacc -d -v cp.y

lex.yy.c:    cp.l 
	flex cp.l

clean:
	rm -f libclab.a
	rm -f *.o core *~
	rm -f y.output
# If re-lexing and yaccing CLab you have to run
# yacc -d -v cp.y from the command prompt in order to make
# y.tab.h with out breaking dependency relations 
#	rm y.tab.c
#	rm lex.yy.c
#	rm y.tab.h

depend:
	g++ -x c++ -I$(INCDIR)  -MM $(CFILES) > depend.inf
	g++ -I$(INCDIR) -MM $(CCFILES) >> depend.inf

###
include depend.inf

