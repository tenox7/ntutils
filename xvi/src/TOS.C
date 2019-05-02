/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)tos.c	2.1 (Chris & John Downey) 7/29/92";
#endif

/***

* program name:
    xvi
* function:
    Portable version of UNIX "vi" editor, with extensions.
* module name:
    tos.c
* module function:
    System interface module for the Atari ST.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

    Atari port modified by Steve Found for LATTICE C V5.0

***/

#include "xvi.h"

#include <osbind.h>

/*
 * The following buffer is used to work around a bug in TOS. It appears that
 * unread console input can cause a crash, but only if console output is
 * going on. The solution is to always grab any unread input before putting
 * out a character. The following buffer holds any characters read in this
 * fashion. The problem can be easily produced because we can't yet keep
 * up with the normal auto-repeat rate in insert mode.
 */
#define IBUFSZ	128

static	long	inbuf[IBUFSZ];		/* buffer for unread input */
static	long	*inptr = inbuf;		/* where to put next character */

/*
 * These are globals which are set by the OS-specific module,
 * and used for various purposes throughout the rest of xvi.
 */
int	Rows;				/* Number of Rows and Columns */
int	Columns;			/* in the current window. */

static	char	tmpbuff[L_tmpnam];
static	char	*logscreen;

/*
 * inchar() - get a character from the keyboard
 *
 * Certain special keys are mapped to values above 0x80. These
 * mappings are defined in keymap.h. If the key has a non-zero
 * ascii value, it is simply returned. Otherwise it may be a
 * special key we want to map.
 *
 * The ST has a bug involving keyboard input that seems to occur
 * when typing quickly, especially typing capital letters. Sometimes
 * a value of 0x02540000 is read. This doesn't correspond to anything
 * on the keyboard, according to my documentation. My solution is to
 * loop when any unknown key is seen. Normally, the bell is rung to
 * indicate the error. If the "bug" value is seen, we ignore it completely.
 *
 * "timeout" parameter not handled at the moment.
 */
int
inchar(timeout)
long	timeout;
{
    int		k, s;

    for (;;) {
	long c;

	/*
	 * Get the next input character, either from the input
	 * buffer or directly from TOS.
	 */
	if (inptr != inbuf) {	/* input in the buffer, use it */
	    long	*p;

	    c = inbuf[0];
	    /*
	     * Shift everything else in the buffer down. This
	     * would be cleaner if we used a circular buffer,
	     * but it really isn't worth it.
	     */
	    inptr--;
	    for (p = inbuf; p < inptr ;p++)
		*p = *(p+1);
	} else {
	    c = Crawcin();
	}

	k = (c & 0xFF);
	s = (c >> 16) & 0xFF;
	if (k != 0)
	    break;

	switch (s) {

	case 0x62: k = K_HELP; break;
	case 0x61: k = K_UNDO; break;
	case 0x52: k = K_INSERT; break;
	case 0x47: k = K_HOME; break;
	case 0x48: k = K_UARROW; break;
	case 0x50: k = K_DARROW; break;
	case 0x4b: k = K_LARROW; break;
	case 0x4d: k = K_RARROW; break;
	case 0x29: k = K_CGRAVE; break; /* control grave accent */

	/*
	 * Occurs due to a bug in TOS.
	 */
	case 0x54:
	    break;

	/*
	 * Add the function keys here later if we put in support
	 * for macros.
	 */

	default:
	    k = 0;
	    alert();
	    break;
	}

	if (k != 0) {
	    break;
	}
    }
    return(k);
}

/*
 * get_inchars - snarf away any pending console input
 *
 * If the buffer overflows, we discard what's left and ring the bell.
 */
static void
get_inchars()
{
    while (Cconis()) {
	if (inptr >= &inbuf[IBUFSZ]) {	/* no room in buffer? */
	    Crawcin();		/* discard the input */
	    alert();			/* and sound the alarm */
	} else {
	    *inptr++ = Crawcin();
	}
    }
}

void
outchar(c)
char	c;
{
    get_inchars();
    Cconout((short)c);
}

void
outstr(s)
register char	*s;
{
    get_inchars();
    Cconws(s);
}

#define BGND	0
#define TEXT	1

/*
 * vbeep() - visual bell
 */
static void
vbeep()
{
    int		text, bgnd;		/* text and background colors */
    long	l;

    text = Setcolor(TEXT, -1);
    bgnd = Setcolor(BGND, -1);

    Setcolor(TEXT, (short) bgnd);		/* swap colors */
    Setcolor(BGND, (short) text);

    for (l=0; l < 5000 ;l++)	/* short pause */
	;

    Setcolor(TEXT, (short) text);		/* restore colors */
    Setcolor(BGND, (short) bgnd);
}

void
alert()
{
    if (Pb(P_vbell))
	vbeep();
    else
	outchar('\007');
}

bool_t
can_write(file)
char	*file;
{
    if (access(file, 0) == -1 || access(file, 2) == 0) {
	return(TRUE);
    } else {
	return(FALSE);
    }
}

/*
 * remove(file) - remove a file
 * I don't know whether success is detectable here - cmd.
 */
#ifndef LATTICE
bool_t
remove(file)
char	*file;
{
    Fdelete(file);
    return(TRUE);
}
#endif

void
sys_init()
{
    logscreen = (char *) Logbase();
    if (Getrez() == 0)
	Columns = 40;		/* low resolution */
    else
	Columns = 80;		/* medium or high */

    Rows = 25;

    Cursconf(1, NULL);
}

void
sys_startv()
{
}

void
sys_endv()
{
}

void
sys_exit(r)
int	r;
{
    tty_goto(25, 0);
    outchar('\n');
    exit(r);
}

void
tty_goto(r, c)
int	r, c;
{
    outstr("\033Y");
    outchar(r + ' ');
    outchar(c + ' ');
}

/*
 * System calls or library routines missing in TOS.
 */

void
sleep(n)
unsigned	n;
{
    int		k;

    k = Tgettime();
    while (Tgettime() <= k + n)
	;
}

void
delay()
{
    long	n;

    for (n = 0; n < 8000; n++)
	;
}

#ifndef LATTICE
int
system(cmd)
char	*cmd;
{
    char	arg[1];

    arg[0] = '\0';		/* no arguments passed to the shell */

    if (Pexec(0, cmd, arg, 0L) < 0) {
	return(-1);
    } else {
	return(0);
    }
}
#endif

#ifdef	MEGAMAX
char *
strchr(s, c)
char	*s;
int	c;
{
    do {
	if (*s == c)
	    return(s);
    } while (*s++);

    return(NULL);
}
#endif /* MEGAMAX */

/*
 * getenv() - get a string from the environment
 *
 * Both Alcyon and Megamax are missing getenv(). This routine works for
 * both compilers and with the Beckemeyer and Gulam shells. With gulam,
 * the env_style variable should be set to either "mw" or "gu".
 */
#ifndef LATTICE
char *
getenv(name)
char	*name;
{
    extern long	_base;
    char	*envp, *p;

    envp = *((char **) (_base + 0x2c));

    for ( ; *envp; envp += strlen(envp) + 1) {
	if (strncmp(envp, name, strlen(name)) == 0) {
	    p = envp + strlen(name);
	    if (*p++ == '=')
		return(p);
	}
    }
    return(NULL);
}
#endif

/*
 * Set the specified colour. Just does standout/standend mode for now.
 * Optimisation here to avoid setting standend when we aren't in
 * standout; assumes calling routines are well-behaved (i.e. only do
 * screen movement in P_colour) or some terminals will write garbage
 * all over the screen.
 */
void
set_colour(c)
int	c;
{
    static int	oldc = -1;

    if (c == oldc)
	return;

    if (c != 0)
	outstr("\033p");
    else
	outstr("\033q");

    oldc = c;
}

/*
 * tempfname - Create a temporary file name.
 */
char *
tempfname(srcname)
char *srcname;
{
    return(tmpnam(tmpbuff));
}

#ifndef ABS
#	define ABS(n) ((n) < 0 ? -(n) : (n))
#endif

/*
 * Scroll the ST Monochrome screen.
 */
void
st_scroll(start, end, nlines)
unsigned start, end;
int nlines;
{
    char *s, *e, *d;
    char *s2;
    size_t bytes, clr_bytes;

    if (ABS(nlines) > (end + 1 - start) || nlines == 0)
	return;

    invis_cursor();

    if (nlines > 0) {
	d = logscreen + (start * 1280);
	s = d + (nlines * 1280);
	s2 = d + bytes;
    } else /* (nlines < 0) */ {
	nlines = -nlines;
	s2 = s = logscreen + (start * 1280);
	d = s + (nlines * 1280);
    }

    /*
     * Move the appropriate lines up or down.
     */
    bytes = (end + 1 - start - nlines) * 1280;
    memmove(d, s, bytes);

    /*
     * Clear the ones left behind.
     */
    clr_bytes = ((end + 1 - start) * 1280) - bytes;
    (void) memset(s2, 0, clr_bytes);

    vis_cursor();
}
