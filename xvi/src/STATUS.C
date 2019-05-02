/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)status.c	2.1 (Chris & John Downey) 7/29/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    status.c
* module function:
    Routines to print status line messages.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

/*
 * Set up the "slinetype" field in the given buffer structure,
 * according to the number of columns available.
 */
void
init_sline(win)
Xviwin	*win;
{
    flexclear(&win->w_statusline);
}

/*VARARGS2*/
/*PRINTFLIKE*/
void
show_message
#ifdef	__STDC__
    (Xviwin *window, char *format, ...)
#else /* not __STDC__ */
    (window, format, va_alist)
    Xviwin	*window;
    char	*format;
    va_dcl
#endif	/* __STDC__ */
{
    va_list	argp;

    VA_START(argp, format);
    (void) flexclear(&window->w_statusline);
    (void) vformat(&window->w_statusline, format, argp);
    va_end(argp);

    update_sline(window);
}

/*VARARGS2*/
/*PRINTFLIKE*/
void
show_error
#ifdef	__STDC__
    (Xviwin *window, char *format, ...)
#else /* not __STDC__ */
    (window, format, va_alist)
    Xviwin	*window;
    char	*format;
    va_dcl
#endif	/* __STDC__ */
{
    va_list	argp;

    beep(window);
    VA_START(argp, format);
    (void) flexclear(&window->w_statusline);
    (void) vformat(&window->w_statusline, format, argp);
    va_end(argp);

    update_sline(window);
}

void
show_file_info(window)
Xviwin	*window;
{
    if (echo & e_SHOWINFO) {
	Buffer	*buffer;
	Flexbuf	*slp;
	long	position, total;
	long	percentage;

	buffer = window->w_buffer;

	position = lineno(buffer, window->w_cursor->p_line);
	total = lineno(buffer, buffer->b_lastline->l_prev);
	percentage = (total > 0) ? (position * 100) / total : 0;

	slp = &window->w_statusline;
	flexclear(slp);
	if (buffer->b_filename == NULL) {
	    (void) lformat(slp, "No File ");
	} else {
	    (void) lformat(slp, "\"%s\" ", buffer->b_filename);
	}
	(void) lformat(slp, "%s%s%sline %ld of %ld (%ld%%)",
		is_readonly(buffer) ? "[Read only] " : "",
		not_editable(buffer) ? "[Not editable] " : "",
		is_modified(buffer) ? "[Modified] " : "",
		position,
		total,
		percentage);
    }
    update_sline(window);
}
