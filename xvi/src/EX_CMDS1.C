/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)ex_cmds1.c	2.1 (Chris & John Downey) 7/29/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    ex_cmds1.c
* module function:
    File, window and buffer-related command functions
    for ex (colon) commands.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

#ifdef	MEGAMAX
overlay "ex_cmds1"
#endif

static	char	**files;	/* list of input files */
static	int	numfiles;	/* number of input files */
static	int	curfile;	/* number of the current file */

char		*altfilename;
static	long	altfileline;

static	char	nowrtmsg[] = "No write since last change (use ! to override)";
static	char	nowrtbufs[] = "Some buffers not written (use ! to override)";

static	bool_t	more_files P((void));

void
do_quit(window, force)
Xviwin	*window;
bool_t	force;
{
    Xviwin	*wp;
    bool_t	changed;
    bool_t	canexit;

    if (force) {
	canexit = TRUE;
    } else {
	/*
	 * See if any buffers remain modified and unwritten.
	 */
	changed = FALSE;
	wp = window;
	do {
	    if (is_modified(wp->w_buffer)) {
		changed = TRUE;
	    }
	} while ((wp = next_window(wp)) != window);

	if (changed) {
	    show_error(window, nowrtbufs);
	    canexit = FALSE;
	} else {
	    canexit = ! more_files();
	}
    }

    if (canexit) {
	sys_exit(0);
    }
}

/*
 * Split the current window into two, leaving both windows mapped
 * onto the same buffer.
 */
void
do_split_window(window)
Xviwin	*window;
{
    Xviwin	*newwin;

    newwin = split_window(window);
    if (newwin == NULL) {
	show_error(window, "No more windows!");
	return;
    }

    map_window_onto_buffer(newwin, window->w_buffer);

    /*
     * Update the status line of the old window
     * (since it will have been moved).
     * Also update the window - this will almost certainly
     * have no effect on the screen, but is necessary.
     */
    show_file_info(window);
    update_window(window);

    /*
     * Show the new window.
     */
    init_sline(newwin);
    update_window(newwin);
    show_file_info(newwin);

    /*
     * Update the global window variable.
     */
    curwin = newwin;
}

/*
 * Open a new buffer window, with a possible filename arg.
 *
 * do_buffer() is responsible for updating the screen image for the
 * old window, but not the new one, since we may want to move to a
 * different location in the new buffer (e.g. for a tag search).
 */
bool_t
do_buffer(window, filename)
Xviwin	*window;
char	*filename;
{
    Buffer	*buffer;
    Buffer	*new;
    Xviwin	*newwin;

    buffer = window->w_buffer;

    if (window->w_nrows < (MINROWS + 1) * 2) {
	show_error(window, "Not enough room!");
	return(FALSE);
    }

    new = new_buffer();
    if (new == NULL) {
	show_error(window, "No more buffers!");
	return(FALSE);
    }
    newwin = split_window(window);
    if (newwin == NULL) {
	free_buffer(new);
	show_error(window, "No more windows!");
	return(FALSE);
    }

    map_window_onto_buffer(newwin, new);

    /*
     * Update the status lines of each buffer.
     *
     * Even if (echo & e_SHOWINFO) is turned off, show_file_info()
     * will always call update_sline(), which is what we really
     * need here.
     *
     * Note that we don't need to call move_window_to_cursor() for
     * the old window until it becomes the current window again.
     */
    show_file_info(window);
    init_sline(newwin);

    if (filename != NULL) {
	(void) do_edit(newwin, FALSE, filename);
    } else {
	new->b_filename = new->b_tempfname = NULL;
	show_file_info(newwin);
    }

    update_window(window);

    /*
     * The current buffer (a global variable) has
     * to be updated here. No way around this.
     */
    curbuf = new;
    curwin = newwin;

    return(TRUE);
}

/*
 * "close" (the current window).
 */
void
do_close_window(win, force)
Xviwin	*win;
bool_t	force;
{
    Buffer	*buffer;
    Xviwin	*best;

    buffer = win->w_buffer;

    if (is_modified(buffer) && !force && buffer->b_nwindows < 2) {
	/*
	 * Don't close a modified buffer.
	 */
	show_error(win, nowrtmsg);
    } else if (next_window(win) != win || !more_files()) {
	Xviwin	*w;

	/*
	 * We can close this window if:
	 *
	 * (
	 *	the buffer has not been modified
	 *  or	they are forcing the close
	 *  or	there are other windows onto this buffer
	 * )
	 * AND
	 * (
	 *	there are other windows still open
	 *  or	there are no more files to be edited
	 * )
	 */

	/*
	 * Find an adjacent window to take up the screen
	 * space used by the one being closed.
	 */
	best = NULL;
	for (w = next_window(win); w != win; w = next_window(w)) {

	    if (w->w_cmdline + 1 == win->w_winpos ||
		w->w_winpos - 1 == win->w_cmdline ||
		w->w_nrows == 0) {

		/*
		 * We have found an adjacent window;
		 * if it is the first such, or if
		 * it is smaller than the previous
		 * best, it is now the new best.
		 */
		if (best == NULL || w->w_nrows < best->w_nrows) {
		    best = w;
		}
	    }
	}

	if (best == NULL) {
	    sys_exit(0);
	}

	if (buffer->b_nwindows == 1 && buffer->b_filename != NULL) {
	    /*
	     * Before we free the buffer, save its filename.
	     */
	    if (altfilename != NULL) {
		free(altfilename);
	    }
	    altfilename = buffer->b_filename;
	    buffer->b_filename = NULL;
	    altfileline = lineno(buffer,
		    win->w_cursor->p_line);
	}

	/*
	 * Now "best" points  to the smallest adjacent window;
	 * amalgamate the spaces used.
	 */
	if (best->w_winpos > win->w_winpos) {
	    best->w_winpos = win->w_winpos;
	}
	best->w_nrows += win->w_nrows;
	best->w_cmdline = best->w_winpos + best->w_nrows - 1;
	free_window(win);

	if (buffer->b_nwindows == 0) {
	    free_buffer(buffer);
	}

	/*
	 * Have to update the globals "curbuf" and "curwin" here.
	 */
	curwin = best;
	curbuf = best->w_buffer;
	{
	    unsigned	savecho;

	    savecho = echo;
	    /*
	     * Adjust position of new current window
	     * within buffer before updating it, to avoid
	     * wasting screen output - but don't do any
	     * scrolling at this stage because the old
	     * window is still on the screen.
	     */
	    echo &= ~(e_CHARUPDATE | e_SHOWINFO | e_SCROLL);
	    move_window_to_cursor(curwin);
	    echo = savecho;

	}
	update_window(curwin);
	show_file_info(curwin);
    }
}

/*
 * Close current window.
 *
 * If it is the last window onto the buffer, also close the buffer.
 *
 * If the buffer has been modified, we must write it out before closing it.
 */
void
do_xit(window)
Xviwin	*window;
{
    Buffer	*buffer;

    buffer = window->w_buffer;

    if (is_modified(buffer) && buffer->b_nwindows < 2) {
	if (buffer->b_filename != NULL) {
	    if (!writeit(window, buffer->b_filename,
				(Line *) NULL, (Line *) NULL, FALSE)) {
		return;
	    }
	} else {
	    show_error(window, "No output file");
	    return;
	}
    }

    do_close_window(window, FALSE);
}

/*
 * Edit the given filename in the given buffer,
 * replacing any current contents.  Note that the
 * screen is not updated, since there are routines
 * which use this function before moving the cursor
 * to a different position in the file.
 *
 * Returns TRUE for success, FALSE for failure.
 */
bool_t
do_edit(window, force, arg)
Xviwin	*window;
bool_t	force;
char	*arg;
{
    long	line = 1;		/* line # to go to in new file */
    long	nlines;			/* no of lines read from file */
    Line	*head;			/* start of list of lines */
    Line	*tail;			/* last element of list of lines */
    bool_t	readonly;		/* true if cannot write file */
    Buffer	*buffer;
    Xviwin	*wp;

    buffer = window->w_buffer;

    if (!force && is_modified(buffer)) {
	show_error(window, nowrtmsg);
	return(FALSE);
    }

    if (arg == NULL || arg[0] == '\0') {
	/*
	 * No filename specified; we must already have one.
	 */
	if (buffer->b_filename == NULL) {
	    show_error(window, "No filename");
	    return(FALSE);
	}
    } else /* arg != NULL */ {
	/*
	 * Filename specified.
	 */

	/*
	 * First detect a ":e" on the current file. This is mainly
	 * for ":ta" commands where the destination is within the
	 * current file.
	 */
	if (buffer->b_filename != NULL &&
	    strcmp(arg, buffer->b_filename) == 0) {
	    if (!is_modified(buffer) || (is_modified(buffer) && !force)) {
		return(TRUE);
	    }
	}

	/*
	 * Detect an edit of the alternate file, and set
	 * the line number.
	 */
	if (altfilename != NULL && strcmp(arg, altfilename) == 0) {
	    line = altfileline;
	}

	/*
	 * Save the name of the previous file.
	 * If the strsave() of the new filename
	 * fails, we will have lost the previous
	 * value of altfilename. What a shame.
	 */
	if (buffer->b_filename != NULL) {
	    if (altfilename != NULL)
		free(altfilename);
	    altfilename = strsave(buffer->b_filename);
	    altfileline = lineno(buffer, window->w_cursor->p_line);
	}

	/*
	 * Edit a named file.
	 */
	buffer->b_filename = strsave(arg);
	if (buffer->b_filename == NULL)
	    return(FALSE);
	if (buffer->b_tempfname != NULL)
	    free(buffer->b_tempfname);
	buffer->b_tempfname = NULL;
    }

    /*
     * Clear out the old buffer and read the file.
     */
    if (clear_buffer(buffer) == FALSE) {
	show_error(window, "Out of memory");
	return(FALSE);
    }

    /*
     * Be sure to re-map all window structures onto the buffer,
     * in order to eliminate any pointers into the old buffer.
     */
    wp = window;
    do {
	if (wp->w_buffer != buffer)
	    continue;

	unmap_window(wp);
	map_window_onto_buffer(wp, buffer);

    } while ((wp = next_window(wp)) != window);

    readonly = Pb(P_readonly) || !can_write(buffer->b_filename);

    nlines = get_file(window, buffer->b_filename, &head, &tail,
				    (readonly ? " [Read only]" : ""),
				    " [New file]");

    update_sline(window);		/* ensure colour is updated */

    if (nlines == gf_NEWFILE) {	/* no such file */
	return(FALSE);
    } else if (nlines >= 0) {
	unsigned savecho;

	/*
	 * Success.
	 */
	if (readonly) {
	    buffer->b_flags |= FL_READONLY;
	} else {
	    buffer->b_flags &= ~FL_READONLY;
	}

	if (nlines == 0) {	/* empty file */
	    return(TRUE);
	}

	/*
	 * We have successfully read the file in,
	 * so now we must link it into the buffer.
	 */
	replbuffer(window, head);

	move_cursor(window, gotoline(buffer, line), 0);
	begin_line(window, TRUE);
	setpcmark(window);

	/*
	 * We only call update_window() here because we want
	 * window->w_botline to be updated; we don't let it do any
	 * actual screen updating, for the reason explained above.
	 */
	savecho = echo;
	echo &= ~(e_CHARUPDATE | e_SCROLL | e_REPORT | e_SHOWINFO);
	update_window(window);
	echo = savecho;

	return(TRUE);
    } else {
	/*
	 * We failed to read in the file. An appropriate
	 * message will already have been printed by
	 * get_file() (or alloc()).
	 */

	if (buffer->b_filename != NULL)
	    free(buffer->b_filename);
	if (buffer->b_tempfname != NULL)
	    free(buffer->b_tempfname);
	buffer->b_filename = buffer->b_tempfname = NULL;
	return(FALSE);
    }
}

void
do_args(window)
Xviwin	*window;
{
    register char	*tmpbuf;
    int		count;
    register int	curpos = 0;

    if (numfiles == 0) {
	show_message(window, "No files");
	return;
    }

    tmpbuf = alloc((unsigned) window->w_ncols + 1);
    if (tmpbuf == NULL) {
	return;
    }

    for (count = 0; count < numfiles; count++) {
	register char	*sp;

	if (count == curfile && curpos < window->w_ncols)
	    tmpbuf[curpos++] =  '[';
	for (sp = files[count]; curpos < window->w_ncols &&
		    (tmpbuf[curpos] = *sp++) != '\0'; curpos++) {
	    ;
	}
	if (count == curfile && curpos < window->w_ncols)
	    tmpbuf[curpos++] =  ']';
	if (curpos < window->w_ncols)
	    tmpbuf[curpos++] =  ' ';
    }
    tmpbuf[curpos < window->w_ncols ? curpos : window->w_ncols] =  '\0';

    show_message(window, "%s", tmpbuf);

    free(tmpbuf);
}

/*
 * Change the current file list to the one specified, or edit the next
 * file in the current file list, or edit the next file in the list if
 * no argument is given.
 */
void
do_next(window, argc, argv, force)
Xviwin	*window;
int	argc;
char	*argv[];
bool_t	force;
{
    unsigned	savecho;

    savecho = echo;
    if (argc > 0) {
	int	count;

	/*
	 * Arguments given - this means a new set of filenames.
	 */
	if (!force && is_modified(window->w_buffer)) {
	    show_error(window, nowrtmsg);
	    return;
	}

	/*
	 * There were no files before, so start from square one.
	 */
	if (numfiles == 0) {
	    files = (char **) alloc((unsigned) argc * sizeof(char *));
	    if (files == NULL) {
		return;
	    }
	} else {
	    /*
	     * We can change the existing list of files.
	     * Free up all the individual filenames
	     * which we got last time.
	     */
	    for (count = 0; count < numfiles; count++) {
		free(files[count]);
	    }
	    if (argc != numfiles) {
		files = (char **) realloc((char *) files,
				    (unsigned) argc * sizeof(char *));
		if (files == NULL) {
		    numfiles = 0;
		    return;
		}
	    }
	}

	/*
	 * Now record all the new filenames.
	 */
	for (count = 0; count < argc; count++) {
	    files[count] = strsave(argv[count]);
	    if (files[count] == NULL) {
		/*
		 * Aargh. Failed half-way through.
		 * Clean up the mess ...
		 */
		while (--count >= 0)
		    free(files[count]);
		free((char *) files);
		files = NULL;
		numfiles = 0;
		return;
	    }
	}
	numfiles = argc;
	curfile = 0;

	/*
	 * And try to edit the first few of them.
	 *
	 * In this case, we don't want report() or
	 * show_file_info() to be called, because otherwise
	 * the messages printed by get_file() won't be seen.
	 */
	echo &= ~(e_SCROLL | e_REPORT | e_SHOWINFO);

	(void) do_edit(curwin, force, files[0]);

	/*
	 * This is not very good because it
	 * doesn't split the screen evenly for
	 * autosplit > 2.  However, it will
	 * just have to do for the moment.
	 */

	/*
	 * Update the current window before
	 * creating any new ones.
	 */
	move_window_to_cursor(curwin);

	while ((curfile + 1) < numfiles && can_split()) {
	    bool_t	success;

	    success = do_buffer(curwin, files[++curfile]);
	    /*
	     * Make sure move_window_to_cursor() is called
	     * for every window before calling
	     * update_buffer().
	     */
	    move_window_to_cursor(curwin);
	    if (!success)
		break;
	}
	update_window(curwin);

    } else if ((curfile + 1) < numfiles) {
	/*
	 * No arguments; this is the normal usage, and
	 * indicates we should edit the next file in the list.
	 * Don't grab the next file if the current one is
	 * modified and not written, or we will "lose"
	 * files from the list.
	 */
	if (!force && is_modified(window->w_buffer)) {
	    show_error(window, nowrtmsg);
	    return;
	}

	/*
	 * Just edit the next file.
	 */
	echo &= ~(e_SCROLL | e_REPORT | e_SHOWINFO);
	(void) do_edit(window, force, files[++curfile]);
	move_window_to_cursor(window);
	update_buffer(window->w_buffer);
    } else {
	show_message(window, "No more files");
    }
    echo = savecho;
}

/*ARGSUSED*/
void
do_rewind(window, force)
Xviwin	*window;
bool_t	force;
{
    unsigned	savecho;

    if (numfiles <= 1)		/* nothing to rewind */
	return;

    curfile = 0;

    savecho = echo;
    echo &= ~(e_SCROLL | e_REPORT | e_SHOWINFO);
    (void) do_edit(window, force, files[0]);
    move_window_to_cursor(window);
    update_buffer(window->w_buffer);
    echo = savecho;
}

/*
 * Write out the buffer, to the given filename,
 * from "line1" to "line2", forcing if necessary.
 *
 * If no filename given, use the buffer's filename.
 */
bool_t
do_write(window, filename, l1, l2, force)
Xviwin	*window;
char	*filename;
Line	*l1, *l2;
bool_t	force;
{
    if (filename == NULL) {
	filename = window->w_buffer->b_filename;
    }

    if (filename == NULL) {
	show_error(window, "No output file");
	return(FALSE);
    } else {
	return(writeit(window, filename, l1, l2, force));
    }
}

/*
 * Write to the given filename then quit.
 */
void
do_wq(window, filename, force)
Xviwin	*window;
char	*filename;
bool_t	force;
{
    if (do_write(window, filename, (Line *) NULL, (Line *) NULL, force)) {
	do_quit(window, force);	
    }
}

/*
 * Read the given file into the buffer after the specified line.
 * The line may not be NULL, but should be a line in the buffer
 * referenced by the passed window parameter.
 */
void
do_read(window, filename, atline)
Xviwin	*window;
char	*filename;
Line	*atline;
{
    Line	*head;		/* start of list of lines */
    Line	*tail;		/* last element of list of lines */
    long	nlines;		/* number of lines read */

    nlines = get_file(window, filename, &head, &tail, "", " No such file");

    /*
     * If nlines > 0, we need to insert the lines returned into
     * the buffer. Otherwise, either the file is empty or an error
     * message has already been printed: in either case, we don't
     * need to do anything.
     */
    if (nlines > 0) {
	/*
	 * We want to see the message printed by
	 * get_file() here, not the message printed by
	 * report().
	 */
	echo &= ~e_REPORT;
	repllines(window, atline->l_next, 0L, head);
	echo |= e_REPORT;
	update_buffer(window->w_buffer);

	/*
	 * Move the cursor to the first character
	 * of the file we just read in.
	 */
	move_cursor(window, atline->l_next, 0);
	begin_line(window, TRUE);
    }
}

/*
 * Edit alternate file. Called when control-^ is typed.
 */
void
do_alt_edit(window)
Xviwin	*window;
{
    if (altfilename == NULL) {
	show_error(window, "No alternate file to edit");
    } else {
	if (do_buffer(window, altfilename)) {
	    move_window_to_cursor(curwin);
	    update_window(curwin);
	}
    }
}

void
do_compare()
{
    Xviwin		*w;
    enum mvtype	incres;
    Posn		pos1, pos2;

    w = next_window(curwin);
    if (w == curwin) {
	show_error(curwin, "No other buffers to compare");
    } else if (w->w_buffer == curbuf) {
	show_error(curwin, "Next window has same buffer");
    } else {
	pos1 = *(curwin->w_cursor);
	pos2 = *(w->w_cursor);
	while ((incres = inc(&pos1)) == inc(&pos2)) {
	    if (incres == mv_EOL) {
		continue;
	    } else if (incres == mv_NOMOVE) {
		(void) dec(&pos1);
		(void) dec(&pos2);
		break;
	    } else {
		if (gchar(&pos1) != gchar(&pos2)) {
		    break;
		}
	    }
	}
	if (gchar(&pos1) == '\0' && pos1.p_index > 0) {
	    (void) dec(&pos1);
	}
	if (gchar(&pos2) == '\0' && pos2.p_index > 0) {
	    (void) dec(&pos2);
	}
	move_cursor(curwin, pos1.p_line, pos1.p_index);
	move_cursor(w, pos2.p_line, pos2.p_index);
	move_window_to_cursor(w);
	cursupdate(w);
	wind_goto(w);
    }
}

static bool_t
more_files()
{
    int	n;

    n = numfiles - (curfile + 1);
    if (n > 0) {
	show_error(curwin, "%d more file%s to edit", n, (n > 1) ? "s" : "");
	return(TRUE);
    } else {
	return(FALSE);
    }
}
