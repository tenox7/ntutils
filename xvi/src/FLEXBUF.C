/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)flexbuf.c	2.2 (Chris & John Downey) 8/27/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    flexbuf.c
* module function:
    Routines for Flexbufs (variable-length FIFO queues).

    Some of the access routines are implemented as macros in xvi.h.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

#define FLEXEXTRA	64

/*
 * Append a single character to a Flexbuf. Return FALSE if we've run
 * out of space.
 *
 * Note that the f->fxb_chars array is not necessarily null-terminated.
 */
bool_t
flexaddch(f, ch)
register Flexbuf	*f;
int	ch;
{
    if (flexempty(f))
	f->fxb_rcnt = f->fxb_wcnt = 0;
    if (f->fxb_wcnt >= f->fxb_max) {
	if (f->fxb_max == 0) {
	    if ((f->fxb_chars = alloc(FLEXEXTRA)) == NULL) {
		return FALSE;
	    } else {
		f->fxb_max = FLEXEXTRA;
	    }
	} else {
	    unsigned newsize = f->fxb_wcnt + FLEXEXTRA;

	    if ((f->fxb_chars = realloc(f->fxb_chars, newsize)) == NULL) {
		f->fxb_wcnt = f->fxb_max = 0;
		return FALSE;
	    } else {
		f->fxb_max = newsize;
	    }
	}
    }
    f->fxb_chars[f->fxb_wcnt++] = ch;
    return TRUE;
}

/*
 * Return contents of a Flexbuf as a null-terminated string.
 */
char *
flexgetstr(f)
Flexbuf	*f;
{
    if (!flexempty(f) && flexaddch(f, '\0')) {
	--f->fxb_wcnt;
	return &f->fxb_chars[f->fxb_rcnt];
    } else {
	return "";
    }
}

/*
 * Remove the first character from a Flexbuf and return it.
 */
int
flexpopch(f)
Flexbuf	*f;
{
    return flexempty(f) ?
	0 : (unsigned char) f->fxb_chars[f->fxb_rcnt++];
}

/*
 * Free storage belonging to a Flexbuf.
 */
void
flexdelete(f)
Flexbuf	*f;
{
    if (f->fxb_max > 0) {
	(void) free(f->fxb_chars);
	f->fxb_wcnt = f->fxb_max = 0;
    }
}

/*
 * The following routines provide for appending formatted data to a
 * Flexbuf using a subset of the format specifiers accepted by
 * printf(3). The specifiers we understand are currently
 *
 *	%c %d %ld %lu %s %u
 *
 * Field width, precision & left justification can also be specified
 * as for printf().
 *
 * The main advantage of this is that we don't have to worry about the
 * end of the destination array being overwritten, as we do with
 * sprintf(3).
 */

static	unsigned    width;
static	unsigned    prec;
static	bool_t	    ljust;

/*
 * Append a string to a Flexbuf, truncating the string & adding filler
 * characters as specified by the width, prec & ljust variables.
 *
 * The precision specifier gives the maximum field width.
 */
static bool_t
strformat(f, p)
Flexbuf *f;
register char *p;
{
    register int c;

    if (width != 0 && !ljust) {
	register unsigned len;

	len = strlen(p);
	if (prec != 0 && prec < len)
	    len = prec;
	while (width > len) {
	    if (!flexaddch(f, ' '))
		return FALSE;
	    --width;
	}
    }

    while ((c = *p++) != '\0') {
	if (!flexaddch(f, c))
	    return FALSE;
	if (width != 0)
	    --width;
	if (prec != 0) {
	    if (--prec == 0) {
		break;
	    }
	}
    }
    while (width != 0) {
	if (!flexaddch(f, ' '))
	    return FALSE;
	--width;
    }
    return TRUE;
}

/*
 * Given a binary long integer, convert it to a decimal number &
 * append it to the end of a Flexbuf.
 *
 * The precision specifier gives the minimum number of decimal digits.
 */
static bool_t
numformat(f, n, uflag)
Flexbuf	*f;
long	n;
bool_t	uflag;
{
    register char *s;
    register unsigned len;

    if (n == 0) {
	/*
	 * Special case.
	 */
	s = "0";
	len = 1;
    } else {
	static char dstr[sizeof (long) * 3 + 2];
	register unsigned long un;
	int neg;

	* (s = &dstr[sizeof dstr - 1]) = '\0';

	if (!uflag && n < 0) {
	    neg = 1;
	    un = -n;
	} else {
	    neg = 0;
	    un = n;
	}

	while (un > 0 && s > &dstr[1]) {
	    *--s = (char)((un % 10) + '0');
	    un /= 10;
	}

	if (neg)
	    *--s = '-';
	len = &dstr[sizeof dstr - 1] - s;
    }

    while (width > len && width > prec) {
	if (!flexaddch(f, ' '))
	    return FALSE;
	--width;
    }

    while (prec > len) {
	if (!flexaddch(f, '0'))
	    return FALSE;
	--prec;
	if (width > 0)
	    --width;
    }
    prec = 0;
    return strformat(f, s);
}

/*
 * Main formatting routine.
 */
bool_t
vformat
#ifdef __STDC__
    (Flexbuf *f, register char *format, register va_list argp)
#else
    (f, format, argp)
    Flexbuf		*f;
    register char	*format;
    register va_list	argp;
#endif
{
    register int c;

    while ((c = *format++) != '\0') {
	if (c == '%') {
	    static char cstr[2];
	    bool_t lnflag;
	    bool_t zflag;

	    lnflag = FALSE;
	    ljust = FALSE;
	    width = 0;
	    prec = 0;
	    zflag = FALSE;
	    if ((c = *format++) == '-') {
		ljust = TRUE;
		c = *format++;
	    }

	    /*
	     * Get width specifier.
	     */
	    if (c == '0') {
		if (ljust)
		    /*
		     * It doesn't make sense to
		     * have a left-justified
		     * numeric field with zero
		     * padding.
		     */
		    return FALSE;
		zflag = TRUE;
		c = *format++;
	    }

	    while (c && is_digit(c)) {
		if (width != 0)
		    width *= 10;
		width += (c - '0');
		c = *format++;
	    }

	    if (c == '.') {
		/*
		 * Get precision specifier.
		 */
		while ((c = *format++) != '\0' && is_digit(c)) {
		    if (prec != 0)
			prec *= 10;
		    prec += (c - '0');
		}
	    }

	    switch (c) {
	    case '%':
		cstr[0] = c;
		if (!strformat(f, cstr))
		    return FALSE;
		continue;

	    case 'c':
		cstr[0] = va_arg(argp, int);
		if (!strformat(f, cstr))
		    return FALSE;
		continue;

	    case 'l':
		switch (c = *format++) {
		case 'd':
		case 'u':
		    lnflag = TRUE;
		    break;
		default:
		    /*
		     * Syntax error.
		     */
		    return FALSE;
		}
		/* fall through ... */

	    case 'd':
	    case 'u':
	    {
		long n;

		if (lnflag)
		{
		    n = (c == 'u' ?
		    		(long) va_arg(argp, unsigned long) :
				va_arg(argp, long));
		} else {
		    n = (c == 'u' ?
				(long) va_arg(argp, unsigned int) :
				(long) va_arg(argp, int));
		}

		/*
		 * For integers, the precision
		 * specifier gives the minimum number
		 * of decimal digits.
		 */
		if (zflag && prec < width) {
		    prec = width;
		    width = 0;
		}
		if (!numformat(f, n, (c == 'u')))
		    return FALSE;
		continue;
	    }

	    case 's':
	    {
		char *sp;

		if ((sp = va_arg(argp, char *)) == NULL ||
						!strformat(f, sp)) {
		    return FALSE;
		}
		continue;
	    }

	    default:
		/*
		 * Syntax error.
		 */
		return FALSE;
	    }
	} else if (!flexaddch(f, c)) {
	    return FALSE;
	}
    }
    return TRUE;
}

/*
 * Front end callable with a variable number of arguments.
 */
/*VARARGS2*/
bool_t
lformat
#ifdef __STDC__
    (Flexbuf *f, char *format, ...)
#else
    (f, format, va_alist)
    Flexbuf	*f;
    char	*format;
    va_dcl
#endif
{
    va_list argp;
    bool_t retval;

    VA_START(argp, format);
    retval = vformat(f, format, argp);
    va_end(argp);
    return retval;
}
