#
# Makefile for tde 5.1 for djgpp, unix & MinGW
# June 5, 1994, Frank Davis
# July 24, 1997, Jason Hood
# May 25, 1998, Jason Hood - combined unix and djgpp makefiles; added install
# August 23, 2002, Jason Hood - place object files in the OS directory
# August 27, 2002, Jason Hood - add win32 OS for MinGW32.
# March 18, 2003, Jason Hood - add BRIEF option.
#
# UNIX works with ncurses 1.8.5 (according to Frank).
#
# I've used make 3.79.1; it may work with previous versions.
#
# jmh 050818: changed djgpp project to "tdep.exe" (protected mode)
#	      changed Win32 project to "tdew.exe" (Windows)
# jmh 050920: djgpp/Windows install as "tde.exe";
#	      only create link for unix, at install; use supplied link file for
#	       djgpp and Windows.
#

# Define the system for which to compile.
OS = djgpp
#OS = win32
#OS = unix

# Location to place the executable/binary file.
ifeq ($(OS),djgpp)
bindir = d:/utils
else
ifeq ($(OS),win32)
bindir = c:/utils
else
bindir = /usr/local/bin
endif
endif

# Should the executable/binary be stripped?
STRIP = strip
#STRIP = true

# Should the executable be compressed?
DJP = upx -9 -qq	# djgpp users should use v1.23.
#DJP = true

# Comment the following to see the compilation commands.
BRIEF = @

# Version number for the distribution
ifeq ($(VER),)
ifeq ($(OS),unix)
VER = 5.1
else
VER = 51
endif
endif

CFLAGS	= -Wall -g
CFLAGS += -O2 -finline-functions
CFLAGS += -mtune=pentiumpro -fomit-frame-pointer
CFLAGS += -DNDEBUG

ifeq ($(OS),unix)
CFLAGS += -D__UNIX__
# Uncomment the below to use the DOS graphics characters (codepage 437).
# Note: It doesn't work in an xterm.
#CFLAGS += -DPC_CHARS
PROJ	= tde
DBG	= tdedbg
LIBS	= -lncurses
else
ifeq ($(OS),win32)
PROJ	= tdew.exe
DBG	= tdewdbg.exe
LINK	= tdv.cmd		# XP batch to call tde -v
else
PROJ	= tdep.exe
DBG	= tdepdbg.exe
LINK	= tdv.exe		# links to tde *not* to tdep
endif
LIBS	=
endif

CC = gcc

VPATH	 = $(OS)
CPPFLAGS = -I.

OBJS = bj_ctype.o  block.o    cfgfile.o  config.o    console.o	criterr.o  \
       default.o   dialogs.o  diff.o	 dirlist.o   ed.o	file.o	   \
       filmatch.o  findrep.o  global.o	 help.o      hwind.o	macro.o    \
       main.o	   memory.o   menu.o	 movement.o  port.o	prompts.o  \
       pull.o	   query.o    regx.o	 sort.o      syntax.o	tab.o	   \
       undo.o	   utils.o    window.o	 wordwrap.o

OBJECTS = $(addprefix $(OS)/, $(OBJS))

INC  = tdestr.h common.h tdefunc.h define.h

$(OS)/%.o : %.c
ifdef BRIEF
	@echo Compiling $<
endif
	$(BRIEF)$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(PROJ): $(OBJECTS)
ifdef BRIEF
	@echo Creating $(PROJ)
endif
	$(BRIEF)$(CC) -o $@ $^ $(LIBS)

$(OS)/bj_ctype.o:  bj_ctype.c bj_ctype.h tdestr.h common.h
$(OS)/block.o:	   block.c    $(INC)
$(OS)/cfgfile.o:   cfgfile.c  tdestr.h syntax.h config.h
$(OS)/config.o:    config.c   config.h $(INC)
$(OS)/console.o:   console.c  $(INC)
$(OS)/criterr.o:   criterr.c  criterr.h $(INC)
$(OS)/default.o:   default.c  tdestr.h define.h
$(OS)/dialogs.o:   dialogs.c  tdestr.h
$(OS)/diff.o:	   diff.c     $(INC)
$(OS)/dirlist.o:   dirlist.c  $(INC)
$(OS)/ed.o:	   ed.c       $(INC)
$(OS)/file.o:	   file.c     $(INC)
$(OS)/filmatch.o:  filmatch.c filmatch.h tdestr.h common.h
$(OS)/findrep.o:   findrep.c  $(INC)
$(OS)/global.o:    global.c   tdestr.h tdefunc.h
$(OS)/help.o:	   help.c
$(OS)/hwind.o:	   hwind.c    $(INC)
$(OS)/macro.o:	   macro.c    $(INC)
$(OS)/main.o:	   main.c     $(INC)
$(OS)/memory.o:    memory.c   $(INC)
$(OS)/menu.o:	   menu.c     tdestr.h define.h
$(OS)/movement.o:  movement.c $(INC)
$(OS)/port.o:	   port.c     $(INC)
$(OS)/prompts.o:   prompts.c  bj_ctype.h
$(OS)/pull.o:	   pull.c     $(INC)
$(OS)/query.o:	   query.c    $(INC)
$(OS)/regx.o:	   regx.c     $(INC)
$(OS)/sort.o:	   sort.c     $(INC)
$(OS)/syntax.o:    syntax.c   syntax.h $(INC)
$(OS)/tab.o:	   tab.c      $(INC)
$(OS)/undo.o:	   undo.c     $(INC)
$(OS)/utils.o:	   utils.c    $(INC)
$(OS)/window.o:    window.c   $(INC)
$(OS)/wordwrap.o:  wordwrap.c $(INC)

install: $(PROJ)
	cp $(PROJ) $(DBG)
	-$(STRIP) $(PROJ)
	-$(DJP) $(PROJ)
ifeq ($(OS),unix)
	cp $(PROJ) $(bindir)
	ln -fs $(bindir)/$(PROJ) $(bindir)/tdv
else
	cp $(PROJ) $(bindir)/tde.exe
	cp $(LINK) $(bindir)
endif

clean:
	rm -f $(OS)/*.o

dist:
ifeq ($(OS),unix)
	tar -cf tde-$(VER).tar.gz --name-prefix=tde-$(VER)/ \
	--files-from=src.lst --gzip
else
	mv tde.shl tdesrc.shl
	mv tdedist.shl tde.shl
	zip -X9 tde$(VER)b -@ <bin.lst
	mv tde.shl tdedist.shl
	mv tdesrc.shl tde.shl
	zip -X9 tde$(VER)s -@ <src.lst
endif
