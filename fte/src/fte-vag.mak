#
#
#  Makefile for VisualAge C++ version 3.00
#
#

EXE =

OEXT = obj

DEBUG = 0
#DEBUG = 1

CC = icc
LINK = ilink
RC = rc
VOID = echo > NUL

!if $(DEBUG)
C_FLAGS = /Ti /Tx
!else
C_FLAGS = /O /Gs-
!endif
C_OPTIONS = /Q /Tl /G4 /Gm+ /DOS2 /DINCL_32 $(C_FLAGS)

!if $(DEBUG)
CPP_FLAGS = /Ti /Tm /Tx
!else
CPP_FLAGS = /O /Gs-
!endif
CPP_OPTIONS = /Q /G4 /Gm+ /DOS2 /DINCL_32 $(CPP_FLAGS)

!if $(DEBUG)
L_FLAGS = /DEBUG /DBGPACK
!else
L_FLAGS = /EXEPACK:2 /PACKC /PACKD /OPTF
!endif
L_OPTIONS = /BASE:0x010000 /EXEC /NOE /NOLOGO $(L_FLAGS)

RC_OPT = -n

C_SRC =
C_H =
CPP_SRC	= 
CPP_HPP =

!include objs.inc

RES = 

LIBS = 
HLIB =

DEF = 


.SUFFIXES:
.SUFFIXES: .cpp .rc

all: cfte.exe fte.exe ftepm.exe clipserv.exe cliputil.exe fte.cnf

clean:
	-del bin2c.exe
	-del bin2c.map
	-del cfte.exe
	-del cfte.map
	-del clipserv.exe
	-del clipserv.map
	-del cliputil.exe
	-del cliputil.map
	-del defcfg.cnf
	-del defcfg.h
	-del fte.cnf
	-del fte.exe
	-del fte.his
	-del fte.map
	-del ftepm.exe
	-del ftepm.map
	-del ftepm.res
	-del *.obj

clipserv.exe: clipserv.$(OEXT) clipserv.def
	$(VOID) <<clipserv.lnk
clipserv.$(OEXT)  $(L_OPTIONS)
/OUT:$@ /MAP:clipserv.MAP
$(LIBS)
clipserv.def
<< 
	$(LINK) @clipserv.lnk

cliputil.exe: cliputil.$(OEXT) clip_vio.$(OEXT) cliputil.def
	$(VOID) <<cliputil.lnk
cliputil.$(OEXT) clip_vio.$(OEXT) $(L_OPTIONS)
/OUT:$@ /MAP:cliputil.MAP
$(LIBS)
cliputil.def
<< 
	$(LINK) @cliputil.lnk

cfte.exe: $(CFTE_OBJS) cfte.def
 	$(VOID) <<cfte.lnk
$(CFTE_OBJS) $(L_OPTIONS)
/OUT:$@ /MAP:cfte.MAP
$(LIBS)
cfte.def
<< 
	$(LINK) @cfte.lnk

defcfg.cnf: defcfg.fte cfte.exe
	cfte defcfg.fte defcfg.cnf

defcfg.h: defcfg.cnf bin2c.exe
	bin2c defcfg.cnf >defcfg.h

fte.cnf: ..\config\* cfte.exe
	cfte ..\config\main.fte fte.cnf

bin2c.obj: bin2c.cpp

bin2c.exe: bin2c.obj
 	$(VOID) <<bin2c.lnk
bin2c.obj $(L_OPTIONS) /PM:VIO
/OUT:$@ /MAP:bin2c.MAP
$(LIBS)
$(DEF)
<< 
	$(LINK) @bin2c.lnk

c_config.$(OEXT): defcfg.h

fte.exe: $(OBJS) $(VIOOBJS) fte.def
 	$(VOID) <<fte.lnk
$(OBJS) $(VIOOBJS) $(L_OPTIONS)
/OUT:$@ /MAP:fte.MAP
$(LIBS)
fte.def
<< 
	$(LINK) @fte.lnk

ftepm.res: ftepm.rc pmdlg.rc

ftepm.exe: $(OBJS) $(PMOBJS) ftepm.def ftepm.res
 	$(VOID) <<ftepm.lnk
$(OBJS) $(PMOBJS) $(L_OPTIONS)
/OUT:$@ /MAP:ftepm.MAP
$(LIBS)
ftepm.def
<< 
	$(LINK) @ftepm.lnk
	$(RC) $(RC_OPT) ftepm.res ftepm.EXE

$(EXE).EXE: $(OBJS) $(C_SRC:.c=.obj) $(CPP_SRC:.cpp=.obj) $(RES) $(DEF) $(LIBS)
 	$(VOID) <<$(EXE).lnk
$(OBJS) $(C_SRC:.c=.obj) $(CPP_SRC:.cpp=.obj) $(L_OPTIONS)
/OUT:$@ /MAP:$(EXE).MAP
$(LIBS)
$(DEF)
<< 
	$(LINK) @$(EXE).lnk
	$(RC) $(RC_OPT) $(RES) $(EXE).EXE


# $(C_SRC:.c=.obj): $(C_SRC) $(C_H)

# $(CPP_SRC:.cpp=.obj): $(CPP_SRC) $(CPP_HPP)

# $(RES): $(RES:.res=.rc)

.C.$(OEXT):
	$(CC) /C $(C_OPTIONS) $<
    
.CPP.$(OEXT):
	$(CC) /C $(CPP_OPTIONS) $<

.RC.RES:
	$(RC) -r $(RC_OPT) $<
