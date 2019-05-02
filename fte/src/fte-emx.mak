INCDIR    =
LIBDIR    =

#OPTIMIZE  = -g
OPTIMIZE  = -O -s
#OPTIMIZE   = -O2 -s

MT        = -Zmt

CC        = gcc
LD        = gcc

#XTYPE      =  -Zomf
#XLTYPE     = -Zsys -Zlinker /map -Zlinker /runfromvdm # -Zomf
#OEXT=obj
OEXT=o

#DEFS       = -DDEBUG_EDITOR -DCHECKHEAP
#LIBS      = -lmalloc1
#DEFS      = -DDEBUG_EDITOR -DDBMALLOC -I/src/dbmalloc
#LIBS      = -L/src/dbmalloc -ldbmalloc
LIBS      = -lstdcpp

DEFS=-DINCL_32  #-DUSE_OS2_TOOLKIT_HEADERS

CCFLAGS   = $(OPTIMIZE) $(MT) $(XTYPE) -x c++ -Wall -DOS2 $(DEFS) $(INCDIR) -pipe
LDFLAGS   = $(OPTIMIZE) $(MT) -Zmap $(XLTYPE) $(LIBDIR)

.SUFFIXES: .cpp .$(OEXT)

include objs.inc

.cpp.$(OEXT):
	$(CC) $(CCFLAGS) -c $<

.c.$(OEXT):
	$(CC) $(CCFLAGS) -c $<

all: cfte.exe fte.exe ftepm.exe clipserv.exe cliputil.exe

clipserv.exe: clipserv.$(OEXT) clipserv.def
	$(LD) $(LDFLAGS) clipserv.$(OEXT) clipserv.def -o clipserv.exe $(LIBS)

cliputil.exe: cliputil.$(OEXT) clip_vio.$(OEXT) cliputil.def
	$(LD) $(LDFLAGS) cliputil.$(OEXT) clip_vio.$(OEXT) cliputil.def -o cliputil.exe $(LIBS)

cfte.exe: $(CFTE_OBJS) cfte.def
	$(LD) $(LDFLAGS) $(CFTE_OBJS) cfte.def -o cfte.exe $(LIBS)

defcfg.cnf: defcfg.fte cfte.exe
	cfte defcfg.fte defcfg.cnf

defcfg.h: defcfg.cnf bin2c.exe
	bin2c defcfg.cnf >defcfg.h

bin2c.exe: bin2c.cpp
	$(CC) $(CCFLAGS) bin2c.cpp -o bin2c.exe

c_config.$(OEXT): defcfg.h

fte.exe: $(OBJS) $(VIOOBJS) fte.def
	$(LD) $(LDFLAGS) $(OBJS) $(VIOOBJS) fte.def -o fte.exe $(LIBS)

ftepm.res: ftepm.rc pmdlg.rc bmps/*.bmp
	rc -r -i \emx\include ftepm.rc ftepm.res

ftepm.exe: $(OBJS) $(PMOBJS) ftepm.def ftepm.res
	$(LD) $(LDFLAGS) $(OBJS) $(PMOBJS) ftepm.def ftepm.res -o ftepm.exe $(LIBS)

fte.cnf: cfte.exe
	cfte ..\config\main.fte fte.cnf

#rc -i \emx\include ftepm.rc ftepm.exe

#ftepm.exe:: ftepm.res
#	rc ftepm.res ftepm.exe

distro: ftepm.exe fte.exe fte.cnf cfte.exe clipserv.exe cliputil.exe
	zip ../fte-os2.zip ftepm.exe fte.exe fte.cnf cfte.exe clipserv.exe cliputil.exe
	(cd .. && zip -r fte-config.zip Artistic doc config)
