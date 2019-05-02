/*    con_os2.cpp
 *
 *    Copyright (c) 1994-1998, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

// include

#include "console.h"
#include "gui.h"
#include "sysdep.h"

#include <process.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef OS2_INCLUDED
#include <os2.h>
#endif

extern int ShowVScroll;

//#define INCL_NOPM
//#define INCL_WINSWITCHLIST
#define INCL_WIN
#define INCL_SUB
#define INCL_KBD
#define INCL_VIO
#define INCL_MOU
#define INCL_BASE
#define INCL_DOS
#define INCL_DOSDEVIOCTL


#define MAX_PIPES 4
#define PIPE_BUFLEN 4096

typedef struct {
    int used;
    int id;
    int reading, stopped;
    TID tid;
    HMTX Access;
    HEV ResumeRead;
    HEV NewData;
    char *buffer;
    int buflen;
    int bufused;
    int bufpos;
    EModel *notify;
    char *Command;
    int RetCode;
    int DoTerm;
} GPipe;

static GPipe Pipes[MAX_PIPES] = {
    { 0 }, { 0 }, { 0 }, { 0 }
};

static long MouseAutoDelay = 400;
static long MouseAutoRepeat = 5;
static long MouseMultiClick = 300;

static int Initialized = 0;
static int MousePresent = 0;
static int CursorVisible = 1; /* 1 means visible */
static int MouseVisible = 0; /* 0 means hidden */
static TEvent MouseEv = { evNone };
static TEvent EventBuf = { evNone };
static HMOU MouseHandle = 0;
static KBDINFO SaveKbdState;

// misc

static void DrawCursor(int Show) {
    VIOCURSORINFO vci;
    VioGetCurType(&vci, 0);
    if (Show == 1)
        vci.attr = 1;
    else
        vci.attr = (SHORT)-1;
    VioSetCurType(&vci, 0);
}

static void DrawMouse(int Show) {
    if (!MousePresent) return;
    if (Show == 1) {
        MouDrawPtr(MouseHandle);
    } else {
        NOPTRRECT npr;
        int W, H;

        npr.row = 0;
        npr.col = 0;
        ConQuerySize(&W, &H);
        npr.cCol = (USHORT) (W - 1);
        npr.cRow = (USHORT) (H - 1);
        MouRemovePtr(&npr, MouseHandle);
    }
}

// *INDENT-OFF*
static struct { // TransCharScan
    USHORT CharScan;
    TKeyCode KeyCode;
} TransCharScan[] = {
    { 0x0100, kbEsc },                     { 0x011B, kbEsc },
    { 0x1C0D, kbEnter },                   { 0x1C0A, kbEnter },
    { 0x1C00, kbEnter },                   { 0xE00D, kbEnter | kfGray },
    { 0xA600, kbEnter | kfGray },          { 0xE00A, kbEnter | kfGray },
    { 0x0E08, kbBackSp },                  { 0x0E7F, kbBackSp },
    { 0x0E00, kbBackSp },                  { 0x0F09, kbTab },
    { 0x9400, kbTab },                     { 0xA500, kbTab },
    { 0x0F00, kbTab },                     { 0x4E00, '+' | kfGray },
    { 0x9000, '+' | kfGray },              { 0x4E2B, '+' | kfGray },
    { 0x4A00, '-' | kfGray },              { 0x8E00, '-' | kfGray },
    { 0x4A2D, '-' | kfGray },              { 0x3700, '*' | kfGray },
    { 0x9600, '*' | kfGray },              { 0x372A, '*' | kfGray },
    { 0xE02F, '/' | kfGray },              { 0xA400, '/' | kfGray },
    { 0x9500, '/' | kfGray },              { 0x0300, 0 }
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
    { 0x23, 'H' }, { 0x24, 'J' }, { 0x25, 'K' }, { 0x26, 'L' },

    { 0x2C, 'Z' }, { 0x2D, 'X' }, { 0x2E, 'C' }, { 0x2F, 'V' }, { 0x30, 'B' },
    { 0x31, 'N' }, { 0x32, 'M' },

    { 0x29, '`' }, { 0x82, '-' }, { 0x83, '=' }, { 0x2B, '\\' }, { 0x1A, '[' },
    { 0x1B, ']' }, { 0x27, ';' }, { 0x28, '\'' }, { 0x33, ',' }, { 0x34, '.' },
    { 0x35, '/' }, { 0x37, '*' }, { 0x4E, '+' }, { 0x4A, '-' },

    { 0x3B, kbF1    },  { 0x3C, kbF2    },  { 0x3D, kbF3    },
    { 0x3E, kbF4    },  { 0x3F, kbF5    },  { 0x40, kbF6    },
    { 0x41, kbF7    },  { 0x42, kbF8    },  { 0x43, kbF9    },
    { 0x44, kbF10   },  { 0x85, kbF11   },  { 0x86, kbF12   },

    { 0x54, kbF1    },  { 0x55, kbF2    },  { 0x56, kbF3    },
    { 0x57, kbF4    },  { 0x58, kbF5    },  { 0x59, kbF6    },
    { 0x5A, kbF7    },  { 0x5B, kbF8    },  { 0x5C, kbF9    },
    { 0x5D, kbF10   },  { 0x87, kbF11   },  { 0x88, kbF12   },

    { 0x5E, kbF1    },  { 0x5F, kbF2    },  { 0x60, kbF3    },
    { 0x61, kbF4    },  { 0x62, kbF5    },  { 0x63, kbF6    },
    { 0x64, kbF7    },  { 0x65, kbF8    },  { 0x66, kbF9    },
    { 0x67, kbF10   },  { 0x89, kbF11   },  { 0x8A, kbF12   },

    { 0x68, kbF1    },  { 0x69, kbF2    },  { 0x6A, kbF3    },
    { 0x6B, kbF4    },  { 0x6C, kbF5    },  { 0x6D, kbF6    },
    { 0x6E, kbF7    },  { 0x6F, kbF8    },  { 0x70, kbF9    },
    { 0x71, kbF10   },  { 0x8B, kbF11   },  { 0x8C, kbF12   },

    { 0x47, kbHome  },  { 0x48, kbUp    },  { 0x49, kbPgUp  },
    { 0x4B, kbLeft  },  { 0x4C, kbCenter},  { 0x4D, kbRight },
    { 0x4F, kbEnd   },  { 0x50, kbDown  },  { 0x51, kbPgDn  },
    { 0x52, kbIns   },  { 0x53, kbDel   },

    { 0x77, kbHome  },  { 0x8D, kbUp    },  { 0x84, kbPgUp  },
    { 0x73, kbLeft  },                      { 0x74, kbRight },
    { 0x75, kbEnd   },  { 0x91, kbDown  },  { 0x76, kbPgDn  },
    { 0x92, kbIns   },  { 0x93, kbDel   },

    { 0x97, kbHome  | kfGray },  { 0x98, kbUp    | kfGray },  { 0x99, kbPgUp  | kfGray },
    { 0x9B, kbLeft  | kfGray },                               { 0x9D, kbRight | kfGray },
    { 0x9F, kbEnd   | kfGray },  { 0xA0, kbDown  | kfGray },  { 0xA1, kbPgDn  | kfGray },
    { 0xA2, kbIns   | kfGray },  { 0xA3, kbDel   | kfGray }
};
// *INDENT-ON*

int ReadKbdEvent(TEvent *Event, int Wait) {
    KBDKEYINFO ki;
    UCHAR CharCode, ScanCode;
    ULONG KeyCode, KeyFlags;
    USHORT CharScan, Flags;
    static USHORT PrevFlags = 0;
    unsigned int I;

    Event->What = evNone;
    KbdCharIn(&ki, IO_NOWAIT, 0);
    if (!(ki.fbStatus & 0x40)) return 0;

    Event->What = evKeyDown;

    CharCode = ki.chChar;
    ScanCode = ki.chScan;
    CharScan = (USHORT)((((USHORT)ScanCode) << 8) | ((USHORT)CharCode));
    Flags = ki.fsState;
    KeyCode = 0;
    KeyFlags = 0;

/*   printf("Key: %X %X %X %X %X \n", (unsigned long) ki.bNlsShift, (unsigned long) ki.fbStatus, (unsigned long) Flags, (unsigned long) CharCode, (unsigned long) ScanCode);*/

    if ((Flags & (LEFTSHIFT | RIGHTSHIFT)) != 0) KeyFlags |= kfShift;
    if ((Flags & (LEFTCONTROL | RIGHTCONTROL)) != 0) KeyFlags |= kfCtrl;

/*    cpCount = sizeof(cpList);*/
/*    rc = DosQueryCp(sizeof(cpList), cpList, &cpCount);  // get active code page*/
    if (CharCode != 0) {
        if ((Flags & (LEFTALT)) != 0) KeyFlags |= kfAlt;
    } else {
        if ((Flags & (LEFTALT | RIGHTALT)) != 0) KeyFlags |= kfAlt;
    }
/*    if (rc != 0) printf("rc = %d\n", rc);*/

    if (CharScan == 0) { /* shift/alt/ctrl/caps/scroll/num */

    } else if (ScanCode == 0) { /* alt numeric */
        KeyCode = CharCode;
        KeyFlags |= kfAltXXX;
    } else { /* now check special combinations */
        for (I = 0; I < sizeof(TransCharScan)/sizeof(TransCharScan[0]); I++)
            if (TransCharScan[I].CharScan == CharScan) {
                KeyCode = TransCharScan[I].KeyCode;
                break;
            }
        if (KeyCode == 0) {
            if ((CharCode == 0) || (CharCode == 0xE0)) {
                if (CharCode == 0xE0)
                    KeyFlags |= kfGray;
                for (I = 0; I < sizeof(TransScan)/sizeof(TransScan[0]); I++)
                    if (TransScan[I].ScanCode == ScanCode) {
                        KeyCode = TransScan[I].KeyCode;
                        break;
                    }
            } else {
                if (CharCode < 32)
                    if (KeyFlags & kfCtrl)
                        CharCode += 64;
                KeyCode = CharCode;
            }
        }
    }
    Event->Key.Code = KeyCode | KeyFlags;
    PrevFlags = Flags;
    return 1;
}

#define TM_DIFF(x,y) ((long)(((long)(x) < (long)(y)) ? ((long)(y) - (long)(x)) : ((long)(x) - (long)(y))))

int ReadMouseEvent(TEvent *Event, ULONG EventMask) {
    static unsigned short PrevState = 0;
    static unsigned short PrevButtons = 0;
    static TEvent LastMouseEvent = { evNone };
    static ULONG LastEventTime = 0;
    static ULONG LastClick = 0;
    static ULONG LastClickTime = 0;
    static ULONG LastClickCount = 0;
    MOUEVENTINFO mi;
    unsigned short Buttons, State, Btn;
    USHORT fWait = MOU_NOWAIT;
    MOUQUEINFO mq;
    ULONG CurTime;

    DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT, &CurTime, 4);

    Event->What = evNone;
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
    } else {
        Event->What = evMouseMove;
        if (Event->Mouse.X == LastMouseEvent.Mouse.X &&
            Event->Mouse.Y == LastMouseEvent.Mouse.Y)
            return 0;
    }
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
/*    if (Event->What == evMouseMove) {
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


int ConClear() {
    int W, H;
    TDrawBuffer B;

    MoveChar(B, 0, ConMaxCols, ' ', 0x07, 1);
    if (!ConQuerySize(&W, &H)
        || !ConSetBox(0, 0, W, H, B[0]))
            return 0;

    return 1;
}

int ConPutBox(int X, int Y, int W, int H, PCell Cell) {
    int I;
    int MX, MY;
    int MouseHidden = 0;
    unsigned char *p = (unsigned char *) Cell;

    if (MouseVisible)
        ConQueryMousePos(&MX, &MY);

    for (I = 0; I < H; I++) {
        if (MouseVisible)
            if (Y + I == MY)
                if ((MX >= X) && (MX <= X + W)) {
                    DrawMouse(0);
                    MouseHidden = 1;
                }
        VioWrtCellStr((PCH)p, (USHORT)(W << 1), (USHORT)(Y + I), (USHORT)X, 0);

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
    USHORT WW = (USHORT)(W << 1);
    unsigned char *p = (unsigned char *) Cell;

    if (MouseVisible)
        ConQueryMousePos(&MX, &MY);

    for (I = 0; I < H; I++) {
        if (MouseVisible)
            if (Y + I == MY)
                if (MX >= X && MX < X + W) {
                    DrawMouse(0);
                    MouseHidden = 1;
                }
        VioReadCellStr((PCH)p, &WW, (USHORT)(Y + I), (USHORT)X, 0);

        if (MouseHidden) {
            DrawMouse(1);
            MouseHidden = 0;
        }
        p += W << 1;
    }

    return 1;
}

int ConPutLine(int X, int Y, int W, int H, PCell Cell) {
    int I;
    int MX, MY;
    int MouseHidden = 0;
    unsigned char *p = (unsigned char *) Cell;

    if (MouseVisible)
        ConQueryMousePos(&MX, &MY);

    for (I = 0; I < H; I++) {
    if (MouseVisible)
        if (Y + I == MY)
            if (MX >= X && MX < X + W) {
                DrawMouse(0);
                MouseHidden = 1;
            }
        VioWrtCellStr((PCH)p, (USHORT)(W << 1), (USHORT)(Y + I), (USHORT)X, 0);

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
    unsigned char *p = (unsigned char *) &Cell;

    if (MouseVisible)
        ConQueryMousePos(&MX, &MY);

    for (I = 0; I < H; I++) {
        if (MouseVisible)
            if (Y + I == MY)
                if (MX >= X && MX < X + W) {
                    DrawMouse(0);
                    MouseHidden = 1;
                }
        VioWrtNCell((PCH)p, (USHORT)(W), (USHORT)(Y + I), (USHORT)X, 0);

        if (MouseHidden) {
            DrawMouse(1);
            MouseHidden = 0;
        }
    }
    return 0;
}

int ConScroll(int Way, int X, int Y, int W, int H, TAttr Fill, int Count) {
    int MX, MY;
    int MouseHidden = 0;
    TCell FillCell = (TCell)(Fill << 8);

    if (MousePresent && MouseVisible) {
        ConQueryMousePos(&MX, &MY);
        if (MX >= X && MX < X + W && MY >= Y && MY < Y + H) {
            DrawMouse(0);
            MouseHidden = 1;
        }
    }

    switch (Way) {
    case csUp:
        VioScrollUp((USHORT)Y, (USHORT)X, (USHORT)(Y + H - 1), (USHORT)(X + W - 1), (USHORT)Count, (PBYTE)&FillCell, 0);
        break;
    case csDown:
        VioScrollDn((USHORT)Y, (USHORT)X, (USHORT)(Y + H - 1), (USHORT)(X + W - 1), (USHORT)Count, (PBYTE)&FillCell, 0);
        break;
    case csLeft:
        VioScrollLf((USHORT)Y, (USHORT)X, (USHORT)(Y + H - 1), (USHORT)(X + W - 1), (USHORT)Count, (PBYTE)&FillCell, 0);
        break;
    case csRight:
        VioScrollRt((USHORT)Y, (USHORT)X, (USHORT)(Y + H - 1), (USHORT)(X + W - 1), (USHORT)Count, (PBYTE)&FillCell, 0);
        break;
    }

    if (MouseHidden)
        DrawMouse(1);

    return 1;
}

int ConSetSize(int X, int Y) {
    VIOMODEINFO vmi;
    int rc;

    vmi.cb = sizeof(VIOMODEINFO);
    VioGetMode(&vmi, 0);
    vmi.col = (USHORT)X;
    vmi.row = (USHORT)Y;
    vmi.cb = 2 + 1 + 1 + 2 + 2;
    rc = VioSetMode(&vmi, 0);

    return (rc == 0);
}

int ConQuerySize(int *X, int *Y) {
    VIOMODEINFO vmi;

    vmi.cb = sizeof(VIOMODEINFO);
    VioGetMode(&vmi, 0);
    if (X) *X = vmi.col;
    if (Y) *Y = vmi.row;

    return 1;
}

int ConSetCursorPos(int X, int Y) {
    VioSetCurPos((USHORT)Y, (USHORT)X, 0);

    return 1;
}

int ConQueryCursorPos(int *X, int *Y) {
    USHORT AX, AY;

    VioGetCurPos(&AY, &AX, 0);
    if (X) *X = AX;
    if (Y) *Y = AY;

    return 1;
}

int ConShowCursor() {
    CursorVisible = 1;
    DrawCursor(1);

    return 1;
}

int ConHideCursor() {
    CursorVisible = 0;
    DrawCursor(0);

    return 1;
}

int ConSetCursorSize(int Start, int End) {
    VIOCURSORINFO ci;

    VioGetCurType(&ci, 0);
    ci.yStart = -Start;
    ci.cEnd = -End;
    ci.cx = 0;
    VioSetCurType(&ci, 0);

    return 1;
}

int ConSetMousePos(int X, int Y) {
    PTRLOC mp;

    if (!MousePresent)
            return 0;

    mp.col = (USHORT)X;
    mp.row = (USHORT)Y;
    MouSetPtrPos(&mp, MouseHandle);

    return 1;
}

int ConQueryMousePos(int *X, int *Y) {
    PTRLOC mp;

    if (!MousePresent)
            return 0;

    MouGetPtrPos(&mp, MouseHandle);
    if (X) *X = mp.col;
    if (Y) *Y = mp.row;

    return 1;
}

int ConShowMouse() {
    MouseVisible = 1;

    if (!MousePresent)
            return 0;

    DrawMouse(1);

    return 1;
}

int ConHideMouse() {
    MouseVisible = 0;

    if (!MousePresent)
            return 0;

    DrawMouse(0);

    return 1;
}

int ConMouseVisible() {
    return (MouseVisible == 1);
}

int ConQueryMouseButtons(int *ButtonCount) {
    USHORT Count;

    if (MouGetNumButtons(&Count, MouseHandle) != 0)
            return 0;

    if (ButtonCount)
            *ButtonCount = Count;

    return 1;
}



int ConInit(int XSize, int YSize) {
    USHORT MevMask = 127;

    if (Initialized)
        return 0;

    EventBuf.What = evNone;
    MousePresent = (MouOpen(NULL, &MouseHandle) == 0) ?1:0;

    if (MousePresent)
        MouSetEventMask(&MevMask, MouseHandle);

    memset(&SaveKbdState, 0, sizeof(SaveKbdState));
    SaveKbdState.cb = sizeof(SaveKbdState);
    assert(KbdGetStatus(&SaveKbdState, 0) == 0);
    ConContinue();

    Initialized = 1;

    return 1;
}

int ConDone() {
    return ConSuspend();
}

int ConSuspend() {
    VIOINTENSITY vi;
    static KBDINFO ki;

    vi.cb = 6;
    vi.type = 2;
    vi.fs = 0;
    VioSetState(&vi, 0);

    ki = SaveKbdState;
    ki.fsMask &= ~(KEYBOARD_ECHO_OFF | KEYBOARD_BINARY_MODE);
    ki.fsMask |=  (KEYBOARD_ECHO_ON | KEYBOARD_ASCII_MODE);
    assert(0 == KbdSetStatus(&ki, 0));

    ConHideMouse();

    signal(SIGBREAK, SIG_DFL);
    signal(SIGINT, SIG_DFL);

    return 1;
}

int ConContinue() {
    VIOINTENSITY vi;
    static KBDINFO ki;

    signal(SIGBREAK, SIG_IGN);
    signal(SIGINT, SIG_IGN);

    ki = SaveKbdState;
    ki.fsMask &= ~(KEYBOARD_ECHO_ON | KEYBOARD_ASCII_MODE);
    ki.fsMask |=  (KEYBOARD_ECHO_OFF | KEYBOARD_BINARY_MODE);
    assert(KbdSetStatus (&ki, 0) == 0);

    vi.cb = 6;
    vi.type = 2;
    vi.fs = 1;
    VioSetState(&vi, 0);
    ConShowMouse();

    return 1;
}

int GetPipeEvent(TEvent *Event) {
    ULONG ulPostCount;
    int i;

    Event->What = evNone;
    for (i = 0; i < MAX_PIPES; i++) {
        if (Pipes[i].used == 0) continue;
        if (Pipes[i].notify == 0) continue;
        if (DosResetEventSem(Pipes[i].NewData, &ulPostCount) != 0)
            continue;
        if (ulPostCount > 0) {
            //fprintf(stderr, "Pipe New Data: %d\n", i);
            Event->What = evNotify;
            Event->Msg.View = 0;
            Event->Msg.Model = Pipes[i].notify;
            Event->Msg.Command = cmPipeRead;
            Event->Msg.Param1 = i;
            return 1;
        }
    }

    return 0;
}

int ConGetEvent(TEventMask EventMask, TEvent *Event, int WaitTime, int Delete) {
    KBDINFO ki;

    if (EventBuf.What != evNone) {
        *Event = EventBuf;
        if (Delete) EventBuf.What = evNone;
        return 0;
    }
    if (MouseEv.What != evNone) {
        *Event = MouseEv;
        if (Delete) MouseEv.What = evNone;
        return 0;
    }
    EventBuf.What = evNone;
    Event->What = evNone;

    ki = SaveKbdState;
    ki.fsMask &= ~(KEYBOARD_ECHO_ON | KEYBOARD_ASCII_MODE);
    ki.fsMask |=  (KEYBOARD_ECHO_OFF | KEYBOARD_BINARY_MODE);
    assert(KbdSetStatus (&ki, 0) == 0);

    while ((WaitTime == -1) || (WaitTime >= 0)) {
        if ((ReadKbdEvent(Event, WaitTime) == 1) && (EventMask & evKeyboard)) break;
        else if (MousePresent && (ReadMouseEvent(Event, EventMask) == 1) && (EventMask & evMouse)) break;
        else if (GetPipeEvent(Event) == 1) break;

        if (WaitTime == 0) return 0;
        DosSleep(5);
        if (WaitTime > 0) {
            WaitTime -= 5;
            if (WaitTime <= 0) return 0;
        }
    }
    if (Event->What != evNone) {
        if (Event->What == evMouseMove) {
            while (ReadMouseEvent(&MouseEv, EventMask) == 1) {
                if (MouseEv.What == evMouseMove) {
                    *Event = MouseEv;
                    MouseEv.What = evNone;
                } else break;
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
}

GUI::~GUI() {
    RestoreScreen();

    if (SavedScreen)
        free(SavedScreen);

    ::ConDone();
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

static int CreatePipeChild(PID &pid, HPIPE &hfPipe, char *Command) {
    static int PCount = 0;
    char szPipe[32];
    char FailBuf[256];
    char *Args;
    int arglen = 0;
    char *Prog;
    RESULTCODES rc_code;
    ULONG ulAction;
    //ULONG ulNew;
    HPIPE hfChildPipe;
    HFILE hfNewStdOut = (HFILE)-1, hfNewStdErr = (HFILE)-1;
    HFILE hfStdOut = 1, hfStdErr = 2;
    int rc;

    sprintf(szPipe, "\\PIPE\\FTE%d\\CHILD%d", getpid(), PCount);
    PCount++;

    rc = DosCreateNPipe(szPipe, &hfPipe,
                         NP_NOINHERIT | NP_ACCESS_INBOUND,
                         NP_NOWAIT | NP_TYPE_BYTE | NP_READMODE_BYTE | 1,
                         0, 4096, 0);
    if (rc != 0)
        return 0;

    rc = DosConnectNPipe (hfPipe);
    if (rc != 0 && rc != ERROR_PIPE_NOT_CONNECTED) {
        DosClose(hfPipe);
        return 0;
    }

    rc = DosSetNPHState (hfPipe, NP_WAIT | NP_READMODE_BYTE);
    if (rc != 0) {
        DosClose(hfPipe);
        return 0;
    }

    rc = DosOpen (szPipe, &hfChildPipe, &ulAction, 0,
                  FILE_NORMAL,
                  OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW,
                  OPEN_ACCESS_WRITEONLY | OPEN_SHARE_DENYREADWRITE,
                  NULL);
    if (rc != 0) {
        DosClose (hfPipe);
        return 0;
    }

    // Duplicate handles
    DosDupHandle(hfStdOut, &hfNewStdOut);
    DosDupHandle(hfStdErr, &hfNewStdErr);
    // Close existing handles for current process
    DosClose(hfStdOut);
    DosClose(hfStdErr);
    // Redirect existing handles to new file
    DosDupHandle(hfChildPipe, &hfStdOut);
    DosDupHandle(hfChildPipe, &hfStdErr);
    // Let started program inherit handles from parent

    Prog = getenv("COMSPEC");

    Args = (char *)malloc(strlen(Prog) + 1
                          + 3 + strlen(Command) + 1
                          + 1);
    if (Args == NULL) {
        DosClose(hfPipe);
        return 0;
    }

    strcpy(Args, Prog);
    arglen = strlen(Args) + 1;
    strcpy(Args + arglen, "/c ");
    arglen += 3;
    strcpy(Args + arglen, Command);
    arglen += strlen(Command) + 1;
    Args[arglen] = '\0';

    rc = DosExecPgm(FailBuf, sizeof(FailBuf),
                    EXEC_ASYNCRESULT, // | EXEC_BACKGROUND,
                    Args,
                    0,
                    &rc_code,
                    Prog);

    free(Args);

    // Get back original handles
    DosDupHandle(hfNewStdOut, &hfStdOut);
    DosDupHandle(hfNewStdErr, &hfStdErr);
    // Close the duplicated handles - no longer needed
    DosClose(hfNewStdOut);
    DosClose(hfNewStdErr);

    DosClose(hfChildPipe); // pipe one way, close out write end

    if (rc != 0) {
        DosClose(hfPipe);
        return 0;
    }

    pid = rc_code.codeTerminate; // get pid when successful

    return 1;
}

static void _LNK_CONV PipeThread(void *p) {
    GPipe *pipe = (GPipe *)p;
    int rc;
    ULONG ulPostCount;
    ULONG used;
    PID pid;
    HPIPE hfPipe;
    RESULTCODES rc_code;

    rc = CreatePipeChild(pid, hfPipe, pipe->Command);

    if (rc != 0) {
        //fprintf(stderr, "Failed createpipe");
        DosRequestMutexSem(pipe->Access, SEM_INDEFINITE_WAIT);
        pipe->reading = 0;
        DosPostEventSem(pipe->NewData);
        DosReleaseMutexSem(pipe->Access);
        return;
    }

    //fprintf(stderr, "Pipe: Begin: %d %s\n", pipe->id, Args);
    while (1) {
        //fprintf(stderr, "Waiting on pipe\n");
            //fread(pipe->buffer, 1, pipe->buflen, fp);
        rc = DosRead(hfPipe, pipe->buffer, pipe->buflen, &used);
        if (rc < 0)
            used = 0;

        DosRequestMutexSem(pipe->Access, SEM_INDEFINITE_WAIT);
        //fprintf(stderr, "Waiting on mutex\n");
        pipe->bufused = used;
        //fprintf(stderr, "Pipe: fread: %d %d\n", pipe->id, pipe->bufused);
        DosResetEventSem(pipe->ResumeRead, &ulPostCount);
        if (pipe->bufused == 0)
            break;
        if (pipe->notify) {
            DosPostEventSem(pipe->NewData);
            pipe->stopped = 0;
        }
        DosReleaseMutexSem(pipe->Access);
        if (pipe->DoTerm)
            break;
        //fprintf(stderr, "Waiting on sem\n");
        DosWaitEventSem(pipe->ResumeRead, SEM_INDEFINITE_WAIT);
        //fprintf(stderr, "Read: Released mutex\n");
        if (pipe->DoTerm)
            break;
    }
    DosClose(hfPipe);
    //fprintf(stderr, "Pipe: pClose: %d\n", pipe->id);
    rc = DosWaitChild(DCWA_PROCESS, DCWW_WAIT,
                      &rc_code,
                      &pid,
                      pid);
    pipe->RetCode = (rc_code.codeResult == 0);
            // pclose(fp);
    pipe->reading = 0;
    DosPostEventSem(pipe->NewData);
    DosReleaseMutexSem(pipe->Access);
    //fprintf(stderr, "Read: Released mutex\n");
}

int GUI::OpenPipe(const char *Command, EModel *notify) {
    for (int i = 0; i < MAX_PIPES; i++) {
        if (Pipes[i].used == 0) {
            Pipes[i].reading = 1;
            Pipes[i].stopped = 1;
            Pipes[i].id = i;
            Pipes[i].bufused = 0;
            Pipes[i].bufpos = 0;
            Pipes[i].buflen = PIPE_BUFLEN;
            Pipes[i].Command = strdup(Command);
            Pipes[i].notify = notify;
            Pipes[i].DoTerm = 0;

            if ((Pipes[i].buffer = (char *)malloc(PIPE_BUFLEN)) == 0) {
                free(Pipes[i].Command);
                return -1;
            }

            if (0 != DosCreateMutexSem(0, &Pipes[i].Access, 0, 0)) {
                free(Pipes[i].Command);
                free(Pipes[i].buffer);
                return -1;
            }

            if (0 != DosCreateEventSem(0, &Pipes[i].ResumeRead, 0, 0)) {
                free(Pipes[i].Command);
                free(Pipes[i].buffer);
                DosCloseMutexSem(Pipes[i].Access);
                return -1;
            }

            if (0 != DosCreateEventSem(0, &Pipes[i].NewData, 0, 0)) {
                free(Pipes[i].Command);
                free(Pipes[i].buffer);
                DosCloseEventSem(Pipes[i].ResumeRead);
                DosCloseMutexSem(Pipes[i].Access);
                return -1;
            }

#if defined(__MT__) || defined(__MULTI__)
            Pipes[i].tid = _beginthread(PipeThread,
                                        FAKE_BEGINTHREAD_NULL
                                        16384, &Pipes[i]);
#else
            DosCreateThread(Pipes[i].tid,
                            (PFNTHREAD)PipeThread,
                            &Pipes[i],
                            0, /* immediate */
                            16384);
#endif
            Pipes[i].used = 1;
            //fprintf(stderr, "Pipe Open: %d\n", i);
            return i;
        }
    }
    return -1;
}

int GUI::SetPipeView(int id, EModel *notify) {
    if (id < 0 || id > MAX_PIPES)
        return 0;
    if (Pipes[id].used == 0)
        return 0;

    DosRequestMutexSem(Pipes[id].Access, SEM_INDEFINITE_WAIT);
    //fprintf(stderr, "Pipe View: %d %08X\n", id, notify);
    Pipes[id].notify = notify;
    DosReleaseMutexSem(Pipes[id].Access);

    return 1;
}

ssize_t GUI::ReadPipe(int id, void *buffer, size_t len) {
    ssize_t l = -1;
    //ULONG ulPostCount;

    if (id < 0 || id > MAX_PIPES || Pipes[id].used == 0)
        return -1;

    //fprintf(stderr, "Read: Waiting on mutex\n");
    //ConContinue();
    DosRequestMutexSem(Pipes[id].Access, SEM_INDEFINITE_WAIT);
    //fprintf(stderr, "Pipe Read: Get %d %d\n", id, len);
    if (Pipes[id].bufused - Pipes[id].bufpos > 0) {
        l = len;
        if (l > Pipes[id].bufused - Pipes[id].bufpos) {
            l = Pipes[id].bufused - Pipes[id].bufpos;
        }
        memcpy(buffer,
               Pipes[id].buffer + Pipes[id].bufpos,
               l);
        Pipes[id].bufpos += l;
        if (Pipes[id].bufpos == Pipes[id].bufused) {
            Pipes[id].bufused = 0;
            Pipes[id].bufpos = 0;
            //fprintf(stderr, "Pipe Resume Read: %d\n", id);
            Pipes[id].stopped = 1;
            //fprintf(stderr, "Read: posting sem\n");
            DosPostEventSem(Pipes[id].ResumeRead);
        }
    } else if (Pipes[id].reading)
        l = 0;
//        DosBeep(200, 200);
    }
    //fprintf(stderr, "Pipe Read: Got %d %d\n", id, l);
    DosReleaseMutexSem(Pipes[id].Access);
    //fprintf(stderr, "Read: Released mutex\n");
    return l;
}

int GUI::ClosePipe(int id) {
    if (id < 0 || id > MAX_PIPES)
        return 0;
    if (Pipes[id].used == 0)
        return 0;
    if (Pipes[id].reading == 1) {
        Pipes[id].DoTerm = 1;
        DosPostEventSem(Pipes[id].ResumeRead);
        DosWaitThread(&Pipes[id].tid, DCWW_WAIT);
    }
    free(Pipes[id].buffer);
    free(Pipes[id].Command);
    DosCloseEventSem(Pipes[id].NewData);
    DosCloseEventSem(Pipes[id].ResumeRead);
    DosCloseMutexSem(Pipes[id].Access);
//    fprintf(stderr, "Pipe Close: %d\n", id);
    Pipes[id].used = 0;
    //ConContinue();
    return Pipes[id].RetCode;
}

int GUI::RunProgram(int mode, char *Command) {
    int rc, W, H, W1, H1;

    ConQuerySize(&W, &H);
    ConHideMouse();
    ConSuspend();

    if (Command == 0 || *Command == 0)  // empty string = shell
        Command = getenv("COMSPEC");

    rc = (system(Command) == 0);

    ConContinue();
    ConShowMouse();
    ConQuerySize(&W1, &H1);

    if (W != W1 || H != H1)
        frames->Resize(W1, H1);

    frames->Repaint();

    return rc;
}

int ConSetTitle(const char *Title, const char *STitle) {
/*    HSWITCH hsw;
    SWCNTRL sw;
    HAB hab;
    PID pid;
    TID tid;

    static PVOID Shmem = NULL;

    if (Shmem == NULL)
        DosAllocSharedMem(&Shmem, NULL, 4096,
                          PAG_COMMIT | PAG_READ | PAG_WRITE | OBJ_GIVEABLE);

    strcpy(Shmem, Title);

    hab = WinInitialize(0);

    hsw = WinQuerySwitchHandle(NULLHANDLE, getpid());

    if (WinQuerySwitchEntry(hsw, &sw) != 0)
        printf("\x7\n");
    else {

        strncpy (sw.szSwtitle, Title, MAXNAMEL - 1);
        sw.szSwtitle[MAXNAMEL-1] = 0;

        printf("hwnd: %X, hwndIcon: %X, pid: %d\n",
               sw.hwnd,
               sw.hwndIcon,
               sw.idProcess);

        WinQueryWindowProcess(sw.hwnd, &pid, &tid);

        DosGiveSharedMem(Shmem, pid, PAG_READ | PAG_WRITE);

        printf("txt 1: %d\n", WinSetWindowText(sw.hwnd, Shmem));
//        printf("txt 2: %d\n", WinSetWindowText(Wsw.hwndIcon, Shmem));

        WinChangeSwitchEntry(hsw, &sw);
    }

    WinTerminate(hab);
  */
    return 1;
}

int ConGetTitle(char *Title, size_t MaxLen, char *STitle, size_t SMaxLen) {
    strlcpy(Title, "FTE",  MaxLen);
    strlcpy(STitle, "FTE", SMaxLen);

    return 1;
}

int ConCursorVisible() {
    return (CursorVisible == 1);
}

int ConPutEvent(const TEvent& Event) {
    EventBuf = Event;
    return 1;
}

extern int SevenBit;

char ConGetDrawChar(unsigned int index) {
    static const char tab[] =  "Ú¿ÀÙÄ³ÂÃ´ÁÅ\x1AúÄ±°\x1B\x1A";
    static const char tab7[] = "++++-|+++++\x1A.-++#+\x1B\x1A";

    assert(index < strlen(tab));

    return (SevenBit) ? tab7[index] : tab[index];
}
