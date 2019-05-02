MAKE=make

all:
	@echo 'What (emx, unix, cygwin, bcc2, win32ms, win32bcc5, win32mingw, djgpp2, vag)?'

emx:
	$(MAKE) -f fte-emx.mak

unix:
	$(MAKE) -f fte-unix.mak

cygwin:
	$(MAKE) -f fte-cygwin-xf86.mak

bcc2:
	$(MAKE) -f fte-bcc2.mak

win32ms:
	nmake /f fte-msvc.mak

win32mingw:
	$(MAKE) -f fte-mngw.mak

win32bcc5:
	$(MAKE) -f fte-bcc5.mak

djgpp2:
	$(MAKE) -f fte-dj2.mak

vag:
	nmake /f fte-vag.mak

.PHONY: tags
tags:
	$(RM) -f tags
	find . -type f \( -name '*.cpp' -o -name '*.c' -o -name '*.h' \) | xargs ctags -a

clean:
	$(RM) -f core *.o *~ *.gch cscope.out .depend defcfg.h defcfg.cnf tags
