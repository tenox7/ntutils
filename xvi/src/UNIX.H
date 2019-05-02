/* Copyright (c) 1990,1991,1992 Chris and John Downey */
/***

* @(#)unix.h	2.1 (Chris & John Downey) 7/29/92

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    unix.h
* module function:
    Definitions for UNIX system interface module.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include <errno.h>
#include <fcntl.h>
#if defined(BSD) && !defined(__STDC__)
#   include <sys/time.h>
#else
#   include <time.h>
#endif

#if defined(SUNVIEW) || defined(XVIEW)
#   include "sunview.h"
#else
#   include "termcap.h"
#endif

/*
 * Default value for helpfile parameter.
 */
#ifndef HELPFILE
#   define  HELPFILE	"/usr/lib/xvi.help"
#endif

#if defined(sun) || defined(ultrix) || defined(XENIX) || defined(__STDC__)
#   define SETVBUF_AVAIL
    /*
     * These are the buffer sizes we use for reading & writing files.
     */
#   define  READBUFSIZ	16384
#   define  WRTBUFSIZ	16384
#endif

/*
 * If we don't have an ANSI compiler, strerror() is implemented in unix.c.
 */
#define STRERROR_AVAIL

#ifndef	__STDC__
    /*
     * Conditionally compile this to be safe.
     */
    extern const char	*strerror P((int));
#endif

#ifdef sun
    /*
     * getwd() is a system call, which doesn't need to run the pwd
     * program (as getcwd() does).
     */
    extern char		*getwd P((char *));
#   define getcwd(p,s)	getwd(p)
#endif

/*
 * Macros to open files in binary mode.
 */
#define fopenrb(f)	fopen((f),"r")
#define fopenwb(f)	fopen((f),"w")

/*
 * ANSI C libraries should have remove(), but unlink() does exactly
 * the same thing.
 */
#define remove(f)	unlink(f)

/*
 * BSD doesn't provide memcpy, but bcopy does the same thing.
 * Similarly, strchr=index and strrchr=rindex.
 * However, any __STDC__ environment must provide these,
 * and SunOS provides them too, as does ULTRIX.
 */
#if defined(BSD) && !defined(__STDC__) && !defined(sun) && !defined(ultrix)
#   define  memcpy(to, from, n)	bcopy((from), (to), (n))
#   define  strchr		index
#   define  strrchr		rindex
#endif

/*
 * System-dependent constants - these are needed by file i/o routines.
 */
#ifdef	BSD
#   include <sys/param.h>
#   include <sys/dir.h>
#   include <sys/file.h>	/* get W_OK define for access() */
#else	/* not BSD */
    /*
     * I think these are right for System V. (jmd)
     */
#   define  MAXPATHLEN	1024	/* max length of full path name */
#   define  MAXNAMLEN	14	/* max length of file name */
#endif	/* BSD */

#ifndef W_OK
#   define	F_OK	0
#   define	W_OK	2
#endif

/*
 * exists(): TRUE if file exists.
 */
#define exists(f)	(access((f),F_OK) == 0)

/*
 * can_write(): TRUE if file does not exist or exists and is writeable.
 */
#define can_write(f)	(access((f),F_OK) != 0 || access((f), W_OK) == 0)

#define	DIRSEPS		"/"	/* directory separators within pathnames */

/*
 * Default file format.
 */
#define DEF_TFF		fmt_UNIX

/*
 * These are needed for the termcap terminal interface module.
 */
#define oflush()	(void) fflush(stdout)
#define moutch(c)	putchar(c)

/*
 * Declarations for standard UNIX functions.
 */
extern	int		rename P((const char *, const char *));

/*
 * Declarations for system interface routines in unix.c.
 */
extern	char		*fexpand P((char *));
extern	void		foutch P((int));
extern	void		delay P((void));
extern	void		sys_init P((void));
extern	void		sys_startv P((void));
extern	void		sys_endv P((void));
extern	void		sys_exit P((int));
extern	int		call_shell P((char *));
extern	int		call_system P((char *));
extern	bool_t		sys_pipe P((char *, int (*)(FILE *), long (*)(FILE *)));
extern	char		*tempfname P((char *));
