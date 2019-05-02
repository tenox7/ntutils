/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)unix.c	2.1 (Chris & John Downey) 7/29/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    unix.c
* module function:
    System interface routines for all versions of UNIX.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

#ifndef SIGINT
#   include <signal.h>		/* get signals for call_shell() */
#endif

#ifdef	BSD
#   include <sys/wait.h>	/* get wait stuff for call_shell() */
    typedef	union wait Wait_t;
#else
    typedef	int Wait_t;
#endif

/*
 * CTRL is defined by sgtty.h (or by a file it includes)
 * so we undefine it here to avoid conflicts with the
 * version defined in "xvi.h".
 */
#undef	CTRL

#ifdef	sun
#   ifndef TERMIOS
#	define	TERMIOS
#   endif
#endif

#ifdef TERMIOS
#   ifndef TERMIO
#	define	TERMIO
#   endif
#endif

#ifdef	TERMIO
#   ifdef	TERMIOS
#	include <termios.h>
   typedef struct termios	Termstate;
#	define getstate(p)	((void) tcgetattr(0, (p)))
#	define setstate(p)	((void) tcsetattr(0, TCSANOW, (p)))
#	define w_setstate(p)	((void) tcsetattr(0, TCSADRAIN, (p)))
#   else	/* no TERMIOS */
#	include <termio.h>
   typedef struct termio	Termstate;
#	define getstate(p)	((void) ioctl(0,TCGETA,(char *)(p)))
#	define setstate(p)	((void) ioctl(0,TCSETA,(char *)(p)))
#	define w_setstate(p)	((void) ioctl(0,TCSETAW,(char *)(p)))
#   endif	/* no TERMIOS */

    /*
     * Table of line speeds ... exactly 16 long, and the CBAUD mask
     * is 017 (i.e. 15) so we will never access outside the array.
     */
    short	speeds[] = {
	/* B0 */	0,
	/* B50 */	50,
	/* B75 */	75,
	/* B110 */	110,
	/* B134 */	134,
	/* B150 */	150,
	/* B200 */	200,
	/* B300 */	300,
	/* B600 */	600,
	/* B1200 */	1200,
	/* B1800 */	1800,
	/* B2400 */	2400,
	/* B4800 */	4800,
	/* B9600 */	9600,
	/* EXTA */	19200,		/* not defined at V.2 */
	/* EXTB */	38400,		/* not defined at V.2 */
    };

#else	/* not TERMIO */

#   include <sgtty.h>
    typedef struct sgttyb	Termstate;

    static	struct	tchars	ckd_tchars, raw_tchars;
    static	struct	ltchars	ckd_ltchars, raw_ltchars;

#   ifdef FD_SET
#	define	fd_set_type	fd_set
#   else		/* FD_SET not defined */
	/*
	 * BSD 4.2 doesn't have these macros.
	 */
	typedef int fd_set_type;
#	define	FD_ZERO(p)	(*(p) = 0)
#	define	FD_SET(f,p)	(*(p) |= (1 << (f)))
#   endif		/* FD_SET not defined */
#endif	/* not TERMIO */

static	Termstate	cooked_state, raw_state;

#undef	CTRL

#ifdef	SETVBUF_AVAIL
    /*
     * Output buffer to save function calls.
     */
    static	char	outbuffer[128];
#endif

#ifdef MEMTEST
#   include <sys/resource.h>
#endif		/* MEMTEST */

/*
 * Expected for termcap's benefit.
 */
short		ospeed;			/* tty's baud rate */

/*
 * We sometimes use a lot of system calls while trying to read from
 * the keyboard; these are needed to make our automatic buffer
 * preservation and input timeouts work properly. Nevertheless, it
 * is possible that, with this much overhead, a reasonably fast typist
 * could get ahead of us, so we do a small amount of input buffering
 * to reduce the number of system calls.
 *
 * This variable gives the number of characters in the buffer.
 */
static int	kb_nchars;

/*
 * Get a single byte from the keyboard.
 *
 * If the keyboard input buffer is empty, & read() fails or times out,
 * return EOF.
 */
static int
kbgetc()
{
    static unsigned char	kbuf[48];
    static unsigned char	*kbp;

    if (kb_nchars <= 0) {
	int nread;

	if ((nread = read(0, (char *) kbuf, sizeof kbuf)) <= 0) {
	    return EOF;
	} else {
	    kb_nchars = nread;
	    kbp = kbuf;
	}
    }
    --kb_nchars;
    return(*kbp++);
}

#ifdef TERMIO

/*
 * Set a timeout on standard input. 0 means no timeout.
 *
 * This depends on raw_state having been properly initialized, which
 * should have been done by sys_startv().
 */
static void
input_timeout(msec)
long	msec;
{
    int		vtime;
    static int	lastvtime;

    /*
     * If the device state hasn't been changed since last time, we
     * don't need to do anything.
     */
    if ((vtime = (msec + 99) / 100) != lastvtime) {
	lastvtime = vtime;
	raw_state.c_cc[VMIN] = (vtime == 0 ? 1 : 0);
	raw_state.c_cc[VTIME] = vtime;
	setstate(&raw_state);
    }
}

#endif	/* TERMIO */

/*
 * Get a character from the keyboard.
 *
 * Make sure screen is updated first.
 */
int
inch(timeout)
long	timeout;
{
    int		c;

    /*
     * If we had characters left over from last time, return one.
     *
     * Note that if this happens, we don't call flush_output().
     */
    if (kb_nchars > 0) {
	return(kbgetc());
    }

    /*
     * Need to get a character. First, flush output to the screen.
     */
    flush_output();

#ifdef TERMIO
    if (timeout != 0) {
	input_timeout(timeout);
	c = kbgetc();
	input_timeout(0L);
	return(c);
    }
#else	/* no TERMIO */
    if (timeout != 0) {
	struct timeval	tv;
	fd_set_type	readfds;

	tv.tv_sec = (long) (timeout / 1000);
	tv.tv_usec = ((long) timeout * 1000) % (long) 1000000;

	FD_ZERO(&readfds);
	FD_SET(0, &readfds);

	/*
	 * If select does not return 0, some input is available
	 * (ignoring the possibility of errors). Otherwise, we
	 * timed out, so return EOF.
	 */
	if (select(1, &readfds, (fd_set_type *) NULL,
		    (fd_set_type *) NULL, &tv) == 0) {
	    return(EOF);
	}
    }
#endif	/* no TERMIO */

    /*
     * Keep trying until we get at least one character.
     */
    while ((c = kbgetc()) == EOF)
	;

    return(c);
}

#if !(defined(__STDC__) || (defined(ultrix) && defined(mips)))
/*
 * If we have ANSI C, we should have strerror() anyway. Also, Ultrix
 * on DECStations provides it.
 */
const char *
strerror(err)
int	err;
{
    extern char	*sys_errlist[];
    extern int	sys_nerr;

    return(
	err == 0 ?
	    "No error"
	    :
	    (err > 0 && err < sys_nerr) ?
		sys_errlist[err]
		:
		"Unknown error"
    );
}

#endif /* !(defined(__STDC__) || (defined(ultrix) && defined(mips))) */

static int
runvp(args)
char	**args;
{
    int		pid;
    Wait_t	status;

    pid = fork();
    switch (pid) {
    case -1:		/* fork() failed */
	return(-1);

    case 0:			/* this is the child */
	(void) signal(SIGINT, SIG_DFL);
	(void) signal(SIGQUIT, SIG_DFL);
	(void) execvp(args[0], args);

	/*
	 * Couldn't do it ... use standard output functions here
	 * because none of xvi's higher-level functions are usable
	 * from this module.
	 */
	(void) fputs("\007Can't execute ", stdout);
	(void) fputs(args[0], stdout);
	(void) fputs("\n(", stdout);
	(void) fputs(strerror(errno), stdout);
	(void) fputs(")\nHit return to continue", stdout);
	(void) fflush(stdout);
	(void) getc(stdin);
	exit(1);

    default:		/* this is the parent */
	while (wait(&status) != pid)
	    ;
	return(0);
    }
}

int
call_shell(sh)
char	*sh;
{
    static char	*args[] = { NULL, NULL };

    args[0] = sh;
    return(runvp(args));
}

int
call_system(command)
char	*command;
{
    static char	*args[] = { NULL, "-c", NULL, NULL };

    if (Ps(P_shell) == NULL) {
	(void) puts("\007Can't execute command without SHELL parameter");
	return(-1);
    }
    args[0] = Ps(P_shell);
    args[2] = command;
    return(runvp(args));
}

#ifdef ITIMER_REAL
    static int
    nothing()
    {
	return(0);
    }
#endif

/*
 * Delay for a short time - preferably less than 1 second.
 * This is for use by showmatch, which wants to hold the
 * cursor over the matching bracket for just long enough
 * that it will be seen.
 */
void
delay()
{
#ifdef ITIMER_REAL
    struct itimerval	timer;
    
    (void) signal(SIGALRM, nothing);

    /*
     * We want to pause for 200 msec (1/5th of a second) here,
     * as this seems like a reasonable figure. Note that we can
     * assume that the implementation will have defined tv_usec
     * of a type large enough to hold up to 999999, since that's
     * the largest number of microseconds we can possibly need.
     */
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 200000;
    if (setitimer(ITIMER_REAL, &timer, (struct itimerval *) NULL) == -1)
	return;
    (void) pause();

    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    (void) setitimer(ITIMER_REAL, &timer, (struct itimerval *) NULL);
#else /* not ITIMER_REAL */
    sleep(1);
#endif /* not ITIMER_REAL */
}

/*
 * Initialise the terminal so that we can do single-character
 * input and output, with no strange mapping, and no echo of
 * input characters.
 *
 * This must be done before any screen-based i/o is performed.
 */
void
sys_init()
{
#ifdef	TIOCGWINSZ
    struct winsize	winsz;	/* for getting window wize */
#endif

    /*
     * What the device driver thinks the window's dimensions are.
     */
    unsigned int	rows = 0;
    unsigned int	columns = 0;

#ifdef MEMTEST
    {
	static struct rlimit dlimit = { 400 * 1024, 400 * 1024 };

	(void) setrlimit(RLIMIT_DATA, &dlimit);
    }
#endif		/* MEMTEST */

    /*
     * Set up tty flags in raw and cooked structures.
     * Do this before any termcap-initialisation stuff
     * so that start sequences can be sent.
     */
#ifdef	TERMIO

    getstate(&cooked_state);
    raw_state = cooked_state;
#   ifdef   POSIX
	raw_state.c_oflag &= ~(OPOST
#	ifdef	ONLCR
				| ONLCR
#	endif
#	ifdef	OXTABS
				| OXTABS
#	endif
			    );
	raw_state.c_iflag &= ~(ICRNL | IGNCR | INLCR);
	raw_state.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL
#	ifdef	ECHOCTL
				| ECHOCTL
#	endif
#	ifdef	ECHOPRT
				| ECHOPRT
#	endif
#	ifdef	ECHOKE
				| ECHOKE
#	endif
			    );
#   else    /* not POSIX */
	/*
	 * Assume this is a Sun, but it might not be ...
	 */
	raw_state.c_oflag &= ~(OLCUC | ONLCR | OCRNL | ONLRET | XTABS);
	raw_state.c_iflag &= ~(ICRNL | IGNCR | INLCR);
	raw_state.c_lflag &= ~(ICANON | XCASE | ECHO | ECHOE | ECHOK |
		   ECHONL | ECHOCTL | ECHOPRT | ECHOKE);
#   endif
    raw_state.c_cc[VMIN] = 1;
    raw_state.c_cc[VTIME] = 0;
#   ifdef	TERMIOS
#	ifdef	    VDISCARD
	    raw_state.c_cc[VDISCARD] = 0;
#	endif
	raw_state.c_cc[VSUSP] = 0;
#   endif	/* TERMIOS */

#else	/* not TERMIO */

    /*
     * Probably a BSD system; we must
     *	turn off echo,
     *	set cbreak mode (not raw because we lose
     *		typeahead when switching modes),
     *	turn off tab expansion on output (in case termcap
     *		puts out any tabs in cursor motions etc),
     *	turn off CRMOD so we get \r and \n as they are typed,
     * and
     *	turn off nasty interrupt characters like ^Y so that
     *		we get the control characters we want.
     *
     * All this has to be put back as it was when we go into
     * system mode; a pain, but we have to get it right.
     */
    (void) ioctl(0, TIOCGETP, (char *) &cooked_state);
    raw_state = cooked_state;
    raw_state.sg_flags |= CBREAK;
    raw_state.sg_flags &= ~ (ECHO | XTABS | CRMOD);

    (void) ioctl(0, TIOCGETC, (char *) &ckd_tchars);
    raw_tchars = ckd_tchars;
    raw_tchars.t_quitc = -1;

    (void) ioctl(0, TIOCGLTC, (char *) &ckd_ltchars);
    raw_ltchars = ckd_ltchars;
    raw_ltchars.t_flushc = -1;
    raw_ltchars.t_lnextc = -1;
    raw_ltchars.t_suspc = -1;
    raw_ltchars.t_dsuspc = -1;

#endif	/* not TERMIO */

#ifdef	SETVBUF_AVAIL
    /*
     * Set output buffering to avoid calling _flsbuf()
     * for every character sent to the screen.
     */
    setvbuf(stdout, outbuffer, _IOFBF, sizeof(outbuffer));
#endif

    /*
     * This is for termcap's benefit.
     */
#ifdef	TERMIO
#   ifdef	POSIX
	ospeed = cooked_state.c_ospeed;
#   else	/* not POSIX */
	ospeed = speeds[cooked_state.c_cflag & CBAUD];
#   endif
#else
    ospeed = cooked_state.sg_ospeed;
#endif

#ifdef	TIOCGWINSZ
    /*
     * Find out how big the kernel thinks the window is.
     * These values (if they are non-zero) will override
     * any settings obtained by the terminal interface code.
     */
    (void) ioctl(0, TIOCGWINSZ, (char *) &winsz);
    rows = winsz.ws_row;
    columns = winsz.ws_col;
#else
    rows = columns = 0;
#endif

    /*
     * Now set up the terminal interface.
     */
    tty_open(&rows, &columns);

    /*
     * Go into raw/cbreak mode, and do any initialisation stuff.
     */
    sys_startv();
}

static enum { m_SYS = 0, m_VI = 1 } curmode;

/*
 * Set terminal into the mode it was in when we started.
 *
 * sys_endv() can be called when we're already in system mode, so we
 * have to check.
 */
void
sys_endv()
{
    if (curmode == m_SYS)
	return;
    tty_goto((int) Rows - 1, 0);
    set_colour(Pn(P_systemcolour));
    erase_line();
    tty_endv();

    (void) fflush(stdout);

    /*
     * Restore terminal modes.
     */
#ifdef	TERMIO
    w_setstate(&cooked_state);
#else
    (void) ioctl(0, TIOCSETN, (char *) &cooked_state);
    (void) ioctl(0, TIOCSETC, (char *) &ckd_tchars);
    (void) ioctl(0, TIOCSLTC, (char *) &ckd_ltchars);
#endif
    curmode = m_SYS;
}

/*
 * Set terminal to raw/cbreak mode.
 */
void
sys_startv()
{
    if (curmode == m_VI)
	return;
#ifdef	TERMIO
    w_setstate(&raw_state);
#else
    (void) ioctl(0, TIOCSETN, (char *) &raw_state);
    (void) ioctl(0, TIOCSETC, (char *) &raw_tchars);
    (void) ioctl(0, TIOCSLTC, (char *) &raw_ltchars);
#endif

    tty_startv();

    set_colour(Pn(P_colour));

    curmode = m_VI;
}

/*
 * "Safe" form of exit - doesn't leave the tty in yillruction mode.
 */
void
sys_exit(code)
int	code;
{
    sys_endv();
    tty_close();
    (void) fclose(stdin);
    (void) fclose(stdout);
    (void) fclose(stderr);

    /*
     * This is desperation really.
     * On some BSD systems, calling exit() here produces a core dump.
     */
    _exit(code);
}

/*
 * This is a routine that can be passed to tputs() (in the termcap
 * library): it does the same thing as putchar().
 */
void
foutch(c)
int	c;
{
    putchar(c);
}

/*
 * Construct unique name for temporary file, to be used as a backup
 * file for the named file.
 */
char *
tempfname(srcname)
char	*srcname;
{
    char	tailname[MAXNAMLEN + 1];
    char	*srctail;
    char	*endp;
    char	*retp;
    unsigned	headlen;
    unsigned	rootlen;
    unsigned	indexnum = 0;

    if ((srctail = strrchr(srcname, '/')) == NULL) {
	srctail = srcname;
    } else {
	srctail++;
    }
    headlen = srctail - srcname;

    /*
     * Make copy of filename's tail & change it from "wombat" to
     * "#wombat.tmp".
     */
    tailname [0] = '#';
    (void) strncpy(& tailname [1], srctail, sizeof tailname - 1);
    tailname [sizeof tailname - 1] = '\0';
    endp = tailname + strlen(tailname);

    /*
     * Don't let name get too long.
     */
    if (endp > & tailname [sizeof tailname - 5]) {
	endp = & tailname [sizeof tailname - 5];
    }
    rootlen = endp - tailname;

    /*
     * Now allocate space for new pathname.
     */
    retp = alloc(headlen + rootlen + 5);
    if (retp == NULL) {
	return(NULL);
    }

    /*
     * Copy name to new storage, leaving space for ".tmp"
     * (or ".xxx") suffix ...
     */
    if (headlen > 0) {
	(void) memcpy(retp, srcname, (int) headlen);
    }
    (void) memcpy(&retp[headlen], tailname, (int) rootlen);

    /*
     * ... & make endp point to the space we're leaving for the suffix.
     */
    endp = &retp[headlen + rootlen];
    strcpy(endp++, ".tmp");
    while (access(retp, 0) == 0) {
	/*
	 * Keep trying this until we get a unique file name.
	 */
	Flexbuf suffix;

	flexnew(&suffix);
	(void) lformat(&suffix, "%03u", ++indexnum);
	(void) strncpy(endp, flexgetstr(&suffix), 3);
	flexdelete(&suffix);
    }
    return retp;
}

/*
 * Fork off children thus:
 *
 * [CHILD 1] --- pipe1 ---> [CHILD 2] --- pipe2 ---> [PARENT]
 *    |                        |                        |
 *    V                        V                        V
 * writefunc                exec(cmd)                readfunc
 *
 * connecting the pipes to stdin/stdout as appropriate.
 *
 * We assume that on entry to this function, file descriptors 0, 1 and 2
 * are already open, so we do not have to deal with the possibility of
 * them being allocated as pipe descriptors.
 */
bool_t
sys_pipe(cmd, writefunc, readfunc)
char	*cmd;
int	(*writefunc) P((FILE *));
long	(*readfunc) P((FILE *));
{
    int		pd1[2], pd2[2];		/* descriptors for pipes 1 and 2 */
    int		pid1, pid2;		/* process ids for children 1 and 2 */
    int		died;
    int		retval;
    FILE	*fp;
    Wait_t	status;
    static char	*args[] = { NULL, "-c", NULL, NULL };

    if (Ps(P_shell) == NULL) {
	return(FALSE);
    }
    args[0] = Ps(P_shell);
    args[2] = cmd;

    /*
     * Initialise these values so we can work out what has happened
     * so far if we have to goto fail for any reason.
     */
    pd1[0] = pd1[1] = pd2[0] = pd2[1] = -1;
    pid1 = pid2 = -1;

    if (pipe(pd1) == -1) {
	goto fail;
    }

    switch (pid1 = fork()) {
    case -1:					/* error */
	goto fail;

    case 0:					/* child 1 */
	/*
	 * Fdopen pipe1 to get a stream.
	 */
	(void) close(pd1[0]);
	fp = fdopen(pd1[1], "w");
	if (fp == NULL) {
	    exit(1);
	}

	/*
	 * Call writefunc.
	 */
	(void) (*writefunc)(fp);
	(void) fclose(fp);

	/*
	 * Our work is done.
	 */
	exit(0);

    default:					/* parent */
	(void) close(pd1[1]);
	break;
    }

    if (pipe(pd2) == -1) {
	goto fail;
    }
    switch (pid2 = fork()) {
    case -1:					/* error */
	goto fail;

    case 0:						/* child 2 */

	/*
	 * Rearrange file descriptors onto stdin/stdout/stderr.
	 */
	(void) close(pd1[1]);
	(void) close(pd2[0]);

	(void) close(0);
	(void) dup(pd1[0]);		/* dup2(pd1[0], 0) */
	(void) close(pd1[0]);

	(void) close(1);
	(void) dup(pd2[1]);		/* dup2(pd2[1], 1) */
	(void) close(pd2[1]);

	(void) close(2);
	(void) dup(1);			/* dup2(1, 2) */
	
	/*
	 * Exec the command.
	 */
	(void) execvp(args[0], args);
	exit(1);

    default:					/* parent */
	(void) close(pd1[0]);
	(void) close(pd2[1]);
	pd1[0] = pd2[1] = -1;
	fp = fdopen(pd2[0], "r");
	if (fp == NULL) {
	    goto fail;
	}
	(void) (*readfunc)(fp);
	(void) fclose(fp);
	break;
    }

    /*
     * Finally, clean up the children.
     */
    retval = TRUE;
    goto cleanup;

fail:
    /*
     * Come here if anything fails (if we are the original process).
     * Close any open pipes and goto cleanup to reap the child processes.
     */
    if (pd1[0] >= 0) {
	(void) close(pd1[0]);
	(void) close(pd1[1]);
    }
    if (pd2[0] >= 0) {
	(void) close(pd2[0]);
	(void) close(pd2[1]);
    }
    retval = FALSE;

cleanup:
    /*
     * Come here whether or not we failed, to clean up the children ...
     */
#ifdef	WIFEXITED
#   define	FAILED(s)	(!WIFEXITED(s) || (s).w_retcode != 0)
#else
#   define	FAILED(s)	((s) != 0)
#endif
#ifndef WTERMSIG
#   ifdef   WIFSIGNALED
#	define	WTERMSIG(s)	(WIFSIGNALED(s) ? (s).w_termsig : 0)
#   else
#	define	WTERMSIG(s)	((s) & 0177)
#   endif
#endif
    while ((died = wait(&status)) != -1) {
	if (died == pid1 || died == pid2) {
	    /*
	     * If child 1 was killed with SIGPIPE -
	     * because child 2 exited before reading all
	     * of its input - it isn't necessarily an
	     * error.
	     */
	    if (
		FAILED(status)
		&&
		(
		    died == pid2
		    ||
		    WTERMSIG(status) != SIGPIPE
		)
	    ) {
		retval = FALSE;
	    }
	}
    }

    return(retval);
}

char *
fexpand(name)
char	*name;
{
    static char	meta[] = "*?[]~${}`";
    char	*cp;
    int		pd[2];
    int		has_meta;

    has_meta = FALSE;
    for (cp = meta; *cp != '\0'; cp++) {
	if (strchr(name, *cp) != NULL) {
	    has_meta = TRUE;
	    break;
	}
    }
    if (!has_meta) {
	return(name);
    }

    if (Ps(P_shell) == NULL) {
	return(name);
    }
    if (pipe(pd) == -1) {
	return(name);
    }
    fflush(stdout);
    switch (fork()) {
    case -1:				/* error */
	return(name);

    case 0:				/* child */
    {
	static char	*args[] = {
	    NULL,			/* path of shell */
	    "-c",
	    NULL,			/* echo %s */
	    NULL,
	};
	Flexbuf		cmd;
	int		errout;

	args[0] = Ps(P_shell);
	flexnew(&cmd);
	(void) lformat(&cmd, "echo %s", name);
	args[2] = flexgetstr(&cmd);
	(void) fclose(stdout);
	(void) fclose(stderr);
	(void) close(pd[0]);
	while (dup(pd[1]) < 2)		/* redirect both stdout & stderr */
	    ;
	(void) close(pd[1]);
	(void) close(0);
	(void) execvp(args[0], args);
	puts(name);
	exit(0);
    }
    default:				/* parent */
    {
	Wait_t		status;
	FILE		*pfp;
	static Flexbuf	newname;
	register int	c;

	flexclear(&newname);
	(void) close(pd[1]);
	pfp = fdopen(pd[0], "r");
	if (pfp != NULL) {
	    while ((c = getc(pfp)) != EOF && c != '\n') {
		if (!flexaddch(&newname, c)) {
		    break;
		}
	    }
	}
	(void) fclose(pfp);
	(void) wait(&status);
	if (flexempty(&newname)) {
	    return(name);
	}
	return(flexgetstr(&newname));
    }
    }
}
