/* Copyright (c) 1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)version.c	2.4 (Chris & John Downey) 10/15/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    version.c
* module function:
    Version string definition.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#ifndef lint
static char *copyright = "@(#)Copyright (c) 1992 Chris & John Downey";
#endif

#ifdef __DATE__
    char	Version[] = "Xvi 2.15 " __DATE__;
#else
    char	Version[] = "Xvi 2.15 15th October 1992";
#endif
