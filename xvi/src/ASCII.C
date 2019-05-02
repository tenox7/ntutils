/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)ascii.c	2.1 (Chris & John Downey) 7/29/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    ascii.c
* module function:
    Visual representations of control & other characters.

    This file is specific to the ASCII character set;
    versions for other character sets could be implemented if
    required.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

/*
 * Output visual representation for a character. Return value is the
 * width, in display columns, of the representation; the actual string
 * is returned in *pp unless pp is NULL.
 *
 * If tabcol is -1, a tab is represented as "^I"; otherwise, it gives
 * the current physical column, & is used to determine the number of
 * spaces in the string returned as *pp.
 *
 * Looks at the parameters tabs, list, cchars and mchars.
 * This may sound costly, but in fact it's a single memory
 * access for each parameter.
 */
unsigned
vischar(c, pp, tabcol)
    register int	c;
    register char	**pp;
    register int	tabcol;
{
    static char		crep[5];

    if (c == '\t' && tabcol >= 0 && Pb(P_tabs) && !Pb(P_list)) {
	/*
	 * Tab which we have to display as a string of
	 * spaces (rather than "^I" or directly).
	 */
	register unsigned	nspaces;

	while (tabcol >= Pn(P_tabstop))
	    tabcol -= Pn(P_tabstop);
	nspaces = Pn(P_tabstop) - tabcol;
	if (pp != NULL) {
	    static char	spstr[MAX_TABSTOP + 1];
	    static unsigned	lastnum;

	    /*
	     * Paranoia (maybe).
	     */
	    if (nspaces > MAX_TABSTOP)
		nspaces = MAX_TABSTOP;
	    if (nspaces > lastnum)
		(void) memset(&spstr[lastnum], ' ',
		       (int) (nspaces - lastnum));
	    spstr[lastnum = nspaces] = '\0';
	    *pp = spstr;
	}
	return nspaces;
    } else if (((unsigned char) c < ' ' || c == DEL) && !Pb(P_cchars)) {
	/*
	 * ASCII Control characters.
	 */
	if (pp != NULL) {
	    *pp = crep;
	    crep[0] = '^';
	    crep[1] = (c == DEL ? '?' : c + ('A' - 1));
	    crep[2] = '\0';
	}
	return 2;
    } else if ((c & ~0177) && !Pb(P_mchars)) {
	/*
	 * If Pb(P_mchars) is unset, we display non-ASCII characters
	 * (i.e. top-bit-set characters) as octal escape sequences.
	 */
	if (pp != NULL) {
	    *pp = crep;
	    crep[0] = '\\';
	    crep[1] = ((c >> 6) & 7) + '0';
	    crep[2] = ((c >> 3) & 7) + '0';
	    crep[3] = (c	    & 7) + '0';
	    crep[4] = '\0';
	}
	return 4;
    } else {
	/*
	 * Printable character.
	 */
	if (pp != NULL) {
	    *pp = crep;
	    crep[0] = c;
	    crep[1] = '\0';
	}
	return 1;
    }
}
