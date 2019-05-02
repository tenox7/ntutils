/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)ptrfunc.c	2.2 (Chris & John Downey) 8/28/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    ptrfunc.c
* module function:
    Primitive functions on "Posn"s.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

/*
 * The routines in this file attempt to imitate many of the operations
 * that used to be performed on simple character pointers and are now
 * performed on Posn's. This makes it easier to modify other sections
 * of the code. Think of a Posn as representing a position in the file.
 * Posns can be incremented, decremented, compared, etc. through the
 * functions implemented here.
 *
 * Note that some functions are now implemented as macros, in ptrfunc.h.
 */

/*
 * inc(p)
 *
 * Increment the line pointer 'p' crossing line boundaries as
 * necessary. Return mv_CHLINE when crossing a line, mv_NOMOVE when at
 * end of file, mv_SAMELINE otherwise.
 */
enum mvtype
inc(lp)
register Posn	*lp;
{
    register char *p;

    p = &(lp->p_line->l_text[lp->p_index]);

    if (*p != '\0') {			/* still within line */
	lp->p_index++;
	return((p[1] != '\0') ? mv_SAMELINE : mv_EOL);
    }

    if (!is_lastline(lp->p_line->l_next)) {
	lp->p_index = 0;
	lp->p_line = lp->p_line->l_next;
	return(mv_CHLINE);
    }

    return(mv_NOMOVE);
}

/*
 * dec(p)
 *
 * Decrement the line pointer 'p' crossing line boundaries as
 * necessary. Return mv_CHLINE when crossing a line, mv_NOMOVE when at
 * start of file, mv_SAMELINE otherwise.
 */
enum mvtype
dec(lp)
register Posn	*lp;
{
    if (lp->p_index > 0) {			/* still within line */
	lp->p_index--;
	return(mv_SAMELINE);
    }

    if (!is_line0(lp->p_line->l_prev)) {
	lp->p_line = lp->p_line->l_prev;
	lp->p_index = strlen(lp->p_line->l_text);
	return(mv_CHLINE);
    }

    return(mv_NOMOVE);				/* at start of file */
}

/*
 * pswap(a, b) - swap two position pointers.
 */
void
pswap(a, b)
register Posn	*a, *b;
{
    register Posn	tmp;

    tmp = *a;
    *a  = *b;
    *b  = tmp;
}

/*
 * Posn comparisons.
 */
bool_t
lt(a, b)
register Posn	*a, *b;
{
    if (a->p_line != b->p_line) {
	return(earlier(a->p_line, b->p_line));
    } else {
	return(a->p_index < b->p_index);
    }
}
