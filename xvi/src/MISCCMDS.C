/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)misccmds.c	2.2 (Chris & John Downey) 8/28/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    misccmds.c
* module function:
    Miscellaneous functions.

    This module will probably get hacked later and split
    up more sensibly.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

/*
 * Add a blank line above or below the current line.
 * Returns TRUE for success, FALSE for failure to get memory.
 *
 * The single boolean parameter tells us whether to split the
 * current line at the cursor position, or just to open a new
 * line leaving the current one intact.
 */
bool_t
openfwd(split_line)
bool_t	split_line;
{
    Line		*l;		/* pointer to newly allocated line */
    register Posn	*oldposn;
    register Line	*oldline;
    register char	*otext;

    oldposn = curwin->w_cursor;
    oldline = oldposn->p_line;
    otext = oldline->l_text;

    /*
     * First find space for new line.
     *
     * By asking for as much space as the prior line had we make sure
     * that we'll have enough space for any auto-indenting.
     */
    l = newline(strlen(otext) + SLOP);
    if (l == NULL)
	return(FALSE);

    /*
     * Link the new line into the list.
     */
    repllines(curwin, oldline->l_next, 0L, l);

    /*
     * Do auto-indent.
     */
    if (Pb(P_autoindent)) {
	*l->l_text = '\0';
	indentchars = set_indent(l, get_indent(oldline));
    } else {
	indentchars = 0;
    }

    /*
     * If we're in insert mode, we need to move the remainder of the
     * current line onto the new line. Otherwise the new line is left
     * blank.
     */
    if (split_line) {
	char	*s;

	s = otext + oldposn->p_index;

	replchars(curwin, l, indentchars, 0, s);
	replchars(curwin, oldline, oldposn->p_index, strlen(s), "");
    }

    /*
     * Move cursor to the new line.
     */
    move_cursor(curwin, l, indentchars);
    move_window_to_cursor(curwin);
    cursupdate(curwin);
    update_buffer(curbuf);

    return(TRUE);
}

/*
 * Add a blank line above the current line.
 * Returns TRUE for success, FALSE for failure to get memory.
 */
bool_t
openbwd()
{
    Line		*l;
    register Line	*oldline;
    register char	*otext;

    oldline = curwin->w_cursor->p_line;
    otext = oldline->l_text;

    /*
     * First find space for new line.
     */
    l = newline(strlen(otext) + SLOP);
    if (l == NULL)
	return(FALSE);

    /*
     * Link the new line into the list.
     */
    repllines(curwin, oldline, 0L, l);

    /*
     * Do auto-indent.
     */
    if (Pb(P_autoindent)) {
	*l->l_text = '\0';
	indentchars = set_indent(l, get_indent(oldline));
    } else {
	indentchars = 0;
    }

    /*
     * Ensure the cursor is pointing at the right line.
     */
    move_cursor(curwin, l, indentchars);
    move_window_to_cursor(curwin);
    cursupdate(curwin);
    update_buffer(curbuf);

    return(TRUE);
}

/*
 * Count the number of lines between the two given lines.
 * If the two given lines are the same, the return value
 * is 1, not 0; i.e. the count is inclusive.
 *
 * Note that this function has been changed to give the
 * correct number of lines, even if they are ordered wrongly.
 * This change is backwards-compatible with the old version.
 */
long
cntllines(pbegin, pend)
Line		*pbegin;
register Line	*pend;
{
    register Line	*lp;
    register long	lnum;
    bool_t		swapped = FALSE;

    /*
     * Ensure correct ordering.
     */
    if (later(pbegin, pend)) {
	lp = pbegin;
	pbegin = pend;
	pend = lp;
	swapped = TRUE;
    }

    for (lnum = 1, lp = pbegin; lp != pend; lp = lp->l_next) {
	lnum++;
    }

    if (swapped)
	lnum = - lnum;

    return(lnum);
}

/*
 * plines(lp) - return the number of physical screen lines taken by line 'lp'.
 */
long
plines(win, lp)
Xviwin	*win;
Line	*lp;
{
    register long	col;
    register char	*s;

    s = lp->l_text;

    if (*s == '\0')		/* empty line */
	return(1);

    /*
     * If list mode is on, then the '$' at the end of
     * the line takes up one extra column.
     */
    col = Pb(P_list) ? 1 : 0;

    if (Pb(P_number)) {
	col += NUM_SIZE;
    }

    for ( ; *s != '\0'; s++) {
	col += vischar(*s, (char **) NULL, (int) col);
    }

    {
	register int	row;
	register int	columns;

	columns = win->w_ncols;
	for (row = 1; col > columns; ) {
	    row++;
	    col -= columns;
	}
	return row;
    }
}

/*
 * Count the number of physical lines between the two given lines.
 *
 * This routine is like cntllines(), except that:
 *	it counts physical rather than logical lines
 *	it always returns the absolute number of physical lines
 *	it is non-inclusive
 *	if the physical line count for a group of lines is greater
 *	than or equal to rows * 2, we just return rows * 2; we assume
 *	the caller isn't interested in the exact number.
 */
long
cntplines(win, pbegin, pend)
Xviwin		*win;
Line		*pbegin;
register Line	*pend;
{
    register Line	*lp;
    register long	physlines;
    unsigned		toomuch;

    /*
     * Ensure correct ordering.
     */
    if (later(pbegin, pend)) {
	lp = pbegin;
	pbegin = pend;
	pend = lp;
    }

    toomuch = win->w_nrows * 2;
    for (physlines = 0, lp = pbegin; lp != pend; lp = lp->l_next) {
	physlines += plines(win, lp);
	if (physlines >= toomuch)
	    break;
    }

    return(physlines);
}

/*
 * gotoline(buffer, n) - return a pointer to line 'n' in the given buffer
 *
 * Returns the first line of the file if n is 0.
 * Returns the last line of the file if n is beyond the end of the file.
 */
Line *
gotoline(b, n)
Buffer			*b;
register unsigned long	n;
{
    if (n == 0) {
	return(b->b_file);
    } else {
	register Line	*lp;

	for (lp = b->b_file; --n > 0 && lp->l_next != b->b_lastline;
							lp = lp->l_next) {
	    ;
	}
	return(lp);
    }
}

int
get_indent(lp)
register Line	*lp;
{
    register char   *text;
    register int    indent;
    register int    ts = Pn(P_tabstop);	/* synonym for efficiency */

    if (lp == NULL || (text = lp->l_text) == NULL) {
	show_error(curwin, "Internal error: get_indent(NULL)");
	return 0;
    }

    for (indent = 0; *text != '\0' && (*text == ' ' || *text == '\t');
    								text++) {
	indent += *text == ' ' ? 1 : ts - indent % ts;
    }
    return indent;
}

/*
 * Set number of columns of leading whitespace on line, regardless of
 * what was there before, & return number of characters (not columns)
 * used.
 */
int
set_indent(lp, indent)
Line		*lp;
register int	indent;
{
    register char	*cp;		/* temp char pointer for loops */
    register int	ntabs;		/* number of tabs to use */
    unsigned		nnew;		/* no of chars used in old line */
    unsigned		nold;		/* no of chars used in new version */
    char		*newstr;	/* allocated string for new indent */

    if (lp == NULL || lp->l_text == NULL) {
	show_error(curwin, "Internal error: set_indent(0)");
	return(0);
    }

    /*
     * Find out how many tabs we need, & how many spaces.
     */
    for (ntabs = 0; indent >= Pn(P_tabstop); ntabs++)
	indent -= Pn(P_tabstop);

    /*
     * Find out how many characters were used for initial
     * whitespace in the current (old) line.
     */
    for (cp = lp->l_text; *cp == ' ' || *cp == '\t'; cp++) {
	;
    }
    nold = cp - lp->l_text;

    /*
     * "nnew" is the number of characters we will use
     * for indentation in the new version of the line.
     */
    nnew = ntabs + indent;

    /*
     * Get some space, and place into it the string of tabs
     * and spaces which will form the new indentation.
     * If no space available, return nold as we have not
     * changed the line; this is the correct action.
     */
    newstr = alloc((unsigned) nnew + 1);
    if (newstr == NULL)
	return(nold);

    cp = newstr;
    while (ntabs-- > 0)
	*cp++ = '\t';
    while (indent-- > 0)
	*cp++ = ' ';
    *cp = '\0';

    /*
     * Finally, replace the old with the new.
     */
    replchars(curwin, lp, 0, (int) nold, newstr);

    free(newstr);

    return(nnew);
}

/*
 * tabinout(inout, start, finish)
 *
 * "inout" is either '<' or '>' to indicate left or right shift.
 */
void
tabinout(inout, start, finish)
int	inout;
Line	*start;
Line	*finish;
{
    Line	*lp;
    long	nlines = 0;

    if (!start_command(curwin)) {
	return;
    }

    finish = finish->l_next;

    for (lp = start; lp != finish; lp = lp->l_next) {
	register char *p;

	/*
	 * Find out whether it's a blank line (either
	 * empty or containing only spaces & tabs).
	 * If so, just remove all whitespace from it.
	 */
	for (p = lp->l_text; *p && (*p == '\t' || *p == ' '); p++)
	    ;
	if (*p == '\0') {
	    if (p > lp->l_text) {
		replchars(curwin, lp, 0, p - lp->l_text, "");
	    }
	} else if (inout == '<') {
	    int oldindent = get_indent(lp);

	    (void) set_indent(lp, (oldindent <= Pn(P_shiftwidth) ?
				    0 : oldindent - Pn(P_shiftwidth)));
	} else {
	    (void) set_indent(lp, get_indent(lp) + Pn(P_shiftwidth));
	}

	nlines++;
    }

    end_command(curwin);

    if (nlines > Pn(P_report)) {
	show_message(curwin, "%ld lines %ced", nlines, inout);
    }
}

/*
 * Construct a vector of pointers into each word in
 * the given string. Intervening whitespace will be
 * converted to null bytes.
 *
 * Returned vector is constructed in allocated space,
 * and so must be freed after use.
 *
 * An extra NULL pointer is always allocated for safety.
 *
 * If memory cannot be allocated, or if there are no
 * words in the given string, or if it is a NULL ptr,
 * then the returned values will be 0 and NULL.
 *
 * The "whites" argument is a pointer to an array of
 * characters which are to be considered as whitespace.
 */
void
makeargv(str, argcp, argvp, whites)
char	*str;
int	*argcp;
char	***argvp;
char	*whites;
{
    int		argc;
    char	**argv;
    int		argv_size;

    *argcp = 0;
    *argvp = NULL;

    if (str == NULL)
	return;

    /*
     * Scan past initial whitespace.
     */
    while (*str != '\0' && strchr(whites, *str) != NULL) {
	if (*str == '\\' && strchr(whites, str[1]) != NULL) {
	    str++;
	}
	str++;
    }
    if (*str == '\0')
	return;

    argv = (char **) alloc(sizeof(char *) * 8);
    if (argv == NULL)
	return;
    argv_size = 8;
    argc = 0;

    do {
	if (argc >= (argv_size - 1)) {
	    argv_size += 8;
	    argv = (char **) realloc((char *) argv,
				(unsigned) argv_size * sizeof(char *));
	    if (argv == NULL)
		return;
	}

	argv[argc++] = str;

	while (*str != '\0' && strchr(whites, *str) == NULL) {
	    if (*str == '\\' && strchr(whites, str[1]) != NULL) {
		char	*p;

		/*
		 * What a hack. Copy the rest of the string
		 * down by one byte to remove the backslash.
		 * Don't forget to copy the null byte.
		 */
		for (p = str + 1; (*(p-1) = *p) != '\0'; p++)
		    ;
	    }
	    str++;
	}
	while (*str != '\0' && strchr(whites, *str) != NULL)
	    *str++ = '\0';

    } while (*str != '\0');

    argv[argc] = NULL;

    *argvp = argv;
    *argcp = argc;
}
