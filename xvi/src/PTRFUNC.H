/* Copyright (c) 1990,1991,1992 Chris and John Downey */
/***

* @(#)ptrfunc.h	2.1 (Chris & John Downey) 7/29/92

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    ptrfunc.h
* module function:
    Functions on Posn's - defined here for speed, since they
    typically get called "on a per-character basis".
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

/*
 * gchar(lp) - get the character at position "lp"
 */
#define	gchar(lp)	((lp)->p_line->l_text[(lp)->p_index])
