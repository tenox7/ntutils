/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)qnx.c	2.2 (Chris & John Downey) 8/7/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    qnx.c
* module function:
    QNX system interface module.

    Note that this module assumes the C86 compiler,
    which is an ANSI compiler, rather than the standard
    QNX compiler, which is not.

* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

/*
 * QNX-specific include files.
 *
 * <stdio.h> etc. get included by "xvi.h".
 */
#include <io.h>
#include <dev.h>
#include <sys/stat.h>
#include <timer.h>
#include <tcap.h>
#include <tcapkeys.h>
#include <taskmsgs.h>

#include "xvi.h"

#define	PIPE_MASK_1	"/tmp/xvipipe1XXXXXX"
#define	PIPE_MASK_2	"/tmp/xvipipe2XXXXXX"
#define	EXPAND_MASK	"/tmp/xvexpandXXXXXX"

extern	int		tcap_row;
extern	int		tcap_col;

int			can_scroll_area;
void			(*up_func)(int, int, int);
void			(*down_func)(int, int, int);

static	int		disp_inited = 0;
static	unsigned	screen_colour = DEF_COLOUR;
static	int		old_options;

static	bool_t		triggered;

static void
catch_usr1(i)
int	i;
{
    (void) signal(SIGUSR1, SIG_IGN);
    triggered = TRUE;
}

/*
 * inchar() - get a character from the keyboard
 */
int
inchar(timeout)
long	timeout;
{
    register int	c;
    bool_t		setsignal = FALSE;

    /*
     * Set a timeout on input if asked to do so.
     */
    if (timeout != 0) {
	/*
	 * Timeout value is in milliseconds, so we have to convert
	 * to 50 millisecond ticks, rounding up as we do it.
	 */
	(void) signal(SIGUSR1, catch_usr1);
	(void) set_timer(TIMER_SET_EXCEPTION, RELATIVE,
			(int) ((timeout + 19) / 20),
			SIGUSR1,
			(char *) NULL);
	setsignal = TRUE;
    }

    flush_output();
    for (;;) {
	triggered = FALSE;

	c = term_key();

	if (triggered) {
	    return(EOF);
	}

	if (c > 127) {
	    /*
	     * Must be a function key press.
	     */

	    if (State != NORMAL) {
		/*
		 * Function key pressed during insert
		 * or command line mode.
		 */
		alert();
		continue;
	    }

	    switch (c) {
	    case KEY_F1: /* F1 key */
		c = K_HELP;
		break;
	    case KEY_HOME:
		c = 'H';
		break;
	    case KEY_END:
		c = 'L';
		break;
	    case KEY_PAGE_UP:
		c = CTRL('B');
		break;
	    case KEY_PAGE_DOWN:
		c = CTRL('F');
		break;
	    case KEY_UP:
		c = K_UARROW;
		break;
	    case KEY_LEFT:
		c = K_LARROW;
		break;
	    case KEY_RIGHT:
		c = K_RARROW;
		break;
	    case KEY_DOWN:
		c = K_DARROW;
		break;
	    case KEY_WORD_LEFT:
		c = 'b';
		break;
	    case KEY_WORD_RIGHT:
		c = 'w';
		break;
	    case KEY_INSERT:
		c = 'i';
		break;
	    case KEY_DELETE:
		c = 'x';
		break;
	    case KEY_RUBOUT:
		c = '\b';
		break;
	    case KEY_TAB:
		c = '\t';
		break;
	    }
	}
	if (setsignal)
	    (void) signal(SIGUSR1, SIG_IGN);
	return(c);
    }
    /*NOTREACHED*/
}

void
sys_init()
{
    if (!disp_inited) {
	term_load(stdin);
	term_video_on();
    }
    if (strcmp(tcap_entry.term_name, "qnx") == 0) {
	up_func = co_up;
	down_func = co_down;
	can_scroll_area = TRUE;
    } else if (strncmp(tcap_entry.term_name, "vt100", 5) == 0) {
	up_func = vt_up;
	down_func = vt_down;
	can_scroll_area = TRUE;
    } else {
	can_scroll_area = FALSE;
    }

    /*
     * Support different number of rows for VGA text modes.
     */
    if (tcap_entry.term_type == VIDEO_MAPPED) {
	unsigned	console;
	unsigned	row_col;

	console = fdevno(stdout);
	row_col = video_get_size(console);
	Rows = (row_col >> 8) & 0xff;
	Columns = row_col & 0xff;
    }

    sys_startv();
    disp_inited = 1;
}

static	int		pcmode;			/* display mode */

void
sys_exit(r)
int	r;
{
    if (disp_inited) {
	term_cur(Rows - 1, 0);
	sys_endv();
    }
    exit(r);
}

/*
 * Returns TRUE if file does not exist or exists and is writeable.
 */
bool_t
can_write(file)
char	*file;
{
    return(access(file, W_OK) == 0 || access(file, F_OK) != 0);
}

bool_t
exists(file)
char *file;
{
    return(access(file, F_OK) == 0);
}

/*
 * Scroll an area on the console screen up by nlines.
 * The number of lines is always positive, and
 * we can assume we will not be called with
 * parameters which would result in scrolling
 * the entire window.
 *
 * Note that this routine will not be called unless
 * can_scroll_area is TRUE, which is only the case
 * for the console screen (i.e. term_name == "qnx").
 */
void
co_up(start, end, nlines)
int	start, end, nlines;
{
    char	*buf;
    int		i;

    buf = malloc((unsigned) Columns * term_save_image(0, 0, NULL, 0));
    if (buf == NULL)
	return;

    for (i = start; i <= end - nlines; i++) {
	term_save_image(i + nlines, 0, buf, Columns);
	term_restore_image(i, 0, buf, Columns);
    }

    for ( ; i <= end; i++) {
	term_cur(i, 0);
	term_clear(_CLS_EOL);
    }

    free(buf);
}

/*
 * Scroll an area on the console screen down by nlines.
 * The number of lines is always positive, and
 * we can assume we will not be called with
 * parameters which would result in scrolling
 * the entire window.
 *
 * Note that this routine will not be called unless
 * can_scroll_area is TRUE, which is only the case
 * for the console screen (i.e. term_name == "qnx").
 */
void
co_down(start, end, nlines)
int	start, end, nlines;
{
    char	*buf;
    int		i;

    buf = malloc((unsigned) Columns * term_save_image(0, 0, NULL, 0));
    if (buf == NULL)
	return;

    for (i = end; i >= start + nlines; --i) {
	term_save_image(i - nlines, 0, buf, Columns);
	term_restore_image(i, 0, buf, Columns);
    }

    for ( ; i >= start; --i) {
	term_cur(i, 0);
	term_clear(_CLS_EOL);
    }

    free(buf);
}

/*
 * Scroll an area on a vt100 terminal screen up by nlines.
 * The number of lines is always positive, and we can assume
 * we will not be called with parameters which would result
 * in scrolling the entire window.
 */
void
vt_up(start, end, nlines)
int	start, end, nlines;
{
    int		count;

    (void) printf("\0337");				/* save cursor */
    (void) printf("\033[%d;%dr", start + 1, end + 1);	/* set scroll region */
    (void) printf("\033[%d;1H", end + 1);		/* goto bottom left */
    for (count = 0; count < nlines; count++) {
	putchar('\n');
    }
    (void) printf("\033[1;%dr", Rows);			/* set scroll region */
    (void) printf("\0338");				/* restore cursor */
}

/*
 * Scroll an area on a vt100 terminal screen down by nlines.
 * The number of lines is always positive, and we can assume
 * we will not be called with parameters which would result
 * in scrolling the entire window.
 */
void
vt_down(start, end, nlines)
int	start, end, nlines;
{
    int		count;

    (void) printf("\0337");				/* save cursor */
    (void) printf("\033[%d;%dr", start + 1, end + 1);	/* set scroll region */
    (void) printf("\033[%d;1H", start + 1);		/* goto top left */
    for (count = 0; count < nlines; count++) {
	fputs("\033[L", stdout);
    }
    (void) printf("\033[1;%dr", Rows);			/* set scroll region */
    (void) printf("\0338");				/* restore cursor */
}

int
call_shell(sh)
char	*sh;
{
    return(spawnlp(0, sh, sh, (char *) NULL));
}

void
alert()
{
    putchar('\007');
}

void
delay()
{
    (void) set_timer(TIMER_WAKEUP, RELATIVE, 2, 0, NULL);
}

void
outchar(c)
int	c;
{
    char	cbuf[2];

    cbuf[0] = c;
    cbuf[1] = '\0';
    outstr(cbuf);
}

void
outstr(s)
char	*s;
{
    term_type(-1, -1, s, 0, screen_colour);
}

void
flush_output()
{
    (void) fflush(stdout);
}

void
set_colour(c)
int	c;
{
    screen_colour = (unsigned) c;
    term_colour(screen_colour >> 8);
}

void
tty_goto(r, c)
int	r, c;
{
    term_cur(r, c);
}

void
sys_startv()
{
    /*
     * Turn off:
     *	EDIT	to get "raw" mode
     *	ECHO	to stop characters being echoed
     *	MAPCR	to stop mapping of \r into \n
     */
    old_options = get_option(stdin);
    set_option(stdin, old_options & ~(EDIT | ECHO | MAPCR));
}

void
sys_endv()
{
    term_cur(Rows - 1, 0);
    set_colour(Pn(P_systemcolour));
    erase_line();
    flush_output();

    /*
     * This flushes the typeahead buffer so that recall of previous
     * commands will not work - this is desirable because some of
     * the commands typed to vi (i.e. control characters) will
     * have a deleterious effect if given to the shell.
     */
    flush_input(stdin);

    set_option(stdin, old_options);
}

/*
 * Construct unique name for temporary file,
 * to be used as a backup file for the named file.
 */
char *
tempfname(srcname)
char	*srcname;
{
    char	*newname;		/* ptr to allocated new pathname */
    char	*last_slash;		/* ptr to last slash in srcname */
    char	*last_hat;		/* ptr to last slash in srcname */
    int		tail_length;		/* length of last component */
    int		head_length;		/* length up to (including) '/' */

    /*
     * Obtain the length of the last component of srcname.
     */
    last_slash = strrchr(srcname, '/');
    last_hat = strrchr(srcname, '^');
    if ((last_slash == NULL) ||
	(last_slash != NULL && last_hat != NULL && last_hat > last_slash)) {
	last_slash = last_hat;
    }

    /*
     * last_slash now points at the last delimiter in srcname,
     * or is NULL if srcname contains no delimiters.
     */
    if (last_slash != NULL) {
	tail_length = strlen(last_slash + 1);
	head_length = last_slash - srcname;
    } else {
	tail_length = strlen(srcname);
	head_length = 0;
    }

    /*
     * We want to add ".tmp" onto the end of the name,
     * and QNX restricts filenames to 16 characters.
     * So we must ensure that the last component
     * of the pathname is not more than 12 characters
     * long, or truncate it so it isn't.
     */
    if (tail_length > 12)
	tail_length = 12;

    /*
     * Create the new pathname. We need the total length
     * of the path as is, plus ".tmp" plus a null byte.
     */
    newname = alloc(head_length + tail_length + 5);
    if (newname != NULL) {
	(void) strncpy(newname, srcname, head_length + tail_length);
    }

    {
	char	*endp;
	int	indexnum;

	endp = newname + head_length + tail_length;
	indexnum = 0;
	do {
	    /*
	     * Keep trying this until we get a unique file name.
	     */
	    if (indexnum > 0) {
		(void) sprintf(endp, ".%03d", indexnum);
	    } else {
		(void) strcpy(endp, ".tmp");
	    }
	    indexnum++;
	} while (access(newname, 0) == 0 && indexnum < 1000);
    }

    return(newname);
}

/*
 * Fake out a pipe by writing output to temp file, running a process with
 * i/o redirected from this file to another temp file, and then reading
 * the second temp file back in.
 */
bool_t
sys_pipe(cmd, writefunc, readfunc)
char	*cmd;
int	(*writefunc) P((FILE *));
long	(*readfunc) P((FILE *));
{
    Flexbuf		cmdbuf;
    static char	*args[] = { NULL, "+s", NULL, NULL };
    char		*temp1;
    char		*temp2;
    char		buffer[512];
    FILE		*fp;
    bool_t		retval = FALSE;

    if (Ps(P_shell) == NULL) {
	return(FALSE);
    }
    args[0] = Ps(P_shell);

    temp1 = mktemp(PIPE_MASK_1);
    temp2 = mktemp(PIPE_MASK_2);

    if (temp1 == NULL || temp2 == NULL) {
	return(FALSE);
    }

    /*
     * At this stage we are comitted; any failures must go through
     * the cleanup code at the end of the routine.
     */

    sys_endv();

    flexnew(&cmdbuf);
    (void) lformat(&cmdbuf, "%s < %s > %s", cmd, temp1, temp2);
    args[2] = flexgetstr(&cmdbuf);

    fp = fopen(temp1, "w+");
    if (fp == NULL) {
	retval = FALSE;
	goto ret;
    }

    (void) (*writefunc)(fp);
    (void) fclose(fp);

    if (createv(buffer, 0, -1, 0, BEQUEATH_TTY, 0, args, NULL) != 0) {
	retval = FALSE;
	goto ret;
    }

    fp = fopen(temp2, "r");
    if (fp == NULL) {
	retval = FALSE;
	goto ret;
    }

    (void) (*readfunc)(fp);
    (void) fclose(fp);

    retval = TRUE;

ret:
    flexdelete(&cmdbuf);

    (void) remove(temp1);
    (void) remove(temp2);
    free(temp1);
    free(temp2);

    sys_startv();

    return(retval);
}

/*
 * Expand filename.
 * Have to do this using the shell, using a /tmp filename in the vain
 * hope that it will be on a ramdisk and hence reasonably fast.
 */
char *
fexpand(name)
char	*name;
{
    static char	meta[] = "*?";
    int		has_meta;
    static char	*args[] = { NULL, "+s", NULL, NULL };
    static char	*io[4];
    Flexbuf	cmdbuf;
    char	*temp = NULL;
    FILE	*fp;
    static char	newname[256];
    char	*cp;

    if (name == NULL || *name == '\0') {
	return(name);
    }

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
	goto fail;
    }
    args[0] = Ps(P_shell);

    temp = mktemp(EXPAND_MASK);

    if (temp == NULL) {
	goto fail;
    }

    flexnew(&cmdbuf);
    (void) lformat(&cmdbuf, "echo %s > %s", name, temp);
    args[2] = flexgetstr(&cmdbuf);

    io[0] = io[1] = io[2] = "$tty99";
    if (createv(NULL, 0, -1, 0, 0, 0, args, NULL) != 0) {
	goto fail;
    }
    flexdelete(&cmdbuf);

    fp = fopen(temp, "r");
    if (fp == NULL) {
	goto fail;
    }

    if (fgets(newname, sizeof(newname), fp) == NULL) {
	(void) fclose(fp);
	goto fail;
    }
    cp = strchr(newname, '\n');
    if (cp != NULL) {
	*cp = '\0';
    }

    (void) fclose(fp);
    (void) remove(temp);
    free(temp);

    return(newname);
    
fail:
    if (temp != NULL) {
	free(temp);
    }
    return(name);
}
