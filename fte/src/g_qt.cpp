/*    g_qt.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *    Copyright (c) 2010, Zdenek Kabelac
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 *    Only a demo code - not really fully functional
 */

#include "gui.h"
#include "s_string.h"
#include "sysdep.h"

#include <qapp.h>
#include <qclipbrd.h>
#include <qframe.h>
#include <qkeycode.h>
#include <qmenubar.h>
#include <qpainter.h>
#include <qscrbar.h>
#include <qtimer.h>
#include <qwidget.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define DEBUGX(x)
//#define DEBUGX(x) printf x

#define EDIT_BORDER 2
#define SCROLLBAR_SIZE 16

#define MAX_PIPES 4

typedef struct {
    int used;
    int id;
    int fd;
    int pid;
    int stopped;
    EModel *notify;
    //XtInputId input;
} GPipe;

static GPipe Pipes[MAX_PIPES] = {
    { 0 }, { 0 }, { 0 }, { 0 }
};

#define sfFocus   1

class QEText: public QWidget {
    Q_OBJECT
public:
    GViewPeer *view;
    
    QEText(GViewPeer *peer, QWidget *parent = 0, const char *name = 0);
    virtual ~QEText();

protected:
    void handleKeyPressEvent(QKeyEvent *qe);
    void ActiveEvent(TEvent &Event);
    void handleMouse(QMouseEvent *qe);
        
    virtual void resizeEvent(QResizeEvent *qe);
    virtual void paintEvent(QPaintEvent *qe);
    virtual void mousePressEvent(QMouseEvent *qe);
    virtual void mouseMoveEvent(QMouseEvent *qe);
    virtual void mouseReleaseEvent(QMouseEvent *qe);
    virtual void keyPressEvent(QKeyEvent *qe);
    virtual void focusInEvent(QFocusEvent *qe);
    virtual void focusOutEvent(QFocusEvent *qe);
};

class QEView: public QFrame {
    Q_OBJECT
public:
    GViewPeer   *view;
    QEText      *text;
    QScrollBar  *horz;
    QScrollBar  *vert;
    
    QEView(GViewPeer *peer, QWidget *parent = 0, const char *name = 0);
    virtual ~QEView();

    void ActiveEvent(TEvent &Event);
    void setViewPos(int x, int y, int w, int h);

//protected:
//    virtual void resizeEvent(QResizeEvent *qe);

protected slots:
    void sbHmoveLeft();
    void sbHmoveRight();
    void sbHpageLeft();
    void sbHpageRight();
    void sbHmoveTo(int pos);

    void sbVmoveUp();
    void sbVmoveDown();
    void sbVpageUp();
    void sbVpageDown();
    void sbVmoveTo(int pos);
};

class QEFrame : public QFrame {
    Q_OBJECT
public:
    QMenuBar    *menubar;
    QEFrame(GFramePeer *peer, QWidget *parent = 0, const char *name = 0);

    QMenuBar *CreateMenuBar(QWidget *parent, int Id);
    QPopupMenu *CreatePopup(QWidget *parent, int Id, int do_connect);

protected:
    virtual void resizeEvent(QResizeEvent *qe);
    virtual void closeEvent(QCloseEvent *qe);
public slots:
    void selectedMain(int id);
    void timerDone();
private:
    GFramePeer  *frame;
};

class GViewPeer {
public:
    QEView *qView;
    GC GCs[256];
    
    GView *View;
//    int wX, wY;
    int wW, wH, wState, wRefresh;
    int cX, cY, cVisible, cStart, cEnd;
    int sbVstart, sbVamount, sbVtotal;
    int sbHstart, sbHamount, sbHtotal;
    int VertPos, HorzPos;
    unsigned char *ScreenBuffer;
    
    GViewPeer(GView *view, int XSize, int YSize);
    ~GViewPeer();
    
    int AllocBuffer();
    void DrawCursor(/*QPainter *painter, */int Show);
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
    QEFrame *qFrame;
    
    GFramePeer(GFrame *aFrame, int Width, int Height);
    ~GFramePeer();
    
    int ConSetTitle(const char *Title, const char *STitle);
    int ConGetTitle(char *Title, size_t MaxLen, char *STitle, size_t SMaxLen);
    
    int ConSetSize(int X, int Y);
    int ConQuerySize(int *X, int *Y);
    void MapFrame();
    void ShowFrame();
};

int ShowVScroll = 1;
int ShowHScroll = 0;
int ShowMenuBar = 1;
int ShowToolBar = 0;

GFrame *frames = 0;
GUI *gui = 0;

static GView *MouseCapture = 0;
static GView *FocusCapture = 0;

static int cxChar = 1;
static int cyChar = 1;
//static int fmAscent;
static int FinalExit = 0;

static TEvent EventBuf = { evNone };

struct qEvent {
    TEvent event;
    struct qEvent *next;
};

TEvent NextEvent = { evNone };
static const QColor colors[16] = {
    Qt::black,
    Qt::darkBlue,
    Qt::darkGreen,
    Qt::darkCyan,
    Qt::darkRed,
    Qt::darkMagenta,
    Qt::darkYellow,
    Qt::gray,
    
    Qt::darkGray,
    Qt::blue,
    Qt::green,
    Qt::cyan,
    Qt::red,
    Qt::magenta,
    Qt::yellow,
    Qt::white
};

static XFontStruct *fontStruct;
static Display *display;

static QPoint LastMousePos;

//static int LastMouseX = -1, LastMouseY = -1;

static qEvent *event_head = 0;
static qEvent *event_tail = 0;

static int qPutEvent(TEvent &Event) {
    qEvent *q = new qEvent;

    q->event = Event;
    q->next = 0;

    if (event_tail) {
        event_tail->next = q;
        event_tail = q;
    } else {
        event_head = event_tail = q;
    }
    FinalExit = 0;
    qApp->exit_loop();
    return 0;
}

static int qHasEvent() {
    return event_head ? 1 : 0;
}

static void qGetEvent(TEvent &Event) {
    qEvent *q = event_head;
    
    Event = q->event;
    event_head = q->next;
    if (!event_head)
        event_tail = 0;
    delete q;
}

QEView::QEView(GViewPeer *peer, QWidget *parent, const char *name) :
    QFrame(parent, name),
    view(peer),
    text(new QEText(peer, this)),
    horz(new QScrollBar(QScrollBar::Horizontal, parent, 0)),
    vert(new QScrollBar(QScrollBar::Vertical, parent, 0))
{
    CHECK_PTR(text);
    CHECK_PTR(horz);
    CHECK_PTR(vert);
    horz->show();
    vert->show();

    setFrameStyle(Panel|Sunken);
    setLineWidth(EDIT_BORDER);

    connect(vert, SIGNAL(valueChanged(int)), SLOT(sbVmoveTo(int)));
    connect(vert, SIGNAL(sliderMoved(int)), SLOT(sbVmoveTo(int)));
    connect(vert, SIGNAL(prevLine()), SLOT(sbVmoveUp()));
    connect(vert, SIGNAL(nextLine()), SLOT(sbVmoveDown()));
    connect(vert, SIGNAL(prevPage()), SLOT(sbVpageUp()));
    connect(vert, SIGNAL(nextPage()), SLOT(sbVpageDown()));
    
    connect(horz, SIGNAL(valueChanged(int)), SLOT(sbHmoveTo(int)));
    connect(horz, SIGNAL(sliderMoved(int)), SLOT(sbHmoveTo(int)));
    connect(horz, SIGNAL(prevLine()), SLOT(sbHmoveLeft()));
    connect(horz, SIGNAL(nextLine()), SLOT(sbHmoveRight()));
    connect(horz, SIGNAL(prevPage()), SLOT(sbHpageLeft()));
    connect(horz, SIGNAL(nextPage()), SLOT(sbHpageRight()));
}

QEView::~QEView() {
    delete horz;
    delete vert;
}

void QEView::setViewPos(int x, int y, int w, int h) {
    setGeometry(x, y, w - SCROLLBAR_SIZE, h - SCROLLBAR_SIZE);
    text->setGeometry(contentsRect());
    vert->setGeometry(x + w - SCROLLBAR_SIZE, y, SCROLLBAR_SIZE, h - SCROLLBAR_SIZE);
    horz->setGeometry(x, y + h - SCROLLBAR_SIZE, w - SCROLLBAR_SIZE, SCROLLBAR_SIZE);
}

void QEView::ActiveEvent(TEvent &Event) {
    if (!view->View->IsActive())
        view->View->Parent->SelectView(view->View);
    qPutEvent(Event);
}

void QEView::sbHmoveLeft() {
    NextEvent.What = evCommand;
    NextEvent.Msg.View = view->View;
    NextEvent.Msg.Command = cmHScrollLeft;
    NextEvent.Msg.Param1 = 1;
    ActiveEvent(NextEvent);
}

void QEView::sbHmoveRight() {
    NextEvent.What = evCommand;
    NextEvent.Msg.View = view->View;
    NextEvent.Msg.Command = cmHScrollRight;
    NextEvent.Msg.Param1 = 1;
    ActiveEvent(NextEvent);
}

void QEView::sbHpageLeft() {
    NextEvent.What = evCommand;
    NextEvent.Msg.View = view->View;
    NextEvent.Msg.Command = cmHScrollPgLt;
    ActiveEvent(NextEvent);
}

void QEView::sbHpageRight() {
    NextEvent.What = evCommand;
    NextEvent.Msg.View = view->View;
    NextEvent.Msg.Command = cmHScrollPgRt;
    ActiveEvent(NextEvent);
}

void QEView::sbHmoveTo(int pos) {
    NextEvent.What = evCommand;
    NextEvent.Msg.View = view->View;
    NextEvent.Msg.Command = cmHScrollMove;
    NextEvent.Msg.Param1 = pos;
    ActiveEvent(NextEvent);
}

void QEView::sbVmoveUp() {
    NextEvent.What = evCommand;
    NextEvent.Msg.View = view->View;
    NextEvent.Msg.Command = cmVScrollUp;
    NextEvent.Msg.Param1 = 1;
    ActiveEvent(NextEvent);
}

void QEView::sbVmoveDown() {
    NextEvent.What = evCommand;
    NextEvent.Msg.View = view->View;
    NextEvent.Msg.Command = cmVScrollDown;
    NextEvent.Msg.Param1 = 1;
    ActiveEvent(NextEvent);
}

void QEView::sbVpageUp() {
    NextEvent.What = evCommand;
    NextEvent.Msg.View = view->View;
    NextEvent.Msg.Command = cmVScrollPgUp;
    ActiveEvent(NextEvent);
}

void QEView::sbVpageDown() {
    NextEvent.What = evCommand;
    NextEvent.Msg.View = view->View;
    NextEvent.Msg.Command = cmVScrollPgDn;
    ActiveEvent(NextEvent);
}

void QEView::sbVmoveTo(int pos) {
    NextEvent.What = evCommand;
    NextEvent.Msg.View = view->View;
    NextEvent.Msg.Command = cmVScrollMove;
    NextEvent.Msg.Param1 = pos;
    ActiveEvent(NextEvent);
}


QEText::QEText(GViewPeer *peer, QWidget *parent, const char *name): QWidget(parent, name) {
    view = peer;

    setFocusPolicy(QWidget::StrongFocus);
    setMinimumSize(100, 80);
    DEBUGX(("QEText %p\n", this));
}

QEText::~QEText() {
}

void QEText::resizeEvent(QResizeEvent *qe) {
    int X, Y;

    DEBUGX(("A: %p\n", qe));
    QWidget::resizeEvent(qe);
    DEBUGX(("B\n"));
    
    X = qe->size().width();
    Y = qe->size().height(); //qe->size().height() - frameWidth() * 2;
    DEBUGX(("Resize %d, %d\n", X, Y));
    X /= cxChar;
    Y /= cyChar;
    DEBUGX(("!! Resize %d, %d\n", X, Y));
    if (X > 0 && Y > 0) {
        view->ConSetSize(X, Y);
        NextEvent.What = evCommand;
        NextEvent.Msg.View = view->View;
        NextEvent.Msg.Command = cmResize;
        qPutEvent(NextEvent);
    }
}

void QEText::paintEvent(QPaintEvent *qe) {
    DEBUGX(("Paint %p\n", qe));
    view->UpdateWindow(qe->rect().x(),
                       qe->rect().y(),
                       qe->rect().width(),
                       qe->rect().height());
}

void QEText::handleMouse(QMouseEvent *qe) {
    int event = qe->button();
    int state = qe->state();
    int X = (qe->pos().x()/* - frameWidth()*/) / cxChar;
    int Y = (qe->pos().y()/* - frameWidth()*/) / cyChar;

    NextEvent.Mouse.View = view->View;

    LastMousePos = mapToGlobal(qe->pos());

    switch (qe->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
        if (!view->View->IsActive())
            view->View->Parent->SelectView(view->View);
        NextEvent.What = evMouseDown;
        break;
    case QEvent::MouseButtonRelease:
        NextEvent.What = evMouseUp;
        break;
    case QEvent::MouseMove:
        NextEvent.What = evMouseMove;
        break;
    default:
        return ;
    }
    NextEvent.Mouse.Buttons = 0;

    if (NextEvent.What == evMouseMove) {
        if (state & LeftButton)
            NextEvent.Mouse.Buttons |= 1;
        if (state & RightButton)
            NextEvent.Mouse.Buttons |= 2;
        if (state & MidButton)
            NextEvent.Mouse.Buttons |= 4;
    } else {
        if (event & LeftButton)
            NextEvent.Mouse.Buttons |= 1;
        if (event & RightButton)
            NextEvent.Mouse.Buttons |= 2;
        if (event & MidButton)
            NextEvent.Mouse.Buttons |= 4;
    }
    NextEvent.Mouse.KeyMask = 0;
    if (state & ShiftButton)
        NextEvent.Mouse.KeyMask |= kfShift;
    if (state & ControlButton)
        NextEvent.Mouse.KeyMask |= kfCtrl;
    if (state & AltButton)
        NextEvent.Mouse.KeyMask |= kfAlt;
    
    NextEvent.Mouse.Count = 1;
    if (qe->type() == QEvent::MouseButtonDblClick)
        NextEvent.Mouse.Count = 2;
    NextEvent.Mouse.X = X;
    NextEvent.Mouse.Y = Y;
    qPutEvent(NextEvent);
}


void QEText::mousePressEvent(QMouseEvent *qe) {
    handleMouse(qe);
}

void QEText::mouseMoveEvent(QMouseEvent *qe) {
    handleMouse(qe);
}

void QEText::mouseReleaseEvent(QMouseEvent *qe) {
    handleMouse(qe);
}

static const struct {
    unsigned int q_code;
    TKeyCode keyCode;
} key_table[] = {
    { Qt::Key_Escape,     kbEsc },
    { Qt::Key_Tab,        kbTab },
    { Qt::Key_Backtab,    kbTab | kfShift },
    { Qt::Key_Backspace,  kbBackSp },
    { Qt::Key_Return,     kbEnter },
    { Qt::Key_Enter,      kbEnter },
    { Qt::Key_Insert,     kbIns },
    { Qt::Key_Delete,     kbDel },
    { Qt::Key_Pause,      kbPause },
    { Qt::Key_Print,      kbPrtScr },
    { Qt::Key_SysReq,     kbSysReq },
    { Qt::Key_Home,       kbHome },
    { Qt::Key_End,        kbEnd },
    { Qt::Key_Left,       kbLeft },
    { Qt::Key_Up,         kbUp },
    { Qt::Key_Right,      kbRight },
    { Qt::Key_Down,       kbDown },
    { Qt::Key_Prior,      kbPgUp },
    { Qt::Key_Next,       kbPgDn },
    { Qt::Key_Shift,      kbShift | kfModifier },
    { Qt::Key_Control,    kbCtrl | kfModifier },
    { Qt::Key_Meta,       kbAlt | kfModifier },
    { Qt::Key_Alt,        kbAlt | kfModifier },
    { Qt::Key_CapsLock,   kbCapsLock | kfModifier },
    { Qt::Key_NumLock,    kbNumLock | kfModifier },
    { Qt::Key_ScrollLock, kbScrollLock | kfModifier },
    { Qt::Key_F1,         kbF1 },
    { Qt::Key_F2,         kbF2 },
    { Qt::Key_F3,         kbF3 },
    { Qt::Key_F4,         kbF4 },
    { Qt::Key_F5,         kbF5 },
    { Qt::Key_F6,         kbF6 },
    { Qt::Key_F7,         kbF7 },
    { Qt::Key_F8,         kbF8 },
    { Qt::Key_F9,         kbF9 },
    { Qt::Key_F10,        kbF10 },
    { Qt::Key_F11,        kbF11 },
    { Qt::Key_F12,        kbF12 },
};

void QEText::ActiveEvent(TEvent &Event) {
    if (!view->View->IsActive())
        view->View->Parent->SelectView(view->View);
    qPutEvent(Event);
}

void QEText::handleKeyPressEvent(QKeyEvent *qe) {
    TKeyCode keyCode;
    TKeyCode keyFlags;
    int state = qe->state();
    int ascii = qe->ascii();
    unsigned int key = qe->key();
    
    DEBUGX(("key: %d, ascii: %d(%c) state:%X\n",
           qe->key(),
           qe->ascii(),
           qe->ascii(),
           qe->state()));

    keyFlags = 0;
    if (state & ShiftButton)
        keyFlags |= kfShift;
    if (state & ControlButton)
        keyFlags |= kfCtrl;
    if (state & AltButton)
        keyFlags |= kfAlt;

    keyCode = 0;
    for (size_t i = 0; i < FTE_ARRAY_SIZE(key_table); ++i)
        if (key == key_table[i].q_code) {
            keyCode = key_table[i].keyCode;
            break;
        }

    if (keyCode == 0 && ascii != 0) {
        if (keyFlags & (kfCtrl | kfAlt)) {
            keyCode = toupper(ascii);
        } else {
            keyCode = ascii;
        }
    }
    if (keyCode == 0) {
        QWidget::keyPressEvent(qe);
        return;
    }

    DEBUGX(("key: %d, flags:%d\n", (int)keyCode, (int)keyFlags));

    NextEvent.What = evKeyDown;
    NextEvent.Key.View = view->View;
    NextEvent.Key.Code = keyCode | keyFlags;
    qPutEvent(NextEvent);
}

void QEText::keyPressEvent(QKeyEvent *qe) {
    handleKeyPressEvent(qe);
}

//void QEFrame::keyPressEvent(QKeyEvent *qe) {
//    frame->Frame->Active->Peer->qView->handleKeyPressEvent(qe);
//}

void QEText::focusInEvent(QFocusEvent *qe) {
    repaint(FALSE);
}

void QEText::focusOutEvent(QFocusEvent *qe) {
    repaint(FALSE);
}

QEFrame::QEFrame(GFramePeer *peer, QWidget *parent, const char *name): QFrame(parent, name)
{
    frame = peer;
    menubar = 0;
    //setAcceptFocus(TRUE);
}

void QEFrame::resizeEvent(QResizeEvent *qe) {
    int y = 0;
    int x = 0;
    int h = qe->size().height();
    int w = qe->size().width();
    int count, cur;
    int cy, ch;

    GView *p;
    
    if (menubar) {
        h -= menubar->height();
        y += menubar->height();
    }

    p = frame->Frame->Top;
    count = 0;
    if (p) do {
        count++;
        p = p->Next;
    } while (p != frame->Frame->Top);

    DEBUGX(("count: %d size: %d %d\n", count, w, h));

    p = frame->Frame->Top;
    cur = 0;
    if (p) do {
        ch = h / count;
        if (p->Next == frame->Frame->Top)
            ch = h - (h / count) * (count - 1);
        cy = y + h * cur / count;
        cur++;
        DEBUGX(("setting: %d %d %d %d\n", x, cy, w, ch));
        p->Peer->qView->setViewPos(x, cy, w, ch);
        p = p->Next;
    } while (p != frame->Frame->Top);

//    frame->Frame->Top->Peer->qView->SetViewGeom(x, y, w, h);
}

void QEFrame::closeEvent(QCloseEvent *qe) {
    DEBUGX(("Close Selected: %p\n", qe));
    NextEvent.What = evCommand;
    NextEvent.Msg.View = frame->Frame->Active;
    NextEvent.Msg.Command = cmClose;
    qPutEvent(NextEvent);
}

void QEFrame::selectedMain(int id) {
    DEBUGX(("Menu Selected: %d\n", id));
    NextEvent.What = evCommand;
    NextEvent.Msg.View = frame->Frame->Active;
    NextEvent.Msg.Command = id;
    qPutEvent(NextEvent);
}

#include "g_qt.moc"

#if 0

void ProcessXEvents(XEvent *event, TEvent *Event, GViewPeer *Peer) {
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

static void CloseWindow(Widget w, GFramePeer *frame, XEvent *event, Boolean *cont) {
    if (event->type != ClientMessage ||
        ((XClientMessageEvent *)event)->data.l[0] != WM_DELETE_WINDOW)
    {
        return ;
    }
    NextEvent.What = evCommand;
    NextEvent.Msg.Command = cmClose;
    *cont = False;
}

#endif
///////////////////////////////////////////////////////////////////////////

GViewPeer::GViewPeer(GView *view, int XSize, int YSize) :
    View(view),
    wW(XSize),
    wH(YSize),
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
         GCs[jj] = 0;

    qView = new QEView(this, frames->Peer->qFrame);
    qView->show();
}

GViewPeer::~GViewPeer() {
    //delete qView; // auto deleted
}

int GViewPeer::AllocBuffer() {
    int i;
    unsigned char *p;
    
    ScreenBuffer = (unsigned char *)malloc(2 * wW * wH);
    if (ScreenBuffer == NULL) return -1;
    for (i = 0, p = ScreenBuffer; i < wW * wH; i++) {
        *p++ = 32;
        *p++ = 0x07;
    }
    return 0;
}

#define InRange(x,a,y) (((x) <= (a)) && ((a) < (y)))
#define CursorXYPos(x,y) (ScreenBuffer + ((x) + ((y) * wW)) * 2)

void GViewPeer::DrawCursor(/*QPainter *painter, */int Show) {
    if (!(View && View->Parent))
        return ;

    if (qView->text->winId() == 0)
        return ;
        
    if (!(wState & sfFocus))
        Show = 0;

    if (cX >= wW || cY >= wW ||
        cX + 1 > wW || cY + 1 > wH)
    {
        DEBUGX(("bounds %d %d\n", wW, wH));
        return;
    }
    
    DEBUGX(("DrawCursor %d %d\n", cX, cY));
    //    if (!XtIsManaged(TextWin)) return ;

    if (cVisible && cX >= 0 && cY >= 0) {
        unsigned char* p = CursorXYPos(cX, cY);
        unsigned char attr;

        attr = p[1];
        //if (Show) attr = ((((attr << 4) & 0xF0)) | (attr >> 4)) ^ 0x77;
        if (Show) attr = (attr ^ 0x77);

        if (GCs[attr] == 0) {
            XGCValues gcv;
            
            gcv.foreground = colors[attr & 0xF].pixel();
            gcv.background = colors[(attr >> 4) & 0xF].pixel();
            gcv.font = fontStruct->fid;
            GCs[attr] = XCreateGC(display, qView->text->winId(),
                                  GCForeground | GCBackground | GCFont,
                                  &gcv);
        }

        XDrawImageString(display, qView->text->winId(), GCs[attr & 0xFF],
                         /*qView->frameWidth() +*/ cX * cxChar,
                         /*qView->frameWidth() +*/ fontStruct->max_bounds.ascent + cY * cyChar,
                         (char *)p, 1);

        /*inter->setPen(colors[attr & 0xF]);
        painter->setBackgroundColor(colors[(attr >> 4) & 0xF]);

        painter->drawText(qView->frameWidth() + cX * cxChar,
                          qView->frameWidth() + cY * cyChar + fmAscent,
                          p, 1);*/
    }
}

int GViewPeer::ConPutBox(int X, int Y, int W, int H, PCell Cell) {
    int i;
    unsigned char temp[256], attr;
    unsigned char *p, *ps, *c, *ops;
    int len, x, l, ox, olen, skip;
    //int local_painter = 0;

    
    if (!(View && View->Parent))
        return 1;

    if (qView->text->winId() == 0)
        return 1;
    
    //if (Visibility == VisibilityFullyObscured)
    //    return - 1;
        
    if (X >= wW || Y >= wH ||
        X + W > wW || Y + H > wH)
    {
        DEBUGX(("bounds %d %d\n",wW, wH));
        return -1;
    }

    /*if (qView->Painter == 0) {
     DEBUGX(("{{{ New painter\n"));
        qView->Painter = new QPainter();
        qView->Painter->begin(qView);
        local_painter = 1;
    }
    qView->Painter->setBackgroundMode(OpaqueMode);
    qView->Painter->setFont(Font);*/
        
    DEBUGX(("PutBox %d | %d %d %d %d | %d %d\n", wRefresh, X, Y, W, H, wW, wH));
    for (i = 0; i < H; i++) {
        len = W;
        p = CursorXYPos(X, Y + i);
        ps = (unsigned char *) Cell;
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
            
            /*qView->Painter->setPen(colors[attr & 0xF]);
            qView->Painter->setBackgroundColor(colors[(attr >> 4) & 0xF]);

            qView->Painter->drawText(qView->frameWidth() + x * cxChar,
                              qView->frameWidth() + (Y + i) * cyChar + fmAscent,
                              (char *)temp, l);*/

            if (GCs[attr] == 0) {
                XGCValues gcv;

                gcv.foreground = colors[attr & 0xF].pixel();
                gcv.background = colors[(attr >> 4) & 0xF].pixel();
                gcv.font = fontStruct->fid;
                GCs[attr] = XCreateGC(display, qView->text->winId(),
                                      GCForeground | GCBackground | GCFont,
                                      &gcv);
            }

            XDrawImageString(display, qView->text->winId(), GCs[attr & 0xFF],
                             /*qView->frameWidth() +*/ x * cxChar,
                             /*qView->frameWidth() +*/ fontStruct->max_bounds.ascent + (Y + i) * cyChar,
                             (char *)temp, l);
            
            x += l;
            len -= l;
        }
        p = CursorXYPos(X, Y + i);
        memmove(p, Cell, W * sizeof(TCell));
        if (i + Y == cY)
            DrawCursor(1);
        Cell += W;
    }
    /*if (local_painter) {
     DEBUGX(("}}} Destroying painter\n"));
     qView->Painter->end();
        delete qView->Painter;
        qView->Painter = 0;
    }*/
    DEBUGX(("done putbox\n"));
    return 0;
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
    //XFlush(display);
    wRefresh = 0;
}

int GViewPeer::ConGetBox(int X, int Y, int W, int H, PCell Cell) {
    int i;
    
    for (i = 0; i < H; i++) {
        memcpy(Cell, CursorXYPos(X, Y + i), 2 * W);
        Cell += W;
    }
    return 0;
}

int GViewPeer::ConPutLine(int X, int Y, int W, int H, PCell Cell) {
    int i;
    
    for (i = 0; i < H; i++) {
        if (ConPutBox(X, Y + i, W, 1, Cell) != 0) return -1;
    }
    return 0;
}

int GViewPeer::ConSetBox(int X, int Y, int W, int H, TCell Cell) {
    TDrawBuffer B;
    int i;
    
    for (i = 0; i < W; i++) B[i] = Cell;
    ConPutLine(X, Y, W, H, B);
    return 0;
}

int GViewPeer::ConScroll(int Way, int X, int Y, int W, int H, TAttr Fill, int Count) {
    TCell Cell;
    int l;
    int fw = 0; //qView->frameWidth();

    if (qView->text->winId() == 0)
        return 1;

    if (GCs[Fill] == 0) {
        XGCValues gcv;

        gcv.foreground = colors[Fill & 0xF].pixel();
        gcv.background = colors[(Fill >> 4) & 0xF].pixel();
        gcv.font = fontStruct->fid;
        GCs[Fill] = XCreateGC(display, qView->text->winId(),
                              GCForeground | GCBackground | GCFont,
                              &gcv);
    }

    MoveCh(&Cell, ' ', Fill, 1);
    DrawCursor(0);
    if (Way == csUp) {
        XCopyArea(display, qView->text->winId(), qView->text->winId(), GCs[Fill],
                  fw + X * cxChar,
                  fw + (Y + Count) * cyChar,
                  W * cxChar,
                  (H - Count) * cyChar,
                  fw + X * cxChar,
                  fw + Y * cyChar
                 );
        for (l = 0; l < H - Count; l++) {
            memcpy(CursorXYPos(X, Y + l), CursorXYPos(X, Y + l + Count), 2 * W);
        }
        if (ConSetBox(X, Y + H - Count, W, Count, Cell) == -1) return -1;
    } else if (Way == csDown) {
        XCopyArea(display, qView->text->winId(), qView->text->winId(), GCs[Fill],
                  fw + X * cxChar,
                  fw + Y * cyChar,
                  W * cxChar, 
                  (H - Count) * cyChar, 
                  fw + X * cxChar,
                  fw + (Y + Count)* cyChar
                 );
        for (l = H - 1; l >= Count; l--) {
            memcpy(CursorXYPos(X, Y + l), CursorXYPos(X, Y + l - Count), 2 * W);
        }
        if (ConSetBox(X, Y, W, Count, Cell) == -1) return -1;
    }
    DrawCursor(1);
    return 0;

    /*TCell Cell;
    int l;
    
    MoveCh(&Cell, ' ', Fill, 1);
    QPainter painter;
    painter.begin(qView);
    painter.setBackgroundMode(OpaqueMode);
    painter.setFont(Font);
    DrawCursor(&painter, 0);
    if (Way == csUp) {
        bitBlt(qView,
               qView->frameWidth() + X * cxChar,
               qView->frameWidth() + Y * cyChar,
               qView,
               qView->frameWidth() + X * cxChar,
               qView->frameWidth() + (Y + Count) * cyChar,
               W * cxChar, (H - Count) * cyChar,
               CopyROP, TRUE);
               for (l = 0; l < H - Count; l++) {
               memcpy(CursorXYPos(X, Y + l), CursorXYPos(X, Y + l + Count), 2 * W);
               }
               } else if (Way == csDown) {
               bitBlt(qView,
               qView->frameWidth() + X * cxChar,
               qView->frameWidth() + (Y + Count) * cyChar,
               qView,
               qView->frameWidth() + X * cxChar,
               qView->frameWidth() + Y * cyChar,
               W * cxChar, (H - Count) * cyChar,
               CopyROP, TRUE);
               for (l = H - 1; l >= Count; l--) {
               memcpy(CursorXYPos(X, Y + l), CursorXYPos(X, Y + l - Count), 2 * W);
               }
               }
               DrawCursor(&painter, 1);
               painter.end();
               if (Way == csUp) {
               ConSetBox(X, Y, W, Count, Cell);
               } else if (Way == csDown) {
               ConSetBox(X, Y + H - Count, W, Count, Cell);
               }
               return 0;*/
}

int GViewPeer::ConSetSize(int X, int Y) {
    unsigned char *NewBuffer;
    unsigned char *p;
    int i;
    int MX, MY;
    
    p = NewBuffer = (unsigned char *) malloc(X * Y * 2);
    if (NewBuffer == NULL) return -1;
    for (i = 0; i < X * Y; i++) {
        *p++ = ' ';
        *p++ = 0x07;
    }
    if (ScreenBuffer) {
	MX = wW; if (X < MX) MX = X;
        MY = wH; if (Y < MY) MY = Y;
        if (X < MX) MX = X;
        p = NewBuffer;
        for (i = 0; i < MY; i++) {
            memcpy(p, CursorXYPos(0, i), MX * 2);
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
//        qView->setSize(0,
//        XResizeWindow(display, win, ScreenCols * cxChar, ScreenRows * cyChar);
    qView->show();
    return 1;
}

int GViewPeer::ConQuerySize(int *X, int *Y) {
    if (X) *X = wW;
    if (Y) *Y = wH;
    return 1;
}

int GViewPeer::ConSetCursorPos(int X, int Y) {
    if (X < 0) X = 0;
    if (X >= wW) X = wW - 1;
    if (Y < 0) Y = 0;
    if (Y >= wH) Y = wH - 1;

    /*QPainter painter;
    painter.begin(qView);
    painter.setBackgroundMode(OpaqueMode);
    painter.setFont(Font);*/
    DrawCursor(0);
    cX = X;
    cY = Y;
    DrawCursor(1);
    //painter.end();
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
    else
        return 1;
}

int GViewPeer::ExpandHeight(int DeltaY) {
    return 0;
}

int GViewPeer::QuerySbVPos() {
    return sbVstart;
}

int GViewPeer::SetSbVPos(int Start, int Amount, int Total) {
    if (sbVstart != Start ||
        sbVamount != Amount ||
        sbVtotal != Total)
    {
        sbVstart = Start;
        sbVamount = Amount;
        sbVtotal = Total;
        
        if (View->Parent == 0)
            return 0;
        if (Amount < 1 || Start + Amount > Total) {
            qView->vert->setRange(0, 0);
            qView->vert->setSteps(0, 0);
            qView->vert->setValue(0);
        } else {
            qView->vert->setRange(0, Total - Amount);
            qView->vert->setSteps(1, Amount);
            qView->vert->setValue(Start);
        }
    }
    return 1;
}

int GViewPeer::SetSbHPos(int Start, int Amount, int Total) {
    if (sbHstart != Start ||
        sbHamount != Amount ||
        sbHtotal != Total)
    {
        sbHstart = Start;
        sbHamount = Amount;
        sbHtotal = Total;
        
        if (View->Parent == 0)
            return 0;
        
        if (Amount < 1 || Start + Amount > Total) {
            qView->horz->setRange(0, 0);
            qView->horz->setSteps(0, 0);
            qView->horz->setValue(0);
        } else {
            qView->horz->setRange(0, Total - Amount);
            qView->horz->setSteps(1, Amount);
            qView->horz->setValue(Start);
        }
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
    return 1;
}

int GViewPeer::PMHideCursor() {
    return 1;
}

int GViewPeer::PMSetCursorPos() {
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
    if (Peer->ConSetSize(X, Y)) ;
//        Resize(X, Y);
    else
        return 0;
    return 1;
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
    } else {
        Peer->wState &= ~sfFocus;
    }
    Repaint();
}

int GView::CaptureMouse(int grab) {
    if (MouseCapture == 0) {
        if (grab)
            MouseCapture = this;
        else
            return 0;
    } else {
        if (grab || MouseCapture != this)
            return 0;
        else
            MouseCapture = 0;
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////

GFramePeer::GFramePeer(GFrame *aFrame, int Width, int Height) {
    Frame = aFrame;
    qFrame = new QEFrame(this, 0, 0);
    CHECK_PTR(qFrame);

    if (Width != -1 && Height != -1)
        ConSetSize(Width, Height);
}

GFramePeer::~GFramePeer() {
    delete qFrame;
}

int GFramePeer::ConSetSize(int X, int Y) {
    //return ::ConSetSize(X, Y);
    return 0;
}

int GFramePeer::ConQuerySize(int *X, int *Y) {
//    ::ConQuerySize(&fW, &fH);
//    if (X) *X = fW;
//    if (Y) *Y = fH;
    return 1;
}   

//int GFrame::ConQuerySize(int *X, int *Y) {
//    ::ConQuerySize(X, Y);
//    if (ShowVScroll)
//        --*X;
//}

int GFramePeer::ConSetTitle(const char *Title, const char *STitle) {
    qFrame->setCaption(Title);
    qFrame->setIconText(STitle);
    return 1;
}

int GFramePeer::ConGetTitle(char *Title, size_t MaxLen, char *STitle, size_t SMaxLen) {
    strlcpy(Title, qFrame->caption(), MaxLen);
    strlcpy(STitle, qFrame->iconText(), SMaxLen);
    return 1;
}

void GFramePeer::MapFrame() {
    qApp->setMainWidget(qFrame);
    qFrame->menubar->show();

    int menubarHeight =
        qFrame->menubar->height();

    qFrame->setMinimumSize(30 * cxChar + SCROLLBAR_SIZE + EDIT_BORDER * 2,
                           8 * cyChar + SCROLLBAR_SIZE + EDIT_BORDER * 2 +
                           menubarHeight);
    qFrame->setMaximumSize(160 * cxChar + SCROLLBAR_SIZE + EDIT_BORDER * 2,
                           100 * cyChar + SCROLLBAR_SIZE + EDIT_BORDER * 2 +
                           menubarHeight);

    qFrame->resize(80 * cxChar + SCROLLBAR_SIZE + EDIT_BORDER * 2,
                   40 * cyChar + SCROLLBAR_SIZE + EDIT_BORDER * 2 +
                   menubarHeight);
    qFrame->show();

    //    qFrame->setSizeIncrement(cxChar, cyChar);
    {
        XSizeHints hints;

        hints.flags = PBaseSize | PResizeInc;
        hints.width_inc = cxChar;
        hints.height_inc = cyChar;
        hints.base_width = SCROLLBAR_SIZE + EDIT_BORDER * 2;
        hints.base_height = SCROLLBAR_SIZE + EDIT_BORDER * 2 + menubarHeight;

        XSetWMNormalHints(display, qFrame->winId(), &hints);
    }
}

void GFramePeer::ShowFrame() {
    qFrame->show();
}

///////////////////////////////////////////////////////////////////////////

GFrame::GFrame(int XSize, int YSize) {
    Menu = 0;
    if (frames == 0) {
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
    if (Peer) {
        delete Peer;
        Peer = 0;
    }
    if (Next == this) {
        frames = 0;
//        DEBUGX(("No more frames\x7\x7\n"));
    } else {
        Next->Prev = Prev;
        Prev->Next = Next;
        frames = Next;
    }
    Next = Prev = 0;
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
    newview->Peer->wH = view->Peer->wH - view->Peer->wH / 2;
    view->Peer->wH /= 2;
    InsertView(view, newview);
    view->ConSetSize(view->Peer->wW, view->Peer->wH);
    newview->ConSetSize(newview->Peer->wW, newview->Peer->wH);
    return 0;
}

int GFrame::ConCloseView(GView *view) {
    return 0;
}

int GFrame::ConResizeView(GView *view, int DeltaY) {
    return 0;
}

int GFrame::AddView(GView *view) {
    if (Top != 0) {
        return ConSplitView(Top, view);
    } else {
//        int W, H;
        
        view->Parent = this;
        view->Prev = view->Next = 0;
        
//        view->Peer->wX = 0;
//        view->Peer->wY = 0;
//        ConQuerySize(&W, &H);
//        view->ConSetSize(W, H);
        InsertView(Top, view);
        return 0;
    }
}

void GFrame::Update() {
    GView *v = Active;
    
    UpdateMenu();
    while (v) {
        v->Update();
        v = v->Next;
        if (v == Active) 
            break;
    }
}

void GFrame::UpdateMenu() {
}

void GFrame::Repaint() {
    GView *v = Active;
    
    while (v) {
        v->Repaint();
        v = v->Next;
        if (v == Active) 
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
        
        if (Top == view) {
            Top = view->Next;
//            Top->Peer->wY -= view->Peer->wH;
//            Top->ConSetSize(Top->Peer->wW, Top->Peer->wH + view->Peer->wH);
        } else {
//            view->Prev->ConSetSize(view->Prev->Peer->wW,
//                                   view->Prev->Peer->wH + view->Peer->wH);
        }
        
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
    else if (c == 0)
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
    //if (Active)
    //    XtSetKeyboardFocus(Peer->PanedWin, Active->Peer->TextWin);
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
    //if (Active)
    //    XtSetKeyboardFocus(Peer->PanedWin, Active->Peer->TextWin);
    return 1;
}

void GFrame::Resize(int width, int height) {
    if (!Top)
        return;
    
    if (width < 8 || height < 2)
        return;
    
    if (Top == Top->Next) {
        Top->ConSetSize(width, height);
    } else {
    }
}

void GFrame::Show() {
    Update();
    Peer->MapFrame();
}

void GFrame::Activate() {
    frames = this;
    Update();
    Peer->ShowFrame();
}

QPopupMenu *QEFrame::CreatePopup(QWidget *parent, int Id, int do_connect) {
    QPopupMenu *menu = new QPopupMenu(parent);
    CHECK_PTR(menu);

    //menu->setFont(QFont("Helvetica", 12, QFont::Bold));
    for (unsigned i = 0; i < Menus[Id].Count; ++i) {
        if (Menus[Id].Items[i].Name) {
            //puts(Menus[Id].Items[i].Name);
            if (Menus[Id].Items[i].SubMenu != -1) {
                QPopupMenu *submenu =
                    CreatePopup(0,
                                Menus[Id].Items[i].SubMenu, 0);
                CHECK_PTR(submenu);

                menu->insertItem(Menus[Id].Items[i].Name,
                                    submenu);
            } else {
                menu->insertItem(Menus[Id].Items[i].Name,
                                 Menus[Id].Items[i].Cmd);
            }
        } else {
            menu->insertSeparator();
        }
    }
    if (do_connect)
        connect(menu, SIGNAL(activated(int)), SLOT(selectedMain(int)));
    return menu;
}

QMenuBar *QEFrame::CreateMenuBar(QWidget *parent, int Id) {
    QMenuBar *menu = new QMenuBar(parent);
    CHECK_PTR(menu);

    //menu->setFont(QFont("Helvetica", 12, QFont::Bold));
    for (unsigned i = 0; i < Menus[Id].Count; ++i) {
        if (Menus[Id].Items[i].Name) {
            //puts(Menus[Id].Items[i].Name);

            if (Menus[Id].Items[i].SubMenu != -1) {
                QPopupMenu *submenu =
                    CreatePopup(0,
                                Menus[Id].Items[i].SubMenu, 0);
                CHECK_PTR(submenu);

                menu->insertItem(Menus[Id].Items[i].Name,
                                    submenu);
            } else {
                menu->insertItem(Menus[Id].Items[i].Name,
                                 Menus[Id].Items[i].Cmd);
            }
        } else {
            menu->insertSeparator();
        }
    }
    connect(menu, SIGNAL(activated(int)), SLOT(selectedMain(int)));
    return menu;
}

int GFrame::SetMenu(const char *Name) {
    int id = GetMenuId(Name);

    if (Menu)
        free(Menu);
    Menu = strdup(Name);

    if (Peer->qFrame->menubar) {
        delete Peer->qFrame->menubar;
        Peer->qFrame->menubar = 0;
    }

    DEBUGX(("setting main menu: %s, id=%d\n", Name, id));
    Peer->qFrame->menubar = Peer->qFrame->CreateMenuBar(Peer->qFrame, id);
    //Peer->qFrame->menubar->setFrameStyle(QFrame::Raised | QFrame::Panel);
    //Peer->qFrame->menubar->setLineWidth(1);
    Peer->qFrame->menubar->show();
    return 1;
}

int GFrame::ExecMainMenu(char Sub) {
    return 0;
}

int GFrame::PopupMenu(const char *Name) {
    int id = GetMenuId(Name);

    QPopupMenu *popup = Peer->qFrame->CreatePopup(0, id, 1);
    popup->popup(LastMousePos);
    return 1;
}

void QEFrame::timerDone() {
    FinalExit = 0;
    qApp->exit_loop();
}

// GUI

GUI::GUI(int &argc, char **argv, int XSize, int YSize) :
    fArgc(argc),
    fArgv(argv)
{

    new QApplication(argc, argv);

    display = qt_xdisplay();
    
    char *fs = getenv("VIOFONT");
    fontStruct = NULL;
    if (fs == 0 && WindowFont[0] != 0)
        fs = WindowFont;
    if (fs) 
        fontStruct = XLoadQueryFont(display, fs);
    if (fontStruct == NULL) 
        fontStruct = XLoadQueryFont(display, "8x13");
    if (fontStruct == NULL)
        fontStruct = XLoadQueryFont(display, "fixed");
    if (fontStruct == NULL) 
        return;

    cxChar = fontStruct->max_bounds.width;
    cyChar = fontStruct->max_bounds.ascent + fontStruct->max_bounds.descent;

    gui = this;
}

GUI::~GUI() {
    gui = 0;
}

int GUI::ConGrabEvents(TEventMask EventMask) {
    return 0;
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
    return 0;
}

int GUI::ConPutEvent(const TEvent& Event) {
    EventBuf = Event;
    return 0;
}

int GUI::ConFlush() {
    return 0;
}

void GUI::ProcessEvent() {
    static int need_update = 1;

    /* should process until no more events,
     * but this is close enough */
    
    if (need_update)
        QTimer::singleShot(10, frames->Peer->qFrame, SLOT(timerDone()));

    FinalExit = 1;
    NextEvent.What = evNone;
    qApp->enter_loop();
    if (FinalExit && NextEvent.What == evNone) {
        DEBUGX(("Final exit\n"));
        //delete frames;
        //frames = 0;
        //return ;
    }

    if (need_update) {
        DEBUGX(("Updating\n"));
        frames->Update();
        need_update = 0;
    }
    while (doLoop && qHasEvent()) {
        NextEvent.What = evNone;
        qGetEvent(NextEvent);
        DEBUGX(("Got event: %d\n", (int)NextEvent.What));
        if (NextEvent.What == evMouseDown) {
            DEBUGX(("x:%d, y:%d, buttons:%d\n",
                    NextEvent.Mouse.X,
                    NextEvent.Mouse.Y,
                    NextEvent.Mouse.Buttons));
        }
        DispatchEvent(frames, NextEvent.Msg.View, NextEvent);
        NextEvent.What = evNone;
        need_update = 1;
    }
}

int GUI::Run() {
    if (Start(fArgc, fArgv) == 0) {
        doLoop = 1;
        frames->Show();

        while (doLoop)
            ProcessEvent();

        Stop();
        return 0;
    }
    return 1;
}

int GUI::ShowEntryScreen() {
    return 1;
}

int GUI::RunProgram(int mode, char *Command) {
    char Cmd[1024] = {0};

    //strncpy(Cmd, XShellCommand, sizeof(Cmd));

    if (*Command == 0)  // empty string = shell
        strlcat(Cmd, " -ls &", sizeof(Cmd));
    else {
        strlcat(Cmd, " -e ", sizeof(Cmd));
	strlcat(Cmd, Command, sizeof(Cmd));
	if (mode == RUN_ASYNC)
            strlcat(Cmd, " &", sizeof(Cmd));
    }

    return system(Cmd);
}

/*void PipeCallback(GPipe *pipe, int *source, int *input) {
    if (pipe && pipe->notify && *source == pipe->fd) {
        NextEvent.What = evNotify;
        NextEvent.Msg.View = frames->Active;
        NextEvent.Msg.Model = pipe->notify;
        NextEvent.Msg.Command = cmPipeRead;
        NextEvent.Msg.Param1 = pipe->id;
        pipe->stopped = 0;
    }
        DEBUGX(("Pipe %d\n", *source));
}
  */
int GUI::OpenPipe(const char *Command, EModel *notify) {
    int i;
    
    for (i = 0; i < MAX_PIPES; i++) {
        if (Pipes[i].used == 0) {
            int pfd[2];
            
            Pipes[i].id = i;
            Pipes[i].notify = notify;
            Pipes[i].stopped = 1;
            
            if (pipe((int *)pfd) == -1)
                return -1;
            
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
            //Pipes[i].input =
            //    XtAppAddInput(AppContext, Pipes[i].fd, XtInputReadMask, PipeCallback, &Pipes[i]);
            Pipes[i].used = 1;
            DEBUGX(("Pipe Open: %d\n", i));
            return i;
        }
    }
    return -1;
}

int GUI::SetPipeView(int id, EModel *notify) {
    if (id < 0 || id > MAX_PIPES)
        return -1;
    if (Pipes[id].used == 0)
        return -1;
    DEBUGX(("Pipe View: %d %p\n", id, notify));
    Pipes[id].notify = notify;
    if (notify != Pipes[id].notify) {
        if (notify) {
            //Pipes[id].input =
            //    XtAppAddInput(AppContext, Pipes[id].fd, XtInputReadMask, PipeCallback, &Pipes[id]);
        } else {
            //if (Pipes[id].input != 0) {
            //    XtRemoveInput(Pipes[id].input);
          //      Pipes[id].input = 0;
            //}
	}
    }

    return 0;
}

ssize_t GUI::ReadPipe(int id, void *buffer, size_t len) {
    ssize_t rc;
    
    if (id < 0 || id > MAX_PIPES)
        return -1;
    if (Pipes[id].used == 0)
        return -1;
    DEBUGX(("Pipe Read: Get %d %d\n", id, (int)len));
    
    rc = read(Pipes[id].fd, buffer, len);
    DEBUGX(("Pipe Read: Got %d %d\n", id, (int)len));
    if (rc == 0) {
        //if (Pipes[id].input != 0) {
            //XtRemoveInput(Pipes[id].input);
       //     Pipes[id].input = 0;
       // }
        close(Pipes[id].fd);
        return -1;
    }
    if (rc == -1)
        Pipes[id].stopped = 1;

    return rc;
}

int GUI::ClosePipe(int id) {
    int status;
    
    if (id < 0 || id > MAX_PIPES)
        return -1;
    if (Pipes[id].used == 0)
        return -1;
    waitpid(Pipes[id].pid, &status, 0);
    DEBUGX(("Pipe Close: %d\n", id));
    Pipes[id].used = 0;
    return WEXITSTATUS(status);
}

int GUI::multiFrame() { return 1; }

int GetXSelection(int *len, char **data, int clipboard) {
    QClipboard *cb = QApplication::clipboard();
    const char *text;
    QClipboard::Mode mode;
    switch (clipboard) {
    case 0:
        mode = QClipboard::Clipboard;
        break;
    case 1:
        mode = QClipboard::Selection;
        break;
    default:
        // not supported
        return -1;
    }

    text = cb->text(mode);
    if (text == 0)
        return -1;

    *len = (int)strlen(text);
    *data = (char *)malloc(*len);
    if (*data == 0)
        return -1;
    memcpy(*data, text, *len);
    return 0;
}

int SetXSelection(int len, char *data, int clipboard) {
    QClipboard* cb = QApplication::clipboard();
    char *text = (char *)malloc(len + 1);
    QClipboard::Mode mode;
    if (text == 0)
        return -1;
    switch (clipboard) {
    case 0:
        mode = QClipboard::Clipboard;
        break;
    case 1:
        mode = QClipboard::Selection;
        break;
    default:
        // not supported
        return -1;
    }
    memcpy(text, data, len);
    text[len] = 0;
    cb->setText(text, mode);
    free(text);
    return 0;
}

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
