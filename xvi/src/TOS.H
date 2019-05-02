/* Copyright (c) 1990,1991,1992 Chris and John Downey */
/***

* @(#)tos.h	2.1 (Chris & John Downey) 7/29/92

* program name:
    xvi
* function:
    Portable version of UNIX "vi" editor, with extensions.
* module name:
    tos.h
* module function:
    Definitions for TOS system interface module.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#ifndef	HELPFILE
#   define  HELPFILE	"c:\\bin\\help.xvi"
#endif

/*
 * System-dependent constants
 */

/*
 * These numbers may not be right for TOS; who knows?
 */
#define	MAXPATHLEN	79	/* maximum length of full path name */
#define	MAXNAMLEN	12	/* maximum length of file name */
#define	DIRSEPS		"\\"

extern	int		Rows;			/* size of screen */
extern	int		Columns;

/*
 * Default file format.
 */
#define DEF_TFF		fmt_TOS

/*
 * No need to flush output - it has already happened.
 */
#define	flush_output()

/*
 * Terminal driving functions.
 */
#define	erase_line()	outstr("\033K")	/* erase to end of line */
#define	insert_line()	outstr("\033L")	/* insert one line */
#define	delete_line()	outstr("\033M")	/* delete one line */
#define	erase_display()	outstr("\033E")	/* erase display */
#define	invis_cursor()	outstr("\033f")	/* invisible cursor */
#define	vis_cursor()	outstr("\033e")	/* visible cursor */
#define	scroll_up(x, y, z)	st_scroll((x),(y),(z))	/* scroll up area */
#define	scroll_down(x, y, z)	st_scroll((x),(y),-(z))	/* scroll down area */

#define	cost_goto	8			/* cost of using tty_goto() */
#define	can_ins_line	TRUE
#define	can_del_line	TRUE
/*
 * tty_linefeed() isn't needed if can_del_line is TRUE.
 */
#define tty_linefeed()

#define	can_scroll_area	TRUE

#define	can_inschar	FALSE
#define	inschar(c)

/*
 * Colour handling: just b/w on Ataris at the moment.
 */
#define	DEF_COLOUR	0
#define	DEF_STCOLOUR	1
#define	DEF_SYSCOLOUR	0

/*
 * Macros to open files in binary mode,
 * and to expand filenames.
 */
#define fopenrb(f)	fopen((f), "rb")
#define fopenwb(f)	fopen((f), "wb")
#define	fexpand(f)	(f)

#define call_system(s)	system(s)
#define call_shell(s)	forkl((s), (char *) NULL)

/*
 * Declarations for system interface routines in tos.c.
 */
extern	int		inchar(long);
extern	void		outchar(char);
extern	void		outstr(char *);
extern	void		alert(void);
#ifndef LATTICE
    extern  bool_t	remove(char *);
#endif
extern	bool_t		can_write(char *);
extern	void		sys_init(void);
extern	void		sys_startv(void);
extern	void		sys_endv(void);
extern	void		sys_exit(int);
extern	void		tty_goto(int, int);
extern	void		delay(void);
extern  void		sleep(unsigned);
extern  void		set_colour(int);
extern  char		*tempfname(char *);
