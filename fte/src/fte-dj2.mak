# MSDOS makefile for djgpp v2 and GNU make
# Contributed by Markus F.X.J. Oberhumer <markus.oberhumer@jk.uni-linz.ac.at>

CC        = gxx
LD        = gxx

INCDIR    =
LIBDIR    =

#OPTIMIZE  = -g
OPTIMIZE  = -O2 -fno-strength-reduce -fomit-frame-pointer -s

CCFLAGS   = -x c++ -fconserve-space $(OPTIMIZE) $(INCDIR)
CCFLAGS  += -Wall -W -Wsynth -Wno-unused
CCFLAGS  += -DDOSP32 -D__32BIT__
LDFLAGS   = $(OPTIMIZE) $(LIBDIR)

OEXT=o

.SUFFIXES: .cpp .$(OEXT)

include objs.inc
CFTE_OBJS += port.$(OEXT)
CFTE_OBJS += memicmp.$(OEXT)

.cpp.$(OEXT):
	$(CC) $(CCFLAGS) -c $<

.c.$(OEXT):
	$(CC) $(CCFLAGS) -c $<

.PHONY: default all clean

default all: cfte.exe fte.exe

clean:
	-$(RM) *.o bin2c.exe cfte.exe fte.exe defcfg.cnf defcfg.h

cfte.exe: $(CFTE_OBJS)
	$(LD) $(LDFLAGS) $(CFTE_OBJS) -o $@

defcfg.cnf: defcfg.fte cfte.exe
	cfte defcfg.fte defcfg.cnf

defcfg.h: defcfg.cnf bin2c.exe
	bin2c defcfg.cnf >defcfg.h

bin2c.exe: bin2c.cpp
	$(CC) $(CCFLAGS) $< -o $@

c_config.$(OEXT): defcfg.h

fte.exe: $(OBJS) $(DOSP32OBJS)
	$(LD) $(LDFLAGS) $^ -o $@
