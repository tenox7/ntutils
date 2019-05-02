/* Copyright (c) 1990,1991,1992 Chris and John Downey */
/***

* @(#)qnx.h	2.1 (Chris & John Downey) 7/29/92

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    qnx.h
* module function:
    Definitions for QNX system interface module.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include <tcap.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>
#include <lfsys.h>
#include <process.h>

#ifndef	HELPFILE
#   define	HELPFILE	"/user/local/bin/xvi.help"
#endif

/*
 * These are the buffer sizes we use for reading & writing files.
 */
#define SETVBUF_AVAIL
#define	READBUFSIZ	4096
#define	WRTBUFSIZ	4096

/*
 * Execute a command in a subshell.
 */
#define	call_system(s)	system(s)

/*
 * System-dependent constants.
 */
#define	MAXPATHLEN	79	/* maximum length of full path name */
#define	MAXNAMLEN	16	/* maximum length of file name */
#define	DIRSEPS		"/^"	/*
				 * directory separators within
				 * pathnames
				 */

/*
 * Under QNX, characters with the top bit set are perfectly valid
 * (although not necessarily always what you want to see).
 */
#define	DEF_CCHARS	TRUE
#define	DEF_MCHARS	TRUE

/*
 * Default file format.
 */
#define DEF_TFF		fmt_QNX

#define	Rows		tcap_entry.term_num_rows
#define	Columns		tcap_entry.term_num_cols

/*
 * Size of buffer for file i/o routines.
 * The SETVBUF_AVAIL forces the file i/o routines to
 * use a large buffer for reading and writing, and
 * this results in a large performance improvement.
 */
#define SETVBUF_AVAIL
#define BIGBUF		16384

/*
 * Macros to open files in binary mode,
 * and to expand filenames.
 */
#define fopenrb(f)	fopen((f),"r")
#define fopenwb(f)	fopen((f),"w")

/*
 * Terminal driving functions.
 *
 * Assume TCAP driver.
 */
#define	erase_line()	term_clear(_CLS_EOL)
#define	insert_line()	term_esc(tcap_entry.disp_insert_line)
#define	delete_line()	term_esc(tcap_entry.disp_delete_line)
#define	erase_display()	term_clear(_CLS_SCRH)
#define	invis_cursor()
#define	vis_cursor()

#define	cost_goto	8

#define	can_ins_line	FALSE
#define	can_del_line	FALSE

extern	bool_t		can_scroll_area;
extern	void		(*up_func)(int, int, int);
extern	void		(*down_func)(int, int, int);
#define	scroll_up	(*up_func)
#define	scroll_down	(*down_func)

#define tty_linefeed()	putchar('\n')
extern	bool_t		can_scroll_area;
#define	can_inschar	FALSE
#define	inschar(c)

/*
 * Colour handling: QNX attributes.
 * These are defined so as to work on both colour and monochrome screens.
 *
 * The colour word contains the following fields:
 *
 *	eBBB_FFF__uihb
 *
 * where:
 *	e	means enable colour
 *	BBB	is the background colour
 *	FFF	is the foreground colour
 *	u	means underline
 *	i	means inverse
 *	h	means high brightness
 *	b	means blinking
 *
 * The colours that may be represented using the three bits of FFF or
 * BBB are:
 *	0	black		4	red
 *	1	blue		5	magenta
 *	2	green		6	yellow
 *	3	cyan		7	white
 *
 * We always set 'e', sometimes 'h' and never 'u', or 'b'.
 * 'i' is set for colours which want to be inverse in monochrome.
 */
#define	DEF_COLOUR	(0x8000 | 0x00 | 0x0700)   /* white on black      */
#define	DEF_SYSCOLOUR	(0x8000 | 0x00 | 0x0700)   /* white on black      */
#define	DEF_STCOLOUR	(0x8000 | 0x06 | 0x6100)   /* bright cyan on blue */
#define	DEF_ROSCOLOUR	(0x8000 | 0x06 | 0x7400)   /* bright white on red */

/*
 * Declarations for OS-specific routines in qnx.c.
 */
extern	int		inchar(long);
extern	void		sys_init(void);
extern	void		sys_exit(int);
extern	bool_t		can_write(char *);
extern	bool_t		exists(char *);
extern	int		call_shell(char *);
extern	void		alert(void);
extern	void		delay(void);
extern	void		outchar(int);
extern	void		outstr(char *);
extern	void		flush_output(void);
extern	void		set_colour(int);
extern	void		tty_goto(int, int);
extern	void		co_up(int, int, int);
extern	void		co_down(int, int, int);
extern	void		vt_up(int, int, int);
extern	void		vt_down(int, int, int);
extern	void		sys_startv(void);
extern	void		sys_endv(void);
extern	char		*tempfname(char *);
extern	bool_t		sys_pipe P((char *, int (*)(FILE *), long (*)(FILE *)));
extern	char		*fexpand P((char *));
