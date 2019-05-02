/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)edit.c	2.4 (Chris & John Downey) 8/26/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    edit.c
* module function:
    Insert and replace mode handling.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

static	void	end_replace P((int));

/*
 * Position of start of insert. This is used
 * to prevent backing up past the starting point.
 */
static	Posn	Insertloc;

/*
 * This flexbuf is used to hold the current insertion text.
 */
static	Flexbuf	Insbuff;

/*
 * Replace-mode stuff.
 */
static	enum	{
		replace_one,	/* replace command was an 'r' */
		got_one,	/* normal ending state for replace_one */
		overwrite	/* replace command was an 'R' */
}		repstate;

static	char	*saved_line;	/*
				 * record of old line before replace
				 * started; note that, if
				 * (repstate != overwrite), this
				 * should never be referenced.
				 */
static	int	nchars;		/* no of chars in saved_line */
static	int	start_index;	/* index into line where we entered replace */
static	int	start_column;	/* virtual col corresponding to start_index */

/*
 * Process the given character, in insert mode.
 */
bool_t
i_proc(c)
int	c;
{
    register Posn	*curpos;
    static bool_t	literal_next = FALSE;
    static bool_t	wait_buffer = FALSE;

    curpos = curwin->w_cursor;

    if (wait_buffer || (!literal_next && c == CTRL('A'))) {
	/*
	 * Add contents of named buffer, or the last
	 * insert buffer if CTRL('A') was typed.
	 */
	if (!wait_buffer) {
	    c = '<';
	}
	yp_stuff_input(curwin, c, TRUE);
	wait_buffer = FALSE;
	return(FALSE);

    } else if (!literal_next) {
	/*
	 * This switch is for special characters; we skip over
	 * it for normal characters, or for literal-next mode.
	 */
	switch (c) {
	case ESC:	/* an escape ends input mode */
	{
	    char	*cltext;

	    cltext = curpos->p_line->l_text;

	    curwin->w_set_want_col = TRUE;

	    /*
	     * If there is only auto-indentation
	     * on the current line, delete it.
	     */
	    if (curpos->p_index == indentchars &&
		cltext[indentchars] == '\0') {
		replchars(curwin, curpos->p_line, 0, indentchars, "");
		begin_line(curwin, FALSE);
	    }
	    indentchars = 0;

	    /*
	     * The cursor should end up on the last inserted
	     * character. This is an attempt to match the real
	     * 'vi', but it may not be quite right yet.
	     */
	    while (one_left(curwin, FALSE) &&
				gchar(curwin->w_cursor) == '\0') {
		;
	    }

	    State = NORMAL;

	    end_command(curwin);

	    (void) yank_str('<', flexgetstr(&Insbuff), FALSE);
	    flexclear(&Insbuff);

	    if (!(echo & e_CHARUPDATE)) {
		echo |= e_CHARUPDATE;
		move_window_to_cursor(curwin);
		cursupdate(curwin);
	    }
	    update_buffer(curbuf);
	    return(TRUE);
	}

	case CTRL('T'):
	case CTRL('D'):
	    /*
	     * If we're at the beginning of the line, or just
	     * after autoindent characters, move one shiftwidth
	     * left (CTRL('D')) or right (CTRL('T')).
	     */
	    if (curpos->p_index <= indentchars) {
		Line	*lp;
		int	ind;

		lp = curpos->p_line;
		ind = get_indent(curpos->p_line);
		ind += (c == CTRL('D') ? (ind < Pn(P_shiftwidth) ?
					     -ind :
					     -Pn(P_shiftwidth)) :
					Pn(P_shiftwidth));
		indentchars = set_indent(lp, ind);
		move_cursor(curwin, curpos->p_line, indentchars);
		cursupdate(curwin);
		updateline(curwin);
		(void) flexaddch(&Insbuff, c);
	    } else {
		beep(curwin);
	    }
	    return(TRUE);

	case '\b':
	case DEL:
	    /*
	     * Can't backup past starting point.
	     */
	    if (curpos->p_line == Insertloc.p_line &&
			    curpos->p_index <= Insertloc.p_index) {
		beep(curwin);
		return(TRUE);
	    }

	    /*
	     * Can't backup to a previous line.
	     */
	    if (curpos->p_line != Insertloc.p_line && curpos->p_index <= 0) {
		beep(curwin);
		return(TRUE);
	    }
	    (void) one_left(curwin, FALSE);
	    if (curpos->p_index < indentchars)
		indentchars--;
	    replchars(curwin, curpos->p_line, curpos->p_index, 1, "");
	    (void) flexaddch(&Insbuff, '\b');
	    cursupdate(curwin);
	    if (curwin->w_col == 0) {
		/*
		 * Make sure backspacing over a physical line
		 * break updates the screen correctly.
		 */
		update_buffer(curbuf);
	    } else {
		updateline(curwin);
	    }
	    return(TRUE);

	case '\r':
	case '\n':
	    {
		int	i;
		int	previndex;
		Line	*prevline;

		(void) flexaddch(&Insbuff, '\n');

		i = indentchars;
		prevline = curpos->p_line;
		previndex = curpos->p_index;

		if (openfwd(TRUE) == FALSE) {
		    stuff("%c", ESC);
		    show_error(curwin,
			    "No buffer space - returning to command mode");
		    return(TRUE);
		}

		/*
		 * If the previous line only had
		 * auto-indent on it, delete it.
		 */
		if (i == previndex) {
		    replchars(curwin, prevline, 0, i, "");
		}

		move_window_to_cursor(curwin);
		cursupdate(curwin);
		update_buffer(curbuf);
	    }
	    return(TRUE);

	case CTRL('B'):
	    wait_buffer = TRUE;
	    return(FALSE);
	    break;

	case CTRL('V'):
	    (void) flexaddch(&Insbuff, CTRL('V'));
	    literal_next = TRUE;
	    return(FALSE);
	}
    }

    /*
     * If we get here, we want to insert the character into the buffer.
     */

    /*
     * We may already have been in literal-next mode.
     * Careful not to reset this until after we need it.
     */
    literal_next = FALSE;

    /*
     * Deal with wrapmargin.
     *
     * OK, so it isn't really right yet.
     */
    if ((c == ' ' || c == '\t') && Pn(P_wrapmargin) != 0 &&
	curwin->w_cursor->p_index >= curwin->w_ncols - Pn(P_wrapmargin)) {
	(void) i_proc('\n');
	/*
	 * We shouldn't really be putting the newline
	 * in the redo buffer, so we change it to the
	 * character we actually got.
	 */
	flexrmchar(&Insbuff);
	(void) flexaddch(&Insbuff, c);
	return(TRUE);
    }

    /*
     * Do the actual insertion of the new character.
     */
    replchars(curwin, curpos->p_line, curpos->p_index, 0, mkstr(c));

    /*
     * Update the screen.
     */
    s_inschar(curwin, c);
    updateline(curwin);

    /*
     * Put the character into the insert buffer.
     */
    (void) flexaddch(&Insbuff, c);

    /*
     * If showmatch mode is set, check for right parens
     * and braces. If there isn't a match, then beep.
     * If there is a match AND it's on the screen,
     * flash to it briefly.
     *
     * These characters included to make this
     * source file work okay with showmatch: [ { (
     */
    if (Pb(P_showmatch) && (c == ')' || c == '}' || c == ']')) {
	Posn	*lpos, csave;

	lpos = showmatch();
	if (lpos == NULL) {
	    beep(curwin);
	} else if (!earlier(lpos->p_line, curwin->w_topline) &&
		       earlier(lpos->p_line, curwin->w_botline)) {
	    /*
	     * Show the match if it's on screen.
	     */
	    update_buffer(curbuf);

	    csave = *curpos;
	    move_cursor(curwin, lpos->p_line, lpos->p_index);
	    cursupdate(curwin);
	    wind_goto(curwin);

	    delay();

	    move_cursor(curwin, csave.p_line, csave.p_index);
	    cursupdate(curwin);
	}
    }

    /*
     * Finally, move the cursor right one space.
     */
    (void) one_right(curwin, TRUE);

    return(TRUE);
}

/*
 * This function is the interface provided for functions in
 * normal mode to go into insert mode. We only come out of
 * insert mode when the user presses escape.
 *
 * The parameter is a flag to say whether we should start
 * at the start of the line.
 *
 * Note that we do not have to call start_command() as the
 * caller does that for us - this is so the caller can include
 * any other stuff (e.g. an initial delete) into the command.
 */
void
startinsert(startln)
int	startln;	/* if set, insert point really at start of line */
{
    Insertloc = *curwin->w_cursor;
    if (startln)
	Insertloc.p_index = 0;
    flexclear(&Insbuff);
    State = INSERT;
}

/*
 * Process the given character, in replace mode.
 */
bool_t
r_proc(c)
int	c;
{
    Posn		*curpos;
    static bool_t	literal_next = FALSE;
    static bool_t	wait_buffer = FALSE;

    curpos = curwin->w_cursor;

    if (wait_buffer || (!literal_next && c == CTRL('A'))) {
	/*
	 * Add contents of named buffer, or the last
	 * insert buffer if CTRL('A') was typed.
	 */
	if (!wait_buffer) {
	    c = '<';
	}
	yp_stuff_input(curwin, c, TRUE);
	wait_buffer = FALSE;
	return(FALSE);

    } else if (!literal_next) {
	switch (c) {
	case ESC:			/* an escape ends input mode */
	    end_replace(c);
	    return(TRUE);

	case '\b':			/* back space */
	case DEL:
	    if (repstate == overwrite &&
		curwin->w_virtcol > start_column) {
		(void) one_left(curwin, FALSE);
		replchars(curwin, curpos->p_line,
				curpos->p_index, 1,
				(curpos->p_index < nchars) ?
				mkstr(saved_line[curpos->p_index]) : "");
		updateline(curwin);
		(void) flexaddch(&Insbuff, '\b');
	    } else {
		beep(curwin);
		if (repstate == replace_one) {
		    end_replace(c);
		}
	    }
	    return(TRUE);

	case K_LARROW:			/* left arrow */
	    if (repstate == overwrite && curwin->w_virtcol > start_column &&
						one_left(curwin, FALSE)) {
		(void) flexaddch(&Insbuff, c);
		return(TRUE);
	    } else {
		beep(curwin);
		if (repstate == replace_one) {
		    end_replace(c);
		}
		return(FALSE);
	    }

	case K_RARROW:			/* right arrow */
	    if (repstate == overwrite && one_right(curwin, FALSE)) {
		(void) flexaddch(&Insbuff, c);
		return(TRUE);
	    } else {
		beep(curwin);
		if (repstate == replace_one) {
		    end_replace(c);
		}
		return(FALSE);
	    }

	case '\r':			/* new line */
	case '\n':	
	    if (curpos->p_line->l_next == curbuf->b_lastline &&
					    repstate == overwrite) {
		/*
		 * Don't allow splitting of last line of
		 * buffer in overwrite mode. Why not?
		 */
		beep(curwin);
		return(TRUE);
	    }

	    if (repstate == replace_one) {
		echo &= ~e_CHARUPDATE;
		if (openfwd(TRUE) == FALSE) {
		    show_error(curwin, "No buffer space!");
		    return(TRUE);
		}

		(void) flexaddch(&Insbuff, c);

		/*
		 * Having split the line, we must
		 * delete the character which was
		 * supposed to be replaced with
		 * the newline.
		 */
		replchars(curwin, curpos->p_line, curpos->p_index, 1, "");
		repstate = got_one;
		end_replace('\n');

	    } else {
		(void) flexaddch(&Insbuff, '\n');

		(void) onedown(curwin, 1L);

		/*
		 * This is wrong, but it's difficult
		 * to get it right.
		 */
		coladvance(curwin, start_column);

		free(saved_line);
		saved_line = strsave(curpos->p_line->l_text);
		if (saved_line == NULL) {
		    State = NORMAL;
		    return(TRUE);
		}
		nchars = strlen(saved_line);
	    }

	    return(TRUE);

	case CTRL('B'):
	    wait_buffer = TRUE;
	    return(FALSE);
	    break;

	case CTRL('V'):	
	    (void) flexaddch(&Insbuff, CTRL('V'));
	    literal_next = TRUE;
	    return(TRUE);
	}
    }

    /*
     * If we get here, we want to insert the character into the buffer.
     */

    /*
     * We may already have been in literal-next mode.
     * Careful not to reset this until after we need it.
     */
    literal_next = FALSE;

    /*
     * Put the character into the insert buffer.
     */
    (void) flexaddch(&Insbuff, c);

    if (repstate == overwrite || repstate == replace_one) {
	replchars(curwin, curpos->p_line, curpos->p_index, 1, mkstr(c));
	updateline(curwin);
	(void) one_right(curwin, TRUE);
    }

    /*
     * If command was an 'r', leave replace mode after one character.
     */
    if (repstate == replace_one) {
	repstate = got_one;
	end_replace(c);
    }

    return(TRUE);
}

/*
 * This function is the interface provided for functions in
 * normal mode to go into replace mode. We only come out of
 * replace mode when the user presses escape, or when they
 * used the 'r' command and typed a single character.
 *
 * The parameter is the command character which took us into replace mode.
 */
void
startreplace(c)
int	c;
{
    if (!start_command(curwin)) {
	return;
    }
    start_index = curwin->w_cursor->p_index;
    start_column = curwin->w_virtcol;

    if (c == 'r') {
	repstate = replace_one;
	saved_line = NULL;
    } else {
	repstate = overwrite;
	saved_line = strsave(curwin->w_cursor->p_line->l_text);
	if (saved_line == NULL) {
	    beep(curwin);
	    end_command(curwin);
	    return;
	}
	nchars = strlen(saved_line);

	/*
	 * Initialize Insbuff. Note that we don't do this if the
	 * command was 'r', because they might type ESC to abort
	 * the command, in which case we shouldn't change Insbuff.
	 */
	flexclear(&Insbuff);

    }
    State = REPLACE;
}

static void
end_replace(c)
    int	c;
{
    Posn	*curpos;
    char	*cltext;

    curpos = curwin->w_cursor;

    State = NORMAL;
    end_command(curwin);

    /*
     * If (repstate == replace_one), they must have typed 'r', then
     * thought better of it & typed ESC; so we shouldn't complain or
     * change the buffer, the cursor position, or Insbuff.
     */
    if (repstate != replace_one) {

	(void) yank_str('<', flexgetstr(&Insbuff), FALSE);
	flexclear(&Insbuff);

	/*
	 * Free the saved line if necessary.
	 */
	if (repstate == overwrite) {
	    free(saved_line);
	}

	/*
	 * The cursor should end up on the
	 * last replaced character.
	 */
	cltext = curpos->p_line->l_text;
	while (one_left(curwin, FALSE) && gchar(curwin->w_cursor) == '\0') {
	    ;
	}

	if (!(echo & e_CHARUPDATE)) {
	    echo |= e_CHARUPDATE;
	    move_window_to_cursor(curwin);
	    cursupdate(curwin);
	}
	update_buffer(curbuf);
    }
}

char *
mkstr(c)
int	c;
{
    static	char	s[2];

    s[0] = c;
    s[1] = '\0';

    return(s);
}
