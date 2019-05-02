/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)ibmpc_c.c	2.1 (Chris & John Downey) 7/29/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    ibmpc_c.c
* module function:
    C part of terminal interface module for IBM PC compatibles
    running MS-DOS.

* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

/*
 * Screen dimensions, defined here so they'll go in the C default data
 * segment.
 */
unsigned int	Rows;
unsigned int	Columns;

/*
 * IBM-compatible PC's have a default typeahead buffer which is only
 * big enough for 16 characters, & some 8088-based PC's are so
 * unbelievably slow that xvi can't handle more than about 2
 * characters a second. So we do some input buffering here to
 * alleviate the problem.
 */

static	char	    kbuf[16];
static	char	    *kbrp = kbuf;
static	char	    *kbwp = kbuf;
static	unsigned    kbcount;

static void near
kbfill()
{
    register int	c;

    while (kbcount < sizeof kbuf && (c = getchnw()) >= 0) {
	kbcount++;
	*kbwp = c;
	if (kbwp++ >= &kbuf[sizeof kbuf - 1])
	    kbwp = kbuf;
    }
}

static unsigned char near
kbget()
{
    for (;;) {
	if (kbcount > 0) {
	    unsigned char c;

	    --kbcount;
	    c = *kbrp;
	    if (kbrp++ >= &kbuf[sizeof kbuf - 1])
		kbrp = kbuf;
	    return c;
	} else {
	    kbfill();
	}
    }
}

/*
 * Convert milliseconds to clock ticks.
 */
#if CLK_TCK > 1000
#		define	MS2CLK(m)	((long)(m) * (CLK_TCK / 1000))
#else
#	if CLK_TCK < 1000
#		define	MS2CLK(m)	((long)(m) / (1000 / CLK_TCK))
#	else
#		define	MS2CLK(m)	(m)
#	endif
#endif

/*
 * inchar() - get a character from the keyboard
 */
int
inchar(long mstimeout)
{
    clock_t		stoptime;

    if (kbcount == 0) {
	flush_output();
	if (mstimeout != 0) {
	    stoptime = clock() + MS2CLK(mstimeout);
	}
	kbfill();
    }
    for (;;) {
	static clock_t	lastevent;
	register int	c;

	if (kbcount == 0) {
	    unsigned	prevbstate;
	    unsigned	prevx, prevy;
	    bool_t	isdrag;

	    if (State == NORMAL) {
		showmouse();
		prevbstate = 0;
	    }
	    for (; kbcount == 0; kbfill()) {
		/*
		 * Time out if we have to.
		 */
		if (mstimeout != 0 && clock() > stoptime) {
		    break;
		}
		if (State == NORMAL) {
		    unsigned	buttonstate;
		    unsigned	mousex,
				mousey;

		    /*
		     * If there's no keyboard input waiting to be
		     * read, watch out for mouse events. We don't do
		     * this if we're in insert or command line mode.
		     */

		    buttonstate = mousestatus(&mousex, &mousey) & 7;
		    mousex /= 8;
		    mousey /= 8;
		    if (prevbstate == 0) {
			isdrag = FALSE;
		    } else {
			if (buttonstate) {
			    /*
			     * If a button is being held down, & the
			     * position has changed, this is a mouse
			     * drag event.
			     */
			    if (mousex != prevx || mousey != prevy) {
				hidemouse();
				mousedrag(prevy, mousey, prevx, mousex);
				showmouse();
				isdrag = TRUE;
			    }
			} else {
			    if (!isdrag) {
				/*
				 * They've pressed & released a button
				 * without moving the mouse.
				 */
				hidemouse();
				mouseclick(mousey, mousex);
				showmouse();
			    }
			}
		    }
		    if ((prevbstate = buttonstate) != 0) {
			prevx = mousex;
			prevy = mousey;
		    }
		}
	    }
	    if (State == NORMAL) {
		hidemouse();
	    }
	    if (kbcount == 0) {
		/*
		 * We must have timed out.
		 */
		return EOF;
	    }
	}
	c = kbget();
	/*
	 * On IBM compatible PC's, function keys return '\0' followed
	 * by another character. Check for this, and turn function key
	 * presses into appropriate "normal" characters to do the
	 * right thing in xvi.
	 */
	if (c != '\0') {
	    return(c);
	}
	/* else must be a function key press */
	{
	    if (State != NORMAL) {
		/*
		 * Function key pressed during insert or command line
		 * mode. Get the next character ...
		 */
		if (kbget() == 0x53) {
		    /*
		     * ... and if it's the delete key, return it as a
		     * backspace ...
		     */
		    return '\b';
		}
		/*
		 * ... otherwise it isn't valid ...
		 */
		alert();

		/*
		 * Typical MS-DOS users are fairly naive & may not
		 * understand how to get out of insert mode. To make
		 * things easier, we do it for them here.
		 */
		switch (State) {
		case INSERT:
		case REPLACE:
		    return ESC;
		default:
		    continue;
		}
	    }
	    /* else (State == NORMAL) ... */
	    switch (kbget()) {
	    case 0x3b: return(K_HELP);		/* F1 key */
	    case 0x47: return('H');		/* home key */
	    case 0x48: return('k');		/* up arrow key */
	    case 0x49: return (CTRL('B'));	/* page up key */
	    case 0x4b: return('\b');		/* left arrow key */
	    case 0x4d: return(' ');		/* right arrow key */
	    case 0x4f: return('L');		/* end key */
	    case 0x50: return('j');		/* down arrow key */
	    case 0x51: return (CTRL('F'));	/* page down key */
	    case 0x52: return('i');		/* insert key */
	    case 0x53: return('x');		/* delete key */
	    /*
	     * default:
	     *	fall through and ignore both characters ...
	     */
	    }
	    continue;
	}
    }
}

#ifdef __ZTC__
#   ifdef DOS386
#	define Z386
#   endif
#endif

#ifndef Z386

/*
 * The routines in ibmpc_a.asm need to call this because they can't
 * invoke C macros directly.
 *
 * Return Pn(P_colour) in the al register, Pn(P_statuscolour) in ah, &
 * Pb(P_vbell) in dx.
 *
 * This will only work with a Microsoft-compatible compiler.
 */
long
cparams()
{
    return ((long) Pb(P_vbell) << 16) |
	   (unsigned short) ((Pn(P_statuscolour) << 8) |
		 (unsigned char) Pn(P_colour));
}

#endif	/* not Z386 */
