/*
 *  portdos.c:  Portable calls for DOS
 *  ==================================
 *  (C)1996 by F.Jalvingh; Public domain.
 *  (C)1997 by Markus F.X.J. Oberhumer; Public domain.
 *
 *  $Header: /cvsroot/fte/fte/src/portdos.c,v 1.1.1.1 2000/01/30 17:23:45 captnmark Exp $
 *
 *  $Log: portdos.c,v $
 *  Revision 1.1.1.1  2000/01/30 17:23:45  captnmark
 *  initial import
 *
 */
#include    "port.h"
#include    <assert.h>
#include    <string.h>

#if defined(DOS) || defined(DOSP32)
#include    <dos.h>
#if defined(__DJGPP__)
#include    <pc.h>
#include    <dpmi.h>
#include    <go32.h>
#include    <sys/movedata.h>
#define inp	inportb
#define outp outportb
#endif

struct plScnInfo
{
	int     		s_inited;			// T when current status is known.
	int     		s_wid, s_hig;       // Current screen size
	enum ePlScnType	s_type;				// Current screen type,
	boolean			s_ismono;			// T when black- and white.
	boolean         s_isbright;
	
	//** Cursor info,
	boolean         s_curinit;
	boolean         s_curon;            // T when cursor is visible
	int             s_cur_x, s_cur_y;   // Current cursor position,
	int             s_curstart,         // Start of cursor blob,
					s_curend;           // End of blob;

	
    UWORD           s_iobase;

#if defined(__DOS4G__) || defined(__DJGPP__)
 	ULONG           s_pbase;            // Physical base address of video memory
#else
	UWORD           s_pseg;             // Segment of video memory
#endif
};


/*--------------------------------------------------------------------------*/
/*  STATIC GLOBALS..                                                        */
/*--------------------------------------------------------------------------*/
static struct plScnInfo     plScnI;
static void					plScnInit(void);


/*
 *  between() returns a # between bounds.
 */
static int between(int x, int low, int hig)
{
	if(x < low)
		return low;
	else if(x > hig)
		return hig;
	return x;
}


/****************************************************************************/
/*																			*/
/*  CODING: Screen type determination for DOS..                             */
/*																			*/
/****************************************************************************/
/*
 *  This is very important for the working of the DOS interface. It determines
 *  the screen type, and derives some stuff from that:
 *  - screen base address (i/o port)
 *  - video memory address (for direct I/O to the screen).
 */
/*
 *	VgaMonoMonitor() returns T if the monitor attached to a VGA adaptor is a
 *	monochrome monitor.
 */
static boolean VgaMonoMonitor(void)
{
	union dosxReg	r;
	
	memset(&r, 0, sizeof(r));			// Clear all unused regs,
	r.w.ax	= 0x1a00;					// Function 1A subfunction 00
	dosxIntr(0x10, &r); 				// Call VGA BIOS,
	if(r.h.al != 0x1a) return FALSE;	// Assume color if call not supported,
	if(r.h.bl == 0x01 || r.h.bl == 0x05 || r.h.bl == 0x07 ||r.h.bl == 0x0B)
		return TRUE;					// These types are MONO,
	else
		return FALSE;
}


/*
 *	biosisVGA() returns TRUE if a VGA-card is found.
 */
static boolean biosisVGA(void)
{
	boolean isvga;
	
	outp(0x3ce, 8); 				// Bit mask register, 
	outp(0x3cf, 1); 				// Test mask, (1) 
	outp(0x3ce, 8); 				// Bit mask register again, 
	if(inp(0x3cf) == 1) 			// Can we read the same value? 
		isvga	= TRUE; 			// Yes -> is VGA! 
	else
		isvga	= FALSE;
	outp(0x3ce, 8); 				// Bit mask register last time, 
	outp(0x3cf, 0xff);				// All on (default value) 
	return isvga;
}


/*
 *	biosisEGA() returns TRUE if the card is an EGA card (or a VGA card).
 */
static boolean biosisEGA(void)
{
	union dosxReg	r;
	UBYTE			ega;
	
	//** Get EGA BIOS byte.
	dosxMemRead(&ega, 0x00400087ul, 1); 		// Read EGA BIOS sysvar,
	if(! (ega & 0x08))							// There's an active EGA in the system! 
	{
		//** EGA is active: do EGA bios call.. 
		memset(&r, 0, sizeof(r));
		r.h.ah	= 0x12; 						// EGA BIOS Get info, 
		r.h.bl	= 0x10; 						// Get info 
		r.h.bh	= 0xff; 						// Inpossible return value, 
		dosxIntr(0x10, &r); 					// Call EGA BIOS, 
		if(r.h.bh != 0xff)						// EGA found? 
			return TRUE;						// Yes! 
	}
	return FALSE;
}


/*
 *	biosisMono() returns TRUE if the BIOS is currently in MONO mode. This
 *	is the mode selected with the MODE co80 or MODE MONO command. For EGA and
 *	VGA cards this mode doesn't tell a thing about the MONITOR attached!!!
 */
static boolean biosisMono(void)
{
	union dosxReg	r;
	
	//** Call the BIOS get video mode 
	memset(&r, 0, sizeof(r));
	r.w.ax	= 0x0f00;
	dosxIntr(0x10, &r);
	r.h.al &= 0x7f;								// Mask out DONT-CLEAR bit,
	if(r.h.al == 7) return TRUE;				// Monochrome screen
	if(r.h.al == 2 || r.h.al == 3) return FALSE;// Color screen,
	return TRUE;								// All else: return MONO
}


/*
 *  getPrimaryType() returns the primary card type.
 */
static enum ePlScnType getPrimaryType()
{
	if(biosisEGA())             // Minimal EGA?
		return biosisVGA() ? plsctVGA : plsctEGA;
	return biosisMono() ? plsctMono : plsctCGA;
}


/*
 *  getType() determines the screen type.
 */
static void getType(void)
{
	enum ePlScnType	sct;
	
	sct = getPrimaryType();                 // Get primary screen type;
	switch(sct)
	{
		default:		plScnI.s_ismono = biosisMono(); 	break;
		case plsctVGA:	plScnI.s_ismono = VgaMonoMonitor(); break;
	}

	//** Determine hardware addresses..
#if defined(__DJGPP__)
	plScnI.s_pbase  = ScreenPrimary;
#elif defined(__DOS4G__)
	plScnI.s_pbase  = plScnI.s_ismono ? 0xb0000 : 0xb8000;
#elif defined(__MSDOS__)
	plScnI.s_pseg   = plScnI.s_ismono ? 0xb000 : 0xb800;
#endif
	plScnI.s_iobase = plScnI.s_ismono ? 0x3b4 : 0x3d4;
    plScnI.s_type   = sct;
}


/****************************************************************************/
/*																			*/
/*  CODING: Screen - Other base helper calls..								*/
/*																			*/
/****************************************************************************/
/*
 *  getSize() inquires the BIOS about the screen size.
 */
static void getSize(void)
{
	union dosxReg	r;

	//** Get the current width of the screen,
	memset(&r, 0, sizeof(r));
	r.w.ax	= 0x0f00;					// Get current video mode,
	dosxIntr(0x10, &r);         		// Video BIOS
	plScnI.s_wid = between(r.h.ah, 80, 256);

	if(plScnI.s_type < plsctEGA)        // For all lower as EGA use 25 height
		plScnI.s_hig = 25;
	else
	{
		r.w.ax	= 0x1130;				// Get FONT information,
		r.w.bx	= 0;
		dosxIntr(0x10, &r);
		r.h.dl	+= 1;					// For some reason it reports 1 too small,
		plScnI.s_hig = between(r.h.dl, 25, 128);
	}
}


/*
 *  getCurInfo() gets the current cursor info: position and shape, visibility.
 */
static void getCurInfo(void)
{
    union dosxReg       r;
	
	//** Ask the BIOS for the current cursor shape,
	memset(&r, 0, sizeof(r));
	r.w.ax	= 0x0300;				// Get cursor shape,
	r.w.bx	= 0;					// Page 0
	dosxIntr(0x10, &r);
    plScnI.s_cur_x  = r.h.dl;
    plScnI.s_cur_y  = r.h.dh;
	plScnI.s_curstart   = r.h.ch;
	plScnI.s_curend = r.h.cl;
	if(! plScnI.s_curinit)          // Cursor state not initialized?
	{
		plScnI.s_curon  = TRUE;     // Default to cursor ON,
		plScnI.s_curinit= TRUE;
	}
}


/*
 *  plScnInit() is called to get most of the info required to do screen I/O.
 *  Depending on the "inited" flag it will get the current screen size, the
 *  mode, cursor shape and visibility etc.
 */
static void plScnInit(void)
{
	if(plScnI.s_inited) return;         // Exit immediately if already inited,
    getType();                          // ALWAYS get TYPE 1st!
	getSize();                          // Get current mode (screen size)
	getCurInfo();
	plScnI.s_inited = TRUE;             // Don't init again.
}


/*
 *  plScnSetFlash() is called to SET/CLEAR the flash/bright indicator.
 */
void plScnSetFlash(boolean on)
{
	union dosxReg       r;

	memset(&r, 0, sizeof(r));
	r.w.ax	= 0x1003;
	r.w.bx	= on ? 1 : 0;           // Set HILITE or FLASH mode.
	dosxIntr(0x10, &r);
	plScnI.s_isbright   = !on;      // Set ISBRIGHT mode flag.
}


#if defined(__DJGPP__)
static void ScreenSetCursorShape(int shape)
{
	__dpmi_regs reg;

	reg.h.ah = 0x01;
	reg.h.al = ScreenMode();
	reg.x.cx = shape & 0x7f1f;
	__dpmi_int(0x10, &reg);
}
#else
/*
 *  cardOut()..
 */
static void cardOut(int port, int val)
{
	outp(plScnI.s_iobase, port);
	outp(plScnI.s_iobase + 1, val);
}
#endif


/*
 *  plScnCursorOn() switches the cursor ON or OFF.
 */
void plScnCursorOn(boolean on)
{
    UWORD       v;

	plScnInit();

	plScnI.s_curon  = on;
	v	= (UWORD) ( ((plScnI.s_curstart & 0xff) << 8) | (plScnI.s_curend & 0xff));
	if(! on) v |= 0x2000;           // Disable if required..

#if defined(__DJGPP__)
	ScreenSetCursorShape(v);
#else
	dosxDisable();
	cardOut(10, v >> 8);
	cardOut(11, v & 0xff);

	if(! on)
	{
		cardOut(14, 20);
		cardOut(15, 0);
	}
	dosxEnable();
#endif
}


/*
 *  plScnCursorShape() sets the cursor shape.
 */
void plScnCursorShape(int start, int end)
{
	plScnInit();
	plScnI.s_curstart   = start;
	plScnI.s_curend   	= end;

	plScnCursorOn(plScnI.s_curon);          // Switch cursor on/off; sets shape.
}


/*
 *  plScnCursorShapeGet()
 */
void plScnCursorShapeGet(int* sp, int* ep)
{
	getCurInfo();
	*sp = plScnI.s_curstart;
	*ep = plScnI.s_curend;
}


/*
 *  plScnCursorPos() sets the cursor's position.
 */
void plScnCursorPos(int x, int y)
{
#if defined(__DJGPP__)
	ScreenSetCursor(y,x);
#else
	UWORD       soff;

	plScnInit();

	soff= (UWORD) (y * plScnI.s_wid + x);
	dosxDisable();
	cardOut(14, (soff >> 8));
	cardOut(15, soff & 0xff);
	dosxEnable();
	plScnI.s_cur_x  = x;
	plScnI.s_cur_y  = y;
#endif
}


/*
 *  plScnCursorPosGet()
 */
void plScnCursorPosGet(int* xp, int* yp)
{
//	getCurInfo();
	*xp = plScnI.s_cur_x;
	*yp = plScnI.s_cur_y;
}



/****************************************************************************/
/*																			*/
/*	DJGPP implementation. 													*/
/*																			*/
/****************************************************************************/
#if defined(__DJGPP__)              // Protected Mode

void plScnWrite(unsigned x, unsigned y, unsigned short* buf, unsigned nch)
{
	plScnInit();
 	movedata(_my_ds(), (unsigned) buf, _dos_ds, plScnI.s_pbase+(((y * plScnI.s_wid) + x)*2), nch*2);
}

void plScnRead(unsigned x, unsigned y, unsigned short* buf, unsigned nch)
{
	plScnInit();
	movedata(_dos_ds, plScnI.s_pbase+((y * plScnI.s_wid) + x)*2, _my_ds(), (unsigned) buf, nch*2);
}

static void moveVm(unsigned to, unsigned from, unsigned len)
{
	movedata(_dos_ds, plScnI.s_pbase + from*2, _dos_ds,  plScnI.s_pbase + to*2, len*2);
}


/****************************************************************************/
/*																			*/
/*	CODING: Dos4GW implementation.. 										*/
/*																			*/
/****************************************************************************/
#elif defined(__DOS4G__)

void plScnWrite(unsigned x, unsigned y, unsigned short* buf, unsigned nch)
{
    plScnInit();
	memcpy((UBYTE*) plScnI.s_pbase+(((y * plScnI.s_wid) + x)*2), buf, nch<<1);
}

void plScnRead(unsigned x, unsigned y, unsigned short* buf, unsigned nch)
{
	plScnInit();
	memcpy(buf, (UBYTE*)plScnI.s_pbase+(((y * plScnI.s_wid) + x)*2), nch<<1);
}

static void moveVm(unsigned to, unsigned from, unsigned len)
{
    memcpy((UBYTE*) plScnI.s_pbase+to*2, (UBYTE*) plScnI.s_pbase+from*2, len*2);
}


/****************************************************************************/
/*																			*/
/*	REAL MODE implementation. 												*/
/*																			*/
/****************************************************************************/

#elif defined(__MSDOS__)              // REAL Mode
#include	<dos.h>

void plScnWrite(unsigned x, unsigned y, unsigned short* buf, unsigned nch)
{
	plScnInit();
	memcpy(MK_FP(plScnI.s_pseg, ((y * plScnI.s_wid) + x)*2), buf, nch<<1);
}

void plScnRead(unsigned x, unsigned y, unsigned short* buf, unsigned nch)
{
	plScnInit();
	memcpy(buf, MK_FP(plScnI.s_pseg, ((y * plScnI.s_wid) + x)*2), nch<<1);
}

static void moveVm(unsigned to, unsigned from, unsigned len)
{
	memcpy(MK_FP(plScnI.s_pseg, to*2), MK_FP(plScnI.s_pseg, from*2), len << 1);
}

#endif


/****************************************************************************/
/*																			*/
/*  CODING: Screen - Shared Base functions.									*/
/*																			*/
/****************************************************************************/
/*
 *  plScnWidth() returns the width of the screen.
 */
int plScnWidth(void)
{
	plScnInit();
	return plScnI.s_wid;
}


/*
 *  plScnHeight() returns the current height of the screen.
 */
int plScnHeight(void)
{
	plScnInit();
	return plScnI.s_hig;
}


/*
 *  plScnType() returns the current screen type.
 */
enum ePlScnType plScnType(void)
{
	plScnInit();
	return plScnI.s_type;
}


/*
 *  plScnIsMono() returns T if the screen is a MONO screen.
 */
boolean plScnIsMono(void)
{
	plScnInit();
	return plScnI.s_ismono;
}


/****************************************************************************/
/*																			*/
/*  CODING: Screen - Set and Scroll functions.								*/
/*																			*/
/****************************************************************************/
#define BUFSZ       150

/*
 *  plScnSetCell()
 */
void plScnSetCell(unsigned x, unsigned y, unsigned wid, unsigned hig, UWORD cell)
{
	UWORD   	buf[BUFSZ], *p, *pend;
	unsigned    len, lleft, px;

	//** Fill buffer,
	len = wid;
	if(len > BUFSZ) len = BUFSZ;
	p   = buf;
	pend= p + len;
	while(p < pend) *p++ = cell;            // Set as much as needed,
	
	//** For all lines,
	while(hig-- > 0)
	{
		//** For the current line,
		lleft = wid;
		px    = x;
		while(lleft > 0)
		{
			len = lleft;                    // Determine #bytes from buf to use,
			if(len > BUFSZ) len = BUFSZ;
			plScnWrite(px, y, buf, len);
			px  += len;
			lleft -= len;
		}
		y++;
	}
}


/*
 *  plScnScrollUp() scrolls the region specified UP.
 */
void plScnScrollUp(int x, int y, int ex, int ey, int nlines, UWORD fill)
{
	int		from, to;

	//** Calculate the from and to offsets;
	to  = y*plScnI.s_wid + x;               // TO is top-of-window,
	from= to + nlines*plScnI.s_wid;         // FROM is #lines further down,
	while(y < ey-nlines)
	{
		moveVm(to, from, (ex-x));
		to  += plScnI.s_wid;
		from+= plScnI.s_wid;
		y++;
	}
    plScnSetCell(x, y, (ex-x), nlines, fill);	// Clear bottom.
}


/*
 *  plScnScrollDown() scrolls the region DOWN.
 */
void plScnScrollDown(int x, int y, int ex, int ey, int nlines, UWORD fill)
{
	int		from, to, len;

	//** Scrolling down: move from bottom to top;
	to  = ey * plScnI.s_wid + x;
	from= to - plScnI.s_wid*nlines;
	while(ey > y)
	{
		ey--;                               // Back one line;
		from    -= plScnI.s_wid;
		to    	-= plScnI.s_wid;
		moveVm(to, from, (ex-x));
	}
	plScnSetCell(x, y, (ex-x), nlines, fill);
}


/****************************************************************************/
/*																			*/
/*  CODING: Keyboard interface for DOS and DOS extender(s)..                */
/*																			*/
/****************************************************************************/
/*
 *  Memo to self: I've changed the "get character" request code from the 00h
 *  and 01h to 10h and 11h (extended keyboard). I should inquire the BIOS about
 *  the availability of these calls, but preliminary tests show that most
 *  BIOSes lie about the keyboard type, or do not implement the required
 *  int15 calls proper.
 *	Should work for most modern machines!
 */

/*
 *	kbHasChar() returns T if a character is present in the keyboard buffer. It
 *	is checked by comparing the keyboard queue ptrs in the bios segment.
 */
static boolean kbHasChar(void)
{
	UWORD			p1, p2;
	union dosxReg	r;

#if 0   // Switched off for extended kbd support.
	dosxMemRead(&p1, 0x0040001aul, 2);		// Get 1st keybd pointer,
	dosxMemRead(&p2, 0x0040001cul, 2);		// And 2nd,
	if(p1 == p2) return FALSE;				// Exit if not ok;
#endif

	//** Now ask if a real code is available by non-destructive qry of bios.
	memset(&r, 0, sizeof(r));				// Clear unused registers,
	r.w.ax	= 0x1100;                       // !! Was 0x0100
	dosxIntr(0x16, &r); 					// Call keyboard shit.
	if(r.w.flags & 0x40) return FALSE;      // Z flag SET -> no data
	return TRUE;
}


/*
 *	kbDosRead() does a destructive read by calling BIOS call 16H code 0.
 */
static boolean kbDosRead(struct plKbdInfo* ki)
{
	union dosxReg	r;
    UBYTE   v;

	memset(&r, 0, sizeof(r));					// Make sure all segregs are 0
	r.w.ax = 0x1000;                        // Was 0x0, now extended
	dosxIntr(0x16, &r);
	if((r.x.flags & 0x40) == 0) 				// Z flag NOT set?
	{
		ki->ki_scan = r.h.ah;
		ki->ki_ascii= r.h.al;
		dosxMemRead(&v, 0x00400017ul, 1);

		ki->ki_flags= (v & 0x03) ? PLKF_SHIFT : 0;
		if(v & 0x08) ki->ki_flags |= PLKF_ALT;
		if(v & 0x04) ki->ki_flags |= PLKF_CTRL;
		if(v & 0x10) ki->ki_flags |= PLKF_SCROLLLOCK;
		if(v & 0x20) ki->ki_flags |= PLKF_NUMLOCK;
		if(v & 0x40) ki->ki_flags |= PLKF_CAPSLOCK;
		
		return TRUE;
	}
	return FALSE;
}


/*
 *	kbDosIdle()
 */
static void kbDosIdle(void)
{
	union dosxReg	r;
	
	memset(&r, 0, sizeof(r));
	dosxIntr(0x28, &r);
}


/*
 *  plKbdReadF() does a non-waiting read of the keyboard.
 */
boolean plKbdReadF(struct plKbdInfo* ki)
{
	if(kbHasChar())
	{
		if(kbDosRead(ki))
			return TRUE;
	}
	kbDosIdle();
	return FALSE;
}


/****************************************************************************/
/*																			*/
/*  CODING: Mouse - Dos MOUSE interface (Mou calls).                        */
/*																			*/
/****************************************************************************/
/*--------------------------------------------------------------------------*/
/*	DEFINES:	Mouse command codes.										*/
/*--------------------------------------------------------------------------*/
#define MC_RESET		0x0000
#define MC_CURSEN		0x0001			/* Enable cursor position */
#define MC_CURSDI		0x0002			/* Disable cursor */
#define MC_GETCUR		0x0003			/* Get cursor position */
#define MC_SETCUR		0x0004			/* Set cursor position */
#define MC_GETBPRESS	0x0005			/* Get button press state */
#define MC_GETBRELEASE	0x0006			/* Button release state */
#define MC_SETXRANGE	0x0007			/* Set X cursor range */
#define MC_SETYRANGE	0x0008			/* Set Y cursor range */
#define MC_GRCURSOR 	0x0009			/* Set Graphic cursor style */
#define MC_TXCURSOR 	0x000a			/* Set text mode cursor style */
#define MC_GETMOTION	0x000b			/* Get mouse motion # */
#define MC_SETEVENT 	0x000c			/* DefiNE event handler location */
#define MC_LPENABLE 	0x000d			/* Enable light pen emulation */
#define MC_LPDISABLE	0x000e			/* Disable light pen emulation */
#define MC_SETSENS		0x000f			/* Set sensitivity (mousemove/pixel) */
#define MC_CDRANGE		0x0010			/* Disable cursor in given range */
#define MC_DBLSPEEDTH	0x0013			/* DefiNE double-speed treshold */


static ULONG			MouseCursorState;	/* Mouse saved cursor state */
static boolean          MousePresent;       // T if mouse was located at init.
static ULONG            MousePressCount;    // #times the mouse was clicked.

/*
 *  MOUSIsPresent() returns T if the mouse is present. It can be called only
 *  when the MOUSInit call has been done.
 */
boolean MOUSIsPresent(void)
{
	return MousePresent;
}


/*
 *  MOUSPressCount()
 */
ULONG MOUSPressCount(void)
{
	return MousePressCount;
}


/*
 *	MOUSInit() initializes the mouse driver. It returns FALSE if no mouse's
 *	present. When it's not called by the user program no mouse support
 *	is given..
 */
boolean MOUSInit(void)
{
//	UBYTE	 		*p;
	union dosxReg   r;

	if(! MousePresent)							/* If not already called */
	{
#if 0                                           // No vector for EXTENDERS
		/**** Get the contents of interrupt 33H ****/
		p = (UBYTE *)getvect(0x33);
		if(p == NULL || *p == 0xcf) return FALSE;
#endif

		/**** Mouse driver present: Call the mouse driver	****/
		memset(&r, 0, sizeof(r));
		r.w.ax	= MC_RESET;
		dosxIntr(0x33, &r);
		if(r.w.ax != 0) 							/* Mouse attached? */
		{
			MousePresent = TRUE;
			MOUSSetTextCursor(0, 0);				/* Use default text cursor */
		}
		MouseCursorState= 0;						/* Cursor invisible */
	}
	else
		assert(0);
	return MousePresent; 						/* Return mouse state */
}


/*
 *	MOUSExit() doesn't do a thing but is called to maintain compatibility.
 */
void MOUSExit(void)
{
	MousePresent = FALSE;
	MouseCursorState= 0;
}


/*
 *	MOUSCursen() enables/disables the mouse cursor. It takes notice of the
 *	previous state and calls the routine ONLY if the state needs a change.
 */
void MOUSCursen(boolean enable)
{
	union dosxReg	r;

	if(MousePresent)
	{
		if( (enable && !(MouseCursorState & 1)) ||
			(!enable && (MouseCursorState & 1)))
		{
			memset(&r, 0, sizeof(r));
			r.w.ax	= enable ? MC_CURSEN : MC_CURSDI;
			dosxIntr(0x33, &r);
			if(enable)
				MouseCursorState |= 1;
			else
				MouseCursorState &= ~1;
		}
	}
}


/*
 *	MOUSSaveCurs() saves the current cursor state.
 */
void MOUSSaveCurs(void)
{
	ULONG	state;

	state	= MouseCursorState & 1;
	MouseCursorState = (MouseCursorState << 1) | state;
}


/*
 *	MOUSRestCurs() restores the last cursor state.
 */
void MOUSRestCurs(void)
{
	ULONG   nstate;
	
	nstate  = MouseCursorState >> 1;            // Get new state of cursor,
	MOUSCursen((boolean) (nstate & 1) );     	// Make mouse's new state,
	MouseCursorState = nstate;                	// And set the new state.
}


/*
 *	MOUSPos() returns the current mouse position and the button states.
 */
void MOUSPos(UWORD *x, UWORD *y, boolean *leftbutton, boolean *rightbutton)
{
	union dosxReg	r;

	if(MousePresent)
	{
		memset(&r, 0, sizeof(r));
		r.w.ax	= MC_GETCUR;
		dosxIntr(0x33, &r);
		if(leftbutton != NULL) *leftbutton = (r.w.bx & 1) != 0;
		if(rightbutton != NULL) *rightbutton = (r.w.bx & 2) != 0;

		if(x != NULL) *x = r.w.cx;
		if(y != NULL) *y = r.w.dx;
	}
}


/*
 *	MOUSSetpos() sets a new mouse cursor position
 */
void MOUSSetpos(UWORD x, UWORD y)
{
	union dosxReg	r;

	if(MousePresent)
	{
		r.w.ax	= MC_SETCUR;
		r.w.cx	= x;
		r.w.dx	= y;
		dosxIntr(0x33, &r);
	}
}


/*
 *	MOUSSetTextCursor() sets the text cursor style. It always selects a
 *	software cursor and, if both screenmask and cursormask are zero it
 *	selects an inverse 'o' (0x09) as cursor character.
 */
void MOUSSetTextCursor(UWORD screenmask, UWORD cursormask)
{
	union dosxReg	r;

	if(MousePresent)
	{
		memset(&r, 0, sizeof(r));
		r.w.ax	= MC_TXCURSOR;			/* Define text cursor command */
		r.w.bx	= 0;					/* Software text cursor */
		if(screenmask == 0 && cursormask == 0)
		{
#if 0
			r.w.cx	= 0x0000;			/* Screen mask value: all bits OFF */
			r.w.dx	= 0x1f09;			/* Cursor mask: an 'o' */
#endif
			r.w.cx	= 0x7700;			/* Screen mask value */
			r.w.dx	= 0x77fe;			/* Cursor: a blob */

		}
		else
		{
			r.w.cx	= screenmask;
			r.w.dx	= cursormask;
		}
		dosxIntr(0x33, &r);
	}
}


/*
 *	MOUSPressed() returns TRUE when the mouse key was pressed AND:
 *	a.	The mouse key has been released since the last call OR
 *	b.	The mouse position has changed.
 *	It returns the new mouse coordinates in character positions.
 */
boolean MOUSPressed(UWORD *x, UWORD *y)
{
	boolean 		leftbutton;
	static boolean	waspressed;
	static UWORD	lx, ly;

	if(MousePresent)
	{
		MOUSPos(x, y, &leftbutton, NULL);		/* Get current mouse pos & state */
		*x	/= 8;								/* Scale to characters */
		*y	/= 8;								/* Idem */
		if(leftbutton)							/* Key pressed? */
		{
			/**** Key was pressed! Have the coords changed since the last	****/
			/**** time? 													****/
			if(*x == lx && *y == ly && waspressed) return FALSE;	/* Nothing changed */

			lx	= *x;
			ly	= *y;
			waspressed= TRUE;
			return TRUE;
		}
		else
			waspressed = FALSE;
	}
	return FALSE;
}



#endif          // DOS || DOSP32
