/* Copyright (c) 1990,1991,1992 Chris and John Downey */
/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    nt.h
* module function:
    Definitions for Windows NT system interface module.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <io.h>
#include <string.h>
#include <process.h>
#include <direct.h>

/*
 * Default value for helpfile parameter.
 */
#ifndef HELPFILE
#   define  HELPFILE	"c:\\xvi\\help"
#endif

#   define SETVBUF_AVAIL
    /*
     * These are the buffer sizes we use for reading & writing files.
     */
#   define  READBUFSIZ	16384
#   define  WRTBUFSIZ	16384

#define STRERROR_AVAIL

/*
 * Macros to open files in binary mode.
 */
#define fopenrb(f)	fopen((f),"rb")
#define fopenwb(f)	fopen((f),"wb")

/* THIS SHOULD BE FIXED TO DO REAL FILENAME EXPANSION!!! */
#define fexpand(f)	(f)

#define call_system(s)	system(s)
#define call_shell(s)	spawnlp(0, (s), (s), (char *) NULL)

/*
 * System-dependent constants - these are needed by file i/o routines.
 */
#   define  MAXPATHLEN	_MAX_PATH	/* max length of full path name */
#   define  MAXNAMLEN	(_MAX_FNAME+1+_MAX_EXT)	/* max length of file name */

#ifndef W_OK
#   define	F_OK	0
#   define	W_OK	2
#endif

#define exists(f)	(_access((f),F_OK) == 0)
#define can_write(f)	(_access((f),F_OK) != 0 || _access((f), W_OK) == 0)
#define chdir		_chdir
#define getcwd		_getcwd

#define	DIRSEPS		"\\/"	/* directory separators within pathnames */

/*
 * Default file format.
 */
#define DEF_TFF		fmt_MSDOS

/*
 * These are needed for the termcap terminal interface module.
 */
#define oflush()	(void) fflush(stdout)
#define moutch(c)	putchar(c)

/*
 * Declarations for standard UNIX functions.
 */
extern	int		rename P((const char *, const char *));

/*
 * Declarations for system interface routines in unix.c.
 */
extern	void		foutch P((int));
extern	void		delay P((void));
extern	void		sys_init P((void));
extern	void		sys_startv P((void));
extern	void		sys_endv P((void));
extern	void		sys_exit P((int));
extern	bool_t		sys_pipe P((char *, int (*)(FILE *), long (*)(FILE *)));
extern	char		*tempfname P((char *));

/*
 * From here on down is the terminal interface stuff.
 */
#include <conio.h>

#define can_ins_line	FALSE
#define can_del_line	FALSE
#define can_scroll_area TRUE

/*
 * tty_linefeed() isn't needed if can_scroll_area is TRUE.
 */
#define tty_linefeed()
#define can_inschar	FALSE
#define inschar(c)
#define cost_goto	0	/* cost of tty_goto() */

#define tty_close()
#define vis_cursor()
#define invis_cursor()
#define insert_line()
#define delete_line()

/*
 * Colour handling: default screen colours for PC's.
 */
#define DEF_COLOUR	0
#define DEF_STCOLOUR	1
#define DEF_SYSCOLOUR	2

/*
 * Screen dimensions.
 */
extern int	Columns;
extern int	Rows;

/*
 * Declarations for routines in ibmpc_a.asm & ibmpc_c.c:
 */
extern void		alert P((void));
extern void		erase_display P((void));
extern void		erase_line P((void));
extern void		flush_output P((void));
extern void		hidemouse P((void));
extern int		inchar P((long));
extern unsigned		mousestatus P((unsigned *, unsigned *));
extern void		outchar P((int));
extern void		outstr P((char *));
extern void		scroll_down P((unsigned, unsigned, int));
extern void		scroll_up P((unsigned, unsigned, int));
extern void		set_colour P((int));
extern void		showmouse P((void));
extern void		tty_endv P((void));
extern void		tty_goto P((int, int));
extern void		tty_open P((unsigned *, unsigned *));
extern void		tty_startv P((void));
