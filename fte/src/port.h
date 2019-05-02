#ifndef PORT_PORT_H
#define PORT_PORT_H

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef ULONG
#	define	ULONG	unsigned long
#endif

#ifndef UWORD
#	define	UWORD	unsigned short
#endif

#ifndef UBYTE
#	define	UBYTE	unsigned char
#endif

#ifndef boolean
#	define	boolean int
#endif

#ifndef FALSE
#	define	FALSE	0
#endif

#ifndef TRUE
#	define	TRUE	1
#endif



/*--------------------------------------------------------------------------*/
/*	STRUCTURES: dosxEreg is a union containing all extended registers..	*/
/*--------------------------------------------------------------------------*/
#if defined(__32BIT__)
 #define PORT_FILLER(a) unsigned short a;
#else
 #define PORT_FILLER(a)
#endif

struct dosxEreg {
	unsigned long	eax, ebx, ecx, edx, ebp, esi, edi;
	unsigned short	ds, es, fs, gs;
	unsigned long	flags;
};

struct dosxBreg {
	unsigned char al, ah;	PORT_FILLER(_1)
	unsigned char bl, bh;	PORT_FILLER(_2)
	unsigned char cl, ch;	PORT_FILLER(_3)
	unsigned char dl, dh;	PORT_FILLER(_4)
};

struct dosxWreg {
	unsigned short ax;	PORT_FILLER(_1)
	unsigned short bx;	PORT_FILLER(_2)
	unsigned short cx;	PORT_FILLER(_3)
	unsigned short dx;	PORT_FILLER(_4)
	unsigned short bp;	PORT_FILLER(_5)
	unsigned short si;	PORT_FILLER(_6)
	unsigned short di;	PORT_FILLER(_7)
	unsigned short ds;
	unsigned short es;
	unsigned short fs;
	unsigned short gs;
	unsigned long	flags;
};

/*--------------------------------------------------------------------------*/
/*	STRUCTURES: dosxReg contains all usable registers.						*/
/*--------------------------------------------------------------------------*/
union dosxReg {
	struct dosxBreg 	h;
	struct dosxWreg 	w;
	struct dosxEreg 	x;
};


/*--------------------------------------------------------------------------*/
/*	DEFINES:	PLRF_* defines are the processor flags. 					*/
/*--------------------------------------------------------------------------*/
#define PLCPUF_C	0x0001	// Carry flag
#define PLCPUF_P	0x0004	// Parity
#define PLCPUF_A	0x0010	// Auxiliary carry
#define PLCPUF_Z	0x0040	// Zero flag,
#define PLCPUF_S	0x0080	// Sign flag
#define PLCPUF_T	0x0100	// Trace flag
#define PLCPUF_I	0x0200	// Interrupt
#define PLCPUF_D	0x0400	// Direction
#define PLCPUF_O	0x0800	// Overflow.


/*--------------------------------------------------------------------------*/
/*	DEFINES:	plFnsplit() defines.										*/
/*--------------------------------------------------------------------------*/
#define PL_WILDCARDS	0x01
#define PL_EXTENSION	0x02
#define PL_FILENAME	0x04
#define PL_DIRECTORY	0x08
#define PL_DRIVE		0x10

/*--------------------------------------------------------------------------*/
/*	DEFINES:	File attributes.											*/
/*--------------------------------------------------------------------------*/
#define PLFA_NORMAL	0x00		/* Normal file, no attributes */
#define PLFA_RDONLY	0x01		/* Read only attribute */
#define PLFA_HIDDEN	0x02		/* Hidden file */
#define PLFA_SYSTEM	0x04		/* System file */
#define PLFA_LABEL		0x08		/* Volume label */
#define PLFA_DIREC		0x10		/* Directory */
#define PLFA_ARCH		0x20		/* Archive */


/*--------------------------------------------------------------------------*/
/*	STRUCTURES: plFTime holds date/time for a file (plGetftime()).			*/
/*--------------------------------------------------------------------------*/
struct plFTime
{
	unsigned	ft_tsec  : 5;	// Two second interval
	unsigned	ft_min	 : 6;	// Minute
	unsigned	ft_hour  : 5;	// Hour
	unsigned	ft_day	 : 5;	// Day
	unsigned	ft_month : 4;	// Month
	unsigned	ft_year  : 7;	// Year
};


/*--------------------------------------------------------------------------*/
/*	STRUCTURES: plDFree holds "disk free space" info for plGetdfree().     
*/
/*--------------------------------------------------------------------------*/
struct plDFree
{
	unsigned	df_avail;
	unsigned	df_total;
	unsigned	df_bsec;
	unsigned	df_sclus;
};


/*--------------------------------------------------------------------------*/
/*	DEFINES:	plLocking() mode flags...									*/
/*--------------------------------------------------------------------------*/
#define PL_LK_UNLCK	0		// unlock the file region
#define PL_LK_LOCK		1		// lock the file region
#define PL_LK_NBLCK	2		// non-blocking lock
#define PL_LK_RLCK		3		// lock for writing
#define PL_LK_NBRLCK	4		// non-blocking lock for writing



//** Hardware interface: DOS & extended DOS only!!
void	dosxIntr(int inr, union dosxReg* r);

void	dosxMemRead(void* dest, ULONG ra, unsigned nbytes);
void	dosxMemWrite(ULONG ra, void* src, unsigned nbytes);
void	dosxPMemRead(void* dest, ULONG pa, unsigned nbytes);
void	dosxPMemWrite(ULONG pa, void* src, unsigned nbytes);

void	dosxDisable(void);
void	dosxEnable(void);

//** Disk stuff..
int	plGetdisk(void);
int	plSetdisk(int dnr);
int	plGetcurdir(int drive, char *dir);

//** Misc..
char	*plSearchpath(char *name);
int	plFnsplit(const char *pathp, char *drivep, char *dirp, char *namep,
char *extp); void	plFnmerge(char *out, char *drive, char *dir, char *name,
char *ext);


/****************************************************************************/
/*																			*/
/*	Hardware portability calls.											*/
/*																			*/
/****************************************************************************/

//** Mouse calls
//** ===========
boolean MOUSInit(void);
void	MOUSExit(void);
void	MOUSCursen(boolean enable);
void	MOUSPos(UWORD *x, UWORD *y, boolean *leftbutton, boolean
*rightbutton); void	MOUSSetpos(UWORD x, UWORD y);
void	MOUSSetTextCursor(UWORD screenmask, UWORD cursormask);
void	MOUSSaveCurs(void);
void	MOUSRestCurs(void);
boolean MOUSIsPresent(void);
ULONG	MOUSPressCount(void);

//** Timer calls
//** ===========
int			plTiAlloc(unsigned long cnt);
void			plTiSet(int t, unsigned long cnt);
void			plTiFree(int t);
unsigned long	plTiQValue(int t);

//** Thread/task based.
//** ==================
void			plDelay(unsigned int msecs);
unsigned long	plTmGet(void);					// Avoid using this.
void			plCpuRelease(void);


//** Thread interface!
//** =================
int plThrdStart(void (*exfn)(void*), unsigned long stksz, void* args);


//** FTE specific.
int fnmatch(char* pat, char* in, int vv);


#ifdef DOSP32

/*--------------------------------------------------------------------------*/
/*  ENUM:   ePlScnType defines a screen type.				    */
/*--------------------------------------------------------------------------*/
enum ePlScnType
{
	plsctUnknown,
	plsctMono,		    // Monochrome adapter / unknown type,
	plsctCGA,		    // Color graphics adapter
	plsctEGA,		    // EGA adapter,
	plsctVGA,		    // VGA adapter,
	plsctLast
};


//** Informational functions,
enum ePlScnType plScnType(void);
boolean 		plScnIsMono(void);
int			plScnWidth(void);
int			plScnHeight(void);
void			plScnCursorShapeGet(int* sp, int* ep);
void			plScnCursorPosGet(int* xp, int* yp);


//** Change parameters e.a.
void	plScnSetFlash(boolean on);
void	plScnCursorOn(boolean on);
void	plScnCursorShape(int start, int end);
void	plScnCursorPos(int x, int y);


//** Writing the screen.
void	plScnWrite(unsigned x, unsigned y, unsigned short* buf, unsigned nch);
void	plScnRead(unsigned x, unsigned y, unsigned short* buf, unsigned nch);
void	plScnSetCell(unsigned x, unsigned y, unsigned wid, unsigned hig, UWORD cell);
void	plScnScrollDown(int x, int y, int ex, int ey, int nlines, UWORD fill);
void	plScnScrollUp(int x, int y, int ex, int ey, int nlines, UWORD fill);


/****************************************************************************/
/*																			*/
/*  Keyboard interface..						    */
/*																			*/
/****************************************************************************/
struct plKbdInfo
{
	UBYTE	ki_scan;
	UBYTE	ki_ascii;
	ULONG	ki_flags;	    // PLKF_ defines, above;
};

#define PLKF_SHIFT		0x0001
#define PLKF_CTRL		0x0002
#define PLKF_ALT		0x0004
#define PLKF_SCROLLLOCK 	0x0008
#define PLKF_NUMLOCK		0x0010
#define PLKF_CAPSLOCK		0x0020


boolean plKbdReadF(struct plKbdInfo* ki);

#endif // DOSP32

#ifdef	__cplusplus
};
#endif

#endif // PORT_PORT_H
