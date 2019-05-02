#ifndef lint
static char *sccsid = "@(#)os2vio.c	2.1 (Chris & John Downey) 7/29/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    os2vio.c

* module function:
    OS/2 system interface module.

    This is a character-based implementation using the VIO & KBD
    families of system calls. It doesn't use the Presentation Manager
    but, on OS/2 version 1.* at least, it can be made to work in a PM
    shell window by using markexe (see makefile.os2).

    Like the MS-DOS version, this one saves the screen contents &
    restores them when it exits.

    Currently, the mouse input code doesn't work, & so is commented
    out. I suspect that, if we want to have both mouse & keyboard
    input, we have to use a device monitor, or develop a real PM
    implementation.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

#define NOMOUSE

#ifdef __ZTC__
/*
 * Set default stack size.
 *
 * See i286.asm for an explanation of why it has to be so big.
 */
unsigned	_stack = 44 * 1024;
#endif

/*
 * These are globals which are set by the system interface or terminal
 * interface module, & used for various purposes throughout the rest
 * of xvi.
 *
 * Number of rows & columns in the current window.
 */
unsigned	Rows,
		Columns;
/*
 * Current position for screen writes.
 */
unsigned char	virt_row,
		virt_col;
/*
 * Screen cell (character & attribute): current colour is stored here.
 */
unsigned char	curcell [2];

/*
 * Time of last keypress or mouse button press (or garbage if
 * (keystrokes < PSVKEYS)).
 *
 * This should only be referenced within a thread's critical section.
 * Referencing a 32-bit variable is not generally an atomic operation
 * on the 80286.
 */
static volatile clock_t lastevent;

#ifndef NOMOUSE
    /*
     * This is FALSE if we don't appear to have a mouse driver.
     */
    static bool_t	usemouse;

    /*
     * Our logical mouse handle.
     */
    static HMOU	mousenum;

#else	/* NOMOUSE */
#   define	usemouse	FALSE
#endif	/* NOMOUSE */

#ifndef NOMOUSE
    /*
     * Hide mouse cursor.
     */
    static void
    hidemouse()
    {
	NOPTRRECT	r;

	r.row = r.col = 0;
	r.cRow = Rows - 1;
	r.cCol = Columns - 1;
	(void) MouRemovePtr((PNOPTRRECT) &r, mousenum);
    }

#endif	/* NOMOUSE */

/*
 * Show mouse cursor. (This is for symmetry with hidemouse().)
 */
#define showmouse()	((void) MouDrawPtr(mousenum))

static long	semvec [2];

/*
 * This semaphore needs to be acquired by a thread before it enters a
 * critical region.
 */
#define		control ((HSEM)(long FAR *)&semvec[0])

/*
 * This semaphore is used for communication between the main thread &
 * the thread which handles automatic buffer preservation. It should
 * be clear when (keystrokes >= PSVKEYS), otherwise it should be set.
 */
#define		psvsema ((HSEM)(long FAR *)&semvec[1])

#ifndef NOMOUSE

static void
mousehandler()
{
    for (;;) {
	MOUEVENTINFO	m;
	unsigned short	status;
	clock_t		start;

#if 0
	if (MouGetDevStatus((PUSHORT) &status, mousenum) != 0
	    ||
	    (status & (MOUSE_UNSUPPORTED_MODE | MOUSE_DISABLED))
	) {
	    hidemouse();
	    (void) MouClose(mousenum);
	    DosExit(EXIT_THREAD, 0);
	}
#endif
	status = MOU_WAIT;
	MouReadEventQue((PMOUEVENTINFO) &m, (PUSHORT) &status, mousenum);
	/*
	 * If we don't get the control semaphore immediately,
	 * we do nothing. Delayed responses to mouse button
	 * presses could be confusing.
	 */
#if 0
	start = clock();
#endif
	if (DosSemRequest(control, SEM_IMMEDIATE_RETURN) != 0)
	    continue;
#if 0
	if (clock() != start) {
	    (void) fprintf(stderr, "mouse thread: %d\n", __LINE__);
	    DosSemClear(control);
	    continue;
	}
#endif
	/*
	 * Start of critical section.
	 */
	if (++keystrokes >= PSVKEYS)
	    lastevent = clock();
	if (State == NORMAL &&
		(m.fs & (MOUSE_BN1_DOWN | MOUSE_BN2_DOWN | MOUSE_BN3_DOWN))) {
	    hidemouse();
	    mouseclick(m.row, m.col);
	    showmouse();
	}
	/*
	 * End of critical section.
	 */
	DosSemClear(control);
    }
}

#endif	/* NOMOUSE */

/*
 * Macro to convert clock ticks to milliseconds.
 */
#if CLK_TCK == 1000
#   define CLK2MS(c)		(c)
#else
#   if CLK_TCK < 1000
#	define CLK2MS(c)	((c) * (1000 / CLK_TCK))
#   else
#	define CLK2MS(c)	((c) / (CLK_TCK / 1000))
#   endif	/* CLK_TCK > 1000 */
#endif	/* CLK_TCK != 1000 */

/*
 * Number of keystrokes or mouse button presses since the last buffer
 * preservation.
 */
volatile int		keystrokes;

/*
 * This function handles automatic buffer preservation. It runs in its
 * own thread, which is only awake when keystrokes >= PSVKEYS and the
 * main thread is waiting for keyboard input. Even then, it spends
 * most of its time asleep.
 */
static void FAR
psvhandler()
{
    for (;;) {
	long	sleeptime;

	DosSemWait(psvsema, SEM_INDEFINITE_WAIT);
	DosSemRequest(control, SEM_INDEFINITE_WAIT);
	/*
	 * Start of critical section.
	 */
	if (keystrokes < PSVKEYS) {
	    sleeptime = 0;
	    /*
	     * If we haven't had at least PSVKEYS
	     * keystrokes, psvsema should be set.
	     */
	    DosSemSet(psvsema);
	} else if ((sleeptime = (long) Pn(P_preservetime) * 1000 -
		      CLK2MS(clock() - lastevent)) <= 0) {
	    /*
	     * If Pn(P_presevetime) seconds haven't yet
	     * elapsed, sleep until they should have - but
	     * NOT within the critical section (!).
	     *
	     * Otherwise do automatic preserve.
	     *
	     * do_preserve() should reset keystrokes to 0.
	     */
	    (void) do_preserve();
	    sleeptime = 0;
	}
	/*
	 * End of critical section.
	 */
	DosSemClear(control);
	/*
	 * Sleep if we have to.
	 */
	if (sleeptime != 0)
	    DosSleep(sleeptime);
    }
}

/*
 * inchar() - get a character from the keyboard.
 *
 * Timeout not implemented yet for OS/2.
 */
int
inchar(long mstimeout)
{
    for (;;) {
	KBDKEYINFO k;
	bool_t	mstatus,
		psvstatus;

	flush_output();

	mstatus = (usemouse && State == NORMAL);
	psvstatus = (keystrokes >= PSVKEYS);
	/*
	 * We don't have to give control to any other thread
	 * if neither of these conditions is true.
	 */
	if (mstatus || psvstatus) {
#ifndef NOMOUSE
	    if (mstatus)
		showmouse();
#endif
	    if (psvstatus && DosSemWait(psvsema, SEM_IMMEDIATE_RETURN)
						    == ERROR_SEM_TIMEOUT) {
		/*
		 * If psvsema is set, clear it.
		 */
		DosSemClear(psvsema);
	    }
	    DosSemClear(control);
	}
	/*
	 * Start of non-critical section.
	 *
	 * Wait for character from keyboard.
	 */
	KbdCharIn((PKBDKEYINFO) &k, IO_WAIT, 0);
	/*
	 * End of non-critical section.
	 */
	if (mstatus || psvstatus) {
	    DosSemRequest(control, SEM_INDEFINITE_WAIT);
#ifndef NOMOUSE
	    if (mstatus)
		hidemouse();
#endif
	}
	if (++keystrokes >= PSVKEYS)
	    lastevent = clock();
	/*
	 * Now deal with the keypress information.
	 */
	if ((unsigned char) k.chChar == (unsigned char) 0xe0) {
	/*
	 * It's (probably) a function key.
	 */
	    if (k.chScan == 0x53)
		/*
		 * It's the delete key.
		 */
		return State == NORMAL ? 'x' : '\b';
	     /* else */
	    if (State == NORMAL) {
		/*
		 * Assume it must be a function key.
		 */
		switch (k.chScan) {
		    case 0x3b: return(K_HELP);
			/* F1 key */
		    case 0x47: return('H');
			/* home key */
		    case 0x48: return('k');
			/* up arrow key */
		    case 0x49: return(CTRL('B'));
			/* page up key */
		    case 0x4b: return('\b');
			/* left arrow key */
		    case 0x4d: return(' ');
			/* right arrow key */
		    case 0x4f: return('L');
			/* end key */
		    case 0x50: return('j');
			/* down arrow key */
		    case 0x51: return(CTRL('F'));
			/* page down key */
		    case 0x52: return('i');
			/* insert key */
		    default:
			/* just ignore it ... */
			continue;
		}
		/*
		 * If we aren't in command mode, 0xe0
		 * is a perfectly legitimate
		 * character, & we can't really tell
		 * whether or not it's supposed to be
		 * a function key, so we just have to
		 * return it as is.
		 */
	    }
	}
	return (unsigned char) k.chChar;
    }
}

void
outchar(int c)
{
    curcell [0] = c;
    VioWrtNCell((PBYTE) curcell, 1, virt_row, virt_col, 0);
    if (++virt_col >= Columns) {
	virt_col -= Columns;
	if (++virt_row >= Rows)
	    virt_row = Rows - 1;
    }
}

void
outstr(char* s)
{
    unsigned len = strlen(s);

    VioWrtCharStrAtt((PCH) s, len, virt_row, virt_col,
					 (PBYTE) & curcell [1], 0);
    if ((virt_col += len) >= Columns) {
	virt_col -= Columns;
	if (++virt_row >= Rows)
	    virt_row = Rows - 1;
    }
}

void
erase_display()
{
    curcell[1] = Pn(P_colour);
    curcell[0] = ' ';
    VioWrtNCell((PBYTE) curcell, Rows * Columns, 0, 0, 0);
}

void
erase_line()
{
    curcell [0] = ' ';
    VioWrtNCell((PBYTE) curcell, Columns - virt_col, virt_row, virt_col, 0);
}

void
scroll_down(unsigned start, unsigned end, unsigned nlines)
{
    curcell [0] = ' ';
    VioScrollDn(start, 0, end, Columns - 1, nlines, (PBYTE) curcell, 0);
}

void
scroll_up(unsigned start, unsigned end, unsigned nlines)
{
    curcell [0] = ' ';
    VioScrollUp(start, 0, end, Columns - 1, nlines, (PBYTE) curcell, 0);
}

/*
 * Attributes for colour systems
 */
#define BRIGHT	8	/* only available for foreground colours */
#define BLACK	0
#define BLUE	1
#define GREEN	2
#define CYAN	(BLUE | GREEN)
#define RED	4
#define BROWN	(RED | GREEN)
#define YELLOW	(BRIGHT | BROWN)
#define WHITE	(RED | GREEN | BLUE)

/*
 * macro to set up foreground & background colours
 */
#define mkcolour(f,b)	((unsigned char) (((b) << 4) | ((f) & 0xf)))

static char				*oldscreen;
static unsigned short			scrsize;
static enum { m_SYS = 0, m_VI = 1 }	curmode;

/*
 * Save screen contents & set up video & keyboard states for editor.
 */
void
sys_startv()
{
    if (curmode == m_VI)
	return;
    if (oldscreen != NULL) {
	/*
	 * Save contents of screen so we can restore them
	 * afterwards.
	 */
	VioReadCellStr((PCH) oldscreen, (PUSHORT) &scrsize, 0, 0, 0);
    }
    set_colour(Pn(P_colour));
    /*
     * Change keyboard status.
     *
     * We only do this when we've disabled keyboard interrupts.
     */
    {
	KBDINFO		k;

	k.cb = sizeof k;
	KbdGetStatus((PKBDINFO) &k, 0);
	k.fsMask = (k.fsMask
		/*
		 * turn these flags off:
		 */
		 & ~(KEYBOARD_ECHO_ON |
		 KEYBOARD_ASCII_MODE |
		 KEYBOARD_MODIFY_STATE |
		 KEYBOARD_MODIFY_INTERIM |
		 KEYBOARD_MODIFY_TURNAROUND |
		 KEYBOARD_2B_TURNAROUND |
		 KEYBOARD_SHIFT_REPORT))
		/*
		 * turn these flags on:
		 */
		| KEYBOARD_ECHO_OFF |
		  KEYBOARD_BINARY_MODE;
	KbdSetStatus((PKBDINFO) &k, 0);
    }
    curmode = m_VI;
}

void
sys_init()
{
    {
	VIOMODEINFO	v;

	/*
	 * Get information about display.
	 */
	v.cb = sizeof v;
	VioGetMode((PVIOMODEINFO) &v, 0);
	Rows = v.row;
	Columns = v.col;
	scrsize = (Rows - 1) * Columns * 2;
	if (v.color >= COLORS_16) {
	    /*
	     * Statically defined values are for mono systems:
	     * these are defaults for colour systems.
	     */
	    set_param(P_colour, mkcolour(BRIGHT | WHITE, BLUE));
	    set_param(P_statuscolour, mkcolour(YELLOW, BLACK));
	    set_param(P_roscolour, mkcolour(BRIGHT | RED, BLACK));
	    set_param(P_systemcolour, mkcolour(BRIGHT | CYAN, BLACK));
	}
    }
    oldscreen = malloc(scrsize);
    /*
     * We have to acquire this semaphore before we start any other
     * threads.
     */
    DosSemSet(control);
#ifndef NOMOUSE
    /*
     * Open mouse device if we can.
     */
    if (MouOpen((PSZ) NULL, (PHMOU) &mousenum) == 0
#if 0
	&& MouSynch(0) != 0
#endif
    ) {
	TID		mousethread;
	short		mask = MOUSE_BN1_DOWN |
			       MOUSE_BN2_DOWN |
			       MOUSE_BN3_DOWN;

	hidemouse();
#if 0
	MouSetEventMask((PUSHORT) &mask, mousenum);
#endif
	/*
	 * Create concurrent thread to handle mouse events.
	 *
	 * According to Microsoft, the ES register should be
	 * set to 0 first.
	 */
	DosCreateThread((PFNTHREAD) mousehandler,
		(es0(), (PTID) &mousethread), (PBYTE) newstack(32000));
	usemouse = TRUE;
    }
#endif	/* NOMOUSE */
    /*
     * Initialize semaphore for automatic buffer preservation. It
     * should only be clear if (keystrokes >= PSVKEYS).
     */
    DosSemSet(psvsema);
    /*
     * Create concurrent thread to do automatic preserves.
     *
     * According to Microsoft, the ES register should be set to 0 first.
     */
    {
	TID psvthread;

	if (DosCreateThread((PFNTHREAD) psvhandler,
		    (es0(), (PTID) &psvthread),
		    (PBYTE) newstack(20000)) != 0) {
	    (void) fputs("Can't create thread for automatic preserves\r\n",
			 stderr);
	    exit(1);
	}
    }
    /*
     * Disable system critical error handler.
     */
    DosError(HARDERROR_DISABLE);
    sys_startv();
}

/*
 * Restore video & keyboard states to what they were when we started.
 *
 * sys_endv() can be called when we're already in system mode, so we
 * have to check.
 */
void
sys_endv()
{
    KBDINFO k;

    if (curmode == m_SYS)
	return;
    k.cb = sizeof k;
    KbdGetStatus((PKBDINFO) &k, 0);
    k.fsMask = (k.fsMask
	    /*
	     * turn these flags off:
	     */
	     & ~(KEYBOARD_ECHO_OFF |
	     KEYBOARD_BINARY_MODE |
	     KEYBOARD_MODIFY_STATE |
	     KEYBOARD_MODIFY_INTERIM |
	     KEYBOARD_MODIFY_TURNAROUND |
	     KEYBOARD_2B_TURNAROUND |
	     KEYBOARD_SHIFT_REPORT))
	    /*
	     * turn these flags on:
	     */
	    | KEYBOARD_ECHO_ON |
	      KEYBOARD_ASCII_MODE;
    KbdSetStatus((PKBDINFO) &k, 0);
    if (oldscreen != (char*) 0)
	/*
	 * Restore contents of screen saved by
	 * sys_startv().
	 */
	VioWrtCellStr((PCH) oldscreen, scrsize, 0, 0, 0);
    tty_goto(Rows - 1, 0);
    set_colour(Pn(P_systemcolour));
    erase_line();
    flush_output();
    curmode = m_SYS;
}

void
sys_exit(int r)
{
    sys_endv();
#ifndef NOMOUSE
    if (usemouse)
	MouClose(mousenum);
#endif
    exit(r);
}

void
sleep(unsigned seconds)
{
    DosSleep(seconds * (long) 1000);
}

/*
 * This function is only used by tempfname(). It constructs a filename
 * suffix based on an index number.
 *
 * The suffix ".$$$" is commonly used for temporary file names on
 * MS-DOS & OS/2 systems. We also use the sequence ".$$1", ".$$2" ...
 * ".fff" (all digits are hexadecimal).
 */
static char*
hexsuffix(unsigned i)
{
    static char	suffix[] = ".$$$";
    static char	hextab[] = "0123456789abcdef";
    char	*sp = &suffix[3];

    while (sp > suffix) {
	if (i > 0) {
	    *sp-- = hextab[i & 0xf];
	    i >>= 4;
	} else {
	    *sp-- = '$';
	}
    }
    return suffix;
}

/*
 * Construct unique name for temporary file, to be used as a backup
 * file for the named file.
 */
char *
tempfname(char *srcname)
{
    char	*srctail,
		*srcdot,
		*endp,
		*retp;
    unsigned	indexnum = 0;
    unsigned	baselen;

    srctail = srcdot = NULL;
    endp = srcname;

    while (*endp) {
	switch (*endp++) {
	case '\\':
	case '/':
	    srctail = endp;
	    srcdot = (char*) 0;
	    continue;
	case '.':
	    srcdot = endp - 1;
	}
    }
    if (srctail == NULL) {
	/*
	 * We haven't found any directory separators ('/' or '\\').
	 */
	srctail = srcname;
	/*
	 * Check to see if there's a disk drive name. If there
	 * is, skip over it.
	 */
	if (*srcname && is_alpha(*srcname) && srcname[1] == ':')
	    srctail = &srcname[2];
    }
    /*
     * There isn't a dot in the trailing part of the filename:
     * just add it at the end.
     */
    if (srcdot == NULL)
	srcdot = endp;
    /*
     * Don't make name too long.
     */
    if (srcdot - srctail > MAXNAMLEN - 4)
	srcdot = srctail + MAXNAMLEN - 4;
    if (srcdot - srcname > MAXPATHLEN - 4)
	srcdot = srcname + MAXPATHLEN - 4;
    baselen = srcdot - srcname;
    /*
     * Allocate space for new temporary file name ...
     */
    if ((retp = alloc(baselen + 5)) == (char*) 0)
	return (char*) 0;
    if (baselen > 0)
	(void) memcpy(retp, srcname, baselen);
    do {
	/*
	 * Keep trying this until we get a unique file name.
	 */
	strcpy(&retp[baselen], hexsuffix(indexnum++));
    } while (exists(retp));
    return retp;
}

/*
 * Fake out a pipe by writing output to temp file, running a process with
 * i/o redirected from this file to another temp file, and then reading
 * the second temp file back in.
 *
 * OS/2 does have real pipes, but I don't know how to avoid deadlock
 * when connecting concurrent processes with bidirectional pipes.
 */
bool_t
sys_pipe(cmd, writefunc, readfunc)
char	*cmd;
int	(*writefunc) P((FILE *));
long	(*readfunc) P((FILE *));
{
    char	*temp1;
    FILE	*fp;
    bool_t	retval;

    /*
     * Create first temporary file ...
     */
    if (
	(temp1 = tempfname("xvi_out")) == NULL
	||
	(fp = fopen(temp1, "w")) == NULL
    ) {
	retval = FALSE;
    } else {
	char	*temp2 = NULL;
	int	savcon;
	int	fd1 = -1,
		fd2 = -1;

	/*
	 * ... then write to it & close it ...
	 */
	(void) (*writefunc)(fp);
	(void) fclose(fp);

	/*
	 * ... then re-open it for reading, open second one
	 * for writing & re-arrange file descriptors.
	 *
	 * Note that we assume that the editor's standard
	 * input, output & error files are the same device,
	 * since I can't imagine how any of them could
	 * usefully be redirected to anything else.
	 */

#ifndef O_BINARY
#	define O_BINARY 0
#endif
	if (
	    (savcon = dup(0)) < 3
	    ||
	    (fd1 = open(temp1, O_RDONLY | O_BINARY)) < 3
	    ||
	    (temp2 = tempfname("xvi_in")) == NULL 
	    ||
	    (fd2 = open(temp2,
	    		O_WRONLY | O_CREAT | O_EXCL | O_BINARY, 0600)) < 3
	) {
	    retval = FALSE;
	} else {
	    (void) dup2(fd1, 0);
	    (void) dup2(fd2, 1);
	    (void) dup2(fd2, 2);

	    (void) close(fd1);
	    (void) close(fd2);
	    fd1 = fd2 = -1;

	    /*
	     * Run the command.
	     */
	    (void) system(cmd);

	    /*
	     * Restore our standard input, output & error
	     * files.
	     */
	    (void) dup2(savcon, 0);
	    (void) dup2(savcon, 1);
	    (void) dup2(savcon, 2);

	    /*
	     * Now read from the second temporary file,
	     * close it, & we're done.
	     */
	    if ((fp = fopen(temp2, "r")) == NULL) {
		retval = FALSE;
	    } else {
		(void) (*readfunc)(fp);
		(void) fclose(fp);
		retval = TRUE;
	    }
	}
	/*
	 * Clean up.
	 */
	if (temp2) {
	    (void) remove(temp2);
	    free(temp2);
	}
	if (savcon > 2)
	    (void) close(savcon);
	if (fd1 > 2)
	    (void) close(fd1);
	if (fd2 > 2)
	    (void) close(fd2);
    }

    if (temp1) {
	(void) remove(temp1);
	free(temp1);
    }

    return(retval);
}
