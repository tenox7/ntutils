# Copyright (c) 1990,1991,1992 Chris and John Downey
#***
#
# @(#)makefile.sv	2.2 (Chris & John Downey) 7/31/92
#
# program name:
#	xvi
# function:
#	PD version of UNIX "vi" editor, with extensions.
# module name:
#	makefile.sv
# module function:
#	Makefile for SunView version
# history:
#	STEVIE - ST Editor for VI Enthusiasts, Version 3.10
#	Originally by Tim Thompson (twitch!tjt)
#	Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
#	Heavily modified by Chris & John Downey
#***

SYSDEFS=	-DUNIX -DBSD -DSUNVIEW
INCDIRS=

FRONTLIB=	-lsuntool -lsunwindow -lpixrect
BACKLIB=
LDFLAGS=

CFLAGS=		$(SYSDEFS) $(INCDIRS) -O2
LINTFLAGS=	$(SYSDEFS) $(INCDIRS) -ah

MACHINC=	unix.h sunview.h

GENINC=		ascii.h param.h ptrfunc.h regexp.h regmagic.h xvi.h virtscr.h

GENOBJ=		alloc.o ascii.o buffers.o cmdline.o cursor.o \
		edit.o ex_cmds1.o ex_cmds2.o events.o fileio.o \
		find.o flexbuf.o map.o mark.o misccmds.o movement.o \
		normal.o param.o pipe.o preserve.o ptrfunc.o \
		regexp.o screen.o search.o signal.o startup.o status.o \
		tags.o undo.o version.o windows.o yankput.o

GENSRC=		$(GENOBJ:.o=.c)

BACKOBJ=	$(GENOBJ) unix.o defscr.o sunback.o mouse.o
BACKSRC=	$(BACKOBJ:.o=.c)

FRONTOBJ=	sunfront.o
FRONTSRC=	sunfront.c

SRC=		$(FRONTSRC) $(BACKSRC)
OBJ=		$(FRONTOBJ) $(BACKOBJ)

FRONT=		xvi.sunview
BACK=		xvi.main

ALL=		$(FRONT) $(BACK)

all:		$(ALL)

$(FRONT):	$(FRONTOBJ)
		$(CC) $(CFLAGS) -o $@ $(FRONTOBJ) $(FRONTLIB)

$(BACK):	$(BACKOBJ)
		$(CC) $(CFLAGS) -o $@ $(BACKOBJ) $(BACKLIB)

$(FRONTOBJ):	$(FRONTSRC) $(GENINC) $(MACHINC)
		$(CC) $(CFLAGS) -DXVI_MAINPROG=\"$(BACK)\" \
		-c -o $@ $(FRONTSRC)

.c.o:		$< $(GENINC) $(MACHINC)
		$(CC) $(CFLAGS) -o $@ -c $<

sources:
		sccs check || sccs delget `sccs tell`

lint:
		lint $(LINTFLAGS) $(SRC)

tags:		$(SRC)
		ctags -t $(SRC) $(GENINC) $(MACHINC)

clean:
		csh -c 'rm -f *.{o,obj} &'

install:
		mv $(ALL) $(HOME)/bin/`arch`

$(OBJ):		$(GENINC) $(MACHINC)
