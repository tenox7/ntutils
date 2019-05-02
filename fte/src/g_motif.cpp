/*    g_motif.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *    Copyright (c) 2010, Zdenek Kabelac
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 *    Only demo code
 */

#include "c_config.h"
#include "console.h"
#include "gui.h"
#include "s_string.h"

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#ifdef HPUX
#include </usr/include/X11R5/X11/HPkeysym.h>
#endif

#include <Xm/Xm.h>

#include <Xm/BulletinB.h>
#include <Xm/CascadeB.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/MainW.h>
#include <Xm/MainW.h>
#include <Xm/PanedW.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrolledW.h>
#include <Xm/Separator.h>
#include <Xm/Text.h>

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#define DEBUG(x)
//printf x

#define MAX_PIPES 4
//#define PIPE_BUFLEN 4096

typedef struct {
    int used;
    int id;
    int fd;
    int pid;
    int stopped;
    EModel *notify;
    XtInputId input;
} GPipe;

static GPipe Pipes[MAX_PIPES] = {
    { 0 }, { 0 }, { 0 }, { 0 }
};

#define sfFocus   1

static long MouseAutoDelay = 40;
static long MouseAutoRepeat = 200;
static long MouseMultiClick = 300;

static Atom WM_DELETE_WINDOW;
static Atom XA_CLIPBOARD;

class GViewPeer {
public:
    Widget ScrollWin;
    Widget TextWin;
    Widget SbHorz, SbVert;
    GC gc[256];
    XGCValues gcv;
    //    TAttr GCattr;
    int Visibility;

    GView *View;
//    int wX, wY;
    int wW, wH, wState, wRefresh;
    int cX, cY, cVisible, cStart, cEnd;
    int sbVstart, sbVamount, sbVtotal;
    int sbHstart, sbHamount, sbHtotal;
    int VertPos, HorzPos;
    char *ScreenBuffer;

    GViewPeer(GView *view, int XSize, int YSize);
    ~GViewPeer();

    int AllocBuffer();
    void DrawCursor(int Show);
    void UpdateWindow(int xx, int yy, int ww, int hh);

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

    int QuerySbVPos();
    int SetSbVPos(int Start, int Amount, int Total);
    int SetSbHPos(int Start, int Amount, int Total);
    int ExpandHeight(int DeltaY);

    int UpdateCursor();
    int PMShowCursor();
    int PMHideCursor();
    int PMSetCursorPos();
};

class GFramePeer {
public:
    GFrame *Frame;
    Widget ShellWin, MainWin, PanedWin, MenuBar;

    GFramePeer(GFrame *frame, int Width, int Height);
    ~GFramePeer();

    int ConSetTitle(const char *Title, const char *STitle);
    int ConGetTitle(char *Title, size_t MaxLen, char *STitle, size_t SMaxLen);

    int ConSetSize(int X, int Y);
    int ConQuerySize(int *X, int *Y);
    void MapFrame();
};

int ShowVScroll = 1;
int ShowHScroll = 0;
int ShowMenuBar = 1;
int ShowToolBar = 0;

GFrame *frames = 0;
GUI *gui = 0;

unsigned long HaveGUIDialogs = 0;

static GView *MouseCapture = 0;
static GView *FocusCapture = 0;

static int cxChar = 8;
static int cyChar = 13;

static XtAppContext AppContext;
static Widget TopLevel;
static Display *display;
static Window root;
static int screen;
static Colormap colormap;
static XColor Colors[16];
static XFontStruct *fontStruct;

TEvent EventBuf = { evNone };

TEvent NextEvent = { evNone };

XButtonEvent LastRelease;
Widget LPopupMenu = 0;

static int LastMouseX = -1, LastMouseY = -1;

static void SetColor(int i) {
    int j, k, z;
    j = i & 7;
    k = 65535 - 20480;
    z = (i > 7) ? (20480) : 0;
    Colors[i].blue  = k * (j & 1) + z;
    Colors[i].green = k * ((j & 2)?1:0) + z;
    Colors[i].red   = k * ((j & 4)?1:0) + z;
    Colors[i].flags = DoRed | DoGreen | DoBlue;
}

static int InitXColors() {
    int i, j;
    long d = 0, d1;
    XColor clr;
    long pix;
    int num;
    long d_red, d_green, d_blue;
    unsigned long u_red, u_green, u_blue;

    for (i = 0; i < 16; i++) {
        SetColor(i);
        if (XAllocColor(display, colormap, &Colors[i]) == 0) {
            SetColor(i);
            pix = -1;
            num = DisplayCells(display, DefaultScreen(display));
            for (j = 0; j < num; j++) {
                clr.pixel = j;
                XQueryColor(display, colormap, &clr);

                d_red = clr.red - Colors[i].red;
                d_green = clr.green - Colors[i].green;
                d_blue = clr.blue - Colors[i].blue;

                //fprintf(stderr, "%d:%d dr:%d, dg:%d, db:%d\n", i, j, d_red, d_green, d_blue);

                u_red = d_red / 10 * d_red;
                u_green = d_green / 10 * d_green;
                u_blue = d_blue / 10 * d_blue;

                //fprintf(stderr, "%d:%d dr:%u, dg:%u, db:%u\n", i, j, u_red, u_green, u_blue);

                d1 = u_red + u_blue + u_green;

                if (d1 < 0)
                    d1 = -d1;
                if (pix == -1 || d1 < d) {
                    pix = j;
                    d = d1;
                }
            }
            if (pix == -1) {
                fprintf(stderr, "Color search failed for #%04X%04X%04X\n",
                        Colors[i].red,
                        Colors[i].green,
                        Colors[i].blue);
            }
            clr.pixel = pix;
            XQueryColor(display, colormap, &clr);
            Colors[i] = clr;
            if (XAllocColor(display, colormap, &Colors[i]) == 0) {
                fprintf(stderr, "Color alloc failed for #%04X%04X%04X\n",
                        Colors[i].red,
                        Colors[i].green,
                        Colors[i].blue);
            }
        }
    }
    return 0;
}

// *INDENT-OFF*
static const struct {
    long keysym;
    long keycode;
} key_table[] = {
    { XK_Escape,         kbEsc },
    { XK_Tab,            kbTab },
    { XK_Return,         kbEnter },
    { XK_Pause,          kbPause },
    { XK_BackSpace,      kbBackSp },
    { XK_Home,           kbHome },
    { XK_Up,             kbUp },
    { XK_Prior,          kbPgUp },
    { XK_Left,           kbLeft },
    { XK_Right,          kbRight },
    { XK_End,            kbEnd },
    { XK_Down,           kbDown },
    { XK_Next,           kbPgDn },
    { XK_Select,         kbEnd },
    { XK_KP_Enter,       kbEnter | kfGray },
    { XK_Insert,         kbIns | kfGray },
    { XK_Delete,         kbDel | kfGray },
    { XK_KP_Add,         '+' | kfGray },
    { XK_KP_Subtract,    '-' | kfGray },
    { XK_KP_Multiply,    '*' | kfGray },
    { XK_KP_Divide,      '/' | kfGray },
    { XK_Num_Lock,       kbNumLock },
    { XK_Caps_Lock,      kbCapsLock },
    { XK_Print,          kbPrtScr },
    { XK_Shift_L,        kbShift },
    { XK_Shift_R,        kbShift | kfGray },
    { XK_Control_L,      kbCtrl },
    { XK_Control_R,      kbCtrl | kfGray },
    { XK_Alt_L,          kbAlt },
    { XK_Alt_R,          kbAlt | kfGray },
    { XK_Meta_L,         kbAlt },
    { XK_Meta_R,         kbAlt | kfGray },
    { XK_F1,             kbF1 },
    { XK_F2,             kbF2 },
    { XK_F3,             kbF3 },
    { XK_F4,             kbF4 },
    { XK_F5,             kbF5 },
    { XK_F6,             kbF6 },
    { XK_F7,             kbF7 },
    { XK_F8,             kbF8 },
    { XK_F9,             kbF9 },
    { XK_F10,            kbF10 },
    { XK_F11,            kbF11 },
    { XK_F12,            kbF12 },
    { XK_KP_0,           '0' | kfGray },
    { XK_KP_1,           '1' | kfGray },
    { XK_KP_2,           '2' | kfGray },
    { XK_KP_3,           '3' | kfGray },
    { XK_KP_4,           '4' | kfGray },
    { XK_KP_5,           '5' | kfGray },
    { XK_KP_6,           '6' | kfGray },
    { XK_KP_7,           '7' | kfGray },
    { XK_KP_8,           '8' | kfGray },
    { XK_KP_9,           '9' | kfGray },
    { XK_KP_Decimal,     '.' | kfGray },
    { 0x1000FF6F,        kbDel | kfShift | kfGray },
    { 0x1000FF70,        kbIns | kfCtrl | kfGray },
    { 0x1000FF71,        kbIns | kfShift | kfGray },
    { 0x1000FF72,        kbIns | kfGray },
    { 0x1000FF73,        kbDel | kfGray },
    { 0x1000FF74,        kbTab | kfShift },
    { 0x1000FF75,        kbTab | kfShift },
    { 0, 0 }
};
// *INDENT-ON*

static void ConvertKeyToEvent(KeySym key, KeySym key1, char *keyname, int etype, int state, TEvent *Event) {
    unsigned int myState = 0;
    int k;


    DEBUG(("key: \n"));
    Event->What = evNone;

    switch (etype) {
    case KeyPress:   Event->What = evKeyDown; break;
    case KeyRelease: Event->What = evKeyUp; break;
    }

    if (state & ShiftMask) myState |= kfShift;
    if (state & ControlMask) myState |= kfCtrl;
    if (state & Mod1Mask) myState |= kfAlt;
    if (state & Mod2Mask) myState |= kfAlt;
    if (state & Mod3Mask) myState |= kfAlt;
    if (state & Mod4Mask) myState |= kfAlt;

    DEBUG(("key: %d ; %d ; %d\n", key, key1, state));
    if (key < 256) {
        if (myState == kfShift) myState = 0;
        if (myState & kfCtrl) {
            if (((key >= 'A') && (key < 'A' + 32)) ||
                ((key >= 'a') && (key < 'a' + 32)))
                key &= 0x1F;
        }
        if (myState & kfAlt) {
            if (((key >= 'A') && (key <= 'Z')) ||
                ((key >= 'a') && (key <= 'z')))
                key &= ~0x20;
        }
        Event->Key.Code = key | myState;
        return;
    } else {
        for (size_t i = 0; i < FTE_ARRAY_SIZE(key_table); ++i) {
            if (key1 == key_table[i].keysym) {
                k = key_table[i].keycode;
                if (k < 256)
                    if (myState == kfShift) myState = 0;
                Event->Key.Code = k | myState;
                return;
            }
        }
    }
    DEBUG(("Unknown key: %ld %s %d %d\n", key, keyname, etype, state));
    Event->What = evNone;
}


static TEvent LastMouseEvent = { evNone };

#define TM_DIFF(x,y) ((long)(((long)(x) < (long)(y)) ? ((long)(y) - (long)(x)) : ((long)(x) - (long)(y))))

static void ConvertClickToEvent(int type, int xx, int yy, int button, int state, TEvent *Event, Time time) {
    unsigned int myState = 0;
    static unsigned long LastClickTime = 0;
    static unsigned long LastClickCount = 0;
    static unsigned long LastClick = 0;
    unsigned long CurTime = time;

    if (type == MotionNotify) Event->What = evMouseMove;
    else if (type == ButtonPress) Event->What = evMouseDown;
    else Event->What = evMouseUp;
    Event->Mouse.X = xx / cxChar;
    Event->Mouse.Y = yy / cyChar;
    if (Event->What == evMouseMove)
        if (LastMouseX == Event->Mouse.X &&
            LastMouseY == Event->Mouse.Y)
        {
            Event->What = evNone;
            return;
        }
    LastMouseX = Event->Mouse.X;
    LastMouseY = Event->Mouse.Y;
    Event->Mouse.Buttons = 0;
    if (type == MotionNotify) {
        if (state & Button1Mask) Event->Mouse.Buttons |= 1;
        if (state & Button2Mask) Event->Mouse.Buttons |= 4;
        if (state & Button3Mask) Event->Mouse.Buttons |= 2;
    } else {
        switch (button) {
        case Button1: Event->Mouse.Buttons |= 1; break;
        case Button2: Event->Mouse.Buttons |= 4; break;
        case Button3: Event->Mouse.Buttons |= 2; break;
        }
    }
    Event->Mouse.Count = 1;
    if (state & ShiftMask) myState |= kfShift;
    if (state & ControlMask) myState |= kfCtrl;
    if (state & Mod1Mask) myState |= kfAlt;
    if (state & Mod2Mask) myState |= kfAlt;
    if (state & Mod3Mask) myState |= kfAlt;
    if (state & Mod4Mask) myState |= kfAlt;
    Event->Mouse.KeyMask = myState;

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
        LastClickTime = CurTime;
    }
    /*    if (Event->What == evMouseMove) {
     LastClick = 0;
     LastClickCount = 0;
     LastClickTime = 0;
     }
     */
    DEBUG(("Mouse: %d %d %d\n", Event->What, Event->Mouse.X, Event->Mouse.Y));
    LastMouseEvent = *Event;
}

static void ProcessXEvents(XEvent *event, TEvent *Event, GViewPeer *Peer) {
    XAnyEvent *anyEvent = (XAnyEvent *) event;
    XExposeEvent *exposeEvent = (XExposeEvent *) event;
    XButtonEvent *buttonEvent = (XButtonEvent *) event;
    XKeyEvent *keyEvent = (XKeyEvent *) event;
    XKeyEvent keyEvent1;
    XConfigureEvent *configureEvent = (XConfigureEvent *) event;
    XGraphicsExposeEvent *gexposeEvent = (XGraphicsExposeEvent *) event;
    XMotionEvent *motionEvent = (XMotionEvent *) event;
    KeySym key, key1;
    int state;
    char keyName[32];
    char keyName1[32];
    static int hasConfig = 0;

    Event->What = evNone;
    Event->Msg.View = Peer->View;

    switch (event->type) {
    case ButtonRelease:
    case ButtonPress:
        LastRelease = *buttonEvent;
        ConvertClickToEvent(event->type, buttonEvent->x, buttonEvent->y, buttonEvent->button, buttonEvent->state, Event, motionEvent->time);
        break;
    case KeyPress:
    case KeyRelease:
        state = keyEvent->state;

        keyEvent1 = *keyEvent;
        keyEvent1.state &= ~(ShiftMask | ControlMask | Mod1Mask | Mod2Mask | Mod3Mask | Mod4Mask);

        XLookupString(keyEvent, keyName, sizeof(keyName), &key, 0);
        XLookupString(&keyEvent1, keyName1, sizeof(keyName1), &key1, 0);
        //key1 = XLookupKeysym(keyEvent, 0);
        ConvertKeyToEvent(key, key1, keyName, event->type, state, Event);
        break;
    case MotionNotify:
        ConvertClickToEvent(event->type, motionEvent->x, motionEvent->y, 0, motionEvent->state, Event, motionEvent->time);
        break;
    }
}

static void CloseWindow(Widget w, void *framev, XEvent *event, Boolean *cont)
{
    GFramePeer *frame = (GFramePeer*)framev;
    if (event->type != ClientMessage ||
        ((XClientMessageEvent *)event)->data.l[0] != WM_DELETE_WINDOW)
        return;

    NextEvent.What = evCommand;
    NextEvent.Msg.Command = cmClose;
    *cont = False;
}

static void MainCallback (Widget w, void* itemv, XtPointer callData)
{
    mItem *item = (mItem*) itemv;
    DEBUG(("Main: %d\n", item->Cmd));
    NextEvent.What = evCommand;
    NextEvent.Msg.Command = item->Cmd;
}

static void PopupCallback (Widget w, void* itemv, XtPointer callData)
{
    mItem *item = (mItem*) itemv;
    DEBUG(("Popup: %d\n", item->Cmd));
    NextEvent.What = evCommand;
    NextEvent.Msg.Command = item->Cmd;
}

static void MenuPopdownCb (Widget w, void* itemv, XtPointer callData)
{
    mItem *item = (mItem*) itemv;
    DEBUG(("Popdown: %d\n", item->Cmd));
    if (LPopupMenu != 0) {
        XtDestroyWidget(LPopupMenu);
        LPopupMenu = 0;
    }
}

static void InputWindow(Widget w, void *PeerV, XEvent *event, Boolean *cont)
{
    GViewPeer *Peer = (GViewPeer*)PeerV;

    DEBUG(("Input\n"));
    if (!Peer->View->IsActive())
        Peer->View->Parent->SelectView(Peer->View);
    NextEvent.Msg.View = Peer->View;
    ProcessXEvents(event, &NextEvent, Peer);
    *cont = False;
}

static void VisibilityCb(Widget w, void* peerv, XEvent *event, Boolean *cont)
{
    GViewPeer *peer = (GViewPeer*)peerv;

    if (event->type != VisibilityNotify)
        return;

    peer->Visibility = event->xvisibility.state;
    DEBUG(("Visibility %d\n", peer->Visibility));

    /*
     * When we do an XCopyArea(), and the window is partially obscured, we want
     * to receive an event to tell us whether it worked or not.
     */
    /*XSetGraphicsExposures(display, gui.text_gc,
     gui.visibility != VisibilityUnobscured);*/
    *cont = True;
}

static void ConfigureWindow(Widget w, void *PeerV, XEvent *event, Boolean *cont)
{
    GViewPeer *Peer = (GViewPeer*)PeerV;
    XConfigureEvent *confEvent = (XConfigureEvent*)event;
    int X, Y;

    DEBUG(("Configure\n"));
    X = confEvent->width / cxChar;
    Y = confEvent->height / cyChar;
    DEBUG(("!! Resize %d, %d\n", X, Y));
    if (X > 0 && Y > 0) {
        Peer->ConSetSize(X, Y);
        NextEvent.What = evCommand;
        NextEvent.Msg.Command = cmResize;
    }
    DEBUG(("!! resize done\n"));
    *cont = True;
}

static void ExposeWindow(Widget w, void* PeerV, void* CallV)
{
    GViewPeer *Peer = (GViewPeer*)PeerV;
    XmDrawingAreaCallbackStruct *Call = (XmDrawingAreaCallbackStruct*)CallV;
    XExposeEvent *exposeEvent = (XExposeEvent *) Call->event;

    DEBUG(("Expose\n"));
    //    if (!XtIsManaged(w)) return
    Peer->UpdateWindow(exposeEvent->x,
                       exposeEvent->y,
                       exposeEvent->width,
                       exposeEvent->height);
    DEBUG(("! Expose done\n"));
}


static void VertValueChanged(Widget w, void* PeerV, void* CallV)
{
    GViewPeer *Peer = (GViewPeer*)PeerV;
    XmScrollBarCallbackStruct *Call = (XmScrollBarCallbackStruct*)CallV;

    if (!Peer->View->IsActive())
        Peer->View->Parent->SelectView(Peer->View);
    if (Peer->VertPos != Call->value) {
        NextEvent.What = evCommand;
        NextEvent.Msg.View = Peer->View;
        NextEvent.Msg.Command = cmVScrollMove;
        NextEvent.Msg.Param1 = Call->value;
        DEBUG(("Vert: %d\n", Call->value));
        Peer->VertPos = Call->value;
    }
}

//void HorzValueChanged(Widget w, GViewPeer *Peer, XmScrollBarCallbackStruct *Call)
void HorzValueChanged(Widget w, void* PeerV, void* CallV)
{
    GViewPeer *Peer = (GViewPeer*)PeerV;
    XmScrollBarCallbackStruct *Call = (XmScrollBarCallbackStruct*) CallV;

    if (!Peer->View->IsActive())
        Peer->View->Parent->SelectView(Peer->View);
    if (Call->value != Peer->HorzPos) {
        NextEvent.What = evCommand;
        NextEvent.Msg.View = Peer->View;
        NextEvent.Msg.Command = cmHScrollMove;
        NextEvent.Msg.Param1 = Call->value;
        DEBUG(("Horz: %d\n", Call->value));
        Peer->HorzPos = Call->value;
    }
}

///////////////////////////////////////////////////////////////////////////

GViewPeer::GViewPeer(GView *view, int XSize, int YSize) :
    Visibility(VisibilityFullyObscured),
    View(view),
    //wX(0),
    //wY(0),
    //wW(XSize),
    //wH(YSize),
    wW(80),
    wH(40),
    wState(0),
    wRefresh(0),
    cX(-1),
    cY(-1),
    cVisible(1),
    cStart(0), // %
    cEnd(100),
    sbVstart(0),
    sbVamount(0),
    sbVtotal(0),
    sbHstart(0),
    sbHamount(0),
    sbHtotal(0),
    VertPos(-1),
    HorzPos(-1),
    ScreenBuffer(0)
{
    for (int jj = 0; jj < 256; jj++)
        gc[jj] = 0;

    AllocBuffer();

    ScrollWin = XtVaCreateWidget("ScrollWin",
                                 xmScrolledWindowWidgetClass, frames->Peer->PanedWin,
                                 XmNmarginHeight, 0,
                                 XmNmarginWidth, 0,
                                 XmNspacing, 0,
                                 NULL);

    TextWin = XtVaCreateManagedWidget ("TextWin",
                                       xmDrawingAreaWidgetClass, ScrollWin,
                                       XmNmarginHeight, 0,
                                       XmNmarginWidth, 0,
                                       XmNhighlightThickness, 0,
                                       XmNshadowThickness, 0,
                                       XmNwidthInc,  cxChar,
                                       XmNheightInc, cyChar,
                                       XmNwidth, cxChar * 80,
                                       XmNheight, cyChar * 30,
                                       NULL);

    // XSetWindowColormap(display, XtWindow(TextWin), colormap);

    XtVaSetValues (ScrollWin,
                   XmNworkWindow, TextWin,
                   NULL);

    SbVert = XtVaCreateManagedWidget("VScrollBar",
                                     xmScrollBarWidgetClass, ScrollWin,
                                     XmNmarginHeight, 0,
                                     XmNmarginWidth, 0,
                                     XmNwidth, 20,
                                     NULL);

    SbHorz = XtVaCreateManagedWidget("HScrollBar",
                                     xmScrollBarWidgetClass, ScrollWin,
                                     XmNmarginHeight, 0,
                                     XmNmarginWidth, 0,
                                     XmNorientation, XmHORIZONTAL,
                                     NULL);

    XtVaSetValues(ScrollWin,
                  XmNhorizontalScrollBar, SbHorz,
                  XmNverticalScrollBar, SbVert,
                  NULL);

    XtManageChild(ScrollWin);

    gcv.foreground = Colors[0].pixel;
    gcv.background = Colors[0].pixel;
    gcv.font = fontStruct->fid;
    gc[0] = XtGetGC(TextWin,
                    GCForeground | GCBackground | GCFont, &gcv);
    //    GCattr = 0x07;


    /*    XtAddCallback(TextWin, XmNinputCallback, InputWindow, this);*/

    XtAddEventHandler(TextWin,
                      KeyPressMask | KeyReleaseMask |
                      ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
                      False, InputWindow, this);
    XtAddEventHandler(TextWin,
                      VisibilityChangeMask,
                      False, VisibilityCb,
                      this);
    XtAddEventHandler(TextWin,
                      StructureNotifyMask,
                      False, ConfigureWindow, this);
    //XtAddCallback(TextWin, XmNresizeCallback, ResizeWindow, this);
    XtAddCallback(TextWin, XmNexposeCallback, ExposeWindow, this);

    XtAddCallback(SbHorz, XmNvalueChangedCallback, HorzValueChanged, this);
    XtAddCallback(SbHorz, XmNdragCallback, HorzValueChanged, this);
    XtAddCallback(SbHorz, XmNincrementCallback, HorzValueChanged, this);
    XtAddCallback(SbHorz, XmNdecrementCallback, HorzValueChanged, this);
    XtAddCallback(SbHorz, XmNpageIncrementCallback, HorzValueChanged, this);
    XtAddCallback(SbHorz, XmNpageDecrementCallback, HorzValueChanged, this);
    XtAddCallback(SbHorz, XmNtoTopCallback, HorzValueChanged, this);
    XtAddCallback(SbHorz, XmNtoBottomCallback, HorzValueChanged, this);

    XtAddCallback(SbVert, XmNvalueChangedCallback, VertValueChanged, this);
    XtAddCallback(SbVert, XmNdragCallback, VertValueChanged, this);
    XtAddCallback(SbVert, XmNincrementCallback, VertValueChanged, this);
    XtAddCallback(SbVert, XmNdecrementCallback, VertValueChanged, this);
    XtAddCallback(SbVert, XmNpageIncrementCallback, VertValueChanged, this);
    XtAddCallback(SbVert, XmNpageDecrementCallback, VertValueChanged, this);
    XtAddCallback(SbVert, XmNtoTopCallback, VertValueChanged, this);
    XtAddCallback(SbVert, XmNtoBottomCallback, VertValueChanged, this);
}

GViewPeer::~GViewPeer() {
    // Widget destroyed recursively
    // from master shell widget
    free(ScreenBuffer);
    ScreenBuffer = 0;
}

int GViewPeer::AllocBuffer() {
    int i;
    char *p;
    /* FIXME */
    if (!(ScreenBuffer = (char *)malloc(wW * wH * sizeof(TCell))))
        return 0;
    for (i = 0, p = ScreenBuffer; i < wW * wH; i++) {
        *p++ = 32;
        *p++ = 0x07;
    }

    return 1;
}

#define InRange(x,a,y) (((x) <= (a)) && ((a) < (y)))
#define CursorXYPos(x,y) (ScreenBuffer + ((x) + ((y) * wW)) * 2)

void GViewPeer::DrawCursor(int Show) {
    if (!(View && View->Parent))
        return;

    if (!(wState & sfFocus))
        Show = 0;

    if (Visibility == VisibilityFullyObscured)
        return;

    if (cX >= wW || cY >= wW ||
        cX + 1 > wW || cY + 1 > wH)
    {
        //fprintf(stderr, "%d %d  %d %d %d %d\n", ScreenCols, ScreenRows, X, Y, W, H);
        return;
    }

    DEBUG(("DrawCursor %d %d\n", cX, cY));
    //    if (!XtIsManaged(TextWin)) return ;

    if (cVisible && cX >= 0 && cY >= 0) {
        char *p = CursorXYPos(cX, cY), attr;
        attr = p[1];
        /*if (Show) attr = ((((attr << 4) & 0xF0)) | (attr >> 4)) ^ 0x77;*/
        if (Show) attr = (attr ^ 0x77);

        if (gc[attr] == 0) {
            gcv.foreground = Colors[attr & 0xF].pixel;
            gcv.background = Colors[(attr >> 4) & 0xF].pixel;
            gcv.font = fontStruct->fid;
            gc[attr] = XtGetGC(TextWin,
                               GCForeground | GCBackground | GCFont, &gcv);
        }
        XDrawImageString(display, XtWindow(TextWin), gc[attr],
                         cX * cxChar,
                         fontStruct->max_bounds.ascent + cY * cyChar,
                         (const char*)p, 1);
    }
}

int GViewPeer::ConPutBox(int X, int Y, int W, int H, PCell Cell) {
    int i;
    char temp[256], attr;
    char *p, *ps, *c, *ops;
    int len, x, l, ox, olen, skip;

    if (!(View && View->Parent && gc))
        return 1;

    if (Visibility == VisibilityFullyObscured)
        return 0;

    if (X >= wW || Y >= wH ||
        X + W > wW || Y + H > wH)
    {
        //fprintf(stderr, "%d %d  %d %d %d %d\n", ScreenCols, ScreenRows, X, Y, W, H);
        return 0;
    }

    DEBUG(("PutBox %d | %d %d %d %d | %d %d\n", wRefresh, X, Y, W, H, wW, wH));
    for (i = 0; i < H; i++) {
        len = W;
        p = CursorXYPos(X, Y + i);
        ps = (char *) Cell;
        x = X;
        while (len > 0) {
            if (!wRefresh) {
                c = CursorXYPos(x, Y + i);
                skip = 0;
                ops = ps;
                ox = x;
                olen = len;
                while ((len > 0) && (*(unsigned short *) c == *(unsigned short *)ps)) x++, len--, ps+=2, c+=2, skip++;
                if (len <= 0) break;
                if (skip <= 4) {
                    ps = ops;
                    x = ox;
                    len = olen;
                }
            }
            p = ps;
            l = 1;
            temp[0] = *ps++; attr = *ps++;
            while ((l < len) && ((unsigned char) (ps[1]) == attr)) {
                temp[l++] = *ps++;
                ps++;
            }

            if (gc[attr] == 0) {
                gcv.foreground = Colors[attr & 0xF].pixel;
                gcv.background = Colors[(attr >> 4) & 0xF].pixel;
                gcv.font = fontStruct->fid;
                gc[attr] = XtGetGC(TextWin,
                                   GCForeground | GCBackground | GCFont, &gcv);
            }

            XDrawImageString(display,
                             XtWindow(TextWin), gc[attr],
                             x * cxChar,
                             fontStruct->max_bounds.ascent + (Y + i) * cyChar,
                             temp, l);
            x += l;
            len -= l;
        }
        p = CursorXYPos(X, Y + i);
        memmove(p, Cell, W * sizeof(TCell));
        if (i + Y == cY)
            DrawCursor(1);
        Cell += W;
    }
    DEBUG(("done putbox\n"));

    return 1;
}

void GViewPeer::UpdateWindow(int xx, int yy, int ww, int hh) {
    PCell p;
    int i;

    ww /= cxChar; ww += 2;
    hh /= cyChar; hh += 2;
    xx /= cxChar;
    yy /= cyChar;
    if (xx + ww > wW) ww = wW - xx;
    if (yy + hh > wH) hh = wH - yy;
    wRefresh = 1;
    p = (PCell) CursorXYPos(xx, yy);

    for (i = 0; i < hh; i++) {
        ConPutBox(xx, yy + i, ww, 1, p);
        p += wW;
    }

    XFlush(display);
    wRefresh = 0;
}

int GViewPeer::ConGetBox(int X, int Y, int W, int H, PCell Cell) {
    for (int i = 0; i < H; i++) {
        memcpy(Cell, CursorXYPos(X, Y + i), 2 * W);
        Cell += W;
    }

    return 1;
}

int GViewPeer::ConPutLine(int X, int Y, int W, int H, PCell Cell) {
    for (int i = 0; i < H; i++)
        if (!ConPutBox(X, Y + i, W, 1, Cell))
            return 0;

    return 1;
}

int GViewPeer::ConSetBox(int X, int Y, int W, int H, TCell Cell) {
    TDrawBuffer B;

    for (int i = 0; i < W; i++)
        B[i] = Cell;
    ConPutLine(X, Y, W, H, B);

    return 1;
}

int GViewPeer::ConScroll(int Way, int X, int Y, int W, int H, TAttr Fill, int Count) {
    TCell Cell;
    int l;

    MoveCh(&Cell, ' ', Fill, 1);
    DrawCursor(0);
    if (Way == csUp) {
        XCopyArea(display, XtWindow(TextWin), XtWindow(TextWin), gc[0],
                  X * cxChar,
                  (Y + Count) * cyChar,
                  W * cxChar,
                  (H - Count) * cyChar,
                  X * cxChar,
                  Y * cyChar
                 );
        for (l = 0; l < H - Count; l++)
            memcpy(CursorXYPos(X, Y + l), CursorXYPos(X, Y + l + Count), 2 * W);

        if (!ConSetBox(X, Y + H - Count, W, Count, Cell))
            return 0;
    } else if (Way == csDown) {
        XCopyArea(display, XtWindow(TextWin), XtWindow(TextWin), gc[0],
                  X * cxChar,
                  Y * cyChar,
                  W * cxChar,
                  (H - Count) * cyChar,
                  X * cxChar,
                  (Y + Count)* cyChar
                 );
        for (l = H - 1; l >= Count; l--)
            memcpy(CursorXYPos(X, Y + l), CursorXYPos(X, Y + l - Count), 2 * W);

        if (!ConSetBox(X, Y, W, Count, Cell))
            return 0;
    }
    DrawCursor(1);

    return 0;
}

int GViewPeer::ConSetSize(int X, int Y) {
    char *NewBuffer;
    char *p;
    int i;
    int MX, MY;

    p = NewBuffer = (char *) malloc(X * Y * sizeof(TCell));
    if (NewBuffer == NULL)
        return 0;
    for (int i = 0; i < X * Y; i++) {
        *p++ = ' ';
        *p++ = 0x07;
    }
    if (ScreenBuffer) {
        MX = wW; if (X < MX) MX = X;
        MY = wH; if (Y < MY) MY = Y;
        if (X < MX) MX = X;
        p = NewBuffer;
        for (i = 0; i < MY; i++) {
            memcpy(p, CursorXYPos(0, i), MX * sizeof(TCell));
            p += X * 2;
        }
        free(ScreenBuffer);
    }
    ScreenBuffer = NewBuffer;
    wW = X;
    wH = Y;
    wRefresh = 1;
    View->Resize(wW, wH);
    ConPutBox(0, 0, wW, wH, (PCell) ScreenBuffer);
    wRefresh = 0;
    //    if (Refresh == 0)
    //        XResizeWindow(display, win, ScreenCols * cxChar, ScreenRows * cyChar);

    return 1;
}

int GViewPeer::ConQuerySize(int *X, int *Y) {
    //printf("3CONQUERYSIZE  %d  %d\n", wW, wH);
    if (X) *X = wW;
    if (Y) *Y = wH;

    return 1;
}

int GViewPeer::ConSetCursorPos(int X, int Y) {
    if (X < 0) X = 0;
    else if (X >= wW) X = wW - 1;

    if (Y < 0) Y = 0;
    else if (Y >= wH) Y = wH - 1;
    DrawCursor(0);
    cX = X;
    cY = Y;
    DrawCursor(1);

    return 1;
}

int GViewPeer::ConQueryCursorPos(int *X, int *Y) {
    if (X) *X = cX;
    if (Y) *Y = cY;

    return 1;
}

int GViewPeer::ConShowCursor() {
    cVisible = 1;
    //    DrawCursor(1);

    return 1;
}

int GViewPeer::ConHideCursor() {
    cVisible = 0;
    //  DrawCursor(0);

    return 1;
}

int GViewPeer::ConCursorVisible() {
    return cVisible;
}

int GViewPeer::ConSetCursorSize(int Start, int End) {
    cStart = Start;
    cEnd = End;

    if (wState & sfFocus)
        return 1; //PMSetCursorSize(Start, End);

    return 1;
}

int GViewPeer::ExpandHeight(int DeltaY) {
    return 0;
}

int GViewPeer::QuerySbVPos() {
    return sbVstart;
}

int GViewPeer::SetSbVPos(int Start, int Amount, int Total) {
    if (sbVstart != Start || sbVamount != Amount || sbVtotal != Total) {
        sbVstart = Start;
        sbVamount = Amount;
        sbVtotal = Total;

        if (!View->Parent)
            return 0;

        if (Amount < 1 || Start + Amount > Total) {
            XtVaSetValues(SbVert,
                          XmNmaximum, 1,
                          XmNminimum, 0,
                          XmNpageIncrement, 1,
                          XmNsliderSize, 1,
                          XmNvalue, 0,
                          NULL);
        } else {
            XtVaSetValues(SbVert,
                          XmNmaximum, Total,
                          XmNminimum, 0,
                          XmNpageIncrement, ((Amount > 1) ? Amount : 1),
                          XmNsliderSize, Amount,
                          XmNvalue, Start,
                          NULL);
        }
    }

    return 1;
}

int GViewPeer::SetSbHPos(int Start, int Amount, int Total) {
    if (sbHstart != Start || sbHamount != Amount || sbHtotal != Total) {
        sbHstart = Start;
        sbHamount = Amount;
        sbHtotal = Total;

        if (!View->Parent)
            return 0;

        if (Amount < 1 || Start + Amount > Total)
            XtVaSetValues(SbHorz,
                          XmNmaximum, 1,
                          XmNminimum, 0,
                          XmNpageIncrement, 1,
                          XmNsliderSize, 1,
                          XmNvalue, 0,
                          NULL);
        else
            XtVaSetValues(SbHorz,
                          XmNmaximum, Total,
                          XmNminimum, 0,
                          XmNpageIncrement, ((Amount > 1) ? Amount : 1),
                          XmNsliderSize, Amount,
                          XmNvalue, Start,
                          NULL);
    }

    return 1;
}

int GViewPeer::UpdateCursor() {
    ConSetCursorPos(cX, cY);
    ConSetCursorSize(cStart, cEnd);
    if (cVisible)
        ConShowCursor();
    else
        ConHideCursor();

    return 1;
}

int GViewPeer::PMShowCursor() {
    //    if (wState & sfFocus)
    //        WinShowCursor(hwndView, TRUE);
    return 1;
}

int GViewPeer::PMHideCursor() {
    //    if (wState & sfFocus)
    //        WinShowCursor(hwndView, FALSE);
    return 1;
}

int GViewPeer::PMSetCursorPos() {
    //    if (wState & sfFocus) {
    //        WinDestroyCursor(hwndView);
    //        WinCreateCursor(hwndView,
    //                        cxChar * cX, cyChar * (wH - cY - 1), cxChar, 2,
    //                        CURSOR_TYPE,
    //                        NULL);
    //        WinShowCursor(hwndView, TRUE);
    //    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////

GView::GView(GFrame *parent, int XSize, int YSize) :
    Parent(parent),
    Next(0),
    Prev(0),
    Peer(new GViewPeer(this, XSize, YSize))
{
    if (Parent)
        Parent->AddView(this);
}

GView::~GView() {
    if (Parent)
        Parent->RemoveView(this);
    delete Peer;
}

int GView::ConClear() {
    int W, H;
    TDrawBuffer B;

    ConQuerySize(&W, &H);
    MoveChar(B, 0, W, ' ', 0x07, 1);
    ConSetBox(0, 0, W, H, B[0]);

    return 1;
}

int GView::ConPutBox(int X, int Y, int W, int H, PCell Cell) {
    return Peer->ConPutBox(X, Y, W, H, Cell);
}

int GView::ConGetBox(int X, int Y, int W, int H, PCell Cell) {
    return Peer->ConGetBox(X, Y, W, H, Cell);
}

int GView::ConPutLine(int X, int Y, int W, int H, PCell Cell) {
    return Peer->ConPutLine(X, Y, W, H, Cell);
}

int GView::ConSetBox(int X, int Y, int W, int H, TCell Cell) {
    return Peer->ConSetBox(X, Y, W, H, Cell);
}

int GView::ConScroll(int Way, int X, int Y, int W, int H, TAttr Fill, int Count) {
    return Peer->ConScroll(Way, X, Y, W, H, Fill, Count);
}

int GView::ConSetSize(int X, int Y) {
    printf("ConSetSize %d  %d\n", X, Y);
    if (Peer->ConSetSize(X, Y)) {
        // Resize(X, Y);
        return 1;
    }

    return 0;
}

int GView::ConQuerySize(int *X, int *Y) {
    return Peer->ConQuerySize(X, Y);
}

int GView::ConSetCursorPos(int X, int Y) {
    return Peer->ConSetCursorPos(X, Y);
}

int GView::ConQueryCursorPos(int *X, int *Y) {
    return Peer->ConQueryCursorPos(X, Y);
}

int GView::ConShowCursor() {
    return Peer->ConShowCursor();
}

int GView::ConHideCursor() {
    return Peer->ConHideCursor();
}

int GView::ConCursorVisible() {
    return Peer->ConCursorVisible();
}

int GView::ConSetCursorSize(int Start, int End) {
    return Peer->ConSetCursorSize(Start, End);
}

int GView::QuerySbVPos() {
    return Peer->QuerySbVPos();
}

int GView::SetSbVPos(int Start, int Amount, int Total) {
    return Peer->SetSbVPos(Start, Amount, Total);
}

int GView::SetSbHPos(int Start, int Amount, int Total) {
    return Peer->SetSbHPos(Start, Amount, Total);
}

int GView::ExpandHeight(int DeltaY) {
    return Peer->ExpandHeight(DeltaY);
}

void GView::Update() {
}

void GView::Repaint() {
}

void GView::HandleEvent(TEvent &Event) {
}

void GView::Resize(int width, int height) {
    Repaint();
}

void GView::EndExec(int NewResult) {
    Result = NewResult;
}

int GView::Execute() {
    int SaveRc = Result;
    int NewResult;

    Result = -2;
    while (Result == -2 && frames != 0)
        gui->ProcessEvent();
    NewResult = Result;
    Result = SaveRc;
    return NewResult;
}

int GView::IsActive() {
    return (Parent->Active == this);
}

void GView::Activate(int gotfocus) {
    if (gotfocus) {
        Peer->wState |= sfFocus;
        Peer->UpdateCursor();
    } else
        Peer->wState &= ~sfFocus;

    Repaint();
}

int GView::CaptureMouse(int grab) {
    if (MouseCapture == 0) {
        if (!grab)
            return 0;
        MouseCapture = this;
    } else {
        if (grab || MouseCapture != this)
            return 0;
        MouseCapture = 0;
    }

    return 1;
}

///////////////////////////////////////////////////////////////////////////

GFramePeer::GFramePeer(GFrame *frame, int Width, int Height) :
    MenuBar(0)
{
    ShellWin = XtCreatePopupShell("fte", topLevelShellWidgetClass,
                                  TopLevel, NULL, 0);

    XtVaSetValues(ShellWin,
                  XmNwidthInc,  cxChar,
                  XmNheightInc, cyChar,
                  NULL);

    MainWin = XtCreateManagedWidget("Widget", xmMainWindowWidgetClass,
                                    ShellWin, NULL, 0);

    PanedWin = XtVaCreateManagedWidget("pane",
                                       xmPanedWindowWidgetClass, MainWin,
                                       XmNmarginHeight, 0,
                                       XmNmarginWidth, 0,
                                       NULL);

    XtVaSetValues (MainWin,
                   XmNworkWindow, PanedWin,
                   NULL);

    if (Width != -1 && Height != -1)
        ConSetSize(Width, Height);
}

GFramePeer::~GFramePeer() {
    // Kill master window - deletes recursively all related widgets
    XtDestroyWidget(MenuBar);
    XtDestroyWidget(ShellWin);
}

int GFramePeer::ConSetSize(int X, int Y) {
    printf("SetSIZE %d x %d\n", X, Y);
    //return ::ConSetSize(X, Y);
    return 0;
}

int GFramePeer::ConQuerySize(int *X, int *Y) {
    //::ConQuerySize(&fW, &fH);
    //if (X) *X = fW;
    //if (Y) *Y = fH;
    if (X) *X = 80;
    if (Y) *Y = 25;

    printf("ConQuerySize %d x %d\n", *X, *Y);
    return 1;
}


int GFramePeer::ConSetTitle(const char *Title, const char *STitle) {
    XSetStandardProperties(display, XtWindow(ShellWin), Title, STitle, 0, NULL, 0, NULL);
    return 1;
}

int GFramePeer::ConGetTitle(char *Title, size_t MaxLen, char *STitle, size_t SMaxLen) {
    *Title = 0;
    *STitle = 0;
    return 1;
}

void GFramePeer::MapFrame() {
    //Parent->UpdateMenu();
    XtPopup(ShellWin, XtGrabNone);
    XSetWMProtocols(display, XtWindow(ShellWin), &WM_DELETE_WINDOW, 1);
    XtInsertEventHandler(ShellWin, NoEventMask, True, CloseWindow, NULL, XtListHead);
}

///////////////////////////////////////////////////////////////////////////

GFrame::GFrame(int XSize, int YSize) {
    Menu = 0;
    if (!frames) {
        frames = Prev = Next = this;
    } else {
        Next = frames->Next;
        Prev = frames;
        frames->Next->Prev = this;
        frames->Next = this;
        frames = this;
    }
    Top = Active = 0;
    Peer = new GFramePeer(this, XSize, YSize);
}

GFrame::~GFrame() {
    delete Peer;
    free(Menu);

    if (Next == this) {
        frames = 0;
        //        DEBUG(("No more frames\x7\x7\n"));
    } else {
        Next->Prev = Prev;
        Prev->Next = Next;
        frames = Next;
    }
}

int GFrame::ConSetTitle(const char *Title, const char *STitle) {
    return Peer->ConSetTitle(Title, STitle);
}

int GFrame::ConGetTitle(char *Title, size_t MaxLen, char *STitle, size_t SMaxLen) {
    return Peer->ConGetTitle(Title, MaxLen, STitle, SMaxLen);
}

int GFrame::ConSetSize(int X, int Y) {
    return Peer->ConSetSize(X, Y);
}

int GFrame::ConQuerySize(int *X, int *Y) {
    return Peer->ConQuerySize(X, Y);
}

int GFrame::ConSplitView(GView *view, GView *newview) {
    int dmy;

    newview->Parent = this;
    //    newview->Peer->wX = 0;
    ConQuerySize(&newview->Peer->wW, &dmy);
    //    newview->Peer->wY = view->Peer->wY + view->Peer->wH / 2;
    //    newview->Peer->wH = view->Peer->wH - view->Peer->wH / 2;
    //    view->Peer->wH /= 2;
    InsertView(view, newview);
    view->ConSetSize(view->Peer->wW, view->Peer->wH);
    newview->ConSetSize(newview->Peer->wW, newview->Peer->wH);
    return 1;
}

int GFrame::ConCloseView(GView *view) {
    return 0;
}

int GFrame::ConResizeView(GView *view, int DeltaY) {
    return 0;
}

int GFrame::AddView(GView *view) {
    if (Top != 0)
        return ConSplitView(Top, view);

    //        int W, H;

    view->Parent = this;
    view->Prev = view->Next = 0;

    //        view->Peer->wX = 0;
    //        view->Peer->wY = 0;
    //        ConQuerySize(&W, &H);
    //        view->ConSetSize(W, H);
    InsertView(Top, view);
    return 1;
}

void GFrame::Update() {
    UpdateMenu();
    for (GView *v = Active; v; v = v->Next) {
        v->Update();
        if (v->Next == Active)
            break;
    }
}

void GFrame::UpdateMenu() {
}

void GFrame::Repaint() {
    for (GView* v = Active; v; v = v->Next) {
        v->Repaint();
        if (v->Next == Active)
            break;
    }
}

void GFrame::InsertView(GView *Prev, GView *view) {
    if (!view) return ;
    if (Prev) {
        view->Prev = Prev;
        view->Next = Prev->Next;
        Prev->Next = view;
        view->Next->Prev = view;
    } else {
        view->Prev = view->Next = view;
        Top = view;
    }
    if (Active == 0) {
        Active = view;
        Active->Activate(1);
    }
}

void GFrame::RemoveView(GView *view) {
    if (!view) return ;

    if (Active == view)
        Active->Activate(0);
    if (view->Next == view) {
        Top = Active = 0;
        delete this;
    } else {
        view->Next->Prev = view->Prev;
        view->Prev->Next = view->Next;

        //        if (Top == view) {
        //            Top = view->Next;
        //            Top->Peer->wY -= view->Peer->wH;
        //            Top->ConSetSize(Top->Peer->wW, Top->Peer->wH + view->Peer->wH);
        //        } else {
        //            view->Prev->ConSetSize(view->Prev->Peer->wW,
        //                                   view->Prev->Peer->wH + view->Peer->wH);
        //        }

        if (Active == view) {
            Active = view->Prev;
            Active->Activate(1);
        }
    }
}

void GFrame::SelectNext(int back) {
    GView *c = Active;

    if (c == 0 && Top == 0)
        return;

    if (c == 0)
        c = Active = Top;
    else
        if (back) {
            Active = Active->Prev;
        } else {
            Active = Active->Next;
        }

    if (c != Active) {
        c->Activate(0);
        Active->Activate(1);
    }

    if (Active)
        XtSetKeyboardFocus(Peer->PanedWin, Active->Peer->TextWin);
}

int GFrame::SelectView(GView *view) {
    if (Top == 0)
        return 0;

    if (FocusCapture != 0 || MouseCapture != 0)
        return 0;

    if (Active)
        Active->Activate(0);

    Active = view;

    if (Active)
        Active->Activate(1);

    if (Active)
        XtSetKeyboardFocus(Peer->PanedWin, Active->Peer->TextWin);

    return 1;
}

void GFrame::Resize(int width, int height) {
    if (!Top)
        return;

    if (width < 8 || height < 2)
        return;

    if (Top == Top->Next)
        Top->ConSetSize(width, height);
}

static Widget CreateMotifMenu(Widget parent, int menu, int main, XtCallbackProc MenuProc) {
    Widget hmenu;
    Widget item;

    if (main == 1)
        hmenu = XmCreateMenuBar(parent, "menu", NULL, 0);
    else if (main == 2) {
        hmenu = XmCreatePopupMenu(parent, "submenu",
                                  NULL, 0);
        //        XtCreateManagedWidget ( "Title", xmLabelWidgetClass, hmenu,
        //                               NULL, 0 );

        //        XtCreateManagedWidget ( "separator", xmSeparatorWidgetClass,
        //                            hmenu, NULL, 0 );
    } else
        hmenu = XmCreatePulldownMenu(parent, "submenu",
                                     NULL, 0);

    for (int i = 0; i < Menus[menu].Count; ++i) {
        if (Menus[menu].Items[i].Name) {
            char s[256];
            char *mn = 0;

            strcpy(s, Menus[menu].Items[i].Name);
            char* p = strchr(s, '&');
            if (p) {
                mn = p;
                // for strcpy src & dest may not overlap!
                for (; p[0]; ++p)
                    p[0] = p[1];
            }
            p = strchr(s, '\t');
            if (p) {
                *p = 0;
                p++;
            }
            if (Menus[menu].Items[i].SubMenu != -1) {
                item = XtVaCreateManagedWidget(s,
                                               xmCascadeButtonWidgetClass,
                                               hmenu,
                                               XmNsubMenuId,
                                               CreateMotifMenu(hmenu,
                                                               Menus[menu].Items[i].SubMenu,
                                                               0, MenuProc),
                                               NULL);
            } else {
                item = XtVaCreateManagedWidget(s,
                                               xmPushButtonWidgetClass,
                                               hmenu,
                                               NULL);
                XtAddCallback(item, XmNactivateCallback,
                              MenuProc, &(Menus[menu].Items[i]));
            }

            if (p)
                XtVaSetValues(item,
                              XmNacceleratorText,
                              XmStringCreate(p, XmSTRING_DEFAULT_CHARSET),
                              NULL);
            if (mn)
                XtVaSetValues(item,
                              XmNmnemonic,
                              KeySym(*mn),
                              NULL);
        } else {
            item = XtVaCreateManagedWidget("separator",
                                           xmSeparatorWidgetClass,
                                           hmenu,
                                           NULL);
            //XmCreateSeparator(parent, "xx", 0, 0);
        }
        //        item.id = Menus[menu].Items[i].Cmd & 0xFFFF; // ?
    }
    return hmenu;
}

static Widget CreateMotifMainMenu(Widget parent, char *Name) {
    int id = GetMenuId(Name);

    return CreateMotifMenu(parent, id, 1, MainCallback);
}

int GFrame::SetMenu(const char *Name) {
    free(Menu);
    Menu = strdup(Name);

    if (Peer->MenuBar)
        XtDestroyWidget(Peer->MenuBar);

    Peer->MenuBar = CreateMotifMainMenu(Peer->MainWin, Menu);
    XtManageChild (Peer->MenuBar);
    XtVaSetValues (Peer->MainWin,
                   XmNmenuBar, Peer->MenuBar,
                   NULL);
    return 1;
}

int GFrame::ExecMainMenu(char Sub) {
    return 1;
}

int GFrame::PopupMenu(const char *Name) {
    int id = GetMenuId(Name);

    if (LPopupMenu)
        XtDestroyWidget(LPopupMenu);

    LPopupMenu = CreateMotifMenu(Peer->MainWin, id, 2, PopupCallback);
    XtAddCallback(XtParent(LPopupMenu), XmNpopdownCallback, MenuPopdownCb, 0);
    XmMenuPosition(LPopupMenu, &LastRelease);
    XtManageChild(LPopupMenu);
    return 1;
}

void GFrame::Show() {
    Update();
    Peer->MapFrame();
}

void GFrame::Activate() {
    frames = this;
    Update();
    //Peer->ShowFrame();
}

// GUI

int GUI::multiFrame() {
    return 1;
}

GUI::GUI(int &argc, char **argv, int XSize, int YSize) :
    fArgc(argc),
    fArgv(argv)
{
    char *fs = getenv("VIOFONT");
    if (fs == 0 && WindowFont[0] != 0)
        fs = WindowFont;

    TopLevel = XtVaAppInitialize(&AppContext, "TopLevel", NULL, 0,
                                 &argc, argv, NULL,
                                 XmNmappedWhenManaged, FALSE,
                                 NULL);
    if (!TopLevel)
        return;

    if (!(display = XtDisplay(TopLevel)))
        return;
    //XSynchronize(display, True);

    root = DefaultRootWindow(display);
    screen = DefaultScreen(display);
    colormap = DefaultColormap(display, screen);

    InitXColors();

    fontStruct = NULL;
    if (fs)
        fontStruct = XLoadQueryFont(display, fs);
    else {
#ifdef HPUX
        fontStruct = XLoadQueryFont(display, "8x13");
#endif
#ifdef AIX
        fontStruct = XLoadQueryFont(display, "8x13");
#endif
#ifdef LINUX
        fontStruct = XLoadQueryFont(display, "8x13");
#endif
#ifdef IRIX
        fontStruct = XLoadQueryFont(display, "8x13");
#endif
    }
    if (fontStruct == NULL)
        fontStruct = XLoadQueryFont(display, "fixed");
    if (fontStruct == NULL)
        return;
    cxChar = fontStruct->max_bounds.width;
    cyChar = fontStruct->max_bounds.ascent + fontStruct->max_bounds.descent;
    XtRealizeWidget(TopLevel);

    WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XA_CLIPBOARD = XInternAtom(display, "CLIPBOARD", False);

    gui = this;
}

GUI::~GUI() {
    //core: XtCloseDisplay(display);
    //core: XtDestroyApplicationContext(AppContext);
    gui = 0;
}

int GUI::ConGrabEvents(TEventMask EventMask) {
    return 1;
}

void GUI::DispatchEvent(GFrame *frame, GView *view, TEvent &Event) {
    if (Event.What != evNone) {
        if (view)
            view->HandleEvent(Event);
    }
}

int GUI::ConSuspend() { return 0; }

int GUI::ConContinue() { return 0; }

int GUI::ConGetEvent(TEventMask EventMask, TEvent *Event, int WaitTime, int Delete, GView **view) {
    //return ::ConGetEvent(EventMask, Event, WaitTime, Delete, view);
    assert(1 == 0);
    return 1;
}

int GUI::ConPutEvent(const TEvent& Event) {
    EventBuf = Event;
    return 1;
}

int GUI::ConFlush() {
    return 1;
}

void GUI::ProcessEvent() {
    static int need_update = 1;

    if (need_update && XtAppPending(AppContext) == 0 ) {
        frames->Update();
        need_update = 0;
    }
    XtAppProcessEvent(AppContext, XtIMAll);
    if (NextEvent.What != evNone) {
        DispatchEvent(frames, NextEvent.Msg.View, NextEvent);
        NextEvent.What = evNone;
        need_update = 1;
    }
}

int GUI::Run() {
    if (Start(fArgc, fArgv)) {
        doLoop = 1;
        frames->Show();

        //if (frames && frames->Peer)
        //    frames->Peer->MapFrame();

        //XtAppMainLoop(AppContext);
        while (doLoop)
            ProcessEvent();

        Stop();
        return 1;
    }

    return 0;
}

int GUI::ShowEntryScreen() {
    return 1;
}

int GUI::RunProgram(int mode, char *Command) {
    char Cmd[1024];

    strlcpy(Cmd, XShellCommand, sizeof(Cmd));

    if (*Command == 0)  // empty string = shell
        strlcat(Cmd, " -ls &", sizeof(Cmd));
    else {
        strlcat(Cmd, " -e ", sizeof(Cmd));
        strlcat(Cmd, Command, sizeof(Cmd));
        if (mode == RUN_ASYNC)
            strlcat(Cmd, " &", sizeof(Cmd));
    }

    return (system(Cmd) == 0);
}

//void PipeCallback(GPipe *pipe, int *source, XtInputId *input)
void PipeCallback(void *pipev, int *source, XtInputId *input)
{
    GPipe *pipe = (GPipe*)pipev;
    if (pipe && pipe->notify && *source == pipe->fd) {
        NextEvent.What = evNotify;
        NextEvent.Msg.View = frames->Active;
        NextEvent.Msg.Model = pipe->notify;
        NextEvent.Msg.Command = cmPipeRead;
        NextEvent.Msg.Param1 = pipe->id;
        pipe->stopped = 0;
    }
    //fprintf(stderr, "Pipe %d\n", *source);
}

int GUI::OpenPipe(const char *Command, EModel *notify) {
    for (int i = 0; i < MAX_PIPES; i++) {
        if (Pipes[i].used == 0) {
            int pfd[2];

            Pipes[i].id = i;
            Pipes[i].notify = notify;
            Pipes[i].stopped = 1;

            if (pipe((int *)pfd) == -1)
                return 0;

            switch (Pipes[i].pid = fork()) {
            case -1: /* fail */
                return -1;
            case 0: /* child */
                close(pfd[0]);
                close(0);
                dup2(pfd[1], 1);
                dup2(pfd[1], 2);
                exit(system(Command));
            default:
                close(pfd[1]);
                fcntl(pfd[0], F_SETFL, O_NONBLOCK);
                Pipes[i].fd = pfd[0];
            }
            Pipes[i].input =                                 /* FIXME: */
                XtAppAddInput(AppContext, Pipes[i].fd, (void*)XtInputReadMask, PipeCallback, &Pipes[i]);
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

    //fprintf(stderr, "Pipe View: %d %08X\n", id, notify);
    Pipes[id].notify = notify;
    if (notify != Pipes[id].notify) {
        if (notify) {
            Pipes[id].input =                               /* FIXME */
                XtAppAddInput(AppContext, Pipes[id].fd, (void*)XtInputReadMask, PipeCallback, &Pipes[id]);
        } else {
            if (Pipes[id].input != 0) {
                XtRemoveInput(Pipes[id].input);
                Pipes[id].input = 0;
            }
	}
    }

    return 1;
}

ssize_t GUI::ReadPipe(int id, void *buffer, size_t len) {
    ssize_t rc;

    if (id < 0 || id > MAX_PIPES || Pipes[id].used == 0)
        return -1;

    //fprintf(stderr, "Pipe Read: Get %d %d\n", id, len);

    rc = read(Pipes[id].fd, buffer, len);
    //fprintf(stderr, "Pipe Read: Got %d %d\n", id, len);
    if (rc == 0) {
        if (Pipes[id].input != 0) {
            XtRemoveInput(Pipes[id].input);
            Pipes[id].input = 0;
        }
        close(Pipes[id].fd);
        return -1;
    }
    if (rc == -1)
        Pipes[id].stopped = 1;

    return rc;
}

int GUI::ClosePipe(int id) {
    int status;

    if (id < 0 || id > MAX_PIPES || Pipes[id].used == 0)
        return 0;

    waitpid(Pipes[id].pid, &status, 0);
    //fprintf(stderr, "Pipe Close: %d\n", id);
    Pipes[id].used = 0;

    return (WEXITSTATUS(status) == 0);
}

int GetXSelection(int *len, char **data, int clipboard) {
    // XXX use clipboard?
    if (!(*data = XFetchBytes(display, len)))
        return 0;

    return 1;
}

int SetXSelection(int len, char *data, int clipboard) {
    Atom clip;

    XStoreBytes(display, data, len);
    switch (clipboard) {
    case 0:
        clip =  XA_CLIPBOARD;
        break;
    case 1:
        clip = XA_PRIMARY;
        break;
    case 2:
        clip = XA_SECONDARY;
        break;
    default:
        // not supported
        return 0;
    }
    XSetSelectionOwner(display, clip, None, CurrentTime);

    return 1;
}

/*
 static void SetColor(int i) {
 int j, k, z;
 j = i & 7;
 k = 65535 - 20480;
 z = (i > 7) ? (20480) : 0;
 Colors[i].blue  = k * (j & 1) + z;
 Colors[i].green = k * ((j & 2)?1:0) + z;
 Colors[i].red   = k * ((j & 4)?1:0) + z;
 Colors[i].flags = DoRed | DoGreen | DoBlue;
 }

 static int InitXColors() {
 int i;

 for (i = 0; i < 16; i++) {
 SetColor(i);
 if (XAllocColor(display, colormap, &Colors[i]) == 0) {
 colormap = XCreateColormap(display, win, DefaultVisual(display, screen), AllocNone);
 for (i = 0; i < 16; i++) {
 SetColor(i);
 XAllocColor(display, colormap, &Colors[i]);
 }
 XSetWindowColormap(display, win, colormap);
 return 0;
 }
 }
 return 0;
 }
 static int InitXGCs() {
 int i;
 XGCValues gcv;

 for (i = 0; i < 256; i++) {
 gcv.foreground = Colors[i % 16].pixel;
 gcv.background = Colors[(i / 16)].pixel;
 gcv.font = fontStruct->fid;
 GCs[i] = XCreateGC(display, win, GCForeground | GCBackground | GCFont, &gcv);
 }
 return 0;
 }
 */

void DieError(int rc, const char *msg, ...) {
    va_list ap;

    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    exit(rc);
}

char ConGetDrawChar(unsigned int index) {
    static const char tab[] = "\x0D\x0C\x0E\x0B\x12\x19____+>\x1F\x01\x12\x01\x01 \x02";

    assert(index < sizeof(tab) - 1);

    return tab[index];
}
