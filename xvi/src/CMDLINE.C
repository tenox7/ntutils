/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)cmdline.c	2.2 (Chris & John Downey) 8/6/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    cmdline.c
* module function:
    Command-line handling (i.e. :/? commands) - most
    of the actual command functions are in ex_cmds.c.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews	(onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

#ifdef	MEGAMAX
overlay "cmdline"
#endif

/*
 * The next two variables contain the bounds of any range
 * given in a command. If no range was given, both will be NULL.
 * If only a single line was given, u_line will be NULL.
 * The a_line variable is used for those commands which take
 * a third line specifier after the command, e.g. "move", "copy".
 */
static	Line	*l_line, *u_line;
static	Line	*a_line;

/*
 * Definitions for all ex commands.
 */

#define	EX_ENOTFOUND	-1		/* command not found */
#define	EX_EAMBIGUOUS	-2		/* could be more than one */
#define	EX_ECANTFORCE	-3		/* ! given where not appropriate */
#define	EX_EBADARGS	-4		/* inappropriate args given */

#define	EX_NOCMD	1
#define	EX_SHCMD	2
#define	EX_UNUSED	3	/* unused */
#define	EX_AMPERSAND	4
#define	EX_EXBUFFER	5
#define	EX_LSHIFT	6
#define	EX_EQUALS	7
#define	EX_RSHIFT	8
#define	EX_COMMENT	9
#define	EX_ABBREVIATE	10
#define	EX_APPEND	11
#define	EX_ARGS		12
#define	EX_BUFFER	13
#define	EX_CHDIR	14
#define	EX_CHANGE	15
#define	EX_CLOSE	16
#define	EX_COMPARE	17
#define	EX_COPY		18
#define	EX_DELETE	19
#define	EX_ECHO		20
#define	EX_EDIT		21
#define	EX_EX		22
#define	EX_FILE		23
#define	EX_GLOBAL	24
#define	EX_HELP		25
#define	EX_INSERT	26
#define	EX_JOIN		27
#define	EX_K		28
#define	EX_LIST		29
#define	EX_MAP		30
#define	EX_MARK		31
#define	EX_MOVE		32
#define	EX_NEXT		33
#define	EX_NUMBER	34
#define	EX_OPEN		35
#define	EX_PRESERVE	36
#define	EX_PRINT	37
#define	EX_PUT		38
#define	EX_QUIT		39
#define	EX_READ		40
#define	EX_RECOVER	41
#define	EX_REWIND	42
#define	EX_SET		43
#define	EX_SHELL	44
#define	EX_SOURCE	45
#define	EX_SPLIT	46
#define	EX_SUSPEND	47
#define	EX_SUBSTITUTE	48
#define	EX_TAG		49
#define	EX_UNABBREV	50
#define	EX_UNDO		51
#define	EX_UNMAP	52
#define	EX_V		53
#define	EX_VERSION	54
#define	EX_VISUAL	55
#define	EX_WN		56
#define	EX_WQ		57
#define	EX_WRITE	58
#define	EX_XIT		59
#define	EX_YANK		60
#define	EX_Z		61
#define	EX_GOTO		62
#define	EX_TILDE	63

/*
 * Table of all ex commands, and whether they take an '!'.
 *
 * Note that this table is in strict order, sorted on
 * the ASCII value of the first character of the command.
 *
 * The priority field is necessary to resolve clashes in
 * the first one or two characters; so each group of commands
 * beginning with the same letter should have at least one
 * priority 1, so that there is a sensible default.
 *
 * Commands with argument type ec_rest need no delimiters;
 * they need only be matched. This is really only used for
 * single-character commands like !, " and &.
 */
static	struct	ecmd	{
    char	*ec_name;
    short	ec_command;
    short	ec_priority;
    unsigned	ec_flags;
    /*
     * Flags: EXCLAM means can use !, FILEXP means do filename
     * expansion, INTEXP means do % and # expansion. EXPALL means
     * do INTEXP and FILEXP (they are done in that order).
     *
     * EC_RANGE0 means that the range specifier (if any)
     * may include line 0.
     */
#   define	EC_EXCLAM	0x1
#   define	EC_FILEXP	0x2
#   define	EC_INTEXP	0x4
#   define	EC_EXPALL	EC_FILEXP|EC_INTEXP
#   define	EC_RANGE0	0x8

    enum {
	ec_none,		/* no arguments after command */
	ec_strings,		/* whitespace-separated strings */
	ec_1string,		/* like ec_strings but only one */
	ec_line,		/* line number or target argument */
	ec_rest,		/* rest of line passed entirely */
	ec_nonalnum,		/* non-alphanumeric delimiter */
	ec_1lower		/* single lower-case letter */
    }	ec_arg_type;
} cmdtable[] = {
/*  name		command	     priority	exclam	*/

    /*
     * The zero-length string is used for the :linenumber command.
     */
    "",		    EX_NOCMD,	    1,	EC_RANGE0,		ec_none,
    "!",	    EX_SHCMD,	    0,	EC_INTEXP,		ec_rest,

    "#",	    EX_NUMBER,	    0,	0,			ec_none,
    "&",	    EX_AMPERSAND,   0,	0,			ec_rest,
    "*",	    EX_EXBUFFER,    0,	0,			ec_rest,
    "<",	    EX_LSHIFT,	    0,	0,			ec_none,
    "=",	    EX_EQUALS,	    0,	0,			ec_none,
    ">",	    EX_RSHIFT,	    0,	0,			ec_none,
    "@",	    EX_EXBUFFER,    0,	0,			ec_rest,
    "\"",	    EX_COMMENT,	    0,	0,			ec_rest,

    "abbreviate",   EX_ABBREVIATE,  0,	0,			ec_strings,
    "append",	    EX_APPEND,	    1,	0,			ec_none,
    "args",	    EX_ARGS,	    0,	0,			ec_none,

    "buffer",	    EX_BUFFER,	    0,	EC_EXPALL,		ec_1string,

    "cd",	    EX_CHDIR,	    1,	EC_EXPALL,		ec_1string,
    "change",	    EX_CHANGE,	    2,	0,			ec_none,
    "chdir",	    EX_CHDIR,	    1,	EC_EXPALL,		ec_1string,
    "close",	    EX_CLOSE,	    1,	EC_EXCLAM,		ec_none,
    "compare",	    EX_COMPARE,	    0,	0,			ec_none,
    "copy",	    EX_COPY,	    1,	0,			ec_line,

    "delete",	    EX_DELETE,	    0,	0,			ec_none,

    "echo",	    EX_ECHO,	    0,	EC_INTEXP,		ec_strings,
    "edit",	    EX_EDIT,	    1,	EC_EXCLAM|EC_EXPALL,	ec_1string,
    "ex",	    EX_EX,	    0,	EC_EXPALL,		ec_1string,

    "file",	    EX_FILE,	    0,	EC_EXPALL,		ec_1string,

    "global",	    EX_GLOBAL,	    0,	EC_EXCLAM,		ec_nonalnum,

    "help",	    EX_HELP,	    0,	0,			ec_none,

    "insert",	    EX_INSERT,	    0,	0,			ec_none,

    "join",	    EX_JOIN,	    0,	0,			ec_none,

    "k",	    EX_K,	    0,	0,			ec_1lower,

    "list",	    EX_LIST,	    0,	0,			ec_none,

    "map",	    EX_MAP,	    0,	EC_EXCLAM,		ec_strings,
    "mark",	    EX_MARK,	    0,	0,			ec_1lower,
    "move",	    EX_MOVE,	    1,	0,			ec_line,

    "next",	    EX_NEXT,	    1,	EC_EXCLAM|EC_EXPALL,	ec_strings,
    "number",	    EX_NUMBER,	    0,	0,			ec_none,

    "open",	    EX_OPEN,	    0,	0,			ec_none,

    "preserve",	    EX_PRESERVE,    0,	0,			ec_none,
    "print",	    EX_PRINT,	    1,	0,			ec_none,
    "put",	    EX_PUT,	    0,	EC_RANGE0,		ec_none,

    "quit",	    EX_QUIT,	    0,	EC_EXCLAM,		ec_none,

    "read",	    EX_READ,	    1,	EC_EXPALL|EC_RANGE0,	ec_1string,
    "recover",	    EX_RECOVER,	    0,	0,			ec_none,
    "rewind",	    EX_REWIND,	    0,	EC_EXCLAM,		ec_none,

    "set",	    EX_SET,	    0,	0,			ec_strings,
    "shell",	    EX_SHELL,	    0,	0,			ec_none,
    "source",	    EX_SOURCE,	    0,	EC_EXPALL,		ec_1string,
    "split",	    EX_SPLIT,	    0,	0,			ec_none,
    "stop",	    EX_SUSPEND,	    0,	0,			ec_none,
    "substitute",   EX_SUBSTITUTE,  1,	0,			ec_nonalnum,
    "suspend",	    EX_SUSPEND,	    0,	0,			ec_none,

    "t",	    EX_COPY,	    1,	0,			ec_line,
    "tag",	    EX_TAG,	    0,	EC_EXCLAM,		ec_1string,

    "unabbreviate", EX_UNABBREV,    0,	0,			ec_strings,
    "undo",	    EX_UNDO,	    1,	0,			ec_none,
    "unmap",	    EX_UNMAP,	    0,	EC_EXCLAM,		ec_strings,

    "v",	    EX_V,	    1,	0,			ec_nonalnum,
    "version",	    EX_VERSION,	    0,	0,			ec_none,
    "visual",	    EX_VISUAL,	    0,	EC_EXCLAM|EC_EXPALL,	ec_1string,

    "wn",	    EX_WN,	    0,	EC_EXCLAM,		ec_none,
    "wq",	    EX_WQ,	    0,	EC_EXCLAM|EC_EXPALL,	ec_1string,
    "write",	    EX_WRITE,	    1,	EC_EXCLAM|EC_EXPALL,	ec_1string,

    "xit",	    EX_XIT,	    0,	0,			ec_none,

    "yank",	    EX_YANK,	    0,	0,			ec_none,

    "z",	    EX_Z,	    0,	0,			ec_none,

    "|",	    EX_GOTO,	    0,	0,			ec_none,
    "~",	    EX_TILDE,	    0,	0,			ec_rest,

    NULL,	    0,		    0,	0,			ec_none,
};

/*
 * Internal routine declarations.
 */
static	int	decode_command P((char **, bool_t *, struct ecmd **));
static	bool_t	get_line P((char **, Line **));
static	bool_t	get_range P((char **));
static	void	badcmd P((bool_t, char *));
static	char	*show_line P((void));
static	char	*expand_percents P((char *));

/*
 * These are used for display mode.
 */
static	Line	*curline;
static	Line	*lastline;
static	bool_t	do_line_numbers;

/*
 * Macro to skip over whitespace during command line interpretation.
 */
#define skipblanks(p)	{ while (*(p) != '\0' && is_space(*(p))) (p)++; }

/*
 * do_colon() - process a ':' command.
 *
 * The cmdline argument points to a complete command line to be processed
 * (this does not include the ':' itself).
 */
void
do_colon(cmdline, interactive)
char	*cmdline;			/* optional command string */
bool_t	interactive;			/* true if reading from tty */
{
    char	*arg;			/* ptr to string arg(s) */
    int		argc = 0;		/* arg count for ec_strings */
    char	**argv = NULL;		/* arg vector for ec_strings */
    bool_t	exclam;			/* true if ! was given */
    int		command;		/* which command it is */
    struct ecmd	*ecp;			/* ptr to command entry */
    unsigned	savecho;		/* previous value of echo */

    /*
     * Clear the range variables.
     */
    l_line = NULL;
    u_line = NULL;

    /*
     * Parse a range, if present (and update the cmdline pointer).
     */
    if (!get_range(&cmdline)) {
	return;
    }

    /*
     * Decode the command.
     */
    skipblanks(cmdline);
    command = decode_command(&cmdline, &exclam, &ecp);

    if (command > 0) {
	/*
	 * Check that the range specified,
	 * if any, is legal for the command.
	 */
	if (!(ecp->ec_flags & EC_RANGE0)) {
	    if (l_line == curbuf->b_line0 || u_line == curbuf->b_line0) {
		show_error(curwin,
		    "Specification of line 0 not allowed");
		return;
	    }
	}

	switch (ecp->ec_arg_type) {
	case ec_none:
	    if (*cmdline != '\0' &&
		    (*cmdline != '!' || !(ecp->ec_flags & EC_EXCLAM))) {
		command = EX_EBADARGS;
	    }
	    break;

	case ec_line:
	    a_line = NULL;
	    skipblanks(cmdline);
	    if (!get_line(&cmdline, &a_line) || a_line == NULL) {
		command = EX_EBADARGS;
	    }
	    break;

	case ec_1lower:
	    /*
	     * One lower-case letter.
	     */
	    skipblanks(cmdline);
	    if (!is_lower(cmdline[0]) || cmdline[1] != '\0') {
		command = EX_EBADARGS;
	    } else {
		arg = cmdline;
	    }
	    break;

	case ec_nonalnum:
	case ec_rest:
	case ec_strings:
	case ec_1string:
	    arg = cmdline;
	    if (ecp->ec_arg_type == ec_strings ||
				    ecp->ec_arg_type == ec_1string) {
		if (*arg == '\0') {
		    /*
		     * No args.
		     */
		    arg = NULL;
		} else {
		    /*
		     * Null-terminate the command and skip
		     * whitespace to arg or end of line.
		     */
		    *arg++ = '\0';
		    skipblanks(arg);

		    /*
		     * There was trailing whitespace,
		     * but no args.
		     */
		    if (*arg == '\0') {
			arg = NULL;
		    }
		}
	    } else if (ecp->ec_arg_type == ec_nonalnum && exclam) {
		/*
		 * We don't normally touch the arguments for
		 * this type, but we have to null-terminate
		 * the '!' at least.
		 */
		*arg++ = '\0';
	    }

	    if (arg != NULL) {
		/*
		 * Perform expansions on the argument string.
		 */
		if (ecp->ec_flags & EC_INTEXP) {
		    arg = expand_percents(arg);
		}
		if (ecp->ec_flags & EC_FILEXP) {
		    arg = fexpand(arg);
		}

		if (ecp->ec_arg_type == ec_strings) {
		    makeargv(arg, &argc, &argv, " \t");
		}
	    }
	}
    }

    savecho = echo;

    /*
     * Now do the command.
     */
    switch (command) {
    case EX_SHCMD:
	/*
	 * If a line range was specified, this must be a pipe command.
	 * Otherwise, it's just a simple shell command.
	 */
	if (l_line != NULL) {
	    specify_pipe_range(curwin, l_line, u_line);
	    do_pipe(curwin, arg);
	} else {
	    do_shcmd(curwin, arg);
	}
	break;

    case EX_ARGS:
	do_args(curwin);
	break;

    case EX_BUFFER:
	if (arg != NULL)
	    echo &= ~(e_SCROLL | e_REPORT | e_SHOWINFO);
	(void) do_buffer(curwin, arg);
	move_window_to_cursor(curwin);
	update_window(curwin);
	break;

    case EX_CHDIR:
    {
	char	*error;

	if ((error = do_chdir(arg)) != NULL) {
	    badcmd(interactive, error);
	} else if (interactive) {
	    char	*dirp;

	    if ((dirp = alloc(MAXPATHLEN + 2)) != NULL &&
		getcwd(dirp, MAXPATHLEN + 2) != NULL) {
		show_message(curwin, "%s", dirp);
	    }
	    if (dirp) {
		free(dirp);
	    }
	}
	break;
    }

    case EX_CLOSE:
	do_close_window(curwin, exclam);
	break;

    case EX_COMMENT:		/* This one is easy ... */
	break;

    case EX_COMPARE:
	do_compare();
	break;

    case EX_COPY:
	do_cdmy('c', l_line, u_line, a_line);
	break;

    case EX_DELETE:
	do_cdmy('d', l_line, u_line, (Line *) NULL);
	break;

    case EX_ECHO:			/* echo arguments on command line */
    {
	int	i;

	flexclear(&curwin->w_statusline);
	for (i = 0; i < argc; i++) {
	    if (!lformat(&curwin->w_statusline, "%s ", argv[i])
		    || flexlen(&curwin->w_statusline) >= curwin->w_ncols) {
		break;
	    }
	}
	update_sline(curwin);
	break;
    }

    case EX_EDIT:
    case EX_VISUAL:			/* treat :vi as :edit */
	echo &= ~(e_SCROLL | e_REPORT | e_SHOWINFO);
	(void) do_edit(curwin, exclam, arg);
	move_window_to_cursor(curwin);
	update_buffer(curbuf);
#if 0
	show_file_info(curwin);
#endif
	break;

    case EX_FILE:
	if (arg != NULL) {
	    if (curbuf->b_filename != NULL)
		free(curbuf->b_filename);
	    curbuf->b_filename = strsave(arg);
	    if (curbuf->b_tempfname != NULL) {
		free(curbuf->b_tempfname);
		curbuf->b_tempfname = NULL;
	    }
	}
	show_file_info(curwin);
	break;

    case EX_GLOBAL:
	do_global(curwin, l_line, u_line, arg, !exclam);
	break;

    case EX_HELP:
	do_help(curwin);
	break;

    case EX_MAP:
	xvi_map(argc, argv, exclam, interactive);
	break;

    case EX_UNMAP:
	xvi_unmap(argc, argv, exclam, interactive);
	break;

    case EX_MARK:
    case EX_K:
    {
	Posn	pos;

	pos.p_index = 0;
	if (l_line == NULL) {
	    pos.p_line = curwin->w_cursor->p_line;
	} else {
	    pos.p_line = l_line;
	}
	(void) setmark(arg[0], curbuf, &pos);
	break;
    }

    case EX_MOVE:
	do_cdmy('m', l_line, u_line, a_line);
	break;

    case EX_NEXT:
	/*
	 * do_next() handles turning off the appropriate bits
	 * in echo, & also calls move_window_to_cursor() &
	 * update_buffer() as required, so we don't have to
	 * do any of that here.
	 */
	do_next(curwin, argc, argv, exclam);
	break;

    case EX_PRESERVE:
	if (do_preserve())
	    show_file_info(curwin);
	break;

    case EX_LIST:
    case EX_PRINT:
    case EX_NUMBER:
	if (l_line == NULL) {
	    curline = curwin->w_cursor->p_line;
	    lastline = curline->l_next;
	} else if (u_line ==  NULL) {
	    curline = l_line;
	    lastline = l_line->l_next;
	} else {
	    curline = l_line;
	    lastline = u_line->l_next;
	}
	do_line_numbers = (Pb(P_number) || command == EX_NUMBER);
	disp_init(curwin, show_line, (int) curwin->w_ncols,
				command == EX_LIST || Pb(P_list));
	break;

    case EX_PUT:
    {
	Posn	where;

	if (l_line != NULL) {
	    where.p_index = 0;
	    where.p_line = l_line;
	} else {
	    where.p_index = curwin->w_cursor->p_index;
	    where.p_line = curwin->w_cursor->p_line;
	}
	do_put(curwin, &where, FORWARD, '@');
	break;
    }

    case EX_QUIT:
	do_quit(curwin, exclam);
	break;

    case EX_REWIND:
	do_rewind(curwin, exclam);
	break;

    case EX_READ:
	if (arg == NULL) {
	    badcmd(interactive, "Unrecognized command");
	    break;
	}
	do_read(curwin, arg, (l_line != NULL) ? l_line :
					    curwin->w_cursor->p_line);
	break;

    case EX_SET:
	do_set(curwin, argc, argv, interactive);
	break;

    case EX_SHELL:
	do_shell(curwin);
	break;

    case EX_SOURCE:
	if (arg == NULL) {
	    badcmd(interactive, "Missing filename");
	} else if (do_source(interactive, arg) && interactive) {
	    show_file_info(curwin);
	}
	break;

    case EX_SPLIT:
	/*
	 * "split".
	 */
	do_split_window(curwin);
	break;

    case EX_SUBSTITUTE:
    case EX_AMPERSAND:
    case EX_TILDE:
    {
	long		nsubs;
	register long	(*func) P((Xviwin *, Line *, Line *, char *));

	switch (command) {
	case EX_SUBSTITUTE:
	    func = do_substitute;
	    break;
	case EX_AMPERSAND:
	    func = do_ampersand;
	    break;
	case EX_TILDE:
	    func = do_tilde;
	}

	nsubs = (*func)(curwin, l_line, u_line, arg);
	update_buffer(curbuf);
	cursupdate(curwin);
	begin_line(curwin, TRUE);
	if (nsubs >= Pn(P_report)) {
	    show_message(curwin, "%ld substitution%c",
			nsubs,
			(nsubs > 1) ? 's' : ' ');
	}
	break;
    }

    case EX_SUSPEND:
	do_suspend(curwin);
	break;

    case EX_TAG:
	(void) do_tag(curwin, arg, exclam, TRUE, TRUE);
	break;

    case EX_V:
	do_global(curwin, l_line, u_line, arg, FALSE);
	break;

    case EX_VERSION:
	show_message(curwin, Version);
	break;

    case EX_WN:
	/*
	 * This is not a standard "vi" command, but it
	 * is provided in PC vi, and it's quite useful.
	 */
	if (do_write(curwin, (char *) NULL, (Line *) NULL, (Line *) NULL,
								exclam)) {
	    /*
	     * See comment for EX_NEXT (above).
	     */
	    do_next(curwin, 0, argv, exclam);
#if 0
	    move_window_to_cursor(curwin);
	    update_buffer(curbuf);
#endif
	}
	break;

    case EX_WQ:
	do_wq(curwin, arg, exclam);
	break;

    case EX_WRITE:
	(void) do_write(curwin, arg, l_line, u_line, exclam);
	break;

    case EX_XIT:
	do_xit(curwin);
	break;

    case EX_YANK:
	do_cdmy('y', l_line, u_line, (Line *) NULL);
	break;

    case EX_EXBUFFER:
	yp_stuff_input(curwin, arg[0], FALSE);
	break;

    case EX_EQUALS:
	do_equals(curwin, l_line);
	break;

    case EX_LSHIFT:
    case EX_RSHIFT:
	if (l_line == NULL) {
	    l_line = curwin->w_cursor->p_line;
	}
	if (u_line == NULL) {
	    u_line = l_line;
	}
	tabinout((command == EX_LSHIFT) ? '<' : '>', l_line, u_line);
	begin_line(curwin, TRUE);
	update_buffer(curbuf);
	break;

    case EX_NOCMD:
	/*
	 * If we got a line, but no command, then go to the line.
	 */
	if (l_line != NULL) {
	    if (l_line == curbuf->b_line0) {
		l_line = l_line->l_next;
	    }
	    move_cursor(curwin, l_line, 0);
	    begin_line(curwin, TRUE);
	}
	break;

    case EX_ENOTFOUND:
	badcmd(interactive, "Unrecognized command");
	break;

    case EX_EAMBIGUOUS:
	badcmd(interactive, "Ambiguous command");
	break;

    case EX_ECANTFORCE:
	badcmd(interactive, "Inappropriate use of !");
	break;

    case EX_EBADARGS:
	badcmd(interactive, "Inappropriate arguments given");
	break;

    default:
	/*
	 * Decoded successfully, but unknown to us. Whoops!
	 */
	badcmd(interactive, "Internal error - unimplemented command.");
	break;

    case EX_ABBREVIATE:
    case EX_APPEND:
    case EX_CHANGE:
    case EX_EX:
    case EX_GOTO:
    case EX_INSERT:
    case EX_JOIN:
    case EX_OPEN:
    case EX_RECOVER:
    case EX_UNABBREV:
    case EX_UNDO:
    case EX_Z:
	badcmd(interactive, "Unimplemented command.");
	break;
    }

    echo = savecho;

    if (argc > 0 && argv != NULL) {
	free((char *) argv);
    }
}

/*
 * Find the correct command for the given string, and return it.
 *
 * Updates string pointer to point to 1st char after command.
 *
 * Updates boolean pointed to by forcep according
 * to whether an '!' was given after the command;
 * if an '!' is given for a command which can't take it,
 * this is an error, and EX_ECANTFORCE is returned.
 * For unknown commands, EX_ENOTFOUND is returned.
 * For ambiguous commands, EX_EAMBIGUOUS is returned.
 *
 * Also updates *cmdp to point at entry in command table.
 */
static int
decode_command(str, forcep, cmdp)
char		**str;
bool_t		*forcep;
struct	ecmd	**cmdp;
{
    struct ecmd	*cmdptr;
    struct ecmd	*last_match = NULL;
    bool_t	last_exclam = FALSE;
    int		nmatches = 0;
    char	*last_delimiter = *str;

    for (cmdptr = cmdtable; cmdptr->ec_name != NULL; cmdptr++) {
	char	*name = cmdptr->ec_name;
	char	*strp;
	bool_t	matched;

	strp = *str;

	while (*strp == *name && *strp != '\0') {
	    strp++;
	    name++;
	}

	matched = (
	    /*
	     * we've got a full match, or ...
	     */
	    *strp == '\0'
	    ||
	    /*
	     * ... a whitespace delimiter, or ...
	     */
	    is_space(*strp)
	    ||
	    (
		/*
		 * ... at least one character has been
		 * matched, and ...
		 */
		name > cmdptr->ec_name
		&&
		(
		    (
			/*
			 * ... this command can accept a
			 * '!' qualifier, and we've found
			 * one ...
			 */
			(cmdptr->ec_flags & EC_EXCLAM)
			&&
			*strp == '!'
		    )
		    ||
		    (
			/*
			 * ... or it can take a
			 * non-alphanumeric delimiter
			 * (like '/') ...
			 */
			cmdptr->ec_arg_type == ec_nonalnum
			&&
			!is_alnum(*strp)
		    )
		    ||
		    (
			/*
			 * ... or it doesn't need any
			 * delimiter ...
			 */
			cmdptr->ec_arg_type == ec_rest
		    )
		    ||
		    (
			/*
			 * ... or we've got a full match,
			 * & the command expects a single
			 * lower-case letter as an
			 * argument, and we've got one ...
			 */
			cmdptr->ec_arg_type == ec_1lower
			&&
			*name == '\0'
			&&
			is_lower(*strp)
		    )
		    ||
		    (
			/*
			 * ... or we've got a partial match,
			 * and the command expects a line
			 * specifier as an argument, and the
			 * next character is not alphabetic.
			 */
			cmdptr->ec_arg_type == ec_line
			&&
			!is_alpha(*strp)
		    )
		)
	    )
	);

	if (!matched)
	    continue;

	if (last_match == NULL ||
		(last_match != NULL &&
		 last_match->ec_priority < cmdptr->ec_priority)) {
	    /*
	     * No previous match, or this one is better.
	     */
	    last_match = cmdptr;
	    last_exclam = (*strp == '!');
	    last_delimiter = strp;
	    nmatches = 1;

	    /*
	     * If this is a complete match, we don't
	     * need to search the rest of the table.
	     */
	    if (*name == '\0')
		break;

	} else if (last_match != NULL &&
	       last_match->ec_priority == cmdptr->ec_priority) {
	    /*
	     * Another match with the same priority - remember it.
	     */
	    nmatches++;
	}
    }

    /*
     * For us to have found a good match, there must have been
     * exactly one match at a certain priority, and if an '!'
     * was used, it must be allowed by that match.
     */
    if (last_match == NULL) {
	return(EX_ENOTFOUND);
    } else if (nmatches != 1) {
	return(EX_EAMBIGUOUS);
    } else if (last_exclam && ! (last_match->ec_flags & EC_EXCLAM)) {
	return(EX_ECANTFORCE);
    } else {
	*forcep = last_exclam;
	*str = last_delimiter;
	*cmdp = last_match;
	return(last_match->ec_command);
    }
}

/*
 * get_range - parse a range specifier
 *
 * Ranges are of the form
 *
 * ADDRESS1,ADDRESS2
 * ADDRESS		(equivalent to "ADDRESS,ADDRESS")
 * ADDRESS,		(equivalent to "ADDRESS,.")
 * ,ADDRESS		(equivalent to ".,ADDRESS")
 * ,			(equivalent to ".")
 * %			(equivalent to "1,$")
 *
 * where ADDRESS is
 *
 * /regular expression/ [INCREMENT]
 * ?regular expression? [INCREMENT]
 * $   [INCREMENT]
 * 'x  [INCREMENT]	(where x denotes a currently defined mark)
 * .   [INCREMENT]
 * INCREMENT		(equivalent to . INCREMENT)
 * number [INCREMENT]
 *
 * and INCREMENT is
 *
 * + number
 * - number
 * +			(equivalent to +1)
 * -			(equivalent to -1)
 * ++			(equivalent to +2)
 * --			(equivalent to -2)
 *
 * etc.
 *
 * The pointer *cpp is updated to point to the first character following
 * the range spec. If an initial address is found, but no second, the
 * upper bound is equal to the lower, except if it is followed by ','.
 *
 * Return FALSE if an error occurred, otherwise TRUE.
 */
static bool_t
get_range(cpp)
register char	**cpp;
{
    register char	*cp;
    char		sepchar;

#define skipp	{ cp = *cpp; skipblanks(cp); *cpp = cp; }

    skipp;

    if (*cp == '%') {
	/*
	 * "%" is the same as "1,$".
	 */
	l_line = curbuf->b_file;
	u_line = curbuf->b_lastline->l_prev;
	++*cpp;
	return TRUE;
    }

    if (!get_line(cpp, &l_line)) {
	return FALSE;
    }

    skipp;

    sepchar = *cp;
    if (sepchar != ',' && sepchar != ';') {
	u_line = l_line;
	return TRUE;
    }

    ++*cpp;

    if (l_line == NULL) {
	/*
	 * So far, we've got ",".
	 *
	 * ",address" is equivalent to ".,address".
	 */
	l_line = curwin->w_cursor->p_line;
    }

    if (sepchar == ';') {
	move_cursor(curwin, (l_line != curbuf->b_line0) ?
		    l_line : l_line->l_next, 0);
    }

    skipp;
    if (!get_line(cpp, &u_line)) {
	return FALSE;
    }
    if (u_line == NULL) {
	/*
	 * "address," is equivalent to "address,.".
	 */
	u_line = curwin->w_cursor->p_line;
    }

    /*
     * Check for reverse ordering.
     */
    if (earlier(u_line, l_line)) {
	Line	*tmp;

	tmp = l_line;
	l_line = u_line;
	u_line = tmp;
    }

    skipp;
    return TRUE;
}

static bool_t
get_line(cpp, lpp)
    char		**cpp;
    Line		**lpp;
{
    Line		*pos;
    char		*cp;
    char		c;
    long		lnum;

    cp = *cpp;

    /*
     * Determine the basic form, if present.
     */
    switch (c = *cp++) {

    case '/':
    case '?':
	pos = linesearch(curwin, (c == '/') ? FORWARD : BACKWARD,
				    &cp);
	if (pos == NULL) {
	    return FALSE;
	}
	break;

    case '$':
	pos = curbuf->b_lastline->l_prev;
	break;

	/*
	 * "+n" is equivalent to ".+n".
	 */
    case '+':
    case '-':
	cp--;
	 /* fall through ... */
    case '.':
	pos = curwin->w_cursor->p_line;
	break;

    case '\'': {
	Posn		*lp;

	lp = getmark(*cp++, curbuf);
	if (lp == NULL) {
	    show_error(curwin, "Unknown mark");
	    return FALSE;
	}
	pos = lp->p_line;
	break;
    }
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
	for (lnum = c - '0'; is_digit(*cp); cp++)
	    lnum = lnum * 10 + *cp - '0';

	if (lnum == 0) {
	    pos = curbuf->b_line0;
	} else {
	    pos = gotoline(curbuf, lnum);
	}
	break;
	
    default:
	return TRUE;
    }

    skipblanks(cp);

    if (*cp == '-' || *cp == '+') {
	char	dirchar = *cp++;

	skipblanks(cp);
	if (is_digit(*cp)) {
	    for (lnum = 0; is_digit(*cp); cp++) {
		lnum = lnum * 10 + *cp - '0';
	    }
	} else {
	    for (lnum = 1; *cp == dirchar; cp++) {
		lnum++;
	    }
	}

	if (dirchar == '-')
	    lnum = -lnum;

	pos = gotoline(curbuf, lineno(curbuf, pos) + lnum);
    }

    *cpp = cp;
    *lpp = pos;
    return TRUE;
}

/*
 * This routine is called for ambiguous, unknown,
 * badly defined or unimplemented commands.
 */
static void
badcmd(interactive, str)
bool_t	interactive;
char	*str;
{
    if (interactive) {
	show_error(curwin, str);
    }
}

static char *
show_line()
{
    Line	*lp;

    if (curline == lastline) {
	return(NULL);
    }

    lp = curline;
    curline = curline->l_next;

    if (do_line_numbers) {
	static Flexbuf	nu_line;

	flexclear(&nu_line);
	(void) lformat(&nu_line, NUM_FMT, lineno(curbuf, lp));
	(void) lformat(&nu_line, "%s", lp->l_text);
	return flexgetstr(&nu_line);
    } else {
	return(lp->l_text);
    }
}

static char *
expand_percents(str)
char	*str;
{
    static Flexbuf	newstr;
    register char	*from;
    register bool_t	escaped;

    if (strpbrk(str, "%#") == (char *) NULL) {
	return str;
    }
    flexclear(&newstr);
    escaped = FALSE;
    for (from = str; *from != '\0'; from++) {
	if (!escaped) {
	    if (*from == '%' && curbuf->b_filename != NULL) {
		(void) lformat(&newstr, "%s", curbuf->b_filename);
	    } else if (*from == '#' && altfilename != NULL) {
		(void) lformat(&newstr, "%s", altfilename);
	    } else if (*from == '\\') {
		escaped = TRUE;
	    } else {
		(void) flexaddch(&newstr, *from);
	    }
	} else {
	    if (*from != '%' && *from != '#') {
		(void) flexaddch(&newstr, '\\');
	    }
	    (void) flexaddch(&newstr, *from);
	    escaped = FALSE;
	}
    }
    return flexgetstr(&newstr);
}
