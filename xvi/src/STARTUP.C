/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)startup.c	2.3 (Chris & John Downey) 9/4/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    main.c
* module function:
    Entry point for xvi; setup, argument parsing and signal handling.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

/*
 * References to the current cursor position, and the window
 * and buffer into which it references. These make the code a
 * lot simpler, but we have to be a bit careful to update them
 * whenever necessary.
 */
Buffer	*curbuf;
Xviwin	*curwin;

/*
 * Global variables.
 */
state_t		State = NORMAL;	/* This is the current state of the command */
				/* interpreter. */

unsigned	echo;		/*
				 * bitmap controlling the verbosity of
				 * screen output (see xvi.h for details).
				 */

int		indentchars;	/* number of chars indented on current line */

volatile unsigned char
		kbdintr;	/*
				 * global flag set when a keyboard interrupt
				 * is received
				 */

bool_t		imessage;	/*
				 * global flag to indicate whether we should
				 * display the "Interrupted" message
				 */

/*
 * Internal routines.
 */
static	void	usage P((void));

Xviwin *
xvi_startup(vs, argc, argv, envp)
VirtScr	*vs;
int	argc;
char	*argv[];
char	*envp;				/* init string from the environment */
{
    char	*tag = NULL;		/* tag from command line */
    char	*pat = NULL;		/* pattern from command line */
    long	line = -1;		/* line number from command line */
    char	**files;
    int		numfiles = 0;
    int		count;
    char	*env;

    ignore_signals();

    /*
     * Initialise parameter module.
     */
    init_params();

    /*
     * Initialise yank/put module.
     */
    init_yankput();

    /*
     * The critical path this code has to follow is quite tricky.
     * We can't really run the "env" string until after we've set
     * up the first screen window because we don't know what the
     * commands in "env" might do: they might, for all we know,
     * want to display something. And we can't set up the first
     * screen window until we've set up the terminal interface.
     *
     * Also, we can't read the command line arguments until after
     * we've run the "env" string because a "-s param=value" argument
     * should override any setting of that parameter in the environment.
     *
     * All this means that the usage() function, which tells the
     * user that the command line syntax was wrong, can only be
     * called after the display interface has already been set up,
     * which means we must be in visual mode. So usage() has to
     * switch back to system mode (otherwise, on systems where
     * sys_startv() & sys_endv() switch display pages,
     * the user will never see the output of usage()). So ...
     */

    /*
     * Set up the first buffer and screen window.
     * Must call sys_init first.
     */
    curbuf = new_buffer();
    if (curbuf == NULL) {
	sys_endv();
	(void) fputs("Can't allocate buffer memory.\n", stderr);
	sys_exit(2);
    }
    curwin = init_window(vs);
    if (curwin == NULL) {
	sys_endv();
	(void) fputs("Can't allocate buffer memory.\n", stderr);
	sys_exit(2);
    }

    /*
     * Connect the two together.
     */
    map_window_onto_buffer(curwin, curbuf);

    init_sline(curwin);

    /*
     * Save a copy of the passed environment string in case it was
     * obtained from getenv(), so that the subsequent call we make
     * to get the SHELL parameter value does not overwrite it.
     */
    if (envp != NULL) {
	env = strsave(envp);
    } else {
    	env = NULL;
    }

    /*
     * Try to obtain a value for the "shell" parameter from the
     * environment variable SHELL. If this is NULL, do not override
     * any existing value. The system interface code (sys_init()) is
     * free to set up a default value, and the initialisation string
     * in the next part of the startup is free to override both that
     * value and the one from the environment.
     */
    {
	char	*sh;

	sh = getenv("SHELL");
	if (sh != NULL) {
	    set_param(P_shell, sh);
	}
    }

    /*
     * Run any initialisation string passed to us.
     *
     * We can't really do this until we have set up the terminal
     * because we don't know what the initialisation string might do.
     */
    if (env != NULL) {
	register char	*ep;
	register bool_t	escaped = FALSE;

	/*
	 * Commands in the initialization string can be
	 * separated by '|' (or '\n'), but a literal '|' or
	 * '\n' can be escaped by a preceding '\\', so we have
	 * to process the string, looking for all three
	 * characters.
	 */
	for (ep = env; *ep;) {
	    switch (*ep++) {
	    case '\\':
		escaped = TRUE;
		continue;
	    case '|':
	    case '\n':
		if (escaped) {
		    register char *s, *d;

		    for (d = (s = --ep) - 1; (*d++ = *s++) != '\0'; )
			;
		} else {
		    ep[-1] = '\0';
		    do_colon(env, FALSE);
		    env = ep;
		}
		/* fall through ... */
	    default:
		escaped = FALSE;
	    }
	}
	if (ep > env) {
	    do_colon(env, FALSE);
	}
    }

    /*
     * Process the command line arguments.
     *
     * We can't really do this until we have run the "env" string,
     * because "-s param=value" on the command line should override
     * parameter setting in the environment.
     *
     * This is a bit awkward because it means usage() is called
     * when we're already in vi mode (which means, among other
     * things, that display pages may have been swapped).
     */
    for (count = 1;
	 count < argc && (argv[count][0] == '-' || argv[count][0] == '+');
								count++) {

	if (argv[count][0] == '-') {
	    switch (argv[count][1]) {
	    case 't':
		/*
		 * -t tag or -ttag
		 */
		if (numfiles != 0)
		    usage();
		if (argv[count][2] != '\0') {
		    tag = &(argv[count][2]);
		} else if (count < (argc - 1)) {
		    count += 1;
		    tag = argv[count];
		} else {
		    usage();
		}
		break;

	    case 's':
		/*
		 * -s param=value or
		 * -sparam=value
		 */
		if (argv[count][2] != '\0') {
		    argv[count] += 2;
		} else if (count < (argc - 1)) {
		    count += 1;
		} else {
		    usage();
		}
		do_set(curwin, 1, &argv[count], FALSE);
		break;

	    default:
		usage();
	    }

	} else /* argv[count][0] == '+' */ {
	    char	nc;

	    /*
	     * "+n file" or "+/pat file"
	     */
	    if (count >= (argc - 1))
		usage();

	    nc = argv[count][1];
	    if (nc == '/') {
		pat = &(argv[count][2]);
	    } else if (is_digit(nc)) {
		line = atol(&(argv[count][1]));
	    } else if (nc == '\0') {
		line = 0;
	    } else {
		usage();
	    }
	    count += 1;
	    files = &argv[count];
	    numfiles = 1;
	}
    }
    if (numfiles != 0 || tag != NULL) {
	/*
	 * If we found "-t tag", "+n file" or "+/pat file" on
	 * the command line, we don't want to see any more
	 * file names.
	 */
	if (count < argc)
	    usage();
    } else {
	/*
	 * Otherwise, file names are valid.
	 */
	numfiles = argc - count;
	if (numfiles > 0) {
	    files = &(argv[count]);
	}
    }

    /*
     * Initialise the cursor and top of screen pointers
     * to the start of the first buffer. Note that the
     * bottom of screen pointer is also set up, as some
     * code (e.g. move_window_to_cursor) depends on it.
     */
    curwin->w_topline = curbuf->b_file;
    curwin->w_botline = curbuf->b_file->l_next;
    move_cursor(curwin, curbuf->b_file, 0);
    curwin->w_col = 0;
    curwin->w_row = 0;

    /*
     * Clear the window.
     *
     * It doesn't make sense to do this until we have a value for
     * Pn(P_colour).
     */
    clear(curwin);

    if (numfiles != 0) {
	if (line < 0 && pat == NULL)
	    echo = e_CHARUPDATE | e_SHOWINFO;

	do_next(curwin, numfiles, files, FALSE);

	if (pat != NULL) {
	    echo = e_CHARUPDATE | e_SHOWINFO | e_REGERR | e_NOMATCH;
	    (void) dosearch(curwin, pat, '/');
	} else if (line >= 0) {
	    echo = e_CHARUPDATE | e_SHOWINFO;
	    do_goto((line > 0) ? line : MAX_LINENO);
	}

    } else if (tag != NULL) {
	echo = e_CHARUPDATE | e_SHOWINFO | e_REGERR | e_NOMATCH;
	if (do_tag(curwin, tag, FALSE, TRUE, FALSE) == FALSE) {
	    /*
	     * Failed to find tag - wait for a while
	     * to allow user to read tags error and then
	     * display the "no file" message.
	     */
	    sleep(2);
	    show_file_info(curwin);
	}
    } else {
	echo = e_CHARUPDATE | e_SHOWINFO;
	show_file_info(curwin);
    }

    setpcmark(curwin);

    echo = e_CHARUPDATE;

    /*
     * Ensure we are at the right screen position.
     */
    move_window_to_cursor(curwin);

    /*
     * Draw the screen.
     */
    update_all();

    /*
     * Update the cursor position on the screen, and go there.
     */
    cursupdate(curwin);
    wind_goto(curwin);

    /*
     * Allow everything.
     */
    echo = e_ANY;

    catch_signals();

    if (env != NULL) {
	free(env);
    }

    return(curwin);
}

/*
 * Print usage message and die.
 *
 * This function is only called after we have set the terminal to vi
 * mode.
 *
 * The system interface functions have to ensure that it's safe to
 * call sys_exit() when sys_endv() has already been called (& there
 * hasn't necessarily been any intervening sys_startv()).
 */
static void
usage()
{
    sys_endv();
    (void) fputs("Usage: xvi { options } [ file ... ]\n", stderr);
    (void) fputs("       xvi { options } -t tag\n", stderr);
    (void) fputs("       xvi { options } +[num] file\n", stderr);
    (void) fputs("       xvi { options } +/pat  file\n", stderr);
    (void) fputs("\nOptions are:\n", stderr);
    (void) fputs("       -s [no]boolean-parameter\n", stderr);
    (void) fputs("       -s parameter=value\n", stderr);
    sys_exit(1);
}
