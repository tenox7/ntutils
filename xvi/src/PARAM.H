/* Copyright (c) 1990,1991,1992 Chris and John Downey */
/***

* @(#)param.h	2.1 (Chris & John Downey) 7/29/92

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    param.h
* module function:
    Definitions for parameter access.

    This is all going to change when we have local parameters.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

/*
 * Settable parameters.
 */

typedef union {
    int		pv_i;
    bool_t	pv_b;
    char	*pv_s;
    char	**pv_l;
} Paramval;

typedef	struct	param {
    char	*p_fullname;	/* full parameter name */
    char	*p_shortname;	/* permissible abbreviation */
    int		p_flags;	/* type of parameter, and whether implemented */

    /*
     * This field is used for numeric and boolean values,
     * and for storing the numeric representation for enums.
     */
    int		p_value;

    /*
     * Special function to set parameter.
     * This is called whenever the parameter is set by the user.
     */
    bool_t	(*p_func) P((Xviwin *, Paramval, bool_t));

    /*
     * This field is used for strings, lists and enums.
     * Note that making this the last field allows us to initialise the
     * parameter table, since we can just leave this field uninitialised.
     */
    union {
	char	*pu_str;
	char	**pu_list;
    } pu;
} Param;

#define	p_num	p_value
#define	p_bool	p_value
#define	p_eval	p_value

#define	p_str	pu.pu_str
#define	p_list	pu.pu_list
#define	p_elist	pu.pu_list

#ifdef __ZTC__
#   define	PFUNCADDR(f)	((bool_t (*)(Xviwin *, Paramval, bool_t))(f))
#else
#   define	PFUNCADDR(f)	(f)
#endif

extern	Param	params[];

/*
 * Flags
 */
#define	P_BOOL		0x01	/* the parameter is boolean */
#define	P_NUM		0x02	/* the parameter is numeric */
#define	P_STRING	0x04	/* the parameter is a string */
#define	P_ENUM		0x08	/* the parameter is an enumerated type */
#define	P_LIST		0x20	/* the parameter is a list of strings */

#define	P_TYPE		0x2f	/* used to mask out type from other bits */

#define	P_CHANGED	0x10	/* the parameter has been changed */

/*
 * Macros to set/get the value of a parameter.
 * One each for the four parameter types.
 * May change implementation later to do more checking.
 */
#define	Pn(n)		(params[n].p_num)
#define	Pb(n)		(params[n].p_bool)
#define	Ps(n)		(params[n].p_str)
#define	Pl(n)		(params[n].p_list)
#define	Pen(n)		(params[n].p_value)
#define	Pes(n)		(params[n].p_elist[params[n].p_eval])

#define P_ischanged(n)	(params[n].p_flags & P_CHANGED)
#define P_setchanged(n)	(params[n].p_flags |= P_CHANGED)

/*
 * The following are the indices in the params array for each parameter.
 * They must not be changed without also changing the table in "param.c".
 */
#define P_ada		0
#define P_adapath 	1
#define P_autoindent 	2
#define P_autoprint 	3
#define P_autosplit 	4
#define P_autowrite 	5
#define P_beautify 	6
#define P_cchars 	7
#define P_colour 	8
#define P_directory 	9
#define P_edcompatible 	10
#define P_edit 		11
#define P_errorbells 	12
#define P_format 	13
#define P_hardtabs 	14
#define P_helpfile 	15
#define P_ignorecase 	16
#define P_jumpscroll 	17
#define P_lisp 		18
#define P_list 		19
#define P_magic 	20
#define P_mchars 	21
#define P_mesg 		22
#define P_minrows 	23
#define P_modeline 	24
#define P_number 	25
#define P_open 		26
#define P_optimize 	27
#define P_paragraphs 	28
#define P_preserve 	29
#define P_preservetime 	30
#define P_prompt 	31
#define P_readonly 	32
#define P_redraw 	33
#define P_regextype 	34
#define P_remap 	35
#define P_report 	36
#define P_roscolour 	37
#define P_scroll 	38
#define P_sections 	39
#define P_sentences 	40
#define P_shell 	41
#define P_shiftwidth 	42
#define P_showmatch 	43
#define P_slowopen 	44
#define P_sourceany 	45
#define P_statuscolour 	46
#define P_systemcolour 	47
#define P_tabs	 	48
#define P_tabstop 	49
#define P_taglength 	50
#define P_tags 		51
#define P_term 		52
#define P_terse 	53
#define P_timeout 	54
#define P_ttytype 	55
#define P_vbell 	56
#define P_warn 		57
#define P_window 	58
#define P_wrapmargin 	59
#define P_wrapscan 	60
#define P_writeany 	61
