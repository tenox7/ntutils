/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)param.c	2.1 (Chris & John Downey) 7/29/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    param.c
* module function:
    Code to handle user-settable parameters. This is all pretty much
    table-driven. To add a new parameter, put it in the params array,
    and add a macro for it in param.h.

    The idea of the parameter table is that access to any particular
    parameter has to be fast, so it is done with a table lookup. This
    unfortunately means that the index of each parameter is recorded
    as a macro in param.h, so that file must be changed at the same
    time as the table below, and in the same way.

    When a parameter is changed, a function is called to do the actual
    work; this function is part of the parameter structure.  For many
    parameters, it's just a simple function that prints "not implemented";
    for most others, there are "standard" functions to set bool, numeric
    and string parameters, with a certain amount of checking.

    No bounds checking is done here; we should really include limits
    to numeric parameters in the table. Maybe this will come later.

    The data structures will be changed again shortly to enable
    buffer- and window-local parameters to be implemented.

    One problem with numeric parameters is that they are of type "int";
    this obviously places some restrictions on the sort of things they
    may be used for, and it may be necessary at some point to change
    this type to something like "unsigned long".

* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

#define nofunc	PFUNCADDR(NULL)

/*
 * Default settings for string parameters.
 * These are set by the exported function init_params(),
 * which must be called before any parameters are accessed.
 */
#define	DEF_TAGS	"tags,/usr/lib/tags"
#define	DEF_PARA	"^($|\\.([ILPQ]P|LI|[plib]p))"
#define	DEF_SECTIONS	"^({|\\.([NS]H|HU|nh|sh))"
#define	DEF_SENTENCES	"\\<[A-Z]"

/*
 * Default setting for roscolour parameter is the same
 * as the statuscolour if not specified otherwise.
 */
#ifndef	DEF_ROSCOLOUR
#define	DEF_ROSCOLOUR	DEF_STCOLOUR
#endif

/*
 * Default settings for showing control- and meta-characters are
 * as for "normal" vi, i.e. "old" xvi without SHOW_META_CHARS set.
 */
#ifndef	DEF_CCHARS
#   define	DEF_CCHARS	FALSE
#endif
#ifndef	DEF_MCHARS
#   define	DEF_MCHARS	FALSE
#endif

/*
 * Internal functions.
 */
static	int	strtoi P((char **));
static	bool_t	_do_set P((Xviwin *, char *, bool_t));
static	char	*parmstring P((Param *, int));
static	void	enum_usage P((Xviwin*, Param *));
static	bool_t	not_imp P((Xviwin *, Paramval, bool_t));
static	bool_t	set_magic P((Xviwin *, Paramval, bool_t));
static	bool_t	set_rt P((Xviwin *, Paramval, bool_t));
static	char	*par_show P((void));

/*
 * These are the available parameters. The following are non-standard:
 *
 *	autosplit format helpfile jumpscroll preserve
 *	preservetime regextype vbell edit
 *	colour statuscolour roscolour systemcolour
 */
Param	params[] = {
/*  fullname        shortname       flags       value           function ... */
{   "ada",          "ada",          P_BOOL,     0,              not_imp,   },
{   "adapath",      "adapath",      P_STRING,   0,              not_imp,   },
{   "autoindent",   "ai",           P_BOOL,     0,              nofunc,    },
{   "autoprint",    "ap",           P_BOOL,     0,              not_imp,   },
{   "autosplit",    "as",           P_NUM,      2,              nofunc,    },
{   "autowrite",    "aw",           P_BOOL,     0,              not_imp,   },
{   "beautify",     "bf",           P_BOOL,     0,              not_imp,   },
{   "cchars",       "cchars",       P_BOOL,     DEF_CCHARS,     nofunc,    },
{   "colour",       "co",           P_NUM,      DEF_COLOUR,     nofunc,    },
{   "directory",    "dir",          P_STRING,   0,              not_imp,   },
{   "edcompatible", "edcompatible", P_BOOL,     0,              not_imp,   },
{   "edit",         "edit",         P_BOOL,     TRUE,           set_edit,  },
{   "errorbells",   "eb",           P_BOOL,     0,              nofunc,    },
{   "format",       "fmt",          P_ENUM,     0,              set_format,},
{   "hardtabs",     "ht",           P_NUM,      0,              not_imp,   },
{   "helpfile",     "hf",           P_STRING,   0,              nofunc,    },
{   "ignorecase",   "ic",           P_BOOL,     0,              nofunc,    },
{   "jumpscroll",   "js",           P_ENUM,     0,              nofunc,    },
{   "lisp",         "lisp",         P_BOOL,     0,              not_imp,   },
{   "list",         "ls",           P_BOOL,     0,              nofunc,    },
{   "magic",        "magic",        P_BOOL,     TRUE,           set_magic, },
{   "mchars",       "mchars",       P_BOOL,     DEF_MCHARS,     nofunc,    },
{   "mesg",         "mesg",         P_BOOL,     0,              not_imp,   },
{   "minrows",      "min",          P_NUM,      2,              nofunc,    },
{   "modeline",     "modeline",     P_BOOL,     0,              not_imp,   },
{   "number",       "nu",           P_BOOL,     0,              nofunc,    },
{   "open",         "open",         P_BOOL,     0,              not_imp,   },
{   "optimize",     "opt",          P_BOOL,     0,              not_imp,   },
{   "paragraphs",   "para",         P_STRING,   0,              nofunc,    },
{   "preserve",     "psv",          P_ENUM,     0,              nofunc,    },
{   "preservetime", "psvt",         P_NUM,      5,              nofunc,    },
{   "prompt",       "prompt",       P_BOOL,     0,              not_imp,   },
{   "readonly",     "ro",           P_BOOL,     0,              nofunc,	   },
{   "redraw",       "redraw",       P_BOOL,     0,              not_imp,   },
{   "regextype",    "rt",           P_ENUM,     0,              set_rt,    },
{   "remap",        "remap",        P_BOOL,     0,              nofunc,    },
{   "report",       "report",       P_NUM,      5,              nofunc,    },
{   "roscolour",    "rst",          P_NUM,      DEF_ROSCOLOUR,  nofunc,    },
{   "scroll",       "scroll",       P_NUM,      0,              not_imp,   },
{   "sections",     "sections",     P_STRING,   0,              nofunc,    },
{   "sentences",    "sentences",    P_STRING,   0,              nofunc,    },
{   "shell",        "sh",           P_STRING,   0,              nofunc,    },
{   "shiftwidth",   "sw",           P_NUM,      8,              nofunc,    },
{   "showmatch",    "sm",           P_BOOL,     0,              nofunc,    },
{   "slowopen",     "slowopen",     P_BOOL,     0,              not_imp,   },
{   "sourceany",    "sourceany",    P_BOOL,     0,              not_imp,   },
{   "statuscolour", "st",           P_NUM,      DEF_STCOLOUR,   nofunc,    },
{   "systemcolour", "sy",           P_NUM,      DEF_SYSCOLOUR,  nofunc,    },
{   "tabs",         "tabs",         P_BOOL,     TRUE,           nofunc,    },
{   "tabstop",      "ts",           P_NUM,      8,              nofunc,    },
{   "taglength",    "tlh",          P_NUM,      0,              nofunc,    },
{   "tags",         "tags",         P_LIST,     0,              nofunc,    },
{   "term",         "term",         P_STRING,   0,              not_imp,   },
{   "terse",        "terse",        P_BOOL,     0,              not_imp,   },
{   "timeout",      "timeout",      P_NUM,      DEF_TIMEOUT,    nofunc,    },
{   "ttytype",      "ttytype",      P_STRING,   0,              not_imp,   },
{   "vbell",        "vb",           P_BOOL,     0,              nofunc,    },
{   "warn",         "warn",         P_BOOL,     0,              not_imp,   },
{   "window",       "window",       P_NUM,      0,              not_imp,   },
{   "wrapmargin",   "wm",           P_NUM,      0,              nofunc,    },
{   "wrapscan",     "ws",           P_BOOL,     TRUE,           nofunc,    },
{   "writeany",     "wa",           P_BOOL,     0,              not_imp,   },

{   (char *) NULL,  (char *) NULL,  0,          0,              nofunc,    },

};

/*
 * Special initialisations for string, list and enum parameters,
 * which we cannot put in the table above because C does not
 * allow the initialisation of unions.
 */
static struct {
    int		index;
    char	*value;
} init_str[] = {	/* strings and lists */
    P_helpfile,		HELPFILE,
    P_paragraphs,	DEF_PARA,
    P_sections,		DEF_SECTIONS,
    P_sentences,	DEF_SENTENCES,
    P_tags,		DEF_TAGS,
};
#define	NSTRS	(sizeof(init_str) / sizeof(init_str[0]))

/*
 * Names of values for the P_jumpscroll enumerated parameter.
 *
 * It is essential that these are in the same order as the js_...
 * symbolic constants defined in xvi.h.
 */
static char *js_strings[] =
{
    "off",		/* js_OFF */
    "auto",		/* js_AUTO */
    "on",		/* js_ON */
    NULL
};

static struct {
    int		index;
    int		value;
    char	**elist;
} init_enum[] = {	/* enumerations */
    P_format,		DEF_TFF,	fmt_strings,
    P_jumpscroll,	js_AUTO,	js_strings,
    P_preserve,		psv_STANDARD,	psv_strings,
    P_regextype,	rt_GREP,	rt_strings,
};
#define	NENUMS	(sizeof(init_enum) / sizeof(init_enum[0]))

/*
 * These are used by par_show().
 */
static	bool_t	show_all;
static	Param	*curparam;

/*
 * Initialise parameters.
 *
 * This function is called once from startup().
 */
void
init_params()
{
    Paramval	pv;
    Param	*pp;
    int		i;

    /*
     * First go through the special string and enum initialisation
     * tables, setting the values into the union field in the
     * parameter structures.
     */
    for (i = 0; i < NSTRS; i++) {
	set_param(init_str[i].index, init_str[i].value);
    }
    for (i = 0; i < NENUMS; i++) {
	set_param(init_enum[i].index, init_enum[i].value,
			init_enum[i].elist);
    }

    /*
     * Call any special functions that have been set up for
     * any parameters, using the default values included in
     * the parameter table.
     */
    for (pp = &params[0]; pp->p_fullname != NULL; pp++) {
	if (pp->p_func != nofunc && pp->p_func != PFUNCADDR(not_imp)) {
	    switch (pp->p_flags & P_TYPE) {
	    case P_NUM:
	    case P_ENUM:
		pv.pv_i = pp->p_value;
		break;
	    case P_BOOL:
		pv.pv_b = pp->p_value;
		break;
	    case P_STRING:
		pv.pv_s = pp->p_str;
		break;
	    case P_LIST:
		pv.pv_l = pp->p_list;
	    }
	    (void) (*pp->p_func)(curwin, pv, FALSE);
	}
    }
}

void
do_set(window, argc, argv, inter)
Xviwin	*window;
int	argc;		/* number of parameters to set */
char	*argv[];	/* vector of parameter strings */
bool_t	inter;		/* TRUE if called interactively */
{
    int	count;

    /*
     * First check to see if there were any parameters to set,
     * or if the user just wants us to display the parameters.
     */
    if (argc == 0 || (argc == 1 && (argv[0][0] == '\0' ||
				strncmp(argv[0], "all", 3) == 0))) {
	if (inter) {
	    int pcwidth;

	    show_all = (argc != 0 && argv[0][0] != '\0');
	    curparam = &params[0];
	    pcwidth = (window->w_ncols < 90 ?
			window->w_ncols :
			(window->w_ncols < 135 ?
			    window->w_ncols / 2 :
			    window->w_ncols / 3));

	    disp_init(window, par_show, pcwidth, FALSE);
	}

	return;
    }

    for (count = 0; count < argc; count++) {
	if (!_do_set(window, argv[count], inter)) {
	    break;
	}
    }

    if (inter) {

	/*
	 * Finally, update the screen in case we changed
	 * something like "tabstop" or "list" that will change
	 * its appearance. We don't always have to do this,
	 * but it's easier for now.
	 */
	update_all();
    }
}

/*
 * Convert a string to an integer. The string may encode the integer
 * in octal, decimal or hexadecimal form, in the same style as C. On
 * return, make the string pointer, which is passed to us by
 * reference, point to the first character which isn't valid for the
 * base the number seems to be in.
 */
static int
strtoi(sp)
char	**sp;
{
    register char	*s;
    register int	i, c;
    bool_t		neg;

    i = 0;
    neg = FALSE;
    s = *sp;
    c = *s;
    while (is_space(c)) {
	c = *++s;
    }
    if (c == '-') {
	neg = TRUE;
	c = *++s;
    }
    while (is_space(c)) {
	c = *++s;
    }
    if (c == '0') {
	switch (c = *++s) {
	case 'x': case 'X':
	    /*
	     * We've got 0x ... or 0X ..., so it
	     * looks like a hex. number.
	     */
	    while ((c = *++s) != '\0' && is_xdigit(c)) {
		i = (i * 16) + hex_to_bin(c);
	    }
	    break;

	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
	    /*
	     * It looks like an octal number.
	     */
	    do {
		i = (i * 8) + c - '0';
	    } while ((c = *++s) != '\0' && is_octdigit(c));
	    break;

	default:
	    *sp = s;
	    return(0);
	}
    } else {
	/*
	 * Assume it's decimal.
	 */
	while (c != '\0' && is_digit(c)) {
	    i = (i * 10) + c - '0';
	    c = *++s;
	}
    }
    *sp = s;
    return(neg ? -i : i);
}

/*
 * Internal version of do_set(). Returns TRUE if set of specified
 * parameter was successful, otherwise FALSE.
 */
static bool_t
_do_set(window, arg, inter)
Xviwin	*window;
char	*arg;				/* parameter string */
bool_t	inter;				/* TRUE if called interactively */
{
    int		settype = 0;		/* type they want to set */
    bool_t	bool_value;		/* value they want to set */
    char	*str_value;		/* value they want to set */
    Param	*pp;
    Paramval	val;
    char	*cp;
    char	**ep;


    /*
     * "arg" points at the parameter we are going to try to set.
     * Spot "no" and "=" keys.
     */

    /*
     * Note that we will have to change this later if we
     * want to implement "nomatch" and "nonomatch". :-(
     */
    if (strncmp(arg, "no", 2) == 0) {
	settype = P_BOOL;
	bool_value = FALSE;
	arg += 2;
    } else {
	bool_value = TRUE;
    }

    str_value = strchr(arg, '=');
    if (str_value != NULL) {
	if (settype == P_BOOL) {
	    if (inter) {
		show_error(window, "Can't give \"no\" and a value");
		return(FALSE);
	    }
	}

	/*
	 * Null-terminate the parameter name
	 * and point str_value at the value to
	 * the right of the '='.
	 */
	*str_value++ = '\0';
	settype = P_STRING;

    } else if (settype != P_BOOL) {

	/*
	 * Not already set to P_BOOL, so we haven't seen a "no".
	 * No '=' sign, so assume we are setting a boolean
	 * parameter to TRUE.
	 */
	settype = P_BOOL;
	bool_value = TRUE;
    }

    /*
     * Now search for a complete match of the parameter
     * name with either the full or short name in the
     * parameter table.
     */
    for (pp = &params[0]; pp->p_fullname != NULL; pp++) {
	if (strcmp(arg, pp->p_fullname) == 0 ||
	    strcmp(arg, pp->p_shortname) == 0)
	    break;
    }

    if (pp->p_fullname == NULL) {		/* no match found */
	if (inter) {
	    show_error(window, "No such parameter");
	    return(FALSE);
	}
    }

    /*
     * Check the passed type is appropriate for the
     * parameter's type. If it isn't, winge and return.
     */
    switch (pp->p_flags & P_TYPE) {
    case P_STRING:
    case P_LIST:
    case P_NUM:
	if (settype != P_STRING) {
	    if (inter) {
		show_error(window,
		    (pp->p_flags & P_STRING) ?
		    "Invalid set of string parameter"
		    : (pp->p_flags & P_NUM) ?
		    "Invalid set of numeric parameter"
		    : /* else */
		    "Invalid set of list parameter");
	    }
	    return(FALSE);
	}
	break;

    case P_ENUM:
	if (settype != P_STRING) {
	    if (inter) {
		enum_usage(window, pp);
	    }
	    return(FALSE);
	}
	break;

    case P_BOOL:
	if (settype != P_BOOL) {
	    if (inter) {
		show_error(window, "Invalid set of boolean parameter");
	    }
	    return(FALSE);
	}
    }

    /*
     * Do any type-specific checking, and set up the
     * "val" union to contain the (decoded) value.
     */
    switch (pp->p_flags & P_TYPE) {
    case P_NUM:
    {
	int i;

	cp = str_value;
	i = strtoi(&cp);
	/*
	 * If there are extra characters after the number,
	 * don't accept it.
	 */
	if (*cp != '\0') {
	    if (inter) {
		show_error(window, "Invalid numeric parameter");
	    }
	    return(FALSE);
	}

	val.pv_i = i;
	break;
    }

    case P_ENUM:
	for (ep = pp->p_elist; *ep != NULL; ep++) {
	    if (strcmp(*ep, str_value) == 0)
		break;
	}

	if (*ep == NULL) {
	    if (inter) {
		enum_usage(window, pp);
	    }
	    return(FALSE);
	}

	val.pv_i = ep - pp->p_elist;
	break;

    case P_STRING:
    case P_LIST:
	val.pv_s = str_value;
	break;

    case P_BOOL:
	val.pv_b = bool_value;
	break;
    }

    /*
     * Call the check function if there is one.
     */
    if (pp->p_func != nofunc && (*pp->p_func)(window, val, inter) == FALSE) {
	return(FALSE);
    }

    /*
     * Set the value.
     */
    switch (pp->p_flags & P_TYPE) {
    case P_NUM:
    case P_ENUM:
	pp->p_value = val.pv_i;
	break;

    case P_BOOL:
	pp->p_value = bool_value;
	break;

    case P_STRING:
    case P_LIST:
	set_param(pp - params, str_value);
	break;
    }
    pp->p_flags |= P_CHANGED;

    /*
     * Check the bounds for numeric parameters here.
     * We will have to make this table-driven later.
     */
    if (Pn(P_tabstop) <= 0 || Pn(P_tabstop) > MAX_TABSTOP) {
	if (inter) {
	    show_error(window, "Invalid tab size specified");
	}
	set_param(P_tabstop, 8);
	return(FALSE);
    }

    /*
     * Got through all the bounds checking, so we're okay.
     */
    return TRUE;
}

/*
 * Internal parameter setting - parameters are not considered
 * to have been changed unless they are set by the user.
 *
 * All types of parameter are handled by this function.
 * Enumerated types are special, because two arguments
 * are expected: the first, always present, is the index,
 * but the second may be either a valid list of strings
 * a NULL pointer, the latter indicating that the list
 * of strings is not to be changed.
 *
 * The P_LIST type accepts a string argument and converts
 * it to a vector of strings which is then stored.
 */
/*VARARGS1*/
void
#ifdef	__STDC__
    set_param(int n, ...)
#else
    set_param(n, va_alist)
    int		n;
    va_dcl
#endif
{
    va_list		argp;
    Param		*pp;
    char		*cp;
    char		**elist;

    pp = &params[n];

    VA_START(argp, n);

    switch (pp->p_flags & P_TYPE) {
    case P_ENUM:
	pp->p_value = va_arg(argp, int);

	/*
	 * If the second argument is a non-NULL list of
	 * strings, set it into the parameter structure.
	 * Note that this is not dependent on the return
	 * value from the check function being TRUE,
	 * since the check cannot be made until the
	 * array of strings is in place.
	 */
	elist = va_arg(argp, char **);
	if (elist != NULL) {
	    pp->p_elist = elist;
	}
	break;

    case P_BOOL:
    case P_NUM:
	pp->p_value = va_arg(argp, int);
	break;

    case P_LIST:
	{
	    int	argc;
	    char	**argv;

	    cp = strsave(va_arg(argp, char *));
	    if (cp == NULL) {
		/*
		 * This is not necessarily a good idea.
		 */
		show_error(curwin, "Can't allocate memory for parameter");
		return;
	    }

	    makeargv(cp, &argc, &argv, " \t,");
	    if (argc == 0 || argv == NULL) {
		free(cp);
		return;
	    }
	    if (pp->p_list != NULL) {
		if (pp->p_list[0] != NULL) {
		    free(pp->p_list[0]);
		}
		free((char *) pp->p_list);
	    }
	    pp->p_list = argv;
	}
	break;

    case P_STRING:
	cp = strsave(va_arg(argp, char *));
	if (cp == NULL) {
	    /*
	     * This is not necessarily a good idea.
	     */
	    show_error(curwin, "Can't allocate memory for parameter");
	} else {
	    /*
	     * We always free up the old string, because
	     * it must have been allocated at some point.
	     */
	    if (pp->p_str != NULL) {
		free(pp->p_str);
	    }
	    pp->p_str = cp;
	}
	break;
    }

    va_end(argp);
}

/*
 * Display helpful usage message for an enumerated parameter, listing
 * the legal values for it.
 */
static void
enum_usage(w, pp)
    Xviwin		*w;
    Param		*pp;
{
    Flexbuf		s;
    char		**sp;

    flexnew(&s);
    (void) lformat(&s, "Must be one of:");
    for (sp = (char **) pp->p_str; *sp != NULL; sp++) {
	(void) lformat(&s, " %s", *sp);
    }
    show_error(w, "%s", flexgetstr(&s));
    flexdelete(&s);
}

/*
 * Return a string representation for a single parameter.
 */
static char *
parmstring(pp, leading)
Param	*pp;			/* parameter */
int	leading;		/* number of leading spaces in string */
{
    static Flexbuf	b;

    flexclear(&b);
    while (leading-- > 0) {
	(void) flexaddch(&b, ' ');
    }
    switch (pp->p_flags & P_TYPE) {
    case P_BOOL:
	(void) lformat(&b, "%s%s", (pp->p_value ? "" : "no"), pp->p_fullname);
	break;

    case P_NUM:
	(void) lformat(&b, "%s=%d", pp->p_fullname, pp->p_value);
	break;

    case P_ENUM:
    {
	int	n;
	char	*estr;

	for (n = 0; ; n++) {
	    if ((estr = ((char **) pp->p_str)[n]) == NULL) {
		estr = "INTERNAL ERROR";
		break;
	    }
	    if (n == pp->p_value)
		break;
	}
	(void) lformat(&b, "%s=%s", pp->p_fullname, estr);
	break;
    }

    case P_STRING:
	(void) lformat(&b, "%s=%s", pp->p_fullname,
				    (pp->p_str != NULL) ? pp->p_str : "");
	break;

    case P_LIST:
	{
	    register char	**cpp;

	    (void) lformat(&b, "%s=%s", pp->p_fullname, pp->p_list[0]);
	    for (cpp = pp->p_list + 1; *cpp != NULL; cpp++) {
		(void) lformat(&b, " %s", *cpp);
	    }
	}
	break;
    }
    return(flexgetstr(&b));
}

static char *
par_show()
{
    static bool_t	started = FALSE;

    if (!started) {
	started = TRUE;
	return("Parameters:");
    }

    for ( ; curparam->p_fullname != NULL; curparam++) {

	/*
	 * Ignore unimplemented parameters.
	 */
	if (curparam->p_func != PFUNCADDR(not_imp)) {
	    /*
	     * Display all parameters if show_all is set;
	     * otherwise, just display changed parameters.
	     */
	    if (show_all || (curparam->p_flags & P_CHANGED) != 0) {
		break;
	    }
	}
    }

    if (curparam->p_fullname == NULL) {
	started = FALSE;		/* reset for next time */
	return(NULL);
    } else {
	char	*retval;

	retval = parmstring(curparam, 3);
	curparam++;
	return(retval);
    }
}

/*ARGSUSED*/
static bool_t
not_imp(window, new_value, interactive)
Xviwin		*window;
Paramval	new_value;
bool_t		interactive;
{
    if (interactive) {
	show_message(window, "That parameter is not implemented!");
    }
    return(TRUE);
}

/*ARGSUSED*/
static bool_t
set_magic(window, new_value, interactive)
Xviwin			*window;
Paramval		new_value;
bool_t			interactive;
{
    static int	prev_rt = rt_GREP;

    if (new_value.pv_b) {
	/*
	 * Turn magic on.
	 */
	if (Pn(P_regextype) == rt_TAGS) {
	    set_param(P_regextype, prev_rt, (char **) NULL);
	}
    } else {
	/*
	 * Turn magic off.
	 */
	if (Pn(P_regextype) != rt_TAGS) {
	    prev_rt = Pn(P_regextype);
	    set_param(P_regextype, rt_TAGS, (char **) NULL);
	}
    }
    return(TRUE);
}

/*ARGSUSED*/
static bool_t
set_rt(window, new_value, interactive)
Xviwin		*window;
Paramval	new_value;
bool_t		interactive;
{
    switch (new_value.pv_i) {
    case rt_TAGS:
    case rt_GREP:
    case rt_EGREP:
	set_param(P_magic, (new_value.pv_i != rt_TAGS));
	return(TRUE);
    default:
	return(FALSE);
    }
}
