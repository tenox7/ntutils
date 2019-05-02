/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)alloc.c	2.1 (Chris & John Downey) 7/29/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    alloc.c
* module function:
    Various routines dealing with allocation
    and deallocation of data structures.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

/*
 * We use a special strategy for the allocation & freeing of Change
 * structures to make these operations as fast as possible (since we
 * have to allocate one for each input character in INSERT & REPLACE
 * states).
 *
 * So we have a linked list of reusable Change structures; freeing a
 * Change means just adding it to this list.
 */
static Change	*chlist = NULL;

/*
 * Free a Change structure. This just means adding it to our list of
 * reusable structures, so a later request for an allocation of a
 * Change can be satisfied very quickly from the front of the list.
 */
void
chfree(ch)
    Change *ch;
{
    ch->c_next = chlist;
    chlist = ch;
}

Change *
challoc()
{
    if (chlist) {
	Change	*ch;

	ch = chlist;
	chlist = chlist->c_next;
	return ch;
    }
    return (Change *) alloc(sizeof (Change));
}

/*
 * We also use a similar strategy for Line structures.
 */
static Line	*lnlist = NULL;

char *
alloc(size)
unsigned size;
{
    char *p;		/* pointer to new storage space */

    while ((p = malloc(size)) == NULL) {
	/*
	 * No more memory in the heap, but we may be able to
	 * satisfy the request by recycling entries in one of
	 * our lists of reusable structures.
	 */
	if (lnlist) {
	    p = (char *) lnlist;
	    lnlist = lnlist->l_next;
	    free(p);
	} else if (chlist) {
	    p = (char *) chlist;
	    chlist = chlist->c_next;
	    free(p);
	} else {
	    /*
	     * No: we're out.
	     */
	    show_error(curwin, "Not enough memory!");
	    break;
	}
    }
    return(p);
}

char *
strsave(string)
const char *string;
{
    char	*space;

    space = alloc((unsigned) strlen(string) + 1);
    if (space != NULL)
	(void) strcpy(space, string);
    return(space);
}

/*
 * Allocate and initialize a new line structure with room for
 * 'nchars' characters.
 */
Line *
newline(nchars)
int	nchars;
{
    register Line	*l;
    char		*ltp;

    if (lnlist == NULL) {
	register unsigned n;

	/*
	 * To avoid memory fragmentation, we try to allocate a
	 * contiguous block of 100 Line structures if we
	 * haven't already got any.
	 *
	 * This means that, for every 100 lines of a file we
	 * read in, there should be a block of Line
	 * structures, which may never be freed, followed by
	 * a large arena for the lines' text and other sundry
	 * dynamically allocated objects, which generally will
	 * be.
	 *
	 * If we can't even get one structure, alloc() should
	 * print an error message. For subsequent ones, we use
	 * malloc() instead because it isn't necessarily a
	 * serious error if we can't get any more space than
	 * we've actually been asked for.
	 */
	if ((lnlist = (Line *) alloc(sizeof(Line))) == NULL) {
	    return(NULL);
	}
	lnlist->l_next = NULL;
	for (n = 99; n != 0; n--) {
	    if ((l = (Line *) malloc(sizeof(Line))) == NULL) {
		break;
	    }
	    l->l_next = lnlist;
	    lnlist = l;
	}
    }

    /*
     * Assertion: lnlist != NULL.
     */
    l = lnlist;
    lnlist = l->l_next;

    /*
     * It is okay for newline() to be called with a 0
     * parameter - but we must never call malloc(0) as
     * this will break on many systems.
     */
    if (nchars == 0)
	nchars = 1;
    ltp = alloc((unsigned) nchars);
    if (ltp == NULL) {
	free((char *) l);
	return(NULL);
    }
    ltp[0] = '\0';
    l->l_text = ltp;
    l->l_size = nchars;
    l->l_prev = NULL;
    l->l_next = NULL;

    return(l);
}

/*
 * bufempty() - return TRUE if the buffer is empty
 */
bool_t
bufempty(b)
Buffer	*b;
{
    return(buf1line(b) && b->b_file->l_text[0] == '\0');
}

/*
 * buf1line() - return TRUE if there is only one line
 */
bool_t
buf1line(b)
Buffer	*b;
{
    return(b->b_file->l_next == b->b_lastline);
}

/*
 * endofline() - return TRUE if the given position is at end of line
 *
 * This routine will probably never be called with a position resting
 * on the zero byte, but handle it correctly in case it happens.
 */
bool_t
endofline(p)
Posn	*p;
{
    register char	*endtext = p->p_line->l_text + p->p_index;

    return(*endtext == '\0' || *(endtext + 1) == '\0');
}

/*
 * grow_line(lp, n)
 *	- increase the size of the space allocated for the line by n bytes.
 *
 * This routine returns TRUE immediately if the requested space is available.
 * If not, it attempts to allocate the space and adjust the data structures
 * accordingly, and returns TRUE if this worked.
 * If everything fails it returns FALSE.
 */
bool_t
grow_line(lp, n)
Line	*lp;
register int	n;
{
    register int	nsize;
    register char	*s;		/* pointer to new space */

    nsize = strlen(lp->l_text) + 1 + n;	/* size required */

    if (nsize <= lp->l_size)
	return(TRUE);

    /*
     * Need to allocate more space for the string. Allow some extra
     * space on the assumption that we may need it soon. This avoids
     * excessive numbers of calls to malloc while entering new text.
     */
    s = alloc((unsigned) nsize + SLOP);
    if (s == NULL) {
	return(FALSE);
    }

    lp->l_size = nsize + SLOP;
    (void) strcpy(s, lp->l_text);
    free(lp->l_text);
    lp->l_text = s;

    return(TRUE);
}

/*
 * Free up space used by the given list of lines.
 *
 * Note that the Line structures themselves are just added to the list
 * of reusable ones.
 */
void
throw(lineptr)
Line	*lineptr;
{
    if (lineptr != NULL) {
	Line	*newlist;

	newlist = lineptr;
	for (;;) {
	    Line	*nextline;

	    if (lineptr->l_text != NULL)
		free(lineptr->l_text);
	    if ((nextline = lineptr->l_next) == NULL) {
		/*
		 * We've reached the end of this list;
		 * join it on to lnlist ...
		 */
		lineptr->l_next = lnlist;
		/*
		 * ... & point lnlist at the beginning
		 * of this list.
		 */
		lnlist = newlist;
		return;
	    } else {
		lineptr = nextline;
	    }
	}
    }
}
