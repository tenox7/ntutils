/*	  con_dosx.cpp
 *
 *	  Copyright (c) 1994-1996, Marko Macek
 *	  Free 1996 F.Jalvingh
 *
 *	  You may distribute under the terms of either the GNU General Public
 *	  License or the Artistic License, as specified in the README file.
 *
 */

// include

#include "sysdep.h"
#include "console.h"
#include "c_config.h"
#include "gui.h"
#include "s_string.h"

#include <process.h>
#include "port.h"                // DOS portability calls,

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#ifdef DJGPP
#include <sys/stat.h>
#endif

#define MAX_PIPES 4
#define PIPE_BUFLEN 4096

typedef struct {
	int used;
	int id;
//	int reading;
	int stopped;
//	TID tid;
//	HMTX Access;
//	HEV ResumeRead;
//	HEV NewData;
//	char *buffer;
//	int buflen;
//	int bufused;
//	int bufpos;
	EModel *notify;
//	char *Command;
//	int RetCode;
//	int DoTerm;
        FILE *fp;
} GPipe;

static GPipe Pipes[MAX_PIPES] = {
	{ 0 }, { 0 }, { 0 }, { 0 }
};



#if defined(__DJGPP__)

// adapted from djgpp library source - redirects stderr as well as stdout
// Original author: pacetti@fl-ngnet.army.mil

#include <io.h>
#include <errno.h>
#include <unistd.h>

/* hold file pointer, descriptor, command, mode, temporary file name,
   and the status of the command  */
struct pipe_list {
  FILE *fp;
  int fd;
  int exit_status;
  char *command, mode[10], temp_name[L_tmpnam];
  struct pipe_list *next;
};

/* static, global list pointer */
static struct pipe_list *pl = NULL;

static FILE *
xpopen (const char *cm, const char *md) /* program name, pipe mode */
{
  struct pipe_list *l1, *l2;

  /* make new node */
  if ((l1 = (struct pipe_list *) malloc (sizeof (struct pipe_list))) == NULL)
    return NULL;

  /* zero out elements to we'll get here */
  l1->fd = 0;
  l1->fp = NULL;
  l1->next = NULL;

  /* if empty list - just grab new node */
  if (!pl)
    pl = l1;
  else
  {
    /* otherwise, find last node in list */
    ++(l1->fd);
    l2 = pl;
    while (l2->next)
    {
      ++(l1->fd);
      l2 = l2->next;
    };
    /* add new node to list */
    l2->next = l1;
  }

  /* stick in elements we know already */
  l1->exit_status = -1;
  strcpy (l1->mode, md);
  if (tmpnam (l1->temp_name) == NULL)
    return NULL;

  /* if can save the program name, build temp file */
  if ((l1->command = (char *) malloc(strlen(cm)+1)))
  {
    strcpy(l1->command, cm);
    /* if caller wants to read */
    if (l1->mode[0] == 'r')
    {
#if 1
      int fd2 = -1;
#endif
      /* dup stdout */
      if ((l1->fd = dup (fileno (stdout))) == EOF)
	l1->fp = NULL;
      else if (!(l1->fp = freopen (l1->temp_name, "wb", stdout)))
	l1->fp = NULL;
#if 1
      /* dup stderr */
      else if ((fd2 = dup (STDERR_FILENO)) == EOF)
	l1->fp = NULL;
      /* redirect stderr to new stdout */
      else if (dup2(fileno(l1->fp),STDERR_FILENO) == EOF)
        l1->fp = NULL;
#endif
      else
	/* exec cmd */
	if ((l1->exit_status = system (cm)) == EOF)
	  l1->fp = NULL;
      /* reopen real stdout */
      if (dup2 (l1->fd, fileno (stdout)) == EOF)
	l1->fp = NULL;
      else
	/* open file for reader */
	l1->fp = fopen (l1->temp_name, l1->mode);
      close(l1->fd);
#if 1
      /* restore stderr */
      if (fd2 >= 0) {
        (void) dup2(fd2, STDERR_FILENO);
	(void) close(fd2);
      }
#endif
    }
    else
      /* if caller wants to write */
      if (l1->mode[0] == 'w')
        /* open temp file */
        l1->fp = fopen (l1->temp_name, l1->mode);
      else
        /* unknown mode */
        l1->fp = NULL;
  }
  return l1->fp;              /* return == NULL ? ERROR : OK */
}

static int
xpclose (FILE *pp)
{
  struct pipe_list *l1, *l2;    /* list pointers */
  int retval=0;			/* function return value */

  /* if pointer is first node */
  if (pl->fp == pp)
  {
    /* save node and take it out the list */
    l1 = pl;
    pl = l1->next;
  }
  else
    /* if more than one node in list */
    if (pl->next)
    {
      /* find right node */
      for (l2 = pl, l1 = pl->next; l1; l2 = l1, l1 = l2->next)
        if (l1->fp == pp)
          break;

      /* take node out of list */
      l2->next = l1->next;
    }
    else
      return -1;

  /* if FILE not in list - return error */
  if (l1->fp == pp)
  {
    /* close the (hopefully) popen()ed file */
    fclose (l1->fp);

    /* if pipe was opened to write */
    if (l1->mode[0] == 'w')
    {
      /* dup stdin */
      if ((l1->fd = dup (fileno (stdin))) == EOF)
	retval = -1;
      else
	/* open temp stdin */
	if (!(l1->fp = freopen (l1->temp_name, "rb", stdin)))
	  retval = -1;
	else
	  /* exec cmd */
          if ((retval = system (l1->command)) != EOF)
	  {
            /* reopen stdin */
	    if (dup2 (l1->fd, fileno (stdin)) == EOF)
	      retval = -1;
	  }
      close(l1->fd);
    }
    else
      /* if pipe was opened to read, return the exit status we saved */
      if (l1->mode[0] == 'r')
        retval = l1->exit_status;
      else
        /* invalid mode */
        retval = -1;
  }
  remove (l1->temp_name);       /* remove temporary file */
  free (l1->command);           /* dealloc memory */
  free (l1);                    /* dealloc memory */
  l1 = NULL;                    /* make pointer bogus */

  return retval;              /* retval==0 ? OK : ERROR */
}

#endif /* defined(__DJGPP__) */



static long MouseAutoDelay = 400;
static long MouseAutoRepeat = 5;
static long MouseMultiClick = 300;

static int Initialized = 0;
static int MousePresent = 0;
static int CursorVisible = 1; /* 1 means visible */
static int MouseVisible = 0; /* 0 means hidden */
static TEvent MouseEv = { evNone };
static TEvent EventBuf = { evNone };
//static HMOU MouseHandle = 0;
//static KBDINFO SaveKbdState;

// misc

static void DrawCursor(int Show) {
}

static void DrawMouse(int Show)
{
	if (!MousePresent) return;
	if (Show)
		MOUSCursen(TRUE);
	else
		MOUSCursen(FALSE);
}

// *INDENT-OFF*
static struct { // TransCharScan
    unsigned short CharScan;
    TKeyCode KeyCode;
} TransCharScan[] = {
    { 0x0100, kbEsc },           { 0x011B, kbEsc },
    { 0x1C0D, kbEnter },         { 0x1C0A, kbEnter },
    { 0x1C00, kbEnter },         { 0xE00D, kbEnter | kfGray },
    { 0xA600, kbEnter | kfGray },{ 0xE00A, kbEnter | kfGray },
    { 0x0E08, kbBackSp },        { 0x0E7F, kbBackSp },
    { 0x0E00, kbBackSp },        { 0x0F09, kbTab },
    { 0x9400, kbTab },           { 0xA500, kbTab },
    { 0x0F00, kbTab },           { 0x4E00, '+' | kfGray },
    { 0x9000, '+' | kfGray },    { 0x4E2B, '+' | kfGray },
    { 0x4A00, '-' | kfGray },    { 0x8E00, '-' | kfGray },
    { 0x4A2D, '-' | kfGray },    { 0x3700, '*' | kfGray },
    { 0x9600, '*' | kfGray },    { 0x372A, '*' | kfGray },
    { 0xE02F, '/' | kfGray },    { 0xA400, '/' | kfGray },
    { 0x9500, '/' | kfGray },    { 0x0300, 0 }
};

static struct { // TransScan
    int ScanCode;
    TKeyCode KeyCode;
} TransScan[] = {
    { 0x78, '1' }, { 0x79, '2' }, { 0x7A, '3' }, { 0x7B, '4' }, { 0x7C, '5' },
    { 0x7D, '6' }, { 0x7E, '7' }, { 0x7F, '8' }, { 0x80, '9' }, { 0x81, '0' },

    { 0x10, 'Q' }, { 0x11, 'W' }, { 0x12, 'E' }, { 0x13, 'R' }, { 0x14, 'T' },
    { 0x15, 'Y' }, { 0x16, 'U' }, { 0x17, 'I' }, { 0x18, 'O' }, { 0x19, 'P' },

    { 0x1E, 'A' }, { 0x1F, 'S' }, { 0x20, 'D' }, { 0x21, 'F' }, { 0x22, 'G' },
    { 0x23, 'H' }, { 0x24, 'J' }, { 0x25, 'K' }, { 0x26, 'L' }, { 0x2C, 'Z' },
    { 0x2D, 'X' }, { 0x2E, 'C' }, { 0x2F, 'V' }, { 0x30, 'B' }, { 0x31, 'N' },
    { 0x32, 'M' }, { 0x29, '`' }, { 0x82, '-' }, { 0x83, '=' }, { 0x2B, '\\' },
    { 0x1A, '[' }, { 0x1B, ']' }, { 0x27, ';' }, { 0x28, '\'' }, { 0x33, ',' },
    { 0x34, '.' }, { 0x35, '/' }, { 0x37, '*' }, { 0x4E, '+' }, { 0x4A, '-' },

    { 0x3B, kbF1    }, { 0x3C, kbF2     }, { 0x3D, kbF3    },
    { 0x3E, kbF4    }, { 0x3F, kbF5     }, { 0x40, kbF6    },
    { 0x41, kbF7    }, { 0x42, kbF8     }, { 0x43, kbF9    },
    { 0x44, kbF10   }, { 0x85, kbF11    }, { 0x86, kbF12   },

    { 0x54, kbF1    }, { 0x55, kbF2     }, { 0x56, kbF3    },
    { 0x57, kbF4    }, { 0x58, kbF5     }, { 0x59, kbF6    },
    { 0x5A, kbF7    }, { 0x5B, kbF8     }, { 0x5C, kbF9    },
    { 0x5D, kbF10   }, { 0x87, kbF11    }, { 0x88, kbF12   },

    { 0x5E, kbF1    }, { 0x5F, kbF2     }, { 0x60, kbF3    },
    { 0x61, kbF4    }, { 0x62, kbF5     }, { 0x63, kbF6    },
    { 0x64, kbF7    }, { 0x65, kbF8     }, { 0x66, kbF9    },
    { 0x67, kbF10   }, { 0x89, kbF11    }, { 0x8A, kbF12   },

    { 0x68, kbF1    }, { 0x69, kbF2     }, { 0x6A, kbF3    },
    { 0x6B, kbF4    }, { 0x6C, kbF5     }, { 0x6D, kbF6    },
    { 0x6E, kbF7    }, { 0x6F, kbF8     }, { 0x70, kbF9    },
    { 0x71, kbF10   }, { 0x8B, kbF11    }, { 0x8C, kbF12   },

    { 0x47, kbHome  }, { 0x48, kbUp     }, { 0x49, kbPgUp  },
    { 0x4B, kbLeft  }, { 0x4C, kbCenter }, { 0x4D, kbRight },
    { 0x4F, kbEnd   }, { 0x50, kbDown   }, { 0x51, kbPgDn  },
    { 0x52, kbIns   }, { 0x53, kbDel    }, { 0x77, kbHome  },

    { 0x8D, kbUp    }, { 0x84, kbPgUp   }, { 0x73, kbLeft  },
    { 0x74, kbRight }, { 0x75, kbEnd    }, { 0x91, kbDown  },
    { 0x76, kbPgDn  }, { 0x92, kbIns    }, { 0x93, kbDel   },

    { 0x97, kbHome  | kfGray }, { 0x98, kbUp    | kfGray },
    { 0x99, kbPgUp  | kfGray }, { 0xA1, kbPgDn  | kfGray },
    { 0x9B, kbLeft  | kfGray }, { 0x9D, kbRight | kfGray },
    { 0x9F, kbEnd   | kfGray }, { 0xA0, kbDown  | kfGray },
    { 0xA2, kbIns   | kfGray }, { 0xA3, kbDel   | kfGray }
};
// *INDENT-ON*

int ReadKbdEvent(TEvent *Event, int Wait)
{
	struct plKbdInfo	ki;

	UBYTE CharCode, ScanCode;
	ULONG KeyCode, KeyFlags;
	UWORD CharScan, Flags;
	static ULONG PrevFlags = 0;
	int I;
	const int Tcs = (int) (sizeof(TransCharScan)/sizeof(TransCharScan[0]));
	const int Ts = (int) (sizeof(TransScan)/sizeof(TransScan[0]));

	Event->What = evNone;
	if(! plKbdReadF(&ki)) return 0; 		// No data-> no read..
	Event->What = evKeyDown;

	CharCode = ki.ki_ascii;
	ScanCode = ki.ki_scan;
	CharScan = (UWORD)((((UWORD)ScanCode) << 8) | ((UWORD)CharCode));
	Flags = ki.ki_flags;
	KeyCode = 0;
	KeyFlags = 0;

	/*	 printf("Key: %X %X %X %X %X \n", (unsigned long) ki.bNlsShift, (unsigned long) ki.fbStatus, (unsigned long) Flags, (unsigned long) CharCode, (unsigned long) ScanCode);*/

	if ((Flags & PLKF_SHIFT) != 0) KeyFlags |= kfShift;
	if ((Flags & PLKF_CTRL) != 0) KeyFlags |= kfCtrl;

	/*	  cpCount = sizeof(cpList);*/
	/*	  rc = DosQueryCp(sizeof(cpList), cpList, &cpCount);  // get active code page*/
	if (CharCode != 0) {
		// if ((Flags & PLKF_ALT) != 0) KeyFlags |= kfAlt;
	} else {
		if((Flags & PLKF_ALT) != 0) KeyFlags |= kfAlt;
	}
	/*	  if (rc != 0) printf("rc = %d\n", rc);*/

	if (CharScan == 0) { /* shift/alt/ctrl/caps/scroll/num */

	} else if (ScanCode == 0) { /* alt numeric */
		KeyCode = CharCode;
		KeyFlags |= kfAltXXX;
	} else { /* now check special combinations */
		for (I = 0; I < Tcs; I++)
			if (TransCharScan[I].CharScan == CharScan) {
				KeyCode = TransCharScan[I].KeyCode;
				break;
			}
		if (KeyCode == 0) {
			if ((CharCode == 0) || (CharCode == 0xE0)) {
				if (CharCode == 0xE0)
					KeyFlags |= kfGray;
				for (I = 0; I < Ts; I++)
					if (TransScan[I].ScanCode == ScanCode) {
						KeyCode = TransScan[I].KeyCode;
						break;
					}
			} else {
				KeyCode = CharCode;
			}
		}
	}
	Event->Key.Code = KeyCode | KeyFlags;
	PrevFlags = Flags;
	return 1;
}

#define TM_DIFF(x,y) ((long)(((long)(x) < (long)(y)) ? ((long)(y) - (long)(x)) : ((long)(x) - (long)(y))))

int ReadMouseEvent(TEvent *Event, unsigned long EventMask)
{
	static unsigned short	PrevState = 0;
	static unsigned short	PrevButtons = 0;
	static TEvent			LastMouseEvent = { evNone };
	static ULONG			LastEventTime = 0;
	static ULONG			LastClick = 0;
	static ULONG			LastClickTime = 0;
	static ULONG			LastClickCount = 0;
//	MOUEVENTINFO mi;
	unsigned short			Buttons, State, Btn;
//	USHORT					fWait = MOU_NOWAIT;
//	MOUQUEINFO mq;
	ULONG					CurTime;
	UWORD					cx, cy;
	int 					butf;
	boolean 				rb, lb;
	static int				Scx = -1, Scy = -1;
	static boolean			Slb, Srb;

//	CurTime = plTmGet();            // 2do!
	Event->What = evNone;

	//** Distill an event from the current position & button state of the mouse.
	MOUSPos(&cx, &cy, &lb, &rb);
	cx	/= 8;
	cy	/= 8;
	if(Scx == -1)
	{
		Scx = cx;							// Force initial state to here.
		Scy = cy;
	}

	//** Assume something happened.. Set the mouse' position,
	butf	= (lb ? 0x01 : 0) | (rb ? 0x02 : 0);
	Event->Mouse.X	= cx;
	Event->Mouse.Y	= cy;
	Event->Mouse.Count = 1;

	//** 1. Has the buttons state changed?
	if(Slb != lb)
	{
		Event->What = lb ? evMouseDown : evMouseUp;
		Event->Mouse.Buttons	= 0x01; //lb ? 0x01 : 0;
		Slb = lb;
	}
	else if(Srb != rb)
	{
		Event->What = rb ? evMouseDown : evMouseUp;
		Event->Mouse.Buttons	= 0x02; //	rb ? 0x02 : 0;
		Srb = rb;
	}
	else if(cx != Scx || cy != Scy)
	{
		//** Mouse move.
		Event->What = evMouseMove;
		Event->Mouse.Buttons	= butf;
		Event->Mouse.Count = 0;
	}

	//** Any event?
	if(Event->What == evNone)
	{
		//** Handle repeat here!!
		return 0;
	}
//	printf("[ev=%d, b=%d] ", Event->What, Event->Mouse.Buttons);

	//** Something happened..
	Scx = cx;
	Scy = cy;
	return 1;
}


#if 0


	MouGetNumQueEl(&mq, MouseHandle);
	if (mq.cEvents == 0) {
		if (LastMouseEvent.What == evMouseAuto && (EventMask & evMouseAuto)) {
			if (TM_DIFF(CurTime, LastEventTime) >= MouseAutoRepeat) {
				*Event = LastMouseEvent;
				DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT, &LastEventTime, 4);
				return 1;
			}
		}
		if ((LastMouseEvent.What == evMouseDown || LastMouseEvent.What == evMouseMove)
			&&
			(LastMouseEvent.Mouse.Buttons)
			&& (EventMask & evMouseAuto))
		{
			if (TM_DIFF(CurTime, LastEventTime) >= MouseAutoDelay) {
				LastMouseEvent.What = evMouseAuto;
				*Event = LastMouseEvent;
				DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT, &LastEventTime, 4);
				return 1;
			}
		}
		return 0;
	}

	if (MouReadEventQue(&mi, &fWait, MouseHandle) != 0) return 0;
	Event->Mouse.X = mi.col;
	Event->Mouse.Y = mi.row;
	State = mi.fs;
	Btn = Buttons = ((State & (2 | 4))?1:0) |
		((State & (8 | 16))?2:0) |
		((State & (32 | 64))?4:0);
	if (Buttons != PrevButtons) {
		Buttons ^= PrevButtons;
		if (PrevButtons & Buttons)
			Event->What = evMouseUp;
		else
			Event->What = evMouseDown;
	} else
		Event->What = evMouseMove;
	Event->Mouse.Buttons = Buttons;
	Event->Mouse.Count = 1;
	PrevState = State;
	PrevButtons = Btn;

	if (Event->What == evMouseDown) {
		if (LastClickCount) {
			if (LastClick == Event->Mouse.Buttons) {
				if (TM_DIFF(CurTime, LastClickTime) <= MouseMultiClick) {
					Event->Mouse.Count = ++LastClickCount;
				} else {
					LastClickCount = 0;
				}
			} else {
				LastClick = 0;
				LastClickCount = 0;
				LastClickTime = 0;
			}
		}

		LastClick = Event->Mouse.Buttons;
		if (LastClickCount == 0)
			LastClickCount = 1;
		DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT, &LastClickTime, 4);
	}
	/*	  if (Event->What == evMouseMove) {
	 LastClick = 0;
	 LastClickCount = 0;
	 LastClickTime = 0;
	 }*/
	{
		KBDINFO ki;
		USHORT Flags;
		TKeyCode KeyFlags = 0;

                ki.cb = sizeof(ki);
		KbdGetStatus(&ki, 0);
		Flags = ki.fsState;

		if ((Flags & (LEFTSHIFT | RIGHTSHIFT)) != 0) KeyFlags |= kfShift;
		if ((Flags & (LEFTCONTROL | RIGHTCONTROL)) != 0) KeyFlags |= kfCtrl;
		if ((Flags & (LEFTALT | RIGHTALT)) != 0) KeyFlags |= kfAlt;

		Event->Mouse.KeyMask = KeyFlags;
	}

	LastMouseEvent = *Event;
	DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT, &LastEventTime, 4);
	return 1;
}

#endif

int ConClear()
{
	plScnSetCell(0, 0, plScnWidth(), plScnHeight(), 0x0720);
	return 1;
}

int ConPutBox(int X, int Y, int W, int H, PCell Cell) {
	int I;
	int MX, MY;
	int MouseHidden = 0;
	char *p = (char *) Cell;
	if (MouseVisible)
		ConQueryMousePos(&MX, &MY);

	for (I = 0; I < H; I++) {
		if (MouseVisible)
			if (Y + I == MY)
				if ((MX >= X) && (MX <= X + W)) {
					DrawMouse(0);
					MouseHidden = 1;
				}
		plScnWrite(X, Y+I, (UWORD *)p, W);

		if (MouseHidden) {
			DrawMouse(1);
			MouseHidden = 0;
		}
		p += W << 1;
	}

	return 1;
}

int ConGetBox(int X, int Y, int W, int H, PCell Cell) {
	int I;
	int MX, MY;
	int MouseHidden = 0;
//	USHORT WW = (U)(W << 1);
	char *p = (char *) Cell;

	if (MouseVisible)
		ConQueryMousePos(&MX, &MY);

	for (I = 0; I < H; I++) {
		if (MouseVisible)
			if (Y + I == MY)
				if (MX >= X && MX < X + W) {
					DrawMouse(0);
					MouseHidden = 1;
				}
		plScnRead(X, Y+I, (unsigned short*) p, W);

		if (MouseHidden) {
			DrawMouse(1);
			MouseHidden = 0;
		}
		p += W << 1;
	}

	return 1;
}

int ConPutLine(int X, int Y, int W, int H, PCell Cell)
{
	int I;
	int MX, MY;
	int MouseHidden = 0;
	char *p = (char *) Cell;
	if (MouseVisible)
		ConQueryMousePos(&MX, &MY);

	for (I = 0; I < H; I++) {
		if (MouseVisible)
			if (Y + I == MY)
				if (MX >= X && MX < X + W) {
					DrawMouse(0);
					MouseHidden = 1;
				}
		plScnWrite(X, Y+I, (UWORD *) p, W);

		if (MouseHidden) {
			DrawMouse(1);
			MouseHidden = 0;
		}
	}
	return 1;
}

int ConSetBox(int X, int Y, int W, int H, TCell Cell) {
	int I;
	int MX, MY;
	int MouseHidden = 0;
	char *p = (char *) &Cell;
	if (MouseVisible)
		ConQueryMousePos(&MX, &MY);

	for (I = 0; I < H; I++) {
		if (MouseVisible)
			if (Y + I == MY)
				if (MX >= X && MX < X + W) {
					DrawMouse(0);
					MouseHidden = 1;
				}
		plScnSetCell(X, Y+I, W, 1, (UWORD)Cell);
		//		VioWrtNCell(p, (USHORT)(W), (USHORT)(Y + I), (USHORT)X, 0);

		if (MouseHidden) {
			DrawMouse(1);
			MouseHidden = 0;
		}
	}

	return 1;
}

int ConScroll(int Way, int X, int Y, int W, int H, TAttr Fill, int Count)
{
	int MX, MY;
	int MouseHidden = 0;
	TCell FillCell = (TCell)(Fill << 8);

	if (MousePresent && MouseVisible)
	{
		ConQueryMousePos(&MX, &MY);
		if (MX >= X && MX < X + W && MY >= Y && MY < Y + H)
		{
			DrawMouse(0);
			MouseHidden = 1;
		}
	}

	switch (Way)
	{
		case csUp:	plScnScrollUp(X, Y, X+W, Y+H, Count, (UWORD)FillCell);	break;
		case csDown:plScnScrollDown(X, Y, X+W, Y+H, Count, (UWORD)FillCell);  break;
		case csLeft:
//			VioScrollLf((USHORT)Y, (USHORT)X, (USHORT)(Y + H - 1), (USHORT)(X + W - 1), (USHORT)Count, (PBYTE)&FillCell, 0);
			break;
		case csRight:
//			VioScrollRt((USHORT)Y, (USHORT)X, (USHORT)(Y + H - 1), (USHORT)(X + W - 1), (USHORT)Count, (PBYTE)&FillCell, 0);
			break;
	}
	if (MouseHidden)
		DrawMouse(1);

	return 1;
}

int ConSetSize(int X, int Y) {
	return 1;
}

int ConQuerySize(int *X, int *Y) {
	*X	= plScnWidth();
	*Y	= plScnHeight();

	return 1;
}

int ConSetCursorPos(int X, int Y) {
	plScnCursorPos(X, Y);

	return 1;
}

int ConQueryCursorPos(int *X, int *Y) {
	plScnCursorPosGet(X, Y);

	return 1;
}

int ConShowCursor() {
	CursorVisible = 1;
	plScnCursorOn(TRUE);

	return 1;
}

int ConHideCursor() {
	CursorVisible = 0;
	plScnCursorOn(FALSE);

	return 1;
}

int ConSetCursorSize(int Start, int End) {
	return 1;
}

//int ConSetMousePos(int X, int Y) {
//	return 0;
//}


/****************************************************************************/
/*																			*/
/*	CODING: Mouse stuff.													*/
/*																			*/
/****************************************************************************/


int ConQueryMousePos(int *X, int *Y) {
	UWORD	a, b;
	boolean lb, rb;

	if (!MOUSIsPresent())
	    return 0;

	MOUSPos(&a, &b, &lb, &rb);
	*X	= a / 8;
	*Y	= b / 8;
	return 1;
}

int ConShowMouse()
{
	MouseVisible = TRUE;
	if (!MOUSIsPresent())
	    return 0;

	MOUSCursen(TRUE);

	return 1;
}

int ConHideMouse()
{
	MouseVisible = FALSE;
	if (!MOUSIsPresent())
	    return 0;

	MOUSCursen(FALSE);

	return 1;
}

int ConMouseVisible() {
	return MouseVisible;
}

int ConQueryMouseButtons(int *ButtonCount) {
	if (ButtonCount != 0)
		*ButtonCount = 2;

	return 1;
}



int ConInit(int XSize, int YSize)
{
	if (Initialized)
		return 1;

	EventBuf.What = evNone;
	MousePresent = MOUSInit();
	ConContinue();
	Initialized = 1;

	return 1;
}

int ConDone()
{
	return ConSuspend();
}

int ConSuspend()
{
	plScnSetFlash(TRUE);		// Set "flash" mode, not bright mode
	ConHideMouse();
#if defined(SIGBREAK)
  	signal(SIGBREAK, SIG_DFL);
#endif
	signal(SIGINT, SIG_DFL);

	return 1;
}

int ConContinue()
{
#if defined(SIGBREAK)
  	signal(SIGBREAK, SIG_IGN);
#endif
	signal(SIGINT, SIG_IGN);
	plScnSetFlash(FALSE);		// Set "bright" mode, not flashing
	ConShowMouse();

	return 1;
}

int GetPipeEvent(TEvent *Event) {
#ifdef DJGPP
    int i;

    Event->What = evNone;
    for (i = 0; i < MAX_PIPES; i++) {
        if (Pipes[i].used == 0) continue;
        if (Pipes[i].notify == 0) continue;
        if (1) {
            //fprintf(stderr, "Pipe New Data: %d\n", i);
            Event->What = evNotify;
            Event->Msg.View = 0;
            Event->Msg.Model = Pipes[i].notify;
            Event->Msg.Command = cmPipeRead;
            Event->Msg.Param1 = i;
            return 1;
        }
    }
#endif
    return 0;
}

int ConGetEvent(TEventMask EventMask, TEvent *Event, int WaitTime, int Delete)
{
	if (EventBuf.What != evNone)
	{
		*Event = EventBuf;
		if (Delete) EventBuf.What = evNone;
		return 1;
	}
	if (MouseEv.What != evNone)
	{
		*Event = MouseEv;
		if (Delete) MouseEv.What = evNone;
		return 1;
	}
	EventBuf.What = evNone;
	Event->What = evNone;

	if (!(ReadKbdEvent(Event, WaitTime) == 1) && (EventMask & evKeyboard))
	{
		if (MousePresent
		    && (ReadMouseEvent(Event, EventMask) == 1)
		    && (EventMask & evMouse))
			;
		else if (GetPipeEvent(Event) != 1)
			return 0;
	}
	if (Event->What != evNone)
	{
		if (Event->What == evMouseMove)
		{
			while (ReadMouseEvent(&MouseEv, EventMask) == 1)
			{
				if (MouseEv.What == evMouseMove)
				{
					*Event = MouseEv;
					MouseEv.What = evNone;
				}
				else
					break;
			}
		}
		EventBuf = *Event;
		if (Delete)
			EventBuf.What = evNone;
		return 1;
	}
	return 0;
}

static PCell SavedScreen = 0;
static int SavedX, SavedY, SaveCursorPosX, SaveCursorPosY;

int SaveScreen() {
	if (SavedScreen)
		free(SavedScreen);

	ConQuerySize(&SavedX, &SavedY);

	SavedScreen = (PCell) malloc(SavedX * SavedY * sizeof(PCell));

	if (SavedScreen)
		ConGetBox(0, 0, SavedX, SavedY, SavedScreen);

	ConQueryCursorPos(&SaveCursorPosX, &SaveCursorPosY);

	return 1;
}

int RestoreScreen() {
	if (SavedScreen) {
		ConPutBox(0, 0, SavedX, SavedY, SavedScreen);
		ConSetCursorPos(SaveCursorPosX, SaveCursorPosY);
	}

	return 1;
}


GUI::GUI(int &argc, char **argv, int XSize, int YSize) {
	fArgc = argc;
	fArgv = argv;
	::ConInit(-1, -1);
	SaveScreen();
	::ConSetSize(XSize, YSize);
	gui = this;
#ifdef DJGPP
	// speed up directory access by turning off unused stat functionality
	_djstat_flags |= _STAT_INODE;
	_djstat_flags |= _STAT_EXEC_EXT;
	_djstat_flags |= _STAT_EXEC_MAGIC;
	_djstat_flags |= _STAT_DIRSIZE;
	_djstat_flags |= _STAT_ROOT_TIME;
	_djstat_flags |= _STAT_WRITEBIT;
#endif
}

GUI::~GUI() {
	RestoreScreen();
	::ConDone();

        if(SavedScreen)
        {
            free(SavedScreen);
            SavedScreen = 0;
        }

        gui = 0;
}

int GUI::ConSuspend() {
	RestoreScreen();
	return ::ConSuspend();
}

int GUI::ConContinue() {
	SaveScreen();
	return ::ConContinue();
}

int GUI::ShowEntryScreen() {
	TEvent E;

	ConHideMouse();
	RestoreScreen();
	do { gui->ConGetEvent(evKeyDown, &E, -1, 1, 0); } while (E.What != evKeyDown);
	ConShowMouse();
	if (frames)
		frames->Repaint();

	return 1;
}

//static int CreatePipeChild(PID &pid, HPIPE &hfPipe, char *Command) {
//	return 0;
//}

static void PipeThread(void *p) {
}

int GUI::OpenPipe(const char *Command, EModel *notify) {
#ifdef DJGPP
    int i;

    for (i = 0; i < MAX_PIPES; i++) {
        if (Pipes[i].used == 0) {
            Pipes[i].id = i;
            Pipes[i].notify = notify;
            Pipes[i].stopped = 1;

            Pipes[i].fp = xpopen(Command,"r");
            if (Pipes[i].fp == NULL)
                return 0;
            Pipes[i].used = 1;
            //fprintf(stderr, "Pipe Open: %d\n", i);
            return i;
        }
    }
#endif
    return -1;
}

int GUI::SetPipeView(int id, EModel *notify) {
#ifdef DJGPP
    if (id < 0 || id >= MAX_PIPES)
        return 0;
    if (Pipes[id].used == 0)
        return 0;
    //fprintf(stderr, "Pipe View: %d %08X\n", id, notify);
    Pipes[id].notify = notify;
#endif
    return 1;
}

ssize_t GUI::ReadPipe(int id, void *buffer, size_t len) {
#ifdef DJGPP
    ssize_t rc;
    if (id < 0 || id >= MAX_PIPES)
        return -1;
    if (Pipes[id].used == 0)
        return -1;
    //fprintf(stderr, "Pipe Read: Get %d %d\n", id, len);
    
    rc = fread(buffer,1,len,Pipes[id].fp);
    //fprintf(stderr, "Pipe Read: Got %d %d\n", id, rc);
    if (ferror(Pipes[id].fp)) {
        Pipes[id].stopped = 1;
        rc = -1;
    }
    return rc;
#else
    return -1;
#endif
}

int GUI::ClosePipe(int id) {
#ifdef DJGPP
    if (id < 0 || id >= MAX_PIPES)
        return 0;
    if (Pipes[id].used == 0)
        return 0;
    Pipes[id].used = 0;
    //fprintf(stderr, "Pipe Close: %d\n", id);
    return (xpclose(Pipes[id].fp) == 0);
#else
    return 0;
#endif
}

int GUI::RunProgram(int mode, char *Command) {
	int rc, W, H, W1, H1;

	ConQuerySize(&W, &H);
	ConHideMouse();
	ConSuspend();

	if (*Command == 0)	// empty string = shell
		Command = getenv(
						 "COMSPEC"
						);

	rc = system(Command);

	ConContinue();
	ConShowMouse();
	ConQuerySize(&W1, &H1);

	if (W != W1 || H != H1) {
		frames->Resize(W1, H1);
	}
	frames->Repaint();
	return rc;
}

int ConSetTitle(const char *Title, const char *STitle) {
	return 0;
}

int ConGetTitle(char *Title, size_t MaxLen, char *STitle, size_t SMaxLen) {
	strlcpy(Title, "FTE", MaxLen);
	strlcpy(STitle, "FTE", SMaxLen);
	return 0;
}

int ConCursorVisible() {
	return 0;
}

int ConPutEvent(const TEvent& Event)
{
	EventBuf = Event;
	return 0;
}

char ConGetDrawChar(unsigned int index) {
    static const char *tab = NULL;

    if (!tab)
        tab = GetGUICharacters ("DOS","\xDA\xBF\xC0\xD9\xC4\xB3\xC2\xC3\xB4\xC1\xC5\x1A\xFA\x04\xC4\x18\x19\B1\xB0\x1B\x1A");

    assert(index < strlen(tab) && strlen(tab) > 20);

    return tab[index];
}
