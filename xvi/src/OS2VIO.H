/* Copyright (c) 1990,1991,1992 Chris and John Downey */
/***
* @(#)os2vio.h	2.1 (Chris & John Downey) 7/29/92

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    os2vio.h
* module function:
    Definitions for OS/2 system interface.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

/*
 * System include files.
 */
#define INCL_BASE
#define INCL_DOS
#define INCL_SUB
#define INCL_DOSERRORS

#include <os2.h>

/*
 * Include files for Microsoft C library.
 */
#include <fcntl.h>
#include <malloc.h>
#include <process.h>
#include <stdlib.h>
#include <time.h>
#ifndef SIGINT
#   include <signal.h>
#endif

/*
 * This is a multi-thread program, so we allocate multiple stacks
 * within the same stack segment, so there isn't much point in letting
 * the compiler put in stack probes.
 */
#pragma check_stack(off)

#ifndef HELPFILE
#   define HELPFILE	"d:\\jmd\\xvi\\help"
#endif

/*
 * System-dependent constants.
 *
 * These are right for MS-DOS & (I think) OS/2. (jmd)
 */
#define	MAXPATHLEN	143	/* maximum length of full path name */
#define	MAXNAMLEN	12	/* maximum length of file name */
#define	DIRSEPS		"\\/"	/*
				 * directory separators within
				 * pathnames
				 */

/*
 * Under OS/2, characters with the top bit set are perfectly valid
 * (although not necessarily always what you want to see).
 */
#define	DEF_CCHARS	TRUE
#define	DEF_MCHARS	TRUE

#define	SETVBUF_AVAIL
#define	WRTBUFSIZ	0x7000
#define	READBUFSIZ	0x7000

extern unsigned		Rows, Columns;
extern unsigned char	virt_row, virt_col;
extern unsigned char	curcell[2];

/*
 * Default value for P_format parameter.
 */
#define DEF_TFF		fmt_OS2

/*
 * Terminal driving functions - just use macros here.
 */
#define insert_line()		/* insert one line */
#define delete_line()		/* delete one line */
#define save_cursor()		/* save cursor position */
#define restore_cursor()	/* restore cursor position */
#define invis_cursor()		/* invisible cursor */
#define vis_cursor()		/* visible cursor */
#define tty_goto(r,c)	(virt_row = (r), virt_col = (c))
#define cost_goto	0	/* cost of using tty_goto() */

/*
 * Update actual screen cursor position. This is the only output flushing we
 * need to do, because outstr() & outchar() use system calls which update
 * video memory directly.
 */
#define flush_output()	VioSetCurPos(virt_row, virt_col, 0)

#define can_ins_line	FALSE
#define can_del_line	FALSE
#define can_scroll_area TRUE
/*
 * tty_linefeed() isn't needed if can_scroll_area is TRUE.
 */
#define tty_linefeed()

#define can_inschar	FALSE
#define inschar(c)

#define set_colour(a)	(curcell[1] = (a))
/*
 * User-definable screen colours. Default values are for mono
 * displays.
 */
#define DEF_COLOUR	7
#define DEF_STCOLOUR	112
#define DEF_SYSCOLOUR	7

#define alert()		DosBeep(2000, 150)

/*
 * Macros to open files in binary mode,
 * and to expand filenames.
 */
#define fopenrb(f)	fopen((f),"rb")
#define fopenwb(f)	fopen((f),"wb")
#define fexpand(f)	(f)

/*
 * exists(): TRUE if file exists.
 */
#define exists(f)	(access((f),0) == 0)
/*
 * can_write(): TRUE if file does not exist or exists and is writeable.
 */
#define can_write(f)	(access((f),0) != 0 || access((f), 2) == 0)
#define delay()		DosSleep((long) 200)
#define call_shell(s)	spawnlp(P_WAIT, (s), (s), (char *) NULL)
#define call_system(s)	system(s)

/*
 * Declarations for system interface routines in os2vio.c.
 */
extern	void		erase_display(void);
extern	void		erase_line(void);
extern	int		inchar(long);
extern	void		outchar(int);
extern	void		outstr(char*);
extern	void		scroll_down(unsigned, unsigned, unsigned);
extern	void		scroll_up(unsigned, unsigned, unsigned);
extern	char		*tempfname(char *);
extern	void		sys_init(void);
extern	void		sys_exit(int);
extern	bool_t		sys_pipe P((char *, int (*)(FILE *), long (*)(FILE *)));
extern	void		sys_startv(void);
extern	void		sys_endv(void);

/*
 * in i286.asm:
 */
extern void far			es0(void);
extern unsigned char * far	newstack(int);
