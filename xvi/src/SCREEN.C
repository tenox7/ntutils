/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)screen.c	2.3 (Chris & John Downey) 9/4/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    screen.c
* module function:
    Screen handling functions.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

/*
 * Size of command buffer - we won't allow anything more
 * to be typed when we get to this limit.
 */
#define	CMDSZ	80

/*
 * The following is used to optimise screen updating; we
 * keep a record of the real screen state and compare it
 * with the new version before actually doing any updating.
 *
 * The l_line part is guaranteed to be always null-terminated.
 */
typedef	struct	line_struct	{
    char		*l_line;	/* storage for characters in line */
    int			l_used;		/* number of bytes actually used */
    unsigned int	l_flags;	/* information bits */
} Sline;

/*
 * Bit definitions for l_flags.
 */
#define	L_TEXT		0x01		/* is an ordinary text line */
#define	L_MARKER	0x02		/* is a marker line ('@' or '~') */
#define	L_DIRTY		0x04		/* has been modified */
#define	L_MESSAGE	0x08		/* is a message line */
#define	L_COMMAND	0x10		/* is a command line */
#define	L_READONLY	0x20		/* message line for readonly buffer */

#define	L_STATUS	(L_MESSAGE | L_COMMAND)		/* is a status line */

static	Sline	*new_screen;		/* screen being updated */
static	Sline	*real_screen;		/* state of real screen */

/*
 * Status line glitch handling.
 *
 * Some terminals leave a space when changing colour. The number of spaces
 * left is returned by the v_colour_cost() method within the VirtScr, and
 * stored in the colour_cost variable herein - this is not perfect, it should
 * really be in the Xviwin structure, but what the hell.
 *
 * "st_spare_cols" is the number of columns which are not used at the
 * end of the status line; this is to prevent wrapping on this line,
 * as this can do strange things to some terminals.
 */

static	int	colour_cost = 0;
static	int	st_spare_cols = 1;

static	int	line_to_new P((Xviwin *, Line *, int, long));
static	void	file_to_new P((Xviwin *));
static	void	new_to_screen P((VirtScr *, int, int));
static	void	do_sline P((Xviwin *));
static	void	clrline P((int));

/*
 * This routine must be called to set up the screen memory -
 * if it is not, we will probably get a core dump.
 *
 * Note that, at the moment, it must be called with a whole-screen
 * window, i.e. the first window, and only that window, so that the
 * nrows and ncols fields represent the whole screen.
 */
/*ARGSUSED*/
void
init_screen(win)
Xviwin	*win;
{
    static char		*real_area, *new_area;
    register int	count;
    VirtScr		*vs;

    vs = win->w_vs;

    colour_cost = VScolour_cost(vs);
    st_spare_cols = 1 + (colour_cost * 2);

    /*
     * If we're changing the size of the screen, free the old stuff.
     */
    if (real_screen != NULL) {
	free((char *) real_screen);
	free((char *) new_screen);
	free(real_area);
	free(new_area);
    }

    /*
     * Allocate space for the lines, and for the structure holding
     * information about each line. Notice that we allocate an
     * extra byte at the end of each line for null termination.
     */
    real_screen = (Sline *) malloc((unsigned) VSrows(vs) * sizeof(Sline));
    new_screen = (Sline *) malloc((unsigned) VSrows(vs) * sizeof(Sline));
    real_area = malloc((unsigned) VSrows(vs) * (VScols(vs) + 1));
    new_area = malloc((unsigned) VSrows(vs) * (VScols(vs) + 1));

    if (real_screen == NULL || new_screen == NULL ||
	real_area == NULL || new_area == NULL) {
	/* What to do now? */
	sys_exit(0);
    }

    /*
     * Now assign all the rows ...
     */
    for (count = 0; count < VSrows(vs); count++) {
	register Sline	*rp, *np;
	register int	offset;

	rp = &real_screen[count];
	np = &new_screen[count];

	offset = count * (VScols(vs) + 1);

	rp->l_line = real_area + offset;
	np->l_line = new_area + offset;
	rp->l_line[0] = np->l_line[0] = '\0';
	rp->l_used = np->l_used = 0;
	rp->l_flags = np->l_flags = 0;
    }
}

/*
 * Set the L_DIRTY bit for a given line in both real_screen &
 * new_screen if the stored representations are in fact different:
 * otherwise clear it.
 */
static void
mark_dirty(row)
    int		row;
{
    Sline	*rp;
    Sline	*np;
    int		used;

    rp = &real_screen[row];
    np = &new_screen[row];
    if (

	(rp->l_flags & ~L_DIRTY) != (np->l_flags & ~L_DIRTY)
	||
	(used = rp->l_used) != np->l_used
	||
	strncmp(rp->l_line, np->l_line, used) != 0
    ) {
	/*
	 * The lines are different.
	 */
	np->l_flags |= L_DIRTY;
	rp->l_flags |= L_DIRTY;
    } else {
	rp->l_flags = (np->l_flags &= ~L_DIRTY);
    }
}

/*
 * Transfer the specified window line into the "new" screen array, at
 * the given row. Returns the number of screen lines taken up by the
 * logical buffer line lp, or 0 if the line would not fit; this happens
 * with longlines at the end of the screen. In this case, the lines
 * which could not be displayed will have been marked with an '@'.
 */
static int
line_to_new(window, lp, start_row, line)
Xviwin		*window;
Line		*lp;
int		start_row;
long		line;
{
    register unsigned	c;	/* next character from file */
    register Sline	*curr_line;	/* output line - used for efficiency */
    register char	*ltext;		/* pointer to text of line */
    register int	curr_index;	/* current index in line */
    bool_t		eoln;		/* true when line is done */
    char		extra[MAX_TABSTOP];
					/* Stack for extra characters. */
    int			nextra = 0;	/* index into stack */
    int			srow, scol;	/* current screen row and column */
    int			vcol;		/* virtual column */

    ltext = lp->l_text;
    srow = start_row;
    scol = vcol = 0;
    curr_line = &new_screen[srow];
    curr_index = 0;
    eoln = FALSE;

    if (Pb(P_number)) {
	static Flexbuf	ftmp;

	flexclear(&ftmp);
	(void) lformat(&ftmp, NUM_FMT, line);
	(void) strcpy(curr_line->l_line, flexgetstr(&ftmp));
	scol += NUM_SIZE;
    }

    while (!eoln) {
	/*
	 * Get the next character to put on the screen.
	 */

	/*
	 * "extra" is a stack containing any extra characters
	 * we have to put on the screen - this is for chars
	 * which have a multi-character representation, and
	 * for the $ at end-of-line in list mode.
	 */

	if (nextra > 0) {
	    c = extra[--nextra];
	} else {
	    unsigned	n;

	    c = (unsigned char) (ltext[curr_index++]);

	    /*
	     * Deal with situations where it is not
	     * appropriate just to copy characters
	     * straight onto the screen.
	     */
	    if (c == '\0') {

		if (Pb(P_list)) {
		    /*
		     * Have to show a '$' sign in list mode.
		     */
		    extra[nextra++] = '\0';
		    c = '$';
		}

	    } else {
		char	*p;

		n = vischar((int) c, &p, vcol);
		/*
		 * This is a bit paranoid assuming
		 * that Pn(P_tabstop) can never be
		 * greater than sizeof (extra), but
		 * so what.
		 */
		if (nextra + n > sizeof extra)
		    n = (sizeof extra - nextra);
		/*
		 * Stack the extra characters so that
		 * they appear in the right order.
		 */
		while (n > 1) {
		    extra[nextra++] = p[--n];
		}
		c = p[0];
	    }
	}

	if (c == '\0') {
	    /*
	     * End of line. Terminate it and finish.
	     */
	    eoln = TRUE;
	    curr_line->l_flags = L_TEXT;
	    curr_line->l_used = scol;
	    curr_line->l_line[scol] = '\0';
	    mark_dirty(srow);
	    break;
	} else {
	    /*
	     * Sline folding.
	     */
	    if (scol >= window->w_ncols) {
		curr_line->l_flags = L_TEXT;
		curr_line->l_used = scol;
		curr_line->l_line[scol] = '\0';
		mark_dirty(srow);
		srow += 1;
		scol = 0;
		curr_line = &new_screen[srow];
	    }

	    if (srow >= window->w_cmdline) {
		for (srow = start_row; srow < window->w_cmdline; srow++) {
		    curr_line = &new_screen[srow];

		    curr_line->l_flags = L_MARKER;
		    curr_line->l_used = 1;
		    curr_line->l_line[0] = '@';
		    curr_line->l_line[1] = '\0';
		    mark_dirty(srow);
		}
		return(0);
	    }

	    /*
	     * Store the character in new_screen.
	     */
	    curr_line->l_line[scol++] = c;
	    vcol++;
	}
    }

    return((srow - start_row) + 1);
}

/*
 * file_to_new()
 *
 * Based on the current value of topline, transfer a screenful
 * of stuff from file to new_screen, and update botline.
 */
static void
file_to_new(win)
register Xviwin	*win;
{
    register int	row;
    register Line	*line;
    register Buffer	*buffer;
    long		lnum;

    buffer = win->w_buffer;
    row = win->w_winpos;
    line = win->w_topline;
    lnum = lineno(buffer, line);

    while (row < win->w_cmdline && line != buffer->b_lastline) {
	int nlines;

	nlines = line_to_new(win, line, row, lnum);
	if (nlines == 0) {
	    /*
	     * Make it look like we have updated
	     * all the screen lines, since they
	     * have '@' signs on them.
	     */
	    row = win->w_cmdline;
	    break;
	} else {
	    row += nlines;
	    line = line->l_next;
	    lnum++;
	}
    }

    win->w_botline = line;

    /*
     * If there are any lines remaining, fill them in
     * with '~' characters.
     */
    for ( ; row < win->w_cmdline; row++) {
	register Sline	*curr_line;

	curr_line = &new_screen[row];

	curr_line->l_flags = L_MARKER;
	curr_line->l_used = 1;
	curr_line->l_line[0] = '~';
	curr_line->l_line[1] = '\0';
	mark_dirty(row);
    }
}

/*
 * new_to_screen
 *
 * Transfer the contents of new_screen to the screen,
 * starting at "start_row", for "nlines" lines,
 * using real_screen to avoid unnecessary output.
 */
static void
new_to_screen(vs, start_row, nlines)
VirtScr		*vs;
int		start_row;
int		nlines;
{
    int         row;                    /* current row */
    int         end_row;                /* row after last one to be updated */
    int         columns;

    columns = VScols(vs);

    if (!(echo & e_CHARUPDATE)) {
	return;
    }

    end_row = start_row + nlines;

    VSset_colour(vs, Pn(P_colour));

    for (row = start_row; row < end_row; row++) {
	register int            ncol;   /* current column in new_screen */
	register Sline          *new,
				*real;  /* pointers to current lines */
	register unsigned       nflags;
	unsigned                rflags; /* flags for current lines */
	register char           *ntextp,
				*rtextp;
					/* pointers to line text */
	register int            nc;     /* current character in new_screen */
	int			n_used,
				r_used;

	nflags = (new = &new_screen[row])->l_flags;
	rflags = (real = &real_screen[row])->l_flags;

	/*
	 * If the real and new screens are both "clean",
	 * don't bother.
	 */
	if (!((nflags & L_DIRTY) || (rflags & L_DIRTY))) {
	    continue;
	}

	ntextp = new->l_line;
	rtextp = real->l_line;

	n_used = new->l_used;
	r_used = real->l_used;

	if ((nflags & L_MESSAGE) ||
				(rflags & L_STATUS) != (nflags & L_STATUS)) {
	    /*
	     * If it's a message line, or its status (text line,
	     * command line or message line) has changed, and either
	     * the real line or the new line is "dirty", better update
	     * the whole thing; if any colour changes are required,
	     * the effects of cursor movements may not be predictable
	     * on some terminals.
	     */
	    VSgoto(vs, row, 0);
	    if (nflags & L_STATUS) {
		VSset_colour(vs, (nflags & L_READONLY) ? Pn(P_roscolour) :
						Pn(P_statuscolour));
	    }
	    if ((nc = ntextp[0]) != '\0') {
		VSputc(vs, row, 0, nc);
	    }
	    /*
	     * For command lines, only the first character should be
	     * highlighted.
	     */
	    if (nflags & L_COMMAND) {
		VSset_colour(vs, Pn(P_colour));
	    }
	    if (nc != '\0') {
		VSwrite(vs, row, 1, &ntextp[1]);
	    }

	    /*
	     * Clear the rest of the line, if
	     * there is any left to be cleared.
	     */
	    if (n_used < columns) {
		VSclear_line(vs, row, n_used);
	    }

	    /*
	     * Change back to text colour if we have to.
	     */
	    if ((nflags & L_MESSAGE) != 0) {
		VSset_colour(vs, Pn(P_colour));
	    }

	    (void) strncpy(rtextp, ntextp, (int) (columns - st_spare_cols));
	} else {
	    /*
	     * Look at each character in the line, comparing
	     * the new version with the one on the screen.
	     * If they differ, put it out.
	     *
	     * There is some optimisation here to avoid large
	     * use of tty_goto.
	     */
	    register int        scol;
				/* current column on physical screen */
	    register int        last_ncol;
				/* last column to be updated */

	    for (ncol = scol = last_ncol = 0;
			     ncol < n_used && ncol < r_used;
			     (ncol++, scol++)) {
		if ((nc = ntextp[ncol]) != rtextp[ncol]) {
		    /*
		     * They are different. Get to the right
		     * place before putting out the char.
		     */
		    if (ncol != 0) {
			VSadvise(vs, row, last_ncol + 1,
					    ncol - last_ncol - 1,
					    ntextp + last_ncol + 1);
		    } else {
			VSgoto(vs, row, scol);
			/*
			 * A command line should have the first character
			 * - and only the first character - highlighted.
			 */
			if (ncol == 0 && (nflags & L_STATUS) != 0) {
			    VSset_colour(vs, (nflags & L_READONLY) ?
				    Pn(P_roscolour) : Pn(P_statuscolour));
			}
		    }

		    VSputc(vs, row, ncol, nc);

		    if (ncol == 0 && (nflags & L_COMMAND) != 0) {
			VSset_colour(vs, Pn(P_colour));
		    }
		    last_ncol = ncol;
		    rtextp[ncol] = nc;
		}
		if (ncol == 0 && (nflags & L_COMMAND) != 0) {
		    scol += (colour_cost * 2);
		}
	    }

	    if (n_used > r_used) {
		/*
		 * We have got to the end of the previous
		 * screen line; if there is anything left,
		 * we should just display it.
		 */
		(void) strcpy(&rtextp[ncol], &ntextp[ncol]);
		if (ncol == 0 && (nflags & L_COMMAND) != 0) {
		    /*
		     * A command line should have the first character
		     * - and only the first character - highlighted.
		     */
		    VSgoto(vs, row, 0);
		    VSset_colour(vs, Pn(P_statuscolour));
		    VSputc(vs, row, 0, ntextp[0]);
		    VSset_colour(vs, Pn(P_colour));
		    ncol = 1;
		} else {
		    /*
		     * Skip over initial whitespace.
		     */
		    while (ntextp[ncol] == ' ') {
			ncol++;
			scol++;
		    }
		}
		if (ncol < columns)
		    VSwrite(vs, row, scol, &ntextp[ncol]);
	    } else if (r_used > n_used) {
		/*
		 * We have got to the end of the new screen
		 * line, but the old one still has stuff on
		 * it. We must therefore clear it.
		 */
		VSclear_line(vs, row, scol);
	    }
	}

	real->l_line[n_used] = '\0';
	real->l_used = n_used;

	/*
	 * The real screen line is a message or command line if the
	 * newly-updated one was. Otherwise, it isn't.
	 *
	 * Both the new and real screens may now be considered
	 * "clean".
	 */
	real->l_flags = (
			  /*
			   * Turn these flags off first ...
			   */
			  (rflags & ~(L_STATUS | L_DIRTY))
			  /*
			   * ... then set whatever L_STATUS flags are
			   * set in new_screen.
			   */
			  | (nflags & L_STATUS)
			);
	new->l_flags &= ~L_DIRTY;
    }
}

/*
 * Update the status line of the given window, and cause the status
 * line to be written out. Note that we call new_to_screen() to cause
 * the output to be generated; since there will be no other changes,
 * only the status line will be changed on the screen.
 */
void
update_sline(win)
Xviwin	*win;
{
    do_sline(win);
    new_to_screen(win->w_vs, (int) win->w_cmdline, 1);
    VSflush(win->w_vs);
}

/*
 * Update the status line of the given window,
 * from the one in win->w_statusline.
 */
static void
do_sline(win)
Xviwin	*win;
{
    register char	*from;
    register char	*to;
    register char	*end;
    Sline		*slp;

    from = flexgetstr(&win->w_statusline);
    slp = &new_screen[win->w_cmdline];
    to = slp->l_line;
    end = to + win->w_ncols - st_spare_cols;

    while (*from != '\0' && to < end) {
	*to++ = *from++;
    }

    /*
     * Fill with spaces, and null-terminate.
     */
    while (to < end) {
	*to++ = ' ';
    }
    *to = '\0';

    slp->l_used = win->w_ncols - st_spare_cols;
    slp->l_flags = L_MESSAGE;
    if (is_readonly(win->w_buffer)) {
	slp->l_flags |= L_READONLY;
    }
    mark_dirty(win->w_cmdline);
}

void
update_cline(win)
Xviwin	*win;
{
    Sline	*clp;
    unsigned width, maxwidth;

    clp = &new_screen[win->w_cmdline];

    maxwidth = win->w_ncols - st_spare_cols;
    if ((width = flexlen(&win->w_statusline)) > maxwidth) {
	width = maxwidth;
    }
    (void) strncpy(clp->l_line, flexgetstr(&win->w_statusline),
			    (int) width);
    clp->l_used = width;
    clp->l_line[width] = '\0';
    clp->l_flags = (L_COMMAND | L_DIRTY);
    /*
     * We don't bother calling mark_dirty() here: it isn't worth
     * it because the line's contents have almost certainly
     * changed.
     */
    new_to_screen(win->w_vs, (int) win->w_cmdline, 1);
    VSflush(win->w_vs);
}

/*
 * updateline() - update the line the cursor is on
 *
 * Updateline() is called after changes that only affect the line that
 * the cursor is on. This improves performance tremendously for normal
 * insert mode operation. The only thing we have to watch for is when
 * the cursor line grows or shrinks around a row boundary. This means
 * we have to repaint other parts of the screen appropriately.
 */
void
updateline(window)
Xviwin	*window;
{
    Line	*currline;
    int		nlines;
    int		curs_row;

    currline = window->w_cursor->p_line;

    /*
     * Find out which screen line the cursor line starts on.
     * This is not necessarily the same as window->w_row,
     * because longlines are different.
     */
    if (plines(window, currline) > 1) {
	curs_row = (int) cntplines(window, window->w_topline, currline);
    } else {
	curs_row = window->w_row;
    }

    nlines = line_to_new(window, currline,
			(int) (curs_row + window->w_winpos),
			(long) lineno(window->w_buffer, currline));

    if (nlines != window->w_c_line_size) {
	update_buffer(window->w_buffer);
    } else {
	new_to_screen(window->w_vs,
			(int) (curs_row + window->w_winpos), nlines);
	VSflush(window->w_vs);
    }
}

/*
 * Completely update the representation of the given window.
 */
void
update_window(window)
Xviwin	*window;
{
    if (window->w_nrows > 1) {
	file_to_new(window);
	new_to_screen(window->w_vs,
			(int) window->w_winpos, (int) window->w_nrows);
	VSflush(window->w_vs);
    }
}

/*
 * Update all windows.
 */
void
update_all()
{
    Xviwin	*w = curwin;

    do {
	if (w->w_nrows > 1) {
	    file_to_new(w);
	}
	if (w->w_nrows > 0) {
	    do_sline(w);
	}
    } while ((w = next_window(w)) != curwin);

    new_to_screen(w->w_vs, 0, (int) VSrows(w->w_vs));
    VSflush(w->w_vs);
}

/*
 * Totally redraw the screen.
 */
void
redraw_screen()
{
    if (curwin != NULL) {
	clear(curwin);
	update_all();
    }
}

void
clear(win)
Xviwin	*win;
{
    register int	row;
    int		nrows;

    nrows = VSrows(win->w_vs);

    VSset_colour(win->w_vs, Pn(P_colour));
    VSclear_all(win->w_vs);

    /*
     * Clear the real screen lines, and mark them as modified.
     */
    for (row = 0; row < nrows; row++) {
	clrline(row);
    }
}

/*
 * The rest of the routines in this file perform screen manipulations.
 * The given operation is performed physically on the screen. The
 * corresponding change is also made to the internal screen image. In
 * this way, the editor anticipates the effect of editing changes on
 * the appearance of the screen. That way, when we call screenupdate a
 * complete redraw isn't usually necessary. Another advantage is that
 * we can keep adding code to anticipate screen changes, and in the
 * meantime, everything still works.
 */

/*
 * s_ins(win, row, nlines) - insert 'nlines' lines at 'row'
 */
void
s_ins(win, row, nlines)
Xviwin		*win;
register int	row;
int		nlines;
{
    register int	from, to;
    int			count;
    VirtScr		*vs;

    if (!(echo & e_SCROLL))
	return;

    /*
     * There's no point in scrolling more lines than there are
     * (below row) in the window, or in scrolling 0 lines.
     */
    if (nlines == 0 || nlines + row >= win->w_nrows - 1)
	return;

    /*
     * The row specified is relative to the top of the window;
     * add the appropriate offset to make it into a screen row.
     */
    row += win->w_winpos;

    /*
     * Note that we avoid the use of 1-line scroll regions; these
     * only ever occur at the bottom of a window, and it is better
     * just to leave the line to be updated in the best way by
     * update{line,screen}.
     */
    if (nlines == 1 && row + 1 == win->w_cmdline) {
	return;
    }

    vs = win->w_vs;

    if (vs->v_scroll != NULL) {
	if (!VSscroll(vs, row, (int) win->w_cmdline - 1, -nlines)) {
	    /*
	     * Can't scroll what we were asked to - try scrolling
	     * the whole window including the status line.
	     */
	    VSclear_line(vs, (int) win->w_cmdline, 0);
	    clrline(win->w_cmdline);
	    if (!VSscroll(vs, row, (int) win->w_cmdline, -nlines)) {
		/*
		 * Failed.
		 */
		return;
	    }
	}
    } else {
	return;
    }

    /*
     * Update the stored screen image so it matches what has
     * happened on the screen.
     */

    /*
     * Move section of text down to the bottom.
     *
     * We do this by rearranging the pointers within the Slines,
     * rather than copying the characters.
     */
    for (to = win->w_cmdline - 1, from = to - nlines; from >= row;
							--from, --to) {
	register char	*temp;

	temp = real_screen[to].l_line;
	real_screen[to].l_line = real_screen[from].l_line;
	real_screen[from].l_line = temp;
	real_screen[to].l_used = real_screen[from].l_used;
    }

    /*
     * Clear the newly inserted lines.
     */
    for (count = row; count < row + nlines; count++) {
	clrline(count);
    }
}

/*
 * s_del(win, row, nlines) - delete 'nlines' lines starting at 'row'.
 */
void
s_del(win, row, nlines)
register Xviwin		*win;
int			row;
int			nlines;
{
    register int	from, to;
    int			count;
    VirtScr		*vs;

    if (!(echo & e_SCROLL))
	return;

    /*
     * There's no point in scrolling more lines than there are
     * (below row) in the window, or in scrolling 0 lines.
     */
    if (nlines == 0 || nlines + row >= win->w_nrows - 1)
	return;

    /*
     * The row specified is relative to the top of the window;
     * add the appropriate offset to make it into a screen row.
     */
    row += win->w_winpos;

    /*
     * We avoid the use of 1-line scroll regions, since they don't
     * work with many terminals, especially if we are using
     * (termcap) DO to scroll the region.
     */
    if (nlines == 1 && row + 1 == win->w_cmdline) {
	return;
    }

    vs = win->w_vs;

    if (vs->v_scroll != NULL) {
	if (!VSscroll(vs, row, (int) win->w_cmdline - 1, nlines)) {
	    /*
	     * Can't scroll what we were asked to - try scrolling
	     * the whole window including the status line.
	     */
	    VSclear_line(vs, (int) win->w_cmdline, 0);
	    clrline(win->w_cmdline);
	    if (!VSscroll(vs, row, (int) win->w_cmdline, nlines)) {
		/*
		 * Failed.
		 */
		return;
	    }
	}
    } else {
	return;
    }

    /*
     * Update the stored screen image so it matches what has
     * happened on the screen.
     */

    /*
     * Move section of text up from the bottom.
     *
     * We do this by rearranging the pointers within the Slines,
     * rather than copying the characters.
     */
    for (to = row, from = to + nlines;
	 from < win->w_cmdline;
	 from++, to++) {
	register char	*temp;

	temp = real_screen[to].l_line;
	real_screen[to].l_line = real_screen[from].l_line;
	real_screen[from].l_line = temp;
	real_screen[to].l_used = real_screen[from].l_used;
    }

    /*
     * Clear the deleted lines.
     */
    for (count = win->w_cmdline - nlines; count < win->w_cmdline; count++) {
	clrline(count);
    }
}

/*
 * Insert a character at the cursor position, updating the screen as
 * necessary. Note that this routine doesn't have to do anything, as
 * the screen will eventually be correctly updated anyway; it's just
 * here for speed of screen updating.
 */
void
s_inschar(window, newchar)
Xviwin			*window;
int			newchar;
{
    register char	*curp;
    register char	*cp;
    register char	*sp;
    Sline		*rp;
    Posn		*pp;
    VirtScr		*vs;		/* the VirtScr for this window */
    char		*newstr;	/* printable string for newchar */
    register unsigned	nchars;		/* number of  chars in newstr */
    unsigned		currow;
    unsigned		curcol;
    unsigned		columns;

    vs = window->w_vs;
    if (vs->v_insert == NULL)
	return;

    if (!(echo & e_CHARUPDATE))
	return;

    pp = window->w_cursor;

    /*
     * If we are at (or near) the end of the line, it's not worth
     * the bother. Define near as 0 or 1 characters to be moved.
     */
    cp = pp->p_line->l_text + pp->p_index;
    if (*cp == '\0' || *(cp+1) == '\0')
	return;

    curcol = window->w_col;

    /*
     * If the cursor is on a longline, and not on the last actual
     * screen line of that longline, we can't do it.
     */
    if (window->w_c_line_size > 1 && curcol != window->w_virtcol)
	return;

    nchars = vischar(newchar, &newstr, curcol);

    /*
     * And don't bother if we are (or will be) at the last screen column.
     */
    columns = window->w_ncols;
    if (curcol + nchars >= columns)
	return;

    /*
     * Also, trying to push tabs rightwards doesn't work very
     * well. It's usually better not to use the insert character
     * sequence because in most cases we'll only have to update
     * the line as far as the tab anyway.
     */
    if ((!Pb(P_list) && Pb(P_tabs)) && strchr(cp, '\t') != NULL) {
	return;
    }

    /*
     * Okay, we can do it.
     */
    currow = window->w_row;

    VSinsert(vs, window->w_winpos + currow, curcol, newstr);

    /*
     * Update real_screen.
     */
    rp = &real_screen[window->w_winpos + currow];
    curp = &rp->l_line[curcol];
    if ((rp->l_used += nchars) > columns)
	rp->l_used = columns;
    cp = &rp->l_line[rp->l_used - 1];
    cp[1] = '\0';
    if (cp - curp >= nchars)
    {
	sp = cp - nchars;
	for (;;) {
	    *cp-- = *sp;
	    if (sp-- <= curp)
		break;
	}
    }

    /*
     * This is the string we've just inserted.
     */
    sp = newstr;
    while (nchars-- > 0) {
	*curp++ = *sp++;
    }
}

void
wind_goto(win)
Xviwin	*win;
{
    VirtScr	*vs;

    if (echo & e_CHARUPDATE) {
	vs = win->w_vs;
	VSgoto(vs, (int) win->w_winpos + win->w_row, win->w_col);
	VSflush(vs);
    }
}

static	char		inbuf[CMDSZ];		/* command input buffer */
static	unsigned int	inpos = 0;		/* posn of next input char */
static	unsigned char	colposn[CMDSZ];		/* holds n chars per char */

/*
 * cmd_init(window, firstch)
 *
 * Initialise command line input.
 */
void
cmd_init(win, firstch)
Xviwin	*win;
int	firstch;
{
    if (inpos > 0) {
	show_error(win, "Internal error: re-entered command line input mode");
	return;
    }

    State = CMDLINE;

    flexclear(&win->w_statusline);
    (void) flexaddch(&win->w_statusline, firstch);
    inbuf[0] = firstch;
    inpos = 1;
    update_cline(win);
    colposn[0] = 0;
}

/*
 * cmd_input(window, character)
 *
 * Deal with command line input. Takes an input character and returns
 * one of cmd_CANCEL (meaning they typed ESC or deleted past the
 * prompt character), cmd_COMPLETE (indicating that the command has
 * been completely input), or cmd_INCOMPLETE (indicating that command
 * line is still the right mode to be in).
 *
 * Once cmd_COMPLETE has been returned, it is possible to call
 * get_cmd(win) to obtain the command line.
 */
Cmd_State
cmd_input(win, ch)
Xviwin	*win;
int	ch;
{
    static bool_t	literal_next = FALSE;

    if (!literal_next) {
	switch (ch) {
	case CTRL('V'):
	    literal_next = TRUE;
	    return(cmd_INCOMPLETE);

	case '\n':		/* end of line */
	case '\r':
	    inbuf[inpos] = '\0';	/* terminate input line */
	    inpos = 0;
	    State = NORMAL;		/* return state to normal */
	    do_sline(win);		/* line is now a message line */
	    return(cmd_COMPLETE);	/* and indicate we are done */

	case '\b':		/* backspace or delete */
	case DEL:
	{
	    unsigned len;

	    inbuf[--inpos] = '\0';
	    len = colposn[inpos - 1] + 1;
	    while (flexlen(&win->w_statusline) > len)
		(void) flexrmchar(&win->w_statusline);
	    update_cline(win);
	    if (inpos == 0) {
		/*
		 * Deleted past first char;
		 * go back to normal mode.
		 */
		State = NORMAL;
		return(cmd_CANCEL);
	    }
	    return(cmd_INCOMPLETE);
	}

	case '\033':
	case EOF:
	case CTRL('U'):		/* line kill */
	    inpos = 1;
	    inbuf[1] = '\0';
	    flexclear(&win->w_statusline);
	    (void) flexaddch(&win->w_statusline, inbuf[0]);
	    update_cline(win);
	    return(cmd_INCOMPLETE);

	default:
	    break;
	}
    }

    literal_next = FALSE;

    if (inpos >= sizeof(inbuf) - 1) {
	/*
	 * Must not overflow buffer.
	 */
	beep(win);
    } else {
	unsigned	curposn;
	unsigned	w;
	char		*p;

	curposn = colposn[inpos - 1];
	w = vischar(ch, &p, (int) curposn);
	if (curposn + w >= win->w_ncols - 1) {
	    beep(win);
	} else {
	    colposn[inpos] = curposn + w;
	    inbuf[inpos++] = ch;
	    (void) lformat(&win->w_statusline, "%s", p);
	    update_cline(win);
	}
    }

    return(cmd_INCOMPLETE);
}

/*ARGSUSED*/
char *
get_cmd(win)
Xviwin	*win;
{
    return(inbuf);
}

/*ARGSUSED*/
void
gotocmd(win, clr)
Xviwin	*win;
bool_t	clr;
{
    VirtScr	*vs;

    vs = win->w_vs;
    if (clr) {
	VSclear_line(vs, (int) win->w_cmdline, 0);
    }
    VSgoto(vs, (int) win->w_cmdline, 0);
    VSflush(vs);
}

/*
 * Display a prompt on the bottom line of the screen.
 */
void
prompt(message)
    char	*message;
{
    VirtScr	*vs;
    int	row;

    vs = curwin->w_vs;

    row = VSrows(vs) - 1;
    VSgoto(vs, row, 0);
    VSset_colour(vs, Pn(P_statuscolour));
    VSwrite(vs, row, 0, message);
    VSset_colour(vs, Pn(P_colour));
    VSgoto(vs, row, strlen(message));
    VSflush(vs);
}

/*
 * Sound the alert.
 */
void
beep(window)
register Xviwin *window;
{
    VSbeep(window->w_vs);
}

static char	*(*disp_func) P((void));
static int	disp_colwidth;
static int	disp_maxcol;
static bool_t	disp_listmode;

/*
 * Start off "display" mode. The "func" argument is a function pointer
 * which will be called to obtain each subsequent string to display.
 * The function returns NULL when no more lines are available.
 */
void
disp_init(win, func, colwidth, listmode)
Xviwin		*win;
char		*(*func) P((void));
int		colwidth;
bool_t		listmode;
{
    State = DISPLAY;
    disp_func = func;
    if (colwidth > win->w_ncols)
	colwidth = win->w_ncols;
    disp_colwidth = colwidth;
    disp_maxcol = (win->w_ncols / colwidth) * colwidth;
    disp_listmode = listmode;
    (void) disp_screen(win);
}

/*
 * Display text in glass-teletype mode, in approximately the style of
 * the more(1) program.
 *
 * If the return value from (*disp_func)() is NULL, it means we've got
 * to the end of the text to be displayed, so we wait for another key
 * before redisplaying our editing screen.
 */
bool_t
disp_screen(win)
Xviwin	*win;
{
    int		row;	/* current screen row */
    int		col;	/* current screen column */
    static bool_t	finished = FALSE;
    VirtScr		*vs;

    vs = win->w_vs;

    if (finished || kbdintr) {
	/*
	 * Clear the screen, and then ensure that the window
	 * on the current buffer is in the right place and
	 * updated; finally update the whole screen.
	 */
	clear(win);
	move_window_to_cursor(win);
	update_all();
	State = NORMAL;
	finished = FALSE;
	if (kbdintr) {
	    imessage = TRUE;
	}
	return(TRUE);
    }

    VSclear_all(vs);

    for (col = 0; col < disp_maxcol; col += disp_colwidth) {
	for (row = 0; row < VSrows(vs) - 1; row++) {
	    static char	*line;
	    int		width;

	    if (line == NULL && (line = (*disp_func)()) == NULL) {
		/*
		 * We've got to the end.
		 */
		prompt("[Hit return to continue] ");
		finished = TRUE;
		return(FALSE);
	    }

	    for (width = 0; *line != '\0'; line++) {
		char		*p;
		unsigned	w;

		w = vischar(*line, &p, disp_listmode ? -1 : width);

		if ((width += w) <= disp_colwidth) {
		    VSwrite(vs, row, col + width - w, p);
		} else {
		    /*
		     * The line is too long, so we
		     * have to wrap around to the
		     * next screen line.
		     */
		    break;
		}
	    }

	    if (*line == '\0') {
		if (disp_listmode) {
		    /*
		     * In list mode, we have to
		     * display a '$' to show the
		     * end of a line.
		     */
		    if (width < disp_colwidth) {
			VSputc(vs, row, col + width, '$');
		    } else {
			/*
			 * We have to wrap it
			 * to the next screen
			 * line.
			 */
			continue;
		    }
		}
		/*
		 * If we're not in list mode, or we
		 * were able to display the '$', we've
		 * finished with this line.
		 */
		line = NULL;
	    }
	}
    }

    prompt("[More] ");

    return(FALSE);
}

/*
 * Clear the given line, marking it as dirty.
 */
static void
clrline(line)
int	line;
{
    real_screen[line].l_used = 0;
    real_screen[line].l_line[0] = '\0';
    mark_dirty(line);
}
