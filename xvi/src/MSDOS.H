/* Copyright (c) 1990,1991,1992 Chris and John Downey */
/***
* @(#)msdos.h	2.1 (Chris & John Downey) 7/29/92

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    msdos.h
* module function:
    Definitions for MS-DOS system interface.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <stdlib.h>
#include <time.h>

/*
 * This is a workaround for a particularly nasty bug which appeared
 * in one compiler recently.
 */
#ifdef NULL
#   undef NULL
#endif
#define NULL	((void *) 0)

/*
 * Get definitions for terminal interface.
 */
#ifdef DOS386
#   include "pc386.h"
#else
#   include "ibmpc.h"
#endif

#ifndef HELPFILE
#   define HELPFILE	"c:\\xvi\\help"
#endif

/*
 * System-dependent constants.
 */
#define	MAXPATHLEN	143	/* maximum length of full path name */
#define	MAXNAMLEN	12	/* maximum length of file name */
#define	DIRSEPS		"\\/"	/*
				 * directory separators within
				 * pathnames
				 */
/*
 * Under MS-DOS, characters with the top bit set are perfectly valid
 * (although not necessarily always what you want to see).
 */
#define DEF_CCHARS	TRUE
#define DEF_MCHARS	TRUE

#define DEF_TFF		fmt_MSDOS

/*
 * Macros to open files in binary mode,
 * and to expand filenames.
 */
#define fopenrb(f)	fopen((f),"rb")
#define fopenwb(f)	fopen((f),"wb")
#define fexpand(f)	(f)

/*
 * Execute a command in a subshell.
 */
#define call_system(s)	system(s)

/*
 * Execute a shell.
 */
#define call_shell(s)	spawnlp(0, (s), (s), (char *) NULL)

/*
 * Size of buffer for file i/o routines.
 * The SETVBUF_AVAIL forces the file i/o routines to
 * use a large buffer for reading and writing, and
 * this results in a large performance improvement.
 */
#define SETVBUF_AVAIL
#define WRTBUFSIZ	0x7000
#define READBUFSIZ	0x1000

#ifdef	__ZTC__
#   define exists(f)	(findfirst((f),FA_HIDDEN|FA_SYSTEM|FA_DIREC)\
								!= NULL)
#else /* not Zortech */
#   include <malloc.h>
#   define  exists(f)	(access(f, 0) == 0)
#endif /* not Zortech */

/*
 * Definitions needed for dstat() system call.
 */
struct dstat
{
    char		d_bytealigned[43];
};

#define	dst_mode	d_bytealigned[21]
#define dst_BASENAME(i)	(&(i).d_bytealigned[30])
			/* file's basename */
#define dst_LAST(i)	(&(i).d_bytealigned[42])
			/* pointer to terminating '\0' of basename */

/*
 * Mode bits.
 */
#define	dst_READONLY	1
#define	dst_HIDDEN	2
#define	dst_SYSTEM	4
#define	dst_VLABEL	010
#define	dst_DIR		020
#define	dst_ARCHIVE	040

/*
 * Declarations for system interface routines in msdos_c.c &
 * msdos_a.asm (or pc386.c):
 */
extern	bool_t		can_write P((char *));
extern	void		catchints P((void));
extern	void		delay P((void));
extern	int		statfirst P((char *, struct dstat *, int));
extern	int		statnext P((struct dstat *));
extern	void		sys_init P((void));
extern	void		sys_startv P((void));
extern	void		sys_endv P((void));
extern	void		sys_exit P((int));
extern	bool_t		sys_pipe P((char *, int (*)(FILE *), long (*)(FILE *)));
extern	char		*tempfname P((char *));
