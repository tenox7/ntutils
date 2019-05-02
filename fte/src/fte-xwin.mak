# fte-unix.make modified to be generic enough to work with WINNT or UNIX

# versions of FTE to build

# Versions:
#  xfte - using XLib (the most stable)

#  vfte - for Linux console directly (with limitations, see con_linux.cpp)

#WINNT
ESUF=.exe
OEXT=obj
OUTFLAG = -out:
OUTEXEFLAG = /Fe

#UNIX
#ESUF=
#OEXT=o
#OUTFLAG = -o 
#OUTEXEFLAG = -o 
# -o must have a space after it above

.SUFFIXES: .cpp .$(OEXT)

TARGETS = xfte$(ESUF)
#TARGETS = vfte$(ESUF) xfte$(ESUF)

PRIMARY = xfte$(ESUF)

# Comment or uncoment this two flags below if
# you want to use:

# Keyboard remaping by XFTE
#REMAPFLAG = -DUSE_HARD_REMAP

# Drawing fonts with locale support
#XMBFLAG = -DUSE_XMB

# System X11R6 is compiled with X_LOCALE
#SYSTEM_X_LOCALE = -DX_LOCALE

I18NOPTIONS = $(XMBFLAG) $(REMAPFLAG) $(SYSTEM_X_LOCALE)

# Optionally, you can define:
# -DDEFAULT_INTERNAL_CONFIG to use internal config by default
# -DUSE_XTINIT to use XtInitialize on init
APPOPTIONS = -DDEFAULT_INTERNAL_CONFIG -DUSE_XTINIT

#gcc/g++
#COPTIONS = -Wall -Wpointer-arith -Wconversion -Wwrite-strings \
#           -Wmissing-prototypes -Wmissing-declarations -Winline

#CC       = g++
#LD       = g++
# try this for smaller/faster code and less dependencies
#CC       = g++ -fno-rtti -fno-exceptions
#LD       = gcc -fno-rtti -fno-exceptions

# choose your os here

#######################################################################
#MSVC/EXCEED XDK
X_BASE =	C:\win32app\Exceed\xdk

X_LIBS = -LIBPATH:$(X_BASE)\lib \
	HCLXm.lib HCLMrm.lib HCLXmu.lib HCLXt.lib Xlib.lib hclshm.lib xlibcon.lib \

#	Xm.lib Mrm.lib Xmu.lib Xt.lib Xlib.lib hclshm.lib xlibcon.lib

XINCDIR = -I:$(X_BASE)\include

#optimize
#OPTIMIZE  = /Ox -DNDEBUG

#debug
#OPTIMIZE  = /Od /Zi -D_DEBUG
#LDOPTIMIZE = /DEBUG 

CC        = cl
LD        = link

CCFLAGS   = $(OPTIMIZE) -DNT -DMSVC $(INCDIR) -DWIN32 -D_CONSOLE -DWINHCLX \
	-nologo -W3 -J -DNONXP -DWINNT -D_X86_ -MD -Op -W3 -QIfdiv -GX \
	$(XINCDIR)
LDFLAGS   = $(LDOPTIMIZE) $(LIBDIR) -nologo Advapi32.lib User32.lib Wsock32.lib \
	-map /NODEFAULTLIB:libc.lib $(X_LIBS)


#######################################################################
# Linux
#UOS      = -DLINUX
#XLIBDIR  = -L/usr/X11/lib

#######################################################################
# HP/UX
#UOS      = -DHPUX -D_HPUX_SOURCE -DCAST_FD_SET_INT
#UOS      = -DHPUX -D_HPUX_SOURCE

#CC   = CC +a1
#LD   = CC
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
#UOS      = -DAIX -D_BSD_INCLUDES # not recently tested (it did work)

#CC   = xlC
#LD   = xlC

#######################################################################
# Irix
# missing fnmatch, but otherwise ok (tested only on 64bit)
# 6.x has fnmatch now ;-)
# uncomment below to use SGI CC compiler
#UOS      = -DIRIX
#CC   = CC
#LD   = CC
#COPTIONS = -xc++

#######################################################################
# SunOS (Solaris)
#UOS      = -DSUNOS
#CC = CC
#LD = CC
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
#COPTIONS = +.cpp

#######################################################################
# NCR
#CC = cc -Hnocopyr
#LD = cc -Hnocopyr
#COPTIONS = -w3
#UOS = -DNCR
#XINCDIR  = -I../../include
#SOCKETLIB = -lsocket -lnsl -lc -lucb

#######################################################################

#QTDIR   = /users/markom/qt
#QLIBDIR  = -L$(QTDIR)/lib
#QINCDIR  = -I$(QTDIR)/include
#QINCDIR  = -I/usr/include/qt

MOC      = moc

#LIBDIR   = 
#INCDIR   =

#OPTIMIZE = -O -g
#OPTIMIZE = -O2
#OPTIMIZE = -O2 -s

#UNIX
#CCFLAGS  = $(OPTIMIZE) $(I18NOPTIONS) $(COPTIONS) -DUNIX $(UOS) $(INCDIR) $(XINCDIR) $(QINCDIR) $(MINCDIR) $(SINCDIR)
#LDFLAGS  = $(OPTIMIZE) $(LIBDIR) $(XLIBDIR) $(QLIBDIR) $(MLIBDIR)

include objs.inc

#UNIX
# need -lXt below if USE_XTINIT
#XLIBS    = -lX11 -Xt $(SOCKETLIB)
#VLIBS    = -lgpm -lncurses
# -ltermcap outdated by ncurses
#SLIBS    = -lslang
#QLIBS    = -lqt
#MLIBS    = -lXm -lXp -lXt -lXpm -lXext

.cpp.$(OEXT):
	$(CC) $(CCFLAGS) $(APPOPTIONS) -c $<

.c.$(OEXT):
	$(CC) $(CCFLAGS) $(APPOPTIONS) -c $<

.cpp.moc: 
	$(MOC) $< -o $@

all:    cfte$(ESUF) $(TARGETS)
#	rm -f fteln -s $(PRIMARY) fte

cfte$(ESUF): cfte.$(OEXT) s_files.$(OEXT)
	$(LD) $(LDFLAGS) cfte.$(OEXT) s_files.$(OEXT) $(OUTFLAG)cfte$(ESUF) 

c_config.$(OEXT): defcfg.h


#defcfg.h: defcfg.cnf
#	perl mkdefcfg.pl <defcfg.cnf >defcfg.h

defcfg.h: defcfg.cnf bin2c$(ESUF)
	bin2c$(ESUF) defcfg.cnf >defcfg.h

bin2c$(ESUF): bin2c.cpp
	$(CC) $(CCFLAGS) $(OUTEXEFLAG)bin2c$(ESUF) bin2c.cpp

DEFAULT_FTE_CONFIG = simple.fte
#DEFAULT_FTE_CONFIG = defcfg.fte
#DEFAULT_FTE_CONFIG = defcfg2.fte
#DEFAULT_FTE_CONFIG = ../config/main.fte

defcfg.cnf: $(DEFAULT_FTE_CONFIG) cfte$(ESUF)
	cfte$(ESUF) $(DEFAULT_FTE_CONFIG) defcfg.cnf

xfte$(ESUF): $(OBJS) $(XOBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(XOBJS) $(XLIBS) $(OUTFLAG)xfte$(ESUF)

qfte$(ESUF): g_qt.moc g_qt_dlg.moc $(OBJS) $(QOBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(QOBJS) $(QLIBS) $(XLIBS) $(OUTFLAG)qfte$(ESUF)

vfte$(ESUF): $(OBJS) $(VOBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(VOBJS) $(VLIBS) $(OUTFLAG)vfte$(ESUF)

mfte$(ESUF): $(OBJS) $(MOBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(MOBJS) $(MLIBS) $(XLIBS) $(OUTFLAG)mfte$(ESUF)

g_qt.$(OEXT): g_qt.moc

g_qt_dlg.$(OEXT): g_qt_dlg.moc

clean:
	rm -f *.$(OEXT) $(TARGETS) defcfg.h defcfg.cnf cfte$(ESUF) fte$(ESUF)
