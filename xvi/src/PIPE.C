/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)pipe.c	2.1 (Chris & John Downey) 7/29/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    pipe.c
* module function:
    Handle pipe operators.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

static	int	p_write P((FILE *));
static	long	p_read P((FILE *));

static	Xviwin	*specwin;
static	Line	*line1, *line2;
static	Line	*newlines;

/*
 * This function is called when the ! is typed in initially,
 * to specify the range; do_pipe() is then called later on
 * when the line has been typed in completely.
 *
 * Note that we record the first and last+1th lines to make the loop easier.
 */
void
specify_pipe_range(window, l1, l2)
Xviwin	*window;
Line	*l1;
Line	*l2;
{
    /*
     * Ensure that the lines specified are in the right order.
     */
    if (l1 != NULL && l2 != NULL && earlier(l2, l1)) {
	register Line	*tmp;

	tmp = l1;
	l1 = l2;
	l2 = tmp;
    }
    line1 = (l1 != NULL) ? l1 : window->w_buffer->b_file;
    line2 = (l2 != NULL) ? l2->l_next : window->w_buffer->b_lastline;
    specwin = window;
}

/*
 * Pipe the given sequence of lines through the command,
 * replacing the old set with its output.
 */
void
do_pipe(window, command)
Xviwin	*window;
char	*command;
{
    if (line1 == NULL || line2 == NULL || specwin != window) {
	show_error(window,
	    "Internal error: pipe through badly-specified range.");
	return;
    }

    newlines = NULL;
    if (sys_pipe(command, p_write, p_read) && newlines != NULL) {
	repllines(window, line1, cntllines(line1, line2) - 1, newlines);
	update_buffer(window->w_buffer);
	begin_line(window, TRUE);
    } else {
	show_error(window, "Failed to execute \"%s\"", command);
	redraw_screen();
    }
    cursupdate(window);
}

static int
p_write(fp)
FILE	*fp;
{
    Line	*l;
    long	n;

    for (l = line1, n = 0; l != line2; l = l->l_next, n++) {
	(void) fputs(l->l_text, fp);
	(void) putc('\n', fp);
    }
    return(n);
}

/*
 * Returns the number of lines read, or -1 for failure.
 */
static long
p_read(fp)
FILE	*fp;
{
    Line	*lptr = NULL;	/* pointer to list of lines */
    Line	*last = NULL;	/* last complete line read in */
    Line	*lp;		/* line currently being read in */
    register enum {
	at_soln,
	in_line,
	at_eoln,
	at_eof
    }	state;
    char	*buff;		/* text of line being read in */
    int		col;		/* current column in line */
    unsigned long
		nlines;		/* number of lines read */

    col = 0;
    nlines = 0;
    state = at_soln;
    while (state != at_eof) {

	register int	c;

	c = getc(fp);

	/*
	 * Nulls are special; they can't show up in the file.
	 */
	if (c == '\0') {
	    continue;
	}

	if (c == EOF) {
	    if (state != at_soln) {
		/*
		 * Reached EOF in the middle of a line; what
		 * we do here is to pretend we got a properly
		 * terminated line, and assume that a
		 * subsequent getc will still return EOF.
		 */
		state = at_eoln;
	    } else {
		state = at_eof;
	    }
	} else {
	    if (state == at_soln) {
		/*
		 * We're at the start of a line, &
		 * we've got at least one character,
		 * so we have to allocate a new Line
		 * structure.
		 *
		 * If we can't do it, we throw away
		 * the lines we've read in so far, &
		 * return gf_NOMEM.
		 */
		lp = newline(MAX_LINE_LENGTH);
		if (lp == NULL) {
		    if (lptr != NULL)
			throw(lptr);
		    return(-1);
		} else {
		    buff = lp->l_text;
		}
	    }

	    if (c == '\n') {
		state = at_eoln;
	    }
	}

	/*
	 * Fake eoln for lines which are too long.
	 * Don't lose the input character.
	 */
	if (col >= MAX_LINE_LENGTH - 1) {
	    (void) ungetc(c, fp);
	    state = at_eoln;
	}

	switch (state) {
    /*
     *	case at_eof:
     *		break;
     */

	case at_soln:
	case in_line:
	    state = in_line;
	    buff[col++] = c;
	    break;

	case at_eoln:
	    /*
	     * First null-terminate the old line.
	     */
	    buff[col] = '\0';

	    /*
	     * If this fails, we squeak at the user and
	     * then throw away the lines read in so far.
	     */
	    buff = realloc(buff, (unsigned) col + 1);
	    if (buff == NULL) {
		if (lptr != NULL)
		    throw(lptr);
		return(-1);
	    }
	    lp->l_text = buff;
	    lp->l_size = col + 1;

	    /*
	     * Tack the line onto the end of the list,
	     * and then point "last" at it.
	     */
	    if (lptr == NULL) {
		lptr = lp;
		last = lptr;
	    } else {
		last->l_next = lp;
		lp->l_prev = last;
		last = lp;
	    }

	    nlines++;
	    col = 0;
	    state = at_soln;
	    break;
	}
    }

    newlines = lptr;

    return(nlines);
}
