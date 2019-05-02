/* Copyright (c) 1990,1991,1992 Chris and John Downey */
/***

* @(#)pc386.h	2.1 (Chris & John Downey) 7/29/92

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    pc386.h
* module function:
    Definitions for MS-DOS 386 protected mode version.

    See notes in pc386.c.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include <conio.h>
#include <disp.h>
#include <int.h>
#include <msmouse.h>
#include <sound.h>

/*
 * Screen dimensions.
 */
extern unsigned	Rows,
		Columns;

/*
 * Colour handling: default screen colours for PC's.
 */
#define DEF_COLOUR		7	/* white on black */
#define DEF_STCOLOUR		112	/* black on white */
#define DEF_SYSCOLOUR		7	/* white on black */

#define alert()			sound_beep(0x299)
#define can_ins_line		FALSE
#define can_del_line		FALSE
#define can_scroll_area		TRUE
#define can_inschar		FALSE
#define cost_goto		0	/* cost of tty_goto() */
#define delete_line()
#define erase_display()		(disp_move(0,0),disp_eeop())
#define erase_line()		disp_eeol()
#define flush_output()		disp_flush()
#define hidemouse()		msm_hidecursor()
#define inschar(c)
#define insert_line()
#define invis_cursor()
#define mousestatus(x,y)	msm_getstatus(x,y)
#define outchar(c)		disp_putc(c)
#define outstr(s)		disp_puts(s)
#define scroll_down(s,e,n)	pc_scroll((s),(e),-(n))
#define scroll_up(s,e,n)	pc_scroll(s,e,n)
#define set_colour(n)		disp_setattr(n)
#define showmouse()		msm_showcursor()
#define tty_goto(r,c)		disp_move(r,c)
#define tty_close()		(disp_inited && disp_close())
/*
 * tty_linefeed() isn't needed if can_scroll_area is TRUE.
 */
#define tty_linefeed()
#define vis_cursor()

/*
 * Declarations for routines in ibmpc_c.c & pc386.c.
 */
extern int			inchar P((long));
extern void			pc_scroll P((unsigned, unsigned, int));
extern void			tty_endv P((void));
extern void			tty_open P((unsigned *, unsigned *));
extern void			tty_startv P((void));
