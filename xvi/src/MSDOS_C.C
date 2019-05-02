/* Copyright (c) 1990,1991,1992 Chris and John Downey */
#ifndef lint
static char *sccsid = "@(#)msdos_c.c	2.1 (Chris & John Downey) 7/29/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    msdos_c.c
* module function:
    System interface routines for MS-DOS.

    The system interface consists of this module & msdos_a.asm;
    the terminal interface for IBM PC compatibles consists of
    ibmpc_a.asm & ibmpc_c.c. Some comments on the demarcation
    between system interface and terminal interface modules are in
    the source.ms document.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

#ifdef __ZTC__
    /*
     * Zortech 3.0 has a really stupid method of specifying
     * filename expansion on the command line.
     */
#   ifdef WILDCARDS
	WILDCARDS
#   endif /* WILDCARDS */

    /*
     * Set default stack size. This seems a reasonable compromise.
     */
    unsigned _stack = 20480;
#endif		/* __ZTC__ */

void
ignore_signals(void)
{
    msdsignal(&kbdintr);
}

void
sys_init(void)
{
    char	*sh;

    /*
     * Set up a default value for Ps(P_shell). This can be overridden
     * by xvi_startup() (in startup.c) if %SHELL% is defined, or if a
     * value is given explicitly in the startup
     * script.
     *
     * %COMSPEC% should always be defined on MS-DOS. The system can't
     * really run without it.
     */
    if ((sh = getenv("COMSPEC")) != NULL) {
	set_param(P_shell, sh);
    }
    tty_open(&Rows, &Columns);
    sys_startv();
}

static	enum {
	m_SYS = 0,
	m_VI
}	curmode;

/*
 * Set up video state for editor.
 */
void
sys_startv(void)
{
    if (curmode == m_SYS) {
	tty_startv();
	set_colour(Pn(P_colour));
	curmode = m_VI;
    }
}

/*
 * Restore video state to what it was when we started.
 */
void
sys_endv(void)
{
    if (curmode == m_VI) {
	tty_goto(Rows - 1, 0);
	set_colour(Pn(P_systemcolour));
	erase_line();
	tty_endv();
	/*
	 * tty_endv() is responsible for calling
	 * flush_output().
	 */
	curmode = m_SYS;
    }
}

void
sys_exit(int r)
{
    sys_endv();
    tty_close();
    exit(r);
}

#ifndef W_OK
#	define	F_OK	0
#	define	W_OK	2
#endif

/*
 * Returns TRUE if file does not exist or exists and is writeable.
 */
bool_t
can_write(char* file)
{
#ifdef __ZTC__
    struct FIND	*p;

    p = findfirst(file, FA_HIDDEN | FA_SYSTEM | FA_DIREC);

    return(p == NULL ||
       (p->attribute & (FA_RDONLY | FA_HIDDEN | FA_SYSTEM | FA_DIREC)) == 0);
#else	/* __ZTC__ */
    return access(file, F_OK) != 0 || access(file, W_OK) == 0;
#endif	/* __ZTC__ */
}

#ifndef __ZTC__

/*
 * The Zortech library has this anyway.
 */
void
sleep(n)
unsigned	n;
{
    /*
     * This works with Microsoft C.
     */
    time_t	start = time((time_t*) 0);

    flush_output();
    while (time((time_t*) 0) < start + n)
	;
}

#endif /* not __ZTC__ */

void
delay()
{
    clock_t start = clock();

    flush_output();
    while (clock() < start + CLOCKS_PER_SEC / 5)
	;
}

/*
 * This function is only used by tempfname(). It constructs a filename
 * suffix based on an index number.
 *
 * The suffix ".$$$" is commonly used for temporary file names on
 * MS-DOS & OS/2 systems. We also use the sequence ".$$1", ".$$2" ...
 * ".fff" (all digits are hexadecimal).
 */
static char *
hexsuffix(i)
unsigned	i;
{
    static char	suffix [] = ".$$$";
    static char	hextab [] = "0123456789abcdef";
    char*		sp = &suffix[3];

    while (sp > suffix) {
	if (i > 0) {
	    *sp-- = hextab [i & 0xf];
	    i >>= 4;
	} else {
	    *sp-- = '$';
	}
    }
    return suffix;
}

/*
 * Construct unique name for temporary file, to be used as a backup
 * file for the named file.
 */
char*
tempfname(srcname)
char		*srcname;
{
    char	*srctail,
		*srcdot,
		*endp,
		*retp;
    unsigned	indexnum = 0;
    unsigned	baselen;

    srctail = srcdot = NULL;
    endp = srcname;

    while (*endp) {
	switch (*endp++) {
	case '\\':
	case '/':
	    srctail = endp;
	    srcdot = (char*) 0;
	    continue;
	case '.':
	    srcdot = endp - 1;
	}
    }

    if (srctail == NULL) {
	/*
	 * We haven't found any directory separators ('/' or '\\').
	 */
	srctail = srcname;
	/*
	 * Check to see if there's a disk drive name. If there
	 * is, skip over it.
	 */
	if (*srcname && is_alpha(*srcname) && srcname[1] == ':') {
	    srctail = &srcname[2];
	}
    }

    /*
     * There isn't a dot in the trailing part of the filename:
     * just add it at the end.
     */
    if (srcdot == NULL) {
	srcdot = endp;
    }

    /*
     * Don't make name too long.
     */
    if (srcdot - srctail > MAXNAMLEN - 4)
	srcdot = srctail + MAXNAMLEN - 4;
    if (srcdot - srcname > MAXPATHLEN - 4)
	srcdot = srcname + MAXPATHLEN - 4;
    baselen = srcdot - srcname;
    /*
     * Allocate space for new temporary file name ...
     */
    if ((retp = alloc(baselen + 5)) == NULL)
	return NULL;
    if (baselen > 0)
	(void) memcpy(retp, srcname, baselen);
    do {
	/*
	 * Keep trying this until we get a unique file name.
	 */
	strcpy(&retp[baselen], hexsuffix(indexnum++));
    } while (exists(retp));
    return retp;
}

/*
 * Fake out a pipe by writing output to temp file, running a process with
 * i/o redirected from this file to another temp file, and then reading
 * the second temp file back in.
 */
bool_t
sys_pipe(cmd, writefunc, readfunc)
char	*cmd;
int	(*writefunc) P((FILE *));
long	(*readfunc) P((FILE *));
{
    char	*temp1;
    FILE	*fp;
    bool_t	retval;

    /*
     * Create first temporary file ...
     */
    if ((temp1 = tempfname("xvi_out")) == NULL ||
    				(fp = fopen(temp1, "w")) == NULL) {
	retval = FALSE;
    } else {
	char	*temp2 = NULL;
	int	savcon;
	int	fd1 = -1,
		fd2 = -1;

	/*
	 * ... then write to it & close it ...
	 */
	(void) (*writefunc)(fp);
	(void) fclose(fp);

	/*
	 * ... then re-open it for reading, open second one
	 * for writing & re-arrange file descriptors.
	 *
	 * Note that we assume that the editor's standard
	 * input, output & error files are the same device,
	 * since I can't imagine how any of them could
	 * usefully be redirected to anything else.
	 */

#ifndef _O_BINARY
#	define _O_BINARY 0
#endif
#ifndef _O_EXCL
#	define _O_EXCL 0
#endif
	if (
	    (savcon = dup(0)) < 3
	    ||
	    (fd1 = open(temp1, _O_RDONLY | _O_BINARY)) < 3
	    ||
	    (temp2 = tempfname("xvi_in")) == NULL 
	    ||
	    (fd2 = open(temp2, _O_WRONLY|_O_CREAT|_O_EXCL|_O_BINARY, 0600)) < 3
	) {
	    retval = FALSE;
	} else {
	    (void) dup2(fd1, 0);
	    (void) dup2(fd2, 1);
	    (void) dup2(fd2, 2);

	    (void) close(fd1);
	    (void) close(fd2);
	    fd1 = fd2 = -1;

	    /*
	     * Run the command.
	     */
	    (void) system(cmd);

	    /*
	     * Restore our standard input, output & error
	     * files.
	     */
	    (void) dup2(savcon, 0);
	    (void) dup2(savcon, 1);
	    (void) dup2(savcon, 2);

	    /*
	     * Now read from the second temporary file,
	     * close it, & we're done.
	     */
	    if ((fp = fopen(temp2, "r")) == NULL) {
		retval = FALSE;
	    } else {
		(void) (*readfunc)(fp);
		(void) fclose(fp);
		retval = TRUE;
	    }
	}
	/*
	 * Clean up.
	 */
	if (temp2) {
	    (void) remove(temp2);
	    free(temp2);
	}
	if (savcon > 2)
	    (void) close(savcon);
	if (fd1 > 2)
	    (void) close(fd1);
	if (fd2 > 2)
	    (void) close(fd2);
    }

    if (temp1) {
	(void) remove(temp1);
	free(temp1);
    }

    return(retval);
}

#if 0

/*
 * The following functions are untested because neither of us has
 * access to an MS-DOS compiler at the moment.
 */

/*
 * Expand environment variables in filename.
 */

#define VMETACHAR	'$'

static char *
vexpand(name)
    char		*name;
{
    static Flexbuf	b;
    register char	*p1, *p2;

    if ((p2 = strchr(name, VMETACHAR)) == (char *) NULL) {
	return name;
    }
    flexclear(&b);
    p1 = name;
    while (*p1) {
	register int	c;
	register char	*val;
	Flexbuf		vname;

	while (p1 < p2) {
	    (void) flexaddch(&b, *p1++);
	}
	flexnew(&vname);
	for (p2++; (c = *p2) != '\0' && (is_alnum(c) || c == '_'); p2++) {
	    (void) flexaddch(&vname, c);
	}
	if (!flexempty(&vname)
	    &&
	    (val = getenv(flexgetstr(&vname))) != (char *) NULL) {
	    while ((c = *val++) != '\0') {
		(void) flexaddch(&b, c);
	    }
	    p1 = p2;
	}
	flexdelete(&vname);
	if ((p2 = strchr(p1, VMETACHAR) == (char *) NULL) {
	    while ((c = *p1) != '\0') {
		(void) flexaddch(&b, c);
		p1++;
	    }
	}
    }
    return flexgetstr(&b);
}

static struct dstat	info;

char *
fexpand(name)
    char		*name;
{
    static Flexbuf	newname;
    char		*lastdelim;
    bool_t		has_meta;
    register char	*p;

    has_meta = FALSE;
    lastdelim = (char *) NULL;
    for (name = p = vexpand(name); *p; p++) {
	switch (*p) {
	    case ':':
	    case '\\':
	    case '/':
		lastdelim = p;
		/*
		 * DOS only understands metacharacters in the file's
		 * base name, not in any directory names.
		 */
		has_meta = FALSE;
		continue;
	    case '*':
	    case '?':
		has_meta = TRUE;
		continue;
	}
    }
    if (!has_meta) {
	return name;
    }
    if (statfirst(name, &info, dst_HIDDEN | dst_SYSTEM | dst_DIR) != 0) {
	return name;
    }
    flexclear(&newname);
    for (;;) {
	register int	c;

	if (lastdelim) {
	    for (p = name; p <= lastdelim;) {
		if (!flexaddch(&newname, *p++)) {
		    break;
		}
	    }
	}
	for (p = dst_BASENAME(info);
	     p < &dst_LAST(info) && (c = *p++) != '\0';) {
	    /*
	     * DOS gives us the file's base name in nasty CAPITAL
	     * LETTERS, so we convert them to lower-case here.
	     */
	    if (!flexaddch(&newname, is_upper(c) ? to_lower(c) : (c))) {
		break;
	    }
	}
	/*
	 * Note the order of events here; we only append a ' ' to
	 * newname if there's still at least one more file name to
	 * append after it.
	 */
	if (statnext(&info) != 0) {
	    break;
	}
	if (!flexaddch(&newname, ' ')) {
	    break;
	}
    }
    if (flexempty(&newname)) {
	return name;
    }
    return flexgetstr(&newname);
}

bool_t
exists(name)
    char * name;
{
    return statfirst(name, &info, dst_HIDDEN | dst_SYSTEM | dst_DIR) == 0;
}

bool_t
can_write(name)
    char * name;
{
    return statfirst(name, &info, dst_HIDDEN | dst_SYSTEM | dst_DIR) != 0 ||
	   (info.dst_mode &
	    (dst_READONLY | dst_HIDDEN | dst_SYSTEM | dst_DIR)) == 0;
}

#endif	/* 0 */
