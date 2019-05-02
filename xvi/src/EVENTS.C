/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)events.c	1.1 (Chris & John Downey) 7/31/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    events.c
* module function:
    Deals with incoming events.
    The main entry point for input to the editor.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

static	bool_t	n_proc P((int));
static	bool_t	c_proc P((int));
static	bool_t	d_proc P((int));

volatile int	keystrokes;

long
xvi_handle_event(ev)
xvEvent	*ev;
{
    bool_t		do_update;
    int		c;

    switch (ev->ev_type) {
    case Ev_char:
	keystrokes++;
	map_char(ev->ev_inchar);
	break;

    case Ev_timeout:
	if (map_waiting()) {
	    map_timeout();
	} else if (keystrokes >= PSVKEYS) {
	    do_preserve();
	    keystrokes = 0;
	}
	break;
    }

    /*
     * Look to see if the event produced any input characters
     * which we can feed into the editor. Call the appropriate
     * function for each one, according to the current State.
     */
    do_update = FALSE;
    while ((c = map_getc()) != EOF) {
	bool_t	(*func)P((int));

	switch (State) {
	case NORMAL:
	    func = n_proc;
	    break;

	case CMDLINE:
	    func = c_proc;
	    break;

	case DISPLAY:
	    func = d_proc;
	    break;

	case INSERT:
	    func = i_proc;
	    break;

	case REPLACE:
	    func = r_proc;
	    break;
	}
	if ((*func)(c)) {
	    do_update = TRUE;
	}

	/*
	 * Look at the resultant state, and the
	 * result of the proc() routine, to see
	 * whether to update the display.
	 */
	switch (State) {
	case CMDLINE:
	case DISPLAY:
	    break;

	case NORMAL:
	case INSERT:
	case REPLACE:
	    if (do_update) {
		move_window_to_cursor(curwin);
		cursupdate(curwin);
		wind_goto(curwin);
	    }
	}
    }

    if (kbdintr) {
	if (imessage) {
	    show_message(curwin, "Interrupted");
	    wind_goto(curwin);	/* put cursor back */
	}
	imessage = (kbdintr = 0);
    }

    if (map_waiting()) {
	return((long) Pn(P_timeout));
    } else if (keystrokes >= PSVKEYS) {
	return((long) Pn(P_preservetime) * 1000);
    } else {
	return(0);
    }
}

/*
 * Process the given character in command mode.
 */
static bool_t
n_proc(c)
int	c;
{
    unsigned	savecho;
    bool_t	result;

    savecho = echo;
    result = normal(c);
    echo = savecho;
    return(result);
}

/*
 * Returns TRUE if screen wants updating, FALSE otherwise.
 */
static bool_t
c_proc(c)
int	c;
{
    char	*cmdline;

    switch (cmd_input(curwin, c)) {
    case cmd_CANCEL:
	/*
	 * Put the status line back as it should be.
	 */
	show_file_info(curwin);
	update_window(curwin);
	return(FALSE);

    case cmd_INCOMPLETE:
	return(FALSE);

    case cmd_COMPLETE:
	cmdline = get_cmd(curwin);
	(void) yank_str(cmdline[0], cmdline, TRUE);
	switch (cmdline[0]) {
	case '/':
	case '?':
	    (void) dosearch(curwin, cmdline + 1, cmdline[0]);
	    move_window_to_cursor(curwin);
	    break;

	case '!':
	    do_pipe(curwin, cmdline + 1);
	    break;

	case ':':
	    do_colon(cmdline + 1, TRUE);
	}
	return(TRUE);
    }
    /*NOTREACHED*/
}

/*ARGSUSED*/
static bool_t
d_proc(c)
int	c;
{
    if (c == CTRL('C')) {
	/*
	 * In some environments it's possible to type
	 * control-C without actually generating an interrupt,
	 * but if they do, in this context, they probably want
	 * the semantics of an interrupt anyway.
	 */
	imessage = (kbdintr = 1);
    }
    return(disp_screen(curwin));
}
