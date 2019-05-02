/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)preserve.c	2.1 (Chris & John Downey) 7/29/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    preserve.c
* module function:
    Buffer preservation routines.

    The do_preserve() routine saves the contents of all modified
    buffers in temporary files. It can be invoked with the
    :preserve command or it may be called by one of the system
    interface modules when at least PSVKEYS keystrokes have been
    read, & at least Pn(P_preservetime) seconds have elsapsed
    since the last keystroke. (PSVKEYS is defined in xvi.h.) The
    preservebuf() routine can be used to preserve a single buffer.

* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey
***/

#include "xvi.h"

/*
 * Names of values for the P_preserve enumerated parameter.
 */
char	*psv_strings[] =
{
    "unsafe",
    "standard",
    "safe",
    "paranoid",
    NULL
};

/*
 * Open temporary file for given buffer.
 */
static FILE *
psvfile(window)
Xviwin	*window;
{
    register Buffer	*buffer;
    FILE		*fp;

    buffer = window->w_buffer;

    if (buffer->b_tempfname == NULL) {
	char	*fname;

	fname = buffer->b_filename;
	if (fname == NULL)
	    fname = "unnamed";
	buffer->b_tempfname = tempfname(fname);
	if (buffer->b_tempfname == NULL) {
	    show_error(window, "Can't create name for preserve file");
	    return(NULL);
	}
    }
    fp = fopenwb(buffer->b_tempfname);
    if (fp == NULL) {
	show_error(window, "Can't open preserve file %s",
					     buffer->b_tempfname);
    }
    return(fp);
}

/*
 * Write contents of buffer to file & close file. Return TRUE if no
 * errors detected.
 */
static bool_t
putbuf(wp, fp)
Xviwin		*wp;
register FILE	*fp;
{
    unsigned long	l1, l2;

    if (put_file(wp, fp, (Line *) NULL, (Line *) NULL, &l1, &l2) == FALSE) {
	show_error(wp, "Error writing preserve file %s",
					     wp->w_buffer->b_tempfname);
	return(FALSE);
    } else {
	return(TRUE);
    }
}

/*
 * Preserve contents of a single buffer, so that a backup copy is
 * available in case something goes wrong while the file itself is
 * being written.
 *
 * This is controlled by the P_preserve parameter: if it's set to
 * psv_UNSAFE, we just return. If it's psv_STANDARD, to save time, we
 * only preserve the buffer if it doesn't appear to have been
 * preserved recently: otherwise, if it's psv_SAFE or psv_PARANOID, we
 * always preserve it.
 *
 * Return FALSE if an error occurs during preservation, otherwise TRUE.
 */
bool_t
preservebuf(window)
Xviwin	*window;
{
    FILE	*fp;
    Buffer	*bp;

    if (
	Pn(P_preserve) == psv_UNSAFE
	||
	(
	    Pn(P_preserve) == psv_STANDARD
	    &&
	    /*
	     * If there is a preserve file already ...
	     */
	    (bp = window->w_buffer)->b_tempfname != NULL
	    &&
	    exists(bp->b_tempfname)
	    &&
	    /*
	     * & a preserve appears to have been done recently ...
	     */
	    keystrokes < PSVKEYS
	)
    ) {
	/*
	 * ... don't bother.
	 */
	return(TRUE);
    }

    fp = psvfile(window);
    if (fp == NULL) {
	return(FALSE);
    }

    return(putbuf(window, fp));
}

/*
 * Preserve contents of all modified buffers.
 */
bool_t
do_preserve()
{
    Xviwin		*wp;
    bool_t		psvstatus = TRUE;

    wp = curwin;
    do {
	if (is_modified(wp->w_buffer)) {
	    FILE	*fp;

	    fp = psvfile(wp);
	    if (fp != NULL) {
		if (!putbuf(wp, fp))
		    psvstatus = FALSE;
	    } else {
		psvstatus = FALSE;
	    }
	}
    } while ((wp = next_window(wp)) != curwin);

    return(psvstatus);
}
