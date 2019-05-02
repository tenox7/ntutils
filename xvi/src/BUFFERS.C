/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)buffers.c	2.1 (Chris & John Downey) 7/29/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    buffers.c
* module function:
    Handle buffer allocation, deallocation, etc.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

static	bool_t	setup_buffer P((Buffer *));

/*
 * Create a new buffer.
 */
Buffer *
new_buffer()
{
    Buffer	*new;

    new = (Buffer *) alloc(sizeof(Buffer));
    if (new == NULL) {
	return(NULL);
    }

    /*
     * Allocate memory for lines etc.
     */
    if (setup_buffer(new) == FALSE) {
	free((char *) new);
	return(NULL);
    }

    /*
     * Since setup_buffer() does not set up
     * the filenames, we must do it ourselves.
     */
    new->b_filename = NULL;
    new->b_tempfname = NULL;

    new->b_nwindows = 0;

    return(new);
}

/*
 * Delete the given buffer.
 */
void
free_buffer(buffer)
Buffer	*buffer;
{
    if (buffer == NULL)
	return;

    /*
     * Free all the lines in the buffer.
     */
    throw(buffer->b_line0);

    free((char *) buffer);
}

/*
 * Free up all the memory used indirectly by the buffer,
 * and then get some new stuff. This only has an effect
 * on the allocated fields within the buffer, i.e. it
 * does not change any variables such as filenames.
 *
 * Returns TRUE for success, FALSE for failure to get memory.
 */
bool_t
clear_buffer(buffer)
Buffer	*buffer;
{
    /*
     * Free all the lines in the buffer.
     */
    throw(buffer->b_line0);
    return(setup_buffer(buffer));
}

/*
 * Allocate and initialise a buffer structure.
 *
 * Don't touch filenames.
 *
 * Returns TRUE for success, FALSE if we couldn't get memory.
 */
static bool_t
setup_buffer(b)
Buffer	*b;
{
    /*
     * Allocate the single "dummy" line, and the two
     * out-of-bounds lines for lines 0 and (n+1).
     * This is a little strange ...
     */
    b->b_line0 = newline(0);
    b->b_file = newline(1);
    b->b_lastline = newline(0);
    if (b->b_line0 == NULL || b->b_file == NULL || b->b_lastline == NULL) {
	return(FALSE);
    }

    /*
     * Connect everything togther to form a minimal list.
     */
    b->b_line0->l_next = b->b_file;
    b->b_file->l_prev = b->b_line0;
    b->b_file->l_next = b->b_lastline;
    b->b_lastline->l_prev = b->b_file;

    /*
     * Clear all marks.
     */
    init_marks(b);

    /*
     * Clear the undo status of the buffer.
     */
    init_undo(b);

    /*
     * Clear all flags, and then pay special attention to the
     * "noedit" flag which must be set if the parameter is FALSE.
     */
    b->b_flags = 0;
    if (!Pb(P_edit)) {
	b->b_flags |= FL_NOEDIT;
    }

    return(TRUE);
}
