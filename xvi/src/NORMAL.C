/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)normal.c	2.7 (Chris & John Downey) 8/24/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    normal.c
* module function:
    Main routine for processing characters in command mode
    as well as routines for handling the operators.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

static	bool_t	do_target P((int, int));
static	bool_t	do_cmd P((int, int));
static	bool_t	do_badcmd P((int, int));
static	bool_t	do_page P((int, int));
static	bool_t	do_scroll P((int, int));
static	bool_t	do_word P((int, int));
static	bool_t	do_csearch P((int, int));
static	bool_t	do_z P((int, int));
static	bool_t	do_x P((int, int));
static	bool_t	do_HLM P((int, int));
static	bool_t	do_rchar P((int, int));
static	bool_t	do_ins P((int, int));
static	void	op_shift P((int, int, int, long, Posn *, Posn *));
static	void	op_delete P((int, int, long, Posn *, Posn *));
static	void	op_change P((int, int, long, Posn *, Posn *));
static	void	op_yank P((Posn *, Posn *));
static	bool_t	dojoin P((void));

/*
 * Command type table. This is used for certain operations which are
 * the same for "classes" of commands, e.g. for disallowing their use
 * as targets of operators.
 *
 * Entries in this table having value 0 are unimplemented commands.
 *
 * If TARGET is set, the command may be used as the target for one of
 * the operators (e.g. 'c'); the default is that targets are character-
 * based unless TGT_LINE is set in which case they are line-based.
 * Similarly, the default is that targets are exclusive, unless the
 * TGT_INCLUSIVE flag is set.
 *
 * Q: WHAT DO WE DO ABOUT RETURN and LINEFEED???
 * A: The ascii_map() macro (see ascii.h) handles this for QNX (I think).
 */
#define		COMMAND		0x1		/* is implemented */
#define		TARGET		0x2		/* can serve as a target */
#define		TGT_LINE	0x4		/* a line-based target */
#define		TGT_CHAR	0		/* a char-based target */
#define		TGT_INCLUSIVE	0x8		/* an inclusive target */
#define		TGT_EXCLUSIVE	0		/* an exclusive target */
#define		TWO_CHAR	0x10		/* a two-character command */

static	struct {
    bool_t		(*c_func) P((int, int));
    unsigned char	c_flags;
} cmd_types[256] = {
 /* ^@ */	do_badcmd,	0,
 /* ^A */	do_badcmd,	0,
 /* ^B */	do_page,	COMMAND,
 /* ^C */	do_badcmd,	0,
 /* ^D */	do_scroll,	COMMAND,
 /* ^E */	do_scroll,	COMMAND,
 /* ^F */	do_page,	COMMAND,
 /* ^G */	do_cmd,		COMMAND,
 /* ^H */	do_target,	COMMAND | TARGET | TGT_CHAR | TGT_EXCLUSIVE,
 /* ^I */	do_badcmd,	0,
 /* ^J */	do_target,	COMMAND | TARGET | TGT_LINE | TGT_EXCLUSIVE,
 /* ^K */	do_badcmd,	0,
 /* ^L */	do_cmd,		COMMAND,
 /* ^M */	do_target,	COMMAND | TARGET | TGT_LINE | TGT_EXCLUSIVE,
 /* ^N */	do_target,	COMMAND | TARGET | TGT_LINE | TGT_EXCLUSIVE,
 /* ^O */	do_cmd,		COMMAND,
 /* ^P */	do_target,	COMMAND | TARGET | TGT_LINE | TGT_EXCLUSIVE,
 /* ^Q */	do_badcmd,	0,
 /* ^R */	do_cmd,		COMMAND,
 /* ^S */	do_badcmd,	0,
 /* ^T */	do_cmd,		COMMAND,
 /* ^U */	do_scroll,	COMMAND,
 /* ^V */	do_badcmd,	0,
 /* ^W */	do_cmd,		COMMAND,
 /* ^X */	do_badcmd,	0,
 /* ^Y */	do_scroll,	COMMAND,
 /* ^Z */	do_cmd,		COMMAND,
 /* ESCAPE */	do_cmd,		COMMAND,
 /* ^\ */	do_badcmd,	0,
 /* ^] */	do_cmd,		COMMAND,
 /* ^^ */	do_cmd,		COMMAND,
 /* ^_ */	do_rchar,	COMMAND,
 /* space */	do_target,	COMMAND | TARGET | TGT_CHAR | TGT_EXCLUSIVE,
 /* ! */	do_cmd,		COMMAND,
 /* " */	do_cmd,		COMMAND | TWO_CHAR,
 /* # */	do_badcmd,	0,
 /* $ */	do_target,	COMMAND | TARGET | TGT_CHAR | TGT_INCLUSIVE,
 /* % */	do_target,	COMMAND | TARGET | TGT_CHAR | TGT_INCLUSIVE,
 /* & */	do_cmd,		COMMAND,
 /* ' */	do_target,	COMMAND | TARGET | TGT_LINE | TGT_EXCLUSIVE | TWO_CHAR,
 /* ( */	do_target,	COMMAND | TARGET | TGT_LINE | TGT_EXCLUSIVE,
 /* ) */	do_target,	COMMAND | TARGET | TGT_LINE | TGT_EXCLUSIVE,
 /* * */	do_badcmd,	0,
 /* + */	do_target,	COMMAND | TARGET | TGT_LINE | TGT_EXCLUSIVE,
 /* , */	do_csearch,	COMMAND | TARGET | TGT_CHAR | TGT_EXCLUSIVE,
 /* - */	do_target,	COMMAND | TARGET | TGT_LINE | TGT_EXCLUSIVE,
 /* . */	do_cmd,		COMMAND,
 /* / */	do_cmd,		COMMAND,
 /* 0 */	do_target,	COMMAND | TARGET | TGT_CHAR | TGT_EXCLUSIVE,
 /* 1 */	do_badcmd,	0,
 /* 2 */	do_badcmd,	0,
 /* 3 */	do_badcmd,	0,
 /* 4 */	do_badcmd,	0,
 /* 5 */	do_badcmd,	0,
 /* 6 */	do_badcmd,	0,
 /* 7 */	do_badcmd,	0,
 /* 8 */	do_badcmd,	0,
 /* 9 */	do_badcmd,	0,
 /* : */	do_cmd,		COMMAND,
 /* ; */	do_csearch,	COMMAND | TARGET | TGT_CHAR | TGT_EXCLUSIVE,
 /* < */	do_cmd,		COMMAND,
 /* = */	do_badcmd,	0,
 /* > */	do_cmd,		COMMAND,
 /* ? */	do_cmd,		COMMAND,
 /* @ */	do_cmd,		COMMAND | TWO_CHAR,
 /* A */	do_ins,		COMMAND,
 /* B */	do_word,	COMMAND | TARGET | TGT_CHAR | TGT_EXCLUSIVE,
 /* C */	do_cmd,		COMMAND,
 /* D */	do_cmd,		COMMAND,
 /* E */	do_word,	COMMAND | TARGET | TGT_CHAR | TGT_INCLUSIVE,
 /* F */	do_csearch,	COMMAND | TARGET | TGT_CHAR | TGT_EXCLUSIVE | TWO_CHAR,
 /* G */	do_target,	COMMAND | TARGET | TGT_LINE | TGT_EXCLUSIVE,
 /* H */	do_HLM,		COMMAND | TARGET | TGT_LINE | TGT_EXCLUSIVE,
 /* I */	do_ins,		COMMAND,
 /* J */	do_cmd,		COMMAND,
 /* K */	do_badcmd,	0,
 /* L */	do_HLM,		COMMAND | TARGET | TGT_LINE | TGT_EXCLUSIVE,
 /* M */	do_HLM,		COMMAND | TARGET | TGT_LINE | TGT_EXCLUSIVE,
 /* N */	do_target,	COMMAND | TARGET | TGT_CHAR | TGT_EXCLUSIVE,
 /* O */	do_ins,		COMMAND,
 /* P */	do_cmd,		COMMAND,
 /* Q */	do_badcmd,	0,
 /* R */	do_cmd,		COMMAND,
 /* S */	do_cmd,		COMMAND,
 /* T */	do_csearch,	COMMAND | TARGET | TGT_CHAR | TGT_EXCLUSIVE | TWO_CHAR,
 /* U */	do_badcmd,	0,
 /* V */	do_badcmd,	0,
 /* W */	do_word,	COMMAND | TARGET | TGT_CHAR | TGT_EXCLUSIVE,
 /* X */	do_x,		COMMAND,
 /* Y */	do_cmd,		COMMAND,
 /* Z */	do_cmd,		COMMAND | TWO_CHAR,
 /* [ */	do_target,	COMMAND | TARGET | TGT_LINE | TGT_EXCLUSIVE | TWO_CHAR,
 /* \ */	do_badcmd,	0,
 /* ] */	do_target,	COMMAND | TARGET | TGT_LINE | TGT_EXCLUSIVE | TWO_CHAR,
 /* ^ */	do_target,	COMMAND | TARGET | TGT_CHAR | TGT_EXCLUSIVE,
 /* _ */	do_target,	COMMAND | TARGET | TGT_LINE | TGT_EXCLUSIVE,
 /* ` */	do_target,	COMMAND | TARGET | TGT_CHAR | TGT_EXCLUSIVE | TWO_CHAR,
 /* a */	do_ins,		COMMAND,
 /* b */	do_word,	COMMAND | TARGET | TGT_CHAR | TGT_EXCLUSIVE,
 /* c */	do_cmd,		COMMAND,
 /* d */	do_cmd,		COMMAND,
 /* e */	do_word,	COMMAND | TARGET | TGT_CHAR | TGT_INCLUSIVE,
 /* f */	do_csearch,	COMMAND | TARGET | TGT_CHAR | TGT_INCLUSIVE | TWO_CHAR,
 /* g */	do_cmd,		COMMAND,
 /* h */	do_target,	COMMAND | TARGET | TGT_CHAR | TGT_EXCLUSIVE,
 /* i */	do_ins,		COMMAND,
 /* j */	do_target,	COMMAND | TARGET | TGT_LINE | TGT_EXCLUSIVE,
 /* k */	do_target,	COMMAND | TARGET | TGT_LINE | TGT_EXCLUSIVE,
 /* l */	do_target,	COMMAND | TARGET | TGT_CHAR | TGT_EXCLUSIVE,
 /* m */	do_cmd,		COMMAND | TWO_CHAR,
 /* n */	do_target,	COMMAND | TARGET | TGT_CHAR | TGT_EXCLUSIVE,
 /* o */	do_ins,		COMMAND,
 /* p */	do_cmd,		COMMAND,
 /* q */	do_badcmd,	0,
 /* r */	do_cmd,		COMMAND,
 /* s */	do_cmd,		COMMAND,
 /* t */	do_csearch,	COMMAND | TARGET | TGT_CHAR | TGT_INCLUSIVE | TWO_CHAR,
 /* u */	do_cmd,		COMMAND,
 /* v */	do_badcmd,	0,
 /* w */	do_word,	COMMAND | TARGET | TGT_CHAR | TGT_EXCLUSIVE,
 /* x */	do_x,		COMMAND,
 /* y */	do_cmd,		COMMAND,
 /* z */	do_z,		COMMAND | TWO_CHAR,
 /* { */	do_target,	COMMAND | TARGET | TGT_LINE | TGT_EXCLUSIVE,
 /* | */	do_target,	COMMAND | TARGET | TGT_CHAR | TGT_EXCLUSIVE,
 /* } */	do_target,	COMMAND | TARGET | TGT_LINE | TGT_EXCLUSIVE,
 /* ~ */	do_rchar,	COMMAND,
 /* DEL */	do_badcmd,	0,
 /* K_HELP */	do_cmd,		COMMAND,
 /* K_UNDO */	do_cmd,		COMMAND,
 /* K_INSERT */	do_ins,		COMMAND,
 /* K_HOME */	do_HLM,		COMMAND | TARGET | TGT_CHAR | TGT_EXCLUSIVE,
 /* K_UARROW */	do_target,	COMMAND | TARGET | TGT_LINE | TGT_EXCLUSIVE,
 /* K_DARROW */	do_target,	COMMAND | TARGET | TGT_LINE | TGT_EXCLUSIVE,
 /* K_LARROW */	do_target,	COMMAND | TARGET | TGT_CHAR | TGT_EXCLUSIVE,
 /* K_RARROW */	do_target,	COMMAND | TARGET | TGT_CHAR | TGT_EXCLUSIVE,
 /* K_CGRAVE */	do_cmd,		COMMAND,
};

#define	NOP		'\0'		/* no pending operation */

static	int	operator = NOP;		/* current pending operator */

/*
 * When a cursor motion command is made, it is marked as being a character
 * or line oriented motion. Then, if an operator is in effect, the operation
 * becomes character or line oriented accordingly.
 *
 * Character motions are marked as being inclusive or not. Most char.
 * motions are inclusive, but some (e.g. 'w') are not.
 */

static	enum {
    m_bad,		/* 'bad' motion type marks unusable yank buf */
    m_nonincl,		/* non-inclusive character motion */
    m_incl,		/* inclusive character motion */
    m_line		/* line-based motion */
} mtype;		/* type of the current cursor motion */

/*
 * Cursor position at start of operator.
 */
static	Posn	startop;

/*
 * Operators can have counts either before the operator, or between the
 * operator and the following cursor motion as in:
 *
 *	d3w or 3dw
 *
 * The number is initially stored in Prenum as it is processed.
 *
 * If a count is given before an operator, it is saved in opnum when the
 * initial recognition of the operator takes place. If normal() is called
 * with a pending operator, the count in opnum (if present) overrides
 * any count that comes later.
 */
static	long	Prenum;
static	long	opnum;

/*
 * This variable contains the name of the yank/put buffer we are
 * currently using. It is set by the " command. The default value
 * is always '@'; other values are a-z and possibly ':'.
 */
static	int	cur_yp_name = '@';

/*
 * This state variable is TRUE if we got a preceding buffer
 * name: if set, the buffer_name variable contains the letter
 * which was given as the buffer name.
 */
static	bool_t	got_name = FALSE;
static	char	buffer_name;

/*
 * Return value of Prenum unless it is 0, in which case return the
 * default value of 1.
 */
#define	LDEF1PRENUM	((Prenum == 0 ? 1L : Prenum))

/*
 * Return value of Prenum as an int, unless it is 0, in which case
 * return the default value of 1.
 *
 * Note that this assumes that Prenum will never be negative.
 */
#define	IDEF1PRENUM	(Prenum == 0 ? \
			 1 : \
			 sizeof (int) == sizeof (long) || \
			   Prenum <= INT_MAX ? \
			 (int) Prenum : \
			 INT_MAX)

/*
 * Redo buffer.
 */
struct redo {
    enum {
    	r_insert,
	r_replace1,
	r_normal
    }		r_mode;
    Flexbuf	r_fb;
} Redo;

/*
 * Execute a command in "normal" (i.e. command) mode.
 */
bool_t
normal(c)
register int	c;
{
    /*
     * This variable is used to recall whether we got
     * an operator last time, to decide whether we
     * should apply it this time.
     */
    register bool_t	finish_op;

    /*
     * TRUE if we are awaiting a second character to finish the
     * current command - this is for two-character commands like
     * ZZ, and for commands taking a single character argument.
     * The "first_char" and "second_char" variables are for the
     * first character (stored between calls), and the second one.
     */
    static bool_t	two_char = FALSE;
    static int		first_char;
    int			second_char;
    unsigned char	cflags;
    bool_t		(*cfunc) P((int, int)) = NULL;


    /*
     * If the character is a digit, and it is not a leading '0',
     * compute Prenum instead of doing a command.  Leading zeroes
     * are treated specially because '0' is a valid command.
     *
     * If two_char is set, don't treat digits this way; they are
     * passed in as the second character. This is because none of
     * the two-character commands are allowed to take prenums in
     * the middle; you want a prenum, you have to type it before
     * the command. So "t3" works as you might expect it to.
     */
    if (!two_char && is_digit(c) && (c != '0' || Prenum > 0)) {
	Prenum = Prenum * 10 + (c - '0');
	return(FALSE);
    }

    /*
     * If there is an operator pending, then the command we take
     * this time will terminate it.  Finish_op tells us to finish
     * the operation before returning this time (unless it was
     * cancelled).
     */
    finish_op = (operator != NOP);

    /*
     * If we're in the middle of an operator AND we had a count before
     * the operator, then that count overrides the current value of
     * Prenum. What this means effectively, is that commands like
     * "3dw" get turned into "d3w" which makes things fall into place
     * pretty neatly.
     */
    if (finish_op || two_char) {
	if (opnum != 0) {
	    Prenum = opnum;
	}
    } else {
	opnum = 0;
    }

    /*
     * If we got given a buffer name last time around, it is only
     * good for one operation; so at the start of each new command
     * we set or clear the yankput module's idea of the buffer name.
     * We don't do this if finish_op is set because it is not the
     * start of a new command.
     */
    if (!finish_op) {
	cur_yp_name = got_name ? buffer_name : '@';
	got_name = FALSE;
    }

    if (c > (sizeof(cmd_types) / sizeof(cmd_types[0]) - 1)) {
	operator = NOP;
	Prenum = 0;
    	beep(curwin);
	return(FALSE);
    }

    /*
     * If two_char is set, it means we got the first character of
     * a two-character command last time. So check the second char,
     * and set cflags appropriately.
     */
    if (two_char) {
	second_char = c;
	two_char = FALSE;
	cflags = cmd_types[ascii_map(first_char)].c_flags;
	cfunc = cmd_types[ascii_map(first_char)].c_func;

	/*
	 * This seems to be a universal rule - if a two-character
	 * command has ESC as the second character, it means "abort".
	 */
	if (second_char == ESC) {
	    operator = NOP;
	    finish_op = FALSE;
	    Prenum = 0;
	    return(FALSE);
	}

    } else {
	/*
	 * Received a command. Find out its characteristics ...
	 */
	first_char = c;
	second_char = '\0';
	cflags = cmd_types[ascii_map(first_char)].c_flags;
	cfunc = cmd_types[ascii_map(first_char)].c_func;

	/*
	 * It's a two-character command. So wait until we get
	 * the second character before proceeding.
	 */
	if (cflags & TWO_CHAR) {
	    two_char = TRUE;
	    first_char = c;
	    if (Prenum != 0) {
		opnum = Prenum;
		Prenum = 0;
	    }
	    return(FALSE);
	}

	/*
	 * If we got an operator last time, and the user
	 * typed the same character again, we fake out the
	 * default "apply to this line" rule by changing
	 * the input character to a '_' which means "the
	 * current line."
	 */
	if (finish_op) {
	    if (operator == c) {
		first_char = c = '_';
		cflags = cmd_types[ascii_map(c)].c_flags;
		cfunc = cmd_types[ascii_map(c)].c_func;
	    } else if (!(cflags & TARGET)) {
		beep(curwin);
		operator = NOP;
		Prenum = 0;
		return(FALSE);
	    }
	}
    }

    /*
     * At this point, cfunc must be set - if not, the entry in the
     * command table is zero, so disallow the input character.
     */
    if (cfunc == NULL) {
	operator = NOP;
	Prenum = 0;
    	beep(curwin);
	return(FALSE);
    }

    if (cflags & TARGET) {

	/*
	 * A cursor movement command.
	 */

	if (cflags & TGT_LINE) {
	    mtype = m_line;
	} else {
	    if (cflags & TGT_INCLUSIVE) {
		mtype = m_incl;
	    } else {
		mtype = m_nonincl;
	    }
	}

	if (!(*cfunc)(first_char, second_char)) {
	    beep(curwin);
	    operator = NOP;
	    Prenum = 0;
	    return(FALSE);
	}

	/*
	 * If an operation is pending, handle it...
	 */
	if (finish_op) {
	    Posn	top, bot;

	    top = startop;
	    bot = *curwin->w_cursor;

	    /*
	     * Put the cursor back to its starting position.
	     */
	    move_cursor(curwin, startop.p_line, startop.p_index);

	    if (lt(&bot, &top)) {
		pswap(&top, &bot);
	    }

	    switch (operator) {
	    case '<':
	    case '>':
		op_shift(operator, first_char, second_char, Prenum, &top, &bot);
		break;

	    case 'd':
		op_delete(first_char, second_char, Prenum, &top, &bot);
		break;

	    case 'y':
		op_yank(&top, &bot);
		break;

	    case 'c':
		op_change(first_char, second_char, Prenum, &top, &bot);
		break;

	    case '!':
		specify_pipe_range(curwin, top.p_line, bot.p_line);
		cmd_init(curwin, '!');
		break;

	    default:
		beep(curwin);
	    }
	    operator = NOP;
	}
    } else {
	/*
	 * A command that does something.
	 * Since it isn't a target, no operators need apply.
	 */
	if (finish_op) {
	    beep(curwin);
	    operator = NOP;
	}

	(void) (*cfunc)(first_char, second_char);
    }

    Prenum = 0;
    return(TRUE);
}

/*
 * Handle cursor movement commands.
 * These are used simply to move the cursor somewhere,
 * and also as targets for the various operators.
 *
 * If the return value is FALSE, the caller will complain
 * loudly to the user and cancel any outstanding operator.
 * The cursor should hopefully not have moved.
 *
 * Arguments are the first and second characters (where appropriate).
 */
static bool_t
do_target(c1, c2)
int	c1, c2;
{
    bool_t		skip_spaces = FALSE;
    bool_t		retval = TRUE;

    switch (c1) {
    case 'G':
	setpcmark(curwin);
	do_goto((Prenum > 0) ? Prenum : MAX_LINENO);
	skip_spaces = TRUE;
	break;

    case 'l':
    case ' ':
	c1 = K_RARROW;
	/* fall through ... */
    case K_RARROW:
    case 'h':
    case K_LARROW:
    case CTRL('H'):
    {
	register bool_t (*mvfunc) P((Xviwin *, bool_t));
	register long	n;
	register long	i;

	if (c1 == K_RARROW) {
	    mvfunc = one_right;
	} else {
	    mvfunc = one_left;
	}

	n = LDEF1PRENUM;
	for (i = 0; i < n; i++) {
	    if (!(*mvfunc)(curwin, FALSE)) {
		break;
	    }
	}
	if (i == 0) {
	    retval = FALSE;
	} else {
	    curwin->w_set_want_col = TRUE;
	}
	break;
    }

    case '-':
	skip_spaces = TRUE;
	/* FALL THROUGH */
    case 'k':
    case K_UARROW:
    case CTRL('P'):
	if (!oneup(curwin, LDEF1PRENUM)) {
	    retval = FALSE;
	}
	break;

    case '+':
    case '\r':
	skip_spaces = TRUE;
	/* FALL THROUGH */
    case '\n':
    case 'j':
    case K_DARROW:
    case CTRL('N'):
	if (!onedown(curwin, LDEF1PRENUM)) {
	    retval = FALSE;
	}
	break;

    /*
     * This is a strange motion command that helps make
     * operators more logical. It is actually implemented,
     * but not documented in the real 'vi'. This motion
     * command actually refers to "the current line".
     * Commands like "dd" and "yy" are really an alternate
     * form of "d_" and "y_". It does accept a count, so
     * "d3_" works to delete 3 lines.
     */
    case '_':
	(void) onedown(curwin, LDEF1PRENUM - 1);
	break;

    case '|':
	begin_line(curwin, FALSE);

	if (Prenum > 0) {
	    coladvance(curwin, LONG2INT(Prenum - 1));
	}
	curwin->w_curswant = Prenum - 1;
	break;

    case '%':
	{
	    Posn	*pos = showmatch();

	    if (pos == NULL) {
		retval = FALSE;
	    } else {
		setpcmark(curwin);
		move_cursor(curwin, pos->p_line, pos->p_index);
		curwin->w_set_want_col = TRUE;
	    }
	}
	break;

    case '$':
	while (one_right(curwin, FALSE))
	    ;
	curwin->w_curswant = INT_MAX;
		/* so we stay at the end ... */
	curwin->w_set_want_col = FALSE;
	break;

    case '^':
    case '0':
	begin_line(curwin, c1 == '^');
	break;

    case 'n':
    case 'N':
	curwin->w_set_want_col = TRUE;
	(void) dosearch(curwin, "", c1);
	break;

    case '(':
    case ')':
    case '{':
    case '}':
    case '[':
    case ']':
    {
    	int	dir = FORWARD;
	char	*pattern;
	Posn	*newpos;

	switch (c1) {
	case '(':
	    dir = BACKWARD;
	    /*FALLTHROUGH*/
	case ')':
	    pattern = Ps(P_sentences);
	    break;

	case '{':
	    dir = BACKWARD;
	    /*FALLTHROUGH*/
	case '}':
	    pattern = Ps(P_paragraphs);
	    break;

	case '[':
	    dir = BACKWARD;
	    /*FALLTHROUGH*/
	case ']':
	    if (c1 != c2) {
		retval = FALSE;
	    } else {
		pattern = Ps(P_sections);
	    }
	}
	if (retval) {
	    curwin->w_set_want_col = TRUE;
	    newpos = find_pattern(pattern, dir, IDEF1PRENUM);
	    if (newpos != NULL) {
		setpcmark(curwin);
		move_cursor(curwin, newpos->p_line, newpos->p_index);
	    } else {
		retval = FALSE;
	    }
	}
	break;
    }

    case '\'':
    case '`':
    {
	Posn	*mark;

	mark = getmark(c2, curbuf);
	if (mark == NULL) {
	    retval = FALSE;
	} else {
	    Posn	dest;

	    /*
	     * Record posn before re-setting the mark -
	     * so that we don't accidentally side-effect
	     * the place we are moving to! What a hack.
	     */
	    dest = *mark;

	    setpcmark(curwin);

	    move_cursor(curwin, dest.p_line, c1 == '`' ? dest.p_index : 0);
	    if (c1 == '`') {
		mtype = m_nonincl;
	    } else {
		skip_spaces = TRUE;
	    }
	}
	break;
    }
    }

    if (retval && skip_spaces) {
	begin_line(curwin, TRUE);
    }

    return(retval);
}

static bool_t
do_cmd(c1, c2)
int	c1, c2;
{
    switch (c1) {
    case K_HELP:
	do_help(curwin);
	break;

    case CTRL('R'):
    case CTRL('L'):
	redraw_screen();
	break;

    case CTRL('G'):
	show_file_info(curwin);
	break;

    case CTRL(']'):		/* :ta to current identifier */
	tagword();
	break;

    /*
     * Some convenient abbreviations...
     */
    case 'D':
	stuff("\"%cd$", cur_yp_name);
	break;

    case 'Y':
	stuff("\"%c%dyy", cur_yp_name, IDEF1PRENUM);
	break;

    case 'C':
	stuff("\"%cc$", cur_yp_name);
	break;

    case 'S':
    	stuff("\"%c%dcc", cur_yp_name, IDEF1PRENUM);
	break;

    /*
     * Operators.
     */
    case 'd':
    case 'c':
    case 'y':
    case '>':
    case '<':
    case '!':
	if (Prenum != 0)
	    opnum = Prenum;
	startop = *curwin->w_cursor;
	operator = c1;
	break;

    case 'p':
    case 'P':
	Redo.r_mode = r_normal;
	do_put(curwin, curwin->w_cursor, (c1 == 'p') ? FORWARD : BACKWARD,
							cur_yp_name);
	if (is_digit(cur_yp_name) && cur_yp_name != '0' && cur_yp_name != '9') {
	    cur_yp_name++;
	}
	flexclear(&Redo.r_fb);
	(void) lformat(&Redo.r_fb, "\"%c%d%c", cur_yp_name, IDEF1PRENUM, c1);
	break;

    case 's':		/* substitute characters */
	start_command(curwin);
	replchars(curwin, curwin->w_cursor->p_line,
			    curwin->w_cursor->p_index, IDEF1PRENUM, "");
	updateline(curwin);
	Redo.r_mode = r_insert;
	flexclear(&Redo.r_fb);
	(void) lformat(&Redo.r_fb, "%lds", IDEF1PRENUM);
	startinsert(FALSE);
	break;

    case ':':
    case '?':
    case '/':
	cmd_init(curwin, c1);
	break;

    case '&':
	(void) do_ampersand(curwin, curwin->w_cursor->p_line,
				    curwin->w_cursor->p_line, "");
	begin_line(curwin, TRUE);
	updateline(curwin);
	break;

    case 'R':
    case 'r':
	Redo.r_mode = (c1 == 'r') ? r_replace1 : r_insert;
	flexclear(&Redo.r_fb);
	flexaddch(&Redo.r_fb, c1);
	startreplace(c1);
	break;

    case 'J':
	if (!dojoin())
	    beep(curwin);

	Redo.r_mode = r_normal;
	flexclear(&Redo.r_fb);
	flexaddch(&Redo.r_fb, c1);
	update_buffer(curbuf);
	break;

    case K_CGRAVE:			/* shorthand command */
#ifndef	QNX
    /*
     * We can't use this key on QNX.
     */
    case CTRL('^'):
#endif
	do_alt_edit(curwin);
	break;

    case 'u':
    case K_UNDO:
	undo(curwin);
	break;

    case CTRL('Z'):			/* suspend editor */
	do_suspend(curwin);
	break;

    /*
     * Buffer handling.
     */
    case CTRL('T'):			/* shrink window */
	resize_window(curwin, - IDEF1PRENUM);
	move_cursor_to_window(curwin);
	break;

    case CTRL('W'):			/* grow window */
	resize_window(curwin, IDEF1PRENUM);
	break;

    case CTRL('O'):	/* make window as large as possible */
	resize_window(curwin, INT_MAX);
	break;

    case 'g':
	/*
	 * Find the next window that the cursor
	 * can be displayed in; i.e. at least one
	 * text row is displayed.
	 */
	do {
	    curwin = next_window(curwin);
	} while (curwin->w_nrows < 2);
	curbuf = curwin->w_buffer;
	move_cursor_to_window(curwin);
	wind_goto(curwin);
	break;

    case '"':
	got_name = TRUE;
	buffer_name = c2;
	break;

    case '@':
	yp_stuff_input(curwin, c2, TRUE);
	break;

    /*
     * Marks
     */
    case 'm':
	if (!setmark(c2, curbuf, curwin->w_cursor))
	    beep(curwin);
	break;

    case 'Z':		/* write, if changed, and exit */
	if (c2 != 'Z') {
	    beep(curwin);
	    break;
	}

	/*
	 * Make like a ":x" command.
	 */
	do_xit(curwin);
	break;

    case '.':
	/*
	 * '.', meaning redo. As opposed to '.' as a target.
	 */
	stuff("%s", flexgetstr(&Redo.r_fb));
	if (Redo.r_mode != r_normal) {
	    yp_stuff_input(curwin, '<', TRUE);
	    if (Redo.r_mode == r_insert) {
		stuff("%c", ESC);
	    }
	}
	break;

    default:
	beep(curwin);
	break;
    }

    return(FALSE);
}

static bool_t
do_badcmd(c1, c2)
int	c1, c2;
{
    beep(curwin);
    return(FALSE);
}

/*
 * Handle page motion (control-F or control-B).
 */
static bool_t
do_page(c1, c2)
register int	c1, c2;
{
    long		overlap;
    long		n;

    /*
     * First move the cursor to the top of the screen
     * (for ^B), or to the top of the next screen (for ^F).
     */
    move_cursor(curwin, (c1 == CTRL('B')) ?
		   curwin->w_topline : curwin->w_botline, 0);

    /*
     * Cursor could have moved to the lastline of the buffer,
     * if the window is at the end of the buffer. Disallow
     * the cursor from being outside the buffer's bounds.
     */
    if (curwin->w_cursor->p_line == curbuf->b_lastline) {
	move_cursor(curwin, curbuf->b_lastline->l_prev, 0);
    }

    /*
     * Decide on the amount of overlap to use.
     */
    if (curwin->w_nrows > 10) {
	/*
	 * At least 10 text lines in window.
	 */
	overlap = 2;
    } else if (curwin->w_nrows > 3) {
	/*
	 * Between 3 and 9 text lines in window.
	 */
	overlap = 1;
    } else {
	/*
	 * 1 or 2 text lines in window.
	 */
	overlap = 0;
    }

    /*
     * Given the overlap, decide where to move the cursor;
     * this will determine the new top line of the screen.
     */
    if (c1 == CTRL('F')) {
	n = - overlap;
	n += (LDEF1PRENUM - 1) * (curwin->w_nrows - overlap - 1);
    } else {
	n = (- LDEF1PRENUM) * (curwin->w_nrows - overlap - 1);
    }

    if (n > 0) {
	(void) onedown(curwin, n);
    } else {
	(void) oneup(curwin, -n);
    }

    /*
     * Redraw the screen with the cursor at the top.
     */
    begin_line(curwin, TRUE);
    curwin->w_topline = curwin->w_cursor->p_line;
    update_window(curwin);

    if (c1 == CTRL('B')) {
	/*
	 * And move it to the bottom.
	 */
	move_window_to_cursor(curwin);
	cursupdate(curwin);
	move_cursor(curwin, curwin->w_botline->l_prev, 0);
	begin_line(curwin, TRUE);
    }

    /*
     * Finally, show where we are in the file.
     */
    show_file_info(curwin);

    return(FALSE);
}

static bool_t
do_scroll(c1, c2)
int	c1, c2;
{
    switch (c1) {
    case CTRL('D'):
	scrollup(curwin, curwin->w_nrows / 2);
	(void) onedown(curwin, (long) (curwin->w_nrows / 2));
	break;

    case CTRL('U'):
	scrolldown(curwin, curwin->w_nrows / 2);
	(void) oneup(curwin, (long) (curwin->w_nrows / 2));
	break;

    case CTRL('E'):
	scrollup(curwin, (unsigned) IDEF1PRENUM);
	break;

    case CTRL('Y'):
	scrolldown(curwin, (unsigned) IDEF1PRENUM);
	break;
    }

    update_window(curwin);
    move_cursor_to_window(curwin);
    return(FALSE);
}

/*
 * Handle word motion ('w', 'W', 'b', 'B', 'e' or 'E').
 */
static bool_t
do_word(c1, c2)
register int	c1, c2;
{
    register Posn	*(*func) P((Posn *, int, bool_t));
    register long	n;
    register int	lc;
    register int	type;
    Posn		pos;

    if (is_upper(c1)) {
	type = 1;
	lc = to_lower(c1);
    } else {
	type = 0;
	lc = c1;
    }
    curwin->w_set_want_col = TRUE;

    switch (lc) {
    case 'b':
	func = bck_word;
	break;

    case 'w':
	func = fwd_word;
	break;

    case 'e':
	func = end_word;
	break;
    }

    pos = *curwin->w_cursor;

    for (n = LDEF1PRENUM; n > 0; n--) {
	Posn	*newpos;
	bool_t	skip_whites;

	/*
	 * "cw" is a special case; the whitespace after
	 * the end of the last word involved in the change
	 * does not get changed. The following code copes
	 * with this strangeness.
	 */
	if (n == 1 && operator == 'c' && lc == 'w') {
	    skip_whites = FALSE;
	    mtype = m_incl;
	} else {
	    skip_whites = TRUE;
	}

	newpos = (*func)(&pos, type, skip_whites);

	if (newpos == NULL) {
	    return(FALSE);
	}

	if (n == 1 && lc == 'w' && operator != NOP &&
					newpos->p_line != pos.p_line) {
	    /*
	     * We are on the last word to be operated
	     * upon, and have crossed the line boundary.
	     * This should not happen, so back up to
	     * the end of the line the word is on.
	     */
	    while (dec(newpos) == mv_SAMELINE)
		;
	    mtype = m_incl;
	}

	if (skip_whites == FALSE) {
	    (void) dec(newpos);
	}
	pos = *newpos;
    }
    move_cursor(curwin, pos.p_line, pos.p_index);
    return(TRUE);
}

static bool_t
do_csearch(c1, c2)
int	c1, c2;
{
    bool_t	retval = TRUE;
    Posn	*pos;
    int	dir;

    switch (c1) {
    case 'T':
    case 't':
    case 'F':
    case 'f':
	if (is_upper(c1)) {
	    dir = BACKWARD;
	    c1 = to_lower(c1);
	} else {
	    dir = FORWARD;
	}

	curwin->w_set_want_col = TRUE;
	pos = searchc(c2, dir, (c1 == 't'), IDEF1PRENUM);
	break;

    case ',':
    case ';':
	/*
	 * This should be FALSE for a backward motion.
	 * How do we know it's a backward motion?
	 *
	 * Fix it later.
	 */
	mtype = m_incl;
	curwin->w_set_want_col = TRUE;
	pos = crepsearch(curbuf, c1 == ',', IDEF1PRENUM);
	break;
    }
    if (pos == NULL) {
	retval = FALSE;
    } else {
	move_cursor(curwin, pos->p_line, pos->p_index);
    }
    return(retval);
}

/*
 * Handle adjust window command ('z').
 */
static bool_t
do_z(c1, c2)
int	c1, c2;
{
    Line	*lp;
    int	l;
    int	znum;

    switch (c2) {
    case '\n':				/* put cursor at top of screen */
    case '\r':
	znum = 1;
	break;

    case '.':				/* put cursor in middle of screen */
	znum = curwin->w_nrows / 2;
	break;

    case '-':				/* put cursor at bottom of screen */
	znum = curwin->w_nrows - 1;
	break;

    default:
	return(FALSE);
    }

    if (Prenum > 0) {
	do_goto(Prenum);
    }
    lp = curwin->w_cursor->p_line;
    for (l = 0; l < znum && lp != curbuf->b_line0; ) {
	l += plines(curwin, lp);
	curwin->w_topline = lp;
	lp = lp->l_prev;
    }
    cursupdate(curwin);
    update_window(curwin);

    return(TRUE);
}

/*
 * Handle character delete commands ('x' or 'X').
 */
static bool_t
do_x(c1, c2)
int	c1, c2;
{
    Posn	*curp;
    Posn	lastpos;
    int		nchars;
    int		i;

    nchars = IDEF1PRENUM;
    Redo.r_mode = r_normal;
    flexclear(&Redo.r_fb);
    (void) lformat(&Redo.r_fb, "%d%c", nchars, c1);
    curp = curwin->w_cursor;

    if (c1 == 'X') {
	for (i = 0; i < nchars && one_left(curwin, FALSE); i++)
	    ;
	nchars = i;
	if (nchars == 0) {
	    beep(curwin);
	    return(TRUE);
	}

    } else /* c1 == 'x' */ {
	char	*line;

	/*
	 * Ensure that nchars is not too big.
	 */
	line = curp->p_line->l_text + curp->p_index;
	for (i = 0; i < nchars && line[i] != '\0'; i++)
	    ;
	nchars = i;

	if (curp->p_line->l_text[0] == '\0') {
	    /*
	     * Can't do it on a blank line.
	     */
	    beep(curwin);
	    return(TRUE);
	}
    }

    lastpos.p_line = curp->p_line;
    lastpos.p_index = curp->p_index + nchars - 1;
    yp_push_deleted();
    (void) do_yank(curbuf, curp, &lastpos, TRUE, cur_yp_name);
    replchars(curwin, curp->p_line, curp->p_index, nchars, "");
    if (curp->p_line->l_text[curp->p_index] == '\0') {
	(void) one_left(curwin, FALSE);
    }
    updateline(curwin);
    return(TRUE);
}

/*
 * Handle home ('H') end of page ('L') and middle line ('M') motion commands.
 */
static bool_t
do_HLM(c1, c2)
int	c1, c2;
{
    register bool_t	(*mvfunc) P((Xviwin *, long));
    register long	n;

    if (c1 == K_HOME) {
	c1 = 'H';
    }

    /*
     * Silly to specify a number before 'H' or 'L'
     * which would move us off the screen.
     */
    if (Prenum >= curwin->w_nrows) {
	return(FALSE);
    }
    
    move_cursor(curwin, (c1 == 'L') ? curwin->w_botline->l_prev :
    					curwin->w_topline, 0);

    switch (c1) {
    case 'H':
	mvfunc = onedown;
	n = Prenum - 1;
	break;
    case 'L':
	mvfunc = oneup;
	n = Prenum - 1;
	break;
    case 'M':
	mvfunc = onedown;
	n = (long) (curwin->w_nrows - 1) / 2;
    }

    (void) (*mvfunc)(curwin, n);
    begin_line(curwin, TRUE);
    return(TRUE);
}

/*
 * Handle '~' and CTRL('_') commands.
 */
static bool_t
do_rchar(c1, c2)
int	c1, c2;
{
    Posn	*cp;
    char	*tp;
    int		c;
    char	newc[2];

    Redo.r_mode = r_normal;
    flexclear(&Redo.r_fb);
    flexaddch(&Redo.r_fb, c1);
    cp = curwin->w_cursor;
    tp = cp->p_line->l_text;
    if (tp[0] == '\0') {
	/*
	 * Can't do it on a blank line.
	 */
	beep(curwin);
	return(FALSE);
    }
    c = tp[cp->p_index];

    switch (c1) {
    case '~':
	newc[0] = is_alpha(c) ?
		    is_lower(c) ?
		    	to_upper(c)
			: to_lower(c)
		    : c;
	break;
    case CTRL('_'):			/* flip top bit */
#ifdef	TOP_BIT
	newc[0] = c ^ TOP_BIT;
	if (newc[0] == '\0') {
	    newc[0] = c;
	}
#else	/* not TOP_BIT */
	beep(curwin);
	return(FALSE);
#endif	/* TOP_BIT */
    }

    newc[1] = '\0';
    replchars(curwin, cp->p_line, cp->p_index, 1, newc);
    updateline(curwin);
    (void) one_right(curwin, FALSE);
    return(FALSE);
}

/*
 * Handle commands which just go into insert mode
 * ('i', 'a', 'I', 'A', 'o', 'O').
 */
static bool_t
do_ins(c1, c2)
int	c1, c2;
{
    bool_t	startpos = TRUE;	/* FALSE means start position moved */

    if (!start_command(curwin)) {
	return(FALSE);
    }

    Redo.r_mode = r_insert;
    flexclear(&Redo.r_fb);
    flexaddch(&Redo.r_fb, c1);

    switch (c1) {
    case 'o':
    case 'O':
	if (((c1 == 'o') ? openfwd(FALSE) : openbwd()) == FALSE) {
	    beep(curwin);
	    end_command(curwin);
	    return(FALSE);
	}
	break;

    case 'I':
	begin_line(curwin, TRUE);
	startpos = FALSE;
	break;

    case 'A':
	while (one_right(curwin, TRUE))
	    ;
	startpos = FALSE;
	break;

    case 'a':
	/*
	 * 'a' works just like an 'i' on the next character.
	 */
	(void) one_right(curwin, TRUE);
	startpos = FALSE;
    }

    startinsert(startpos);
    return(FALSE);
}

/*
 * Handle a shift operation. The prenum and operator/operands are
 * passed, along with the first and last positions to be shifted.
 */
static void
op_shift(op, c1, c2, num, top, bottom)
int	op;
int	c1, c2;
long	num;
Posn	*top;
Posn	*bottom;
{
    /*
     * Do the shift.
     */
    tabinout(op, top->p_line, bottom->p_line);

    /*
     * Put cursor on first non-white of line; this is good if the
     * cursor is in the range of lines specified, which is normally
     * will be.
     */
    begin_line(curwin, TRUE);
    update_buffer(curbuf);

    /*
     * Construct redo buffer.
     */
    Redo.r_mode = r_normal;
    flexclear(&Redo.r_fb);
    if (num != 0) {
	(void) lformat(&Redo.r_fb, "%c%ld%c%c", op, num, c1, c2);
    } else {
	(void) lformat(&Redo.r_fb, "%c%c%c", op, c1, c2);
    }
}

/*
 * op_delete - handle a delete operation
 * The characters "c1" and "c2" are the target character and
 * its argument (if it takes one) respectively. E.g. 'f', '/'.
 * The "num" argument is the numeric prefix.
 */
static void
op_delete(c1, c2, num, top, bottom)
int	c1, c2;
long	num;
Posn	*top;
Posn	*bottom;
{
    long	nlines;
    int		n;

    /*
     * If the target is non-inclusive, move back a character.
     * We assume it is okay to do this, and it seems to work,
     * so no checking is performed at the moment.
     */
    if (mtype == m_nonincl) {
	(void) dec(bottom);
    }

    nlines = cntllines(top->p_line, bottom->p_line);

    /*
     * Do a yank of whatever we're about to delete. If there's too much
     * stuff to fit in the yank buffer, disallow the delete, since we
     * probably wouldn't have enough memory to do it anyway.
     */
    yp_push_deleted();
    if (!do_yank(curbuf, top, bottom, (mtype != m_line), cur_yp_name)) {
	show_error(curwin, "Not enough memory to perform delete");
	return;
    }

    if (mtype == m_line) {
	/*
	 * Put the cursor at the start of the section to be deleted
	 * so that repllines will correctly update it and the screen
	 * pointer, and update the screen.
	 */
	move_cursor(curwin, top->p_line, 0);
	repllines(curwin, top->p_line, nlines, (Line *) NULL);
	begin_line(curwin, TRUE);
    } else {
	/*
	 * After a char-based delete, the cursor should always be
	 * on the character following the last character of the
	 * section being deleted. The easiest way to achieve this
	 * is to put it on the character before the section to be
	 * deleted (which will not be affected), and then move one
	 * place right afterwards.
	 */
	move_cursor(curwin, top->p_line,
			    top->p_index - ((top->p_index > 0) ? 1 : 0));

	if (top->p_line == bottom->p_line) {
	    /*
	     * Delete characters within line.
	     */
	    n = (bottom->p_index - top->p_index) + 1;
	    replchars(curwin, top->p_line, top->p_index, n, "");
	} else {
	    /*
	     * Character-based delete between lines.
	     * So we actually have to do three deletes;
	     * one to delete to the end of the top line,
	     * one to delete the intervening lines, and
	     * one to delete up to the target position.
	     */
	    if (!start_command(curwin)) {
		return;
	    }

	    /*
	     * First delete part of the last line.
	     */
	    replchars(curwin, bottom->p_line, 0, bottom->p_index + 1, "");

	    /*
	     * Now replace the rest of the top line with the
	     * remainder of the bottom line.
	     */
	    replchars(curwin, top->p_line, top->p_index, INT_MAX,
						bottom->p_line->l_text);

	    /*
	     * Finally, delete all lines from (top + 1) to bot,
	     * inclusive.
	     */
	    repllines(curwin, top->p_line->l_next,
			    cntllines(top->p_line, bottom->p_line) - 1,
			    (Line *) NULL);

	    end_command(curwin);
	}

	if (top->p_index > 0) {
	    (void) one_right(curwin, FALSE);
	}
    }

    /*
     * Construct redo buffer.
     */
    Redo.r_mode = r_normal;
    flexclear(&Redo.r_fb);
    if (num != 0) {
	(void) lformat(&Redo.r_fb, "d%ld%c%c", num, c1, c2);
    } else {
	(void) lformat(&Redo.r_fb, "d%c%c", c1, c2);
    }

    if (mtype != m_line && nlines == 1) {
	updateline(curwin);
    } else {
	update_buffer(curbuf);
    }
}

/*
 * op_change - handle a change operation
 */
static void
op_change(c1, c2, num, top, bottom)
int	c1, c2;
long	num;
Posn	*top;
Posn	*bottom;
{
    bool_t	doappend;	/* true if we should do append, not insert */

    /*
     * Start the command here so the initial delete gets
     * included in the meta-command and hence undo will
     * work properly.
     */
    if (!start_command(curwin)) {
	return;
    }

    if (mtype == m_line) {
	long	nlines;
	Line	*lp;

	/*
	 * This is a bit awkward ... for a line-based change, we don't
	 * actually delete the whole range of lines, but instead leave
	 * the first line in place and delete its text after the cursor
	 * position. However, yanking the whole thing is probably okay.
	 */
	yp_push_deleted();
	if (!do_yank(curbuf, top, bottom, FALSE, cur_yp_name)) {
	    show_error(curwin, "Not enough memory to perform change");
	    return;
	}

	lp = top->p_line;

	nlines = cntllines(lp, bottom->p_line);
	if (nlines > 1) {
	    repllines(curwin, lp->l_next, nlines - 1, (Line *) NULL);
	}

	move_cursor(curwin, lp, 0);

	/*
	 * This is not right; it won't do the right thing when
	 * the cursor is in the whitespace of an indented line.
	 * However, it will do for the moment.
	 */
	begin_line(curwin, TRUE);

	replchars(curwin, lp, curwin->w_cursor->p_index,
						strlen(lp->l_text), "");
	update_buffer(curbuf);
    } else {
	/*
	 * A character-based change really is just a delete and an insert.
	 * So use the deletion code to make things easier.
	 */
	doappend = (mtype == m_incl) && endofline(bottom);

	op_delete(c1, c2, num, top, bottom);

	if (doappend) {
	    (void) one_right(curwin, TRUE);
	}
    }

    Redo.r_mode = r_insert;
    flexclear(&Redo.r_fb);
    if (num != 0) {
	(void) lformat(&Redo.r_fb, "c%ld%c%c", num, c1, c2);
    } else {
	(void) lformat(&Redo.r_fb, "c%c%c", c1, c2);
    }

    startinsert(FALSE);
}

static void
op_yank(top, bottom)
Posn	*top;
Posn	*bottom;
{
    long	nlines;

    /*
     * Report on the number of lines yanked.
     */
    nlines = cntllines(top->p_line, bottom->p_line);

    if (nlines > Pn(P_report)) {
	show_message(curwin, "%ld lines yanked", nlines);
    }

    /*
     * If the target is non-inclusive and character-based,
     * reduce the final position by one.
     */
    if (mtype == m_nonincl) {
	(void) dec(bottom);
    }

    (void) do_yank(curbuf, top, bottom, (mtype != m_line), cur_yp_name);
}

static bool_t
dojoin()
{
    register Posn	*curr_pos;	/* cursor position (abbreviation) */
    register Line	*curr_line;	/* line we started the join on */
    char		*nextline;	/* text of subsequent line */
    int			join_index;	/* index of start of new text section */
    int			size1;		/* size of the first line */
    int			size2;		/* size of the second line */

    curr_pos = curwin->w_cursor;
    curr_line = curr_pos->p_line;

    /*
     * If we are on the last line, we can't join.
     */
    if (curr_line->l_next == curbuf->b_lastline)
	return(FALSE);

    if (!start_command(curwin)) {
	return(FALSE);
    }

    /*
     * Move cursor to end of line, and find out
     * exactly where we will place the new text.
     */
    while (one_right(curwin, FALSE))
	;
    join_index = curr_pos->p_index;
    if (curr_line->l_text[join_index] != '\0')
	join_index += 1;

    /*
     * Copy the text of the next line after the end of the
     * current line, but don't copy any initial whitespace.
     * Then delete the following line.
     */
    nextline = curr_line->l_next->l_text;
    while (*nextline == ' ' || *nextline == '\t')
	nextline++;
    size1 = strlen(curr_line->l_text);
    size2 = strlen(nextline);

    replchars(curwin, curr_line, join_index, 0, nextline);
    repllines(curwin, curr_line->l_next, (long) 1, (Line *) NULL);

    if (size1 != 0 && size2 != 0) {
	/*
	 * If there is no whitespace on this line,
	 * insert a single space.
	 */
	if (gchar(curr_pos) != ' ' && gchar(curr_pos) != '\t') {
	    replchars(curwin, curr_line, curr_pos->p_index + 1, 0, " ");
	}

	/*
	 * Make sure the cursor sits on the right character.
	 */
	(void) one_right(curwin, FALSE);
    }

    end_command(curwin);

    update_buffer(curbuf);

    return(TRUE);
}
