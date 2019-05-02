/*    console.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#include "fte.h"

/* don't change these, used as index */
enum {
    DCH_C1,	/// upper left corner
    DCH_C2,	/// upper right corner
    DCH_C3,	/// lower left corner
    DCH_C4,	/// lower right corner
    DCH_H,	/// horizontal line
    DCH_V,	/// vertical line
    DCH_M1,	/// tee pointing down
    DCH_M2,	/// tee pointing right
    DCH_M3,	/// tee pointing left
    DCH_M4,	/// tee pointing up
    DCH_X,	/// crossover  10
    DCH_RPTR,	/// arrow pointing right
    DCH_EOL,	/// usually print as bullet
    DCH_EOF,	/// usually print as diamond
    DCH_END,	///
    DCH_AUP,	/// arrow pointing up
    DCH_ADOWN,	/// arrow pointing down
    DCH_HFORE,	/// full square block
    DCH_HBACK,	/// checker board (stipple)
    DCH_ALEFT,	/// arrow pointing left
    DCH_ARIGHT	/// arrow pointing right
};

#define ConMaxCols 256
#define ConMaxRows 128

enum TEventScroll {
    csUp,
    csDown,
    csLeft,
    csRight
};

#define evNone             0
#define evKeyDown     0x0001
#define evKeyUp       0x0002
#define evMouseDown   0x0010
#define evMouseUp     0x0020
#define evMouseMove   0x0040
#define evMouseAuto   0x0080
#define evCommand     0x0100
#define evBroadcast   0x0200
#define evNotify      0x0400

#define evKeyboard    (evKeyDown | evKeyUp)
#define evMouse       (evMouseDown | evMouseUp | evMouseMove | evMouseAuto)
#define evMessage     (evCommand | evBroadcast)

#include "conkbd.h"

enum TEventCommands {
    cmRefresh = 1,
    cmResize,
    cmClose,
    cmPipeRead,
    cmMainMenu,
    cmPopupMenu, // 6

    /* vertical scroll */

    cmVScrollUp, // 10
    cmVScrollDown,
    cmVScrollPgUp,
    cmVScrollPgDn,
    cmVScrollMove, // 14

/* horizontal scroll */

    cmHScrollLeft, // 15
    cmHScrollRight,
    cmHScrollPgLt,
    cmHScrollPgRt,
    cmHScrollMove, // 19

    cmDroppedFile = 30,
    cmRenameFile   /* TODO: in-place editing of titlebar */
};

typedef unsigned char TAttr;
typedef unsigned char TChar;

// we need to use class instead of casting to short
// otherwice we would need to resolve CPU ordering issues
#ifdef NTCONSOLE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
class TCell : public CHAR_INFO {
public:
    TCell(char c = ' ', TAttr a = 0x07) {
        Char.AsciiChar = c;
        Attributes = a;
    }
    char GetChar() const { return Char.AsciiChar; }
    TAttr GetAttr() const { return Attributes; }
    void SetChar(char c) { Char.AsciiChar = c; }
    void SetAttr(TAttr a) { Attributes = a; }
    void Set(char c, TAttr a) { SetChar(c); SetAttr(a); }
    bool operator==(const TCell &c) const {
	return (Char.AsciiChar == c.Char.AsciiChar
		&& Attributes == c.Attributes);
    }
};
#else
class TCell {
    TChar Char;
    TAttr Attr;
    operator const char*();
    operator const unsigned char*();
    operator unsigned char*();
    operator char*();
public:
    TCell(TChar c = ' ', TAttr a = 0x07) : Char(c), Attr(a) {}
    TChar GetChar() const { return Char; }
    TAttr GetAttr() const { return Attr; }
    void SetChar(TChar c) { Char = c; }
    void SetAttr(TAttr a) { Attr = a; }
    void Set(TChar c, TAttr a) { SetChar(c); SetAttr(a); }
    bool operator==(const TCell &c) const {
	return (Char == c.Char && Attr == c.Attr);
    }
};
#endif

typedef TCell *PCell;
typedef TCell TDrawBuffer[ConMaxCols];
typedef TDrawBuffer *PDrawBuffer;
typedef unsigned long TEventMask;
typedef unsigned long TKeyCode;
typedef unsigned long TCommand;

class EModel; // forward
class GView;

struct TKeyEvent {
    TEventMask What;
    GView* View;
    TKeyCode Code;
};

struct TMouseEvent {
    TEventMask What;
    GView* View;
    int X;
    int Y;
    unsigned short Buttons;
    unsigned short Count;
    TKeyCode KeyMask;
};

struct TMsgEvent {
    TEventMask What;
    GView *View;
    EModel *Model;
    TCommand Command;
    long Param1;
    void *Param2;
};

union TEvent {
    TEventMask What;
    TKeyEvent Key;
    TMouseEvent Mouse;
    TMsgEvent Msg;
    char fill[32];
};

#define SUBMENU_NORMAL      (-1)
#define SUBMENU_CONDITIONAL (-2)

struct mItem {
    char *Name;
    char *Arg;
    int SubMenu;
    int Cmd;
};

struct mMenu {
    char *Name;
    unsigned Count;
    mItem *Items;
};

extern int MenuCount;
extern mMenu *Menus;

int ConInit(int XSize, int YSize);
int ConDone();
int ConSuspend();
int ConContinue();
int ConSetTitle(const char *Title, const char *STitle);
int ConGetTitle(char *Title, size_t MaxLen, char *STitle, size_t SMaxLen);

int ConClear();
int ConPutBox(int X, int Y, int W, int H, PCell Cell);
int ConGetBox(int X, int Y, int W, int H, PCell Cell);
int ConPutLine(int X, int Y, int W, int H, PCell Cell);
int ConSetBox(int X, int Y, int W, int H, TCell Cell);
int ConScroll(int Way, int X, int Y, int W, int H, TAttr Fill, int Count);

int ConSetSize(int X, int Y);
int ConQuerySize(int *X, int *Y);

int ConSetCursorPos(int X, int Y);
int ConQueryCursorPos(int *X, int *Y);
int ConShowCursor();
int ConHideCursor();
int ConCursorVisible();
int ConSetCursorSize(int Start, int End);

#ifdef CONFIG_MOUSE
int ConSetMousePos(int X, int Y);
int ConQueryMousePos(int *X, int *Y);
int ConShowMouse();
int ConHideMouse();
int ConMouseVisible();
int ConQueryMouseButtons(int *ButtonCount);
#endif

int ConGetEvent(TEventMask EventMask, TEvent* Event, int WaitTime, int Delete);
int ConPutEvent(const TEvent& Event);

void MoveCh(PCell B, char Ch, TAttr Attr, size_t Count);
void MoveChar(PCell B, int Pos, int Width, const char Ch, TAttr Attr, size_t Count);
void MoveMem(PCell B, int Pos, int Width, const char* Ch, TAttr Attr, size_t Count);
void MoveStr(PCell B, int Pos, int Width, const char* Ch, TAttr Attr, size_t MaxCount);
void MoveCStr(PCell B, int Pos, int Width, const  char* Ch, TAttr A0, TAttr A1, size_t MaxCount);
void MoveAttr(PCell B, int Pos, int Width, TAttr Attr, size_t Count);
void MoveBgAttr(PCell B, int Pos, int Width, TAttr Attr, size_t Count);

size_t CStrLen(const char *s);

int NewMenu(const char *Name);
int NewItem(int menu, const char *Name);
int NewSubMenu(int menu, const char *Name, int submenu, int type);
int GetMenuId(const char *Name);

char ConGetDrawChar(unsigned int index);

extern char WindowFont[64];

struct TRGBColor {
    unsigned char r, g, b;
};

extern TRGBColor RGBColor[16];
extern bool RGBColorValid[16];

#endif // CONSOLE_H
