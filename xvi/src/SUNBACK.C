#ifndef lint
static char *sccsid = "@(#)sunback.c	2.1 (Chris & John Downey) 7/29/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    sunback.c
* module function:
    Terminal interface module for SunView: module for linking to
    main program.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey
***/

#include "xvi.h"

/*
 * Exported.
 */
unsigned int	Rows = 0;			 /* screen dimensions */
unsigned int	Columns = 0;

/*
 * These are used to optimize output.
 */
static	int	real_row, real_col;
static	int	virt_row, virt_col;
static	bool_t	optimize;

#define NO_COLOUR	-1

static int	old_colour = NO_COLOUR;

/*
 * Update real cursor position.
 *
 * SunView can be painfully slow, at least on Sun 3's and 386i's, but
 * it's difficult to know the best way of optimizing it. The number of
 * characters sent to the display probably isn't really significant,
 * because it isn't a terminal. The things that take the most time are
 * more likely to be calculations (especially divisions &
 * multiplications) & manipulating bit maps. The bitmap for a single
 * character has to be xor'ed twice for each cursor movement, since
 * terminal subwindows use a reverse video block cursor.
 */
#define MAXCMOVES	3	/*
				 * This is a guess at the probable
				 * maximum number of individual cursor
				 * movements that's worthwhile to
				 * avoid the calculations involved in
				 * an absolute goto.
				 */
#ifndef ABS
#	define ABS(n) ((n) < 0 ? -(n) : (n))
#endif

static void
xyupdate()
{
    int hdisp,	/* horizontal (rightward) displacement wanted */
	vdisp,	/* vertical (downward) displacement wanted */
	ahdisp,	/* absolute horizontal displacement wanted */
	avdisp,	/* absolute vertical displacement wanted */
	homedisp;	/* total displacement from home position */

    if (optimize) {
	if (virt_col == real_col && virt_row == real_row)
	    /*
	     * Nothing to do.
	     */
	    return;
	} else {
	    hdisp = virt_col - real_col;
	    vdisp = virt_row - real_row;
	    ahdisp = ABS(hdisp);
	    avdisp = ABS(vdisp);
	}
    }
    homedisp = virt_row + virt_col;
    /*
     * Is it worth sending a cursor home sequence?
     */
    if (homedisp < MAXCMOVES && (!optimize || homedisp < avdisp + ahdisp)) {
	fputs("\033[H", stdout);
	real_row = real_col = 0;
	ahdisp = hdisp = virt_col;
	avdisp = vdisp = virt_row;
	optimize = TRUE;
    }
    if (optimize) {
	/*
	 * Is it worth sending a carriage return?
	 */
	if (hdisp < 0 && virt_col < ahdisp && virt_col < MAXCMOVES) {
	    putc('\r', stdout);
	    real_col = 0;
	    ahdisp = hdisp = virt_col;
	}
	if (hdisp == 0 && vdisp == 0) {
	    /*
	     * We're already there, so there's nothing to do.
	     */
	    return;
	}
	/*
	 * Calculate total absolute displacement to see
	 * whether it's worthwhile doing any of this.
	 */
	if (ahdisp + avdisp <= MAXCMOVES) {
	    for (; real_col < virt_col; real_col++)
		/*
		 * Do non-destructive spaces.
		 */
		fputs("\033[C", stdout);
	    for (; real_col > virt_col; real_col--)
		/*
		 * Do backspaces.
		 */
		putc('\b', stdout);
	    for (; real_row < virt_row; real_row++)
		/*
		 * Go down.
		 */
		putc('\n', stdout);
	    for (; real_row > virt_row; real_row--)
		/*
		 * Go up.
		 */
		fputs("\033[A", stdout);
	    return;
	}
	/*
	 * It isn't worthwhile. Give up & send an absolute
	 * goto.
	 */
    }
    fprintf(stdout, "\033[%d;%dH", virt_row + 1, virt_col + 1);
    real_row = virt_row;
    real_col = virt_col;
    optimize = TRUE;
}

/*
 * Flush any pending output, including cursor position.
 */
void
flush_output()
{
    xyupdate();
    (void) fflush(stdout);
}

/*
 * Put out a "normal" character, updating the cursor position.
 */
void
outchar(c)
register int	c;
{
    xyupdate();
    putc(c, stdout);
    if (++virt_col >= Columns) {
	virt_col = 0;
	if (++virt_row >= Rows)
	    --virt_row;
	real_row = virt_row;
    }
    real_col = virt_col;
}

/*
 * Put out a "normal" string, updating the cursor position.
 */
void
outstr(s)
register char	*s;
{
    xyupdate();
    for (; *s; s++)
    {
	putc(*s, stdout);
	if (++virt_col >= Columns)
	{
	    virt_col = 0;
	    if (++virt_row >= Rows)
		--virt_row;
	}
    }
    real_row = virt_row;
    real_col = virt_col;
}

/*
 * Beep at the user.
 */
void
alert()
{
    putc('\007', stdout);
    (void) fflush(stdout);
}

/*
 * Get a hexadecimal number from the front end process.
 */
static
hexnumber()
{
    register int x;

    x = 0;
    for (;;) {
	register int c;

	c = inch(0);
	if (c == ';')
	    return x;
	if (!is_xdigit(c))
	    return -1;
	x = (x << 4) | hex_to_bin(c);
    }
}

/*
 * Get single character or control sequence from front-end process.
 * The following control sequences are understood:
 *
 * Sequence			Meaning
 *
 * PREFIXCHAR 'k'		up arrow key pressed
 * PREFIXCHAR 'j'		down arrow key pressed
 * PREFIXCHAR '\b'		left arrow key pressed
 * PREFIXCHAR ' '		right arrow key pressed
 * PREFIXCHAR CTRL('F')		page down key pressed
 * PREFIXCHAR CTRL('B')		page up key pressed
 * PREFIXCHAR 'H'		home key pressed
 * PREFIXCHAR 'L'		end key pressed
 * PREFIXCHAR PREFIXCHAR	PREFIXCHAR pressed
 * PREFIXCHAR 'p' hexnum ';' hexnum ';'
 *				mouse button pressed at row & column
 *				specified by the digits.
 * PREFIXCHAR 'm' hexnum ';' hexnum ';' hexnum ';' hexnum ';'
 *				mouse dragged with middle button held
 *				down. Numbers give starting row &
 *				column, then ending row & column, in
 *				that order.
 *
 * PREFIXCHAR is currently control-R. A `hexnum' is 0 or more
 * hexadecimal digits.
 *
 * Any other process run in the same tty subwindow (with the ":!" or
 * "sh" commands) will also receive the same sequences for the events
 * shown, but using control-R as a prefix should be reasonably safe.
 *
 * The sequence for mouse clicks is dealt with by mouseposn().
 */
int
inchar(timeout)
long	timeout;
{
    for (;;) {
	register int	c;

	if ((c = inch(timeout)) == PREFIXCHAR) {
	/*
	 * This looks like the beginning of a control sequence.
	 */
	    switch (c = inch(0)) {
		case PREFIXCHAR:
		    return PREFIXCHAR;
		case CTRL('B'): case CTRL('F'):
		case 'H': case 'L':
		case '\b': case ' ':
		case 'j': case 'k':
		    /*
		     * This looks like a function
		     * key.
		     */
		    if (State == NORMAL) {
			return c;
		    } else {
			alert();
			continue;
		    }
		case 'm':
		{
		    unsigned row1, row2, col1, col2;

		    /*
		     * Even if we aren't in normal
		     * mode here, we have to call
		     * hexnumber() to eat all the
		     * hex. digits we should be
		     * getting.
		     */
		    if (
			(row1 = hexnumber()) >= 0
			&&
			(row2 = hexnumber()) >= 0
			&&
			(col1 = hexnumber()) >= 0
			&&
			(col2 = hexnumber()) >= 0
			&&
			State == NORMAL
		    ) {
			mousedrag(row1, row2, col1, col2);
		    }
		    continue;
		}
		case 'p':
		{
		    unsigned row, col;

		    /*
		     * Even if we aren't in normal
		     * mode here, we have to call
		     * hexnumber() to eat all the
		     * hex. digits we should be
		     * getting.
		     */
		    if (
			(row = hexnumber()) >= 0
			&&
			(col = hexnumber()) >= 0
			&&
			State == NORMAL
		    ) {
			mouseclick(row, col);
		    }
		    continue;
		}
		default:
		    continue;
	    }
	} else if (c == CTRL('Z') && State == NORMAL) {
	    /*
	     * This isn't reasonable. Job control isn't
	     * available here because this process wasn't
	     * started from a shell (it was started by the
	     * SunView Notifier). They can always use
	     * another window to run commands.
	     */
	    alert();
	    continue;
	} else {	/* It's just a normal character. */
	    return c;
	}
    }
}

static time_t	starttime;

/*
 * Initialise the terminal interface.
 *
 * This must be done before any screen-based i/o is performed.
 */
void
tty_open(prows, pcolumns)
unsigned int *prows,
	     *pcolumns;
{
    /*
     * If sys_startv() hasn't already got valid values for the
     * window size from its TIOCGWINSZ ioctl, something is
     * seriously wrong with SunOS.
     */
    Rows = *prows;
    Columns = *pcolumns;

    (void) time(&starttime);

#ifdef sparc
    /*
     * Unfortunately, this implementation still doesn't give us
     * scrolling windows, but SparcStations are fast enough that
     * it doesn't really matter.
     */
    set_param(P_jumpscroll, js_OFF, (char **) NULL);
#endif

    {
	/*
	 * These escape sequences may be generated by the
	 * arrow keys, depending on how the defaults database
	 * has been set up. All other function keys (page up,
	 * page down, etc.) should be received as function
	 * keys & handled appropriately by the front end
	 * process.
	 */
	static char *mapargs[] = {
	    "\033[A",	"k",
	    "\033[B",	"j",
	    "\033[C",	"l",
	    "\033[D",	"h",
	    NULL
	};
	char **argp;

	for (argp = mapargs; *argp != NULL; argp += 2) {
	    xvi_keymap(argp[0], argp[1]);
	}
    }
}

void
tty_startv()
{
    old_colour = NO_COLOUR;
    optimize = FALSE;
}

/*
 * Set terminal into the mode it was in when we started.
 */
void
tty_endv()
{
    flush_output();
}

/*
 * Tell front end process to quit.
 */
void
tty_close()
{
    time_t	stayuptime, now;

    /*
     * If we're immediately crashing with an error message, give
     * them a chance (5 seconds) to read it.
     */
    stayuptime = starttime + 5;
    if ((now = time((time_t *) 0)) < stayuptime)
	sleep(stayuptime - now);
    write(3, "q", 1);
}

void
tty_goto(row, col)
int	row, col;
{
    virt_row = row;
    virt_col = col;
}

/*
 * Erase the entire current line.
 */
void
erase_line()
{
    xyupdate();
    fputs("\033[K", stdout);
}

/*
 * Insert one line.
 */
void
insert_line()
{
    xyupdate();
    fputs("\033[L", stdout);
}

/*
 * Delete one line.
 */
void
delete_line()
{
    xyupdate();
    fputs("\033[M", stdout);
}

/*
 * Erase display (may optionally home cursor).
 */
void
erase_display()
{
    putc('\f', stdout);
    optimize = FALSE;
    old_colour = NO_COLOUR;
}

/*
 * Set the specified colour. Just does standout/standend mode for now.
 *
 * Colour mapping is as follows:
 *
 *	0	(default systemcolour)	normal
 *	1	(default colour)	normal
 *	2	(default statuscolour)	reverse video
 */
void
set_colour(c)
int	c;
{
    if (c == 1) {
	c = 0;
    } else if (c > 2) {
	c = 2;
    }
    if (c != old_colour) {
	xyupdate();
	fputs((c ? "\033[7m" : "\033[m"), stdout);
	old_colour = c;
    }
}

/*
 * Insert the given character at the cursor position.
 */
void
inschar(c)
char	c;
{
    xyupdate();
    fputs("\033[@", stdout);
    outchar(c);
}
