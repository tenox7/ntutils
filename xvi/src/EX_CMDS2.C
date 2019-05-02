/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)ex_cmds2.c	2.2 (Chris & John Downey) 8/3/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    ex_cmds2.c
* module function:
    Command functions for miscellaneous ex (colon) commands.
    See ex_cmds1.c for file- and buffer-related colon commands.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

#ifdef	MEGAMAX
overlay "ex_cmds2"
#endif

/*
 * Run shell command.
 *
 * You might think it would be easier to do this as
 *
 *	sys_endv();
 *	system(command);
 *	sys_startv();
 *	prompt("[Hit return to continue] ");
 *	...
 *
 * but the trouble is that, with some systems, sys_startv() can be
 * used to swap display pages, which would mean that the user would
 * never have time to see the output from the command. (This applies
 * to some terminals, as well as machines with memory-mappped video;
 * cmdtool windows on Sun workstations also do the same thing.)
 *
 * This means we have to display the prompt before calling
 * sys_startv(), so we just use good old fputs(). (We're trying not to
 * use printf()).
 */
/*ARGSUSED*/
void
do_shcmd(window, command)
Xviwin	*window;
char	*command;
{
    int	c;

    sys_endv();

    (void) fputs(command, stdout);
    (void) fputs("\r\n", stdout);
    (void) fflush(stdout);
    (void) call_system(command);
    (void) fputs("[Hit return to continue] ", stdout);
    (void) fflush(stdout);
    while ((c = getc(stdin)) != '\n' && c != '\r' && c != EOF)
	;

    sys_startv();
    redraw_screen();
}

void
do_shell(window)
Xviwin	*window;
{
    char	*sh = NULL;
    int	sysret;

    sh = Ps(P_shell);
    if (sh == NULL) {
	show_error(window, "SHELL variable not set");
	return;
    }

    sys_endv();

    sysret = call_shell(sh);

    sys_startv();
    redraw_screen();

    if (sysret != 0) {
#ifdef STRERROR_AVAIL
	show_error(window, "Can't execute %s [%s]", sh,
			(errno > 0 ? strerror(errno) : "Unknown Error"));
#else	/* strerror() not available */
	show_error(window, "Can't execute %s", sh);
#endif	/* strerror() not available */
    }
}

/*ARGSUSED*/
void
do_suspend(window)
Xviwin	*window;
{
    if (State == NORMAL) {
#	ifdef	SIGSTOP
	extern	int	kill P((int, int));
	extern	int	getpid P((void));

	sys_endv();

	(void) kill(getpid(), SIGSTOP);

	sys_startv();
	redraw_screen();

#	else /* SIGSTOP */

	/*
	 * Can't suspend unless we're on a BSD UNIX system;
	 * just pretend by invoking a shell instead.
	 */
	do_shell(window);

#	endif /* SIGSTOP */
    }
}

void
do_equals(window, line)
Xviwin	*window;
Line	*line;
{
    if (line == NULL) {
	/*
	 * Default position is ".".
	 */
	line = window->w_cursor->p_line;
    }

    show_message(window, "Line %ld", lineno(window->w_buffer, line));
}

void
do_help(window)
Xviwin	*window;
{
    unsigned	savecho;

    savecho = echo;
    echo &= ~(e_SCROLL | e_REPORT | e_SHOWINFO);
    if (Ps(P_helpfile) != NULL && do_buffer(window, Ps(P_helpfile))) {
	/*
	 * We use "curbuf" here because the new buffer will
	 * have been made the current one by do_buffer().
	 */
	curbuf->b_flags |= FL_READONLY | FL_NOEDIT;
	move_window_to_cursor(curwin);
	show_file_info(curwin);
	update_window(curwin);
    }
    echo = savecho;
}

bool_t
do_source(interactive, file)
bool_t	interactive;
char	*file;
{
    static char		cmdbuf[256];
    FILE		*fp;
    register int	c;
    register char	*bufp;
    bool_t		literal;

    fp = fopen(file, "r");
    if (fp == NULL) {
	if (interactive) {
	    show_error(curwin, "Can't open \"%s\"", file);
	}
	return(FALSE);
    }

    bufp = cmdbuf;
    literal = FALSE;
    while ((c = getc(fp)) != EOF) {
	if (!literal && (c == CTRL('V') || c == '\n')) {
	    switch (c) {
	    case CTRL('V'):
		literal = TRUE;
		break;

	    case '\n':
		if (kbdintr) {
		    imessage = TRUE;
		    break;
		}
		if (bufp > cmdbuf) {
		    *bufp = '\0';
		    do_colon(cmdbuf, FALSE);
		}
		bufp = cmdbuf;
		break;
	    }
	} else {
	    literal = FALSE;
	    if (bufp < &cmdbuf[sizeof(cmdbuf) - 1]) {
		*bufp++ = c;
	    }
	}
    }
    (void) fclose(fp);
    return(TRUE);
}

/*
 * Change directory.
 *
 * With a NULL argument, change to the directory given by the HOME
 * environment variable if it is defined; with an argument of "-",
 * change back to the previous directory (like ksh).
 *
 * Return NULL pointer if OK, otherwise error message to say
 * what went wrong.
 */
char *
do_chdir(dir)
    char	*dir;
{
    static char	*homedir = NULL;
    static char	*prevdir = NULL;
    char	*ret;
    char	*curdir;

    if (dir == NULL && homedir == NULL &&
    				(homedir = getenv("HOME")) == NULL) {
	return("HOME environment variable not set");
    }

    if (dir == NULL) {
	dir = homedir;
    } else if (*dir == '-' && dir[1] == '\0') {
	if (prevdir == NULL) {
	    return("No previous directory");
	} else {
	    dir = prevdir;
	}
    }

    curdir = malloc(MAXPATHLEN + 2);
    if (curdir != NULL && getcwd(curdir, MAXPATHLEN + 2) == NULL) {
	free(curdir);
	curdir = NULL;
    }
    ret = (chdir(dir) == 0 ? NULL : "Invalid directory");
    if (prevdir) {
	free(prevdir);
	prevdir = NULL;
    }
    if (curdir) {
	prevdir = realloc(curdir, (unsigned) strlen(curdir) + 1);
    }
    return(ret);
}

void
do_cdmy(type, l1, l2, destline)
int	type;			/* one of [cdmy] */
Line	*l1, *l2;		/* start and end (inclusive) of range */
Line	*destline;		/* destination line for copy/move */
{
    Posn	p1, p2;
    Posn	destpos;

    p1.p_line = (l1 != NULL) ? l1 : curwin->w_cursor->p_line;
    p2.p_line = (l2 != NULL) ? l2 : p1.p_line;
    p1.p_index = p2.p_index = 0;

    if (type == 'c' || type == 'm') {
	if (destline == NULL) {
	    show_error(curwin, "No destination specified");
	    return;
	}
    }

    /*
     * Check that the destination is not inside
     * the source range for "move" operations.
     */
    if (type == 'm') {
	unsigned long	destlineno;

	destlineno = lineno(curbuf, destline);
	if (destlineno >= lineno(curbuf, p1.p_line) &&
			    destlineno <= lineno(curbuf, p2.p_line)) {
	    show_error(curwin, "Source conflicts with destination of move");
	    return;
	}
    }

    /*
     * Yank the text to be copied.
     */
    if (do_yank(curbuf, &p1, &p2, FALSE, '@') == FALSE) {
	show_error(curwin, "Not enough memory to yank text");
	return;
    }

    if (!start_command(curwin)) {
	return;
    }

    switch (type) {
    case 'd':			/* delete */
    case 'm':			/* move */
	move_cursor(curwin, p1.p_line, 0);
	repllines(curwin, p1.p_line, cntllines(p1.p_line, p2.p_line),
						(Line *) NULL);
	update_buffer(curbuf);
	cursupdate(curwin);
	begin_line(curwin, TRUE);
    }

    switch (type) {
    case 'c':			/* copy */
    case 'm':			/* move */
	/*
	 * And put it back at the destination point.
	 */
	destpos.p_line = destline;
	destpos.p_index = 0;
	do_put(curwin, &destpos, FORWARD, '@');

	update_buffer(curbuf);
	cursupdate(curwin);
    }

    end_command(curwin);
}
