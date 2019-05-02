/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)pc386.c	2.1 (Chris & John Downey) 7/29/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    pc386.c
* module function:
    Routines for MS-DOS 386 protected mode version.

    This file implements the same routines as ibmpc_a.asm &
    msdos_a.asm, which are for the real mode version.

    Zortech C++ version 3.0 or later & an appropriate DOS extender
    package (such as PharLap's) are required.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey
***/

#include "xvi.h"

#include <cerror.h>
#include <controlc.h>

/*
 * Virtual mode handling.
 *
 * On EGA's & VGA's, the number of rows in text modes can vary because
 * fonts with different sizes can be loaded, so we use virtual modes
 * to contain the additional information needed. (Actually, we handle
 * it a bit simplistically because the Zortech display library only
 * knows about certain cases: 25 rows (default), 43 rows (EGA with
 * 8 x 8 font) & 50 rows (VGA with 8 x 8 font). The last two are
 * treated as equivalent.)
 */
#define MONO25X80	2
#define COLOUR25X80	3
#define MDA25X80	7
#define MASK43		043000
#define COLOUR43X80	(COLOUR25X80 | MASK43)
#define MONO43X80	(MONO25X80 | MASK43)
#define MDA43X80	(MDA25X80 | MASK43)

static unsigned
getvmode()
{
    unsigned	mode;

    if ((mode = disp_getmode()) == COLOUR25X80 || mode == MONO25X80 ||
    							mode == MDA25X80) {
	if (disp_numrows > 25) {
	    return mode | MASK43;
	}
    }
    return mode;
}

static void
setvmode(unsigned vmode)
{
    {
	int	mode;

	if (disp_getmode() != (mode = vmode & ~MASK43)) {
	    disp_close();
	    disp_setmode(mode);
	    disp_open();
	}
    }
    {
	unsigned	m43;

	m43 = vmode & MASK43;
	if (disp_ega && ((m43 && disp_numrows <= 25) ||
			 (!m43 && disp_numrows > 25))) {
	    disp_close();
	    if (m43) {
		disp_set43();
	    } else {
		disp_reset43();
	    }
	    disp_open();
	}
    }
}

static unsigned startmode;		/* system virtual mode */

#if 0

/*
 * Critical error handler.
 *
 * See notes in msdos_a.asm.
 */
static int _far _cdecl
criterr(int *pax, int *pdi)
{
    *pax = ((_osmajor >= 3) ? 3 : 0);
}

#endif

/*
 * Keyboard interrupt handler.
 */
static void _cdecl
inthandler(void)
{
    kbdintr = 1;
#if 0
    return(-1);	/* don't chain to previously installed vector */
#endif
}

/*
 * Install interrupt handlers.
 */
void
msdsignal(unsigned char *flagp)
{
    _controlc_handler = inthandler;
    controlc_open();
#if 0
    _cerror_handler = criterr;
    cerror_open();
    int_intercept(0x23, inthandler, 0);
    int_intercept(0x24, criterr, 0);
#endif
}

void
catch_signals(void)
{
    /*
     * Set console break flag so that we can be interrupted even
     * when we're not waiting for console input.
     */
    dos_set_ctrl_break(1);
}

/*
 * Pointer to copy of previous screen.
 */
static unsigned short *oldscreen = NULL;

void
tty_open(unsigned *prows, unsigned *pcolumns)
{
    if (!disp_inited) {
	disp_open();
    }
    startmode = getvmode();

    if (disp_base) {
	/*
	 * Allocate storage for copy of previous screen
	 * (except bottom line).
	 *
	 * We don't do this if we're using BIOS calls
	 * (indicated by disp_base == 0) because the BIOS
	 * doesn't do it very well (it causes a lot of cursor
	 * flicker).
	 */
	oldscreen = (unsigned short *)
		    malloc((disp_numrows - 1) * disp_numcols *
					    sizeof(unsigned short));
    }
    *prows = disp_numrows;
    *pcolumns = disp_numcols;
}

static enum {
	m_INITIAL = 0,
	m_SYS = 1,
	m_VI = 2
}	curmode;

/*
 * Save screen contents & set up video state for editor.
 */
void
tty_startv(void)
{
    switch (curmode) {
    case m_VI:
	/*
	 * We're already in vi mode.
	 */
	return;
    case m_SYS:
	/*
	 * This isn't the first call. Force display
	 * package's variables to be re-initialized.
	 */
	disp_close();
	disp_open();
    }
    curmode = m_VI;
    if (getvmode() != startmode) {
	/*
	 * The display mode has changed; set it back to what we
	 * started with.
	 */
	setvmode(startmode);
    }
    if (oldscreen) {
	/*
	 * Save screen contents (except bottom line) so they
	 * can be restored afterwards.
	 */
	disp_peekbox(oldscreen, 0, 0, disp_numrows - 2, disp_numcols - 1);
    }
    msm_init();
    /*
     * Apparently, the Microsoft mouse driver sometimes
     * gets the number of screen rows wrong.
     */
    msm_setareay(0, (disp_numrows - 1) * 8);
}

/*
 * Restore video state to what it was when we started.
 *
 * tty_endv() can be called after it's been called already with no
 * intervening tty_startv(), so we have to check.
 */
void
tty_endv(void)
{
    if (curmode != m_VI) {
	return;
    }
    /*
     * Restore contents of screen.
     */
    if (oldscreen != NULL) {
	disp_pokebox(oldscreen, 0, 0, disp_numrows - 2, disp_numcols - 1);
    }
    disp_flush();
    curmode = m_SYS;
    msm_term();
}

#ifndef ABS
#	define ABS(n) ((n) < 0 ? -(n) : (n))
#endif

void
pc_scroll(unsigned start, unsigned end, int nlines)
{
    if (ABS(nlines) > end + 1 - start) {
	nlines = 0;
    }
    disp_scroll(nlines, start, 0, end, disp_numcols - 1, Pn(P_colour));
}

/*
 * Return a character from standard input if one is immediately
 * available, otherwise -1.
 *
 * Don't do anything special if control-C is typed. Just return it.
 */
int
getchnw()
{
    union REGS r;

    r.h.ah = 6;
    r.h.dl = 0xff;
    intdos(&r, &r);
    return (r.e.flags & 0x40) ?		/* zero flag */
	-1 : (unsigned char) r.h.al;
}
