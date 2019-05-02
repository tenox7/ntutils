LIBDIR = \BCOS2\LIB
INCDIR = \BCOS2\INCLUDE

.AUTODEPEND

DEFINES=-DOS2 -DBCPP -DHEAPWALK -DINCL_32

DEBUG =
#DEBUG=-v

INIT = $(LIBDIR)\c02.obj

CC = bcc
LD = tlink
CCFLAGS = $(DEFINES) -L$(LIBDIR) -I$(INCDIR) -H=fte.CSM \
          -k- -sm -K -w -w-par -Ot -RT- -xd- -x- -vi- -d -a -y $(DEBUG)

LDFLAGS = -L$(LIBDIR) $(DEBUG) -s -Toe -Oc -B:0x10000
OEXT=obj

!include objs.inc

.cpp.$(OEXT):
	$(CC) $(CCFLAGS) -c {$< }

.c.$(OEXT):
	$(CC) $(CCFLAGS) -c {$< }

all: cfte.exe fte.exe ftepm.exe clipserv.exe cliputil.exe

defcfg.cnf: defcfg.fte cfte.exe
	cfte defcfg.fte defcfg.cnf

defcfg.h: defcfg.cnf bin2c.exe
	bin2c defcfg.cnf >defcfg.h

c_config.obj: defcfg.h

bin2c.exe: bin2c.cpp
	$(CC) $(CCFLAGS) bin2c.cpp

cfte.exe: $(CFTE_OBJS)
	$(LD) @&&|
        $(LDFLAGS) $(INIT) $**,cfte.exe,cfte.map,os2 c2mt,cfte.def
|

fte.exe: $(OBJS) $(VIOOBJS)
	$(LD) @&&|
        $(LDFLAGS) $(INIT) $**,fte.exe,fte.map,os2 c2mt,fte.def
|

ftepm.exe:: $(OBJS) $(PMOBJS)
	$(LD) @&&|
        $(LDFLAGS) $(INIT) $**,ftepm.exe,ftepm.map,c2mt os2,ftepm.def
|
        rc -i $(INCDIR) ftepm.rc ftepm.exe

clipserv.exe: clipserv.obj
	$(LD) @&&|
        $(LDFLAGS) $(INIT) $**,clipserv.exe,clipserv.map,c2mt os2,clipserv.def
|
        
cliputil.exe: cliputil.obj clip_vio.obj
	$(LD) @&&|
        $(LDFLAGS) $(INIT) $**,cliputil.exe,cliputil.map,c2mt os2,cliputil.def
|

ftepm.exe:: ftepm.res
        rc ftepm.res ftepm.exe

ftepm.res: ftepm.rc pmdlg.rc
        rc -i $(INCDIR) -r ftepm.rc
