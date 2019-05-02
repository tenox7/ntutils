INCDIR    =
LIBDIR    =

OPTIMIZE  = /O2 /MT
#OPTIMIZE  = /Zi /Od /MTd

#DEBUG     = -DMSVCDEBUG

CC        = cl
LD        = cl
#RC	  = rc

OEXT=obj

APPOPTIONS = -DDEFAULT_INTERNAL_CONFIG
#/Fm /GF /J

# Use these settings for MSVC 6
#CCFLAGS   = $(OPTIMIZE) -DNT -DNTCONSOLE -DMSVC -DWIN32 -D_CONSOLE -DNO_NEW_CPP_FEATURES $(INCDIR) /GX \
#	$(APPOPTIONS) $(DEBUG) \
#	/nologo /W3 /J # /YX

# Use these settings for MSVC 2003
#CCFLAGS   = $(OPTIMIZE) -DNT -DNTCONSOLE -DMSVC -DWIN32 -D_CONSOLE $(INCDIR) /GX \
#	$(APPOPTIONS) $(DEBUG) \
#	/nologo /W3 /J # /YX
# Use these settings for Visual C Express 2008
MOREDEFS = -D_CRT_SECURE_NO_DEPRECATE=1 -D_CRT_NONSTDC_NO_DEPRECATE=1
CCFLAGS   = $(OPTIMIZE) -DNT -DNTCONSOLE -DMSVC -DWIN32 -D_CONSOLE $(MOREDEFS) $(INCDIR) /EHsc \
	$(APPOPTIONS) $(DEBUG) /nologo /W3 /J 

LDFLAGS   = $(OPTIMIZE) $(LIBDIR) /nologo
RCFLAGS   =

.SUFFIXES: .cpp .$(OEXT)

!include objs.inc

#NTRES     = ftewin32.res

.cpp.$(OEXT):
	$(CC) $(CCFLAGS) -c $<

.c.$(OEXT):
	$(CC) $(CCFLAGS) -c $<

all: cfte.exe fte.cnf fte.exe

clean:
	-del bin2c.exe
	-del bin2c.pdb
	-del cfte.exe
	-del cfte.pdb
	-del cfte.exp
	-del cfte.lib
	-del defcfg.cnf
	-del defcfg.h
	-del fte.cnf
	-del fte.exe
	-del fte.his
	-del fte.pdb
	-del vc60.pdb
	-del ftewin32.res
	-del *.obj

cfte.exe: $(CFTE_OBJS) cfte.def
	$(LD) $(LDFLAGS) /Fecfte.exe $(CFTE_OBJS) cfte.def

defcfg.cnf: defcfg.fte cfte.exe
	cfte defcfg.fte defcfg.cnf

defcfg.h: defcfg.cnf bin2c.exe
	bin2c defcfg.cnf >defcfg.h

fte.cnf: ..\config\* cfte.exe
	cfte ..\config\main.fte fte.cnf

bin2c.exe: bin2c.cpp
	$(CC) $(CCFLAGS) $(LDFLAGS) /Febin2c.exe bin2c.cpp

c_config.$(OEXT): defcfg.h

#ftewin32.res: ftewin32.rc
#	$(RC) $(RCFLAGS) ftewin32.rc

fte.exe: $(OBJS) $(NTOBJS) $(NTRES)
	$(LD) $(LDFLAGS) /Fefte.exe $(OBJS) $(NTOBJS) user32.lib $(NTRES)

distro: fte.exe fte.cnf cfte.exe
	zip ../fte-nt.zip fte.exe fte.cnf cfte.exe

