/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)map.c	2.1 (Chris & John Downey) 7/29/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    map.c
* module function:
    Keyboard input/pushback routines, "map" command.

    Note that we provide key mapping through a different interface,
    so that cursor key mappings etc do not show up to the user.
    This works by having a two-stage process; first keymapping is
    done, and then the result is fed through the normal mapping
    process. The intent of the keymapping stage is to convert
    machine-local keys into a standard form.

* bug:
    If a map fails, we just pass all characters which had already
    been accepted, plus the character which caused the mismatch,
    straight through. This is not quite correct because we might
    have got a good match starting at the very next character, i.e.
    if we have mapped

    	foo	to	bar

    and get input "ffoo", then the seconf 'f' will cause the map to
    fail and both characters will go through, and so the whole thing
    will pass through unmapped.

    The only way around this problem is to introduce another flexbuf
    on the input side of the keymap stage, to give us somewhere to
    put all characters after the first, when a map fails. I.e. in the
    case above, we would pass the first 'f' through to the "mp_dest"
    flexbuf, and stuff any characters after that into "mp_src".

* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

/*
 * This is the fundamental structure type which is used
 * to hold mappings from one string to another.
 */
typedef struct map {
    struct map	    *m_next;
    char	    *m_lhs;		/* lhs of map */
    char	    *m_rhs;		/* rhs of map */
    unsigned int    m_same;		/* no characters same as next map */
} Map;

/*
 * This structure holds a current position while scanning a map list.
 * It is also effectively used to form a chain of mapping structures,
 * interconnected with flexbufs.
 */
typedef struct mpos {
    Map		*mp_map;
    int		mp_index;
    Flexbuf	*mp_src;
    Flexbuf	*mp_dest;
} Mpos;

/*
 * Two mapping structures exist; one for "map", and one for "map!".
 */
static	Map	*cmd_map = NULL;
static	Map	*ins_map = NULL;
static	Map	*key_map = NULL;

/*
 * This is the position in cmd_map/ins_map when we are waiting for
 * the next character. When not waiting, mp_map is NULL (as at start).
 * Also stores flexbuf pointers for input and output at each stage.
 */
static	Flexbuf	getcbuff;
static	Flexbuf inputbuff;
static	Mpos	npos = { NULL, 0, &inputbuff,	&getcbuff };
static	Mpos	kpos = { NULL, 0, NULL,		&inputbuff };

/*
 * This is used for "display" mode; it records the current map which
 * is being displayed. It is used by show_map().
 */
static	Map	*curmap;
static	char	*show_map P((void));

static	void	mapthrough P((void));
static	bool_t	process_map P((int, Mpos *));
static	void	calc_same P((Map *));
static	void	map_failed P((Mpos *));
static	void	insert_map P((Map **, char *, char *, bool_t));
static	void	delete_map P((Map **, char *));

/*VARARGS1*/
/*PRINTFLIKE*/
void
stuff
#ifdef	__STDC__
    (char *format, ...)
#else /* not __STDC__ */
    (format, va_alist)
    char	*format;
    va_dcl
#endif
{
    va_list	argp;

    VA_START(argp, format);
    (void) vformat(npos.mp_dest, format, argp);
    va_end(argp);
}

int
map_getc()
{
    return(flexempty(npos.mp_dest) ? EOF : flexpopch(npos.mp_dest));
}

void
map_char(c)
register int	c;
{
    /*
     * Send the input character through the keymap list.
     */
    if (kpos.mp_map == NULL) {
    	kpos.mp_map = key_map;
	kpos.mp_index = 0;
    }
    if (process_map(c, &kpos) == FALSE) {
	/*
	 * Send resulting output through the normal map list.
	 */
	kpos.mp_map = NULL;
	mapthrough();
    }
}

/*
 * Process any characters in the input buffer through the
 * cmd_map/ins_map lists into the output buffer, whence
 * characters go into the editor itself.
 */
static void
mapthrough()
{
    while (!flexempty(npos.mp_src)) {
	if (npos.mp_map == NULL) {
	    if (State == NORMAL) {
		npos.mp_map = cmd_map;
	    } else if (State == INSERT || State == REPLACE) {
		npos.mp_map = ins_map;
	    }
	    npos.mp_index = 0;
	}

	if (process_map(flexpopch(npos.mp_src), &npos) == FALSE) {
	    npos.mp_map = NULL;
	}
    }
}

/*
 * Process the given character through the maplist pointed to
 * by the given position. Returns TRUE if we should continue,
 * or FALSE if this attempt at mapping has terminated (either
 * due to success or definite failure).
 */
static bool_t
process_map(c, pos)
register int	c;
register Mpos	*pos;
{
    register Map	*tmp;
    register int	ind;

    ind = pos->mp_index;
    for (tmp = pos->mp_map; tmp != NULL; tmp = tmp->m_next) {
	if (tmp->m_lhs[ind] == c) {
	    if (tmp->m_lhs[ind + 1] == '\0') {
		/*
		 * Found complete match. Insert the result into
		 * the appropriate output buffer, according to
		 * whether "remap" is set or not.
		 */
		(void) lformat((Pb(P_remap) && pos->mp_src != NULL) ?
			    pos->mp_src : pos->mp_dest, "%s", tmp->m_rhs);
		return(FALSE);
	    } else {
		/*
		 * Found incomplete match,
		 * keep going.
		 */
		pos->mp_map = tmp;
		pos->mp_index++;
	    }
	    return(TRUE);
	}

	/*
	 * Can't move on to next map entry unless the m_same
	 * field is sufficient that the match so far would
	 * have worked.
	 */
	if (tmp->m_same < ind) {
	    break;
	}
    }

    map_failed(pos);

    /*
     * Don't forget to re-stuff the character we have just received.
     */
    (void) flexaddch(pos->mp_dest, c);
    return(FALSE);
}

void
map_timeout()
{
    if (kpos.mp_map != NULL) {
	map_failed(&kpos);
	mapthrough();
    } else {
    	map_failed(&npos);
    }
}

bool_t
map_waiting()
{
    return(kpos.mp_map != NULL || npos.mp_map != NULL);
}

/*
 * This routine is called when a timeout has occurred.
 */
static void
map_failed(pos)
Mpos	*pos;
{
    register char	*cp;
    register Flexbuf	*fbp;
    register int	i;

    if (pos->mp_map != NULL) {
	fbp = pos->mp_dest;
	for (i = 0, cp = pos->mp_map->m_lhs; i < pos->mp_index; i++) {
	    (void) flexaddch(fbp, cp[i]);
	}
	pos->mp_map = NULL;
    }
}

/*
 * Insert the key map lhs as mapping into rhs.
 */
void
xvi_keymap(lhs, rhs)
char	*lhs;
char	*rhs;
{
    insert_map(&key_map, lhs, rhs, FALSE);
}

/*
 * Insert the entry "lhs" as mapping into "rhs".
 */
void
xvi_map(argc, argv, exclam, inter)
int	argc;
char	*argv[];
bool_t	exclam;
bool_t	inter;
{
    switch (argc) {
    case 2:				/* valid input */
	if (argv[0][0] == '\0') {
	    if (inter) {
		show_message(curwin, "Usage: map lhs rhs");
	    }
	    return;
	}
	insert_map(exclam ? &ins_map : &cmd_map, argv[0], argv[1], inter);
	break;

    case 0:
	curmap = exclam ? ins_map : cmd_map;
	disp_init(curwin, show_map, (int) curwin->w_ncols, FALSE);
	break;

    default:
	if (inter) {
	    show_message(curwin, "Wrong number of arguments to map");
	}
    }
}

static void
insert_map(maplist, left, right, interactive)
Map		**maplist;
char		*left;
char		*right;
bool_t		interactive;
{
    char	*lhs;			/* saved lhs of map */
    char	*rhs;			/* saved rhs of map */
    Map		*mptr;			/* new map element */
    Map		**p;			/* used for loop to find position */
    int		rel;

    lhs = strsave(left);
    if (lhs == NULL || (rhs = strsave(right)) == NULL) {
	if (interactive) {
	    show_message(curwin, "no memory for that map");
	}
	return;
    }

    mptr = (Map *) alloc(sizeof(Map));
    if (mptr == NULL) {
	free(lhs);
	free(rhs);
	return;
    }

    mptr->m_lhs = lhs;
    mptr->m_rhs = rhs;

    p = maplist;
    if ((*p) == NULL || strcmp((*p)->m_lhs, lhs) > 0) {
	/*
	 * Either there are no maps yet, or the one
	 * we want to enter should go at the start.
	 */
	mptr->m_next = *p;
	*p = mptr;
	calc_same(mptr);
    } else if (strcmp((*p)->m_lhs, lhs) == 0) {
	/*
	 * We need to replace the rhs of the first map.
	 */
	free(lhs);
	free((char *) mptr);
	free((*p)->m_rhs);
	(*p)->m_rhs = rhs;
	calc_same(*p);
    } else {
	for ( ; (*p) != NULL; p = &((*p)->m_next)) {
	    /*
	     * Set "rel" to +ve if the next element is greater
	     * than the current one, -ve if it is less, or 0
	     * if they are the same (if the lhs is the same).
	     */
	    rel = ((*p)->m_next == NULL) ? 1 :
		strcmp((*p)->m_next->m_lhs, lhs);

	    if (rel >= 0) {
		if (rel > 0) {
		    /*
		     * The right place to insert
		     * the new map.
		     */
		    mptr->m_next = (*p)->m_next;
		    (*p)->m_next = mptr;
		    calc_same(*p);
		    calc_same(mptr);
		} else /* rel == 0 */ {
		    /*
		     * The lhs of the new map is identical
		     * to that of an existing map.
		     * Replace the old rhs with the new.
		     */
		    free(lhs);
		    free((char *) mptr);
		    mptr = (*p)->m_next;
		    free(mptr->m_rhs);
		    mptr->m_rhs = rhs;
		    calc_same(mptr);
		}
		return;
	    }
	}
    }
}

void
xvi_unmap(argc, argv, exclam, inter)
int	argc;
char	*argv[];
bool_t	exclam;
bool_t	inter;
{
    int	count;

    if (argc < 1) {
	if (inter) {
	    show_message(curwin, "But what do you want me to unmap?");
	}
	return;
    }

    for (count = 0; count < argc; count++) {
	delete_map(exclam ? &ins_map : &cmd_map, argv[count]);
    }
}

static void
delete_map(maplist, lhs)
Map	**maplist;
char	*lhs;
{
    Map	*p;

    p = *maplist;
    if (p != NULL && strcmp(p->m_lhs, lhs) == 0) {
	*maplist = p->m_next;
    } else {
	for (; p != NULL; p = p->m_next) {
	    if (p->m_next != NULL && strcmp(lhs, p->m_next->m_lhs) == 0) {
		Map	*tmp;

		tmp = p->m_next;
		p->m_next = p->m_next->m_next;
		free(tmp->m_lhs);
		free(tmp->m_rhs);
		free((char *) tmp);
		calc_same(p);
	    }
	}
    }
}

static void
calc_same(mptr)
Map	*mptr;
{
    register char	*a, *b;

    mptr->m_same = 0;
    if (mptr->m_next != NULL) {
	for (a = mptr->m_lhs, b = mptr->m_next->m_lhs; *a == *b; a++, b++) {
	    mptr->m_same++;
	}
    }
}

static char *
show_map()
{
    static Flexbuf	buf;

    /*
     * Have we reached the end?
     */
    if (curmap == NULL) {
	return(NULL);
    }

    flexclear(&buf);
    (void) lformat(&buf, "%-18.18s %-s", curmap->m_lhs, curmap->m_rhs);
    curmap = curmap->m_next;
    return flexgetstr(&buf);
}
