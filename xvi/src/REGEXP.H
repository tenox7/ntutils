/***

* @(#)regexp.h	2.1 7/29/92

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    regexp.h
* module function:
    Definitions for regular expression routines.

* history:
    Regular expression routines by Henry Spencer.
    Modfied for use with STEVIE (ST Editor for VI Enthusiasts,
     Version 3.10) by Tony Andrews.
    Adapted for use with Xvi by Chris & John Downey.
    Original copyright notice is in regexp.c.
    Please note that this is a modified version.
***/

/*
 * Caveat:  this is V8 regexp(3) [actually, a reimplementation thereof],
 * not the System V one.
 */

#define NSUBEXP 10
typedef struct regexp {
    char    *startp[NSUBEXP];
    char    *endp[NSUBEXP];
    char    regstart;		/* Internal use only. */
    char    reganch;		/* Internal use only. */
    char    *regmust;		/* Internal use only. */
    int     regmlen;		/* Internal use only. */
    char    program[1];		/* Unwarranted chumminess with compiler. */
} regexp;

#ifndef P
#   include "xvi.h"
#endif

/* regerror.c */
extern	void	regerror P((char *s));

/* regexp.c */
extern	regexp	*regcomp P((char *exp));
extern	int	regexec P((regexp *prog, char *string, int at_bol));

/* regsub.c */
extern void	regsub P((regexp *prog, char *source, char *dest));
