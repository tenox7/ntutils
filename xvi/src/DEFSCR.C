/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)defscr.c	2.4 (Chris & John Downey) 9/4/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    defscr.c
* module function:
    VirtScr interface using old style porting functions.
    We assume newscr() is only called once; it is an
    error for it to be called more than once.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

static	VirtScr	*newscr P((VirtScr *));
static	void	closescr P((VirtScr *));
static	int	getrows P((VirtScr *));
static	int	getcols P((VirtScr *));
static	void	clear_all P((VirtScr *));
static	void	clear_line P((VirtScr *, int, int));
static	void	xygoto P((VirtScr *, int, int));
static	void	xyadvise P((VirtScr *, int, int, int, char *));
static	void	put_char P((VirtScr *, int, int, int));
static	void	put_str P((VirtScr *, int, int, char *));
static	void	ins_str P((VirtScr *, int, int, char *));
static	void	pset_colour P((VirtScr *, int));
static	int	colour_cost P((VirtScr *));
static	int	scroll P((VirtScr *, int, int, int));
static	void	flushout P((VirtScr *));
static	void	pbeep P((VirtScr *));

VirtScr	defscr = {
    NULL,		/* pv_window	    */
    0,			/* pv_rows	    */
    0,			/* pv_cols	    */

    newscr,		/* v_new	    */
    closescr,		/* v_close	    */
    getrows,		/* v_rows	    */
    getcols,		/* v_cols	    */
    clear_all,		/* v_clear_all	    */
    clear_line,		/* v_clear_line	    */
    xygoto,		/* v_goto	    */
    xyadvise,		/* v_advise	    */
    put_str,		/* v_write	    */
    put_char,		/* v_putc	    */
    pset_colour,	/* v_set_colour	    */
    colour_cost,	/* v_colour_cost    */
    flushout,		/* v_flush	    */
    pbeep,		/* v_beep	    */

    ins_str,		/* v_insert	    */
    scroll,		/* v_scroll	    */
    NULL,		/* v_flash	    */
    NULL,		/* v_status	    */
    NULL,		/* v_activate	    */
};

int
main(argc, argv)
int	argc;
char	*argv[];
{
    xvEvent	event;
    long	timeout = 0;

    /*
     * Set up the system and terminal interfaces. This establishes
     * the window size, changes to raw mode and does whatever else
     * is needed for the system we're running on.
     */
    sys_init();

    if (!can_inschar) {
	defscr.v_insert = NULL;
    }
    if (!can_scroll_area && !can_ins_line && !can_del_line) {
	defscr.v_scroll = NULL;
    }
    defscr.pv_rows = Rows;
    defscr.pv_cols = Columns;

    defscr.pv_window = (genptr *) xvi_startup(&defscr, argc, argv,
    						getenv("XVINIT"));

    while (1) {
	register int	r;

	r = inchar(timeout);
	if (r == EOF) {
	    event.ev_type = Ev_timeout;
	} else {
	    event.ev_type = Ev_char;
	    event.ev_inchar = r;
	}
	timeout = xvi_handle_event(&event);
    }
    return 0;
}

/*ARGSUSED*/
static VirtScr *
newscr(scr)
VirtScr	*scr;
{
    return(NULL);
}

/*ARGSUSED*/
static void
closescr(scr)
VirtScr	*scr;
{
}

/*ARGSUSED*/
static int
getrows(scr)
VirtScr	*scr;
{
    return(scr->pv_rows);
}

/*ARGSUSED*/
static int
getcols(scr)
VirtScr	*scr;
{
    return(scr->pv_cols);
}

/*ARGSUSED*/
static void
clear_all(scr)
VirtScr	*scr;
{
    erase_display();
}

/*ARGSUSED*/
static void
clear_line(scr, row, col)
VirtScr	*scr;
int	row;
int	col;
{
    tty_goto(row, col);
    erase_line();
}

/*ARGSUSED*/
static void
xygoto(scr, row, col)
VirtScr	*scr;
int	row;
int	col;
{
    tty_goto(row, col);
}

/*ARGSUSED*/
static void
xyadvise(scr, row, col, index, str)
VirtScr	*scr;
int	row;
int	col;
int	index;
char	*str;
{
    if (index > cost_goto) {
	tty_goto(row, col + index);
    } else {
	tty_goto(row, col);
	while (--index > 0) {
	    outchar(*str++);
	}
    }
}

/*ARGSUSED*/
static void
put_str(scr, row, col, str)
VirtScr	*scr;
int	row;
int	col;
char	*str;
{
    tty_goto(row, col);
    outstr(str);
}

/*ARGSUSED*/
static void
put_char(scr, row, col, c)
VirtScr	*scr;
int	row;
int	col;
int	c;
{
    tty_goto(row, col);
    outchar(c);
}

/*ARGSUSED*/
static void
ins_str(scr, row, col, str)
VirtScr	*scr;
int	row;
int	col;
char	*str;
{
    /*
     * If we are called, can_inschar is TRUE,
     * so we know it is safe to use inschar().
     */
    tty_goto(row, col);
    for ( ; *str != '\0'; str++) {
	inschar(*str);
    }
}

/*ARGSUSED*/
static void
pset_colour(scr, colour)
VirtScr	*scr;
int	colour;
{
    set_colour(colour);
}

/*ARGSUSED*/
static int
colour_cost(scr)
VirtScr	*scr;
{
#ifdef	SLINE_GLITCH
    return(SLINE_GLITCH);
#else
    return(0);
#endif
}

/*ARGSUSED*/
static int
scroll(scr, start_row, end_row, nlines)
VirtScr	*scr;
int	start_row;
int	end_row;
int	nlines;
{
    if (nlines < 0) {
	/*
	 * nlines negative means scroll reverse - i.e. move
	 * the text downwards with respect to the terminal.
	 */
	nlines = -nlines;

	if (can_scroll_area) {
	    scroll_down(start_row, end_row, nlines);
	} else if (can_ins_line && end_row == Rows - 1) {
	    int	line;

	    for (line = 0; line < nlines; line++) {
		tty_goto(start_row, 0);
		insert_line();
	    }
	} else {
	    return(0);
	}
    } else if (nlines > 0) {
	/*
	 * Whereas nlines positive is "normal" scrolling.
	 */
	if (can_scroll_area) {
	    scroll_up(start_row, end_row, nlines);
	} else if (end_row == Rows - 1) {
	    int	line;

	    if (can_del_line) {
		for (line = 0; line < nlines; line++) {
		    tty_goto(start_row, 0);
		    delete_line();
		}
	    } else if (start_row == 0) {
		tty_goto(start_row, 0);
		for (line = 0; line < nlines; line++) {
		    delete_line();
		}
	    } else {
		return(0);
	    }
	} else {
	    return(0);
	}
    }
    return(1);
}

/*ARGSUSED*/
static void
flushout(scr)
VirtScr	*scr;
{
    flush_output();
}

/*ARGSUSED*/
static void
pbeep(scr)
VirtScr	*scr;
{
    alert();
    flush_output();
}
