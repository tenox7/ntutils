/*    gui.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef GUI_H
#define GUI_H

#include "console.h"

#include <stdarg.h>

class GFramePeer;
class GViewPeer;
class GUI;
class GFrame;

class GView {
public:
    GFrame *Parent;
    GView *Next, *Prev;
    GViewPeer *Peer;
    int Result;
    
    GView(GFrame *parent, int XSize, int YSize);
    virtual ~GView();

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
    
    int CaptureMouse(int grab);
    int CaptureFocus(int grab);
    
    virtual int Execute();
    void EndExec(int NewResult);
    
    int QuerySbVPos();
    int SetSbVPos(int Start, int Amount, int Total);
    int SetSbHPos(int Start, int Amount, int Total);
    int ExpandHeight(int DeltaY);
    
    int IsActive();
    
    virtual void Update();
    virtual void Repaint();
    virtual void Activate(int gotfocus);
    virtual void Resize(int width, int height);
    virtual void HandleEvent(TEvent &Event);
};

class GFrame {
public:
    GFrame *Prev, *Next;
    GView *Top, *Active;
    GFramePeer *Peer;
    char *Menu;
    
    GFrame(int XSize, int YSize);
    virtual ~GFrame();
    
    int ConSetTitle(const char *Title, const char *STitle);
    int ConGetTitle(char *Title, size_t MaxLen, char *STitle, size_t SMaxLen);
    
    int ConSetSize(int X, int Y);
    int ConQuerySize(int *X, int *Y);
    
    int AddView(GView *view);
    int ConSplitView(GView *view, GView *newview);
    int ConCloseView(GView *view);
    int ConResizeView(GView *view, int DeltaY);
    int SelectView(GView *view);
    
    virtual void Update();
    virtual void Repaint();
    virtual void UpdateMenu();
    
    void InsertView(GView *Prev, GView *view);
    void RemoveView(GView *view);
    void SelectNext(int back);
    
    void Resize(int width, int height);
    void DrawMenuBar();
    
    int ExecMainMenu(char Sub);
    int SetMenu(const char *Name);
    char *QueryMenu();
    int PopupMenu(const char *Name);

    void Show();
    void Activate();

    int isLastFrame();
};

class GUI {
public:
    enum {
	RUN_WAIT,
	RUN_ASYNC
    };

    GUI(int &argc, char **argv, int XSize, int YSize);
    virtual ~GUI();
    
    int ConSuspend();
    int ConContinue();
    int ShowEntryScreen();
    
    void ProcessEvent();
    virtual void DispatchEvent(GFrame *frame, GView *view, TEvent &Event);
    int ConGetEvent(TEventMask EventMask, TEvent* Event, int WaitTime, int Delete, GView **view);
    int ConPutEvent(const TEvent& Event);
    int ConFlush();
    int ConGrabEvents(TEventMask EventMask);

    virtual int Start(int &argc, char **argv);
    virtual void Stop();

    int Run();
    void StopLoop();

    int RunProgram(int mode, char *Command);
    
    int OpenPipe(const char *Command, EModel *notify);
    int SetPipeView(int id, EModel *notify);
    ssize_t ReadPipe(int id, void *buffer, size_t len);
    int ClosePipe(int id);

    int multiFrame();

    int fArgc;
    char **fArgv;
    int doLoop;
};

extern GFrame *frames;
extern GUI *gui;

#define GUIDLG_CHOICE      0x00000001
#define GUIDLG_PROMPT      0x00000002
#define GUIDLG_PROMPT2     0x00000004
#define GUIDLG_FILE        0x00000008
#define GUIDLG_FIND        0x00000010
#define GUIDLG_FINDREPLACE 0x00000020

extern unsigned long HaveGUIDialogs;

void DieError(int rc, const char *msg, ...);

#define GF_OPEN    0x0001
#define GF_SAVEAS  0x0002

int DLGGetStr(GView *View, const char *Prompt, unsigned int BufLen, char *Str, int HistId, int Flags);
int DLGGetFile(GView *View, const char *Prompt, unsigned int BufLen, char *FileName, int Flags);

#define GPC_NOTE    0x0000
#define GPC_CONFIRM 0x0001
#define GPC_WARNING 0x0002
#define GPC_ERROR   0x0004
#define GPC_FATAL   0x0008
int DLGPickChoice(GView *View, const char *ATitle, int NSel, va_list ap, int Flags);

#define SEARCH_BACK    0x00000001   // reverse (TODO for regexps)
#define SEARCH_RE      0x00000002   // use regexp
#define SEARCH_NCASE   0x00000004   // case
#define SEARCH_GLOBAL  0x00000008   // start from begining (or end if BACK)
#define SEARCH_BLOCK   0x00000010   // search in block
#define SEARCH_NEXT    0x00000020   // next match
#define SEARCH_NASK    0x00000040   // ask before replacing
#define SEARCH_ALL     0x00000080   // search all
#define SEARCH_REPLACE 0x00000100   // do a replace operation
#define SEARCH_JOIN    0x00000200   // join line
#define SEARCH_DELETE  0x00000400   // delete line
#define SEARCH_CENTER  0x00001000   // center finds
#define SEARCH_NOPOS   0x00002000   // don't move the cursor
#define SEARCH_WORDBEG 0x00004000   // match at beginning of words only
#define SEARCH_WORDEND 0x00008000   // match at end of words only
#define SEARCH_WORD    (SEARCH_WORDBEG | SEARCH_WORDEND)
//0x00000800   // search words
//#define SEARCH_LINE    0x00002000   // search on current line only TODO
//#define SEARCH_WRAP    0x00004000   // similiar to GLOBAL, but goes to start
// only when match from current position fails TODO
//#define SEARCH_BOL     0x00008000   // search at line start
//#define SEARCH_EOL     0x00010000   // search at line end

#define MAXSEARCH 512

struct SearchReplaceOptions {
    int ok;
    int resCount;
    int lastInsertLen;
    unsigned long Options;

    //enum { MAXSEARCH = 512 };
    char strSearch[MAXSEARCH];
    char strReplace[MAXSEARCH];

    SearchReplaceOptions();
};

int DLGGetFind(GView *View, SearchReplaceOptions &sr);
int DLGGetFindReplace(GView *View, SearchReplaceOptions &sr);

enum WaitFdPipe {
    FD_PIPE_ERROR,
    FD_PIPE_1,
    FD_PIPE_2,
    FD_PIPE_EVENT,
    FD_PIPE_TIMEOUT
};
WaitFdPipe WaitFdPipeEvent(TEvent *Event, int fd, int fd2, int WaitTime);

#endif // GUI_H
