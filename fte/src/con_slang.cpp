/*    con_slang.cpp
 *
 *    Copyright (c) 1998, István Váradi
 *    Copyright (c) 2010, Zdenek Kabelac
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

// to have %PRId64 defined for C++
#define __STDC_FORMAT_MACROS

#include "c_config.h"
#include "con_tty.h"
#include "console.h"
#include "gui.h"
#include "s_string.h"
#include "sysdep.h"
//#include "slangkbd.h"

//#include <slang/slang.h>
#include <slang.h>

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

extern TEvent NextEvent; // g_text.cpp

/* These characters cannot appear on a console, so we can detect
 * them in the output routine.
 */
#define DCH_SLANG_C1         128
#define DCH_SLANG_C2         129
#define DCH_SLANG_C3         130
#define DCH_SLANG_C4         131
#define DCH_SLANG_H          132
#define DCH_SLANG_V          133
#define DCH_SLANG_M1         134
#define DCH_SLANG_M2         135
#define DCH_SLANG_M3         136
#define DCH_SLANG_M4         137
#define DCH_SLANG_X          138
#define DCH_SLANG_RPTR       139
#define DCH_SLANG_EOL        140
#define DCH_SLANG_EOF        141
#define DCH_SLANG_END        142
#define DCH_SLANG_AUP        143
#define DCH_SLANG_ADOWN      144
#define DCH_SLANG_HFORE      145
#define DCH_SLANG_HBACK      146
#define DCH_SLANG_ALEFT      147
#define DCH_SLANG_ARIGHT     148

// i  VT
// }  pound
// {  PI
static const char slang_dchs[] =
{
    'l',
    'k',
    'm',
    'j',
    'q',
    'x',
    'w',
    't',
    'u',
    'f',
    'v',
    'n',
    '~',
    '`',
    'q',
    '-',
    '.',
    ' ',
    'a',
    ',',
    '+'
};

static SLsmg_Char_Type raw_dchs[sizeof(slang_dchs)];

static unsigned char ftesl_get_dch(SLsmg_Char_Type raw)
{
    for (size_t i = 0; i < sizeof(slang_dchs); i++)
	if (raw_dchs[i].nchars == raw.nchars
	    && !memcmp(raw_dchs[i].wchars, raw.wchars,
		       raw.nchars * sizeof(*raw.wchars)))
	    return (unsigned char)(DCH_SLANG_C1 + i);
    return DCH_SLANG_EOL;
}

static const char slang_colors[][14] =
{
    "black",
    "blue",
    "green",
    "cyan",
    "red",
    "magenta",
    "brown",
    "lightgray",
    "gray",
    "brightblue",
    "brightgreen",
    "brightcyan",
    "brightred",
    "brightmagenta",
    "yellow",
    "white",
};

static volatile int ScreenSizeChanged;

static void sigwinch_handler(int sig)
{
    ScreenSizeChanged = 1;
}

int ConInit(int /*XSize */ , int /*YSize */ )
{
    SLtt_get_terminfo();

    if ((SLkp_init() == -1)
	|| (SLang_init_tty(0, 1, 1) == -1))
	return 0;

    if (SLsmg_init_smg() == -1) {
	SLang_reset_tty();
	return 0;
    }

    SLsignal_intr(SIGWINCH, sigwinch_handler);
    SLang_set_abort_signal(NULL);
    SLtty_set_suspend_state(0);

    /* skip modification of color 0 and 0x7f */
    for (unsigned i = 1; i < 127; ++i)
	SLtt_set_color(i, NULL, const_cast<char *>(slang_colors[i & 0x0f]),
		       const_cast<char *>(slang_colors[(i >> 4) & 0x07]));

    SLsmg_gotorc(0, 0);
    SLsmg_set_char_set(1);

    SLsmg_write_nchars(const_cast<char*>(slang_dchs), sizeof(slang_dchs));

    SLsmg_gotorc(0, 0);
    SLsmg_read_raw(raw_dchs, sizeof(slang_dchs));
    SLsmg_set_char_set(0);

#ifdef CONFIG_MOUSE
    SLtt_set_mouse_mode(1, 0);
#endif
    SLtt_flush_output();
    //use_esc_hack = (getenv("FTESL_ESC_HACK") != NULL);

    return 1;
}
int ConDone()
{
    SLsmg_reset_smg();
    SLang_reset_tty();

    return 1;
}

int ConSuspend()
{
    SLsmg_suspend_smg();
    SLang_reset_tty();

    return 1;
}
int ConContinue()
{
    SLang_init_tty(-1, 0, 1);
    SLsmg_resume_smg();

    return 1;
}

int ConSetTitle(const char * /*Title */ , const char * /*STitle */ )
{
    return 1;
}

int ConGetTitle(char *Title, size_t MaxLen, char *STitle, size_t SMaxLen)
{
    strlcpy(Title, "", MaxLen);
    strlcpy(STitle, "", SMaxLen);

    return 1;
}

int ConClear()
{
    SLsmg_cls();
    SLsmg_refresh();

    return 1;
}

static void fte_write_color_chars(PCell Cell, int W)
{
    int chset = 0, chsetprev = 2;
    unsigned char ch, col = 0, colprev = 0x80;
    char buf[256];

    while (W > 0) {
	int i;
	for (i = 0; i < W && i < (int)sizeof(buf); ++i) {
	    ch = Cell[i].GetChar();
	    col = Cell[i].GetAttr() & 0x7f;
	    //fprintf(stderr, "W: %d  i:%d  ch: %d %c  col: %2x / %2x\n", W, i, ch, ch, col, Cell[i].GetAttr() & 0x7f);
	    if (ch <= 127 || ch >= 0xa0) {
		buf[i] = (ch < 32) ? '.' : (char)ch;
		chset = 0;
	    } else {
		buf[i] = slang_dchs[ch - 128];
		chset = 1;
	    }

	    if (col != colprev || chset != chsetprev)
		break;
	}

	if (i > 0) {
	    SLsmg_write_nchars(buf, i);
	    W -= i;
	    Cell += i;
	}

	if (col != colprev) {
	    SLsmg_set_color(col);
	    colprev = col;
	}

	if (chset != chsetprev) {
	    SLsmg_set_char_set(chset);
	    chsetprev = chset;
	}
    }
}

int ConPutBox(int X, int Y, int W, int H, PCell Cell)
{
    int CurX, CurY;

    ConQueryCursorPos(&CurX, &CurY);
    for (;H > 0; Cell += W, --H) {
	SLsmg_gotorc(Y++, X);
	fte_write_color_chars(Cell, W);
    }
    ConSetCursorPos(CurX, CurY);

    return 1;
}

static int ConPutBoxRaw(int X, int Y, int W, int H, SLsmg_Char_Type *box)
{
    int CurX, CurY;

    ConQueryCursorPos(&CurX, &CurY);
    for (;H > 0; box += W, --H) {
	SLsmg_gotorc(Y++, X);
	SLsmg_write_raw(box, W);
    }
    ConSetCursorPos(CurX, CurY);

    return 1;
}

int ConGetBox(int X, int Y, int W, int H, PCell Cell)
{
    int CurX, CurY, i;
    SLsmg_Char_Type linebuf[W];

    ConQueryCursorPos(&CurX, &CurY);
    for (;H > 0; Cell += W, --H) {
	SLsmg_gotorc(Y++, X);
	SLsmg_read_raw(linebuf, W);
	for (i = 0; i < W; i++) {
	    if (linebuf[i].color & SLSMG_ACS_MASK)
		Cell[i].SetChar(ftesl_get_dch(linebuf[i]));
	    else
		/*
		 * FIXME: Handle UTF-8 -- way beyond a quick-and-dirty
		 * fix.  --MV
		 */
		Cell[i].SetChar((char)SLSMG_EXTRACT_CHAR(linebuf[i]));
	    /*
	     * FIXME: This preserves only 7 out of 15 bits of color.
	     * Fortunately, we're dealing with color handles rather than
	     * colors themselves -- S-Lang jumps through an extra hoop to
	     * map these to color data.  As long as we use less than 127
	     * different colors, things should be OK.  I think.  --MV
	     */
	    Cell[i].SetAttr(linebuf[i].color & 0x7f);
	}
    }
    ConSetCursorPos(CurX, CurY);

    return 1;
}

static int ConGetBoxRaw(int X, int Y, int W, int H, SLsmg_Char_Type *box)
{
    int CurX, CurY;

    ConQueryCursorPos(&CurX, &CurY);
    for (;H > 0; box += W, --H) {
	SLsmg_gotorc(Y++, X);
	SLsmg_read_raw(box, W);
    }
    ConSetCursorPos(CurX, CurY);

    return 1;
}

int ConPutLine(int X, int Y, int W, int H, PCell Cell)
{
    int CurX, CurY;

    ConQueryCursorPos(&CurX, &CurY);
    for (;H > 0; --H) {
	SLsmg_gotorc(Y++, X);
	fte_write_color_chars(Cell, W);
    }
    ConSetCursorPos(CurX, CurY);

    return 1;
}

int ConSetBox(int X, int Y, int W, int H, TCell Cell)
{
    TCell line[W];

    for (int i = 0; i < W; i++)
	line[i] = Cell;

    ConPutLine(X, Y, W, H, line);

    return 1;
}

int ConScroll(int Way, int X, int Y, int W, int H, TAttr Fill, int Count)
{
    SLsmg_Char_Type box[W * H];
    TCell fill(' ', Fill);

    ConGetBoxRaw(X, Y, W, H, box);

    if (Way == csUp) {
	ConPutBoxRaw(X, Y, W, H - Count, box + W * Count);
	//ConSetBox(X, Y + H - Count, W, Count, fill);
    } else {
	ConPutBoxRaw(X, Y + Count, W, H - Count, box);
	//ConSetBox(X, Y, W, Count, fill);
    }

    return 1;
}

int ConSetSize(int /*X */ , int /*Y */ )
{
    return 0;
}

int ConQuerySize(int *X, int *Y)
{
    *X = SLtt_Screen_Cols;
    *Y = SLtt_Screen_Rows;

    return 1;
}

int ConSetCursorPos(int X, int Y)
{
    SLsmg_gotorc(Y, X);
    SLsmg_refresh();
    return 1;
}

int ConQueryCursorPos(int *X, int *Y)
{
    *X = SLsmg_get_column();
    *Y = SLsmg_get_row();
    return 1;
}

static int CurVis = 1;

int ConShowCursor()
{
    CurVis = 1;
    SLtt_set_cursor_visibility(1);

    return 1;
}
int ConHideCursor()
{
    CurVis = 0;
    SLtt_set_cursor_visibility(0);

    return 1;
}
int ConCursorVisible()
{
    return CurVis;
}

int ConSetCursorSize(int /*Start */ , int /*End */ )
{
    return 1;
}

int ConSetMousePos(int /*X */ , int /*Y */ )
{
    return 0;
}
int ConQueryMousePos(int *X, int *Y)
{
    *X = 0;
    *Y = 0;

    return 1;
}

int ConShowMouse()
{
    return 0;
}

int ConHideMouse()
{
    return 0;
}

int ConMouseVisible()
{
    return 1;
}

int ConQueryMouseButtons(int *ButtonCount)
{
    *ButtonCount = 0;

    return 1;
}

static int getkey(int tsecs)
{
    int key;

    if (SLang_input_pending(tsecs) > 0) {
	key = SLang_getkey();
	//fprintf(stderr, "readkey  0x%2x  %d  %c\n", key, key, isprint(key) ? key : ' ');
    } else
	key = 0;

    return key;
}

static int parseEsc(TEvent *Event)
{
    int key = getkey(0);
    char seq[8] = { (char)key, 0 };
    unsigned seqpos = 1;

    if ((key < 'a' && key) || key > 'z') {
	/* read whole Esc sequence */
	while (seqpos < 7 && (seq[seqpos] = (char)getkey(0))) {
	    if ((unsigned char)seq[seqpos] < ' ') {
		SLang_ungetkey((unsigned char)seq[seqpos]);
		break;
	    }
	    seqpos++;
	}
	seq[seqpos] = 0;
    }
    //if (seqpos > 1) fprintf(stderr, "Seq: %d  %s\n", seqpos, seq);
    if (seqpos == 5 && seq[0] == '[' && seq[1] == 'M') {
#ifdef CONFIG_MOUSE
	// FIXME: hardcoded timeouts
	static const int64_t timeout[] = {
	    300000,// 300ms  double
	    500000,// 500ms  tripple
	    700000,// 700ms  4 clicks
	    900000 // 900ms  5 clicks
	};
	static int64_t time_prev[FTE_ARRAY_SIZE(timeout)];
	struct timeval tv;
	gettimeofday(&tv, NULL);
	int64_t time_new = tv.tv_sec * 1000000 + tv.tv_usec;

	// only mouse clicks are reported
	Event->Mouse.What = evMouseDown;
	Event->Mouse.X = (unsigned char)seq[3] - 33;
	Event->Mouse.Y = (unsigned char)seq[4] - 33;
	Event->Mouse.Buttons = (seq[2] == 32) ? 1 : (seq[2] == 33) ? 4 : 2;

	// FIXME: this code should be moved to upper layer
	// detect 2,3,4 mouse clicks
	Event->Mouse.Count = 1;
	for (size_t i = 0; i < FTE_ARRAY_SIZE(timeout)
             && ((time_new - time_prev[i]) < timeout[i]); ++i)
		Event->Mouse.Count++;
	for (size_t i = FTE_ARRAY_SIZE(timeout); --i > 0;)
	    time_prev[i] = time_prev[i - 1];
	time_prev[0] = time_new;
	//fprintf(stderr, "B:%2d:%d  X:%3d  Y:%3d  Time %"PRId64 "\n",
	//	Event->Mouse.Buttons, Event->Mouse.Count, Event->Mouse.X, Event->Mouse.Y, time_new);

	if (seq[2] & 0x40) {
	    Event->What = evCommand;
	    Event->Msg.Param1 = 10;
	    Event->Msg.Command = (seq[2] & 1) ? cmVScrollDown : cmVScrollUp;
	} else {
	    NextEvent = *Event;
	    NextEvent.Mouse.What = evMouseUp;
	}
#endif // CONFIG_MOUSE
	return 1;
    }

    return TTYParseEsc(seq);
}

static TEvent Prev = { evNone };

int ConGetEvent(TEventMask /*EventMask */ ,
		TEvent * Event, int WaitTime, int Delete)
{
    TKeyEvent& KEvent = Event->Key;
    int key;

    if (ScreenSizeChanged) {
	ScreenSizeChanged = 0;
	SLtt_get_screen_size();
	SLsmg_reinit_smg();
	Event->What = evCommand;
	Event->Msg.Command = cmResize;
	return 1;
    }

    if (Prev.What != evNone) {
	*Event = Prev;
	if (Delete)
	    Prev.What = evNone;
	return 1;
    }

    WaitTime = (WaitTime >= 0) ? WaitTime / 100 : 36000;

    switch (WaitFdPipeEvent(Event, STDIN_FILENO, -1, WaitTime)) {
    case FD_PIPE_1: break;
    default: return 0;
    }

    Event->What = evKeyDown;

    key = getkey(0);
    if (isupper(key))
	key = kfShift | key;
    else if (key < 32) {
	switch (key) {
	case 0:
	    Event->What = evNone;
	    return -1;
	case 8:
	    key = kbBackSp;
	    break;
	case '\t':
	    key = kbTab;
	    break;
	case '\r':
	case '\n':
	    key = kbEnter;
	    break;
	case 27: // Esc
	    key = parseEsc(Event);
	    break;
	default:
	    key = kfCtrl | (key + 'A' - 1);
	}
    } else if (key == 127)
	key = kbBackSp;
    else if (key > 255)
	key = kbEsc;

    if (Event->What == evKeyDown) {
	KEvent.Code = key;
    //fprintf(stderr, "KEY %x \n", key);

	if (!Delete)
	    Prev = *Event;
    }

    return 1;
}

int ConPutEvent(const TEvent& Event)
{
    Prev = Event;

    return 1;
}

GUI::GUI(int &argc, char **argv, int XSize, int YSize)
{
    fArgc = argc;
    fArgv = argv;
    if (TTYInitTable() == 0) {
	::ConInit(-1, -1);
	::ConSetSize(XSize, YSize);
	gui = this;
    }
}

GUI::~GUI()
{
    ::ConDone();
    gui = 0;
}

int GUI::ConSuspend()
{
    return::ConSuspend();
}

int GUI::ConContinue()
{
    return::ConContinue();
}

int GUI::ShowEntryScreen()
{
    TEvent E;

    ConHideMouse();
    do {
	gui->ConGetEvent(evKeyDown, &E, -1, 1, 0);
    } while (E.What != evKeyDown);
    ConShowMouse();
    if (frames)
	frames->Repaint();

    return 1;
}

int GUI::RunProgram(int /*mode */ , char *Command)
{
    int rc, W, H, W1, H1;

    ConQuerySize(&W, &H);
    ConHideMouse();
    ConSuspend();

    if (*Command == 0)		// empty string = shell
	Command = getenv("SHELL");

    rc = system(Command);

    ConContinue();
    ConShowMouse();
    ConQuerySize(&W1, &H1);

    if (W != W1 || H != H1)
	frames->Resize(W1, H1);

    frames->Repaint();

    return rc;
}

char ConGetDrawChar(unsigned int idx)
{
    static const unsigned char tab[] = {
	DCH_SLANG_C1,
	DCH_SLANG_C2,
	DCH_SLANG_C3,
	DCH_SLANG_C4,
	DCH_SLANG_H,
	DCH_SLANG_V,
	DCH_SLANG_M1,
	DCH_SLANG_M2,
	DCH_SLANG_M3,
	DCH_SLANG_M4,
	DCH_SLANG_X,
	'>', // DCH_SLANG_RPTR
	DCH_SLANG_EOL,
	DCH_SLANG_EOF,
	DCH_SLANG_END,
	DCH_SLANG_AUP,
	DCH_SLANG_ADOWN,
	DCH_SLANG_HFORE,
	DCH_SLANG_HBACK,
	DCH_SLANG_ALEFT,
	DCH_SLANG_ARIGHT
    };

    static const unsigned char tab_linux[] = {
	DCH_SLANG_C1,
	DCH_SLANG_C2,
	DCH_SLANG_C3,
	DCH_SLANG_C4,
	DCH_SLANG_H,
	DCH_SLANG_V,
	DCH_SLANG_M1,
	DCH_SLANG_M2,
	DCH_SLANG_M3,
	DCH_SLANG_M4,
	DCH_SLANG_X,
	'>',
	'.',
	DCH_SLANG_EOF,
	DCH_SLANG_END,
	DCH_SLANG_AUP,
	DCH_SLANG_ADOWN,
	DCH_SLANG_HFORE,
	DCH_SLANG_HBACK,
	DCH_SLANG_ALEFT,
	DCH_SLANG_ARIGHT
    };
#if 0
    static const unsigned char tab[] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'j', 'k',
	'l', 'm', 'n', 'o', 'p', 'q'
    };
#endif
    static const char *use_tab;
    static size_t use_tab_size;

    if (!use_tab) {
	const char *c = getenv("TERM");
	use_tab = (const char*)
	    ((!c || strcmp(c, "linux") != 0) ? tab : tab_linux);
	use_tab = GetGUICharacters("Slang", use_tab);
	use_tab_size = strlen(use_tab);
    }

    assert(idx < use_tab_size);

    return use_tab[idx];
}

/*
 * Code bellow is no longer needed
 * - will be removed in future
 */
#if 0
    if (SLang_input_pending(0) > 0) {
	TKeyCode kcode = 0, kcode1;

	key = SLang_getkey();
	int escfirst = 1;

	if (key == 27)
	    while (1) {
		if (use_esc_hack) {
		    if (SLang_input_pending(1) == 0) {
			kcode = kbEsc;
			break;
		    }
		}

		key = SLang_getkey();
		if (key == 3) {
		    SLang_ungetkey((unsigned char)key);
		    SLkp_getkey();
		}
		if (key >= 'a' && key <= 'z')
		    key -= 'a' - 'A';
		if (key == 27) {
		    kcode = kbEsc;
		    break;
		} else if (key == '[' && escfirst) {
		    unsigned char kbuf[2];

		    kbuf[0] = 27;
		    kbuf[1] = (char) key;
		    SLang_ungetkey_string(kbuf, 2);
		    key = SLkp_getkey();
		    if (key == 0xFFFF) {
			if (SLang_input_pending(0) == 0) {
			    /*
			     * SLang got an unknown key and ate it.
			     * beep and bonk out.
			     */
			    SLtt_beep();
			    return -1;
			}
			/*
			 * SLang encountered an unknown key sequence, so we
			 * try to parse the sequence one by one and thus
			 * enable the user to configure a binding for it
			 */
			key = SLang_getkey();
			if (key != 27) {
			    SLtt_beep();
			    SLang_flush_input();
			    return -1;
			}
		    }
		    kcode = ftesl_process_key(key, 0);
		    break;
		} else {
		    kcode1 = ftesl_process_key(key, 1);
		    if (keyCode(kcode1) == kbF1) {
			key = SLang_getkey();
			switch (key) {
			case '0':
			    kcode |= kbF10;
			    break;
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			    kcode |= kbF1 + key - '1';
			    break;
			case 'a':
			case 'b':
			    kcode |= kbF11 + key - 'a';
			    break;
			}
		    } else
			kcode |= kcode1;

		    if (keyCode(kcode) != 0) {
			if (escfirst)
			    kcode |= kfAlt;
			break;
		    }
		}
		escfirst = 0;
	} else {
	    SLang_ungetkey((unsigned char)key);
	    key = SLkp_getkey();
	    kcode = ftesl_process_key(key, 0);
	}

	Event->What = evKeyDown;
	KEvent->Code = kcode;

	if (!Delete)
	    Prev = *Event;

	return 1;
    }

    return -1;
#endif

#if 0
/*
 * Definitions for keyboard handling under SLang.
 */

#define FTESL_KEY            0x00001000		// A key defined by me
#define FTESL_KEY_SHIFT      0x00002000		// Key with Shift
#define FTESL_KEY_CTRL       0x00004000		// Key with Ctrl
#define FTESL_KEY_ALT        0x00008000		// Key with Alt
#define FTESL_KEY_GRAY       0x00010000		// Gray Key

#define FTESL_KEY_ENTER      13
#define FTESL_KEY_TAB         9
#define FTESL_KEY_ESC        27
#define FTESL_KEY_BACKSP      8

#define FTESL_KEY_CTRLAND(x)    (x+1-'a')

static int use_esc_hack = 0;

static const TKeyCode speckeys[] =
{
    kbF1,
    kbF2,
    kbF3,
    kbF4,
    kbF5,
    kbF6,
    kbF7,
    kbF8,
    kbF9,
    kbF10,
    kbF11,
    kbF12,
    kbHome,
    kbEnd,
    kbPgUp,
    kbPgDn,
    kbIns,
    kbDel,
    kbUp,
    kbDown,
    kbLeft,
    kbRight,
    kbEnter,
    kbEsc,
    kbBackSp,
    kbSpace,
    kbTab,
    kbCenter,
};

/*
static int ftesl_getkeysym(TKeyCode keycode)
{
    unsigned key = keyCode(keycode);
    int ksym = -1;

    for (unsigned i = 0; i < sizeof(speckeys) / sizeof(TKeyCode); i++) {
	if (key == speckeys[i]) {
	    ksym = (int) i;
	    break;
	}
    }

    if (ksym < 0 && key < 256) {
	ksym = (int) key;
    }

    if (ksym < 0)
	return ksym;

    if (keycode & kfAlt)
	ksym |= FTESL_KEY_ALT;
    if (keycode & kfCtrl)
	ksym |= FTESL_KEY_CTRL;
    if (keycode & kfShift)
	ksym |= FTESL_KEY_SHIFT;
    if (keycode & kfGray)
	ksym |= FTESL_KEY_GRAY;

    ksym |= FTESL_KEY;
    return ksym;
}
*/

static const TKeyCode keys_ctrlhack[] =
{
    kfAlt,  // A
    kbHome, // B
    kfCtrl,
    kbDown,
    kbEnd,

    kbF1,
    kfCtrl | 'G',
    kbBackSp,
    kbTab,
    kfCtrl | 'J',

    kfCtrl | 'K',
    kbLeft,
    kbEnter,
    kbPgDn,
    kfCtrl | 'O',

    kbPgUp,
    kbIns,
    kbRight,
    kfShift,
    kfCtrl | 'T',

    kbUp,
    kfCtrl | 'V',
    kfCtrl | 'W',
    kbCenter,
    kfCtrl | 'Y',

    kbDel,
    kbEsc,
    kbCtrl | '\\',
    kbCtrl | ']',
    kbCtrl | '^',
    kbCtrl | '_'
};

static TKeyCode ftesl_getftekey(unsigned char key)
{
    if (key < 32)
	return speckeys[key];
    else
	return (TKeyCode) key;
}

/*
 * Keyboard handling with SLang.
 */
static TKeyCode ftesl_process_key(int key, int ctrlhack = 0)
{
    TKeyCode kcode;

    //fprintf(stderr, "KEY  %03d \n", key);
    if (key < 256 && key >= 32) {
	return (TKeyCode) key;
    } else if (key >= 1 && key <= 31 && key != 13 && key != 9 && key != 8
	       && key != 27) {
	if (!ctrlhack)
	    return ((key + 'A' - 1) & 0xff) | kfCtrl;
	else
	    return keys_ctrlhack[key - 1];
    } else if (key & FTESL_KEY) {
	kcode = ftesl_getftekey((unsigned char)key);
	if (key & FTESL_KEY_SHIFT)
	    kcode |= kfShift;
	if (key & FTESL_KEY_CTRL)
	    kcode |= kfCtrl;
	if (key & FTESL_KEY_ALT)
	    kcode |= kfAlt;
	if (key & FTESL_KEY_GRAY)
	    kcode |= kfGray;
	return kcode;
    } else
	switch (key) {
	case SL_KEY_UP:
	    return kbUp;
	case SL_KEY_DOWN:
	    return kbDown;
	case SL_KEY_LEFT:
	    return kbLeft;
	case SL_KEY_RIGHT:
	    return kbRight;
	case SL_KEY_PPAGE:
	    return kbPgUp;
	case SL_KEY_NPAGE:
	    return kbPgDn;
	case SL_KEY_HOME:
	    return kbHome;
	case SL_KEY_END:
	    return kbEnd;
	case SL_KEY_BACKSPACE:
	case FTESL_KEY_BACKSP:
	    return kbBackSp;
	case SL_KEY_ENTER:
	case FTESL_KEY_ENTER:
	    return kbEnter;
	case SL_KEY_IC:
	    return kbIns;
	case SL_KEY_DELETE:
	    return kbDel;
	case SL_KEY_F(1):
	    return kbF1;
	case SL_KEY_F(2):
	    return kbF2;
	case SL_KEY_F(3):
	    return kbF3;
	case SL_KEY_F(4):
	    return kbF4;
	case SL_KEY_F(5):
	    return kbF5;
	case SL_KEY_F(6):
	    return kbF6;
	case SL_KEY_F(7):
	    return kbF7;
	case SL_KEY_F(8):
	    return kbF8;
	case SL_KEY_F(9):
	    return kbF9;
	case SL_KEY_F(10):
	    return kbF10;
	case SL_KEY_F(11):
	    return kbF11;
	case SL_KEY_F(12):
	    return kbF12;
	case FTESL_KEY_TAB:
	    return kbTab;
	case FTESL_KEY_ESC:
	case SL_KEY_ERR:
	    return kbEsc;
	default:
	    return '?';
	}
}
#endif
