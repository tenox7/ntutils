/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)search.c	2.1 (Chris & John Downey) 7/29/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    search.c
* module function:
    Regular expression searching, including global command.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"
#include "regexp.h"	/* Henry Spencer's regular expression routines */
#include "regmagic.h"	/* Henry Spencer's regular expression routines */

#ifdef	MEGAMAX
overlay "search"
#endif

/*
 * String searches
 *
 * The actual searches are done using Henry Spencer's regular expression
 * library.
 */

/*
 * Names of values for the P_regextype enumerated parameter.
 */
char	*rt_strings[] =
{
    "tags",
    "grep",
    "egrep",
    NULL
};

/*
 * Used by g/re/p to remember where we are and what we are doing.
 */
static	Line	*curline;
static	Line	*lastline;
static	long	curnum;
static	bool_t	greptype;

static	Posn	*bcksearch P((Xviwin *, Line *, int, bool_t));
static	Posn	*fwdsearch P((Xviwin *, Line *, int, bool_t));
static	char	*mapstring P((char **, int));
static	char	*compile P((char *, int, bool_t));
static	char	*grep_line P((void));
static	long	substitute P((Xviwin *, Line *, Line *, char *, char *));

/*
 * Convert a regular expression to egrep syntax: the source string can
 * be either tags compatible (only ^ and $ are significant), vi
 * compatible or egrep compatible (but also using \< and \>)
 *
 * Our first parameter here is the address of a pointer, which we
 * point to the closing delimiter character if we found one, otherwise
 * the closing '\0'.
 */
static char *
mapstring(sp, delim)
char	**sp;		/* pointer to pointer to pattern string */
int	delim;		/* delimiter character */
{
    static Flexbuf	ns;
    int			rxtype;	/* can be rt_TAGS, rt_GREP or rt_EGREP */
    register enum {
	m_normal,	/* nothing special */
	m_startccl,	/* just after [ */
	m_negccl,	/* just after [^ */
	m_ccl,		/* between [... or [^... and ] */
	m_escape	/* just after \ */
    }	state = m_normal;
    register char	*s;

    rxtype = Pn(P_regextype);

    flexclear(&ns);
    for (s = *sp; *s != '\0' && (*s != delim || state != m_normal); s++) {
	switch (state) {
	case m_normal:
	    switch (*s) {
	    case '\\':
		state = m_escape;
		break;

	    case '(': case ')': case '+': case '?': case '|':
		/* egrep metacharacters */
		if (rxtype != rt_EGREP)
		    (void) flexaddch(&ns, '\\');
		(void) flexaddch(&ns, *s);
		break;

	    case '*': case '.': case '[':
		/* grep metacharacters */
		if (rxtype == rt_TAGS) {
		    (void) flexaddch(&ns, '\\');
		} else if (*s == '[') {
		    /* start of character class */
		    state = m_startccl;
		}
		 /* fall through ... */

	    default:
		(void) flexaddch(&ns, *s);
	    }
	    break;

	case m_startccl:
	case m_negccl:
	    (void) flexaddch(&ns, *s);
	    state = (*s == '^' && state == m_startccl) ? m_negccl : m_ccl;
	    break;

	case m_ccl:
	    (void) flexaddch(&ns, *s);
	    if (*s == ']')
		state = m_normal;
	    break;

	case m_escape:
	    switch (*s) {
	    case '(':		/* bracket conversion */
	    case ')':
		if (rxtype != rt_GREP)
		    (void) flexaddch(&ns, '\\');
		(void) flexaddch(&ns, *s);
		break;

	    case '.':		/* egrep metacharacters */
	    case '\\':
	    case '[':
	    case '*':
	    case '?':
	    case '+':
	    case '^':
	    case '$':
	    case '|':
		(void) lformat(&ns, "\\%c", *s);
		break;

	    default:		/* a normal character */
		if (*s != delim)
		    (void) flexaddch(&ns, '\\');
		(void) flexaddch(&ns, *s);
	    }
	    state = m_normal;
	}
    }

    *sp = s;

    /*
     * This is horrible, but the real vi does it, so ...
     */
    if (state == m_escape) {
	(void) lformat(&ns, "\\\\");
    }
    return flexgetstr(&ns);
}

/**********************************************************
 *							  *
 * Abstract type definition.				  *
 *							  *
 * Regular expression node, with pointer reference count. *
 *							  *
 * We need this for global substitute commands.		  *
 *							  *
 **********************************************************/

typedef struct {
    regexp	*rn_ptr;
    int		rn_count;
} Rnode;

/*
 * Node for last successfully compiled regular expression.
 */
static Rnode	*lastprogp = NULL;

/*
 * Last regular expression used in a substitution.
 */
static	Rnode	*last_lhs = NULL;

/*
 * Last rhs for a substitution.
 */
static	char	*last_rhs = NULL;

/*
 * rn_new(), rn_delete() & rn_duplicate() perform operations on Rnodes
 * which are respectively analogous to open(), close() & dup() for
 * Unix file descriptors.
 */

/*
 * Make a new Rnode, given a pattern string.
 */
static Rnode *
rn_new(str)
    char	*str;
{
    Rnode	*retp;

    if ((retp = (Rnode *) alloc(sizeof (Rnode))) == NULL)
	return NULL;
    if ((retp->rn_ptr = regcomp(str)) == NULL) {
	free ((char *) retp);
	return NULL;
    }
    retp->rn_count = 1;
    return retp;
}

/*
 * Make a copy of an Rnode pointer & increment the Rnode's reference
 * count.
 */
#define rn_duplicate(s)	((s) ? ((s)->rn_count++, (s)) : NULL)

/*
 * Decrement an Rnode's reference count, freeing it if there are no
 * more pointers pointing to it.
 *
 * In C++, this would be a destructor for an Rnode.
 */
static void
rn_delete(rp)
Rnode	*rp;
{
    if (rp != NULL && --rp->rn_count <= 0) {
	free((char *) rp->rn_ptr);
	free((char *) rp);
    }
}

#if 0
/*
 * Increment the reference count for the current prog,
 * and return it to the caller.
 */
static Rnode *
inccount()
{
    if (lastprogp != NULL) {
	lastprogp->rn_count++;
    }
    return(lastprogp);
}

#endif

#define	cur_prog()	(lastprogp->rn_ptr)

/*
 * Compile given regular expression from string.
 *
 * The opening delimiter for the regular expression is supplied; the
 * end of it is marked by an unescaped matching delimiter or, if
 * delim_only is FALSE, by a '\0' character. We return a pointer to
 * the terminating '\0' or to the character following the closing
 * delimiter, or NULL if we failed.
 *
 * If, after we've found a delimiter, we have an empty pattern string,
 * we use the last compiled expression if there is one.
 *
 * The regular expression is converted to egrep syntax by mapstring(),
 * which also finds the closing delimiter. The actual compilation is
 * done by regcomp(), from Henry Spencer's regexp routines.
 *
 * If we're successful, the compiled regular expression will be
 * pointed to by lastprogp->rn_ptr, & lastprogp->rn_count will be > 0.
 */
static char *
compile(pat, delimiter, delim_only)
char	*pat;
int	delimiter;
bool_t	delim_only;
{
    Rnode	*progp;

    if (pat == NULL) {
	return(NULL);
    }

    /*
     * If we get an empty regular expression, we just use the last
     * one we compiled (if there was one).
     */
    if (*pat == '\0') {
	return((delim_only || lastprogp == NULL) ? NULL : pat);
    }
    if (*pat == delimiter) {
	return((lastprogp == NULL) ? NULL : &pat[1]);
    }

    progp = rn_new(mapstring(&pat, delimiter));
    if (progp == NULL) {
	return(NULL);
    }

    if (*pat == '\0') {
	if (delim_only) {
	    rn_delete(progp);
	    return(NULL);
	}
    } else {
	pat++;
    }
    rn_delete(lastprogp);
    lastprogp = progp;
    return(pat);
}

Posn *
search(window, startline, startindex, dir, strp)
Xviwin		*window;
Line		*startline;
int		startindex;
int		dir;		/* FORWARD or BACKWARD */
char		**strp;
{
    Posn	*pos;
    Posn	*(*sfunc) P((Xviwin *, Line *, int, bool_t));
    char	*str;

    str = compile(*strp, (dir == FORWARD) ? '/' : '?', FALSE);
    if (str == NULL) {
	return(NULL);
    }
    *strp = str;

    if (dir == BACKWARD) {
	sfunc = bcksearch;
    } else {
	sfunc = fwdsearch;
    }
    pos = (*sfunc)(window, startline, startindex, Pb(P_wrapscan));

    return(pos);
}

/*
 * Search for the given expression, ignoring regextype, without
 * wrapscan & and without using the compiled regular expression for
 * anything else (so 'n', 'N', etc., aren't affected). We do, however,
 * cache the compiled form for the last string we were given.
 */
Posn *
nsearch(window, startline, startindex, dir, str)
Xviwin		*window;
Line		*startline;
int		startindex;
int		dir;
char		*str;
{
    static Rnode	*progp = NULL;
    static char		*last_str = NULL;
    Rnode		*old_progp;
    Posn		*pos;
    Posn		*(*sfunc) P((Xviwin *, Line *, int, bool_t));

    if (str == NULL) {
	return(NULL);
    }
    if (str != last_str &&
	(last_str == NULL || strcmp(str, last_str) != 0)) {
	if (progp) {
	    rn_delete(progp);
	}
	progp = rn_new(str);
	last_str = str;
    }
    if (progp == NULL) {
	last_str = NULL;
	return(NULL);
    }

    if (dir == BACKWARD) {
	sfunc = bcksearch;
    } else {
	sfunc = fwdsearch;
    }

    old_progp = lastprogp;

    lastprogp = progp;
    pos = (*sfunc)(window, startline, startindex, FALSE);

    lastprogp = old_progp;

    return(pos);
}

/*
 * Perform line-based search, returning a pointer to the first line
 * (forwards or backwards) on which a match is found, or NULL if there
 * is none in the buffer specified.
 */
Line *
linesearch(window, dir, strp)
Xviwin	*window;
int	dir;
char	**strp;
{
    Posn	pos;
    Posn	*newpos;

    pos = *(window->w_cursor);
    if (dir == FORWARD) {
	/*
	 * We don't want a match to occur on the current line,
	 * but setting the starting position to the next line
	 * is wrong because we will not match a pattern at the
	 * start of the line. So go to the end of this line.
	 */
	if (gchar(&pos) != '\0') {
	    while (inc(&pos) == mv_SAMELINE) {
		;
	    }
	}
    } else {
	pos.p_index = 0;
    }

    newpos = search(window, pos.p_line, pos.p_index, dir, strp);
    return((newpos != NULL) ? newpos->p_line : NULL);
}

/*
 * regerror - called by regexp routines when errors are detected.
 */
void
regerror(s)
char	*s;
{
    if (echo & e_REGERR) {
	show_error(curwin, "%s", s);
    }
    echo &= ~(e_REGERR | e_NOMATCH);
}

/*
 * Find a match at or after "ind" on the given "line"; return
 * pointer to Posn of match, or NULL if no match was found.
 */
static Posn *
match(line, ind)
Line	*line;
int	ind;
{
    static Posn	matchposn;
    char	*s;
    regexp	*prog;

    s = line->l_text + ind;
    prog = cur_prog();

    if (regexec(prog, s, (ind == 0))) {
	matchposn.p_line = line;
	matchposn.p_index = (int) (prog->startp[0] - line->l_text);

	/*
	 * If the match is after the end of the line,
	 * move it to the last character of the line,
	 * unless the line has no characters at all.
	 */
	if (line->l_text[matchposn.p_index] == '\0' &&
					matchposn.p_index > 0) {
	    matchposn.p_index -= 1;
	}

	return(&matchposn);
    } else {
	return(NULL);
    }
}

/*
 * Like match(), but returns the last available match on the given
 * line which is before the index given in maxindex.
 */
static Posn *
rmatch(line, ind, maxindex)
Line		*line;
register int	ind;
int		maxindex;
{
    register int	lastindex = -1;
    Posn		*pos;
    register char	*ltp;

    ltp = line->l_text;
    for (; (pos = match(line, ind)) != NULL; ind++) {
	ind = pos->p_index;
	if (ind >= maxindex)
	    break;
	/*
	 * If we've found a match on the last
	 * character of the line, return it here or
	 * we could get into an infinite loop.
	 */
	if (ltp[lastindex = ind] == '\0' || ltp[ind + 1] == '\0')
	    break;
    }

    if (lastindex >= 0) {
	static Posn	lastmatch;

	lastmatch.p_index = lastindex;
	lastmatch.p_line = line;
	return &lastmatch;
    } else {
	return NULL;
    }
}

/*
 * Search forwards through the buffer for a match of the last
 * pattern compiled.
 */
static Posn *
fwdsearch(window, startline, startindex, wrapscan)
Xviwin		*window;
Line		*startline;
int		startindex;
bool_t		wrapscan;
{
    static Posn	*pos;		/* location of found string */
    Line	*lp;		/* current line */
    Line	*last;

    last = window->w_buffer->b_lastline;

    /*
     * First, search for a match on the current line
     * after the cursor position.
     */
    pos = match(startline, startindex + 1);
    if (pos != NULL) {
	return(pos);
    }

    /*
     * Now search all the lines from here to the end of the file,
     * and from the start of the file back to here if (wrapscan).
     */
    for (lp = startline->l_next; lp != startline; lp = lp->l_next) {
	/*
	 * Wrap around to the start of the file.
	 */
	if (lp == last) {
	    if (wrapscan) {
		lp = window->w_buffer->b_line0;
		continue;
	    }
	     /* else */
		return(NULL);
	}

	pos = match(lp, 0);
	if (pos != NULL) {
	    return(pos);
	}
    }

    /*
     * Finally, search from the start of the cursor line
     * up to the cursor position. (Wrapscan was set if
     * we got here.)
     */
    pos = match(startline, 0);
    if (pos != NULL) {
	if (pos->p_index <= startindex) {
	    return(pos);
	}
    }

    return(NULL);
}

/*
 * Search backwards through the buffer for a match of the last
 * pattern compiled.
 *
 * Because we're searching backwards, we have to return the
 * last match on a line if there is more than one, so we call
 * rmatch() instead of match().
 */
static Posn *
bcksearch(window, startline, startindex, wrapscan)
Xviwin		*window;
Line		*startline;
int		startindex;
bool_t		wrapscan;
{
    Posn	*pos;		/* location of found string */
    Line	*lp;		/* current line */
    Line	*line0;

    /*
     * First, search for a match on the current line before the
     * current cursor position; if "begword" is set, it must be
     * before the current cursor position minus one.
     */
    pos = rmatch(startline, 0, startindex);
    if (pos != NULL) {
	return(pos);
    }

    /*
     * Search all lines back to the start of the buffer,
     * and then from the end of the buffer back to the
     * line after the cursor line if wrapscan is set.
     */
    line0 = window->w_buffer->b_line0;
    for (lp = startline->l_prev; lp != startline; lp = lp->l_prev) {

	if (lp == line0) {
	    if (wrapscan) {
		/*
		 * Note we do a continue here so that
		 * the loop control works properly.
		 */
		lp = window->w_buffer->b_lastline;
		continue;
	    } else {
		return(NULL);
	    }
	}
	pos = rmatch(lp, 0, INT_MAX);
	if (pos != NULL)
	    return pos;
    }

    /*
     * Finally, try for a match on the cursor line
     * after (or at) the cursor position.
     */
    pos = rmatch(startline, startindex, INT_MAX);
    if (pos != NULL) {
	return(pos);
    }

    return(NULL);
}

/*
 * Execute a global command of the form:
 *
 * g/pattern/X
 *
 * where 'x' is a command character, currently one of the following:
 *
 * d	Delete all matching lines
 * l	List all matching lines
 * p	Print all matching lines
 * s	Perform substitution
 * &	Repeat last substitution
 * ~	Apply last right-hand side used in a substitution to last
 *	regular expression used
 *
 * The command character (as well as the trailing slash) is optional, and
 * is assumed to be 'p' if missing.
 *
 * The "lp" and "up" parameters are the first line to be considered, and
 * the last line to be considered. If these are NULL, the whole buffer is
 * considered; if only up is NULL, we consider the single line "lp".
 *
 * The "matchtype" parameter says whether we are doing 'g' or 'v'.
 */
void
do_global(window, lp, up, cmd, matchtype)
Xviwin		*window;
Line		*lp, *up;
char		*cmd;
bool_t		matchtype;
{
    Rnode		*globprogp;
    regexp		*prog;		/* compiled pattern */
    long		ndone;		/* number of matches */
    register char	cmdchar = '\0';	/* what to do with matching lines */

    /*
     * compile() compiles the pattern up to the first unescaped
     * delimiter: we place the character after the delimiter in
     * cmdchar. If there is no such character, we default to 'p'.
     */
    if (*cmd == '\0' || (cmd = compile(&cmd[1], *cmd, FALSE)) == NULL) {
	regerror(matchtype ?
		"Usage: :g/search pattern/command" :
		"Usage: :v/search pattern/command");
	return;
    }
    /*
     * Check we can do the command before starting.
     */
    switch (cmdchar = *cmd) {
    case '\0':
	cmdchar = 'p';
	 /* fall through ... */
    case 'l':
    case 'p':
	break;
    case 's':
    case '&':
    case '~':
	cmd++;	/* cmd points at char after modifier */
	 /* fall through ... */
    case 'd':
	if (!start_command(window)) {
	    return;
	}
	break;
    default:
	regerror("Invalid command character");
	return;
    }

    ndone = 0;

    /*
     * If no range was given, do every line.
     * If only one line was given, just do that one.
     * Ensure that "up" points at the line after the
     * last one in the range, to make the loop easier.
     */
    if (lp == NULL) {
	lp = window->w_buffer->b_file;
	up = window->w_buffer->b_lastline;
    } else if (up == NULL) {
	up = lp->l_next;
    } else {
	up = up->l_next;
    }

    /*
     * If we are going to print lines, it is sensible
     * to find out the line number of the first line in
     * the range before we start, and increment it as
     * we go rather than finding out the line number
     * for each line as it is printed.
     */
    switch (cmdchar) {
    case 'p':
    case 'l':
	curnum = lineno(window->w_buffer, lp);
	curline = lp;
	lastline = up;
	greptype = matchtype;
	disp_init(window, grep_line, (int) Columns,
	      (cmdchar == 'l'));
	return;
    }

    /*
     * This is tricky. do_substitute() might default to
     * using cur_prog(), if the command is of the form
     *
     *	:g/pattern/s//.../
     *
     * so cur_prog() must still reference the expression we
     * compiled. On the other hand, it may compile a
     * different regular expression, so we have to be able
     * to maintain a separate one (which is what globprogp
     * is for). Moreover, if it does compile a different
     * expression, one of them has to be freed afterwards.
     *
     * This is why we use Rnodes, which contain
     * reference counts. An Rnode, & the compiled
     * expression it points to, are only freed when its
     * reference count is decremented to 0.
     */
    globprogp = rn_duplicate(lastprogp);
    prog = cur_prog();

    /*
     * Place the cursor at bottom left of the window,
     * so the user knows what we are doing.
     * It is safe not to put the cursor back, because
     * we are going to produce some more output anyway.
     */
    gotocmd(window, FALSE);
    flush_output();

    /*
     * Try every line from lp up to (but not including) up.
     */
    ndone = 0;
    while (lp != up) {
	if (matchtype == regexec(prog, lp->l_text, TRUE)) {
	    Line	*thisline;

	    /*
	     * Move the cursor to the line before
	     * doing anything. Also move the line
	     * pointer on one before calling any
	     * functions which might alter or delete
	     * the line.
	     */
	    move_cursor(window, lp, 0);

	    thisline = lp;
	    lp = lp->l_next;

	    switch (cmdchar) {
	    case 'd':	/* delete the line */
		repllines(window, thisline, 1L,
			(Line *) NULL);
		ndone++;
		break;
	    case 's':	/* perform substitution */
	    case '&':
	    case '~':
	    {
		register long	(*func) P((Xviwin *, Line *, Line *, char *));
		unsigned	savecho;

		switch (cmdchar) {
		case 's':
		    func = do_substitute;
		    break;
		case '&':
		    func = do_ampersand;
		    break;
		case '~':
		    func = do_tilde;
		}

		savecho = echo;

		echo &= ~e_NOMATCH;
		ndone += (*func)
		(window, thisline, thisline, cmd);

		echo = savecho;
		break;
	    }
	    }
	} else {
	    lp = lp->l_next;
	}
    }

    /*
     * If globprogp is still the current prog, this should just
     * decrement its reference count to 1: otherwise, if
     * do_substitute() has compiled a different pattern, then that
     * counts as the last compiled pattern, globprogp's reference
     * count should be decremented to 0, & it should be freed.
     */
    rn_delete(globprogp);

    switch (cmdchar) {
    case 'd':
    case 's':
    case '&':
    case '~':
	end_command(window);
	if (ndone) {
	    update_buffer(window->w_buffer);
	    cursupdate(window);
	    begin_line(window, TRUE);
	    if (ndone >= Pn(P_report)) {
		show_message(window,
			 (cmdchar == 'd') ?
			 "%ld fewer line%c" :
			 "%ld substitution%c",
			 ndone,
			 (ndone > 1) ?
			 's' : ' ');
	    }
	}
    }

    if (ndone == 0 && (echo & e_NOMATCH)) {
	regerror("No match");
    }
}

static char *
grep_line()
{
    static Flexbuf	b;
    regexp		*prog;

    prog = cur_prog();
    for ( ; curline != lastline; curline = curline->l_next, curnum++) {

	if (greptype == regexec(prog, curline->l_text, TRUE)) {

	    flexclear(&b);
	    if (Pb(P_number)) {
		(void) lformat(&b, NUM_FMT, curnum);
	    }
	    (void) lformat(&b, "%s", curline->l_text);
	    break;
	}
    }

    if (curline == lastline) {
	return(NULL);
    } else {
	curline = curline->l_next;
	curnum++;
	return(flexgetstr(&b));
    }
}

/*
 * regsubst - perform substitutions after a regexp match
 *
 * Adapted from a routine from Henry Spencer's regexp package. The
 * original copyright notice for all these routines is in regexp.c,
 * which is distributed herewith.
 */

#ifndef CHARBITS
#	define	UCHARAT(p)	((int)*(unsigned char *)(p))
#else
#	define	UCHARAT(p)	((int)*(p)&CHARBITS)
#endif

static void
regsubst(prog, src, dest)
register regexp	*prog;
register char	*src;
Flexbuf		*dest;
{
    register int	c;

    if (prog == NULL || src == NULL || dest == NULL) {
	regerror("NULL parameter to regsubst");
	return;
    }

    if (UCHARAT(prog->program) != MAGIC) {
	regerror("Damaged regexp fed to regsubst");
	return;
    }

    while ((c = *src++) != '\0') {
	register int no;

	/*
	 * First check for metacharacters.
	 */
	if (c == '&') {
	    no = 0;
	} else if (c == '\\' && '0' <= *src && *src <= '9') {
	    no = *src++ - '0';
	} else {
	    no = -1;
	}

	if (no < 0) {
	    /*
	     * It's an ordinary character.
	     */
	    if (c == '\\' && *src != '\0')
		c = *src++;

	    (void) flexaddch(dest, c);

	} else if (prog->startp[no] != NULL && prog->endp[no] != NULL) {
	    register char *bracketp;

	    /*
	     * It isn't an ordinary character, but a reference
	     * to a string matched on the lhs. Notice that we
	     * just do nothing if we find a reference to a null
	     * match, or one that doesn't exist; we can't tell
	     * the difference at this stage.
	     */

	    for (bracketp = prog->startp[no]; bracketp < prog->endp[no];
							     bracketp++) {
		if (*bracketp == '\0') {
		    regerror("Damaged match string");
		    return;
		} else {
		    (void) flexaddch(dest, *bracketp);
		}
	    }
	}
    }
}

/*
 * do_substitute(window, lp, up, cmd)
 *
 * Perform a substitution from line 'lp' up to (but not including)
 * line 'up' using the command pointed to by 'cmd' which should be
 * of the form:
 *
 * /pattern/substitution/g
 *
 * The trailing 'g' is optional and, if present, indicates that multiple
 * substitutions should be performed on each line, if applicable.
 * The usual escapes are supported as described in the regexp docs.
 */
long
do_substitute(window, lp, up, command)
Xviwin	*window;
Line	*lp, *up;
char	*command;
{
    char	*copy;		/* for copy of command */
    regexp	*prog;
    char	*sub;
    char	*cp;
    char	delimiter;
    long	nsubs;

    copy = alloc((unsigned) strlen(command) + 1);
    if (copy == NULL) {
	return(0);
    }
    (void) strcpy(copy, command);

    delimiter = *copy;
    if (delimiter == '\0' ||
    			(cp = compile(&copy[1], delimiter, TRUE)) == NULL) {
	regerror("Usage: :s/search pattern/replacement/");
	free(copy);
	return(0);
    }
    sub = cp;
    prog = cur_prog();

    /*
     * Scan past the rhs to the flags, if any.
     */
    for (; *cp != '\0'; cp++) {
	if (*cp == '\\') {
	    if (*++cp == '\0') {
		break;
	    }
	} else if (*cp == delimiter) {
	    *cp++ = '\0';
	    break;
	}
    }

    /*
     * Save the regular expression for do_ampersand().
     */
    if (last_lhs) {
	rn_delete(last_lhs);
    }
    last_lhs = rn_duplicate(lastprogp);

    /*
     * Save the rhs.
     */
    if (last_rhs != NULL) {
	free(last_rhs);
    }
    last_rhs = strsave(sub);

    nsubs = substitute(window, lp, up, sub, cp);

    free(copy);

    return(nsubs);
}

/*
 * Repeat last substitution.
 *
 * For vi compatibility, this also changes the value of the last
 * regular expression used.
 */
long
do_ampersand(window, lp, up, flags)
Xviwin	*window;
Line	*lp, *up;
char	*flags;
{
    long	nsubs;

    if (last_lhs == NULL || last_rhs == NULL) {
	show_error(window, "No substitute to repeat!");
	return(0);
    }
    rn_delete(lastprogp);
    lastprogp = rn_duplicate(last_lhs);
    nsubs = substitute(window, lp, up, last_rhs, flags);
    return(nsubs);
}

/*
 * Apply last right-hand side used in a substitution to last regular
 * expression used.
 *
 * For vi compatibility, this also changes the value of the last
 * substitution.
 */
long
do_tilde(window, lp, up, flags)
Xviwin	*window;
Line	*lp, *up;
char	*flags;
{
    long	nsubs;

    if (lastprogp == NULL || last_rhs == NULL) {
	show_error(window, "No substitute to repeat!");
	return(0);
    }
    if (last_lhs) {
	rn_delete(last_lhs);
    }
    last_lhs = rn_duplicate(lastprogp);
    nsubs = substitute(window, lp, up, last_rhs, flags);
    return(nsubs);
}

static long
substitute(window, lp, up, sub, flags)
Xviwin	*window;
Line	*lp, *up;
char	*sub;
char	*flags;
{
    long	nsubs;
    Flexbuf	ns;
    regexp	*prog;
    bool_t	do_all;		/* true if 'g' was specified */

    if (!start_command(window)) {
	return(0);
    }

    prog = cur_prog();

    do_all = (*flags == 'g');

    nsubs = 0;

    /*
     * If no range was given, do the current line.
     * If only one line was given, just do that one.
     * Ensure that "up" points at the line after the
     * last one in the range, to make the loop easier.
     */
    if (lp == NULL) {
	lp = window->w_cursor->p_line;
    }
    if (up == NULL) {
	up = lp->l_next;
    } else {
	up = up->l_next;
    }
    flexnew(&ns);
    for (; lp != up; lp = lp->l_next) {
	if (regexec(prog, lp->l_text, TRUE)) {
	    char	*p, *matchp;

	    /*
	     * Save the line that was last changed for the final
	     * cursor position (just like the real vi).
	     */
	    move_cursor(window, lp, 0);

	    flexclear(&ns);
	    p = lp->l_text;

	    do {
		/*
		 * Copy up to the part that matched.
		 */
		while (p < prog->startp[0]) {
		    (void) flexaddch(&ns, *p);
		    p++;
		}

		regsubst(prog, sub, &ns);

		/*
		 * Continue searching after the match.
		 *
		 * Watch out for null matches - we
		 * don't want to go into an endless
		 * loop here.
		 */
		matchp = p = prog->endp[0];
		if (prog->startp[0] >= p) {
		    if (*p == '\0') {
			/*
			 * End of the line.
			 */
			break;
		    } else {
			matchp++;
		    }
		}

	    } while (do_all && regexec(prog, matchp, FALSE));

	    /*
	     * Copy the rest of the line, that didn't match.
	     */
	    (void) lformat(&ns, "%s", p);
	    replchars(window, lp, 0, strlen(lp->l_text),
		  flexgetstr(&ns));
	    nsubs++;
	}
    }
    flexdelete(&ns);			/* free the temp buffer */
    end_command(window);

    if (!nsubs && (echo & e_NOMATCH)) {
	regerror("No match");
    }
    return(nsubs);
}
