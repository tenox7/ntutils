/* Copyright (c) 1990,1991,1992 Chris and John Downey */
/***

* @(#)termcap.h	2.1 (Chris & John Downey) 7/29/92

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    termcap.h
* module function:
    Definitions for termcap terminal interface module.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

/*
 * Size of screen.
 */
extern	unsigned int	LI;
extern	unsigned int	CO;

#define Rows		LI
#define Columns 	CO

/*
 * Standout glitch - see termcap.c.
 */
extern	int		SG;

#define SLINE_GLITCH	((unsigned int) SG)

/*
 * For the moment, inchar just maps to the routine provided
 * by the system interface module.
 */
#define inchar(t)	inch(t)

/*
 * There are no termcap capabilities for these:
 */
#define invis_cursor()		/* invisible cursor (very optional) */
#define vis_cursor()		/* visible cursor (very optional) */

/*
 * In the current implementation, this doesn't have to do anything.
 */
#define tty_close()

extern	int		cost_goto;	/* cost of using tty_goto() */

extern	bool_t		can_scroll_area;/* true if has scroll regions */
extern	bool_t		can_del_line;	/* true if we can delete lines */
extern	bool_t		can_ins_line;	/* true if we can insert lines */
extern	bool_t		can_inschar;	/* true if we can insert characters */

/*
 * Colour handling is possible if we have termcap,
 * using the entries c0 .. c9 (not documented).
 */
#define DEF_SYSCOLOUR	0
#define DEF_COLOUR	1
#define DEF_STCOLOUR	2
#define DEF_ROSCOLOUR	3

extern	void		outchar P((int c));
extern	void		outstr P((char *s));
extern	void		alert P((void));
extern	void		flush_output P((void));
extern	void		set_colour P((int c));
extern	void		tty_goto P((int row, int col));
extern	void		tty_linefeed P((void));
extern	void		insert_line P((void));
extern	void		delete_line P((void));
extern	void		inschar P((int));
extern	void		erase_line P((void));
extern	void		erase_display P((void));
extern	void		scroll_up P((int start_row, int end_row, int nlines));
extern	void		scroll_down P((int start_row, int end_row, int nlines));
extern	void		tty_open P((unsigned int *, unsigned int *));
extern	void		tty_startv P((void));
extern	void		tty_endv P((void));
