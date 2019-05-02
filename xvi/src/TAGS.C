/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)tags.c	2.1 (Chris & John Downey) 7/29/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    tags.c
* module function:
    Handle tags.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

#define	LSIZE		512	/* max. size of a line in the tags file */

/*
 * Macro evaluates true if char 'c' is a valid identifier character.
 * Used by tagword().
 */
#define IDCHAR(c)	(is_alnum(c) || (c) == '_')

/*
 * Return values for find_tag().
 */
#define	FT_SUCCESS	0		/* found tag */
#define	FT_CANT_OPEN	1		/* can't open given file */
#define	FT_NO_MATCH	2		/* no such tag in file */
#define	FT_ERROR	3		/* error in tags file */

static	int	find_tag P((char *, char *, char **, char **));

/*
 * Tag to the word under the cursor.
 */
void
tagword()
{
    char	ch;
    Posn	pos;
    char	tagbuf[50];
    char	*tp = tagbuf;

    pos = *curwin->w_cursor;

    ch = gchar(&pos);
    if (!IDCHAR(ch))
	return;

    /*
     * Now grab the chars in the identifier.
     */
    while (IDCHAR(ch) && tp < tagbuf + sizeof(tagbuf)) {
	*tp++ = ch;
	if (inc(&pos) != mv_SAMELINE)
	    break;
	ch = gchar(&pos);
    }

    /*
     * If the identifier is too long, just beep.
     */
    if (tp >= tagbuf + sizeof(tagbuf)) {
	beep(curwin);
	return;
    }

    *tp = '\0';

    (void) do_tag(curwin, tagbuf, FALSE, TRUE, TRUE);
}

/*
 * do_tag(window, tag, force, interactive) - goto tag
 */
bool_t
do_tag(window, tag, force, interactive, split)
Xviwin	*window;
char	*tag;			/* function to search for */
bool_t	force;			/* if true, force re-edit */
bool_t	interactive;		/* true if reading from tty */
bool_t	split;			/* true if want to split */
{
    char	*fname;		/* file name to edit (2nd field) */
    char	*field3;	/* 3rd field - pattern or line number */
    bool_t	edited;		/* TRUE if we have edited the file */
    Xviwin	*tagwindow;	/* tmp window pointer */
    int		status;		/* return value from find_tag() */
    int		count;
    char	**tagfiles;

    if (tag == NULL || tag[0] == '\0') {
	if (interactive) {
	    show_error(window, "Usage: :tag <identifier>");
	} else {
	    beep(window);
	}
	return(FALSE);
    }

    tagfiles = Pl(P_tags);
    if (tagfiles == NULL || tagfiles[0] == NULL) {
	if (interactive) {
	    show_error(window, "No tags parameter set!");
	    return(FALSE);
	}
    }

    gotocmd(window, FALSE);

    for (count = 0; tagfiles[count] != NULL; count++) {
	status = find_tag(tag, tagfiles[count], &fname, &field3);
	if (status == FT_SUCCESS) {
	    break;
	}
    }

    /*
     * Either:
     *	we have found the tag in tagfiles[count]
     * or:
     *	we have failed to find it, and tagfiles[count - 1]
     *	contains the name of the last tags file tried.
     */

    switch (status) {
    case FT_CANT_OPEN:
	if (interactive) {
	    show_error(window, "Can't open tags file \"%s\"",
			    tagfiles[count - 1]);
	}
	return(FALSE);

    case FT_ERROR:
	if (interactive) {
	    show_error(window, "Format error in tags file \"%s\"",
			    tagfiles[count - 1]);
	}
	return(FALSE);

    case FT_NO_MATCH:
	if (interactive) {
	    show_error(window, "Tag not found");
	}
	return(FALSE);
    }

    /*
     * If we are already editing the file, just do the search.
     *
     * If "autosplit" is set, create a new buffer window and
     * edit the file in it.
     *
     * Else just edit it in the current window.
     */
    tagwindow = find_window(window, fname);
    if (tagwindow != NULL) {
	curwin = tagwindow;
	curbuf = curwin->w_buffer;
	edited = TRUE;

    } else if (split && can_split() && do_buffer(window, fname)) {
	edited = TRUE;

    } else if (do_edit(window, force, fname)) {
	edited = TRUE;

    } else {
	/*
	 * If the re-edit failed, abort here.
	 */
	edited = FALSE;
    }

    /*
     * Finally, search for the given pattern in the file,
     * but only if we successfully edited it. Note that we
     * always use curwin at this stage because it is not
     * necessarily the same as our "window" parameter.
     */
    if (edited) {
	char	*cp;

	/*
	 * Remove trailing newline if present.
	 */
	cp = field3 + strlen(field3) - 1;
	if (*cp == '\n') {
	    *cp-- = '\0';
	}

	if (*field3 == '/' || *field3 == '?') {
	    int	old_rxtype;

	    /*
	     * Remove leading and trailing '/'s or '?'s
	     * from the search pattern.
	     */
	    field3++;
	    if (*cp == '/' || *cp == '?') {
		*cp = '\0';
	    }

	    /*
	     * Set the regular expression type to rt_TAGS
	     * so that only ^ and $ have a special meaning;
	     * this is like nomagic in "real" vi.
	     */
	    old_rxtype = Pn(P_regextype);
	    set_param(P_regextype, rt_TAGS, (char **) NULL);

	    if (dosearch(curwin, field3, '/')) {
		show_file_info(curwin);
	    } else {
		beep(curwin);
	    }
	    set_param(P_regextype, old_rxtype, (char **) NULL);
	} else if (is_digit(*field3)) {
	    /*
	     * Not a search pattern; a line number.
	     */
	    do_goto(atol(field3));
	} else {
	    show_error(curwin, "Ill-formed tag pattern \"%s\"", field3);
	}
	move_window_to_cursor(curwin);
	update_all();
    }

    return(TRUE);
}

static int
find_tag(tag, file, fnamep, pattern)
char	*tag;
char	*file;
char	**fnamep;
char	**pattern;
{
    register char	*str;		/* used for scanning strings */
    FILE		*tp;		/* file pointer for tags file */
    static char		lbuf[LSIZE];	/* input line from tags file */
    bool_t		found;
    int			max_chars;

    max_chars = Pn(P_taglength);
    if (max_chars == 0) {
	max_chars = INT_MAX;
    }

    tp = fopen(file, "r");
    if (tp == NULL) {
	return(FT_CANT_OPEN);
    }

    found = FALSE;
    while (fgets(lbuf, LSIZE, tp) != NULL) {
	register char	*tagptr;
	register int	nchars;

	for (str = lbuf, tagptr = tag, nchars = 0;
		*str == *tagptr && nchars < max_chars;
		str++, tagptr++, nchars++) {
	    ;
	}
	if ((*tagptr == '\0' && *str == '\t') || nchars == max_chars) {
	    found = TRUE;
	    break;
	}

    }

    (void) fclose(tp);

    if (!found) {
	return(FT_NO_MATCH);
    }

    /*
     * Okay, we have found the right line.
     * Now extract the second and third fields.
     *
     * If we get here, str points at the tab after the first field,
     * or at part of the tag if we have matched up to taglength.
     * So first move to the start of the second field.
     */
    while (!is_space(*str)) {
	str++;
    }
    while (is_space(*str)) {
	str++;
    }
    *fnamep = str;

    /*
     * Find third field and null-terminate second field.
     */
    str = strchr(str, '\t');
    if (str == NULL) {
	(void) fclose(tp);
	return(FT_ERROR);
    }
    while (is_space(*str)) {
	*str++ = '\0';
    }
    *pattern = str;

    return(FT_SUCCESS);
}
