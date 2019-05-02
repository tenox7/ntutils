# versions of FTE to build

# Versions:
#  xfte - using XLib (the most stable)

#  vfte - for Linux console directly (with limitations, see con_linux.cpp)

TGT_NFTE = nfte
#TGT_QFTE = qfte
TGT_SFTE = sfte
TGT_VFTE = vfte
TGT_XFTE = xfte

TARGETS = $(TGT_XFTE) $(TGT_VFTE) $(TGT_NFTE) $(TGT_SFTE) $(TGT_QFTE)

# Supply icons in X if CONFIG_X11_XICON is defined, requires Xpm library
XPMLIB = -lXpm

# Need -lXt if CONFIG_X11_XTINIT is defined
#XTLIB = -lXt

#gcc/g++
#CC = g++
#LD = g++
CC = $(CXX)
LD = $(CXX)
CPPOPTIONS = -Wall -Wpointer-arith -Wconversion -Wwrite-strings -Winline

# try this for smaller/faster code and less dependencies
#NOEXCEPTION = -fno-rtti -fno-exceptions


# choose your os here

#######################################################################
# Linux
UOS      = -DLINUX
#XLIBDIR  = 

#######################################################################
# HP/UX
#UOS      = -DHPUX -D_HPUX_SOURCE -DCAST_FD_SET_INT
#UOS      = -DHPUX -D_HPUX_SOURCE

#CC  = CC +a1
#LD  = CC
#CC = aCC
#LD = aCC

#XLIBDIR  = -L/usr/lib/X11R6

#XINCDIR  = -I/usr/include/X11R5
#XLIBDIR  = -L/usr/lib/X11R5

#MINCDIR  = -I/usr/include/Motif1.2
#MLIBDIR  = -L/usr/lib/Motif1.2

SINCDIR   = -I/usr/include/slang

#######################################################################
# AIX
#UOS      = -DAIX -D_BSD_INCLUDES

#CC   = xlC
#LD   = xlC
#CPPOPTIONS = -DNO_NEW_CPP_FEATURES
#TARGETS = xfte

#######################################################################
# Irix
# missing fnmatch, but otherwise ok (tested only on 64bit)
# 6.x has fnmatch now ;-)
# uncomment below to use SGI CC compiler
#UOS  = -DIRIX
#CC   = CC
#LD   = CC
#CPPOPTIONS = -DNO_NEW_CPP_FEATURES -OPT:Olimit=3000 # -xc++

#######################################################################
# SunOS (Solaris)
#UOS      = -DSUNOS
#CC = CC -noex
#LD = CC -noex
#CPPOPTIONS = -DNO_NEW_CPP_FEATURES
#XINCDIR  = -I/usr/openwin/include
#XLIBDIR  = -L/usr/openwin/lib

#######################################################################
# for SCO CC
#
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# If you have problems with the program "cfte"
# try to compile this program without optimalization -O2
# use just -O
#
#UOS = -DSCO
#CC  = CC  -b elf
#LD  = $(CC)
#XLIBDIR  = -L/usr/X11R6/lib
#SOCKETLIB = -lsocket
#CPPOPTIONS = +.cpp

#######################################################################
# NCR
#CC = cc -Hnocopyr
#LD = cc -Hnocopyr
#CPPOPTIONS = -w3
#UOS = -DNCR
#XINCDIR  = -I../../include
#SOCKETLIB = -lsocket -lnsl -lc -lucb

#######################################################################

QTDIR   = /usr/lib64/qt-3.3
#/users/markom/qt
QLIBDIR  = -L$(QTDIR)/lib
#QINCDIR  = -I$(QTDIR)/include
#QINCDIR  = -I/usr/include/qt
QINCDIR =  -I/usr/include/qt3 -I/usr/lib64/qt-3.3/include
MOC      = moc

LIBDIRS   =
INCDIRS   = $(XINCDIR) $(QINCDIR) $(MINCDIR) $(SINCDIR)

OPTIMIZE = -g # -O -g
#OPTIMIZE = -O2
#OPTIMIZE = -Os
#OPTIMIZE = -O2 -s

CCFLAGS  = $(CPPOPTIONS) $(OPTIMIZE) $(NOEXCEPTION) $(INCDIRS) -DUNIX $(UOS)
LDFLAGS  = $(OPTIMIZE) $(LIBDIRS)


.SUFFIXES: .cpp .o .moc

OEXT     = o
include objs.inc

COBJS = cfte.o s_files.o s_string.o

#$(QOBJS:.o=.cpp)

SRCS = $(OBJS:.o=.cpp)\
 $(COBJS:.o=.cpp)\
 $(NOBJS:.o=.cpp)\
 $(SOBJS:.o=.cpp)\
 $(VOBJS:.o=.cpp)\
 $(XOBJS:.o=.cpp)

XLIBS    = $(XLIBDIR) -lX11 $(SOCKETLIB) $(XPMLIB) $(XTLIB)
VLIBS    = $(VLIBDIR) -lgpm
NLIBS    = $(NLIBDIR) -lncurses
SLIBS    = $(SLIBDIR) -lslang
#QLIBS    = $(QLIBDIR) -lqt
QLIBS    = $(QLIBDIR) -lqt-mt
MLIBS    = $(MLIBDIR) -lXm -lXp -lXt -lXpm -lXext

.cpp.o:
	$(CC) -c $< $(CXXFLAGS) $(CPPFLAGS) $(CCFLAGS)

.c.o:
	$(CC) -c $< $(CFLAGS) $(CPPFLAGS) $(CCFLAGS)

.cpp.moc:
	$(MOC) -o $@ $<

all:    cfte $(TARGETS)

cfte:   $(COBJS)
	$(LD) -o $@ $(LDFLAGS) $(COBJS)

c_config.o: defcfg.h

defcfg.h: defcfg.cnf
	perl mkdefcfg.pl <defcfg.cnf >defcfg.h

#DEFAULT_FTE_CONFIG = simple.fte
DEFAULT_FTE_CONFIG = defcfg.fte
#DEFAULT_FTE_CONFIG = defcfg2.fte
#DEFAULT_FTE_CONFIG = ../config/main.fte

defcfg.cnf: $(DEFAULT_FTE_CONFIG) cfte
	./cfte $(DEFAULT_FTE_CONFIG) defcfg.cnf

xfte: .depend $(OBJS) $(XOBJS)
	$(LD) -o $@ $(LDFLAGS) $(OBJS) $(XOBJS) $(XLIBS)

qfte: .depend g_qt.moc g_qt_dlg.moc $(OBJS) $(QOBJS)
	$(LD) -o $@ $(LDFLAGS) $(OBJS) $(QOBJS) $(QLIBS) $(XLIBS)

vfte: .depend $(OBJS) $(VOBJS)
	$(LD) -o $@ $(LDFLAGS) $(OBJS) $(VOBJS) $(VLIBS)

sfte: .depend $(OBJS) $(SOBJS) compkeys
	$(LD) -o $@ $(LDFLAGS) $(OBJS) $(SOBJS) $(SLIBS)

nfte: .depend $(OBJS) $(NOBJS) compkeys
	$(LD) -o $@ $(LDFLAGS) $(OBJS) $(NOBJS) $(NLIBS)

compkeys: .depend compkeys.o
	$(LD) -o $@ $(LDFLAGS) compkeys.o

mfte: .depend $(OBJS) $(MOBJS)
	$(LD) -o $@ $(LDFLAGS) $(OBJS) $(MOBJS) $(MLIBS) $(XLIBS)

g_qt.obj: g_qt.moc

g_qt_dlg.obj: g_qt_dlg.moc

.depend: defcfg.h
	$(CC) -MM -MG $(CCFLAGS) $(SRCS) 1>.depend

# purposefully not part of "all".
tags: $(SRCS) $(wildcard *.h)
	ctags *.h $(SRCS)

.PHONY: clean

clean:
	rm -f core *.o *.moc .depend $(TARGETS) defcfg.h defcfg.cnf \
	cfte fte sfte vfte nfte mfte qfte xfte compkeys tags

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
