/* Copyright (c) 1990,1991,1992 Chris and John Downey */
/***

* @(#)xvi.h	2.5 (Chris & John Downey) 9/1/92

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    xvi.h
* module function:
    General definitions for xvi.

    This file should really be split up into several files
    rather than being concentrated into one huge monolith.

* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

/***************************************************************
 *                                                             *
 * SECTION 1: ENVIRONMENT                                      *
 *                                                             *
 * Most of this section is concerned with including the right  *
 * header files; we also define some things by hand if the     *
 * appropriate definitions are not provided by the system.     *
 *                                                             *
 ***************************************************************/

/*
 * System include files ...
 */
#include <stdio.h>
#include <signal.h>
#include <string.h>

#ifdef WIN32

#   define  USE_STDHDRS
#   define  STRERROR_AVAIL
#   include <errno.h>
#   include <varargs.h>
#   define  VA_START(a, b)	va_start(a)
#   define  P(args)		args

#else

#ifdef	__STDC__

#   define  USE_STDHDRS
#   define  STRERROR_AVAIL
#   include <errno.h>
#   include <stdarg.h>
#   define  VA_START(a, b)	va_start(a, b)
#   define  P(args)		args

#else	/* not __STDC__ */

#   ifdef   ultrix
#   	ifdef	mips
#   	    define  USE_STDHDRS
#   	endif   /* mips */
#   endif   /* ultrix */
#   ifdef   sparc
#   	define	USE_STDHDRS
#   endif   /* sparc */
#   include <varargs.h>
#   define  VA_START(a, b)	va_start(a)

#   define  P(args) 		()

#   define  const
#   define  volatile

#endif	/* not __STDC__ */

#endif	/* WIN32 not defined */

#ifdef	USE_STDHDRS
#   include <stdlib.h>
#   include <stddef.h>
#   include <limits.h>
#else	/* USE_STDHDRS not defined */
    extern  FILE	*fopen();
    extern  char	*malloc();
    extern  char	*realloc();
    extern  char	*getenv();
    extern  long	atol();
    extern  char	*memcpy();
    extern  char	*memset();
    extern  void	exit();
#endif	/* USE_STDHDRS not defined */

#undef USE_STDHDRS

/*
 * Functions which ANSI does not specify should
 * be included in any standard header file.
 */
extern	int	chdir P((const char *path));
extern	char	*getcwd P((char *, unsigned));
extern	void	sleep P((unsigned seconds));

/*
 * If we have ANSI C, these should be defined in limits.h:
 */
#ifndef INT_MAX
#   define INT_MAX	((int) ((unsigned int) ~0 >> 1))
#endif
#ifndef INT_MIN
#   define INT_MIN	(~INT_MAX)
#endif
#ifndef ULONG_MAX
#   define ULONG_MAX	0xffffffff
#endif

/*
 * Macro to convert a long to an int.
 * If a long is the same as an int, this is trivial.
 */
#define	LONG2INT(n)	(sizeof(int) == sizeof(long) ? (int) (n) : \
			 (int) ((n) > INT_MAX ? INT_MAX : \
			  ((n) < INT_MIN ? INT_MIN : (n))))


/***************************************************************
 *                                                             *
 * SECTION 2: FUNDAMENTAL TYPES                                *
 *                                                             *
 * These types are used by other included header files.        *
 *                                                             *
 ***************************************************************/

/*
 * Boolean type.
 * It would be possible to make this an enumerated type,
 * but it isn't worth the hassle - enums don't confer any
 * real advantages, and it means we can't do things like
 *
 *	bool_t	value = (i == 47);
 *
 * but instead are forced to write the abominable
 *
 *	bool_t	value = (i == 47) ? TRUE : FALSE;
 *
 * which is silly.
 */
#undef	FALSE			/* just in case */
#undef	TRUE
#define	FALSE		0
#define	TRUE		1
typedef	int		bool_t;


/***************************************************************
 *                                                             *
 * SECTION 3: FUNDAMENTAL HEADER FILES                         *
 *                                                             *
 * These header files define types which are used by Xvi.      *
 *                                                             *
 ***************************************************************/

#include "virtscr.h"


/***************************************************************
 *                                                             *
 * SECTION 4: MISCELLANEOUS DEFINITIONS                        *
 *                                                             *
 * Definitions of limits and suchlike used within the editor.  *
 *                                                             *
 ***************************************************************/

/*
 * Minimum number of rows a window can have, including status line.
 */
#define	MINROWS		2

/*
 * SLOP is the amount of extra space we get for text on a line during
 * editing operations that need more space. This keeps us from calling
 * malloc every time we get a character during insert mode. No extra
 * space is allocated when the file is initially read.
 */
#define	SLOP		10

/*
 * The number of characters taken up by the line number
 * when "number" is set; up to 6 digits plus two spaces.
 */
#define	NUM_SIZE	8
#define	NUM_FMT		"%6ld  "

/*
 * (MAX_LINE_LENGTH - 1) gives the maximum line length this editor can read in.
 * Used by fileio.c (getfile()) and pipe.c (p_read()).
 */
#define	MAX_LINE_LENGTH	1024

/*
 * Maximum value for the tabstop parameter.
 */
#define	MAX_TABSTOP	32

/*
 * Default timeout for keystrokes (in milliseconds).
 * The timeout parameter is set to this value.
 */
#define	DEF_TIMEOUT	200


/***************************************************************
 *                                                             *
 * SECTION 5: PARAMETER TYPE DEFINITIONS                       *
 *                                                             *
 * These are definitions of types for particular parameters.   *
 *                                                             *
 ***************************************************************/

/*
 * Regular expression search modes - used in search.c.
 * These are the integer values to which the P_regextype
 * enumerated parameter may be set. Note that these values
 * are ordered; e.g. rt_GREP is considered to be earlier
 * than, and/or less than, rt_EGREP. If the types are added
 * to, this ordering must be maintained. Also note that the
 * names in the rt_strings table must follow the same order.
 */
#define rt_TAGS		0	/* only ^ and $ are significant */
#define rt_GREP		1	/* like grep, but with \< and \> */
#define rt_EGREP	2	/* like egrep, but with \< and \> */

/*
 * Array of names for the P_regextype enumeration, defined in
 * search.c.
 */
extern	char		*rt_strings[];

/*
 * Integer values for the P_preserve enumerated parameter. Note that
 * the names in psv_strings must follow the same order.
 */
#define psv_UNSAFE	0	/* never preserve buffer before writing */
#define psv_STANDARD	1	/*
				 * only preserve buffer before writing
				 * if it hasn't been preserved recently
				 */
#define psv_SAFE	2	/* always preserve buffer before writing */
#define psv_PARANOID	3	/*
				 * like psv_SAFE, but never remove the
				 * preserve file
				 */

/*
 * Array of names for the P_preserve enumeration, defined in
 * search.c.
 */
extern	char		*psv_strings[];

/*
 * Integer values for the P_format enumerated parameter. These are for
 * the formats we know about so far. Note that the entries in
 * fmt_strings & tftable (defined in fileio.c) must follow the same order.
 */
#define	fmt_CSTRING	0
#define	fmt_MACINTOSH	1
#define	fmt_MSDOS	2
#define	fmt_OS2		3
#define	fmt_QNX		4
#define	fmt_TOS		5
#define	fmt_UNIX	6

/*
 * Array of names for the P_format enumeration.
 */
extern	char		*fmt_strings[];

/*
 * Integer values for the P_jumpscroll enumerated parameter. Note that
 * the entries in js_strings (defined in param.c) must follow the same
 * order.
 */
#define	js_OFF		0
#define	js_AUTO		1
#define	js_ON		2

/***************************************************************
 *                                                             *
 * SECTION 6: EDITOR TYPE DEFINITIONS                          *
 *                                                             *
 ***************************************************************/

/*
 * Possible editor states.
 */
typedef enum {
    NORMAL,			/* command mode */
    INSERT,			/* insert mode */
    REPLACE,			/* overwrite mode */
    CMDLINE,			/* on the : command line */
    DISPLAY			/* in display mode (e.g. g//p or :set all) */
} state_t;

extern	state_t		State;	/* defined in main.c */

/*
 * Possible return values for cmd_input(), which deals with command
 * line input. This is for commands starting with :, /, ? and !.
 */
typedef enum {
    cmd_COMPLETE,		/* user hit return (cmd line available) */
    cmd_INCOMPLETE,		/* not finished typing command line yet */
    cmd_CANCEL			/* user aborted command line */
} Cmd_State;

/*
 * Possible directions for searching, and opening lines.
 */
#define	FORWARD		0
#define	BACKWARD	1

/*
 * Line structure and its friends.
 *
 * The structure used to hold a line; this is used to form
 * a doubly-linked list of the lines comprising a file.
 *
 * The definition for MAX_LINENO depends on the type of
 * l_number, and on the size of the machine; we are fairly
 * safe setting all bits here so long as l_number is always
 * an unsigned type. Should really use a lineno_t here, and
 * get rid of the need for a maximum lineno.
 */
typedef	struct	line {
    struct line		*l_prev;	/* previous line */
    struct line		*l_next;	/* next line */
    char		*l_text;	/* text for this line */
    int			l_size;		/* actual size of space at 's' */
    unsigned long	l_number;	/* line "number" */
} Line;

#define	MAX_LINENO	ULONG_MAX

/*
 * These pseudo-functions operate on lines in a buffer, returning TRUE
 * or FALSE according to whether l1 is later or earlier than l2.
 * Note that there is no macro for "same", which is excluded by both
 * "earlier" and "later".
 */
#define	later(l1, l2)	((l1)->l_number > (l2)->l_number)
#define	earlier(l1, l2)	((l1)->l_number < (l2)->l_number)

/*
 * This macro gives the line number of line 'l' in buffer 'b'.
 */
#define	lineno(b, l)	((l)->l_number)

/*
 * Easy ways of finding out whether a given line is the first
 * or last line of a buffer, without needing a buffer pointer.
 */
#define	is_lastline(lp)	((lp)->l_number == MAX_LINENO)
#define	is_line0(lp)	((lp)->l_number == 0)


/*
 * Structure used to hold a position in a file;
 * this is just a pointer to the line, and an index.
 */
typedef	struct	position {
    Line		*p_line;	/* line we're referencing */
    int			p_index;	/* position within that line */
} Posn;

/*
 * This is stuff to do with marks - it should be privately defined
 * in mark.c, but it needs to be related to each individual buffer.
 */
#define	NMARKS	10		/* max. # of marks that can be saved */

typedef	struct	mark {
    char		m_name;
    Posn		m_pos;
} Mark;

/*
 * Structure used to record a single change to a buffer.
 * A change may either be a number of lines replaced with a
 * new set, a number of characters removed or a number of
 * characters replaced with a new set. Character changes
 * never straddle line boundaries. A list of these
 * structures forms a complex change. There is also a fourth
 * type of "change", which does not actually change the
 * buffer, but is simply a record of the cursor position at
 * the time of the start of the change. This is needed so
 * that the cursor returns to the correct position after an
 * "undo".
 *
 * This entire structure is only used in undo.c and alloc.c, and
 * no other code should touch it.
 */
typedef	struct change {
    struct change	*c_next;
    enum {
	C_LINE,
	C_CHAR,
	C_DEL_CHAR,
	C_POSITION
    }			c_type;
    unsigned long	c_lineno;
    union {
	struct {
	    long    cul_nlines;
	    Line    *cul_lines;
	}	cu_l;
	struct {
	    int	    cuc_index;
	    int	    cuc_nchars;
	    char    *cuc_chars;
	}	cu_c;
	struct {
	    long    cup_line;
	    int	    cup_index;
	}	cu_p;
    }			c_u;
} Change;

#define	c_nlines	c_u.cu_l.cul_nlines
#define	c_lines		c_u.cu_l.cul_lines
#define	c_index		c_u.cu_c.cuc_index
#define	c_nchars	c_u.cu_c.cuc_nchars
#define	c_chars		c_u.cu_c.cuc_chars
#define	c_pline		c_u.cu_p.cup_line
#define	c_pindex	c_u.cu_p.cup_index

/*
 * Variable-length FIFO queue of characters.
 */
typedef struct
{
	char		*fxb_chars;	/* pointer to allocated space */
	unsigned	fxb_max;	/* size of allocated space */
	unsigned	fxb_rcnt;	/* number of characters read */
	unsigned	fxb_wcnt;	/* number of characters written */
/* public: */
/*
 * Initialize a Flexbuf.
 */
#define			flexnew(f)	((f)->fxb_wcnt = (f)->fxb_max = 0)
/*
 * Reset a Flexbuf by clearing its contents, but without freeing the
 * dynamically allocated space.
 */
#define			flexclear(f)	((f)->fxb_wcnt = 0)
/*
 * Test whether a Flexbuf is empty.
 */
#define			flexempty(f)	((f)->fxb_rcnt >= (f)->fxb_wcnt)
/*
 * Remove last character from a Flexbuf.
 */
#define			flexrmchar(f)	(!flexempty(f) && --(f)->fxb_wcnt)
/*
 * Return number of characters in a Flexbuf.
 */
#define			flexlen(f)	(flexempty(f) ? 0 : \
					 (f)->fxb_wcnt - (f)->fxb_rcnt)
}
Flexbuf;

/*
 * Structure used to hold all information about a "buffer" -
 * i.e. the representation of a file in memory.
 */
typedef struct buffer {
    Line		*b_line0;	/* ptr to zeroth line of file */
    Line		*b_file;	/* ptr to first line of file */
    Line		*b_lastline;	/* ptr to (n+1)th line of file */

    /*
     * All of these are allocated, and should be freed
     * before assigning any new value to them.
     */
    char		*b_filename;	/* file name, if any */
    char		*b_tempfname;	/* name for temporary copy of file */

    unsigned int 	b_flags;	/* flags */

    int			b_nwindows;	/* no of windows open on this buffer */

    /*
     * The following only used in mark.c.
     */
    Mark		b_mlist[NMARKS];	/* current marks */
    Mark		b_pcmark;		/* previous context mark */
    bool_t		b_pcvalid;		/* true if pcmark is valid */

    /*
     * The following only used in undo.c.
     */
    unsigned int	b_nlevels;	/* number of brackets surrounding */
					/* current change to buffer */
    Change		*b_change;	/* ptr to list of changes made */

} Buffer;

/*
 * Definitions for the "flags" field of a buffer.
 */
#define	FL_MODIFIED	0x1
#define	FL_READONLY	0x2
#define	FL_NOEDIT	0x4
#define	is_modified(b)	((b)->b_flags & FL_MODIFIED)
#define	is_readonly(b)	(Pb(P_readonly) || ((b)->b_flags & FL_READONLY))
#define	not_editable(b)	((b)->b_flags & FL_NOEDIT)

/*
 * Structure used to hold information about a "window" -
 * this is intimately associated with the Buffer structure.
 */
typedef struct window {
    Posn		*w_cursor;	/* cursor's position in buffer */

    Buffer		*w_buffer;	/* buffer we are a window into */

    Line		*w_topline;	/* line at top of screen */
    Line		*w_botline;	/* line below bottom of screen */

    VirtScr		*w_vs;		/* virtual screen for window */

    unsigned		w_nrows;	/* number of rows in window */
    unsigned		w_ncols;	/* number of columns in window */
    unsigned		w_winpos;	/* row of top line of window */
    unsigned		w_cmdline;	/* row of window command line */

    /*
     * These are used by the ^O command to store the previous
     * size of the window so that we can return to it.
     */
    int			w_2winpos;	/* last row of top line of window */
    int			w_2nrows;	/* last no of rows in buffer window */
    int			w_2cmdline;	/* last row of window command line */


    /*
     * Allocated within screen.c.
     */
    Flexbuf		w_statusline;	/* status information on status line */


    /*
     * These elements are related to the cursor's position in the window.
     */
    int			w_row, w_col;	/* cursor's position in window */

    int			w_virtcol;	/* column number of the file's actual */
					/* line, as opposed to the column */
					/* number we're at on the screen. */
					/* This makes a difference on lines */
					/* which span more than one screen */
					/* line. */

    int			w_curswant;	/* The column we'd like to be at. */
					/* This is used to try to stay in */
					/* the same column through up/down */
					/* cursor motions. */

    bool_t		w_set_want_col;	/* If set, then update w_curswant */
					/* the next time through cursupdate() */
					/* to the current virtual column */

    int			w_c_line_size;	/* current size of cursor line */

    bool_t		w_curs_new;	/* true if cursor should be updated */

    /*
     * The following only used in windows.c.
     */
    struct window	*w_last;	/* first and last pointers */
    struct window	*w_next;
} Xviwin;

/*
 * Values returned by inc() & dec().
 */
enum mvtype {
    mv_NOMOVE = -1,	/* at beginning or end of buffer */
    mv_SAMELINE = 0,	/* still within same line */
    mv_CHLINE = 1,	/* changed to different line */
    mv_EOL = 2		/* in same line, at terminating '\0' */
};

/*
 * Number of input characters since the last buffer preservation.
 */
extern volatile int	keystrokes;

/*
 * Minimum number of keystrokes after which we do an automatic
 * preservation of all modified buffers.
 */
#define PSVKEYS		60

/*
 * Exceptional return values for get_file().
 */
#define gf_NEWFILE	((long)-1)	/* no such file */
#define gf_CANTOPEN	((long)-2)	/* error opening file */
#define gf_IOERR	((long)-3)	/* error reading from file */
#define gf_NOMEM	((long)-4)	/* not enough memory */

/*
 * Editor input events. Handled by xvi_handle_event().
 */
typedef struct event {
    enum {
	Ev_char,
	Ev_timeout
    }			ev_type;
    union {
	/* Ev_char: */
	int	evu_inchar;

	/* Ev_timeout: */
    }			ev_u;
} xvEvent;

#define	ev_inchar	ev_u.evu_inchar


/***************************************************************
 *                                                             *
 * SECTION 7: MISCELLANEOUS MACROS                             *
 *                                                             *
 ***************************************************************/


/***************************************************************
 *                                                             *
 * SECTION 8: XVI-LOCAL HEADER FILES                           *
 *                                                             *
 * Various subsidiary header files with definitions relating   *
 * to particular areas of the editor (or its environment).     *
 * Note that these header files may use definitions in this    *
 * file, so are included after all types are defined.          *
 *                                                             *
 ***************************************************************/

#include "ascii.h"
#include "param.h"
#include "ptrfunc.h"


/***************************************************************
 *                                                             *
 * SECTION 9: SYSTEM-SPECIFIC HEADER FILES                     *
 *                                                             *
 ***************************************************************/

/*
 * Include file for system interface module.
 * We must have one of these.
 */

#ifdef	ATARI
#   include "tos.h"
#   define	GOT_OS
#endif

#ifdef	UNIX
#   include "unix.h"
#   define	GOT_OS
#endif

#ifdef	OS2
    /*
     * Microsoft's wonderful compiler defines MSDOS, even when
     * we're compiling for OS/2, but it doesn't define OS2.
     * Ingenious, eh?
     */
#   undef MSDOS
#   include "os2vio.h"
#   define	GOT_OS
#endif

#ifdef	MSDOS
#   include "msdos.h"
#   define	GOT_OS
#endif

#ifdef	QNX
#   include "qnx.h"
#   define	GOT_OS
#endif

#ifdef WIN32
#   include "nt.h"
#   define	GOT_OS
#endif



/***************************************************************
 *                                                             *
 * SECTION 10: GLOBAL VARIABLE DECLARATIONS                    *
 *                                                             *
 ***************************************************************/

/*
 * Miscellaneous global vars.
 */
extern	Buffer		*curbuf;	/* current buffer */
extern	Xviwin		*curwin;	/* current window */

extern	int		indentchars;	/* auto-indentation on current line */
extern	char		*altfilename;	/* name of current alternate file */
extern	char		Version[];	/* version string for :ve command */

/*
 * This flag is set when a keyboard interrupt is received.
 */
extern volatile unsigned char kbdintr;

/*
 * This one indicates whether we should display the "Interrupted"
 * message.
 */
extern	bool_t		imessage;

/*
 * This variable (defined in main.c) is a bitmap which controls the
 * verbosity of screen output. The meanings of the individual bits
 * are:
 *
 *	e_CHARUPDATE means it's OK to update individual characters in
 *	any window.
 *
 *	e_SCROLL means it's OK to scroll any area of the screen up or
 *	down.
 *
 *	e_REPORT means it's OK for report() to report the number of
 *	lines inserted or deleted.
 *
 *	e_SHOWINFO means it's OK for show_file_info() to display file
 *	information for any buffer.
 *
 *	e_BEEP: not implemented yet.
 *
 *	e_REGERR means it's OK for functions in search.c to display
 *	messages which may have resulted from an invalid regular
 *	expression string.
 *
 *	e_NOMATCH means it's OK for functions in search.c to complain
 *	if they fail to match a regular expression at least once.
 *
 * If we're reading an input sequence & e_CHARUPDATE & e_SCROLL are
 * turned off, no screen updating will be done until an ESC is
 * received.
 */
extern	unsigned	echo;

#define	e_CHARUPDATE	1
#define	e_SCROLL	2
#define	e_REPORT	4
#define	e_SHOWINFO	010
#define	e_BEEP		020
#define	e_REGERR	040
#define	e_NOMATCH	0100

#define e_ANY		0xffff


/***************************************************************
 *                                                             *
 * SECTION 11: FUNCTION DECLARATIONS                           *
 *                                                             *
 ***************************************************************/

/*
 * Declarations of all the routines exported from the various .c files.
 */

/*
 * main.c
 */
extern	Xviwin	*xvi_startup P((VirtScr *, int, char **, char *));

/*
 * alloc.c
 */
extern	Change	*challoc P((void));
extern	void	chfree P((Change *));
extern	char	*alloc P((unsigned int));
extern	char	*strsave P((const char *));
extern	Line	*newline P((int));
extern	bool_t	bufempty P((Buffer *));
extern	bool_t	buf1line P((Buffer *));
extern	bool_t	endofline P((Posn *));
extern	bool_t	grow_line P((Line *, int));
extern	void	throw P((Line *));

/*
 * ascii.c
 */
extern unsigned vischar P((int, char **, int));

/*
 * buffers.c
 */
extern	Buffer	*new_buffer P((void));
extern	void	free_buffer P((Buffer *));
extern	bool_t	clear_buffer P((Buffer *));
extern	int	nbuffers;

/*
 * edit.c
 */
extern	bool_t	i_proc P((int));
extern	bool_t	r_proc P((int));
extern	void	startinsert P((int));
extern	void	startreplace P((int));
extern	char	*mkstr P((int));

/*
 * events.c
 */
extern	long	xvi_handle_event P((xvEvent *));

/*
 * cmdline.c
 */
extern	void	do_colon P((char *, bool_t));
extern	void	wait_return P((void));

/*
 * cursor.c
 */
extern	void	cursupdate P((Xviwin *));
extern	void	curs_horiz P((Xviwin *, int));

/*
 * ex_cmds1.c
 */
extern	void	do_quit P((Xviwin *, bool_t));
extern	void	do_split_window P((Xviwin *));
extern	bool_t	do_buffer P((Xviwin *, char *));
extern	void	do_close_window P((Xviwin *, bool_t));
extern	void	do_xit P((Xviwin *));
extern	bool_t	do_edit P((Xviwin *, bool_t, char *));
extern	void	do_args P((Xviwin *));
extern	void	do_next P((Xviwin *, int, char **, bool_t));
extern	void	do_rewind P((Xviwin *, bool_t));
extern	bool_t	do_write P((Xviwin *, char *, Line *, Line *, bool_t));
extern	void	do_wq P((Xviwin *, char *, bool_t));
extern	void	do_read P((Xviwin *, char *, Line *));
extern	void	do_alt_edit P((Xviwin *));
extern	void	do_compare P((void));

/*
 * ex_cmds2.c
 */
extern	void	do_shell P((Xviwin *));
extern	void	do_shcmd P((Xviwin *, char *));
extern	void	do_suspend P((Xviwin *));
extern	void	do_equals P((Xviwin *, Line *));
extern	void	do_help P((Xviwin *));
extern	bool_t	do_source P((bool_t, char *));
extern	char	*do_chdir P((char *));
extern	void	do_cdmy P((int, Line *, Line *, Line *));

/*
 * fileio.c
 */
extern	bool_t	set_format P((Xviwin *, Paramval, bool_t));
extern	long	get_file P((Xviwin *, char *, Line **, Line **, char *,
							char *));
extern	bool_t	writeit P((Xviwin *, char *, Line *, Line *, bool_t));
extern	bool_t	put_file P((Xviwin *, FILE *, Line *, Line *,
				unsigned long *, unsigned long *));

/*
 * find.c
 */
extern	Posn	*searchc P((int, int, int, int));
extern	Posn	*crepsearch P((Buffer *, int, int));
extern	Posn	*showmatch P((void));
extern	Posn	*find_pattern P((char *, int, int));
extern	Posn	*fwd_word P((Posn *, int, bool_t));
extern	Posn	*bck_word P((Posn *, int, bool_t));
extern	Posn	*end_word P((Posn *, int, bool_t));
extern	bool_t	dosearch P((Xviwin *, char *, int));

/*
 * flexbuf.c
 */
extern	bool_t	flexaddch P((Flexbuf *, int));
extern	char	*flexgetstr P((Flexbuf *));
extern	int	flexpopch P((Flexbuf *));
extern	void	flexdelete P((Flexbuf *));
extern	bool_t	vformat P((Flexbuf *, char *, va_list));
extern	bool_t	lformat P((Flexbuf *, char *, ...));

/*
 * map.c
 */
extern	void	stuff P((char *, ...));
extern	int	map_getc P((void));
extern	void	map_char P((int));
extern	void	map_timeout P((void));
extern	bool_t	map_waiting P((void));
extern	int	mapped_char P((int));
extern	void	xvi_map P((int, char **, bool_t, bool_t));
extern	void	xvi_keymap P((char *, char *));
extern	void	xvi_unmap P((int, char **, bool_t, bool_t));
extern	void	do_unmap P((int, char **, bool_t, bool_t));

/*
 * mark.c
 */
extern	void	init_marks P((Buffer *));
extern	bool_t	setmark P((int, Buffer *, Posn *));
extern	void	setpcmark P((Xviwin *));
extern	Posn	*getmark P((int, Buffer *));
extern	void	clrmark P((Line *, Buffer *));

/*
 * misccmds.c
 */
extern	bool_t	openfwd P((bool_t));
extern	bool_t	openbwd P((void));
extern	long	cntllines P((Line *, Line *));
extern	long	cntplines P((Xviwin *, Line *, Line *));
extern	long	plines P((Xviwin *, Line *));
extern	Line	*gotoline P((Buffer *, unsigned long));
extern	int	get_indent P((Line *));
extern	int	set_indent P((Line *, int));
extern	void	tabinout P((int, Line *, Line *));
extern	void	makeargv P((char *, int *, char ***, char *));

/*
 * mouse.c
 */
extern	void	mouseclick P((int, int));
extern	void	mousedrag P((int, int, int, int));

/*
 * movement.c
 */
extern	int	shiftdown P((Xviwin *, unsigned));
extern	int	shiftup P((Xviwin *, unsigned));
extern	void	scrolldown P((Xviwin *, unsigned));
extern	void	scrollup P((Xviwin *, unsigned));
extern	bool_t	oneup P((Xviwin *, long));
extern	bool_t	onedown P((Xviwin *, long));
extern	bool_t	one_left P((Xviwin *, bool_t));
extern	bool_t	one_right P((Xviwin *, bool_t));
extern	void	begin_line P((Xviwin *, bool_t));
extern	void	coladvance P((Xviwin *, int));
extern	void	do_goto P((long));
extern	void	move_cursor P((Xviwin *, Line *, int));
extern	void	move_window_to_cursor P((Xviwin *));
extern	void	move_cursor_to_window P((Xviwin *));

/*
 * normal.c
 */
extern	bool_t	normal P((int));

/*
 * param.c
 */
extern	void	init_params P((void));
extern	void	do_set P((Xviwin *, int, char **, bool_t));
extern	void	set_param P((int, ...));

/*
 * pipe.c
 */
extern	void	specify_pipe_range P((Xviwin *, Line *, Line *));
extern	void	do_pipe P((Xviwin *, char *));

/*
 * preserve.c
 */
extern	bool_t	preservebuf P((Xviwin *));
extern	bool_t	do_preserve P((void));

/*
 * ptrfunc.c
 */
extern	enum mvtype	inc P((Posn *));
extern	enum mvtype	dec P((Posn *));
extern	void	pswap P((Posn *, Posn *));
extern	bool_t	lt P((Posn *, Posn *));

/*
 * screen.c
 */
extern	void	init_screen P((Xviwin *));
extern	void	updateline P((Xviwin *));
extern	void	update_sline P((Xviwin *));
extern	void	update_window P((Xviwin *));
extern	void	update_all P((void));
extern	void	redraw_screen P((void));
extern	void	clear P((Xviwin *));
extern	void	s_ins P((Xviwin *, int, int));
extern	void	s_del P((Xviwin *, int, int));
extern	void	s_inschar P((Xviwin *, int));
extern	void	wind_goto P((Xviwin *));
extern	void	cmd_init P((Xviwin *, int));
extern	Cmd_State
		cmd_input P((Xviwin *, int));
extern	char	*get_cmd P((Xviwin *));
extern	void	gotocmd P((Xviwin *, bool_t));
extern	void	prompt P((char *));
extern	void	beep P((Xviwin *));
extern	void	disp_init P((Xviwin *, char *(*) P((void)), int, bool_t));
extern	bool_t	disp_screen P((Xviwin *));

/*
 * search.c
 */
extern	Posn	*search P((Xviwin *, Line *, int, int, char **));
extern	Posn	*nsearch P((Xviwin *, Line *, int, int, char *));
extern	Line	*linesearch P((Xviwin *, int, char **));
extern	void	do_global P((Xviwin *, Line *, Line *, char *, bool_t));
extern	long	do_substitute P((Xviwin *, Line *, Line *, char *));
extern	long	do_ampersand P((Xviwin *, Line *, Line *, char *));
extern	long	do_tilde P((Xviwin *, Line *, Line *, char *));

/*
 * signal.c
 */
extern	void	ignore_signals P((void));
extern	void	catch_signals P((void));

/*
 * status.c
 */
extern	void	init_sline P((Xviwin *));
extern	void	show_message P((Xviwin *, char *, ...));
extern	void	show_error P((Xviwin *, char *, ...));
extern	void	show_file_info P((Xviwin *));

/*
 * tags.c
 */
extern	bool_t	set_tags P((Xviwin *, Paramval, bool_t));
extern	void	tagword P((void));
extern	bool_t	do_tag P((Xviwin *, char *, bool_t, bool_t, bool_t));

/*
 * undo.c
 */
extern	void	init_undo P((Buffer *));
extern	bool_t	start_command P((Xviwin *));
extern	void	end_command P((Xviwin *));
extern	void	replchars P((Xviwin *, Line *, int, int, char *));
extern	void	repllines P((Xviwin *, Line *, long, Line *));
extern	void	replbuffer P((Xviwin *, Line *));
extern	void	undo P((Xviwin *));
extern	bool_t	set_edit P((Xviwin *, Paramval, bool_t));

/*
 * windows.c
 */
extern	Xviwin	*init_window P((VirtScr *));
extern	void	free_window P((Xviwin *));
extern	Xviwin	*split_window P((Xviwin *));
extern	void	map_window_onto_buffer P((Xviwin *, Buffer *));
extern	void	unmap_window P((Xviwin *));
extern	Xviwin	*next_window P((Xviwin *));
extern	Xviwin	*find_window P((Xviwin *, char *));
extern	void	resize_window P((Xviwin *, int));
extern	int	move_sline P((Xviwin *, int));
extern	void	update_buffer P((Buffer *));
extern	bool_t	can_split P((void));

/*
 * yankput.c
 */
extern	void	init_yankput P((void));
extern	bool_t	do_yank P((Buffer *, Posn *, Posn *, bool_t, int));
extern	bool_t	yank_str P((int, char *, bool_t));
extern	void	do_put P((Xviwin *, Posn *, int, int));
extern	void	yp_stuff_input P((Xviwin *, int, bool_t));
extern	void	yp_push_deleted P((void));
