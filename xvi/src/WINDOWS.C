/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)windows.c	2.2 (Chris & John Downey) 8/28/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    windows.c
* module function:
    Window handling functions.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

#undef	min
#define	min(a, b)	(((a) < (b)) ? (a) : (b))

static	int	nwindows = 0;

static	Xviwin	*new_window P((Xviwin *, Xviwin *));
static	bool_t	setup_window P((Xviwin *));

Xviwin *
init_window(vs)
VirtScr	*vs;
{
    Xviwin	*newwin;

    newwin = new_window((Xviwin *) NULL, (Xviwin *) NULL);
    if (newwin == NULL) {
	return(NULL);
    }

    newwin->w_vs = vs;
    newwin->w_nrows = (*vs->v_rows)(vs);
    newwin->w_ncols = (*vs->v_cols)(vs);

    /*
     * Initialise screen stuff.
     */
    init_screen(newwin);

    newwin->w_winpos = 0;
    newwin->w_cmdline = Rows - 1;

    return(newwin);
}

/*
 * Split the given window in half, placing a new empty
 * window in the bottom section and resizing the old one
 * in the top half.
 */
Xviwin *
split_window(oldwin)
Xviwin	*oldwin;
{
    Xviwin	*newwin;

    /*
     * Make sure there are enough rows for the new window.
     * This does not obey the minrows parameter, because
     * the point is to have enough space to actually display
     * a window, not just to have a zero-size one.
     */
    if (oldwin->w_nrows < (MINROWS * 2))
	return(NULL);

    newwin = new_window(oldwin, oldwin->w_next);
    if (newwin == NULL) {
	return(NULL);
    }

    newwin->w_vs = oldwin->w_vs;

    /*
     * Calculate size and position of new and old windows.
     */
    newwin->w_nrows = oldwin->w_nrows / 2;
    newwin->w_cmdline = oldwin->w_cmdline;
    newwin->w_winpos = (newwin->w_cmdline - newwin->w_nrows) + 1;

    oldwin->w_nrows -= newwin->w_nrows;
    oldwin->w_cmdline = newwin->w_winpos - 1;

    newwin->w_ncols = oldwin->w_ncols;

    return(newwin);
}

/*
 * Delete the given window.
 */
void
free_window(window)
Xviwin	*window;
{
    if (window == NULL || nwindows < 1)
	return;

    if (window->w_next != NULL) {
	window->w_next->w_last = window->w_last;
    }
    if (window->w_last != NULL) {
	window->w_last->w_next = window->w_next;
    }
    nwindows -= 1;

    window->w_buffer->b_nwindows -= 1;

    free((char *) window->w_cursor);
    flexdelete(&window->w_statusline);
    free((char *) window);
}

/*
 * Allocate a new window.
 */
static Xviwin *
new_window(last, next)
Xviwin	*last, *next;
{
    Xviwin	*newwin;

    newwin = (Xviwin *) malloc(sizeof(Xviwin));
    if (newwin == NULL) {
	return(NULL);
    }

    if (setup_window(newwin) == FALSE) {
	free((char *) newwin);
	return(NULL);
    }

    /*
     * Link the window into the list.
     */
    if (last != NULL) {
	last->w_next = newwin;
    }
    if (next != NULL) {
	next->w_last = newwin;
    }
    newwin->w_last = last;
    newwin->w_next = next;
    nwindows += 1;

    return(newwin);
}

/*
 * Set up and allocate data structures for the given window,
 * assumed to contain a valid pointer to a buffer.
 *
 * This routine should be called after setup_buffer().
 */
static bool_t
setup_window(w)
Xviwin	*w;
{
    /*
     * Allocate space for the status line.
     */
    flexnew(&w->w_statusline);

    /*
     * Allocate a Posn structure for the cursor.
     */
    w->w_cursor = (Posn *) malloc(sizeof(Posn));
    if (w->w_cursor == NULL) {
	return(FALSE);
    }

    return(TRUE);
}

void
map_window_onto_buffer(w, b)
Xviwin	*w;
Buffer	*b;
{
    /*
     * Connect the two together.
     */
    w->w_buffer = b;
    b->b_nwindows += 1;

    /*
     * Put the cursor and the screen in the right place.
     */
    move_cursor(w, b->b_file, 0);
    w->w_topline = b->b_file;
    w->w_botline = b->b_lastline;

    /*
     * Miscellany.
     */
    w->w_row = w->w_col = 0;
    w->w_virtcol = 0;
    w->w_curswant = 0;
    w->w_set_want_col = FALSE;
    w->w_curs_new = TRUE;
}

/*
 * Unmap the given window from its buffer.
 * We don't need to do much here, on the assumption that the
 * calling code is going to do a map_window_onto_buffer()
 * immediately afterwards; the vital thing is to decrement
 * the window reference count.
 */
void
unmap_window(w)
Xviwin	*w;
{
    w->w_buffer->b_nwindows -= 1;

    w->w_cursor->p_line = NULL;
    w->w_topline = NULL;
    w->w_botline = NULL;
}

/*
 * Given a window, find the "next" one in the list.
 */
Xviwin *
next_window(window)
Xviwin	*window;
{
    if (window == NULL) {
	return(NULL);
    } else if (window->w_next != NULL) {
	return(window->w_next);
    } else {
	Xviwin	*tmp;

	/*
	 * No next window; go to start of list.
	 */
	for (tmp = window; tmp->w_last != NULL; tmp = tmp->w_last)
	    ;
	return(tmp);
    }
}

/*
 * Find the next window onto the buffer with the given filename,
 * starting at the current one; if there isn't one, or if it is
 * too small to move into, return NULL.
 */
Xviwin *
find_window(window, filename)
Xviwin	*window;
char	*filename;
{
    Xviwin	*wp;
    char	*f;

    if (window != NULL && filename != NULL) {
	wp = window;
	do {
	    f = wp->w_buffer->b_filename;
	    if (f != NULL && strcmp(filename, f) == 0) {
		return(wp);
	    }
	    wp = next_window(wp);
	} while (wp != window);
    }

    return(NULL);
}

/*
 * Grow or shrink the given buffer window by "nlines" lines.
 * We prefer to move the bottom of the window, and will only
 * move the top when there is no room for manoeuvre below
 * the current one - i.e. any windows are at minimum size.
 */
void
resize_window(window, nlines)
Xviwin	*window;
int	nlines;
{
    unsigned	savecho;

    if (nlines == 0 || nwindows == 1) {
	/*
	 * Nothing to do.
	 */
	return;
    }

    savecho = echo;

    if (nlines < 0) {
	int	spare;		/* num spare lines in this window */

	nlines = - nlines;

	/*
	 * The current window must always contain 2 rows,
	 * so that the cursor has somewhere to go.
	 */
	spare = window->w_nrows - MINROWS;

	/*
	 * If the window is already as small as it
	 * can get, don't bother to do anything.
	 */
	if (spare <= 0)
	    return;

	/*
	 * Don't allow any screen updating until we've
	 * finished moving things around.
	 */
	echo &= ~e_CHARUPDATE;

	/*
	 * First shrink the current window up from the bottom.
	 *
	 * move_sline()'s return value should be negative or 0
	 * in this case.
	 */
	nlines += move_sline(window, - min(spare, nlines));

	/*
	 * If that wasn't enough, grow the window above us
	 * by the appropriate number of lines.
	 */
	if (nlines > 0) {
	    (void) move_sline(window->w_last, nlines);
	}
    } else {
	/*
	 * Don't allow any screen updating until we've
	 * finished moving things around.
	 */
	echo &= ~e_CHARUPDATE;

	/*
	 * Expand window.
	 */
	nlines -= move_sline(window, nlines);
	if (nlines > 0) {
	    (void) move_sline(window->w_last, -nlines);
	}
    }

    /*
     * Update screen. Note that status lines have
     * already been updated by move_sline().
     *
     * This still needs a lot more optimization.
     */
    echo = savecho;
    update_all();
}

/*
 * Adjust the boundary between two adjacent windows by moving the status line
 * up or down, updating parameters for both windows as appropriate.
 *
 * Note that this can shrink the window to size 0.
 */
int
move_sline(wp, nlines)
Xviwin	*wp;		/* window whose status line we have to move */
int	nlines;		/*
			 * number of lines to move (negative for
			 * upward moves, positive for downwards)
			 */
{
    Xviwin	*nextwin;

    if (wp == NULL || (nextwin = wp->w_next) == NULL) {
	return(0);
    }

    if (nlines < 0) {		/* move upwards */
	int	amount;
	int	spare;

	amount = -nlines;
	spare = wp->w_nrows - Pn(P_minrows);

	if (amount > spare && wp->w_last != NULL) {
	    /*
	     * Not enough space: call move_sline() recursively
	     * for previous line; note that the second parameter
	     * should be negative.
	     */
	    (void) move_sline(wp->w_last, spare - amount);
	    spare = wp->w_nrows - Pn(P_minrows);
	}
	if (amount > spare)
	    amount = spare;
	if (amount != 0) {
	    wp->w_nrows -= amount;
	    wp->w_cmdline -= amount;
	    nextwin->w_winpos -= amount;
	    nextwin->w_nrows += amount;
	    (void) shiftdown(nextwin, (unsigned) amount);
	    if (wp->w_nrows > 0) {
		show_file_info(wp);
	    }
	}
	nlines = -amount;	/* return value */
    } else {			/* move downwards */
	int	spare;

	spare = nextwin->w_nrows - Pn(P_minrows);

	if (nlines > spare) {
	    /*
	     * Not enough space: call move_sline()
	     * recursively for next line.
	     */
	    (void) move_sline(nextwin, nlines - spare);
	    spare = nextwin->w_nrows - Pn(P_minrows);
	}
	if (nlines > spare)
	    nlines = spare;
	if (nlines != 0) {
	    wp->w_nrows += nlines;
	    wp->w_cmdline += nlines;
	    nextwin->w_winpos += nlines;
	    nextwin->w_nrows -= nlines;
	    (void) shiftup(nextwin, (unsigned) nlines);
	    if (wp->w_nrows > 0) {
		show_file_info(wp);
	    }
	}
    }
    return(nlines);
}

/*
 * Update all windows associated with the given buffer.
 */
void
update_buffer(buffer)
Buffer	*buffer;
{
    Xviwin	*w;
	    
    w = curwin;		/* as good a place as any to start */
    do {
	if (w->w_buffer == buffer) {
	    update_window(w);
	}
	w = next_window(w);
    } while (w != curwin);
}

bool_t
can_split()
{
    return(nwindows < Pn(P_autosplit));
}
