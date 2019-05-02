# Copyright (c) 1990,1991,1992 Chris and John Downey
#***
#
# @(#)makefile.qc	2.2 (Chris & John Downey) 7/31/92
#
# program name:
#	xvi
# function:
#	PD version of UNIX "vi" editor, with extensions.
# module name:
#	makefile.qc
# module function:
#	Makefile for MS-DOS, using Microsoft Quick C (standalone
#	version) & Microsoft's make utility.
#
#	Microsoft Macro Assembler (or a compatible assembler) is also
#	required.
# history:
#	STEVIE - ST Editor for VI Enthusiasts, Version 3.10
#	Originally by Tim Thompson (twitch!tjt)
#	Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
#	Heavily modified by Chris & John Downey
#***

#
# Name of this file.
#
THISFILE=	makefile.qc
MEMMODEL=	L
CFLAGS=		-A$(MEMMODEL) -D__STDC__=1 -Ol
LDFLAGS=	/NOI /NOE /STACK:20480
LINKFILE=	xvi.lnk

.c.obj:
		qcl -c $(CFLAGS) $*.c

.asm.obj:
		masm -Mx -DMEMMODEL=$(MEMMODEL) $* ;

alloc.obj:	alloc.c

ascii.obj:	ascii.c

buffers.obj:	buffers.c

cmdline.obj:	cmdline.c

cursor.obj:	cursor.c

defscr.obj:	defscr.c

edit.obj:	edit.c

ex_cmds1.obj:	ex_cmds1.c

ex_cmds2.obj:	ex_cmds2.c

events.obj:	events.c

fileio.obj:	fileio.c

find.obj:	find.c

flexbuf.obj:	flexbuf.c

ibmpc_c.obj:	ibmpc_c.c

map.obj:	map.c

mark.obj:	mark.c

misccmds.obj:	misccmds.c

mouse.obj:	mouse.c

movement.obj:	movement.c

msdos_c.obj:	msdos_c.c

normal.obj:	normal.c

param.obj:	param.c

pipe.obj:	pipe.c

preserve.obj:	preserve.c

ptrfunc.obj:	ptrfunc.c

regexp.obj:	regexp.c

screen.obj:	screen.c

search.obj:	search.c

startup.obj:	startup.c

status.obj:	status.c

tags.obj:	tags.c

undo.obj:	undo.c

windows.obj:	windows.c

yankput.obj:	yankput.c

ibmpc_a.obj:	ibmpc_a.asm

msdos_a.obj:	msdos_a.asm

$(LINKFILE):	$(THISFILE)
		+echo alloc + ascii + buffers + > $(LINKFILE)
		+echo cmdline + cursor + >> $(LINKFILE)
		+echo edit + ex_cmds1 + ex_cmds2 + events + >> $(LINKFILE)
		+echo fileio + find + flexbuf + map + mark + >> $(LINKFILE)
		+echo misccmds + mouse + movement + normal + >> $(LINKFILE)
		+echo param + pipe + >> $(LINKFILE)
		+echo preserve + ptrfunc + regexp + screen + >> $(LINKFILE)
		+echo search + startup + status + tags + >> $(LINKFILE)
		+echo undo + version + windows + >> $(LINKFILE)
		+echo yankput + >> $(LINKFILE)
		+echo defscr + >> $(LINKFILE)
		+echo msdos_c + msdos_a + ibmpc_a + ibmpc_c + >> $(LINKFILE)
		+echo $(LIB)\setargv >> $(LINKFILE)
		+echo xvi.exe $(LDFLAGS) ; >> $(LINKFILE)

xvi.exe:	alloc.obj ascii.obj buffers.obj cmdline.obj cursor.obj \
		edit.obj ex_cmds1.obj ex_cmds2.obj events.obj fileio.obj \
		find.obj flexbuf.obj map.obj mark.obj misccmds.obj \
		mouse.obj movement.obj \
		normal.obj param.obj pipe.obj preserve.obj ptrfunc.obj \
		regexp.obj screen.obj search.obj startup.obj status.obj \
		tags.obj undo.obj windows.obj \
		yankput.obj \
		defscr.obj \
		ibmpc_c.obj msdos_c.obj ibmpc_a.obj msdos_a.obj \
		$(LINKFILE)
		qcl -c $(CFLAGS) version.c
		link @$(LINKFILE)
