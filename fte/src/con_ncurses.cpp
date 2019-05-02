/*    con_ncurses.cpp
 *
 *    Ncurses front-end to fte - Don Mahurin
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "c_config.h"
#include "con_tty.h"
#include "gui.h"
#include "o_model.h"  // Msg
#include "s_string.h"
#include "s_util.h"  // S_ERROR
#include "sysdep.h"

#include <ncurses.h>

#include <signal.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <termios.h>

/* Escape sequence delay in milliseconds */
#define escDelay 10

extern TEvent NextEvent; // g_text.cpp

/* translate fte colors to curses*/
static const short fte_curses_colors[] =
{
	COLOR_BLACK,
	COLOR_BLUE,
	COLOR_GREEN,
	COLOR_CYAN,
	COLOR_RED,
	COLOR_MAGENTA,
	COLOR_YELLOW,
	COLOR_WHITE,
};


static PCell *SavedScreen = 0;
static int MaxSavedW = 0, MaxSavedH = 0;

static int ScreenSizeChanged;
static void curses_winch_handler(int signum)
{
	ScreenSizeChanged = 1;
	signal(SIGWINCH, curses_winch_handler);
}


/* Routine to allocate/reallocate and zero space for screen buffer,
   represented as a dynamic array of dynamic PCell lines */
static void SaveScreen() {
	int NewSavedW, NewSavedH;

	ConQuerySize(&NewSavedW, &NewSavedH);
	//fprintf(stderr, "SAVESCREEN  %d %d\n", NewSavedW, NewSavedH);

	if (SavedScreen && NewSavedW > MaxSavedW) { /* Expand and zero maximum width if needed */
		for (int i = 0 ; i < MaxSavedH; ++i) {
			SavedScreen[i] = (PCell)realloc(SavedScreen[i], NewSavedW * sizeof(TCell));
			memset(SavedScreen[i] + MaxSavedW, 0, (NewSavedW - MaxSavedW) * sizeof(TCell));
		}
	}
	MaxSavedW = NewSavedW;

	if (NewSavedH > MaxSavedH) { /* Expand and zero maximum height if needed */
		SavedScreen = (PCell *)realloc(SavedScreen, NewSavedH * sizeof(PCell));
		for (int i = MaxSavedH; i < NewSavedH; ++i) {
			SavedScreen[i] = (PCell)malloc(MaxSavedW * sizeof(TCell));
			memset(SavedScreen[i], 0, MaxSavedW * sizeof(TCell));
		}
		MaxSavedH = NewSavedH;
	}
}

static void ReleaseScreen()
{
	if (!SavedScreen)
		return;
	for (int i = 0; i < MaxSavedH; i++)
		if (SavedScreen[i])
			free(SavedScreen[i]);
	free(SavedScreen);
	SavedScreen = NULL;
	MaxSavedH = MaxSavedW = 0;
}

static int fte_curses_attr[256];

static int key_sup = 0;
static int key_sdown = 0;

static int InitColors()
{
	int c = 0;
	int colors = has_colors();

	if (colors)
		start_color();

	for (int bgb = 0 ; bgb < 2; bgb++) { /* bg bright bit */
		for (int bg = 0 ; bg < 8; bg++) {
			for (int fgb = 0; fgb < 2; fgb++) { /* fg bright bit */
				for (int fg = 0 ; fg < 8; fg++, c++) {
					if (colors) {
						short pair = short(bg * 8 + fg);
						if (c)
							init_pair(pair, fte_curses_colors[fg], fte_curses_colors[bg]);
						fte_curses_attr[c] = unsigned(fgb ? A_BOLD : 0) | COLOR_PAIR(pair);
					} else {
						if (fgb || bgb)
							fte_curses_attr[c] = (bg > fg) ?
								(A_UNDERLINE | A_REVERSE) : A_BOLD;
						else if (bg > fg)
							fte_curses_attr[c] = A_REVERSE;
						else
							fte_curses_attr[c] = 0;
					}
				}
			}
		}
	}
	return colors;
}

int ConInit(int /*XSize */ , int /*YSize */ )
{
	int ch;
	const char *s;

	ESCDELAY = escDelay;
	initscr();
	InitColors();
#ifdef CONFIG_MOUSE
	mouseinterval(200);
#if 1
	mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
#else
	mousemask(BUTTON1_CLICKED | BUTTON2_CLICKED | BUTTON3_CLICKED |
		  BUTTON4_CLICKED
#if NCURSES_MOUSE_VERSION > 1
		  | BUTTON5_CLICKED
#endif
		  , 0);
#endif
#endif
	/*    cbreak (); */
	raw();
	noecho();
	nonl();
	keypad(stdscr,1);
	meta(stdscr,1);
	SaveScreen();
	signal(SIGWINCH, curses_winch_handler);

	/* find shift up/down */
	for (ch = KEY_MAX +1; ; ch++) {
		if (!(s = keyname(ch)))
		      break;

		if (!strcmp(s, "kUP"))
			key_sup = ch;
		else if (!strcmp(s, "kDN"))
			key_sdown = ch;
		if (key_sup > 0 && key_sdown > 0)
			break;
	}
	return 1;
}


int ConDone()
{
	keypad(stdscr,0);
	endwin();
	ReleaseScreen();
	return 1;
}

int ConSuspend()
{
	return 1;
}

int ConContinue()
{
	return 1;
}

int ConSetTitle(const char * /*Title */, const char * /*STitle */)
{
	return 1;
}

int ConGetTitle(char *Title, size_t MaxLen, char *STitle, size_t SMaxLen)
{
	strlcpy(Title, "", MaxLen);
	strlcpy(STitle, "", SMaxLen);
	return 1;
}

int ConClear() /* not used? */
{
	clear();
	refresh();
	return 1;
}

static chtype GetDch(int idx)
{
	switch(idx) {
	case DCH_C1: return ACS_ULCORNER;
	case DCH_C2: return ACS_URCORNER;
	case DCH_C3: return ACS_LLCORNER;
	case DCH_C4: return ACS_LRCORNER;
	case DCH_H: return ACS_HLINE;
	case DCH_V: return ACS_VLINE;
	case DCH_M1: return ACS_TTEE;
	case DCH_M2: return ACS_LTEE;
	case DCH_M3: return ACS_RTEE;
	case DCH_M4 : return ACS_BTEE;
	case DCH_X: return ACS_PLUS;
	case DCH_RPTR: return ACS_RARROW;
	case DCH_EOL: return ACS_BULLET;
	case DCH_EOF: return ACS_DIAMOND;
	case DCH_END: return ACS_HLINE;
	case DCH_AUP: return ACS_UARROW;
	case DCH_ADOWN: return ACS_DARROW;
	case DCH_HFORE: return ' ';// return ACS_BLOCK;
	case DCH_HBACK: return ACS_CKBOARD;
	case DCH_ALEFT: return ACS_LARROW;
	case DCH_ARIGHT: return ACS_RARROW;
	default: return '@';
	}
}

int ConPutBox(int X, int Y, int W, int H, PCell Cell)
{
	int last_attr = ~0;
	int CurX, CurY;
	ConQueryCursorPos(&CurX, &CurY);
	//static int x = 0;
	//fprintf(stderr, "%5d: refresh done  %d  %d   %d x %d   L:%d C:%d\n", x++, X, Y, W, H, LINES, COLS);

	if (Y + H > LINES)
		H = LINES - Y;

	for (int j = 0; j < H; j++) {
		memcpy(SavedScreen[Y + j] + X, Cell, W * sizeof(TCell)); // copy outputed line to saved screen
		move(Y + j, X);
		for (int i = 0; i < W; ++i) {
			unsigned char ch = Cell->GetChar();
			int attr = fte_curses_attr[Cell->GetAttr()];
			if (attr != last_attr) {
				wattrset(stdscr, attr);
				last_attr = attr;
			}

			if (ch < 32)
				waddch(stdscr, (ch <= 20) ? GetDch(ch) : '.');
			else if (ch < 128 || ch >= 160)
				waddch(stdscr, ch);
			/*		    else if(ch < 180)
			 {
			 waddch(stdscr,GetDch(ch-128));
			 }
			 */
			else
				waddch(stdscr, '.');
			Cell++;
		}
	}

	return ConSetCursorPos(CurX, CurY);
}

int ConGetBox(int X, int Y, int W, int H, PCell Cell)
{
	for (int j = 0 ; j < H; Cell += W, ++j)
		memcpy(Cell, SavedScreen[Y+j]+X, W*sizeof(TCell));

	return 1;
}

int ConPutLine(int X, int Y, int W, int H, PCell Cell)
{
	for (int j = 0 ; j < H; ++j)
		ConPutBox(X, Y+j, W, 1, Cell);

	return 1;
}

int ConSetBox(int X, int Y, int W, int H, TCell Cell)
{
	TCell line[W];

	for (int i = 0; i < W; ++i)
		line[i] = Cell;
	ConPutLine(X, Y, W, H, line);
	return 1;
}



int ConScroll(int Way, int X, int Y, int W, int H, TAttr Fill, int Count)
{
	TCell box[W * H];
	ConGetBox(X, Y, W, H, box);

	//TCell fill(' ', Fill);

	if (Way == csUp) {
		ConPutBox(X, Y, W, H - Count, box + W * Count);
		//ConSetBox(X, Y + H - Count, W, Count, fill);
	} else {
		ConPutBox(X, Y + Count, W, H - Count, box);
		//ConSetBox(X, Y, W, Count, fill);
	}

	return 1;
}

int ConSetSize(int /*X */ , int /*Y */ )
{
	return -1;
}

int ConQuerySize(int *X, int *Y)
{
	*X = COLS;
	*Y = LINES;
	return 1;
}

int ConSetCursorPos(int X, int Y)
{
	move(Y,X);
	refresh();
	return 1;
}

int ConQueryCursorPos(int *X, int *Y)
{
	getyx(stdscr, *Y, *X);
	return 1;
}

static int CurVis = 1;

int ConShowCursor()
{
	CurVis = 1;
	curs_set(1);
	refresh();
	return 1;
}
int ConHideCursor()
{
	CurVis = 0;
	curs_set(0);
	refresh();
	return 1;
}
int ConCursorVisible()
{
	return CurVis;
}

int ConSetCursorSize(int /*Start */ , int /*End */ )
{
	return 0;
}

#ifdef CONFIG_MOUSE
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

static int ConGetMouseEvent(TEvent *Event)
{
	MEVENT mevent;
	mmask_t& bstate = mevent.bstate;

	if (getmouse(&mevent) == ERR) {
		 Event->What = evNone;
		 return 0;
	}
#if 0
	fprintf(stderr, "EVENT %x  %x %x  %d  %d\n", (int)bstate,
		(int)BUTTON_CTRL, (int)BUTTON_CTRL, (int)mevent.x, (int)mevent.y);
	for (int i = 1; i <= 5; ++i)
		fprintf(stderr, "EVENTXXX %d R:%lx P:%lx C:%lx D:%lx T:%lx E:%lx\n",
			i, BUTTON_RELEASE(bstate, i),
			BUTTON_PRESS(bstate, i),
			BUTTON_CLICK(bstate, i),
			BUTTON_DOUBLE_CLICK(bstate, i),
			BUTTON_TRIPLE_CLICK(bstate, i),
			BUTTON_RESERVED_EVENT(bstate, i));
#endif
	Event->What = evNone;
	Event->Mouse.X = mevent.x;
	Event->Mouse.Y = mevent.y;
	Event->Mouse.Count = 0;
        int i;
	for (i = 1; i < 3; ++i) {
	    if (BUTTON_CLICK(bstate, i)) {
		Event->Mouse.Count = 1;
                break;
	    } else if (BUTTON_DOUBLE_CLICK(bstate, i)) {
		Event->Mouse.Count = 2;
                break;
	    } else if (BUTTON_TRIPLE_CLICK(bstate, i)) {
		Event->Mouse.Count = 3;
                break;
	    }
	}

	//fprintf(stderr, "BUTTONS b:%d c:%d\n", i, (int)Event->Mouse.Count);
	if (Event->Mouse.Count) {
	    switch (i) {
	    case 1: Event->Mouse.Buttons = 1; break;
	    case 2: Event->Mouse.Buttons = 2; break;
	    case 3: Event->Mouse.Buttons = 4; break;
	    }
	    Event->Mouse.What = evMouseDown;
	    NextEvent = *Event;
	    NextEvent.Mouse.What = evMouseUp;
	} else if (BUTTON_DOUBLE_CLICK(bstate, 5) || BUTTON_PRESS(bstate, 4)) {
	    Event->What = evCommand;
	    Event->Msg.Param1 = 10;
	    /* HACK - with control move in opposite direction
	     * as the 'upward' scroll is really slow with plain Button4
	     */
	    Event->Msg.Command = (bstate & (BUTTON_CTRL | BUTTON4_PRESSED))
		? cmVScrollUp : cmVScrollDown;
	}

	return 1;
}
#endif // CONFIG_MOUSE

static TEvent Prev = { evNone };

#if 1
static int ConGetEscEvent()
{
	int key;

	if ((key = getch()) == ERR)
		return kbEsc;

	if (key >= 'a' && key <= 'z' ) /* Alt-A == Alt-a*/
		return (kfAlt | (key + 'A' - 'a'));

	char seq[8] = { (char)key };
	unsigned seqpos = 1;

	while (seqpos < 7 && (key = getch()) != ERR)
		seq[seqpos++] = (char)key;

	seq[seqpos] = 0;

	if (!(key = TTYParseEsc(seq)))
	    ;//Msg(S_ERROR, "Sequence '%s' not found.", seq);

	return key;
}

#else
/*
 * Routine would have to be far more complex
 * to handle majority of esq sequences
 *
 * Using string table and TTYEscParse
 */
static int ConGetEscEvent()
{
	int ch;
	int code = 0;

	if ((ch = getch()) == 27) {
		ch = getch();
		if (ch == '[' || ch == 'O')
			code |= kfAlt;
	}

	if (ch == ERR)
		return kbEsc;

	if (ch == '[' || ch == 'O') {
		int ch1 = getch();
		int ch2 = 0;
		if (ch1 >= '1' && ch1 <= '8') {
			if ((ch2 = getch()) == ';') {
				if ((ch1 = getch()) == ERR
                                    || ch1 < '1' || ch1 > '8')
                                        return kbEsc;
				ch2 = 0;
			}
		}

		if (ch1 == ERR) /* translate to Alt-[ or Alt-O */
			return  (kfAlt | ch);

		if (ch2 == '~' || ch2 == '$') {
			if (ch2 == '$')
				 code |= kfShift;
			switch (ch1 - '0')
			{
				case 1: code |= kbHome; break;
				case 2: code |= kbIns; break;
				case 3: code |= kbDel; break;
				case 4: code |= kbEnd; break;
				case 5: code |= kbPgUp; break;
				case 6: code |= kbPgDn; break;
				case 7: code |= kbHome; break;
				case 8: code |= kbEnd; break;
				default: code = 0; break;
			}
		} else {
			if (ch2) {
				int ctAlSh = ch2 - '1';
				if (ctAlSh & 0x4) code |= kfCtrl;
				if (ctAlSh & 0x2) code |= kfAlt;
				if (ctAlSh & 0x1) code |= kfShift;
			}

			switch(ch1) {
			case 'A': code |= kbUp; break;
			case 'B': code |= kbDown; break;
			case 'C': code |= kbRight; break;
			case 'D': code |= kbLeft; break;
			case 'F': code |= kbEnd; break;
			case 'H': code |= kbHome; break;
			case 'a': code |= (kfShift | kbUp); break;
			case 'b': code |= (kfShift | kbDown); break;
			case 'c': code |= (kfShift | kbRight); break;
			case 'd': code |= (kfShift | kbLeft); break;
			default:  code = 0; break;
			}
		}
	} else {
		code |= kfAlt;
		if(ch == '\r' || ch == '\n') code |= kbEnter;
		else if(ch == '\t') code |= kbTab;
		else if(ch < 32) code |=  (kfCtrl | (ch+ 0100));
		else {
			if(ch > 0x60 && ch < 0x7b ) /* Alt-A == Alt-a*/
				ch -= 0x20;
			code |= ch;
		}
	}

	return code;
}
#endif

static int ResizeWindow()
{
	struct winsize {
	    unsigned short ws_row;
	    unsigned short ws_col;
	    unsigned short ws_xpixel;   /* unused */
	    unsigned short ws_ypixel;
	} ws = { 0 };

	ScreenSizeChanged = 0;

	if (ioctl(1, TIOCGWINSZ, &ws) == -1
	    || !ws.ws_row || !ws.ws_col)
	    return 0;

	if (is_term_resized(ws.ws_row, ws.ws_col)) {
	    LINES = ws.ws_row;
	    COLS = ws.ws_col;
	    resize_term(LINES, COLS);
	    SaveScreen();
	    ConClear();
	    //fprintf(stderr, "SIGNAL RESIZE  %d  %d\n", COLS, LINES);
	}

	return 1;
}

// *INDENT-OFF*
int ConGetEvent(TEventMask /*EventMask */, TEvent* Event, int WaitTime, int Delete)
{
    TKeyEvent* KEvent = &(Event->Key);

    if (ScreenSizeChanged && (ResizeWindow() > 0)) {
	Event->What = evCommand;
	Event->Msg.Command = cmResize;
	return 1;
    }

    if (WaitTime == 0)
	return 0;

    if (Prev.What != evNone) {
	*Event = Prev;
	if (Delete)
	    Prev.What = evNone;
	return 1;
    }

    if (WaitFdPipeEvent(Event, STDIN_FILENO, -1, -1) ==	FD_PIPE_ERROR)
	return 0;

    if (Event->What == evNotify)
	return 0; // pipe reading

    int ch = wgetch(stdscr);
    Event->What = evKeyDown;
    KEvent->Code = 0;

    //fprintf(stderr, "READKEY %d %s\n", ch, keyname(ch));
    if (SevenBit && (ch > 127) && (ch < 256)) {
	KEvent->Code |= kfAlt;
	ch -= 128;
	if ((ch > 0x60) && (ch < 0x7b)) /* Alt-A == Alt-a*/
	    ch -= 0x20;
    }

    if (ch < 0) Event->What = evNone;
    else if (ch == 27) {
	//fprintf(stderr, "ESCAPEEVENT %x\n", (int)ch);
	keypad(stdscr, 0);
	timeout(escDelay);
	if (!(KEvent->Code = ConGetEscEvent()))
	    Event->What = evNone;
	timeout(-1);
	keypad(stdscr, 1);
    } else if ((ch == '\n') || (ch == '\r'))
	KEvent->Code |= kbEnter;
    else if (ch == '\t')
	KEvent->Code |= kbTab;
    else if ((ch >= 'A') && (ch <= 'Z'))
	KEvent->Code |= kfShift | ch;
    else if (ch < 32)
	KEvent->Code |= (kfCtrl | (ch + 'A' - 1)); // +0100
    else if (ch < 256)
	KEvent->Code |= ch;
    else   // > 255
	switch (ch) {
#ifdef CONFIG_MOUSE
	case KEY_MOUSE:
	    Event->What = evNone;
	    ConGetMouseEvent(Event);
	    break;
#endif
	case KEY_RESIZE: break; // already handled
	case KEY_UP: KEvent->Code = kbUp; break;
	case KEY_SR: KEvent->Code = kfShift | kbUp; break;
	case 559: KEvent->Code = kfAlt | kbUp; break; // kUP3
	case 561: KEvent->Code = kfCtrl | kbUp; break; // kUP5

	case KEY_DOWN: KEvent->Code = kbDown; break;
	case KEY_SF: KEvent->Code = kfShift | kbDown; break;
	case 518: KEvent->Code = kfAlt | kbDown; break; // kDN3
	case 520: KEvent->Code = kfCtrl | kbDown; break; // kDN5

	case KEY_RIGHT: KEvent->Code = kbRight; break;
	case KEY_SRIGHT: KEvent->Code = kfShift | kbRight; break;
	case 553: KEvent->Code = kfAlt | kbRight; break;  // kRIT3
	case 555: KEvent->Code = kfCtrl | kbRight; break; // kRIT5
	case 556: KEvent->Code = kfCtrl | kfShift | kbRight; break; // kRIT6

	case KEY_LEFT: KEvent->Code = kbLeft; break;
	case KEY_SLEFT: KEvent->Code = kfShift | kbLeft; break;
	case 538: KEvent->Code = kfAlt | kbLeft; break; // kLFT3
	case 540: KEvent->Code = kfCtrl | kbLeft; break; // kLFT5
	case 541: KEvent->Code = kfCtrl | kfShift | kbLeft; break; // kLFT6

	case KEY_DC: KEvent->Code = kbDel; break;
	case KEY_SDC: KEvent->Code = kfShift | kbDel; break;

	case KEY_IC: KEvent->Code = kbIns; break;
	case KEY_SIC: KEvent->Code = kfShift | kbIns; break;

	case KEY_BACKSPACE: KEvent->Code = kbBackSp; break;

	case KEY_HOME: KEvent->Code = kbHome; break;
	case KEY_SHOME: KEvent->Code = kfShift | kbHome; break;
	case 528: KEvent->Code = kfCtrl | kbHome; break; // kHOM3
	case 530: KEvent->Code = kfAlt | kbHome; break; // kHOM5
	case 532: KEvent->Code = kfAlt | kfCtrl | kbHome; break; // kHOM7

	case KEY_LL: // used in old termcap/infos
	case KEY_END: KEvent->Code = kbEnd; break;
	case KEY_SEND: KEvent->Code = kfShift | kbEnd; break;
	case 523: KEvent->Code = kfAlt | kbEnd; break; // kEND3
	case 524: KEvent->Code = kfAlt | kfShift | kbEnd; break; // kEND4
	case 525: KEvent->Code = kfCtrl | kbEnd; break; // kEND5
	case 526: KEvent->Code = kfCtrl | kfShift | kbEnd; break; // kEND6
	case 527: KEvent->Code = kfAlt | kfCtrl | kbEnd; break; // kEND7

	case KEY_NPAGE: KEvent->Code = kbPgDn; break;
	case KEY_SNEXT: KEvent->Code = kfShift | kbPgDn; break;
	case 543: KEvent->Code = kfAlt | kbPgDn; break; // kNXT3
	case 545: KEvent->Code = kfCtrl | kbPgDn; break; // kNXT5
	case 547: KEvent->Code = kfAlt | kfCtrl | kbPgDn; break; // kNXT7

	case KEY_PPAGE: KEvent->Code = kbPgUp; break;
	case KEY_SPREVIOUS: KEvent->Code = kfShift | kbPgUp; break;
	case 548: KEvent->Code = kfAlt | kbPgUp; break; // kPRV3
	case 550: KEvent->Code = kfCtrl | kbPgUp; break; // kPRV5
	case 552: KEvent->Code = kfAlt | kfCtrl | kbPgUp; break; // kPRV7

	case KEY_F(1): KEvent->Code = kbF1; break;
	case KEY_F(13): KEvent->Code = kfShift | kbF1; break;
	case KEY_F(25): KEvent->Code = kfCtrl | kbF1; break;
	case KEY_F(37): KEvent->Code = kfCtrl | kfShift | kbF1; break;

	case KEY_F(2): KEvent->Code = kbF2; break;
	case KEY_F(14): KEvent->Code = kfShift | kbF2; break;
	case KEY_F(26): KEvent->Code = kfCtrl | kbF2; break;
	case KEY_F(38): KEvent->Code = kfCtrl | kfShift | kbF2; break;

	case KEY_F(3): KEvent->Code = kbF3; break;
	case KEY_F(15): KEvent->Code = kfShift | kbF3; break;
	case KEY_F(27): KEvent->Code = kfCtrl | kbF3; break;
	case KEY_F(39): KEvent->Code = kfCtrl | kfShift | kbF3; break;

	case KEY_F(4): KEvent->Code = kbF4; break;
	case KEY_F(16): KEvent->Code = kfShift | kbF4; break;
	case KEY_F(28): KEvent->Code = kfCtrl | kbF4; break;
	case KEY_F(40): KEvent->Code = kfCtrl | kfShift | kbF4; break;

	case KEY_F(5): KEvent->Code = kbF5; break;
	case KEY_F(17): KEvent->Code = kfShift | kbF5; break;
	case KEY_F(29): KEvent->Code = kfCtrl | kbF5; break;
	case KEY_F(41): KEvent->Code = kfCtrl | kfShift | kbF5; break;

	case KEY_F(6): KEvent->Code = kbF6; break;
	case KEY_F(18): KEvent->Code = kfShift | kbF6; break;
	case KEY_F(30): KEvent->Code = kfCtrl | kbF6; break;
	case KEY_F(42): KEvent->Code = kfCtrl | kfShift | kbF6; break;

	case KEY_F(7): KEvent->Code = kbF7; break;
	case KEY_F(19): KEvent->Code = kfShift | kbF7; break;
	case KEY_F(31): KEvent->Code = kfCtrl | kbF7; break;
	case KEY_F(43): KEvent->Code = kfCtrl | kfShift | kbF7; break;

	case KEY_F(8): KEvent->Code = kbF8; break;
	case KEY_F(20): KEvent->Code = kfShift | kbF8; break;
	case KEY_F(32): KEvent->Code = kfCtrl | kbF8; break;
	case KEY_F(44): KEvent->Code = kfCtrl | kfShift | kbF8; break;

	case KEY_F(9): KEvent->Code = kbF9; break;
	case KEY_F(21): KEvent->Code = kfShift | kbF9; break;
	case KEY_F(33): KEvent->Code = kfCtrl | kbF9; break;
	case KEY_F(45): KEvent->Code = kfCtrl | kfShift | kbF9; break;

	case KEY_F(10): KEvent->Code = kbF10; break;
	case KEY_F(22): KEvent->Code = kfShift | kbF10; break;
	case KEY_F(34): KEvent->Code = kfCtrl | kbF10; break;
	case KEY_F(46): KEvent->Code = kfCtrl | kfShift | kbF10; break;

	case KEY_F(11): KEvent->Code = kbF11; break;
	case KEY_F(23): KEvent->Code = kfShift | kbF11; break;
	case KEY_F(35): KEvent->Code = kfCtrl | kbF11; break;
	case KEY_F(47): KEvent->Code = kfCtrl | kfShift | kbF11; break;

	case KEY_F(12): KEvent->Code = kbF12; break;
	case KEY_F(24): KEvent->Code = kfShift | kbF12; break;
	case KEY_F(36): KEvent->Code = kfCtrl | kbF12; break;
	case KEY_F(48): KEvent->Code = kfCtrl | kfShift | kbF12; break;

	case KEY_B2: KEvent->Code = kbCenter; break;
	case KEY_ENTER: KEvent->Code = kbEnter; break; /* shift enter */

	default:
	    if ((key_sdown != 0) && (ch == key_sdown))
		KEvent->Code = kfShift | kbDown;
	    else if ((key_sup != 0) && (ch == key_sup))
		KEvent->Code = kfShift | kbUp;
	    else {
		Event->What = evNone;
		//fprintf(stderr, "Unknown 0x%x %d\n", ch, ch);
	    }
	    break;
	}

    if (!Delete)
	Prev = *Event;

    return 1;
}
// *INDENT-ON*

char ConGetDrawChar(unsigned int idx)
{
	//return 128+idx;
	return (char)idx;
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
	return ::ConSuspend();
}

int GUI::ConContinue()
{
	return ::ConContinue();
}

int GUI::ShowEntryScreen()
{
	return 1;
}

int GUI::RunProgram(int /*mode */ , char *Command)
{
	int rc, W, H, W1, H1;

	ConQuerySize(&W, &H);
#ifdef CONFIG_MOUSE
	ConHideMouse();
#endif
	ConSuspend();

	if (!Command[0]) // empty string = shell
		Command = getenv("SHELL");

	rc = system(Command);

	ConContinue();
#ifdef CONFIG_MOUSE
	ConShowMouse();
#endif
	ConQuerySize(&W1, &H1);

	if (W != W1 || H != H1)
		frames->Resize(W1, H1);

	frames->Repaint();

	return rc;
}
