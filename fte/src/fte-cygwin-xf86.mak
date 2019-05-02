# versions of FTE to build

# Versions:
#  xfte - using XLib (the most stable)

TARGETS = xfte

PRIMARY = xfte

# Comment or uncoment this two flags below if
# you want to use:

# Keyboard remaping by XFTE
#REMAPFLAG = -DUSE_HARD_REMAP

# Drawing fonts with locale support
XMBFLAG = -DUSE_XMB

# System X11R6 is compiled with X_LOCALE
#SYSTEM_X_LOCALE = -DX_LOCALE

I18NOPTIONS = $(XMBFLAG) $(REMAPFLAG) $(SYSTEM_X_LOCALE)

# Optionally, you can define:
# -DDEFAULT_INTERNAL_CONFIG to use internal config by default
# -DUSE_XTINIT to use XtInitialize on init
# -DFTE_NO_LOGGING to completely disable trace logging
APPOPTIONS = -DDEFAULT_INTERNAL_CONFIG

#gcc/g++
COPTIONS = -Wall -Wpointer-arith -Wconversion -Wwrite-strings \
           -Wmissing-prototypes -Wmissing-declarations -Winline

#CC       = g++
#LD       = g++
# try this for smaller/faster code and less dependencies
CC       = g++ -fno-rtti -fno-exceptions
LD       = g++ -fno-rtti -fno-exceptions


#######################################################################
# Linux
UOS      = -DLINUX
XINCDIR  = -I/usr/X11R6/include
XLIBDIR  = -L/usr/X11R6/lib -lstdc++
SOCKETLIB = -lwsock32 -liberty

#######################################################################

LIBDIR   = 
INCDIR   =

#OPTIMIZE = -g # -O -g
OPTIMIZE = -O2 
#OPTIMIZE = -O2 -s

CCFLAGS  = $(OPTIMIZE) $(I18NOPTIONS) $(APPOPTIONS) $(COPTIONS) -DUNIX $(UOS) \
           $(INCDIR) $(XINCDIR)
LDFLAGS  = $(OPTIMIZE) $(LIBDIR) $(XLIBDIR)

OEXT     = o

.SUFFIXES: .cpp .o

include objs.inc

# Need -lXt below if USE_XTINIT is defined
XLIBS    = -lX11 $(SOCKETLIB)

.cpp.o:
	$(CC) $(CCFLAGS) -c $<

.c.o:
	$(CC) $(CCFLAGS) -c $<


all:    cfte $(TARGETS)
#rm -f fte ; ln -s $(PRIMARY) fte

cfte: cfte.o s_files.o
	$(LD) $(LDFLAGS) cfte.o s_files.o -o cfte 

c_config.o: defcfg.h

defcfg.h: defcfg.cnf
	perl mkdefcfg.pl <defcfg.cnf >defcfg.h

#DEFAULT_FTE_CONFIG = simple.fte
DEFAULT_FTE_CONFIG = defcfg.fte
#DEFAULT_FTE_CONFIG = defcfg2.fte
#DEFAULT_FTE_CONFIG = ../config/main.fte

defcfg.cnf: $(DEFAULT_FTE_CONFIG) cfte
	./cfte $(DEFAULT_FTE_CONFIG) defcfg.cnf

xfte: $(OBJS) $(XOBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(XOBJS) $(XLIBS) -o xfte

compkeys: compkeys.o 
	$(LD) $(LDFLAGS) compkeys.o -o compkeys

clean:
	rm -f core *.o $(TARGETS) defcfg.h defcfg.cnf cfte fte vfte compkeys
