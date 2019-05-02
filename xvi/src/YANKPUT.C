/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)yankput.c	2.4 (Chris & John Downey) 8/6/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    yankput.c
* module function:
    Functions to handle "yank" and "put" commands.

    Note that there is still some code in normal.c to do
    some of the work - this will have to be changed later.

    Some of the routines and data structures herein assume ASCII
    order, so I don't know if they are particularly portable.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

/*
 * Structure to store yanked text or yanked lines.
 */
typedef struct yankbuffer {
    enum {
	y_none,
	y_chars,
	y_lines
    } y_type;

    char	*y_1st_text;
    char	*y_2nd_text;
    Line	*y_line_buf;
} Yankbuffer;

/*
 * For named buffers, we have an array of yankbuffer structures,
 * mapped by printable ascii characters. Only the alphabetic
 * characters, and '@', are directly settable by the user.
 * The uppercase versions mean "append" rather than "replace".
 */
#define	LOWEST_NAME	' '
#define	HIGHEST_NAME	'Z'
#define	NBUFS		(HIGHEST_NAME - LOWEST_NAME + 1)
#define	bufno(c)	((c) - LOWEST_NAME)
#define	validname(c)	((c) >= LOWEST_NAME && (c) < 'A')

static	Yankbuffer	yb[NBUFS];

static	void		put P((char *, bool_t, bool_t));
static	Yankbuffer	*yp_get_buffer P((int));
static	Line		*copy_lines P((Line *, Line *));
static	char		*yanktext P((Posn *, Posn *));
static	void		yp_free P((Yankbuffer *));

void
init_yankput()
{
}

/*
 * Set the buffer name to be used for the next yank/put operation to
 * the given character. The character '@' is used as a synonym for the
 * default (unnamed) buffer.
 */
static Yankbuffer *
yp_get_buffer(name)
int	name;
{
    int	i;

    if (validname(name)) {
	i = bufno(name);
    } else if (is_alpha(name)) {
	if (is_upper(name)) {
	    show_message(curwin, "Appending to named buffers not supported");
	}
	i = bufno(to_upper(name));
    } else {
	show_error(curwin, "Illegal buffer name");
	return(NULL);
    }
    return(&yb[i]);
}

/*
 * Yank the text specified by the given start/end positions.
 * The fourth parameter is TRUE if we are doing a character-
 * based, rather than a line-based, yank.
 *
 * For line-based yanks, the range of positions is inclusive.
 *
 * Returns TRUE if successfully yanked.
 *
 * Positions must be ordered properly, i.e. "from" <= "to".
 */
/*ARGSUSED*/
bool_t
do_yank(buffer, from, to, charbased, name)
Buffer	*buffer;
Posn	*from, *to;
bool_t	charbased;
int	name;
{
    Yankbuffer	*yp_buf;
    long	nlines;

    yp_buf = yp_get_buffer(name);
    if (yp_buf == NULL) {
	return(FALSE);
    }
    yp_free(yp_buf);

    nlines = cntllines(from->p_line, to->p_line);

    if (charbased) {
	Posn		ptmp;

	/*
	 * First yank either the whole of the text string
	 * specified (if from and to are on the same line),
	 * or from "from" to the end of the line.
	 */
	ptmp.p_line = from->p_line;
	if (to->p_line == from->p_line) {
	    ptmp.p_index = to->p_index;
	} else {
	    ptmp.p_index = strlen(from->p_line->l_text) - 1;
	}
	yp_buf->y_1st_text = yanktext(from, &ptmp);
	if (yp_buf->y_1st_text == NULL) {
	    return(FALSE);
	}

	/*
	 * Next, determine if it is a multi-line character-based
	 * yank, in which case we have to yank from the start of
	 * the line containing "to" up to "to" itself.
	 */
	if (nlines > 1) {
	    ptmp.p_line = to->p_line;
	    ptmp.p_index = 0;
	    yp_buf->y_2nd_text = yanktext(&ptmp, to);
	    if (yp_buf->y_1st_text == NULL) {
		free(yp_buf->y_1st_text);
		return(FALSE);
	    }
	}

	/*
	 * Finally, we may need to yank any lines between "from"
	 * and "to".
	 */
	if (nlines > 2) {
	    yp_buf->y_line_buf =
		copy_lines(from->p_line->l_next, to->p_line);
	    if (yp_buf->y_line_buf == NULL) {
		free(yp_buf->y_1st_text);
		free(yp_buf->y_2nd_text);
		return(FALSE);
	    }
	}

	yp_buf->y_type = y_chars;
    } else {
	/*
	 * Yank lines starting at "from", ending at "to".
	 */
	yp_buf->y_line_buf = copy_lines(from->p_line,
			to->p_line->l_next);
	if (yp_buf->y_line_buf == NULL) {
	    return(FALSE);
	}
	yp_buf->y_type = y_lines;
    }
    return(TRUE);
}

/*
 * Yank the given string.
 *
 * Third parameter indicates whether to do it as a line or a string.
 *
 * Returns TRUE if successfully yanked.
 */
bool_t
yank_str(name, str, line_based)
int	name;
char	*str;
bool_t	line_based;
{
    Yankbuffer		*yp_buf;
    register Line	*tmp;
    register char	*cp;

    yp_buf = yp_get_buffer(name);
    if (yp_buf == NULL) {
	return(FALSE);
    }
    yp_free(yp_buf);

    /*
     * Obtain space to store the string.
     */
    if (line_based) {
	/*
	 * First try to save the string. If no can do,
	 * return FALSE without affecting the current
	 * contents of the yank buffer.
	 */
	tmp = newline(strlen(str) + 1);
	if (tmp == NULL) {
	    return(FALSE);
	}
	tmp->l_prev = tmp->l_next = NULL;
	(void) strcpy(tmp->l_text, str);
    } else {
    	cp = strsave(str);
	if (cp == NULL) {
	    return(FALSE);
	}
    }

    /*
     * Set up the yank structure.
     */
    if (line_based) {
	yp_buf->y_type = y_lines;
	yp_buf->y_line_buf = tmp;
    } else {
    	yp_buf->y_type = y_chars;
	yp_buf->y_1st_text = cp;
    }

    return(TRUE);
}

/*
 * Put back the last yank at the specified position,
 * in the specified direction.
 */
void
do_put(win, location, direction, name)
Xviwin	*win;
Posn	*location;
int	direction;
int	name;
{
    Yankbuffer		*yp_buf;
    register Line	*currline;	/* line we are on now */
    register Line	*nextline;	/* line after currline */
    Buffer		*buffer;

    yp_buf = yp_get_buffer(name);
    if (yp_buf == NULL) {
	return;
    }

    buffer = win->w_buffer;

    /*
     * Set up current and next line pointers.
     */
    currline = location->p_line;
    nextline = currline->l_next;

    /*
     * See which type of yank it was ...
     */
    if (yp_buf->y_type == y_chars) {
	int	l;

	l = win->w_cursor->p_index;
	if (direction == FORWARD && currline->l_text[l] != '\0') {
	    ++l;
	}

	if (!start_command(win)) {
	    return;
	}

	/*
	 * Firstly, insert the 1st_text buffer, since this is
	 * always present. We may wish to split the line after
	 * the inserted text if this was a multi-line yank.
	 */
	replchars(win, currline, l, 0, yp_buf->y_1st_text);
	updateline(win);

	if (yp_buf->y_2nd_text != NULL) {
	    int	end_of_1st_text;
	    Line	*newl;

	    end_of_1st_text = l + strlen(yp_buf->y_1st_text);
	    newl = newline(strlen(yp_buf->y_1st_text) + SLOP);
	    if (newl == NULL)
		return;

	    /*
	     * Link the new line into the list.
	     */
	    repllines(win, nextline, 0L, newl);
	    nextline = newl;
	    replchars(win, nextline, 0, 0,
	    		currline->l_text + end_of_1st_text);
	    replchars(win, currline, end_of_1st_text,
			strlen(currline->l_text + end_of_1st_text), "");

	}

	if (yp_buf->y_line_buf != NULL) {
	    Line	*newlines;

	    newlines = copy_lines(yp_buf->y_line_buf, (Line *) NULL);
	    if (newlines != NULL) {
		repllines(win, nextline, 0L, newlines);
	    }
	}

	if (yp_buf->y_2nd_text != NULL) {
	    if (nextline == buffer->b_lastline) {
		Line	*new;

		/*
		 * Can't put the remainder of the text
		 * on the following line, 'cos there
		 * isn't one, so we have to create a
		 * new line.
		 */
		new = newline(strlen(yp_buf->y_2nd_text) + 1);
		if (new == NULL) {
		    end_command(win);
		    return;
		}
		repllines(win, nextline, 0L, new);
		nextline = new;
	    }
	    replchars(win, nextline, 0, 0, yp_buf->y_2nd_text);
	}

	end_command(win);

	/*
	 * Move on to the last character of the inserted text.
	 */
	if (direction == BACKWARD) {
	    (void) one_left(curwin, FALSE);
	}

	cursupdate(win);
	update_buffer(buffer);

    } else if (yp_buf->y_type == y_lines) {

	Line	*new;		/* first line of lines to be put */

	/*
	 * Make a new copy of the saved lines.
	 */
	new = copy_lines(yp_buf->y_line_buf, (Line *) NULL);
	if (new == NULL) {
	    return;
	}

	repllines(win, (direction == FORWARD) ? nextline : currline, 0L, new);

	/*
	 * Put the cursor at the "right" place
	 * (i.e. the place the "real" vi uses).
	 */
	move_cursor(win, new, 0);
	begin_line(win, TRUE);
	move_window_to_cursor(win);
	cursupdate(win);
	update_buffer(buffer);
    } else {
	show_error(win, "Nothing to put!");
    }
}

/*
 * Stuff the specified buffer into the input stream.
 * Called by the '@' command.
 *
 * The "vi_mode" parameter will be FALSE if the buffer should
 * be preceded by a ':' and followed by a '\n', i.e. it is the
 * result of a :@ command rather than a vi-mode @ command.
 */
void
yp_stuff_input(win, name, vi_mode)
Xviwin	*win;
int	name;
bool_t	vi_mode;
{
    Yankbuffer	*yp_buf;

    yp_buf = yp_get_buffer(name);
    if (yp_buf == NULL) {
	show_error(win, "Nothing in buffer %c", name);
	return;
    }

    switch (yp_buf->y_type) {
    case y_chars:
	put(yp_buf->y_1st_text, vi_mode, FALSE);
	break;

    case y_lines:
	break;

    default:
	show_error(win, "Nothing to put!");
	return;
    }

    if (yp_buf->y_line_buf != NULL) {
	Line	*lp;

	for (lp = yp_buf->y_line_buf; lp != NULL; lp = lp->l_next) {
	    put(lp->l_text, vi_mode, TRUE);
	}
    }

    if (yp_buf->y_type == y_chars && yp_buf->y_2nd_text != NULL) {
	put(yp_buf->y_2nd_text, vi_mode, FALSE);
    }
}

static void
put(str, vi_mode, newline)
char	*str;
bool_t	vi_mode;
bool_t	newline;
{
    stuff("%s%s%s",
	    (!vi_mode && str[0] != ':') ? ":" : "",
	    str,
	    (!vi_mode || newline) ? "\n" : "");
}

/*
 * Copy the lines pointed at by "from", up to but not including
 * pointer "to" (which might be NULL), into new memory and return
 * a pointer to the start of the new list.
 *
 * Returns NULL for errors.
 */
static Line *
copy_lines(from, to)
Line	*from, *to;
{
    Line	*src;
    Line	head;
    Line	*dest = &head;

    for (src = from; src != to; src = src->l_next) {
	Line	*tmp;

	tmp = newline(strlen(src->l_text) + 1);
	if (tmp == NULL) {
	    throw(head.l_next);
	    return(NULL);
	}

	/*
	 * Copy the line's text over, and advance
	 * "dest" to point to the new line structure.
	 */
	(void) strcpy(tmp->l_text, src->l_text);
	tmp->l_next = NULL;
	tmp->l_prev = dest;
	dest->l_next = tmp;
	dest = tmp;
    }

    return(head.l_next);
}

static char *
yanktext(from, to)
Posn	*from, *to;
{
    int		nchars;
    char	*cp;

    nchars = to->p_index - from->p_index + 1;
    cp = (char *) alloc((unsigned) nchars + 1);
    if (cp == NULL) {
	return(NULL);
    }

    (void) strncpy(cp, from->p_line->l_text + from->p_index, nchars);
    cp[nchars] = '\0';

    return(cp);
}

static void
yp_free(yp)
Yankbuffer	*yp;
{
    if (yp->y_type == y_lines) {
	throw(yp->y_line_buf);
	yp->y_line_buf = NULL;
    } else if (yp->y_type == y_chars) {
	free(yp->y_1st_text);
	yp->y_1st_text = NULL;
	if (yp->y_2nd_text != NULL)
	    free(yp->y_2nd_text);
	yp->y_2nd_text = NULL;
	if (yp->y_line_buf != NULL)
	    throw(yp->y_line_buf);
	yp->y_line_buf = NULL;
    }
    yp->y_type = y_none;
}

/*
 * Push up buffers 1..8 by one, spilling 9 off the top.
 * Then move '@' into '1'.
 *
 * This routine assumes contiguity of characters '0' to '9',
 * i.e. probably ASCII, but what the hell.
 */
void
yp_push_deleted()
{
    Yankbuffer	*atp;
    int		c;

    yp_free(&yb[bufno('9')]);
    for (c = '9'; c > '1'; --c) {
    	yb[bufno(c)] = yb[bufno(c - 1)];
    }
    atp = &yb[bufno('@')];
    yb[bufno('1')] = *atp;
    atp->y_type = y_none;
    atp->y_line_buf = NULL;
    atp->y_1st_text = NULL;
    atp->y_2nd_text = NULL;
}
