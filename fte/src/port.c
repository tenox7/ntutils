/*
 *  port.c:	portable versions of often-used calls
 *  =================================================
 *  (C)1996 by F.Jalvingh; Public domain.
 *  (C)1997 by Markus F.X.J. Oberhumer; Public domain.
 *
 *  $Header: /cvsroot/fte/fte/src/port.c,v 1.1.1.1 2000/01/30 17:23:32 captnmark Exp $
 *
 *  $Log: port.c,v $
 *  Revision 1.1.1.1  2000/01/30 17:23:32  captnmark
 *  initial import
 *
 */

#include    "port.h"

#ifndef MAXPATH
#	define	MAXPATH 	256
#endif


/****************************************************************************/
/*																			*/
/*  CODING: Windows NT portable calls.					    */
/*																			*/
/****************************************************************************/
#if defined(NT)


#define WIN32_LEAN_AND_MEAN
#include	<windows.h>

#include	<port/port.h>
#include	<string.h>
#include	<assert.h>


/*
 *	plGetcurdir() returns the current directory for the drive specified. Drive
 *	0=current, 1=a, 2=B etc.
 */
int plGetcurdir(int drive, char *dir)
{
	char	tmp[256], t2[6], org[6];
	int	rv;

	//** For NT: get current disk; then change to the requested one, and rest.
	rv	= -1;
	GetCurrentDirectory(sizeof(tmp), tmp);		// Get current disk & dir;
	assert(tmp[1] == ':');
	if(drive == 0)								// Is current drive?
	{
		strcpy(dir, tmp+3);					// Copy all after C:\ to dir,
		rv = 0;
	}
	else
	{
		strncpy(org, tmp, 2);					// Copy <drive>:
		org[2] = 0;

		//** Move to the required drive,
		*t2 = drive + 'A'-1;                    // Form drive letter.
		strcpy(t2+1, ":");
		if(! SetCurrentDirectory(t2)) return -1;// Try to move there;
		if(GetCurrentDirectory(sizeof(tmp), tmp))
		{
			strcpy(dir, tmp+3);				// Copy to result,
			rv = 0;
		}

		SetCurrentDirectory(org);				// Move back to original..
	}
	return rv;
}


int plGetdisk(void)
{
	char		buf[256];
	int		d;

	GetCurrentDirectory(sizeof(buf), buf);
	d	= toupper(*buf) - 'A';                  // 0=A, 1=B, etc.
	if(d < 0 || d > 25) return -1;
	return d;
}

int plSetdisk(int drive)
{
	char	buf[30];

	if( drive < 0 || drive > 25) return -1;
	*buf = drive + 'A';
	strcpy(buf+1, ":");
	return SetCurrentDirectory(buf) ? 0 : -1;
}


/****************************************************************************/
/*																			*/
/*  CODING: OS/2 portable calls.					    					*/
/*																			*/
/****************************************************************************/
#elif defined(OS2)
#define __32BIT__	1

#define INCL_BASE
#define INCL_DOSMISC
#define INCL_DOS
#define INCL_SUB
#define INCL_DOSERRORS
#define INCL_DOSFILEMGR
#define INCL_NOPMAPI
#include	<os2.h>


/*
 *  plGetcurdir() returns the current directory for the drive specified.
 */
int plGetcurdir(int drive, char *dir)
{
#ifdef __32BIT__
	ULONG	bytes;

	bytes	= 0;
	DosQueryCurrentDir(drive, NULL, &bytes);
	if(bytes > MAXPATH) return -1;
	DosQueryCurrentDir(drive, dir, &bytes);
	return 0;
#else
	USHORT	bytes;

	bytes	= 0;
	DosQCurDir(drive, NULL, &bytes);
	if(bytes > MAXPATH) return -1;
	DosQCurDir(drive, dir, &bytes);
	return 0;
#endif
}


/*
 *  plGetdisk() returns the current disk's number. A=0, B=1 etc.
 */
int plGetdisk(void)
{
#ifdef	__32BIT__
	ULONG	drive, numdrives;

	DosQueryCurrentDisk(&drive, &numdrives);
#else
	USHORT	drive;
	ULONG	numdrives;

	DosQCurDisk(&drive, &numdrives);
#endif
	return (int)drive - 1;

}


/*
 *  plSetdisk() makes another drive the current drive. Use A=0, B=1 etc. It
 *  returns 0 on succes.
 */
int plSetdisk(int drive)
{
#if defined(__32BIT__)
	return (int)DosSetDefaultDisk((ULONG)drive + 1);
#else
	return (int)DosSelectDisk((USHORT)drive + 1);  /* Bcc getdisk: 'A' = 0; MSC DosSelectdisk: 'A' = 1 */
#endif
}


/****************************************************************************/
/*																			*/
/*  CODING: DOS Extender code - basic functionality & interrupt calls.	   
*/ /*																			*/
/****************************************************************************/
#elif defined(DOSP32)

/*
 *  The following code is used to handle DOS-specific calls and memory from
 *  within several DOS extender(s), or from plain DOS. To compile for other
 *  DOS extenders simply recode the base calls; the extended calls are defined
 *  in terms of the base calls.
 *
 *  Currently the following implementations exist:
 *  -	Dos4GW i.c.m. the Watcom C compiler
 *  -	djgpp v2
 */

/*--------------------------------------------------------------------------*/
/*  CODING: 32-bits protected mode DOS with DOS4GW and Watcom C++.	    */
/*--------------------------------------------------------------------------*/
#if defined(__DOS4G__)		// Defined when compiled with Watcom for Dos4GW
#include	<i86.h>

#define HIWORD(x)		( (UWORD) ((x) >> 16))
#define LOWORD(x)		( (UWORD) (x))


/*
 *	dosxIntr() does a 16-bit interrupt with translation for the extender. It
 *	is used to call DOS and BIOS interrupts from within C.
 */
void dosxIntr(int inr, union dosxReg* r)
{
	union REGPACK	rp;

	//** Convert the parameters to a REGPACK structure,
	rp.x.eax = r->x.eax; rp.x.ebx = r->x.ebx;	rp.x.ecx = r->x.ecx;
	rp.x.edx = r->x.edx; rp.x.esi = r->x.esi;	rp.x.edi = r->x.edi;
    rp.x.ds = r->x.ds;	rp.x.es = r->x.es; rp.x.fs = r->x.fs; rp.x.gs = r->x.gs;
	rp.x.flags	= r->x.flags;
	intr(inr, &rp);

	//** And translate back..
	r->x.eax = rp.x.eax; r->x.ebx = rp.x.ebx; r->x.ecx = rp.x.ecx;
	r->x.edx = rp.x.edx; r->x.esi = rp.x.esi; r->x.edi = rp.x.edi;
    r->x.ds = rp.x.ds; r->x.es = rp.x.es; r->x.fs = rp.x.fs; r->x.gs = rp.x.gs;
	r->x.flags = rp.x.flags;
}


/*
 *	The Dos4GW extender maps the DOS memory arena (the 1st 1Mb of physical
 *	memory) to the 1st MB of extender memory. So, to access 0040:0087 in the
 *	BIOS data segment we use the linear address 0x487.
 *
 *	The memory routines translate the seg:off address passed to a linear
 *	address.
 */


/*
 *  SegToPhys() converts an address in seg:off format to a physical address.
 */
static void* SegToPhys(ULONG ra)
{
	return (void*) ( ((ULONG) HIWORD(ra) << 4) + (ULONG)LOWORD(ra)); }


/*
 *	dosxMemRead() reads the specified #bytes from the DOS real memory to the
 *	buffer specified. The address <ra> is specified as a seg:off address (even
 *  in extended mode); it is translated to the appropriate physical address.
 */
void dosxMemRead(void* dest, ULONG ra, unsigned nbytes)
{
	memcpy(dest, SegToPhys(ra), nbytes);
}


/*
 *	dosxMemWrite() writes the specified #bytes to DOS real memory. <ra> is in
 *  seg:off format.
 */
void dosxMemWrite(ULONG ra, void* src, unsigned nbytes)
{
	memcpy(SegToPhys(ra), src, nbytes);
}


/*
 *	dosxPMemRead() reads the specified #bytes from physical memory to the
 *	buffer specified. The physical address is the linear address, not a
 *	segmented one!
 */
void dosxPMemRead(void* dest, ULONG pa, unsigned nbytes)
{
	memcpy(dest, (void*) pa, nbytes);
}


/*
 *	dosxPMemWrite() writes the specified #bytes to physical memory.
 */
void dosxPMemWrite(ULONG pa, void* src, unsigned nbytes)
{
	memcpy((void*) pa, src, nbytes);
}


#elif defined(__DJGPP__)

#include <assert.h>
#include <dos.h>
#include <dpmi.h>
#include <go32.h>
#include <string.h>
#include <sys/movedata.h>

/*
 *	dosxIntr() does a 16-bit interrupt with translation for the extender. It
 *	is used to call DOS and BIOS interrupts from within C.
 */
void dosxIntr(int inr, union dosxReg* r)
{
	__dpmi_regs rp;

	//** Convert the parameters to a REGPACK structure,
	rp.d.eax = r->x.eax; rp.d.ebx = r->x.ebx;	rp.d.ecx = r->x.ecx;
	rp.d.edx = r->x.edx; rp.d.esi = r->x.esi;	rp.d.edi = r->x.edi;
    rp.x.ds = r->x.ds;	rp.x.es = r->x.es; rp.x.fs = r->x.fs; rp.x.gs = r->x.gs;
	rp.x.flags	= r->x.flags;
	__dpmi_int(inr, &rp);

	//** And translate back..
	r->x.eax = rp.d.eax; r->x.ebx = rp.d.ebx; r->x.ecx = rp.d.ecx;
	r->x.edx = rp.d.edx; r->x.esi = rp.d.esi; r->x.edi = rp.d.edi;
    r->x.ds = rp.x.ds; r->x.es = rp.x.es; r->x.fs = rp.x.fs; r->x.gs = rp.x.gs;
	r->x.flags = rp.x.flags;
}


/*
 *	dosxMemRead() reads the specified #bytes from the DOS real memory to the
 *	buffer specified. The address <ra> is specified as a seg:off address (even
 *  in extended mode); it is translated to the appropriate physical address.
 */
void dosxMemRead(void* dest, ULONG ra, unsigned nbytes)
{
	unsigned x = ((ra & 0xffff0000) >> 12) + (ra & 0xffff);
	movedata(_dos_ds, x, _my_ds(), (unsigned) dest, nbytes);
}


/*
 *	dosxMemWrite() writes the specified #bytes to DOS real memory. <ra> is in
 *  seg:off format.
 */
void dosxMemWrite(ULONG ra, void* src, unsigned nbytes)
{
	unsigned x = ((ra & 0xffff0000) >> 12) + (ra & 0xffff);
	movedata(_my_ds(), (unsigned) src, _dos_ds, x, nbytes);
}


/*
 *	dosxPMemRead() reads the specified #bytes from physical memory to the
 *	buffer specified. The physical address is the linear address, not a
 *	segmented one!
 */
void dosxPMemRead(void* dest, ULONG pa, unsigned nbytes)
{
	assert(0);	/* not used */
}


/*
 *	dosxPMemWrite() writes the specified #bytes to physical memory.
 */
void dosxPMemWrite(ULONG pa, void* src, unsigned nbytes)
{
	assert(0);	/* not used */
}



#else		// Not Dos4GW or djgpp
#	error	"Extender type not defined/illegal extender type."
#endif


/*
 *	dosxDisable() disables interrupts.
 */
void dosxDisable(void)
{
#if defined(__WATCOMC__) || defined(__MSC__)
	_disable();
#elif defined(__DJGPP__)
 	__asm__ __volatile__("cli \n");
#else
	disable();
#endif
}

/*
 *	dosxEnable() enables interrupts.
 */
void dosxEnable(void)
{
#if defined(__WATCOMC__) || defined(__MSC__)
	_enable();
#elif defined(__DJGPP__)
	__asm__ __volatile__("sti \n");
#else
	enable();
#endif
}


/****************************************************************************/
/*																			*/
/*  CODING: DOS-based portability calls.				    */
/*																			*/
/****************************************************************************/
#ifdef __WATCOMC__
#include    <i86.h>
#endif
#include	<dos.h>


/*
 *  plGetcurdir() returns the current directory for the drive specified. Drive
 *  0=a, 1=B etc.
 */
int plGetcurdir(int drive, char *dir)
{
	union dosxReg	r;

#if defined(__32BIT__) && !defined(__DJGPP__)
	void far	*ptr;
#endif

	memset(&r, 0, sizeof(r));
	r.w.ax = 0x4700;		// Get current directory
	r.h.dl	= drive;		// Get drive (0=current, 1=a etc)!!
#if defined(__DJGPP__)
	r.w.ds	= _my_ds();
	r.x.esi = (unsigned) dir;	// Pass the address of the buffer.
#elif defined(__32BIT__)
	ptr	= dir;			// Get full 16:32 address of dir,
	r.w.ds	= FP_SEG(ptr);
	r.x.esi = FP_OFF(dir);			// Pass the address of the buffer.
#else
	r.w.ds	= FP_SEG(dir);
	r.w.si	= FP_OFF(dir);
#endif
	dosxIntr(0x21, &r);
	return r.x.flags & 0x80 ? -1 : 0;   // Carry flag is set?
}


#if defined(__WATCOMC__) || defined(__DJGPP__)

int plSetdisk(int drive)
{
	unsigned		ndr;

	_dos_setdrive(drive +1, &ndr);
	return 0;
}

int plGetdisk(void)
{
	unsigned	drv;

	_dos_getdrive(&drv);
	return drv - 1;
}

#elif defined(__BORLANDC__)
int plSetdisk(int drive)
{
    return setdisk(drive);
}

int plGetdisk(void)
{
	return getdisk();
}

#else
#   error   "Undefined compiler for DOS/Extended DOS"
#endif



#else
#   error   "Platform not supported."
#endif


/****************************************************************************/
/*																			*/
/*  CODING: an implementation of fnmatch(), when needed...		    */
/*																			*/
/****************************************************************************/
#if (defined(NT) || defined(DOSP32)) && !defined(__DJGPP__)
/*
 *		Wildcard spec's:
 *		================
 *
 *		*	matches 0-n characters, so * only removes all.
 *		?	matches any character on that position.
 *		#c	matches 0-n occurences of the character c. Note: #? is the same
 *			as *.
 *		a+b Matches either a or b. a and b may be complex expressions.
 *		()	Keep expressions together,
 *
 *		Examples:
 *		---------
 *		*.(obj+lib+exe) 	Matches *.exe, *.lib and *.exe.
 *		t#a.*				Matches t.*, ta.* taa.* etc
 *
 *		Small grammar:
 *		==============
 *		pattern:
 */
#define ENDCH(x)		((x) == '+' || (x) == ')' || (x) == 0)

static int wcMatch(char **pval, char **pw);

/*
 *	Next() moves to the next <ch> in the pattern, taking care of ( and ).
 *	It returns a nonzero value in case of error (bad match).
 */
static int Next(char **pptr, char ch)
{
	int braces;

	braces	= 0;
	for(;;)
	{
		switch(**pptr)
		{
			case 0:
				if(braces == 0)
					return 0;				/* 0 always allowed if not in () */
				else
					return ')';             /* Missing close brace! */

			case '(':
				braces++;
				break;

			case ')':
				if(braces == 0)
				{
					if(ch == ')' || ch == '+')
						return 0;
					else
						return '(';         /* Missing open brace */
				}
				braces--;
				break;

			default:
				if(**pptr == ch && braces == 0) return 0;
		}
		*pptr	+= 1;
	}
}


/*
 *	wcOrs() traverses <pattern> + <pattern> + <pattern>..
 */
static int wcOrs(char **pval, char **pw)
{
	char	*vsave;
	int	res;

	for(;;) 						/* For each orred part, */
	{
		vsave = *pval;				/* Save current string, */
		res = wcMatch(pval, pw);	/* Match part, */
		if(res != 0) return res;	/* Exit if OK or error. */
		*pval = vsave;				/* Back to start of string to match */

		/**** Match not found. Find next ORred item,	****/
		res = Next(pw, '+');
		if(res > 0) return res;
		if(**pw == 0 || **pw == ')') return 0;    /* Exit if at eo$ */
		if(**pw != '+') return '+'; /* Expected | ?? */
		*pw += 1;
	}
}




/*
 *	wcMatch() compares the string to match in *pval with the wildcard
 *	part in *pw. It returns True as soon as a match is found, and False
 *	if a match could not be found.
 */
static int wcMatch(char **pval, char **pw)
{
	char	*vsave, *wsave;
	int	res;

	for(;;)
	{
		vsave	= *pval;					/* Saved string, */
		switch(*(*pw)++)
		{
			case '+':
			case ')':
			case 0:
				/**** End of wildcarded string. If string is at end too ****/
				/**** match is found... 								****/
				*pw -= 1;							/* Back to terminator, */
				return **pval == 0;

			case '?':
				/**** Match-anything..	****/
				if(**pval == 0) return FALSE;		/* Exhausted! No match */
				*pval	+= 1;						/* Match one character, */
				break;								/* And loop, */

			case '(':
				res = wcOrs(pval, pw);				/* OR sequence valid? */
				if(res == 1)						/* 1 -> Matched! */
				{
					/**** OR sequence valid! Move to closing ) ****/
					res = Next(pw, ')');            /* Move past OR-sequence */
					if(res) return res;			/* If error return, */
					if(**pw != ')') return ')';     /* Missing ')' !! */
					*pw += 1;						/* Past ), */
				}
				else
					return res;					/* Not found or Error!! */
				break;

			case '*':
				/**** 0-n (kleene closure) any-character matches.. ****/
				while(**pval != 0)				  /* While not at end of user $ */
				{
					/**** Compare current part of user string with what's   ****/
					/**** left of the wildcards...							****/
					wsave	= *pw;					/* Get current wildcard ptr, after * */
					vsave	= *pval;				/* And to current part of $ */
					res = wcMatch(pval, pw);		/* Check if $ matches, */
					if(res != 0) return res;		/* Exit if matches or error, */
					*pw = wsave;					/* Back wildcards, */
					*pval	= vsave+1;				/* Back user $ one pos further, */
				}
				return ENDCH(**pw);				/* No match. */

			default:
				if(toupper(**pval) != toupper(*(*pw - 1)))
				{
					*pw -= 1;
					return 0;
				}
				*pval += 1;
				break;
		}
	}
}


/*
 *	strCmpWild() returns -1 on wildcard error, 0 on match and 1 on no
match.
 */
int strCmpWild(char *val, char *wild)
{
	return wcOrs(&val, &wild);
}


/*
 *	fnmatch() does a wildcarded match.
 */
int fnmatch(char* pat, char* in, int vv)
{
	return strCmpWild(in, pat) == 1 ? 0 : 1;
}
#endif		    // fnmatch()



