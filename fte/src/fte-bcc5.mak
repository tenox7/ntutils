#
# FTE makefile for use with Borland C++ 5.5.x/C++Builder 5
#
# The author, Jon Svendsen, explicitly places this module 
# in the Public Domain
#
# Assumes compiler-native includes to be in $(MAKEDIR)\..\Include
# and libraries in $(MAKEDIR)\..\Libs
# $(MAKEDIR) is set by Borland make and is the location of your make.exe
#

INCDIR    = $(MAKEDIR)\..\Include
LIBDIR    = $(MAKEDIR)\..\Lib

OPTIMIZE  = -O2

CC        = bcc32
LD        = ilink32

OEXT      = obj

DEFS      = -DNT -DNTCONSOLE -DBCPP

CCFLAGS   = $(OPTIMIZE) -w-aus -w-par -w-inl -tWC $(DEFS) -I$(INCDIR)
LDFLAGS   = -ap -Tpe -c -x -Gn -L$(LIBDIR) -j$(LIBDIR)
LDOBJS    = c0x32.obj
LDLIBS    = import32.lib cw32.lib

!include objs.inc

.cpp.$(OEXT):
	$(CC) $(CCFLAGS) -c {$< }

.c.$(OEXT):
	$(CC) $(CCFLAGS) -c {$< }

all: cfte.exe fte.exe

defcfg.cnf: defcfg.fte cfte.exe
	cfte defcfg.fte defcfg.cnf

defcfg.h: defcfg.cnf bin2c.exe
	bin2c defcfg.cnf >defcfg.h

c_config.obj: defcfg.h

bin2c.exe: bin2c.obj
	$(LD) $(LDFLAGS) $(LDOBJS) bin2c.obj,$< ,,$(LDLIBS) ,,

cfte.exe: $(CFTE_OBJS)
	$(LD) $(LDFLAGS) $(LDOBJS) $(CFTE_OBJS),$< ,,$(LDLIBS) ,,

fte.exe: $(OBJS) $(NTOBJS)
	$(LD) $(LDFLAGS) $(LDOBJS) $(OBJS) $(NTOBJS),$< ,,$(LDLIBS) ,,

clean:
	del *.$(OEXT)
        del *.tds 
        del fte.exe 
        del cfte.exe 
        del bin2c.exe 
        del defcfg.h 
        del defcfg.cnf 
