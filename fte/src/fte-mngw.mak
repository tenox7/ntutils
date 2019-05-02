#
# FTE makefile for use with the MinGW toolchain
#
# Please note that the common GNU Make 3.77 port for MinGW is broken
# and does not process this makefile correctly . Get a working Make 3.79.1 
# from http://www.nextgeneration.dk/gnu/
#
# The author, Jon Svendsen, explicitly places this module 
# in the Public Domain
#

INCDIR    =
LIBDIR    =

OPTIMIZE  = -O -s

MT        = -Zmt

CC        = g++
LD        = g++

# uncomment this if you don't have fileutils installed
# RM 	  = del 

OEXT=o

DEFS=-DNT -DNTCONSOLE -DMINGW

CCFLAGS   = $(OPTIMIZE) -x c++ -Wall $(DEFS) $(INCDIR) -pipe
LDFLAGS   = $(OPTIMIZE) $(LIBDIR) -Wl,-static 

.SUFFIXES: .cpp .$(OEXT)

include objs.inc

.cpp.$(OEXT):
	$(CC) $(CCFLAGS) -c $<

.c.$(OEXT):
	$(CC) $(CCFLAGS) -c $<

all: cfte.exe fte.exe

cfte.exe: $(CFTE_OBJS)
	$(LD) $(LDFLAGS) $(CFTE_OBJS) -o cfte.exe $(LIBS)

defcfg.cnf: defcfg.fte cfte.exe
	-"./cfte.exe" defcfg.fte defcfg.cnf

defcfg.h: defcfg.cnf bin2c.exe
	-"./bin2c.exe" defcfg.cnf >defcfg.h

bin2c.exe: bin2c.cpp
	$(CC) $(CCFLAGS) bin2c.cpp -o bin2c.exe

c_config.$(OEXT): defcfg.h

fte.exe: $(OBJS) $(NTOBJS)
	-$(LD) $(LDFLAGS) $(OBJS) $(NTOBJS) -o fte.exe $(LIBS)

clean:
	-$(RM) fte.exe cfte.exe bin2c.exe defcfg.cnf defcfg.h $(OBJS) $(NTOBJS) $(CFTE_OBJS)