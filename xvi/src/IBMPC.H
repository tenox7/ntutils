/* Copyright (c) 1990,1991,1992 Chris and John Downey */
/***

* @(#)ibmpc.h	2.1 (Chris & John Downey) 7/29/92

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    ibmpc.h
* module function:
    Declarations for terminal interface module for IBM PC
    compatibles running MS-DOS.

* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

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
#define DEF_COLOUR	7	/* white on black */
#define DEF_STCOLOUR	112	/* black on white */
#define DEF_SYSCOLOUR	7	/* white on black */

/*
 * Screen dimensions.
 */
extern unsigned int	Columns;
extern unsigned int	Rows;

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
