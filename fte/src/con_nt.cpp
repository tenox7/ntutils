/*        con_nt.cpp
 *
 *        Copyright (c) 1994-1996, Marko Macek
 *
 *        You may distribute under the terms of either the GNU General Public
 *        License or the Artistic License, as specified in the README file.
 *
 */

/* some functionality is now shared with g_text.cpp and g_draw.cpp */
/* new os/2 code needs to be merged with this */
/* the console APIs on win'95 seem to be really screwed up */

/*
 *      10/13/96 Jal:
 *              Rebuilt for Windows NT, generic; no port/Watcom code should
 *              be needed to compile (jal).
 *              Removed most mouse handling code (unnecessary), added pipe handler by
 *              porting the OS/2 version to NT.
 *              Solved some bugs with regard to the TCell problem.
 *
 *      10/28/96 Jal:
 *              Started to replace threaded pipe code with nonthreaded code, using
 *              overlapped I/O.
 *
 *      Wed Jan 15 1997 (Jal):
 *              -       The Grey-Alt-+ key and some other keys were not recognised. This
 *                      was because NT didn't mark these as "enhanced" keys. Now the code
 *                      translates some (scancode,ascii) pairs to the enhanced keyboard.
 *                      The table was already present!
 *              -       To solve the "flashing cursor" problem: now doesn't enter FTE
 *                      mainloop when console returns empty action..
 *
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#include <signal.h>

#include <wincon.h>

#include <ctype.h>
#include <stdio.h>
#include "sysdep.h"
#include "console.h"
#include "gui.h"
#include "s_files.h"
#include "s_string.h"
#include "c_config.h"

#define True    1
#define False   0


static int Initialized = 0;
static int MousePresent = 0;
static int CursorVisible = 1; /* 1 means visible */
static int MouseVisible = 0; /* 0 means hidden */
static TEvent MouseEv = { evNone };
static TEvent EventBuf = { evNone };
//static TEventMask EventMask;

static HANDLE ConOut;
static HANDLE ConIn;
static HANDLE OurConOut;
static DWORD OldConsoleMode;

static int LastMouseX = 0;
static int      LastMouseY = 0;
//static int      isWin95 = 0;

static char winTitle[256] = "FTE";
static char winSTitle[256] = "FTE";

int codepage;

#define dbm(x)          //printf(x), Sleep(3000)


#if 0
void dbg(const char* s, ...) /*FOLD00*/
{
}
#else

void dbg(const char* s, ...) /*FOLD00*/
{
    char    buf[256];
    va_list args;

    va_start(args, s);
    vsprintf(buf, s, args);
    va_end(args);
    OutputDebugString(buf);
}
#endif


static void DrawCursor(int Show) { /*FOLD00*/
    CONSOLE_CURSOR_INFO cci;

    GetConsoleCursorInfo(OurConOut, &cci);
    cci.bVisible = Show ? TRUE : FALSE;
    SetConsoleCursorInfo(OurConOut, &cci);
}

#define NUMITEMS(x)     (sizeof(x) / sizeof(x[0]))

#if 1
/*
 *      Translation table 1: translate (scan,asciicode) of the input event to a
 *      valid FTE keystroke. This is used because NT sometimes "forgets" to flag
 *      special keys as "enhanced" (grey) keys..
 */
static struct {
    USHORT CharScan;
    TKeyCode KeyCode;
} TransCharScan[] = {
    { 0x0100, kbEsc },                              { 0x011B, kbEsc },
    { 0x1C0D, kbEnter },            { 0x1C0A, kbEnter },
    { 0x1C00, kbEnter },            { 0xE00D, kbEnter | kfGray },
    { 0xA600, kbEnter | kfGray },   { 0xE00A, kbEnter | kfGray },
    { 0x0E08, kbBackSp },           { 0x0E7F, kbBackSp },
    { 0x0E00, kbBackSp },           { 0x0F09, kbTab },
    { 0x9400, kbTab },              { 0xA500, kbTab },
    { 0x0F00, kbTab },              { 0x4E00, '+' | kfGray },
    { 0x9000, '+' | kfGray },       { 0x4E2B, '+' | kfGray },
    { 0x4A00, '-' | kfGray },       { 0x8E00, '-' | kfGray },
    { 0x4A2D, '-' | kfGray },       { 0x3700, '*' | kfGray },
    { 0x9600, '*' | kfGray },       { 0x372A, '*' | kfGray },
    { 0xE02F, '/' | kfGray },       { 0xA400, '/' | kfGray },
    { 0x9500, '/' | kfGray },       { 0x0300, 0 }
};
#endif

#if 0
static struct {
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

    { 0x3B, kbF1    },      { 0x3C, kbF2    },      { 0x3D, kbF3    },
    { 0x3E, kbF4    },      { 0x3F, kbF5    },      { 0x40, kbF6    },
    { 0x41, kbF7    },      { 0x42, kbF8    },      { 0x43, kbF9    },
    { 0x44, kbF10   },      { 0x85, kbF11   },      { 0x86, kbF12   },

    { 0x54, kbF1    },      { 0x55, kbF2    },      { 0x56, kbF3    },
    { 0x57, kbF4    },      { 0x58, kbF5    },      { 0x59, kbF6    },
    { 0x5A, kbF7    },      { 0x5B, kbF8    },      { 0x5C, kbF9    },
    { 0x5D, kbF10   },      { 0x87, kbF11   },      { 0x88, kbF12   },

    { 0x5E, kbF1    },      { 0x5F, kbF2    },      { 0x60, kbF3    },
    { 0x61, kbF4    },      { 0x62, kbF5    },      { 0x63, kbF6    },
    { 0x64, kbF7    },      { 0x65, kbF8    },      { 0x66, kbF9    },
    { 0x67, kbF10   },      { 0x89, kbF11   },      { 0x8A, kbF12   },

    { 0x68, kbF1    },      { 0x69, kbF2    },      { 0x6A, kbF3    },
    { 0x6B, kbF4    },      { 0x6C, kbF5    },      { 0x6D, kbF6    },
    { 0x6E, kbF7    },      { 0x6F, kbF8    },      { 0x70, kbF9    },
    { 0x71, kbF10   },      { 0x8B, kbF11   },      { 0x8C, kbF12   },

    { 0x47, kbHome  },      { 0x48, kbUp    },      { 0x49, kbPgUp  },
    { 0x4B, kbLeft  },      { 0x4C, kbCenter},      { 0x4D, kbRight },
    { 0x4F, kbEnd   },      { 0x50, kbDown  },      { 0x51, kbPgDn  },
    { 0x52, kbIns   },      { 0x53, kbDel   },

    { 0x77, kbHome  },      { 0x8D, kbUp    },      { 0x84, kbPgUp  },
    { 0x73, kbLeft  },                                              { 0x74, kbRight },
    { 0x75, kbEnd   },      { 0x91, kbDown  },      { 0x76, kbPgDn  },
    { 0x92, kbIns   },      { 0x93, kbDel   },

    { 0x97, kbHome  | kfGray },  { 0x98, kbUp        | kfGray },  { 0x99, kbPgUp  | kfGray },
    { 0x9B, kbLeft  | kfGray },                                                       { 0x9D, kbRight | kfGray },
    { 0x9F, kbEnd   | kfGray },  { 0xA0, kbDown  | kfGray },  { 0xA1, kbPgDn  | kfGray },
    { 0xA2, kbIns   | kfGray },  { 0xA3, kbDel       | kfGray }
};
#endif


struct {
    SHORT VirtCode;
    unsigned long KeyCode;
} VirtTab[]  =
{
    { 112, kbF1 },
    { 113, kbF2 },
    { 114, kbF3 },
    { 115, kbF4 },
    { 116, kbF5 },
    { 117, kbF6 },
    { 118, kbF7 },
    { 119, kbF8 },
    { 120, kbF9 },
    { 121, kbF10 },
    { 122, kbF11 },
    { 123, kbF12 },

    { 35, kbEnd },
    { 36, kbHome },
    { 33, kbPgUp },
    { 34, kbPgDn },
    { 38, kbUp },
    { 37, kbLeft },
    { 39, kbRight },
    { 40, kbDown },
    { 45, kbIns },
    { 46, kbDel },

    { 27, kbEsc },
    { 13, kbEnter },
    { 8, kbBackSp },
    { 32, kbSpace },
    { 9, kbTab },

    { 0, 0 }
};

static const char shftwrng[]  = "~!@#$%^&*()_+{}|:\"<>?";
static const char shftright[] = "`1234567890-=[]\\;',./";

int ReadConsoleEvent(TEvent *E) /*FOLD00*/
{
    /*
     *      Reads and interprets the console event. It is called when console input
     *      handle is signalled. To prevent flashing cursors this routine returns
     *      F if there's nothing to do; this causes the caller to loop without
     *      returning to the FTE mainloop.
     */
    INPUT_RECORD inp;
    DWORD           nread;
    TKeyCode        Ch = 0;
    TKeyCode        flg = 0;
    ULONG           flags;
    int             I, i;

    ReadConsoleInput(ConIn, &inp, 1, &nread);
    if (nread != 1) return False;                           // Nothing read after signal??

    switch (inp.EventType)
    {
    case WINDOW_BUFFER_SIZE_EVENT:
        //** Resized the window. Make FTE use the new size..
        frames->Resize(inp.Event.WindowBufferSizeEvent.dwSize.X, inp.Event.WindowBufferSizeEvent.dwSize.Y);
        frames->Repaint();
        return True;

    case KEY_EVENT:
        if (inp.Event.KeyEvent.bKeyDown) {
            if ((inp.Event.KeyEvent.dwControlKeyState & CAPSLOCK_ON) &&
                inp.Event.KeyEvent.wVirtualKeyCode != 106 &&
                inp.Event.KeyEvent.wVirtualKeyCode != 109 &&
                inp.Event.KeyEvent.wVirtualKeyCode != 107)
            {
                if (!(inp.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED)) {
                    for (i=0; shftwrng[i]; i++) {
                        if (inp.Event.KeyEvent.uChar.AsciiChar == shftwrng[i]) {
                            inp.Event.KeyEvent.uChar.AsciiChar = shftright[i];
                            break;
                        }
                    }
                }
                else {
                    for (i=0; shftright[i]; i++) {
                        if (inp.Event.KeyEvent.uChar.AsciiChar == shftright[i]) {
                            inp.Event.KeyEvent.uChar.AsciiChar = shftwrng[i];
                            break;
                        }
                    }
                }
            }
        }
        //** Skip shift, control and alt key stuff.
        switch(inp.Event.KeyEvent.wVirtualKeyCode)
        {       case VK_SHIFT: case VK_CONTROL: case VK_MENU: case VK_PAUSE:
        case VK_CAPITAL: case VK_LWIN:  case VK_RWIN: case VK_APPS:
            return False;
        }

        //** Distill FTE flags from the NT flags. This fails for some keys
        //** because NT has an oddity with enhanced keys (Alt-Grey-+ etc).

        // from chaac: Please do not toutch RIGHT_ALT_PRESSED handling,
        // in some keyboard ALT-GR is used for special characters.
        flags = inp.Event.KeyEvent.dwControlKeyState;
        if (flags & (/*RIGHT_ALT_PRESSED |*/ LEFT_ALT_PRESSED)) flg |= kfAlt;
        if (flags & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) flg |= kfCtrl;
        if (flags & (RIGHT_ALT_PRESSED)) flg &= ~kfCtrl;
        if (flags & SHIFT_PRESSED) flg |= kfShift;
        if (flags & ENHANCED_KEY) flg |= kfGray;

#if 0
        dbg("key1: %s, vk=%x, vscan=%x, flags=%x, rep=%d, ascii=%x (%c).\n",
            inp.Event.KeyEvent.bKeyDown ? "down" : "up",
            inp.Event.KeyEvent.wVirtualKeyCode,
            inp.Event.KeyEvent.wVirtualScanCode,
            flags,
            inp.Event.KeyEvent.wRepeatCount,
            inp.Event.KeyEvent.uChar.AsciiChar,
            inp.Event.KeyEvent.uChar.AsciiChar);
#endif
        Ch = 0;

        // handle special case when user with scandinavian keyboard presses
        // alt-gr + special key and then spacebar
        if (inp.Event.KeyEvent.bKeyDown) {
            if ((inp.Event.KeyEvent.wVirtualKeyCode == 0x20) &&
                (inp.Event.KeyEvent.wVirtualScanCode == 0x39))
            {
                switch(inp.Event.KeyEvent.uChar.AsciiChar)
                {
                case '~': Ch = '~'; break;
                case '^': Ch = '^'; break;
                case '`': Ch = '`'; break;
                case 'ï': Ch = 'ï'; break;
                }
            }
        }

        //** Translate VK codes to FTE codes,
        if (Ch == 0)
        {
            for (I = 0; I < sizeof(VirtTab)/sizeof(VirtTab[0]); I++)
                if (VirtTab[I].VirtCode == inp.Event.KeyEvent.wVirtualKeyCode)
                {
                    Ch = VirtTab[I].KeyCode;
                    break;
                }
        }

        //** Not a virtual key-> do charscan translation, if needed;
        if(Ch == 0)
        {
            unsigned int     cc = ((inp.Event.KeyEvent.wVirtualScanCode << 8) | (unsigned char)inp.Event.KeyEvent.uChar.AsciiChar);
            for(I = 0; I < NUMITEMS(TransCharScan); I++)
            {
                if(cc == TransCharScan[I].CharScan)
                {
                    Ch = TransCharScan[I].KeyCode;
                    break;
                }
            }
        }
        if (Ch == 0)
        {
            if ((Ch = (TKeyCode) (unsigned char)inp.Event.KeyEvent.uChar.AsciiChar) != 0)
            {
                if (flg & kfAlt) Ch = toupper(Ch);
            }
        }

        if (Ch == 0)                            //** Odd: cannot distill keycode.
            return False;

        if (flg & kfCtrl)
            if (Ch < 32)
                Ch += 64;

        E->Key.Code = Ch | flg;         // Set FTE keycode,
        E->What = inp.Event.KeyEvent.bKeyDown ? evKeyDown : evKeyUp;
        return True;

    case MOUSE_EVENT:
        LastMouseX = E->Mouse.X = inp.Event.MouseEvent.dwMousePosition.X;
        LastMouseY = E->Mouse.Y = inp.Event.MouseEvent.dwMousePosition.Y;
        flags = inp.Event.MouseEvent.dwControlKeyState;
        if (flags & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)) flg |= kfAlt;
        if (flags & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) flg |= kfCtrl;
        if (flags & SHIFT_PRESSED) flg |= kfShift;
        E->Mouse.KeyMask = flg;
        E->Mouse.Buttons = (unsigned short)inp.Event.MouseEvent.dwButtonState;
        E->Mouse.Count = 1;
        if (inp.Event.MouseEvent.dwEventFlags & DOUBLE_CLICK)
            E->Mouse.Count = 2;

        if ((inp.Event.MouseEvent.dwEventFlags & MOUSE_WHEELED) == MOUSE_WHEELED)
        {
            E->What        = evCommand;
            E->Msg.View    = 0;
            E->Msg.Model   = 0;
            E->Msg.Param2  = 0;

            // Scroll:
            // (The SDK does not tell how to determine whether the wheel
            // was scrolled up or down. Found an example on:
            // http://www.adrianxw.dk/SoftwareSite/Consoles/Consoles5.html
            if (inp.Event.MouseEvent.dwButtonState & 0xFF000000) {  // Wheel down
                if (flg & kfShift) { // Translate to horizontal scroll.
                    E->Msg.Command = (flg & kfCtrl) ? cmHScrollPgRt : cmHScrollRight;
                } else { // Translate to vertical scroll.
                    E->Msg.Command = (flg & kfCtrl) ? cmVScrollPgDn : cmVScrollDown;
                }
            } else { // Wheel up
                if (flg & kfShift) { // Translate to horizontal scroll.
                    E->Msg.Command = (flg & kfCtrl) ? cmHScrollPgLt : cmHScrollLeft;
                } else { // Translate to vertical scroll.
                    E->Msg.Command = (flg & kfCtrl) ? cmVScrollPgUp : cmVScrollUp;
                }
            }

            E->Msg.Param1 = (flg & kfCtrl) ? 1 : 3; // 1 page / 3 lines

            return True;
        }

        if (inp.Event.MouseEvent.dwEventFlags == MOUSE_MOVED)
        {
            E->What = evMouseMove;
            //puts("Move");
        }
        else
        {
            static unsigned short mb = 0;

            if (inp.Event.MouseEvent.dwButtonState & ~mb)
            {
                E->What = evMouseDown;
                E->Mouse.Buttons = ((unsigned short)inp.Event.MouseEvent.dwButtonState) & ~mb;
                //puts("Down");
            }
            else
            {
                E->What = evMouseUp;
                E->Mouse.Buttons = mb & ~((unsigned short)inp.Event.MouseEvent.dwButtonState);
                //puts("Up");
            }
            mb = (unsigned short)inp.Event.MouseEvent.dwButtonState;
        }
        return True;
    }
    return False;
}


int ConInit(int /*XSize*/, int /*YSize*/) { /*FOLD00*/
    if (Initialized) return 1;

    EventBuf.What = evNone;
    MousePresent    = 0; //MOUSInit();

    //** Get NT/Win95 flag,
    OSVERSIONINFO   oi;

    oi.dwOSVersionInfoSize = sizeof(oi);
    GetVersionEx((LPOSVERSIONINFO) &oi);
    //isWin95 = (oi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);

    ConOut = GetStdHandle(STD_OUTPUT_HANDLE);
    ConIn = GetStdHandle(STD_INPUT_HANDLE);
    codepage = GetConsoleCP();
    GetConsoleMode(ConIn, &OldConsoleMode);
    SetConsoleMode(ConIn,
                   ENABLE_WINDOW_INPUT |
                   ENABLE_MOUSE_INPUT);
    OurConOut = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                          0, NULL,
                                          CONSOLE_TEXTMODE_BUFFER, NULL);
    ConContinue();

    Initialized = 1;

    return 1;
}

int ConDone() { /*FOLD00*/
    ConSuspend();
    CloseHandle(OurConOut);

    return 1;
}

int ConSuspend() { /*FOLD00*/
    SetConsoleActiveScreenBuffer(ConOut);
    SetConsoleMode(ConIn, OldConsoleMode);

    return 1;
}

int ConContinue() { /*FOLD00*/
    SetConsoleActiveScreenBuffer(OurConOut);
    GetConsoleMode(ConIn, &OldConsoleMode);
    SetConsoleMode(ConIn, ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);

    SetConsoleOutputCP(codepage);
    SetConsoleCP(codepage);

    return 1;
}

int ConClear() { /*FOLD00*/
    int W, H;
    TDrawBuffer B;

    MoveChar(B, 0, ConMaxCols, ' ', 0x07, 1);
    if (!ConQuerySize(&W, &H)
        || !ConSetBox(0, 0, W, H, B[0]))
        return 0;

    return 1;
}


#if 0   // Mouse control not necessary when using console functions.
/*--------------------------------------------------------------------------*/
/*      CLASS:  tMouHelp is used to control mouse cursor visibility during              */
/*                      screen updates.                                                                                                 */
/*--------------------------------------------------------------------------*/
class tMouHelp
{
protected:
    int     mh_x, mh_y;                     // Current mouse position / 0
    int     mh_valid;
    int     mh_disabled;                    // T if mouse should be re-enabled.

public:
    tMouHelp() : mh_x(0), mh_y(0), mh_valid(FALSE), mh_disabled(FALSE)      {}
    ~tMouHelp()
    {       if(MouseVisible && mh_disabled) DrawMouse(1);
    }

    void    disIfLine(int x, int w, int y)
    {
        if(mh_disabled) return;
        if(! mh_valid)
        {
            ConQueryMousePos(&mh_x, &mh_y);
            mh_valid = TRUE;
        }
        if(y == mh_y && mh_x >= x && mh_x < x+y)
        {
            mh_disabled= TRUE;
            DrawMouse(0);
        }
    }
};
#endif

int ConPutBox(int X, int Y, int W, int H, PCell Cell) /*FOLD00*/
{
    int             I;
    PCell           p = Cell;
    COORD           corg, csize;
    SMALL_RECT      rcl;
    BOOL            rc;

    for (I = 0; I < H; I++)
    {
        corg.X  = corg.Y = 0;
        csize.X = W;
        csize.Y = 1;
        rcl.Left= X;
        rcl.Top = I + Y;
        rcl.Bottom = I + Y;// + (isWin95 ? 1 : 0);
        rcl.Right = X + W - 1;// + (isWin95 ? 1 : 0);

        rc = WriteConsoleOutput(OurConOut, (PCHAR_INFO)p, csize, corg, &rcl);
        if (rc != TRUE) {
            //("WriteConsoleOutput %d\n", rc);
        }
        p += W;
    }
    return 1;
}

int ConGetBox(int X, int Y, int W, int H, PCell Cell) /*FOLD00*/
{
    int             I;
    USHORT          WW = W << 1;
    PCell           p = Cell;
    COORD           corg, csize;
    SMALL_RECT      rcl;

    for (I = 0; I < H; I++)
    {
        corg.X = corg.Y = 0;
        csize.X = W;
        csize.Y = 1;
        rcl.Left = X;
        rcl.Top = I + Y;
        rcl.Bottom = I + Y;// + (isWin95 ? 1 : 0);
        rcl.Right = X + W - 1;// + (isWin95 ? 1 : 0);

        ReadConsoleOutput(OurConOut, (PCHAR_INFO)p, csize, corg, &rcl);
        p += W;
    }
    return 1;
}

int ConPutLine(int X, int Y, int W, int H, PCell Cell) /*FOLD00*/
{
    int             I;
    COORD           corg, csize;
    SMALL_RECT      rcl;
    BOOL rc;

    for (I = 0; I < H; I++)
    {
        corg.X = corg.Y = 0;
        csize.X = W;
        csize.Y = 1;
        rcl.Left = X;
        rcl.Top = I + Y;
        rcl.Bottom = I + Y;// + (isWin95 ? 1 : 0);
        rcl.Right = X + W - 1;// + (isWin95 ? 1 : 0);

        rc = WriteConsoleOutput(OurConOut, (PCHAR_INFO)Cell, csize, corg, &rcl);
        if (rc != TRUE) {
            //printf("WriteConsoleOutput %d\n", rc);
        }
    }

    return 1;
}

int ConSetBox(int X, int Y, int W, int H, TCell Cell) /*FOLD00*/
{
    int             I;
    COORD           corg, csize;
    SMALL_RECT      rcl;
    TDrawBuffer B;

    I = W;
    while (I-- > 0) B[I] = Cell;

    for (I = 0; I < H; I++)
    {
        corg.X = corg.Y = 0;
        csize.X = W;
        csize.Y = 1;
        rcl.Left = X;
        rcl.Top = I + Y;
        rcl.Bottom = I + Y;// - (isWin95 ? 1 : 0);
        rcl.Right = X + W - 1;// - (isWin95 ? 1 : 0);

        WriteConsoleOutput(OurConOut, (PCHAR_INFO)B, csize, corg, &rcl);
    }

    return 1;
}

int ConScroll(int Way, int X, int Y, int W, int H, TAttr Fill, int Count) /*FOLD00*/
{
    TCell           FillCell;
    SMALL_RECT      rect, clip;
    COORD           dest;

    MoveCh(&FillCell, ' ', Fill, 1);

    clip.Left = X;
    clip.Top = Y;
    clip.Right = X + W - 1;
    clip.Bottom = Y + H - 1;

    rect = clip;
    dest.X = X;
    dest.Y = Y;

    switch (Way) {
    case csUp:
        rect.Top += Count;
        break;
    case csDown:
        rect.Bottom -= Count;
        dest.Y += Count;
        break;
    case csLeft:
        rect.Left += Count;
        break;
    case csRight:
        rect.Right += Count;
        dest.X += Count;
        break;
    }

    ScrollConsoleScreenBuffer(OurConOut, &rect, &clip, dest, (PCHAR_INFO)&FillCell);

    return 1;
}

int ConSetSize(int X, int Y) { /*FOLD00*/
    return 0;
}

int ConQuerySize(int *X, int *Y) { /*FOLD00*/
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    GetConsoleScreenBufferInfo(OurConOut, &csbi);
    *X = csbi.dwSize.X;
    *Y = csbi.dwSize.Y;

    dbg("Console size (%u,%u)\n", *X, *Y);

    return 1;
}

int ConSetCursorPos(int X, int Y) { /*FOLD00*/
    COORD xy;

    xy.X = X;
    xy.Y = Y;
    SetConsoleCursorPosition(OurConOut, xy);

    return 1;
}

int ConQueryCursorPos(int *X, int *Y) { /*FOLD00*/
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    GetConsoleScreenBufferInfo(OurConOut, &csbi);
    *X = csbi.dwCursorPosition.X;
    *Y = csbi.dwCursorPosition.Y;

    return 1;
}

int ConShowCursor() { /*FOLD00*/
    CursorVisible = 1;
    DrawCursor(1);

    return 1;
}

int ConHideCursor() { /*FOLD00*/
    CursorVisible = 0;
    DrawCursor(0);

    return 1;
}

int ConCursorVisible() { /*FOLD00*/
    return (CursorVisible == 1);
}

int ConSetCursorSize(int Start, int End) { /*FOLD00*/
    return 0;
}

int ConSetMousePos(int X, int Y) { /*FOLD00*/
    return 0;
}

int ConQueryMousePos(int *X, int *Y) { /*FOLD00*/
    *X = LastMouseX;
    *Y = LastMouseY;

    // NT does not have this ? (not needed anyway, but check mouse hiding above).
    return 1;
}

int ConShowMouse() { /*FOLD00*/
    MouseVisible = 1;

    return (MousePresent != 0);
}

int ConHideMouse() { /*FOLD00*/
    MouseVisible = 0;

    return (MousePresent != 0);
}

int ConMouseVisible() { /*FOLD00*/
    return (MouseVisible == 1);
}

int ConQueryMouseButtons(int *ButtonCount) { /*FOLD00*/
    return 1;
}

int ConPutEvent(const TEvent& Event) { /*FOLD00*/
    EventBuf = Event;

    return 1;
}

int ConFlush() { /*FOLD00*/
    return 1;
}

int ConGrabEvents(TEventMask EventMask) { /*FOLD00*/
    return 1;
}


static PCell SavedScreen = 0;
static int SavedX, SavedY, SaveCursorPosX, SaveCursorPosY;

int SaveScreen() { /*FOLD00*/
    if (SavedScreen)
        free(SavedScreen);

    ConQuerySize(&SavedX, &SavedY);

    SavedScreen = (PCell) malloc(SavedX * SavedY * sizeof(TCell));

    if (SavedScreen)
        ConGetBox(0, 0, SavedX, SavedY, SavedScreen);

    ConQueryCursorPos(&SaveCursorPosX, &SaveCursorPosY);

    return 1;
}

int RestoreScreen() { /*FOLD00*/
    if (SavedScreen) {
        ConPutBox(0, 0, SavedX, SavedY, SavedScreen);
        ConSetCursorPos(SaveCursorPosX, SaveCursorPosY);
    }
    return 1;
}


GUI::GUI(int &argc, char **argv, int XSize, int YSize) { /*FOLD00*/
    fArgc = argc;
    fArgv = argv;
    ::ConInit(-1, -1);
    SaveScreen();
    ::ConSetSize(XSize, YSize);
    gui = this;
}

GUI::~GUI() { /*FOLD00*/
    RestoreScreen();

    if (SavedScreen)
        free(SavedScreen);

    ::ConDone();
    gui = 0;
}

int GUI::ConSuspend() { /*FOLD00*/
    RestoreScreen();
    return ::ConSuspend();
}

int GUI::ConContinue() { /*FOLD00*/
    SaveScreen();
    return ::ConContinue();
}

int GUI::ShowEntryScreen() { /*FOLD00*/
    TEvent E;

    ConHideMouse();
    RestoreScreen();
    SetConsoleActiveScreenBuffer(ConOut);
    do { gui->ConGetEvent(evKeyDown, &E, -1, 1, 0); } while (E.What != evKeyDown);
    SetConsoleActiveScreenBuffer(OurConOut);
    ConShowMouse();
    if (frames)
        frames->Repaint();
    return 1;
}

char ConGetDrawChar(unsigned int index) { /*FOLD00*/
    static const char *tab=NULL;

    if (!tab) {
        tab=GetGUICharacters ("WindowsNT","Ú¿ÀÙÄ³ÂÃ´ÁÅ\x1AúÄ±°\x1B\x1A");
    }
    assert(index < strlen(tab));

    return tab[index];
}


int GUI::RunProgram(int mode, char *Command) { /*FOLD00*/
    int rc, W, H, W1, H1;

    ConQuerySize(&W, &H);
    ConHideMouse();
    ConSuspend();

    if (*Command == 0)      // empty string = shell
        Command = getenv(
                         "COMSPEC"
                        );

    // we don't want ctrl-c or ctrl-break to exit our editor...
    signal(SIGBREAK, SIG_IGN);
    signal(SIGINT, SIG_IGN);

    rc = system(Command);

    // restore handlers back to default handlers
    signal(SIGBREAK, SIG_DFL);
    signal(SIGINT, SIG_DFL);

    ConContinue();
    ConShowMouse();
    ConQuerySize(&W1, &H1);

    if (W != W1 || H != H1) {
        frames->Resize(W1, H1);
    }
    frames->Repaint();
    return rc;
}

int ConSetTitle(const char *Title, const char *STitle) { /*FOLD00*/
    char buf[sizeof(winTitle)] = {0};
    JustFileName(Title, buf, sizeof(buf));
    if (buf[0] == '\0') // if there is no filename, try the directory name.
        JustLastDirectory(Title, buf, sizeof(buf));

    strlcpy(winTitle, "FTE - ", sizeof(winTitle));
    if (buf[0] != 0) // if there is a file/dir name, stick it in here.
    {
        strlcat(winTitle, buf, sizeof(winTitle));
        strlcat(winTitle, " - ", sizeof(winTitle));
    }
    strlcat(winTitle, Title, sizeof(winTitle));
    strlcpy(winSTitle, STitle, sizeof(winSTitle));
    SetConsoleTitle (winTitle);

    return 1;
}

int ConGetTitle(char *Title, size_t MaxLen, char *STitle, size_t SMaxLen) { /*FOLD00*/
    strlcpy(Title, winTitle, MaxLen);
    strlcpy(STitle, winSTitle, SMaxLen);
    return 1;
}


#if 0
/****************************************************************************/
/*                                                                                                                                                      */
/*      CODING: Pipe handler.                                                                                                   */
/*                                                                                                                                                      */
/****************************************************************************/
/*--------------------------------------------------------------------------*/
/*      STATIC GLOBALS.                                                                                                                 */
/*--------------------------------------------------------------------------*/
#define MAX_PIPES       4
#define PIPE_BUFLEN 4096
#define PIPEBUF_SZ      4096

class NTHandle
{
protected:
    HANDLE  nth_h;

public:
    operator HANDLE()
    {       return nth_h;
    }

    void    close()
    {       if(nth_h != INVALID_HANDLE_VALUE)
    {
        CloseHandle(nth_h);
        nth_h = INVALID_HANDLE_VALUE;
    }
    }


    NTHandle()      {       nth_h = INVALID_HANDLE_VALUE;   }

    ~NTHandle()
    {       close();
    }

    NTHandle(const HANDLE& h) : nth_h(h)    {}
    NTHandle(const NTHandle& nth);                          // UNDEFINED (no assgn)
    NTHandle& operator =(const NTHandle& nth);      // UNDEFINED (no assgn)
    NTHandle& operator =(const HANDLE nth)
    {       close();
        nth_h = nth;
    return *this;
    }
};


class GPipe
{
public:
    int     p_used;
    int     p_id;
    char*   p_buffer;
    int     p_buflen;
    int     p_bufused;
    int     p_bufpos;
    EModel* p_notify;
    char*   p_command;
    int     p_retcode;
    int     p_doterm;

    //** NT specific.
    HANDLE  p_proc_h;                               // Handle of spawned process,
    HANDLE  pipeDataRead;
    HANDLE  pipeStartRead;
    HANDLE  pipeMutex;
    HANDLE  p_pipe_ph;                              // Input pipe (read by FTE)
    HANDLE  p_child_ph;                     // Client side's handle (written to by spawned)
    DWORD   p_read_len;                     // #bytes read in overlapped I/O
    int     p_io_posted;                    // T when overlapped I/O is posted,
    int     p_completed;                    // T when client process closed down.
    int     p_has_data;                     // T when OVERLAPPED completed.

    static GPipe    pipe_ar[MAX_PIPES];


public:
    int     open(char *Command, EModel *notify);
    int     close();
    int     read(void *buffer, int len);
    int     getEvent(TEvent* event);


protected:
    int     createPipe();
    void    releasePipe();
    int     runCommand();
    void    closeProc();
    int     handlePost();
    int     postRead();



public:
    static GPipe*   getFreePipe();
    static GPipe*   getPipe(int id);

};

GPipe GPipe::pipe_ar[MAX_PIPES];


/*
 *      getFreePipe() locates an unused GPipe structure. It also assigns it's ID.
 */
GPipe* GPipe::getFreePipe() /*FOLD00*/
{
    int     i;

    for(i = 0; i < MAX_PIPES; i++)
    {
        if(! pipe_ar[i].p_used)
        {
            pipe_ar[i].p_id = i;            // Set pipenr,
            return pipe_ar + i;
        }
    }
    return NULL;                                            // No free pipe
}


GPipe* GPipe::getPipe(int id) /*FOLD00*/
{
    if (id < 0 || id > MAX_PIPES) return NULL;
    if(! pipe_ar[id].p_used) return NULL;
    return pipe_ar + id;
}


int GPipe::createPipe() /*FOLD00*/
{
    /*
     *      Called from open() to create and open the server and the client pipes.
     */
    static int      PCount = 0;
    //HANDLE          hchild;
    char            pipename[50];
    int             ok;
    SECURITY_ATTRIBUTES sa;

    sa.nLength = sizeof(sa);                        // Security descriptor for INHERIT.
    sa.lpSecurityDescriptor = 0;
    sa.bInheritHandle       = 1;

#if 1
    if (CreatePipe(&p_pipe_ph, &p_child_ph, &sa, 0) == FALSE)
        return FALSE;

    Pipes[i].tid = _beginthread(PipeThread,
                                FAKE_BEGINTHREAD_NULL
                                16384, &Pipes[i]);
#else

    //** Create the named pipe, and handle the SERVER (edit)'s end...
    sprintf(pipename, "\\\\.\\pipe\\fte%d\\child%d", getpid(), PCount);
    p_pipe_ph = CreateNamedPipe(pipename,
                                PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
                                PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                                1,                      // nMaxInstances,
                                0, PIPEBUF_SZ,
                                1000,
                                0);
    if(p_pipe_ph == INVALID_HANDLE_VALUE)
        return FALSE;
    PCount++;

    /*
     *      Client side: get a connection to the server's pipe. Do this before the
     *      call to ConnectNamedPipe() to prevent it from blocking.
     */

#if 1
    p_child_ph      = CreateFile(pipename, GENERIC_WRITE, 0, &sa,
                                 OPEN_EXISTING, 0, 0);
#else
    p_child_ph      = CreateFile("_test", GENERIC_WRITE|GENERIC_READ, 0, &sa,
                                 CREATE_ALWAYS, 0, 0);
#endif
    if(p_child_ph == INVALID_HANDLE_VALUE)
        dbm("CreateFile(client_side_pipe) has failed");
    else
    {
        //** Server side: aquire connection..
        ok      = TRUE;
        if(! ConnectNamedPipe(p_pipe_ph, 0))    // Get connect;
        {
            if(GetLastError() != ERROR_PIPE_CONNECTED)
                ok = FALSE;
        }

        //** Connect worked?
        if(!ok)
            dbm("ConnectNmPipe() has failed");
        else
            return TRUE;                            // All opened & ready for action!

        //** Something went wrong.
        CloseHandle(p_child_ph);                // Close child: was inh.
        DisconnectNamedPipe(p_pipe_ph); // Force disconnection of client (-)
        CloseHandle(p_child_ph);
    }
    CloseHandle(p_pipe_ph);
#endif
    return FALSE;                                                                   // Something has failed.
}


void GPipe::releasePipe() /*FOLD00*/
{
    /*
     *      releasePipe() releases all that createPipe() allocates. It's usually
     *      called when an error causes the process to abort.
     */
    if(p_child_ph != INVALID_HANDLE_VALUE)
    {
        CloseHandle(p_child_ph);
        p_child_ph      = INVALID_HANDLE_VALUE;
    }

    if(p_pipe_ph != 0)
    {
        //DisconnectNamedPipe(p_pipe_ph);
        CloseHandle(p_pipe_ph);
        p_pipe_ph = INVALID_HANDLE_VALUE;
    }
}


int GPipe::runCommand() /*FOLD00*/
{
    /*
     *      runCommand() takes the child pipe, dups it onto stdout and stderr while
     *      saving their old assignments, then it spawns
     */
    int                         ok;
    char*                       comspec, *args, tbuf[256];
    HANDLE                          errh;
    PROCESS_INFORMATION pi;
    STARTUPINFO             si;
    const char          nt4[] = "4nt.exe";

    ok              = FALSE;
    comspec = getenv("COMSPEC");

    /*
     *  BUG workaround: When using 4NT, it doesn't properly reassign stderr!
     *  This is a bug in 4NT, so if comspec *is* 4nt use cmd.exe instead...
     */
    if(comspec == 0) return -1;
    int l = strlen(comspec);
    if(strnicmp(comspec + (l- sizeof(nt4)+1), nt4, sizeof(nt4)-1) == 0)
    {
        //** It's 4DOS all right..
        args = getenv("SystemRoot");
        if(args== 0) return -1;
        strlcpy(tbuf, args, sizeof(tbuf));                 // Get to c:\winnt
        strlcat(tbuf, "\\system32\\cmd.exe", sizeof(tbuf));
        comspec = tbuf;
    }

    int argslen = strlen(comspec) + strlen(p_command) + 120;
    args    = (char *)malloc(argslen);
    if(args == 0)
        dbm("malloc() failed for command line..");
    else
    {
        //** Form a command line for the process;
        strlcpy(args, comspec, argslen);
        strlcat(args, " /c ", argslen);
        strlcat(args, p_command, argslen);

        //** Dup the child handle to get separate handles for stdout and err,
        if (DuplicateHandle(GetCurrentProcess(), p_child_ph, // Source,
                           GetCurrentProcess(), &errh,          // Destination,
                           0, True,                                             // Same access, inheritable
                           DUPLICATE_SAME_ACCESS));

        {
            /* Set up members of STARTUPINFO structure. */
            memset(&si, 0, sizeof(si));
            si.cb = sizeof(STARTUPINFO);
            si.lpReserved = NULL;
            si.lpReserved2 = NULL;
            si.cbReserved2 = 0;
            si.lpDesktop = NULL;
/*            si.dwFlags = STARTF_USESTDHANDLES;
#if 1
            si.hStdOutput = p_child_ph;
            si.hStdError    = errh;
            si.hStdInput    = INVALID_HANDLE_VALUE;
#else
            si.hStdOutput = errh;
            si.hStdError    = p_child_ph;
            si.hStdInput    = INVALID_HANDLE_VALUE;
#endif*/
            if(CreateProcess(NULL, args, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
            {
                ok      = TRUE;
                CloseHandle(pi.hThread);        // Thread handle not needed
                p_proc_h        = pi.hProcess;  // Return process handle (to get RC)
            }
            CloseHandle(errh);                                      // Close error handle,
        }
        else
            dbm("DupHandle for stderr failed.");

        free(args);                                     // Release command line.
    }

    //        SetConsoleMode(horgout, ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);

    //** And last but not least: close the child handle.
    CloseHandle(p_child_ph);
    p_child_ph = INVALID_HANDLE_VALUE;
    return ok;
}


void GPipe::closeProc() /*FOLD00*/
{
    /*
     *      closeProc() gets called when a read fails. It assumes the process has
     *      ended, retrieves the process return code, then it closes all handles.
     *      The state is set to p_completed.
     */
    DWORD   ec;

    dbg("[closeProc] ");

    if(! GetExitCodeProcess(p_proc_h, &ec)) ec = 0xabcd;
    p_retcode       = ec;                                           // Save return code of process,
    if(p_proc_h != INVALID_HANDLE_VALUE)    // Close process,
    {
        CloseHandle(p_proc_h);
        p_proc_h = INVALID_HANDLE_VALUE;
    }

    //** Close the main pipe,
    if(p_pipe_ph != INVALID_HANDLE_VALUE)
    {
        CloseHandle(p_pipe_ph);
        p_pipe_ph = INVALID_HANDLE_VALUE;
    }
    p_completed = TRUE;
    p_has_data      = TRUE;
}


int GPipe::startRead() /*FOLD00*/
{
    /*
     *      postRead() checks if an overlapped READ needs to be posted by checking
     *      the io_posted flag. If that's clear and no termination or closed flag
     *      is set then a new overlapped I/O request is issued.
     */
    p_has_data      = FALSE;
    dbg("[postRead ");
    if(p_io_posted || p_completed)
        dbg("no action: %s] ", p_io_posted ? "posted" : "complete");
    else
    {
        p_ovl.hEvent    = p_data_evh;           // Signal this when done,
        if(!ReadFile(p_pipe_ph, p_buffer, p_buflen, &p_read_len, NULL))
        {
            DWORD   ec = GetLastError();
            if(ec != ERROR_IO_PENDING)
            {
                //** Something's wrong. Treat as closed pipe for now.
                closeProc();                            // Close pipe, complete stuff...
                dbg("postfail] ");
                return FALSE;                           // And return failure.
            }
        }
        p_io_posted = TRUE;                             // Handle pending ioresult.
        dbg("posted] ");
    }
    return TRUE;
}


int GPipe::open(char* command, EModel* notify) /*FOLD00*/
{
    memset(&p_ovl, 0, sizeof(p_ovl));               // Clear overlapped,
    p_bufused       = 0;
    p_bufpos        = 0;
    p_io_posted = FALSE;
    p_has_data      = FALSE;
    p_completed = FALSE;                                    // T if client closed.
    p_doterm        = FALSE;
    p_buflen        = PIPE_BUFLEN;
    p_notify        = notify;
    p_doterm        = FALSE;

    p_pipe_ph       = INVALID_HANDLE_VALUE;
    p_child_ph      = INVALID_HANDLE_VALUE;
    if( (p_command = strdup(command)) == 0)
        return -1;

    //** Allocate the read buffer;
    if ((p_buffer = (char*) malloc(p_buflen)) != 0) {
        if ((pipeDataRead = CreateEvent(0, 1, 0, 0)) == 0) {
            dbm("CreateEvent(data_evh) failed.");
            goto fail;
        }
        if ((pipeStartRead = CreateEvent(0, 1, 0, 0)) == 0) {
            dbm("CreateEvent(data_evh) failed.");
            goto fail;
        }
        if ((pipeMutex = CreateMutex(NULL, FALSE, NULL)) == NULL) {
            dbm("Failed pipe mutex");
            goto fail;
        }


        else
        {
            if(createPipe())                                // Create server & client pipe.
            {
                if(! postRead())
                    dbm("postRead() initial failed.");
                else
                {
                    if(runCommand())
                    {
                        p_used  = TRUE;
                        return p_id;
                    }
                }
                releasePipe();                          // Release pipes,
            }
            CloseHandle(p_data_evh);
        }
        free(p_buffer);
    }
    free(p_command);
    return -1;
}


int GPipe::close() /*FOLD00*/
{
    /*
     *      close() disconnects from the spawned task, closes the pipe and releases
     *      all stuff.
     */
    if(! p_used) return -1;
    if(! p_completed)                                               // Overlapped I/O not complete yet?
    {
        //** We *must* wait till the overlapped I/O completes,
        if(p_io_posted)
        {
            GetOverlappedResult(p_pipe_ph, &p_ovl, &p_read_len, TRUE);
            p_io_posted = FALSE;
        }
    }
    p_completed= TRUE;

    //** Now close all that might be pending,
    free(p_buffer);
    free(p_command);

    releasePipe();                                          // Close all pipe stuff,
    if(p_proc_h != INVALID_HANDLE_VALUE)
    {
        CloseHandle(p_proc_h);
        p_proc_h = INVALID_HANDLE_VALUE;
    }

    CloseHandle(pipeStartRead);
    CloseHandle(pipeDataRead);
    CloseHandle(pipeMutex);

    p_used = FALSE;
    return p_retcode;
}


int GPipe::read(void *buffer, int len) /*FOLD00*/
{
    /*
     *      read() is called to get the current data from the pipe. It takes the
     *      #bytes read and returns them. It returns data till the buffer is
     *      exhausted. If the process is completed it returns -1; else it returns
     *      the #bytes read. It returns 0 if the buffer's empty.
     */
    dbg("[read ");
    if(p_has_data)
    {
        if(p_bufpos < p_read_len)                               // Data in buffer?
        {
            unsigned        l;

            l       = p_read_len - p_bufpos;                // Try to output all,
            if(l > len) l = len;
            memcpy(buffer, p_buffer+p_bufpos, l);   // Copy data from the buffer,
            p_bufpos        += l;
            dbg("%u data] ", l);
            return l;                                                       // Data returned,
        }

        //** There's nothing left in the buffer. Is the task complete?
        if(p_completed)
        {
            dbg("no data, complete] ");
            return -1;
        }

        if(! postRead())
        {
            dbg("post failed-> complete] ");
            return -1;
        }

        dbg("nodata, post] ");
        return 0;
    }
    else if(p_completed)
    {
        dbg("completed] ");
        return -1;
    }

    dbg("nothing] ");
    return 0;
}


int GPipe::getEvent(TEvent* event) /*FOLD00*/
{
    dbg("[getpipeevent: ");
    event->What = evNone;

    if(! p_used || p_notify == 0) return 0;         // No data.
    if(! handlePost()) return 0;                            // Again: no data,
    //** This pipe has data!
    event->What             = evNotify;
    event->Msg.View         = 0;
    event->Msg.Model        = p_notify;
    event->Msg.Command      = cmPipeRead;
    event->Msg.Param1       = p_id;
    dbg("ok] ");
    return 1;
}


/*
 *      NT Pipe handler - overview
 *      ==========================
 *      The NT pipe handler uses overlapped I/O to read console events.
 *
 *      OpenPipe():
 *      When the pipe is opened, one of the pipe structures is allocated and set
 *      to used. Then an event semaphore (reset_manual) is created. This semaphore
 *      will be signalled when data is available on the input pipe which gathers
 *      the spawned tasks's output.
 *
 *      Then a pipe is created, opened for the client side and stdout and stderr
 *      are redirected therein. After that the client task is spawned.
 *
 *      If the spawn succeeds an overlapped READ is posted for the pipe; then the
 *      OpenPipe function returns.
 *
 *      ConGetEvent():
 *      The ConGetEvent() handler does a WaitForMultipleObjects() on the console
 *      handle and all pipe handles currently active. If a pipe has data the
 *      overlapped result is gotten, and the output is sent to the message window.
 *      Then, if the thread didn't finish, a new overlapped read is posted.
 *
 *
 */
int GUI::OpenPipe(const char *Command, EModel *notify) /*FOLD00*/
{
    GPipe*  gp;

    if( (gp = GPipe::getFreePipe()) == 0)
        return -1;                                                      // Out of pipes.
    return gp->open(Command, notify);               // And ask the pipe to init.
}


int GUI::SetPipeView(int id, EModel *notify) /*FOLD00*/
{
    GPipe*  p;

    if( (p = GPipe::getPipe(id)) == 0) return -1;
    p->lock();
    p->p_notify = notify;
    p->unlock();
    return 0;
}


ssize_t GUI::ReadPipe(int id, void *buffer, size_t len) /*FOLD00*/
{
    //int     l;
    GPipe*  p;

    if( (p = GPipe::getPipe(id)) == 0) return -1;
    return p->read(buffer, len);
}


int GUI::ClosePipe(int id) /*FOLD00*/
{
    GPipe*  p;

    if( (p = GPipe::getPipe(id)) == 0) return -1;
    return p->close();
}


static int GetPipeEvent(int id, TEvent *Event) /*FOLD00*/
{
    //int     i;
    GPipe*  p;

    if( (p = GPipe::getPipe(id)) == 0) return -1;
    return p->getEvent(Event);
}

#else

#define MAX_PIPES 4
#define PIPE_BUFLEN 4096

typedef struct {
    int used;
    int id;
    int reading, stopped;
    HANDLE Thread;
    HANDLE Access;
    HANDLE ResumeRead;
    HANDLE NewData;
    char *buffer;
    int buflen;
    int bufused;
    int bufpos;
    EModel *notify;
    char *Command;
    DWORD RetCode;
    int DoTerm;
} GPipe;

static GPipe Pipes[MAX_PIPES] = {
    { 0 }, { 0 }, { 0 }, { 0 }
};

static int CreatePipeChild(HANDLE &child, HANDLE &hPipe, char *Command) {
    //static int PCount = 0;
    int arglen = 0;
    HANDLE hChildPipe;
    BOOL rc;

    SECURITY_ATTRIBUTES sa;

    sa.nLength = sizeof(sa);                        // Security descriptor for INHERIT.
    sa.lpSecurityDescriptor = 0;
    sa.bInheritHandle       = 1;

    rc = CreatePipe(&hPipe, &hChildPipe, &sa, 0);
    if (rc != TRUE)
        return -1;

    int                         ok;
    char*                       comspec, *args, tbuf[256];
    //HANDLE                          errh;
    PROCESS_INFORMATION pi;
    STARTUPINFO             si;
    HANDLE hNul;
    const char          nt4[] = "4nt.exe";

    ok              = FALSE;
    comspec = getenv("COMSPEC");

    /*
     *  BUG workaround: When using 4NT, it doesn't properly reassign stderr!
     *  This is a bug in 4NT, so if comspec *is* 4nt use cmd.exe instead...
     */
    if (comspec == 0)
        return -1;
    int l = strlen(comspec);
    if (strnicmp(comspec + (l- sizeof(nt4)+1), nt4, sizeof(nt4)-1) == 0)
    {
        //** It's 4DOS all right..
        args = getenv("SystemRoot");
        if(args== 0) return -1;
        strlcpy(tbuf, args, sizeof(tbuf));                 // Get to c:\winnt
        strlcat(tbuf, "\\system32\\cmd.exe", sizeof(tbuf));
        comspec = tbuf;
    }

    int argslen = strlen(comspec) + strlen(Command) + 120;
    args    = (char *)malloc(argslen);
    if(args == 0)
        dbm("malloc() failed for command line..");
    else
    {
        //** Form a command line for the process;
        strlcpy(args, comspec, argslen);
        strlcat(args, " /c ", argslen);
        strlcat(args, Command, argslen);

        //** Dup the child handle to get separate handles for stdout and err,
        /*if (DuplicateHandle(GetCurrentProcess(), hChildPipe,
                           GetCurrentProcess(), &errh,
                           0, True, DUPLICATE_SAME_ACCESS))*/
        //fprintf(stderr, "open NUL\n");

        hNul = CreateFile("NUL",
                          GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL, OPEN_EXISTING,
                          0,
                          NULL);
        //fprintf(stderr, "starting %s\n", args);

        if (hNul != NULL) {
            /* Set up members of STARTUPINFO structure. */
            memset(&si, 0, sizeof(si));
            si.cb = sizeof(STARTUPINFO);
            si.lpReserved = NULL;
            si.lpReserved2 = NULL;
            si.cbReserved2 = 0;
            si.lpDesktop = NULL;
            si.dwFlags = STARTF_USESTDHANDLES;
#if 1
            si.hStdOutput = hChildPipe;
            si.hStdError    = hChildPipe;
            si.hStdInput    = hNul;//INVALID_HANDLE_VALUE;
#else
            si.hStdOutput = errh;
            si.hStdError    = hChildPipe;
            si.hStdInput    = INVALID_HANDLE_VALUE;
#endif
            if (CreateProcess(NULL, args, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi) == TRUE)
            {
                ok      = TRUE;
                CloseHandle(pi.hThread);        // Thread handle not needed
                //fprintf(stderr, "create process success\n");
                child = pi.hProcess;  // Return process handle (to get RC)
            } else
                //fprintf(stderr, "create process failed %d\n" + GetLastError());
            CloseHandle(hNul);                                      // Close error handle,
        }
        else
            dbm("DupHandle for stderr failed.");

        free(args);
    }

    CloseHandle(hChildPipe);
    return 0;
}

static DWORD __stdcall PipeThread(void *p) {
    GPipe *pipe = (GPipe *)p;
    BOOL rc;
    DWORD used;
    HANDLE child;
    HANDLE hfPipe;

    rc = CreatePipeChild(child, hfPipe, pipe->Command);

    if (rc != 0) {
        //fprintf(stderr, "Failed CreatePipeChild\n");
        WaitForSingleObject(pipe->Access, INFINITE);
        pipe->reading = 0;
        SetEvent(pipe->NewData);
        ReleaseMutex(pipe->Access);
        return 0;
    }

    //fprintf(stderr, "Pipe: Begin: %d %s\n", pipe->id, pipe->Command);
    while (1) {
        //fprintf(stderr, "Waiting on pipe\n");
        rc = ReadFile(hfPipe, pipe->buffer, pipe->buflen, &used, NULL);
        if (rc != TRUE) {
            //fprintf(stderr, "ReadFile failed %d %ld", GetLastError(), used);
            used = 0;
        }

        WaitForSingleObject(pipe->Access, INFINITE);
        //fprintf(stderr, "Waiting on mutex\n");
        pipe->bufused = used;
        //fprintf(stderr, "Pipe: fread: %d %d\n", pipe->id, pipe->bufused);
        ResetEvent(pipe->ResumeRead);
        if (pipe->bufused == 0)
            break;
        if (pipe->notify) {
            SetEvent(pipe->NewData);
            pipe->stopped = 0;
        }
        ReleaseMutex(pipe->Access);
        if (pipe->DoTerm)
            break;
        //fprintf(stderr, "Waiting on sem\n");
        WaitForSingleObject(pipe->ResumeRead, INFINITE);
        //fprintf(stderr, "Read: Released mutex\n");
        if (pipe->DoTerm)
            break;
    }
    CloseHandle(hfPipe);
    //fprintf(stderr, "Pipe: pClose: %d\n", pipe->id);
    rc = WaitForSingleObject(child, INFINITE);
    GetExitCodeProcess(child, &pipe->RetCode);
    CloseHandle(child);
    pipe->reading = 0;
    SetEvent(pipe->NewData);
    ReleaseMutex(pipe->Access);
    //fprintf(stderr, "Read: Released mutex\n");
    return 1;
}

int GUI::OpenPipe(const char *Command, EModel *notify) {
    int i;

    for (i = 0; i < MAX_PIPES; i++) {
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
            if ((Pipes[i].buffer = (char *)malloc(PIPE_BUFLEN)) == 0)
                return -1;

            if ((Pipes[i].Access = CreateMutex(NULL, FALSE, NULL)) == NULL) {
                free(Pipes[i].Command);
                free(Pipes[i].buffer);
                return -1;
            }

            if ((Pipes[i].ResumeRead = CreateEvent(0, 1, 0, 0)) == NULL) {
                free(Pipes[i].Command);
                free(Pipes[i].buffer);
                CloseHandle(Pipes[i].Access);
                return -1;
            }

            if ((Pipes[i].NewData = CreateEvent(0, 1, 0, 0)) == NULL) {
                free(Pipes[i].Command);
                free(Pipes[i].buffer);
                CloseHandle(Pipes[i].ResumeRead);
                CloseHandle(Pipes[i].Access);
                return -1;
            }

            DWORD tid;

            if ((Pipes[i].Thread = CreateThread(NULL, 32768,
                                                &PipeThread, &Pipes[i],
                                                0, &tid)) == NULL)
            {
                free(Pipes[i].Command);
                free(Pipes[i].buffer);
                CloseHandle(Pipes[i].ResumeRead);
                CloseHandle(Pipes[i].Access);
                CloseHandle(Pipes[i].NewData);
                return -1;
            }
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
    WaitForSingleObject(Pipes[id].Access, INFINITE);
    //fprintf(stderr, "Pipe View: %d %08X\n", id, notify);
    Pipes[id].notify = notify;
    ReleaseMutex(Pipes[id].Access);
    return 1;
}

ssize_t GUI::ReadPipe(int id, void *buffer, size_t len) {
    ssize_t l;
    //ULONG ulPostCount;

    if (id < 0 || id > MAX_PIPES || Pipes[id].used == 0)
        return -1;

    //fprintf(stderr, "Read: Waiting on mutex\n");
    //ConContinue();
    WaitForSingleObject(Pipes[id].Access, INFINITE);
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
            SetEvent(Pipes[id].ResumeRead);
        }
    } else if (Pipes[id].reading == 0)
        l = -1;
    else {
        l = 0;
//        DosBeep(200, 200);
    }
    //fprintf(stderr, "Pipe Read: Got %d %d\n", id, l);
    ReleaseMutex(Pipes[id].Access);
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
        SetEvent(Pipes[id].ResumeRead);
        WaitForSingleObject(&Pipes[id].Thread, INFINITE);
    }
    free(Pipes[id].buffer);
    free(Pipes[id].Command);
    CloseHandle(Pipes[id].NewData);
    CloseHandle(Pipes[id].ResumeRead);
    CloseHandle(Pipes[id].Access);
    CloseHandle(Pipes[id].Thread);
    //fprintf(stderr, "Pipe Close: %d\n", id);
    Pipes[id].used = 0;
    //ConContinue();
    return (Pipes[id].RetCode == 0);
}

int GetPipeEvent(int i, TEvent *Event) {
    Event->What = evNone;
    if (Pipes[i].used == 0) return 0;
    if (Pipes[i].notify == 0) return 0;
    ResetEvent(Pipes[i].NewData);
    //fprintf(stderr, "Pipe New Data: %d\n", i);
    Event->What = evNotify;
    Event->Msg.View = 0;
    Event->Msg.Model = Pipes[i].notify;
    Event->Msg.Command = cmPipeRead;
    Event->Msg.Param1 = i;
    return 1;
}

#endif

int ConGetEvent(TEventMask EventMask, TEvent *Event, int WaitTime, int Delete) /*FOLD00*/
{
    //** Any saved events left?
    if (EventBuf.What != evNone) {
        *Event = EventBuf;
        if (Delete) EventBuf.What = evNone;
        return 1;
    }
    if (MouseEv.What != evNone) {
        *Event = MouseEv;
        if (Delete) MouseEv.What = evNone;
        return 1;
    }

    //** Now block and wait for a new event on the console handle and all pipes,
    HANDLE  o_ar[1 + MAX_PIPES];
    DWORD   rc;
    int     i, nh;

    EventBuf.What = evNone;
    Event->What = evNone;

    //** Fill the handle array with all active handles for pipes && console,
    o_ar[0] = ConIn;
    for(i = 0, nh = 1; i < MAX_PIPES; i++) {    // For all possible pipes
        if (Pipes[i].used)
            o_ar[nh++] = Pipes[i].NewData;
    }

    for(;;) {
        rc = WaitForMultipleObjects(nh, o_ar, FALSE, WaitTime);
        if (rc != WAIT_FAILED && (rc >= WAIT_OBJECT_0 && rc < WAIT_OBJECT_0+nh)) {
            i = rc - WAIT_OBJECT_0;             // Get item that signalled new data
	    if (i == 0) {                       // Was console?
                if (ReadConsoleEvent(Event))    // Get console,
                    return 1;                   // And exit if valid,
            } else {
                GetPipeEvent(i - 1, Event);     // Read data from pipe.
                return 1;
            }
        }
        else
            return 0;                          // Something's wrong!
    }
}

#include "clip.h"

int GetClipText(ClipData *cd) {
    int rc = 0;
    cd->fLen = 0;
    cd->fChar = NULL;
    if (OpenClipboard(NULL)) {
        HANDLE hmem;

        if ((hmem = GetClipboardData(CF_TEXT)) != NULL) {
            LPVOID data;

            if ((data = GlobalLock(hmem)) != NULL) {
                int len = strlen((char *)data);

                cd->fChar = (char *)malloc(len);
                if (cd->fChar != NULL) {
                    cd->fLen = len;
                    memcpy(cd->fChar, data, len);
                    rc = 1;
                }
                GlobalUnlock(hmem);
            }
        }
        CloseClipboard();
    }
    return rc;
}

int PutClipText(ClipData *cd) {
    int rc = 0;
    if (OpenClipboard(NULL)) {
        if (EmptyClipboard()) {
            HGLOBAL hmem;

            if ((hmem = GlobalAlloc(GMEM_MOVEABLE, cd->fLen + 1)) != NULL) {
                LPVOID data;

                if ((data = GlobalLock(hmem)) != NULL) {
                    memcpy(data, cd->fChar, cd->fLen);
                    ((char *)data)[cd->fLen] = 0;
                    GlobalUnlock(hmem);
                    if (SetClipboardData(CF_TEXT, hmem))
                        rc = 1;
                }
            }
        }
        CloseClipboard();
    }
    return rc;
}
