/*    g_pm.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

/*
 * here's how it works:
 * there's one visible and one object window per view
 * events are forwarded from the visible on to object window and pulled
 * into the editor from the worker thread (there's only one, FTE
 * editor core is single-threaded).
 * SIQ is never blocked.
 * the only problem is that window doesn't repaint correctly after resize,
 * until the worker thread finishes processing, but we can't really
 * do anything with this as the editor core is not thread-safe.
 */

#define INCL_WIN
#define INCL_GPI
#define INCL_VIO
#define INCL_AVIO
#define INCL_DOS
#define INCL_DOSERRORS

#include "c_color.h"
#include "c_commands.h"
#include "c_config.h"
#include "c_history.h"
#include "c_mode.h"
#include "ftever.h"
#include "gui.h"
#include "log.h"
#include "s_files.h"
#include "s_string.h"
#include "sysdep.h"

#include <ctype.h>
#include <os2.h>
#include <process.h>
#include <stdarg.h>
#include <stdio.h>

#define PM_STACK_SIZE (96 * 1024)

#define UWM_NOTIFY (WM_USER + 1)
#define UWM_DESTROY (WM_USER + 2)
#define UWM_DESTROYHWND (WM_USER + 3)
#define UWM_DROPPEDFILE (WM_USER + 4)
#define UWM_FILEDIALOG (WM_USER + 5)
#define UWM_DLGBOX (WM_USER + 6)
#define UWM_PROCESSDLG (WM_USER + 7)
#define UWM_CHOICE (WM_USER + 8)
#define UWM_CREATECHILD (WM_USER + 9)
#define UWM_CREATEWORKER (WM_USER + 10)
#define UWM_CREATEFRAME (WM_USER + 11)
#define UWM_CREATEMAINMENU (WM_USER + 12)
#define UWM_CREATEPOPUPMENU (WM_USER + 13)

#define CURSOR_TYPE (CURSOR_FLASH | CURSOR_SOLID)

//#define SIZER_HEIGHT  4

#define FID_MTOOLBAR  10001

#define MAXXSIZE  160
#define MAXYSIZE  96

#define MAX_PIPES 4
#define PIPE_BUFLEN 4096

typedef struct {
    int used;
    int id;
    int reading, stopped;
    TID tid;
    HMTX Access;
    HEV ResumeRead;
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

#define sfFocus   1

typedef struct _PMPTR { // for passing pointers to winprocs
    USHORT len;
    void *p;
} PMPTR;

class GViewPeer;

struct PMData {
    GViewPeer *Peer;
    HVPS hvps;
    HPS hps;
    SHORT cxChar;
    SHORT cyChar;
    HWND hwndWorker;
};

class GViewPeer {
public:
    GView *View;
//    int wX, wY;
    int wW, wH, wState;
    int cX, cY, cVisible, cStart, cEnd;
    int sbVstart, sbVamount, sbVtotal;
    int sbHstart, sbHamount, sbHtotal;
    
    HWND hwndView;
    HWND hwndVscroll, hwndHscroll;
    HWND hwndWorker;
    PMData *pmData;
    int OldMouseX, OldMouseY;
    
    GViewPeer(GView *view, int XSize, int YSize);
    ~GViewPeer();

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
    HWND hwndFrame;
    HWND menuBar;
    HWND hwndToolBar;
    PFNWP oldFrameProc;
    
    GFramePeer(GFrame *aFrame, int Width, int Height);
    ~GFramePeer();
    
    int ConSetTitle(const char *Title, const char *STitle);
    int ConGetTitle(char *Title, size_t MaxLen, char *STitle, size_t SMaxLen);
    
    int ConSetSize(int X, int Y);
    int ConQuerySize(int *X, int *Y);
    void MapFrame();
    void ShowFrame();
    void SizeFrame();
};

int ShowVScroll = 1;
int ShowHScroll = 0;
int ShowMenuBar = 1;
int ShowToolBar = 1;
unsigned long HaveGUIDialogs =
    GUIDLG_FILE |
    GUIDLG_CHOICE |
    GUIDLG_FIND |
    GUIDLG_FINDREPLACE |
    GUIDLG_PROMPT;
extern int PMDisableAccel;

GFrame *frames = 0;
GUI *gui = 0;

GView *MouseCapture = 0;
GView *FocusCapture = 0;

static HEV WorkerStarted, StartInterface;

HWND CreatePMMainMenu(HWND parent, HWND owner, char *Name);
HWND CreatePMMenu(HWND parent, HWND owner, int menu, int id, int style);
HWND CreateToolBar(HWND parent, HWND owner, int id);

MRESULT EXPENTRY FrameWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
//MRESULT EXPENTRY SizerWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY AVIOWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY ObjectWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY CreatorWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

HAB hab = 0;
HAB habW = 0;
static char szClient[] = "EViewer";
static char szObject[] = "EWorker";
static char szCreator[] = "ECreator";
//static char szSizeBar[] = "ESizeBar" // TODO
static ULONG flFrame =
    FCF_TITLEBAR      | FCF_SYSMENU  |
    FCF_SIZEBORDER    | FCF_MAXBUTTON | FCF_HIDEBUTTON |
    FCF_SHELLPOSITION | FCF_TASKLIST |
//    FCF_VERTSCROLL    | FCF_HORZSCROLL |
    FCF_MENU          | FCF_ICON;

SWP swp;
HMQ hmq = 0;
HMQ hmqW = 0;
HMTX hmtxPMData = 0;
ULONG cxScreen, cyScreen,
      cyTitleBar, //cyMenuBar,
      cxBorder, cyBorder,
      cxScrollBar, cyScrollBar;
SHORT reportSize = 1;
TEvent EventBuf = { evNone };
HWND hwndCreatorUser, hwndCreatorWorker;
char dragname[CCHMAXPATH];

#ifdef OLD_PMMENUTOOLBAR  /* implemented using menus */

struct {
    int id;
    HBITMAP handle;
    int cmd;
    int flags;
} tools[] =
{
//    { 101, 0, ExExitEditor, MIS_BITMAP },
//    { 0, 0, 0, MIS_SEPARATOR },
    { 102, 0, ExFileOpen, MIS_BITMAP },
    { 103, 0, ExFileSave, MIS_BITMAP },
    { 104, 0, ExFileClose, MIS_BITMAP },
    { 0, 0, 0, MIS_SEPARATOR },
    { 105, 0, ExFilePrev, MIS_BITMAP },
    { 106, 0, ExFileLast, MIS_BITMAP },
    { 107, 0, ExFileNext, MIS_BITMAP },
    { 0, 0, 0, MIS_SEPARATOR },
    { 108, 0, ExUndo, MIS_BITMAP },
    { 109, 0, ExRedo, MIS_BITMAP },
    { 0, 0, 0, MIS_SEPARATOR },
    { 110, 0, ExBlockCut, MIS_BITMAP },
    { 111, 0, ExBlockCopy, MIS_BITMAP },
    { 112, 0, ExBlockPasteStream, MIS_BITMAP },
    { 113, 0, ExBlockPasteColumn, MIS_BITMAP },
    { 0, 0, 0, MIS_SEPARATOR },
    { 114, 0, ExCompilePrevError, MIS_BITMAP },
    { 115, 0, ExCompileNextError, MIS_BITMAP },
    { 0, 0, 0, MIS_SEPARATOR },
    { 116, 0, ExTagFindWord, MIS_BITMAP },
    { 119, 0, ExTagPop, MIS_BITMAP },
    { 117, 0, ExTagNext, MIS_BITMAP },
    { 118, 0, ExTagPrev, MIS_BITMAP },
};

HWND CreateToolBar(HWND parent, HWND owner, int id) {
    HWND menu;
    int i;
    MENUITEM item;
    HPS ps;
    
    menu = WinCreateWindow(parent,
                           WC_MENU, "menu", WS_VISIBLE | MS_ACTIONBAR,
                           0, 0, 0, 0,
                           owner, HWND_TOP, id, 0, 0);

    //WinEnableWindowUpdate(hmenu, FALSE);
    
    ps = WinGetPS(menu);
    for (i = 0; i < sizeof(tools)/sizeof(tools[0]); i++) {
        if (tools[i].handle == 0 && (tools[i].flags & MIS_BITMAP)) {
            tools[i].handle = GpiLoadBitmap(ps, NULLHANDLE, tools[i].id, 0, 0);
        }
        memset((void *)&item, 0, sizeof(item));
        item.iPosition = i;
        item.hwndSubMenu = 0;
        item.afStyle = tools[i].flags;
        item.id = tools[i].cmd + 16384 + 8192;
        item.afAttribute = 0;
        item.hItem = tools[i].handle;
        WinSendMsg(menu, MM_INSERTITEM, MPFROMP(&item), MPFROMP(0));
    }
    WinReleasePS(ps);

    return menu;
}
#else
#include "pm_tool.h"
#include "pm_tool.cpp"

#define CMD(x) ((x) + 16384 + 8192)

ToolBarItem tools[] =
{
    //   { tiBITMAP, 101, CMD(ExExitEditor), 0, 0 },
    { tiBITMAP, 102, CMD(ExFileOpen), 0, 0 },
    { tiBITMAP, 103, CMD(ExFileSave), 0, 0 },
    { tiBITMAP, 104, CMD(ExFileClose), 0, 0 },
    { tiSEPARATOR, 0, 0, 0, 0},
    { tiBITMAP, 105, CMD(ExFilePrev), 0, 0 },
    { tiBITMAP, 106, CMD(ExFileLast), 0, 0 },
    { tiBITMAP, 107, CMD(ExFileNext), 0, 0 },
    { tiSEPARATOR, 0, 0, 0, 0},
    { tiBITMAP, 108, CMD(ExUndo), 0, 0 },
    { tiBITMAP, 109, CMD(ExRedo), 0, 0 },
    { tiSEPARATOR, 0, 0, 0, 0},
    { tiBITMAP, 110, CMD(ExBlockCut), 0, 0 },
    { tiBITMAP, 111, CMD(ExBlockCopy), 0, 0 },
    { tiBITMAP, 112, CMD(ExBlockPasteStream), 0, 0 },
    { tiBITMAP, 113, CMD(ExBlockPasteColumn), 0, 0 },
    { tiSEPARATOR, 0, 0, 0, 0},
    { tiBITMAP, 114, CMD(ExCompilePrevError), 0, 0 },
    { tiBITMAP, 115, CMD(ExCompileNextError), 0, 0 },
    { tiSEPARATOR, 0, 0, 0, 0},
    { tiBITMAP, 116, CMD(ExTagFindWord), 0, 0 },
    { tiBITMAP, 119, CMD(ExTagPop), 0, 0 },
    { tiBITMAP, 117, CMD(ExTagNext), 0, 0 },
    { tiBITMAP, 118, CMD(ExTagPrev), 0, 0 },
};

HWND CreateToolBar(HWND parent, HWND owner, int id) {
    STARTFUNC("CreateToolBar{g_pm.cpp}");
    static int reged = 0;
    HPS hps;
    unsigned int i;

    if (!reged) {
        RegisterToolBarClass(hab);
        reged = 1;
    }

    hps = WinGetPS(parent);
    
    for (i = 0; i < sizeof(tools)/sizeof(tools[0]); i++)
    {
        if (tools[i].hBitmap == 0 && (tools[i].ulType == tiBITMAP))
            tools[i].hBitmap = GpiLoadBitmap(hps, NULLHANDLE, tools[i].ulId, 0, 0);
    }
    
    WinReleasePS(hps);
    
    return CreateToolBar(parent, owner, id,
                         sizeof(tools)/sizeof(tools[0]),
                         tools);
}

#endif

HWND CreatePMMenu(HWND parent, HWND owner, int menu, int id, int style) {
    HWND hmenu;
    int i;
    MENUITEM item;
    char s[256];
    char *p;
    
    hmenu = WinCreateWindow(parent, 
                            WC_MENU, "menu", style & ~MS_CONDITIONALCASCADE,
                            0, 0, 0, 0, 
                            owner, HWND_TOP, id, 0, 0);
    
    //WinEnableWindowUpdate(hmenu, FALSE);
    
    for (i = 0; i < Menus[menu].Count; i++) {
        memset((void *)&item, 0, sizeof(item));
        item.iPosition = i;
        item.hwndSubMenu = 0;
        if (Menus[menu].Items[i].Name) {
            if (Menus[menu].Items[i].SubMenu != -1) {
                item.afStyle = MIS_SUBMENU | MIS_TEXT;
                item.hwndSubMenu = CreatePMMenu(HWND_DESKTOP, owner,
                                                Menus[menu].Items[i].SubMenu, 0,
                                                (Menus[menu].Items[i].Cmd == SUBMENU_CONDITIONAL) ? MS_CONDITIONALCASCADE : 0);
                {
                    static ids = 1000;
                    item.id = ids++;
                    if (ids == 7000) {
                        ids = 1000;
                    }
                }
            } else {
                item.afStyle = MIS_TEXT;
                item.id = (Menus[menu].Items[i].Cmd & 0xFFFF) + 8192; // ?
            }
        } else {
            item.afStyle = MIS_SEPARATOR;
            item.id = 0;
        }
        item.afAttribute = 0;
        item.hItem = 0;
        if (Menus[menu].Items[i].Name) {
            strcpy(s, Menus[menu].Items[i].Name);
            p = strchr(s, '&');
            if (p) 
                (*p) = '~';
            p = (char *)&s;
        } else {
            p = 0;
        }
        WinSendMsg(hmenu, MM_INSERTITEM, MPFROMP(&item), MPFROMP(p));
        if (i == 0 && style == MS_CONDITIONALCASCADE) {
            WinSetWindowBits(hmenu, QWL_STYLE,
                             MS_CONDITIONALCASCADE, MS_CONDITIONALCASCADE);
            WinSendMsg(hmenu, MM_SETDEFAULTITEMID, MPFROMSHORT(item.id), 0);
        }
    }
    //WinEnableWindowUpdate(hmenu, TRUE);
    return hmenu;
}

HWND CreatePMMainMenu(HWND parent, HWND owner, char *Name) {
    int id = GetMenuId(Name);
    HWND main;

    assert (id != -1);

    main = CreatePMMenu(parent, owner, id, FID_MENU, MS_ACTIONBAR);
    return main;
}

#include "pmdlg.h"

void InsertHistory(HWND hwnd, int id, int maxlen) {
    int i, count;
    char *str;
    count = CountInputHistory(id);
    
    str = (char *)malloc(maxlen + 1);
    if (str == 0)
        return;
    
    for (i = 0; i < count; i++) {
        if (GetInputHistory(id, str, maxlen, i + 1) == 1)
            WinInsertLboxItem(hwnd, LIT_END, str);
    }
    free(str);
}

MRESULT EXPENTRY FileDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2) {
    FILEDLG *dlg;

    dlg = (FILEDLG *)WinQueryWindowULong(hwnd, QWL_USER);
    
    switch (msg) {
    case WM_INITDLG:
        WinSendMsg(hwnd, WM_SETICON,
                   MPFROMLONG(WinLoadPointer(HWND_DESKTOP, 0, 1)), 0);
        
        InsertHistory(WinWindowFromID(hwnd, DID_FILENAME_ED), HIST_PATH, MAXPATH);
        WinInvalidateRect(hwnd, 0, TRUE);
        WinRestoreWindowPos("FTEPM",
                            ((dlg->fl & FDS_SAVEAS_DIALOG) ? "FileSaveDlg" : "FileOpenDlg"),
                            hwnd);
        break;
    case WM_COMMAND:
        switch (SHORT1FROMMP(mp1)) {
        case DID_OK:
            WinShowWindow(hwnd, FALSE);
            WinStoreWindowPos("FTEPM",
                              ((dlg->fl & FDS_SAVEAS_DIALOG) ? "FileSaveDlg" : "FileOpenDlg"),
                              hwnd);
            break;
            
        case DID_CANCEL:
            WinShowWindow(hwnd, FALSE);
            WinStoreWindowPos("FTEPM",
                              ((dlg->fl & FDS_SAVEAS_DIALOG) ? "FileSaveDlg" : "FileOpenDlg"),
                              hwnd);
            break;
        }
        break;
    case WM_CLOSE:
        WinShowWindow(hwnd, FALSE);
        WinStoreWindowPos("FTEPM",
                          ((dlg->fl & FDS_SAVEAS_DIALOG) ? "FileSaveDlg" : "FileOpenDlg"),
                          hwnd);
        break;
    }
    return WinDefFileDlgProc(hwnd, msg, mp1, mp2);
}

int DLGGetFile(GView *View, const char *Prompt, unsigned int BufLen, char *FileName, int Flags) {
    FILEDLG dlg;
    
    memset((void *)&dlg, 0, sizeof(dlg));
    
    dlg.cbSize = sizeof(dlg);
    dlg.fl =
        /*FDS_CENTER |*/ FDS_CUSTOM |
        ((Flags & GF_SAVEAS) ? FDS_SAVEAS_DIALOG : FDS_OPEN_DIALOG);
    dlg.pszTitle = (char*)Prompt;
    strcpy(dlg.szFullFile, FileName);
    dlg.hMod = NULLHANDLE;
    dlg.usDlgId = IDD_FILEDLG;
    dlg.pfnDlgProc = FileDlgProc;

    if (!LONGFROMMR(WinSendMsg(View->Parent->Peer->hwndFrame,
                               UWM_FILEDIALOG, MPFROMP(&dlg), 0)))
        return 0;
    
    if (dlg.lReturn == DID_OK) {
        strlcpy(FileName, dlg.szFullFile, BufLen);
        AddInputHistory(HIST_PATH, FileName);
        return 1;
    }
    return 0;
}

typedef struct {
    char *Title;
    int NSel;
    va_list ap;
    int Flags;
} ChoiceInfo;

static int DoChoice(HWND hwndFrame, ChoiceInfo *choice) {
    char msg[1024];
    char Prompt[1024];
    char *fmt;
    char *p;
    int rc;
    HWND hwndDlg;
    HWND hwndStatic;
    HWND hwndButton[40];
    int cyBorder, cxBorder;
    SWP swp, swp1;
    int i, x, y;
    ULONG flFrame = FCF_TITLEBAR | FCF_SYSMENU | FCF_DLGBORDER;
    HPS ps;
    int xw, yw, nx, ny;
    RECTL tr;
    int cp, cd;
    char msgbox[100];
    int SPC = 4;
    
    sprintf(msgbox, "MsgBox: %s", choice->Title);
    
    cxBorder = WinQuerySysValue(HWND_DESKTOP, SV_CXDLGFRAME);
    cyBorder = WinQuerySysValue(HWND_DESKTOP, SV_CYDLGFRAME);
    
    hwndDlg = WinCreateStdWindow(HWND_DESKTOP,
                                 WS_VISIBLE,
                                 &flFrame,
                                 0,
                                 choice->Title,
                                 0,
                                 0,
                                 0, 0);
    
    WinSendMsg(hwndDlg, WM_SETICON,
               MPFROMLONG(WinLoadPointer(HWND_DESKTOP, 0, 1)), 0);
    
    WinSetOwner(hwndDlg, hwndFrame);
    
    x = SPC;
    for (i = 0; i < choice->NSel; i++) {
        char button[60];
        strcpy(button, va_arg(choice->ap, char *));
        p = strchr(button, '&');
        if (p)
            *p = '~';
        
        hwndButton[i] =
            WinCreateWindow(hwndDlg,
                            WC_BUTTON,
                            button,
                            WS_VISIBLE | BS_PUSHBUTTON | BS_AUTOSIZE | ((i == 0) ? BS_DEFAULT | WS_TABSTOP | WS_GROUP: 0),
                            cxBorder + x, SPC + cyBorder, 0, 0,
                            hwndDlg, ((i == 0) ? HWND_TOP: hwndButton[i - 1]),
                            200 + i,
                            NULL, NULL);
        
        WinQueryWindowPos(hwndButton[i], &swp);
        x += SPC + swp.cx;
    }
    
    fmt = va_arg(choice->ap, char *);
    vsprintf(msg, fmt, choice->ap);
    strlcpy((PCHAR)Prompt, msg, sizeof(Prompt));

    hwndStatic = WinCreateWindow(hwndDlg,
                                 WC_STATIC,
                                 Prompt,
                                 WS_VISIBLE | SS_TEXT | DT_TOP | DT_LEFT | DT_WORDBREAK,
                                 0, 0, 0, 0,
                                 hwndDlg, HWND_TOP,
                                 100,
                                 NULL, NULL);
    
    WinRestoreWindowPos("FTEPM", msgbox, hwndDlg);
    
    xw = cxScreen / 2;
    if (x - SPC > xw)
        xw = x - SPC;
    
    yw = 0;
    ps = WinGetPS(hwndStatic);
    
    cp = 0;
    for (;;) {
        tr.xLeft = 0;
        tr.xRight = xw;
        tr.yTop = cyScreen / 2;
        tr.yBottom = 0;
        
        cd = WinDrawText(ps, -1, (Prompt + cp),
                         &tr,
                         0, 0,
                         DT_LEFT | DT_TOP | DT_WORDBREAK | DT_TEXTATTRS |
                         DT_QUERYEXTENT | DT_EXTERNALLEADING);
        if (!cd)
            break;
        cp += cd;
        yw += tr.yTop - tr.yBottom;
    }
    
    WinReleasePS(ps);
    
    WinSetWindowPos(hwndStatic, 0,
                    cxBorder + SPC,
                    cyBorder + SPC + swp.cy + SPC,
                    xw, yw, SWP_MOVE | SWP_SIZE);
    
    WinQueryWindowPos(hwndStatic, &swp1);
    WinQueryWindowPos(hwndButton[0], &swp);
    
    nx = cxBorder + SPC + xw + SPC + cxBorder;
    ny = cyBorder + SPC + swp.cy + SPC + swp1.cy + SPC + cyTitleBar + cyBorder;
    
    WinQueryWindowPos(hwndDlg, &swp);
    
    x = swp.x;
    y = swp.y + swp.cy - ny;
    
    if (y < cyBorder) y = - cyBorder;
    if (y + ny >= cyScreen + cyBorder) y = cyScreen - ny + cyBorder;
    if (x + nx >= cxScreen + cxBorder) x = cxScreen - nx + cxBorder;
    if (x < -cxBorder) x = -cxBorder;
    
    WinSetWindowPos(hwndDlg, 0,
                    x, y,
                    nx, ny,
                    SWP_SIZE | SWP_SHOW | SWP_MOVE | SWP_ACTIVATE);
    
    WinSubclassWindow(hwndDlg, (PFNWP) WinDefDlgProc);
    
    if (choice->Flags & (GPC_ERROR | GPC_FATAL))
        WinAlarm(HWND_DESKTOP, WA_ERROR);
    else if (choice->Flags & (GPC_CONFIRM))
        WinAlarm(HWND_DESKTOP, WA_NOTE);
    else if (choice->Flags & (GPC_WARNING))
        WinAlarm(HWND_DESKTOP, WA_WARNING);
    
    rc = LONGFROMMR(WinSendMsg(hwndFrame, UWM_PROCESSDLG,
                               MPFROMLONG(hwndDlg), 0));
    
    WinStoreWindowPos("FTEPM", msgbox, hwndDlg);
    
    WinSendMsg(hwndFrame, UWM_DESTROYHWND, MPFROMLONG(hwndDlg), 0);
    if (rc == DID_CANCEL || rc == DID_ERROR)
        return choice->NSel - 1;
    
    if (rc >= 200 && rc < choice->NSel + 200)
        return rc - 200;
    
    return 0;
}

/* reimplemented most of the WinMessageBox code to get store/restore position to work */
int DLGPickChoice(GView *View, const char *ATitle, int NSel, va_list ap, int Flags) {
    ChoiceInfo choice;

    choice.Title = (char *)ATitle;
    choice.NSel = NSel;
    choice.ap = ap;
    choice.Flags = Flags;
    return LONGFROMMR(WinSendMsg(View->Parent->Peer->hwndFrame, UWM_CHOICE, MPFROMP(&choice), 0));
}

static struct {
    char *Title;
    char *Entry;
    int MaxLen;
    int HistId;
} PromptInfo;

MRESULT EXPENTRY PromptDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2) {
    switch (msg) {
    case WM_INITDLG:
        WinSendMsg(hwnd, WM_SETICON,
                   MPFROMLONG(WinLoadPointer(HWND_DESKTOP, 0, 1)), 0);
        
        WinSendDlgItemMsg(hwnd, IDE_FIELD, EM_SETTEXTLIMIT, MPFROMLONG(PromptInfo.MaxLen), 0);
        WinSetDlgItemText(hwnd, IDE_FIELD, PromptInfo.Entry);
        InsertHistory(WinWindowFromID(hwnd, IDE_FIELD), PromptInfo.HistId, PromptInfo.MaxLen);
        WinSetDlgItemText(hwnd, IDS_PROMPT, PromptInfo.Title);
        WinSetWindowText(hwnd, PromptInfo.Title);
        WinInvalidateRect(hwnd, 0, TRUE);
        WinRestoreWindowPos("FTEPM", "PromptDlg", hwnd);
        return WinDefDlgProc(hwnd, msg, mp1, mp2);
        
    case WM_COMMAND:
        switch (SHORT1FROMMP(mp1)) {
        case DID_OK:
            WinQueryDlgItemText(hwnd, IDE_FIELD, PromptInfo.MaxLen, PromptInfo.Entry);
            PromptInfo.Entry[PromptInfo.MaxLen - 1] = 0;
            AddInputHistory(PromptInfo.HistId, PromptInfo.Entry);
            
            WinShowWindow(hwnd, FALSE);
            WinStoreWindowPos("FTEPM", "PromptDlg", hwnd);
            WinDismissDlg(hwnd, TRUE);
            return (MRESULT)FALSE;
            
        case DID_CANCEL:
            WinShowWindow(hwnd, FALSE);
            WinStoreWindowPos("FTEPM", "PromptDlg", hwnd);
            WinDismissDlg(hwnd, FALSE);
            return (MRESULT)FALSE;
        }
        break;
    case WM_CLOSE:
        WinShowWindow(hwnd, FALSE);
        WinStoreWindowPos("FTEPM", "PromptDlg", hwnd);
        /* passthru */
    default:
        return WinDefDlgProc(hwnd, msg, mp1, mp2);
    }
    return (MRESULT)FALSE;
}

int DLGGetStr(GView *View, const char *Prompt, unsigned int BufLen, char *Str, int HistId, int Flags) {
    assert(BufLen > 0);
    PromptInfo.MaxLen = BufLen - 1;
    PromptInfo.Title = (char *)Prompt;
    PromptInfo.Entry = Str;
    PromptInfo.HistId = HistId;

    if (LONGFROMMR(WinSendMsg(View->Parent->Peer->hwndFrame,
                   UWM_DLGBOX, MPFROMP(PFNWP(PromptDlgProc)), MPFROMLONG(IDD_PROMPT))) != DID_OK)
        return 0;
    
    return 1;
}

static SearchReplaceOptions SearchOpt;
static int ReplaceDlg = 0;

MRESULT EXPENTRY FindDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2) {
    switch (msg) {
    case WM_INITDLG:
        WinSendMsg(hwnd, WM_SETICON,
                   MPFROMLONG(WinLoadPointer(HWND_DESKTOP, 0, 1)), 0);
        
        WinSendDlgItemMsg(hwnd, IDE_FIND, EM_SETTEXTLIMIT, MPFROMLONG(MAXSEARCH), 0);
        WinSetDlgItemText(hwnd, IDE_FIND, SearchOpt.strSearch);
        InsertHistory(WinWindowFromID(hwnd, IDE_FIND), HIST_SEARCH, MAXSEARCH);
        WinCheckButton(hwnd, IDC_IGNORECASE, (SearchOpt.Options & SEARCH_NCASE) ? 1 : 0);
        WinCheckButton(hwnd, IDC_REGEXPS, (SearchOpt.Options & SEARCH_RE) ? 1 : 0);
        WinCheckButton(hwnd, IDC_WORDS, (SearchOpt.Options & SEARCH_WORD) ? 1 : 0);
        WinCheckButton(hwnd, IDC_BLOCK, (SearchOpt.Options & SEARCH_BLOCK) ? 1 : 0);
        WinCheckButton(hwnd, IDC_GLOBAL, (SearchOpt.Options & SEARCH_GLOBAL) ? 1 : 0);
        WinCheckButton(hwnd, IDC_REVERSE, (SearchOpt.Options & SEARCH_BACK) ? 1 : 0);
        WinCheckButton(hwnd, IDC_ALLOCCURENCES, (SearchOpt.Options & SEARCH_ALL) ? 1 : 0);
        WinCheckButton(hwnd, IDC_JOINLINE, (SearchOpt.Options & SEARCH_JOIN) ? 1: 0);
        if (ReplaceDlg) {
            WinSendDlgItemMsg(hwnd, IDE_REPLACE, EM_SETTEXTLIMIT, MPFROMLONG(MAXSEARCH), 0);
            WinSetDlgItemText(hwnd, IDE_REPLACE, SearchOpt.strReplace);
            InsertHistory(WinWindowFromID(hwnd, IDE_REPLACE), HIST_SEARCH, MAXSEARCH);
            WinCheckButton(hwnd, IDC_NOPROMPTING, (SearchOpt.Options & SEARCH_NASK) ? 1 : 0);
        } else {
            WinCheckButton(hwnd, IDC_DELETELINE, (SearchOpt.Options & SEARCH_DELETE) ? 1 : 0);
        }
        WinInvalidateRect(hwnd, 0, TRUE);
        WinRestoreWindowPos("FTEPM", ReplaceDlg ? "ReplaceDlg" : "FindDlg", hwnd);
        return WinDefDlgProc(hwnd, msg, mp1, mp2);
        
    case WM_COMMAND:
        switch (SHORT1FROMMP(mp1)) {
        case DID_OK:
            SearchOpt.ok = 1;
            SearchOpt.resCount = 0;
            SearchOpt.Options = 0;
            strcpy(SearchOpt.strReplace, "");
            WinQueryDlgItemText(hwnd, IDE_FIND, MAXSEARCH, SearchOpt.strSearch);
            SearchOpt.strSearch[MAXSEARCH - 1] = 0;
            AddInputHistory(HIST_SEARCH, SearchOpt.strSearch);
            
            if (WinQueryButtonCheckstate(hwnd, IDC_IGNORECASE))
                SearchOpt.Options |= SEARCH_NCASE;
            if (WinQueryButtonCheckstate(hwnd, IDC_REGEXPS))
                SearchOpt.Options |= SEARCH_RE;
            if (WinQueryButtonCheckstate(hwnd, IDC_WORDS))
                SearchOpt.Options |= SEARCH_WORD;
            if (WinQueryButtonCheckstate(hwnd, IDC_BLOCK))
                SearchOpt.Options |= SEARCH_BLOCK;
            if (WinQueryButtonCheckstate(hwnd, IDC_GLOBAL))
                SearchOpt.Options |= SEARCH_GLOBAL;
            if (WinQueryButtonCheckstate(hwnd, IDC_REVERSE))
                SearchOpt.Options |= SEARCH_BACK;
            if (WinQueryButtonCheckstate(hwnd, IDC_ALLOCCURENCES))
                SearchOpt.Options |= SEARCH_ALL;
            if (WinQueryButtonCheckstate(hwnd, IDC_JOINLINE))
                SearchOpt.Options |= SEARCH_JOIN;
            
            if (ReplaceDlg) {
                WinQueryDlgItemText(hwnd, IDE_REPLACE, MAXSEARCH, SearchOpt.strReplace);
                SearchOpt.strReplace[MAXSEARCH - 1] = 0;
                AddInputHistory(HIST_SEARCH, SearchOpt.strReplace);
                SearchOpt.Options |= SEARCH_REPLACE;
                if (WinQueryButtonCheckstate(hwnd, IDC_NOPROMPTING))
                    SearchOpt.Options |= SEARCH_NASK;
            } else {
                if (WinQueryButtonCheckstate(hwnd, IDC_DELETELINE))
                    SearchOpt.Options |= SEARCH_DELETE;
            }
            
            WinShowWindow(hwnd, FALSE);
            WinStoreWindowPos("FTEPM", ReplaceDlg ? "ReplaceDlg" : "FindDlg", hwnd);
            WinDismissDlg(hwnd, TRUE);
            return (MRESULT)FALSE;
            
        case DID_CANCEL:
            WinShowWindow(hwnd, FALSE);
            WinStoreWindowPos("FTEPM", ReplaceDlg ? "ReplaceDlg" : "FindDlg", hwnd);
            WinDismissDlg(hwnd, FALSE);
            return (MRESULT)FALSE;
        }
        break;
    case WM_CLOSE:
        WinShowWindow(hwnd, FALSE);
        WinStoreWindowPos("FTEPM", ReplaceDlg ? "ReplaceDlg" : "FindDlg", hwnd);
        /* passthru */
    default:
        return WinDefDlgProc(hwnd, msg, mp1, mp2);
    }
    return (MRESULT)FALSE;
}

int DLGGetFind(GView *View, SearchReplaceOptions &sr) {
    SearchOpt = sr;
    ReplaceDlg = 0;

    if (LONGFROMMR(WinSendMsg(View->Parent->Peer->hwndFrame, UWM_DLGBOX,
                   PVOIDFROMMP(PFNWP(FindDlgProc)), MPFROMLONG(IDD_FIND))) != DID_OK)
        return 0;
    
    sr = SearchOpt;
    
    return 1;
}

int DLGGetFindReplace(GView *View, SearchReplaceOptions &sr) {
    SearchOpt = sr;
    ReplaceDlg = 1;
    
    if (LONGFROMMR(WinSendMsg(View->Parent->Peer->hwndFrame, UWM_DLGBOX,
                   PVOIDFROMMP(PFNWP(FindDlgProc)), MPFROMLONG(IDD_FINDREPLACE))) != DID_OK)
        return 0;
    
    sr = SearchOpt;
    
    return 1;
}

struct {
    int vk;
    TKeyCode kc;
    char *name;
} lvirt[] = {
{ VK_F1, kbF1, "F1" },
{ VK_F2, kbF2, "F2" },
{ VK_F3, kbF3, "F3" },
{ VK_F4, kbF4, "F4" },
{ VK_F5, kbF5, "F5" },
{ VK_F6, kbF6, "F6" },
{ VK_F7, kbF7, "F7" },
{ VK_F8, kbF8, "F8" },
{ VK_F9, kbF9, "F9" },
{ VK_F10, kbF10, "F10" },
{ VK_F11, kbF11, "F11" },
{ VK_F12, kbF12, "F12" },

{ VK_ESC, kbEsc, "Esc" },
{ VK_ENTER, kbEnter | kfGray, "Enter" },
{ VK_NEWLINE, kbEnter, "Enter" },
{ VK_BACKSPACE, kbBackSp, "BackSp" },
{ VK_SPACE, kbSpace, "Space" },
{ VK_TAB, kbTab, "Tab" },
{ VK_BACKTAB, kbTab | kfShift, "Tab" },

{ VK_UP, kbUp, "Up" },
{ VK_DOWN, kbDown, "Down" },
{ VK_LEFT, kbLeft, "Left" },
{ VK_RIGHT, kbRight, "Right" },
{ VK_HOME, kbHome, "Home" },
{ VK_END, kbEnd, "End" },
{ VK_PAGEDOWN, kbPgDn, "PgDn" },
{ VK_PAGEUP, kbPgUp, "PgUp" },
{ VK_INSERT, kbIns, "Ins" },
{ VK_DELETE, kbDel, "Del" },

{ VK_CTRL, kbCtrl | kfModifier, "Ctrl" },
{ VK_ALT, kbAlt | kfModifier, "Alt" },
{ VK_ALTGRAF, kbAlt | kfModifier, "Alt" },
{ VK_SHIFT, kbShift | kfModifier, "Shift" },
{ VK_CAPSLOCK, kbCapsLock | kfModifier, "CapsLock" },
{ VK_NUMLOCK, kbNumLock | kfModifier, "NumLock" },
{ VK_SCRLLOCK, kbScrollLock | kfModifier, "ScrollLock" },
{ VK_BREAK, kbBreak, "Break" },
{ VK_PAUSE, kbPause, "Pause" },
{ VK_PRINTSCRN, kbPrtScr, "PrtScr" },
{ VK_SYSRQ, kbSysReq, "SysReq", },
};

char *ConvertKey(int ch, int virt, int flags, int scan, TEvent &Event) {
    int keyFlags = 0;
    static char name[40];
    char keyname[40];
    TKeyCode keyCode = 0;
    
    strcpy(keyname, "UNKNOWN");
    
    //printf("ch:%d, virt:%d, flags:%d, scan:%d\n", ch, virt, flags, scan);
    
    name[0] = 0;
    
    if (flags & KC_CTRL)
        keyFlags |= kfCtrl;
    if (flags & KC_ALT)
        keyFlags |= kfAlt;
    if (flags & KC_SHIFT)
        keyFlags |= kfShift;
    if ((ch != 0xE0) && ((ch & 0xFF) == 0xE0))
        keyFlags |= kfGray;
    
    if (keyFlags == kfAlt) {// do not produce anything on alt+XXX
        switch (scan) {
        case 71: case 72: case 73:
        case 75: case 76: case 77:
        case 79: case 80: case 81:
        case 82: case 83:
            return name;
        }
    }
    if (ch != 0 && (flags & KC_CHAR)) {
        switch (scan) {
        case 71: case 72: case 73:
        case 75: case 76: case 77:
        case 79: case 80: case 81:
        case 82: case 83:
            virt = 0;
        }
    }
    {
        int i;

        for (i = 0; i < (sizeof(lvirt)/sizeof(lvirt[0])); i++)
            if (lvirt[i].vk == virt) {
                keyCode = lvirt[i].kc;
                strcpy(keyname, lvirt[i].name);
                break;
            }
    }
    if (keyCode == 0) {
        char c[2];

        if( ch == 0 && scan == 86 ){
            // Fix for OS/2 bug with UK keyboard layout
            // This is shift-'\' (to the left of Z), which returns 0
            ch = '|';
        }

        c[0] = char(ch);
        c[1] = 0;
        
        if (ch == '+' && scan == 78)
            keyCode = '+' | kfGray;
        else if (ch == '-' && scan == 74)
            keyCode = '-' | kfGray;
        else if (ch == '*' && scan == 55)
            keyCode = '*' | kfGray;
        else if (ch == '/' && scan == 92)
            keyCode = '/' | kfGray;
        else {
            keyCode = ch;
            //if (keyFlags == kfShift)
            //    keyFlags = 0;
        }
        
        keyname[0] = 0;
        
        if (keyCode & kfGray)
            strcpy(keyname, "G+");
        
        strcat(keyname, c);
    }
    
    if ((keyFlags & (kfAlt | kfSpecial | kfGray)) == kfAlt) {
        if (keyCode >= 'a' && keyCode <= 'z')
            keyCode -= 'a' - 'A';
    }
    
    if ((keyFlags & (kfCtrl | kfAlt | kfSpecial | kfGray)) == kfCtrl) {
        if (keyCode >= 'a' && keyCode < 'a' + 32)
            keyCode -= 'a' - 1;
        else if (keyCode >= 'A' && keyCode < 'A' + 32)
            keyCode -= 'A' - 1;
    }

    if (keyFlags & kfCtrl)
        if (keyCode < 32)
            keyCode += 64;
    
    keyCode |= keyFlags;
    
    if (keyCode & kfKeyUp)
        strcat(name, "UP ");
    else
        strcat(name, "DN ");
    if (keyCode & kfAlt)
        strcat(name, "A+");
    if (keyCode & kfCtrl)
        strcat(name, "C+");
    if (keyCode & kfGray)
        strcat(name, "G+");
    if (keyCode & kfShift)
        strcat(name, "S+");
    strcat(name, keyname);
    
    Event.What = evKeyDown;
    if (flags & KC_KEYUP) {
        keyFlags |= kfKeyUp;
        Event.What = evKeyUp;
    }
    Event.Key.Code = keyCode;
    return name;
}

MRESULT CreateChild(HWND parent, GViewPeer *peer, PMData *pmData) {
    PMPTR ptr;

    ptr.len = sizeof(PMPTR);
    ptr.p = pmData;

    peer->hwndView = WinCreateWindow(parent,
                                     szClient, "FTE",
                                     WS_VISIBLE, 0, 0, 0, 0,
                                     NULLHANDLE, HWND_TOP, FID_CLIENT,
                                     (void *)&ptr, NULL);

    assert(peer->hwndView != NULLHANDLE);
    
    peer->hwndVscroll = WinCreateWindow(parent,
                                        WC_SCROLLBAR, "",
                                        WS_VISIBLE | SBS_VERT | SBS_AUTOTRACK, 0, 0, 0, 0,
                                        peer->hwndView, HWND_TOP, 0,
                                        (void *)&ptr, NULL);

    assert(peer->hwndVscroll != NULLHANDLE);

    peer->hwndHscroll = WinCreateWindow(parent,
                                        WC_SCROLLBAR, "",
                                        WS_VISIBLE | SBS_HORZ | SBS_AUTOTRACK, 0, 0, 0, 0,
                                        peer->hwndView, HWND_TOP, 0,
                                        (void *)&ptr, NULL);

    assert(peer->hwndHscroll != NULLHANDLE);
    
    return (MRESULT)TRUE;
}

BOOL CalcFrameSWP(HWND hwnd, PSWP pswp, BOOL bFrame) {
    BOOL bSuccess;
    RECTL rcl;
    
    rcl.xLeft = pswp->x;
    rcl.yBottom = pswp->y;
    rcl.xRight = pswp->x + pswp->cx;
    rcl.yTop = pswp->y + pswp->cy;
    
    bSuccess = WinCalcFrameRect(hwnd, &rcl, bFrame);
    
    pswp->x = rcl.xLeft;
    pswp->y = rcl.yBottom;
    pswp->cx = rcl.xRight - rcl.xLeft;
    pswp->cy = rcl.yTop - rcl.yBottom;
    
    return bSuccess;
}

MRESULT EXPENTRY FrameWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2) {
    GFramePeer *peer = (GFramePeer *) WinQueryWindowULong(hwnd, QWL_USER);
    
    if (peer) switch (msg) {
    case UWM_DESTROY:
        WinDestroyWindow(hwnd);
        return 0;
    
    case UWM_DESTROYHWND:
        WinDestroyWindow(LONGFROMMP(mp1));
        return 0;

    case UWM_FILEDIALOG:
        return MRFROMLONG(WinFileDlg(HWND_DESKTOP, hwnd, (FILEDLG *)PVOIDFROMMP(mp1)));

    case UWM_DLGBOX:
        return MRFROMLONG(WinDlgBox(HWND_DESKTOP, hwnd,
                                    PFNWP(PVOIDFROMMP(mp1)), 0, LONGFROMMP(mp2), NULL));

    case UWM_PROCESSDLG:
        return MRFROMLONG(WinProcessDlg(HWNDFROMMP(mp1)));

    case UWM_CHOICE:
        return MRFROMLONG(DoChoice(hwnd, (ChoiceInfo *)PVOIDFROMMP(mp1)));

    case UWM_CREATECHILD:
        //DosBeep(2500, 1000);
        return CreateChild(hwnd, (GViewPeer *)PVOIDFROMMP(mp1), (PMData *)PVOIDFROMMP(mp2));
            
    case WM_TRANSLATEACCEL:
        // check for keys not to be translated
        {
            QMSG *qmsg = (QMSG *)mp1;
            USHORT vk = SHORT2FROMMP((qmsg->mp2));
            USHORT fl = (USHORT)(SHORT1FROMMP((qmsg->mp1)) & (KC_ALT | KC_SHIFT | KC_CTRL | KC_KEYUP));
            USHORT ch = SHORT1FROMMP((qmsg->mp2));

            if ((vk == VK_MENU || vk == VK_F1) && fl == 0 ||
                vk == VK_NEWLINE && fl == KC_ALT ||
                vk == VK_ENTER && fl == KC_ALT ||
                vk == VK_SPACE && fl == KC_ALT)
                return (MRESULT)FALSE;

            if ((ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z') &&
                (fl & KC_ALT))
                    return (MRESULT)FALSE;

            if (PMDisableAccel)
                if (fl & KC_ALT)
                    if (vk >= VK_F1 && vk <= VK_F24)
                        return (MRESULT)FALSE;

        }
        break;

    /*case WM_CALCFRAMERECT:
        {
            PRECTL rcl = (PRECTL)PVOIDFROMMP(mp1);
            USHORT isFrame = SHORT1FROMMP(mp2);
            BOOL fSuccess = LONGFROMMR(peer->oldFrameProc(hwnd, msg, mp1, mp2));

            if (ShowToolBar && fSuccess) {
                SWP swpToolBar;
                HWND hwndToolBar = WinWindowFromID(hwnd, FID_MTOOLBAR);

                WinQueryWindowPos(hwndToolBar, &swpToolBar);
                WinSendMsg(hwndToolBar,
                           WM_ADJUSTWINDOWPOS,
                           MPFROMP(&swpToolBar),
                           MPARAM(0));

                if (isFrame)
                    rcl->yTop -= swpToolBar.cy;
                else
                    rcl->yTop += swpToolBar.cy;
            }

            return MRFROMLONG(fSuccess);
        }*/
        
    case WM_QUERYTRACKINFO:
        {
            MRESULT mr;
            
            mr = peer->oldFrameProc(hwnd, msg, mp1, mp2);
            
            if (mr == (MRESULT)FALSE)
                return mr;
            
            DosRequestMutexSem(hmtxPMData, SEM_INDEFINITE_WAIT);
            if ((SHORT1FROMMP(mp1) & TF_MOVE) != TF_MOVE) {
                PTRACKINFO pti;
                
                pti = (PTRACKINFO) PVOIDFROMMP(mp2);
                pti->cxGrid = peer->Frame->Top->Peer->pmData->cxChar;
                pti->cyGrid = peer->Frame->Top->Peer->pmData->cyChar;
                pti->cxKeyboard = peer->Frame->Top->Peer->pmData->cxChar;
                pti->cyKeyboard = peer->Frame->Top->Peer->pmData->cyChar;
                pti->fs |= TF_GRID;
            }
            DosReleaseMutexSem(hmtxPMData);
            return mr;
        }

    case WM_MINMAXFRAME:
        {
            PSWP pswp = (PSWP) PVOIDFROMMP(mp1);

            if (pswp->fl & SWP_MAXIMIZE) {
                GView *v;
                int cnt;
                SWP swpMenu;
                SWP swpToolBar;
                HWND hwndMenu = WinWindowFromID(hwnd, FID_MENU);
                HWND hwndToolBar = WinWindowFromID(hwnd, FID_MTOOLBAR);

                WinQueryWindowPos(hwndMenu, &swpMenu);
                swpMenu.x = 0;
                swpMenu.y = 0;
                swpMenu.cx = cxScreen - 2 * cxBorder;
                swpMenu.cy = cyScreen;
                WinSendMsg(hwndMenu,
                           WM_ADJUSTWINDOWPOS,
                           MPFROMP(&swpMenu),
                           MPARAM(0));

                if (ShowToolBar) {
                    WinQueryWindowPos(hwndToolBar, &swpToolBar);
                    swpToolBar.x = 0;
                    swpToolBar.y = 0;
                    swpToolBar.cx = cxScreen - 2 * cxBorder;
                    swpToolBar.cy = cyScreen;
                    WinSendMsg(hwndToolBar,
                               WM_ADJUSTWINDOWPOS,
                               MPFROMP(&swpToolBar),
                               MPARAM(0));
                } else {
                    swpToolBar.cy = 0;
                }
                
                DosRequestMutexSem(hmtxPMData, SEM_INDEFINITE_WAIT);

                pswp->cx = cxScreen - cxScrollBar;
                pswp->cy = cyScreen - cyTitleBar - swpMenu.cy - swpToolBar.cy;

                cnt = 0;
                v = peer->Frame->Top;
                while (v) {
                    cnt++;
                    v = v->Prev;
                    if (v == peer->Frame->Top) break;
                }
            
                pswp->cy -= cnt * cyScrollBar;
                
                if (pswp->cy < 0) pswp->cy = 0;
                
                if (v) {
                    pswp->cx /= v->Peer->pmData->cxChar;
                    if (pswp->cx > MAXXSIZE)
                        pswp->cx = MAXXSIZE;
                    pswp->cx *= v->Peer->pmData->cxChar;

                    pswp->cy /= v->Peer->pmData->cyChar;
                    if (pswp->cy > MAXYSIZE)
                        pswp->cy = MAXYSIZE;
                    pswp->cy *= v->Peer->pmData->cyChar;
                }
                
                pswp->cy += cnt * cyScrollBar;

                pswp->cx += cxBorder * 2 + cxScrollBar;
                pswp->cy += cyBorder * 2 + cyTitleBar + swpMenu.cy + swpToolBar.cy;
                
                pswp->y = cyScreen - pswp->cy + cyBorder;

                DosReleaseMutexSem(hmtxPMData);
                return (MRESULT)FALSE;
            }
        }
        break;

    case WM_ADJUSTWINDOWPOS:
        {
            PSWP pswp = (PSWP) PVOIDFROMMP(mp1);
            
            if (pswp->fl & (SWP_SIZE | SWP_MOVE | SWP_MAXIMIZE)) {
                GView *v;
                int cnt;
                SWP swpToolBar;

                if (pswp->cx < 0 || pswp->cy <= cyTitleBar + 2 * cyBorder)
                    break;
                
                DosRequestMutexSem(hmtxPMData, SEM_INDEFINITE_WAIT);
                //printf("Before 1: %d %d | %d %d\n", pswp->cx, pswp->x, pswp->cy, pswp->y);
                CalcFrameSWP(hwnd, pswp, TRUE);

                if (ShowToolBar) {
                    HWND hwndToolBar = WinWindowFromID(hwnd, FID_MTOOLBAR);
                    
                    WinQueryWindowPos(hwndToolBar, &swpToolBar);
                    swpToolBar.x = 0;
                    swpToolBar.y = 0;
                    swpToolBar.cx = pswp->cx;
                    swpToolBar.cy = pswp->cy;
                    WinSendMsg(hwndToolBar,
                               WM_ADJUSTWINDOWPOS,
                               MPFROMP(&swpToolBar),
                               MPARAM(0));
                } else {
                    swpToolBar.cy = 0;
                }
                
                pswp->cy -= swpToolBar.cy;
                
                pswp->cx -= cxScrollBar;
                
                cnt = 0;
                v = peer->Frame->Top;
                while (v) {
                    cnt++;
                    v = v->Prev;
                    if (v == peer->Frame->Top) break;
                }
                
                pswp->cy -= cnt * cyScrollBar;
                //if (pswp->cy < 0) pswp->cy = 0;
                
                if (v) {
                    pswp->cx /= v->Peer->pmData->cxChar;
                    //if (pswp->cx < 8) pswp->cx = 8;
                    if (pswp->cx > MAXXSIZE)
                        pswp->cx = MAXXSIZE;
                    pswp->cx *= v->Peer->pmData->cxChar;
                    
                    pswp->cy /= v->Peer->pmData->cyChar;
                    //if (pswp->cy < cnt * 2) pswp->cy = cnt * 2;
                    if (pswp->cy > MAXYSIZE)
                        pswp->cy = MAXYSIZE;
                    pswp->cy *= v->Peer->pmData->cyChar;
                }
                
                pswp->cy += cnt * cyScrollBar;
                
                pswp->cx += cxScrollBar;
                pswp->cy += swpToolBar.cy;
                CalcFrameSWP(hwnd, pswp, FALSE);
                DosReleaseMutexSem(hmtxPMData);
            }
        }
        break;
    
    case WM_QUERYFRAMECTLCOUNT:
        {
            SHORT sCount = SHORT1FROMMR(peer->oldFrameProc(hwnd, msg, mp1, mp2));
            GView *v;
            
            DosRequestMutexSem(hmtxPMData, SEM_INDEFINITE_WAIT);
            v = peer->Frame->Top;
            while (v) {
                sCount += (SHORT)3;
                v = v->Prev;
                if (v == peer->Frame->Top) break;
            }
            if (ShowToolBar)
                sCount++;
            DosReleaseMutexSem(hmtxPMData);
            return MRESULT(sCount - 1);
        }
    
    case WM_FORMATFRAME:
        {
            SHORT sCount = SHORT1FROMMR(peer->oldFrameProc(hwnd, msg, mp1, mp2));
            PSWP pswp;
            HWND Bhwnd;
            GView *v;
            int x, w, h, fl, y;
            int ctl, cnt;
            int Hy, H1, yPos;
            HWND hwndToolBar = WinWindowFromID(hwnd, FID_MTOOLBAR);
            SWP swpToolBar;
            PRECTL prcl;

            pswp = (PSWP) mp1;
            prcl = (PRECTL) mp2;

            DosRequestMutexSem(hmtxPMData, SEM_INDEFINITE_WAIT);
            sCount--;

            cnt = 0;
            v = peer->Frame->Top;
            while (v) {
                cnt++;
                v = v->Prev;
                if (v == peer->Frame->Top) break;
            }
            
            if (cnt == 0) {
                DosReleaseMutexSem(hmtxPMData);
                return MRFROMSHORT(sCount);
            }

            fl = pswp[sCount].fl;
            x = pswp[sCount].x;
            y = pswp[sCount].y;
            w = pswp[sCount].cx;
            h = pswp[sCount].cy;
            Bhwnd = pswp[sCount].hwndInsertBehind;

            if (ShowToolBar) {
                swpToolBar = pswp[sCount];
                WinSendMsg(hwndToolBar,
                           WM_ADJUSTWINDOWPOS,
                           MPFROMP(&swpToolBar),
                           MPARAM(0));


                pswp[sCount].hwndInsertBehind = Bhwnd;
                Bhwnd = pswp[sCount].hwnd = hwndToolBar;
                pswp[sCount].fl = fl;
                pswp[sCount].x = cxBorder;
                pswp[sCount].cx = w;

                if (ShowToolBar == 1) {
                    pswp[sCount].y = y + h - swpToolBar.cy;
                    pswp[sCount].cy = swpToolBar.cy;

                    h -= swpToolBar.cy;
                    if (prcl)
                        prcl->yTop -= swpToolBar.cy;
                } else if (ShowToolBar == 2) {
                    pswp[sCount].y = y;
                    pswp[sCount].cy = swpToolBar.cy;
                    
                    y += swpToolBar.cy;
                    h -= swpToolBar.cy;
                    if (prcl)
                        prcl->yBottom += swpToolBar.cy;
                }
                sCount++;
            }
            
            ctl = 0;
            v = peer->Frame->Top;

            H1 = (h - cyScrollBar * cnt) / cnt;
            H1 /= v->Peer->pmData->cyChar;
            H1 *= v->Peer->pmData->cyChar;

            yPos = 0;

            while (v) {
                v = v->Prev;
                if (ctl == cnt - 1) {
                    Hy = h - yPos - cyScrollBar;
                    Hy /= v->Peer->pmData->cyChar;
                    Hy *= v->Peer->pmData->cyChar;
                } else {
                    Hy = H1;
                }

                pswp[sCount].fl = fl;
                pswp[sCount].hwndInsertBehind = Bhwnd;
                Bhwnd = pswp[sCount].hwnd = v->Peer->hwndView;
                pswp[sCount].x = x;
                pswp[sCount].cx = w - cxScrollBar;
                pswp[sCount].y = yPos + y + cyScrollBar;
                pswp[sCount].cy = Hy;

                sCount++;

                pswp[sCount].fl = fl;
                pswp[sCount].hwndInsertBehind = Bhwnd;
                Bhwnd = pswp[sCount].hwnd = v->Peer->hwndHscroll;
                pswp[sCount].x = x;
                pswp[sCount].cx = w - cxScrollBar;
                pswp[sCount].y = yPos + y;
                pswp[sCount].cy = cyScrollBar;

                yPos += cyScrollBar;

                sCount++;

                pswp[sCount].fl = fl;
                pswp[sCount].hwndInsertBehind = Bhwnd;
                Bhwnd = pswp[sCount].hwnd = v->Peer->hwndVscroll;
                pswp[sCount].x = x + w - cxScrollBar;
                pswp[sCount].cx = cxScrollBar;
                pswp[sCount].y = yPos - cyScrollBar + y;
                pswp[sCount].cy = Hy + cyScrollBar;

                yPos += Hy;

                sCount++;

                ctl++;
                if (v == peer->Frame->Top) break;
            }
            DosReleaseMutexSem(hmtxPMData);
            return MRFROMSHORT(sCount);
        }
    }
    return peer->oldFrameProc(hwnd, msg, mp1, mp2);
}

/*MRESULT EXPENTRY SizerWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2) {
    switch (msg) {
    case WM_CREATE:
        break;
    case WM_DESTROY:
        break;
        
    case WM_BUTTON1DOWN:
    case WM_BUTTON1UP:
    case WM_MOUSEMOVE:
        break;

    case WM_PAINT:
        {
            SWP    swp;
            HPS    hps;
            RECTL  rc;
            POINTL pt;


            WinQueryWindowPos(hwnd, &swp);
            hps = WinBeginPaint(hwnd, (HPS)NULL, &rc);
            GpiSetColor(hps, CLR_GRAY);
            GpiSetBackColor(hps, CLR_PALEGRAY);
            if (swp.cy > 2 && swp.cx > 2) {
                ptl.x = 1;
                ptl.y = 1;
                GpiMove(hps, &ptl);
                ptl.x += swp.cx - 2;
                ptl.y += swp.cy - 2;
                GpiBox(hps, DRO_FILL, &ptl, 0, 0);
            }
            GpiSetColor(hps, CLR_WHITE);
            ptl.x = 0;
            ptl.y = 0;
            GpiMove(hps, &ptl);
            ptl.y += swp.cy;
            GpiLine(hps, &ptl);
            ptl.x += swp.cx;
            GpiLine(hps, &ptl);
            GpiSetColor(hps, CLR_GRAY);
            ptl.y = 0;
            GpiLine(hps, &ptl);
            ptl.x = 0;
            GpiLine(hps, &ptl);

            WinEndPaint(hps);
        }
        break;
    }
    return WinDefWndProc(hwnd, msg, mp1, mp2);
}
*/
MRESULT EXPENTRY AVIOWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2) {
    PMPTR *ptr = 0;
    PMData *pmData = (PMData *)WinQueryWindowULong(hwnd, QWL_USER);
    BYTE bBlank[2] = " ";
    SHORT cxClient, cyClient;
    HDC    hdc;
    SIZEL  sizl;
    PDRAGINFO pDragInfo;
    PDRAGITEM pDragItem;
    
    switch (msg) {
    case WM_CREATE:
        ptr = (PMPTR *)mp1;
        pmData = ptr ? (PMData *)ptr->p : 0;
        assert(pmData != 0);
        assert(WinSetWindowULong(hwnd, QWL_USER, (ULONG)pmData) == TRUE);
        
        hdc = WinOpenWindowDC(hwnd);
        VioCreatePS(&pmData->hvps, MAXYSIZE, MAXXSIZE, 0, 1, 0);
        sizl.cx = sizl.cy = 0;
        pmData->hps = GpiCreatePS(hab, hdc, &sizl,
                          PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC);
        VioAssociate(hdc, pmData->hvps);
        if (WindowFont[0] != 0) {
            int x, y;

            if (sscanf(WindowFont, "%dx%d", &x, &y) == 2)
                VioSetDeviceCellSize((SHORT)y, (SHORT)x, pmData->hvps);
        }
        VioGetDeviceCellSize(&pmData->cyChar, &pmData->cxChar, pmData->hvps);
        bBlank[1] = hcPlain_Background;
        VioScrollUp(0, 0, -1, -1, -1, bBlank, pmData->hvps);
        return 0;

    case WM_DESTROY:
        VioAssociate(NULLHANDLE, pmData->hvps);
        VioDestroyPS(pmData->hvps);
        GpiDestroyPS(pmData->hps);
        pmData->Peer->pmData = 0;
        return 0;
    
    case UWM_DESTROY:
        {
            GViewPeer *Peer = pmData->Peer;
            WinDestroyWindow(Peer->hwndVscroll);
            WinDestroyWindow(Peer->hwndHscroll);
            WinDestroyWindow(Peer->hwndView);
        }
        return 0;
        
    case WM_ERASEBACKGROUND:
        return (MRESULT) FALSE;
        
    case WM_CLOSE:
        if (WinPostMsg(pmData->hwndWorker, msg, mp1, mp2) == FALSE) {
            WinMessageBox(HWND_DESKTOP, HWND_DESKTOP,
                          "WinPostMsg failed, WM_CLOSE", "FTE/PM",
                          0, MB_OK | MB_ERROR | MB_APPLMODAL | MB_MOVEABLE);
        }
        return (MRESULT) FALSE;

    case WM_SIZE:
        DosRequestMutexSem(hmtxPMData, SEM_INDEFINITE_WAIT);
        {
            GViewPeer *Peer = pmData->Peer;

            cxClient = SHORT1FROMMP(mp2);
            cyClient = SHORT2FROMMP(mp2);
            WinDefAVioWindowProc(hwnd, (USHORT)msg, (ULONG)mp1, (ULONG)mp2);

            if (cxClient <= pmData->cxChar || cyClient <= pmData->cyChar || reportSize == 0) {
                DosReleaseMutexSem(hmtxPMData);
                break;
            }
            
            Peer->wW = cxClient / pmData->cxChar;
            Peer->wH = cyClient / pmData->cyChar;
            if (hwnd == WinQueryFocus(HWND_DESKTOP)) {
                WinDestroyCursor(hwnd);
                WinCreateCursor(hwnd, 
                                pmData->cxChar * Peer->cX,
                                pmData->cyChar * (Peer->wH - Peer->cY - 1) +
                                pmData->cyChar * (100 - Peer->cEnd) / 100,
                                pmData->cxChar, pmData->cyChar * (Peer->cEnd - Peer->cStart) / 100,
                                CURSOR_TYPE,
                                NULL);
                WinShowCursor(hwnd, TRUE);
            }
            //DosBeep(500, 100);
            if (WinPostMsg(pmData->hwndWorker, msg, mp1, mp2) == FALSE) {
                WinMessageBox(HWND_DESKTOP, HWND_DESKTOP,
                              "WinPostMsg failed, WM_SIZE", "FTE/PM",
                              0, MB_OK | MB_ERROR | MB_APPLMODAL | MB_MOVEABLE);
            }
        }
        DosReleaseMutexSem(hmtxPMData);
        break;

    case WM_ACTIVATE:
    case WM_SETSELECTION:
        if (WinPostMsg(pmData->hwndWorker, msg, mp1, mp2) == FALSE) {
            WinMessageBox(HWND_DESKTOP, HWND_DESKTOP,
                          "WinPostMsg failed, WM_SETFOCUS", "FTE/PM",
                          0, MB_OK | MB_ERROR | MB_APPLMODAL | MB_MOVEABLE);
        }
        break;
    
    case WM_SETFOCUS:
        DosRequestMutexSem(hmtxPMData, SEM_INDEFINITE_WAIT);
        {
            GViewPeer *Peer = pmData->Peer;
            
            if (SHORT1FROMMP(mp2)) {
                WinCreateCursor(hwnd,
                                pmData->cxChar * Peer->cX,
                                pmData->cyChar * (Peer->wH - Peer->cY - 1) +
                                pmData->cyChar * (100 - Peer->cEnd) / 100,
                                pmData->cxChar, pmData->cyChar * (Peer->cEnd - Peer->cStart) / 100,
                                CURSOR_TYPE,
                                NULL);
                WinShowCursor(hwnd, TRUE);
                Peer->wState |= sfFocus;
            } else {
                WinDestroyCursor(hwnd);
                Peer->wState &= ~sfFocus;
            }
        }
        DosReleaseMutexSem(hmtxPMData);
        if (WinPostMsg(pmData->hwndWorker, msg, mp1, mp2) == FALSE) {
            WinMessageBox(HWND_DESKTOP, HWND_DESKTOP,
                          "WinPostMsg failed, WM_SETFOCUS", "FTE/PM",
                          0, MB_OK | MB_ERROR | MB_APPLMODAL | MB_MOVEABLE);
        }
        break;
        
    case WM_VSCROLL:
        if (WinPostMsg(pmData->hwndWorker, msg, mp1, mp2) == FALSE) {
            WinMessageBox(HWND_DESKTOP, HWND_DESKTOP,
                          "WinPostMsg failed, WM_VSCROLL", "ftePM",
                          0, MB_OK | MB_ERROR | MB_APPLMODAL | MB_MOVEABLE);
        }
        return 0;
        
    case WM_HSCROLL:
        if (WinPostMsg(pmData->hwndWorker, msg, mp1, mp2) == FALSE) {
            WinMessageBox(HWND_DESKTOP, HWND_DESKTOP,
                          "WinPostMsg failed, WM_HSCROLL", "ftePM",
                          0, MB_OK | MB_ERROR | MB_APPLMODAL | MB_MOVEABLE);
        }
        return 0;
        
    case WM_COMMAND:
        if (SHORT1FROMMP(mp1) >= 8192) {
            if (WinPostMsg(pmData->hwndWorker, msg, mp1, mp2) == FALSE) {
                WinMessageBox(HWND_DESKTOP, HWND_DESKTOP,
                              "WinPostMsg failed, WM_COMMAND", "ftePM",
                              0, MB_OK | MB_ERROR | MB_APPLMODAL | MB_MOVEABLE);
            }
        }
        break;

    case WM_CHAR:
        if (WinPostMsg(pmData->hwndWorker, msg, mp1, mp2) == FALSE) {
            WinMessageBox(HWND_DESKTOP, HWND_DESKTOP,
                          "WinPostMsg failed, WM_CHAR", "ftePM",
                          0, MB_OK | MB_ERROR | MB_APPLMODAL | MB_MOVEABLE);
        }
        return (MRESULT) TRUE;

    case WM_MOUSEMOVE:
        {
            long b;
            b = 0;
            if (WinGetKeyState(HWND_DESKTOP, VK_BUTTON1) & 0x8000)
                b |= 1;
            if (WinGetKeyState(HWND_DESKTOP, VK_BUTTON2) & 0x8000)
                b |= 2;
            if (WinGetKeyState(HWND_DESKTOP, VK_BUTTON3) & 0x8000)
                b |= 4;
            if (b == 0)
                break;
        }
    case WM_BUTTON1DOWN:
    case WM_BUTTON1DBLCLK:
    case WM_BUTTON1UP:
    case WM_BUTTON2DOWN:
    case WM_BUTTON2DBLCLK:
    case WM_BUTTON2UP:
    case WM_BUTTON3DOWN:
    case WM_BUTTON3DBLCLK:
    case WM_BUTTON3UP:
        if (WinPostMsg(pmData->hwndWorker, msg, mp1, mp2) == FALSE) {
            WinMessageBox(HWND_DESKTOP, HWND_DESKTOP,
                          "WinPostMsg failed, WM_MOUSE", "ftePM",
                          0, MB_OK | MB_ERROR | MB_APPLMODAL | MB_MOVEABLE);
        }
        break;

    case WM_PAINT:
        DosRequestMutexSem(hmtxPMData, SEM_INDEFINITE_WAIT);
        {
            GViewPeer *Peer = pmData->Peer;
            
            WinBeginPaint(hwnd, pmData->hps, NULL);
            if (Peer->wH > 0 && Peer->wH <= MAXYSIZE &&
                Peer->wW > 0 && Peer->wW <= MAXXSIZE)
                VioShowBuf(0, (SHORT)(MAXXSIZE * Peer->wH * 2), pmData->hvps);
            WinEndPaint(pmData->hps);
        }
        DosReleaseMutexSem(hmtxPMData);
        return 0;

    case DM_DRAGOVER:
        {
            char buf[1024] = "";
            pDragInfo = (PDRAGINFO) mp1;
            DrgAccessDraginfo( pDragInfo );
            pDragItem = DrgQueryDragitemPtr( pDragInfo, 0 );

            // Don't accept multi select and non-file
            DrgQueryNativeRMF(pDragItem, sizeof(buf), buf);

            if (pDragInfo->cditem > 1 || strstr(buf, "DRM_OS2FILE") == 0)
                return (MRFROM2SHORT((DOR_NEVERDROP), (DO_UNKNOWN)));
            else
                return (MRFROM2SHORT((DOR_DROP), (DO_UNKNOWN)));
        }
        
    case DM_DROP:
        {
            pDragInfo = (PDRAGINFO)mp1;
            DrgAccessDraginfo(pDragInfo);
            pDragItem = DrgQueryDragitemPtr(pDragInfo, 0);

            DrgQueryStrName(pDragItem->hstrContainerName,100,dragname);
            DrgQueryStrName(pDragItem->hstrSourceName,100,dragname+strlen(dragname));
            WinPostMsg(pmData->hwndWorker, UWM_DROPPEDFILE, 0, 0);
        }
        break;
    }
    return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

MRESULT EXPENTRY ObjectWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2) {
    switch (msg) {
    case WM_CREATE:
        PMPTR *ptr = (PMPTR *)mp1;
        GViewPeer *peer = ptr ? (GViewPeer *)ptr->p : 0;
        assert(WinSetWindowULong(hwnd, QWL_USER, (ULONG)peer) == TRUE);
        break;
    }
    return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

MRESULT EXPENTRY CreatorWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2) {
    switch (msg) {
    case UWM_CREATEWORKER:
        {
            GViewPeer *peer = (GViewPeer *)mp1;
            PMData *pmData = (PMData *)mp2;
            PMPTR ptr;

            ptr.len = sizeof(ptr);
            ptr.p = peer;

            //DosBeep(2500, 1000);
            
            peer->hwndWorker = pmData->hwndWorker =
                WinCreateWindow(HWND_OBJECT,
                                szObject,
                                "Worker",
                                0,
                                0, 0, 0, 0,
                                HWND_OBJECT, HWND_TOP,
                                0, (void *)&ptr, NULL);

            assert(peer->hwndWorker != NULLHANDLE);
            
            return (MRESULT)TRUE;
        }
        
    case UWM_CREATEFRAME:
        {
            GFramePeer *peer = (GFramePeer *)PVOIDFROMMP(mp1);
            FRAMECDATA fcdata;

            fcdata.cb = sizeof(FRAMECDATA);
            fcdata.flCreateFlags = flFrame;
            fcdata.hmodResources = 0;
            fcdata.idResources   = 1;
            
            //DosBeep(2500, 1000);
            
            peer->hwndFrame = WinCreateWindow(HWND_DESKTOP,
                                              WC_FRAME, NULL, 0, 0, 0, 0, 0,
                                              hwndCreatorUser, HWND_TOP, 1,
                                              &fcdata, NULL);

            assert(peer->hwndFrame != NULLHANDLE);

            WinSetWindowULong(peer->hwndFrame, QWL_USER, (ULONG)peer);
            
            peer->oldFrameProc = WinSubclassWindow(peer->hwndFrame, (PFNWP) FrameWndProc);
            
            if (ShowToolBar) {
                peer->hwndToolBar = CreateToolBar(peer->hwndFrame, peer->hwndFrame, FID_MTOOLBAR);
                assert(peer->hwndToolBar != NULLHANDLE);
            }

            
            return (MRESULT)TRUE;
        }

    case UWM_CREATEMAINMENU:
        {
            GFramePeer *peer = (GFramePeer *)mp1;
            char *Menu = (char *)PVOIDFROMMP(mp2);
            HWND hwnd, hwndMenu;
            char font[256];
            ULONG AttrFound = 0;
            LONG len = -1;
            
            hwnd = WinWindowFromID(peer->hwndFrame, FID_MENU);
            if (hwnd != NULLHANDLE) {
                if (len == -1) {
                    AttrFound = 0;
                    len = WinQueryPresParam(hwnd, PP_FONTNAMESIZE, PP_FONTHANDLE, &AttrFound, sizeof(font), font, 0);
                }
                WinDestroyWindow(hwnd);
            }
            
            hwndMenu = CreatePMMainMenu(peer->hwndFrame, peer->hwndFrame, Menu);
            
            if (len > 0)
                WinSetPresParam(hwndMenu, AttrFound, len, font);
            WinSendMsg(peer->hwndFrame, WM_UPDATEFRAME, MPFROMLONG(FCF_MENU), 0);

            return (MRESULT)hwndMenu;
        }

    case UWM_CREATEPOPUPMENU:
        {
            GFramePeer *peer = (GFramePeer *)mp1;
            int MenuId = LONGFROMMP(mp2);
            
            static HWND hwnd = 0;
            POINTL ptl;
            char font[256];
            ULONG AttrFound = 0;
            ULONG len;
            
            if (hwnd != 0) {
                WinDestroyWindow(hwnd);
                hwnd = 0;
            }
            
            hwnd = CreatePMMenu(HWND_DESKTOP, peer->hwndFrame, MenuId, 0, 0);
            
            WinQueryMsgPos(hab, &ptl);
            
            len = WinQueryPresParam(peer->menuBar, PP_FONTNAMESIZE, PP_FONTHANDLE, &AttrFound, sizeof(font), font, 0);
            if (len > 0)
                WinSetPresParam(hwnd, AttrFound, len, font);
            
            WinPopupMenu(HWND_DESKTOP, peer->Frame->Active->Peer->hwndView, hwnd,
                         ptl.x, ptl.y,
                         0,
                         PU_HCONSTRAIN  | PU_VCONSTRAIN |
                         PU_NONE | PU_KEYBOARD |
                         PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 | PU_MOUSEBUTTON3);

            return (MRESULT)TRUE;
        }
    }
    return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

int ConGetEvent(TEventMask EventMask, TEvent *Event, int WaitTime, int Delete, GView **view) {
    QMSG qmsg;
    GFrame *f;
    GView *v;
    
    if (view) 
        *view = 0;
    
    if (EventBuf.What != evNone) {
        *Event = EventBuf;
        if (Delete) EventBuf.What = evNone;
        return 0;
    }
    EventBuf.What = evNone;
    Event->What = evNone;
    
    //DosBeep(800, 10);

    if (WaitTime != -1) {
        if (WinPeekMsg(hab, &qmsg, NULLHANDLE, 0, 0, PM_NOREMOVE) == FALSE)
            return 0;
    }
    if (WinGetMsg(hab, &qmsg, NULLHANDLE, 0, 0) == FALSE)
        return -1;
    
    //DosBeep(800, 10);
    
    f = frames;
    while (f) {
        v = f->Top;
        while (frames && v) {
            if (v->Peer->hwndWorker == qmsg.hwnd) {
                if (FocusCapture && v != FocusCapture) {
                    WinSetFocus(HWND_DESKTOP, FocusCapture->Peer->hwndView);
                    frames = f;
                    break;
                }
                if (view)
                    *view = v;
                switch (qmsg.msg) {
                /*case WM_ACTIVATE:
                case WM_SETSELECTION:
                    if (SHORT1FROMMP(qmsg.mp1) == TRUE) {
                        if (!v->IsActive())
                            v->Parent->SelectView(v);
                        DosBeep(800, 10);
                        return 0;
                    }
                    break;*/
                case WM_SETFOCUS:
                    if (SHORT1FROMMP(qmsg.mp2) == TRUE) {
                        if (!v->IsActive())
                            v->Parent->SelectView(v);
                        //DosBeep(800, 10);
                        return 0;
                    }
                    break;
                    
                case UWM_NOTIFY:
                    //DosBeep(200, 200);
                    Event->What = evNotify;
                    Event->Msg.View = v;
                    Event->Msg.Model = (EModel *)qmsg.mp1;
                    Event->Msg.Command = cmPipeRead;
                    Event->Msg.Param1 = (long)qmsg.mp2;
                    frames = f;
                    return 0;

                case UWM_DROPPEDFILE:
                    if (!v->IsActive())
                        v->Parent->SelectView(v);
                    Event->What = evCommand;
                    Event->Msg.View = v;
                    Event->Msg.Command = cmDroppedFile;
                    Event->Msg.Param1 = 0;
                    Event->Msg.Param2 = &dragname;
                    frames = f;
                    return 0;
                    
                case WM_SIZE:
                    frames = f;
                    //DosBeep(500, 500);
                    v->Resize(v->Peer->wW, v->Peer->wH);
                    return 0;
                    
                case WM_VSCROLL:
                    //DosBeep(200, 2000);
                    if (!v->IsActive())
                        v->Parent->SelectView(v);
                    Event->What = evNone;
                    Event->Msg.View = v;
                    Event->Msg.Param1 = 0;
                    Event->Msg.Param2 = 0;
                    switch (SHORT2FROMMP(qmsg.mp2)) {
                    case SB_LINEUP:
                        Event->What = evCommand;
                        Event->Msg.Command = cmVScrollUp;
                        Event->Msg.Param1 = 1;
                        break;
                        
                    case SB_LINEDOWN:
                        Event->What = evCommand;
                        Event->Msg.Command = cmVScrollDown;
                        Event->Msg.Param1 = 1;
                        break;
                        
                    case SB_PAGEUP:
                        Event->What = evCommand;
                        Event->Msg.Command = cmVScrollPgUp;
                        break;
                        
                    case SB_PAGEDOWN:
                        Event->What = evCommand;
                        Event->Msg.Command = cmVScrollPgDn;
                        break;
                        
                    case SB_ENDSCROLL:
                        WinSetFocus(HWND_DESKTOP, v->Parent->Active->Peer->hwndView);
                        /* no break */
                    case SB_SLIDERPOSITION:
                    case SB_SLIDERTRACK:
                        {
                            SHORT x = SHORT1FROMMP(qmsg.mp2);
                            
                            if (x != 0) {
                                Event->What = evCommand;
                                Event->Msg.Command = cmVScrollMove;
                                if (v->Peer->sbVtotal > 32000)
                                    Event->Msg.Param1 = (x - 1) * v->Peer->sbVtotal / 32000;
                                else
                                    Event->Msg.Param1 = x - 1;
                            }
                        }
                        break;
                    }
                    return 0;
                    
                case WM_HSCROLL:
                    //DosBeep(800, 2000);
                    if (!v->IsActive())
                        v->Parent->SelectView(v);
                    Event->What = evNone;
                    Event->Msg.View = v;
                    Event->Msg.Param1 = 0;
                    Event->Msg.Param2 = 0;
                    switch (SHORT2FROMMP(qmsg.mp2)) {
                    case SB_LINEUP:
                        Event->What = evCommand;
                        Event->Msg.Command = cmHScrollLeft;
                        Event->Msg.Param1 = 1;
                        break;
                        
                    case SB_LINEDOWN:
                        Event->What = evCommand;
                        Event->Msg.Command = cmHScrollRight;
                        Event->Msg.Param1 = 1;
                        break;
                        
                    case SB_PAGEUP:
                        Event->What = evCommand;
                        Event->Msg.Command = cmHScrollPgLt;
                        break;
                        
                    case SB_PAGEDOWN:
                        Event->What = evCommand;
                        Event->Msg.Command = cmHScrollPgRt;
                        break;
                        
                    case SB_ENDSCROLL:
                        WinSetFocus(HWND_DESKTOP, v->Parent->Active->Peer->hwndView);
                        /* no break */
                    case SB_SLIDERPOSITION:
                    case SB_SLIDERTRACK:
                        {
                            SHORT x = SHORT1FROMMP(qmsg.mp2);
                            
                            if (x != 0) {
                                Event->What = evCommand;
                                Event->Msg.Command = cmHScrollMove;
                                if (v->Peer->sbHtotal > 32000)
                                    Event->Msg.Param1 = (x - 1) * v->Peer->sbHtotal / 32000;
                                else
                                    Event->Msg.Param1 = x - 1;
                            }
                        }
                        break;
                    }
                    return 0;
                
                case WM_CLOSE:
                    //DosBeep(500, 1500);
                    frames = f;
                    Event->What = evCommand;
                    Event->Msg.View = v->Parent->Active;
                    Event->Msg.Command = cmClose;
                    return 0;
                    
                case WM_COMMAND:
                    //DosBeep(50, 2500);
                    if (SHORT1FROMMP(qmsg.mp1) >= 8192) {
                        Event->What = evCommand;
                        Event->Msg.View = v->Parent->Active;
                        Event->Msg.Command = (SHORT1FROMMP(qmsg.mp1) - 8192) + 65536;
                        frames = f;
                        return 0;
                    }
                    break;
                    
                case WM_CHAR:
                    //DosBeep(50, 500);
                    Event->What = evNone;
                    Event->Msg.View = v;
                    ConvertKey(SHORT1FROMMP(qmsg.mp2), /* char */
                               SHORT2FROMMP(qmsg.mp2), /* virtual */
                               SHORT1FROMMP(qmsg.mp1), /* flags */
                               CHAR4FROMMP(qmsg.mp1), /* scan */
                               *Event);
                    frames = f;
                    return 0;
                
                case WM_BUTTON1DOWN:
                case WM_BUTTON2DOWN:
                case WM_BUTTON3DOWN:
                case WM_BUTTON1DBLCLK:
                case WM_BUTTON2DBLCLK:
                case WM_BUTTON3DBLCLK:
                    if (!v->IsActive()) 
                        v->Parent->SelectView(v);
                case WM_BUTTON1UP:
                case WM_BUTTON2UP:
                case WM_BUTTON3UP:
                case WM_MOUSEMOVE:
                    Event->Mouse.What = evNone;
                    Event->Msg.View = v;
                    Event->Mouse.X = ((SHORT)SHORT1FROMMP(qmsg.mp1)) / v->Peer->pmData->cxChar;
                    Event->Mouse.Y = v->Peer->wH - ((SHORT)SHORT2FROMMP(qmsg.mp1)) / v->Peer->pmData->cyChar - 1;
                    Event->Mouse.Buttons = 1;
                    Event->Mouse.Count = 1;
                    Event->Mouse.KeyMask = 0;
                    if (WinGetKeyState(HWND_DESKTOP, VK_CTRL) & 0x8000)
                        Event->Mouse.KeyMask |= kfCtrl;
                    if (WinGetKeyState(HWND_DESKTOP, VK_ALT) & 0x8000)
                        Event->Mouse.KeyMask |= kfAlt;
                    if (WinGetKeyState(HWND_DESKTOP, VK_SHIFT) & 0x8000)
                        Event->Mouse.KeyMask |= kfShift;
//                    printf("KeyFlags: %d\n", Event->Mouse.KeyMask);
                    //DosBeep(2000, 50);
                    frames = f;
                
                    switch (qmsg.msg) {
                    case WM_BUTTON1DOWN:
                        Event->What = evMouseDown;
                        v->Peer->OldMouseX = Event->Mouse.X;
                        v->Peer->OldMouseY = Event->Mouse.Y;
                        return 0;
                    
                    case WM_BUTTON1DBLCLK:
                        Event->What = evMouseDown;
                        Event->Mouse.Count = 2;
                        v->Peer->OldMouseX = Event->Mouse.X;
                        v->Peer->OldMouseY = Event->Mouse.Y;
                        return 0;
                    
                    case WM_BUTTON1UP:
                        Event->What = evMouseUp;
                        v->Peer->OldMouseX = Event->Mouse.X;
                        v->Peer->OldMouseY = Event->Mouse.Y;
                        return 0;
                
                    case WM_BUTTON2DOWN:
                        Event->What = evMouseDown;
                        Event->Mouse.Buttons = 2;
                        v->Peer->OldMouseX = Event->Mouse.X;
                        v->Peer->OldMouseY = Event->Mouse.Y;
                        return 0;
                    
                    case WM_BUTTON2DBLCLK:
                        Event->What = evMouseDown;
                        Event->Mouse.Buttons = 2;
                        Event->Mouse.Count = 2;
                        v->Peer->OldMouseX = Event->Mouse.X;
                        v->Peer->OldMouseY = Event->Mouse.Y;
                        return 0;
                
                    case WM_BUTTON2UP:
                        Event->What = evMouseUp;
                        Event->Mouse.Buttons = 2;
                        v->Peer->OldMouseX = Event->Mouse.X;
                        v->Peer->OldMouseY = Event->Mouse.Y;
                        return 0;
                
                    case WM_BUTTON3DOWN:
                        Event->What = evMouseDown;
                        Event->Mouse.Buttons = 4;
                        v->Peer->OldMouseX = Event->Mouse.X;
                        v->Peer->OldMouseY = Event->Mouse.Y;
                        return 0;
                
                    case WM_BUTTON3DBLCLK:
                        Event->What = evMouseDown;
                        Event->Mouse.Buttons = 4;
                        Event->Mouse.Count = 2;
                        v->Peer->OldMouseX = Event->Mouse.X;
                        v->Peer->OldMouseY = Event->Mouse.Y;
                        return 0;
                
                    case WM_BUTTON3UP:
                        Event->What = evMouseUp;
                        Event->Mouse.Buttons = 4;
                        v->Peer->OldMouseX = Event->Mouse.X;
                        v->Peer->OldMouseY = Event->Mouse.Y;
                        return 0;
                
                    case WM_MOUSEMOVE:
                        Event->What = evMouseMove;
                        Event->Mouse.Buttons = 0;
                        if (WinGetKeyState(HWND_DESKTOP, VK_BUTTON1) & 0x8000)
                            Event->Mouse.Buttons |= 1;
                        if (WinGetKeyState(HWND_DESKTOP, VK_BUTTON2) & 0x8000)
                            Event->Mouse.Buttons |= 2;
                        if (WinGetKeyState(HWND_DESKTOP, VK_BUTTON3) & 0x8000)
                            Event->Mouse.Buttons |= 4;
                        
                        if (Event->Mouse.Buttons != 0) {
                            if (Event->Mouse.X != v->Peer->OldMouseX ||
                                Event->Mouse.Y != v->Peer->OldMouseY) 
                            {
                                v->Peer->OldMouseX = Event->Mouse.X;
                                v->Peer->OldMouseY = Event->Mouse.Y;
                                return 0;
                            } else  if (Event->Mouse.X >= 0 &&
                                        Event->Mouse.Y >= 0 &&
                                        Event->Mouse.X < v->Peer->wW &&
                                        Event->Mouse.Y < v->Peer->wH)
                            {
                                Event->What = evNone;
                            }
                        }
                    }
                    break;

                default:
                    WinDispatchMsg(hab, &qmsg);
                    return 0;
                }
            }
            v = v->Next;
            if (v == f->Top) break;
        }
        f = f->Next;
        if (f == frames) break;
    }
    WinDispatchMsg(hab, &qmsg);
    return 0;
}

static void _LNK_CONV WorkThread(void *) {
    habW = WinInitialize(0);
    hmqW = WinCreateMsgQueue(hab, 0);
    
    hwndCreatorWorker = WinCreateWindow(HWND_OBJECT,
                                        szCreator,
                                        "Creator",
                                        0,
                                        0, 0, 0, 0,
                                        HWND_OBJECT, HWND_TOP,
                                        0, NULL, NULL);

    assert(hwndCreatorWorker != 0);
    
    // work thread started
    DosPostEventSem(WorkerStarted);

    ULONG ulPostCount;
    DosWaitEventSem(StartInterface, SEM_INDEFINITE_WAIT);
    DosResetEventSem(StartInterface, &ulPostCount);
        
    //DosBeep(200, 200);
    if (gui->Start(gui->fArgc, gui->fArgv) == 0) {
        gui->doLoop = 1;
        //DosBeep(500, 500);
        while (gui->doLoop)
            gui->ProcessEvent();
        
        gui->Stop();
    }
    
    WinDestroyMsgQueue(hmqW);
    WinTerminate(habW);
    //DosBeep(500, 500);
    WinPostQueueMsg(hmq, WM_QUIT, 0, 0);
    _endthread();
}

///////////////////////////////////////////////////////////////////////////

GViewPeer::GViewPeer(GView *view, int XSize, int YSize) {
    HWND parent;
    
    View = view;
//    wX = 0;
//    wY = 0;
    wW = XSize;
    wH = YSize;
    sbVtotal = 0;
    sbVstart = 0;
    sbVamount = 0;
    sbHtotal = 0;
    sbHstart = 0;
    sbHamount = 0;
    wState = 0;
    cVisible = 1;
    cStart = 0; // %
    cEnd = 100;
    OldMouseX = OldMouseY = 0xFFFF;
    
    pmData = (PMData *)malloc(sizeof(PMData));
    
    pmData->Peer = this;
    pmData->hvps = 0;
    pmData->cxChar = 8;
    pmData->cyChar = 14;
    pmData->hwndWorker = 0;
        
    parent = View->Parent->Peer->hwndFrame;
    
    WinSendMsg(hwndCreatorWorker, UWM_CREATEWORKER, MPFROMP(this), MPFROMP(pmData));
    
    WinSendMsg(parent, UWM_CREATECHILD, MPFROMP(this), MPFROMP(pmData));
}

GViewPeer::~GViewPeer() {
    WinSendMsg(hwndView, UWM_DESTROY, 0, 0);
    WinDestroyWindow(hwndWorker);
    free(pmData);
}

int GViewPeer::ConPutBox(int X, int Y, int W, int H, PCell Cell) {
    int I;
    char *p = (char *) Cell;
    int Hidden = 0;

    DosRequestMutexSem(hmtxPMData, SEM_INDEFINITE_WAIT);
    if (Y < 0 || Y >= wH || X < 0 || X >= wW ||
        Y + H < 0 || Y + H > wH || X + W < 0 || X + W > wW)
    {
        //fprintf(stderr,
        //       "X:%d, Y:%d, W:%d, H:%d, wW:%d, wH:%d\n",
        //        X, Y, W, H, wW, wH);
        DosReleaseMutexSem(hmtxPMData);
        return -1;
    }
    for (I = 0; I < H; I++) {
        if (I + Y == cY)
            Hidden = PMHideCursor();
        VioWrtCellStr(p, (USHORT)(W << 1), (USHORT)(Y + I), (USHORT)X, pmData->hvps);
        if (Hidden)
            PMShowCursor();
        p += W << 1;
    }
    DosReleaseMutexSem(hmtxPMData);
    return 0;
}

int GViewPeer::ConGetBox(int X, int Y, int W, int H, PCell Cell) {
    int I;
    USHORT WW = (USHORT)(W << 1);
    char *p = (char *) Cell;
    
    DosRequestMutexSem(hmtxPMData, SEM_INDEFINITE_WAIT);
    if (Y < 0 || Y >= wH || X < 0 || X >= wW ||
        Y + H < 0 || Y + H > wH || X + W < 0 || X + W > wW)
    {
        //fprintf(stderr,
        //       "X:%d, Y:%d, W:%d, H:%d, wW:%d, wH:%d\n",
        //       X, Y, W, H, wW, wH);
        DosReleaseMutexSem(hmtxPMData);
        return -1;
    }
    for (I = 0; I < H; I++) {
        VioReadCellStr((char *)p, &WW, (USHORT)(Y + I), (USHORT)X, pmData->hvps);
        p += W << 1;
    }
    DosReleaseMutexSem(hmtxPMData);
    return 0;
}

int GViewPeer::ConPutLine(int X, int Y, int W, int H, PCell Cell) {
    int I;
    char *p = (char *) Cell;
    int Hidden = 0;
    
    DosRequestMutexSem(hmtxPMData, SEM_INDEFINITE_WAIT);
    if (Y < 0 || Y >= wH || X < 0 || X >= wW ||
        Y + H < 0 || Y + H > wH || X + W < 0 || X + W > wW)
    {
        //fprintf(stderr,
        //        "X:%d, Y:%d, W:%d, H:%d, wW:%d, wH:%d\n",
        //        X, Y, W, H, wW, wH);
        DosReleaseMutexSem(hmtxPMData);
        return -1;
    }
    for (I = 0; I < H; I++) {
        if (I + Y == cY)
            Hidden = PMHideCursor();
        VioWrtCellStr(p, (USHORT)(W << 1), (USHORT)(Y + I), (USHORT)X, pmData->hvps);
        if (Hidden)
            PMShowCursor();
    }
    DosReleaseMutexSem(hmtxPMData);
    return 0;
}

int GViewPeer::ConSetBox(int X, int Y, int W, int H, TCell Cell) {
    int I;
    char *p = (char *) &Cell;
    int Hidden = 0;
    
    DosRequestMutexSem(hmtxPMData, SEM_INDEFINITE_WAIT);
    if (Y < 0 || Y >= wH || X < 0 || X >= wW ||
        Y + H < 0 || Y + H > wH || X + W < 0 || X + W > wW)
    {
        //fprintf(stderr,
        //        "X:%d, Y:%d, W:%d, H:%d, wW:%d, wH:%d\n",
        //        X, Y, W, H, wW, wH);
        DosReleaseMutexSem(hmtxPMData);
        return -1;
    }
    for (I = 0; I < H; I++) {
        if (I + Y == cY)
            Hidden = PMHideCursor();
        VioWrtNCell(p, (USHORT)(W), (USHORT)(Y + I), (USHORT)X, pmData->hvps);
        if (Hidden)
            PMShowCursor();
    }
    DosReleaseMutexSem(hmtxPMData);
    return 0;
}

int GViewPeer::ConScroll(int Way, int X, int Y, int W, int H, TAttr Fill, int Count) {
    TCell FillCell = (TCell)(Fill << 8);
    int Hidden = 0;
    
    DosRequestMutexSem(hmtxPMData, SEM_INDEFINITE_WAIT);
    if (Y < 0 || Y >= wH || X < 0 || X >= wW ||
        Y + H < 0 || Y + H > wH || X + W < 0 || X + W > wW)
    {
        //fprintf(stderr,
        //        "X:%d, Y:%d, W:%d, H:%d, wW:%d, wH:%d\n",
        //        X, Y, W, H, wW, wH);
        DosReleaseMutexSem(hmtxPMData);
        return -1;
    }
    Hidden = PMHideCursor();
    switch (Way) {
    case csUp:
        VioScrollUp((USHORT)Y, (USHORT)X, (USHORT)(Y + H - 1), (USHORT)(X + W - 1), (USHORT)Count, (PBYTE)&FillCell, pmData->hvps);
        break;
    case csDown:
        VioScrollDn((USHORT)Y, (USHORT)X, (USHORT)(Y + H - 1), (USHORT)(X + W - 1), (USHORT)Count, (PBYTE)&FillCell, pmData->hvps);
        break;
    case csLeft:
        VioScrollLf((USHORT)Y, (USHORT)X, (USHORT)(Y + H - 1), (USHORT)(X + W - 1), (USHORT)Count, (PBYTE)&FillCell, pmData->hvps);
        break;
    case csRight:
        VioScrollRt((USHORT)Y, (USHORT)X, (USHORT)(Y + H - 1), (USHORT)(X + W - 1), (USHORT)Count, (PBYTE)&FillCell, pmData->hvps);
        break;
    }
    if (Hidden)
        PMShowCursor();
    DosReleaseMutexSem(hmtxPMData);
    return 0;
}

int GViewPeer::ConSetSize(int X, int Y) {
    wW = X;
    wH = Y;
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
    cX = X;
    cY = Y;
    if (wState & sfFocus)
        return PMSetCursorPos();
    else
        return 1;
}

int GViewPeer::ConQueryCursorPos(int *X, int *Y) {
    if (X) *X = cX;
    if (Y) *Y = cY;
    return 1;
}

int GViewPeer::ConShowCursor() {
    cVisible = 1;
    if (wState & sfFocus)
        return PMShowCursor();
    else
        return 1;
}

int GViewPeer::ConHideCursor() {
    cVisible = 0;
    if (wState & sfFocus)
        return PMHideCursor();
    else
        return 1;
}

int GViewPeer::ConCursorVisible() {
    return cVisible;
}

int GViewPeer::ConSetCursorSize(int Start, int End) {
    cStart = Start;
    cEnd = End;
    if (wState & sfFocus)
        return PMSetCursorPos();
    else
        return 1;
}

int GViewPeer::ExpandHeight(int DeltaY) {
    if (View->Parent->Top == View->Next)
        return -1;
    if (DeltaY + wH < 3)
        return -1;
    if (View->Next->Peer->wH - DeltaY < 3)
        return -1;
    ConSetSize(wW, wH + DeltaY);
    //View->Next->Peer->wY += DeltaY;
    View->Next->ConSetSize(View->Next->Peer->wW, View->Next->Peer->wH - DeltaY);
    View->Parent->Peer->SizeFrame();
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
        
        WinEnableWindowUpdate(hwndVscroll, FALSE);
        if (sbVamount < sbVtotal) {
            if (sbVtotal > 32000) {
                int total = 32000;
                int start = total * sbVstart / sbVtotal;
                int amount = total * sbVamount / sbVtotal;

                WinSendMsg(hwndVscroll, SBM_SETTHUMBSIZE,
                           (MPFROM2SHORT((amount), (total))), 0);
                WinSendMsg(hwndVscroll, SBM_SETSCROLLBAR,
                           (MPFROMSHORT((start + 1))),
                           (MPFROM2SHORT((1), (total - amount + 2))));
            } else {
                WinSendMsg(hwndVscroll, SBM_SETTHUMBSIZE,
                           (MPFROM2SHORT((sbVamount), (sbVtotal))), 0);
                WinSendMsg(hwndVscroll, SBM_SETSCROLLBAR,
                           (MPFROMSHORT((sbVstart + 1))),
                           (MPFROM2SHORT((1), (sbVtotal - sbVamount + 2))));
            }
        } else {
            WinSendMsg(hwndVscroll, SBM_SETTHUMBSIZE,
                       MPFROM2SHORT(0, 0), 0);
            WinSendMsg(hwndVscroll, SBM_SETSCROLLBAR,
                       MPFROMSHORT(0),
                       MPFROM2SHORT(0, 0));
        }
        WinEnableWindowUpdate(hwndVscroll, TRUE);
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
        
        WinEnableWindowUpdate(hwndHscroll, FALSE);
        if (sbHtotal > sbHamount) {
            if (sbHtotal > 32000) {
                int total = 32000;
                int start = total * sbVstart / sbVtotal;
                int amount = total * sbVamount / sbVtotal;

                WinSendMsg(hwndHscroll, SBM_SETTHUMBSIZE,
                           (MPFROM2SHORT(amount, total)), 0);
                WinSendMsg(hwndHscroll, SBM_SETSCROLLBAR,
                           (MPFROMSHORT(start + 1)),
                           (MPFROM2SHORT(1, total - amount + 2)));
            } else {
                WinSendMsg(hwndHscroll, SBM_SETTHUMBSIZE,
                           (MPFROM2SHORT(sbHamount, sbHtotal)), 0);
                WinSendMsg(hwndHscroll, SBM_SETSCROLLBAR,
                           (MPFROMSHORT(sbHstart + 1)),
                           (MPFROM2SHORT(1, sbHtotal - sbHamount + 2)));
            }
        } else {
            WinSendMsg(hwndHscroll, SBM_SETTHUMBSIZE,
                       (MPFROM2SHORT(0, 0)), 0);
            WinSendMsg(hwndHscroll, SBM_SETSCROLLBAR,
                       (MPFROMSHORT(0)),
                       (MPFROM2SHORT(0, 0)));
        }
        WinEnableWindowUpdate(hwndHscroll, TRUE);
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
    if (wState & sfFocus)
        WinShowCursor(hwndView, TRUE);
    return 1;
}

int GViewPeer::PMHideCursor() {
    if (wState & sfFocus)
        WinShowCursor(hwndView, FALSE);
    return 1;
}

int GViewPeer::PMSetCursorPos() {
    if (wState & sfFocus) {
        WinDestroyCursor(hwndView);
        WinCreateCursor(hwndView,
                        pmData->cxChar * cX,
                        pmData->cyChar * (wH - cY - 1) +
                        pmData->cyChar * (100 - cEnd) / 100,
                        pmData->cxChar, pmData->cyChar * (cEnd - cStart) / 100,
                        CURSOR_TYPE,
                        NULL);
        WinShowCursor(hwndView, TRUE);
    }
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
    if (FocusCapture == this)
        CaptureFocus(0);
    if (MouseCapture == this)
        CaptureMouse(0);
    if (Parent)
        Parent->RemoveView(this);
    if (Peer) {
        Peer->View = 0;
        delete Peer;
    }
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
    if (Peer->ConSetSize(X, Y))
        Resize(X, Y);
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
    int didFocus = 0;
    
    if (FocusCapture == 0) {
        if (CaptureFocus(1) == 0) return -1;
        didFocus = 1;
    } else
        if (FocusCapture != this)
            return -1;
    Result = -2;
    while (Result == -2 && frames != 0)
        gui->ProcessEvent();
    NewResult = Result;
    Result = SaveRc;
    if (didFocus)
        CaptureFocus(0);
    return NewResult;
}

int GView::IsActive() {
    return (Parent->Active == this && Parent == frames);
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
        if (grab) {
            MouseCapture = this;
            WinSetCapture(HWND_DESKTOP, Peer->hwndView);
        } else
            return 0;
    } else {
        if (grab || MouseCapture != this)
            return 0;
        else {
            MouseCapture = 0;
            WinSetCapture(HWND_DESKTOP, NULLHANDLE);
        }
    }
    return 1;
}

int GView::CaptureFocus(int grab) {
    if (FocusCapture == 0) {
        if (grab) {
            FocusCapture = this;
            WinSetFocus(HWND_DESKTOP, Peer->hwndView);
        } else
            return 0;
    } else {
        if (grab || FocusCapture != this)
            return 0;
        else
            FocusCapture = 0;
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////

GFramePeer::GFramePeer(GFrame *aFrame, int Width, int Height) {
    Frame = aFrame;

    WinSendMsg(hwndCreatorUser, UWM_CREATEFRAME, MPFROMP(this), MPFROMLONG(0));

    if (Width != -1 && Height != -1)
        ConSetSize(Width, Height);
}

GFramePeer::~GFramePeer() {
    WinStoreWindowPos("FTEPM", "Frame1", hwndFrame);
    WinSendMsg(hwndFrame, UWM_DESTROY, 0, 0);
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

int GFramePeer::ConSetTitle(const char *Title, const char *STitle) {
    char szTitle[256] = {0};

    JustFileName(Title, szTitle, sizeof(szTitle));
    if (szTitle[0] == '\0') // if there is no filename, try the directory name.
        JustLastDirectory(Title, szTitle, sizeof(szTitle));

    if (szTitle[0] != '\0') // if there is something...
        strlcat(szTitle, " - ", sizeof(szTitle));
    strlcat(szTitle, Title, sizeof(szTitle));

    WinSetWindowText(hwndFrame, szTitle);
    return 1;
}

int GFramePeer::ConGetTitle(char *Title, size_t MaxLen, char *STitle, size_t SMaxLen) {
    WinQueryWindowText(hwndFrame, MaxLen, Title);
    WinQueryWindowText(hwndFrame, SMaxLen, STitle);
    return 1;
}

void GFramePeer::MapFrame() {
    if (frames != frames->Next || WinRestoreWindowPos("FTEPM", "Frame1", hwndFrame) == FALSE) {
        WinQueryTaskSizePos(hab, 0, &swp);
        
        WinSetWindowPos(hwndFrame, HWND_TOP,
                        swp.x,
                        swp.y,
                        swp.cx,
                        swp.cy,
                        SWP_MOVE | SWP_SIZE);
    }
    //    WinSendMsg(Peer->hwndFrame, WM_UPDATEFRAME, 0, 0);
    SizeFrame();
    ShowFrame();
}

void GFramePeer::ShowFrame() {
    WinSetWindowPos(hwndFrame, HWND_TOP, 0, 0, 0, 0,
                    SWP_ZORDER | SWP_SHOW | SWP_ACTIVATE);
}

void GFramePeer::SizeFrame() {
    POINTL ptl;
    GView *v = Frame->Top;
    SWP swp;
    SWP swpMenu;
    HWND hwndMenu = WinWindowFromID(hwndFrame, FID_MENU);
    SWP swpToolBar;

    if (ShowToolBar) {
        HWND hwndToolBar = WinWindowFromID(hwndFrame, FID_MTOOLBAR);

        WinQueryWindowPos(hwndToolBar, &swpToolBar);
        WinSendMsg(hwndToolBar,
                   WM_ADJUSTWINDOWPOS,
                   MPFROMP(&swpToolBar),
                   MPARAM(0));
    } else {
        swpToolBar.cy = 0;
    }
    
    WinQueryWindowPos(hwndMenu, &swpMenu);
    WinSendMsg(hwndMenu,
               WM_ADJUSTWINDOWPOS,
               MPFROMP(&swpMenu),
               MPARAM(0));

    //WinEnableWindowUpdate(hwndFrame, FALSE);
    DosRequestMutexSem(hmtxPMData, SEM_INDEFINITE_WAIT);
    ptl.x = v->Peer->wW * v->Peer->pmData->cxChar + cxScrollBar + 2 * cxBorder;
    ptl.y = 2 * cyBorder + cyTitleBar + swpMenu.cy + swpToolBar.cy;

    while (v) {
        v = v->Prev;
        ptl.y += v->Peer->wH * v->Peer->pmData->cyChar + cyScrollBar;
        if (v == Frame->Top) break;
    }
    
    reportSize = 0;
    DosReleaseMutexSem(hmtxPMData);
    WinQueryWindowPos(hwndFrame, &swp);
    swp.y = swp.y + swp.cy - ptl.y;
    swp.cx = ptl.x;
    swp.cy = ptl.y;
    WinSendMsg(hwndFrame,
               WM_ADJUSTWINDOWPOS,
               MPFROMP(&swp),
               MPARAM(0));
    WinSetWindowPos(hwndFrame, HWND_TOP, swp.x, swp.y, swp.cx, swp.cy, SWP_SIZE | SWP_MOVE);
    DosRequestMutexSem(hmtxPMData, SEM_INDEFINITE_WAIT);
    reportSize = 1;
    DosReleaseMutexSem(hmtxPMData);
    swp.x = cxBorder;
    swp.y = cyBorder;
    if (ShowToolBar == 2) {
        swp.y += swpToolBar.cy;
    }
    v = Frame->Top;
    while (v) {
        v = v->Prev;
        swp.cx = v->Peer->wW * v->Peer->pmData->cxChar;
        swp.cy = cyScrollBar;
        WinSetWindowPos(v->Peer->hwndHscroll, 0,
                        swp.x, swp.y, swp.cx, swp.cy, SWP_SIZE | SWP_MOVE);
        swp.y += cyScrollBar;
        swp.cy = v->Peer->wH * v->Peer->pmData->cyChar;
        WinSetWindowPos(v->Peer->hwndView, 0,
                        swp.x, swp.y, swp.cx, swp.cy, SWP_SIZE | SWP_MOVE);
        swp.x = v->Peer->wW * v->Peer->pmData->cxChar + cxBorder;
        swp.y -= cyScrollBar;
        swp.cx = cxScrollBar;
        swp.cy = cyScrollBar + v->Peer->wH * v->Peer->pmData->cyChar;
        WinSetWindowPos(v->Peer->hwndVscroll, 0,
                        swp.x, swp.y, swp.cx, swp.cy, SWP_SIZE | SWP_MOVE);
        swp.y += cyScrollBar + v->Peer->wH * v->Peer->pmData->cyChar;
        swp.x = cxBorder;
        if (v == Frame->Top)
            break;
    }
    //WinEnableWindowUpdate(hwndFrame, TRUE);
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
    GView *P = Top, *Q;
    if (P) do {
        Q = P;
        P = Q->Next;
        Q->Parent = 0;
        delete Q;
    } while (P != Top);
    
    Top = Active = 0;
    if (Peer) {
        delete Peer;
        Peer = 0;
    }
    if (Next == this) {
        frames = 0;
        //DosBeep(500, 500);
//        printf("No more frames\x7\x7\n");
    } else {
        Next->Prev = Prev;
        Prev->Next = Next;
        frames = Next;
        frames->Activate();
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
    view->ConQuerySize(&newview->Peer->wW, &dmy);
    newview->Peer->wH = view->Peer->wH - view->Peer->wH / 2 - 1;
    view->Peer->wH /= 2;
    InsertView(view, newview);
    view->ConSetSize(view->Peer->wW, view->Peer->wH);
    newview->ConSetSize(newview->Peer->wW, newview->Peer->wH);
    Peer->SizeFrame();
    return 0;
}

int GFrame::ConCloseView(GView *view) {
    return 0;
}

int GFrame::ConResizeView(GView *view, int DeltaY) {
    return 0;
}

int GFrame::AddView(GView *view) {
    if (Active != 0) {
        return ConSplitView(Active, view);
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
            Top->ConSetSize(Top->Peer->wW, Top->Peer->wH + view->Peer->wH + 1);
        } else {
            view->Prev->ConSetSize(view->Prev->Peer->wW,
                                   view->Prev->Peer->wH + view->Peer->wH + 1);
        }
        
        if (Active == view) {
            Active = view->Prev;
            WinSetFocus(HWND_DESKTOP, Active->Peer->hwndView);
            Active->Activate(1);
        }
    }
    Peer->SizeFrame();
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
        WinSetFocus(HWND_DESKTOP, Active->Peer->hwndView);
    }
}

int GFrame::SelectView(GView *view) {
    if (Top == 0)
        return 0;
    
    if (FocusCapture != 0 || MouseCapture != 0)
        return 0;
    
    if (Active)
        Active->Activate(0);
    Active = view;
    if (Active) {
        Active->Activate(1);
    }
    //DosBeep(50, 500);
    frames = this;
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

int GFrame::SetMenu(const char *Name) {
    //WinAlarm(HWND_DESKTOP, WA_NOTE);
    if (Menu && Name && strcmp(Name, Menu) == 0)
        return 1;
    
    free(Menu);
    Menu = strdup(Name);

    Peer->menuBar = (HWND)WinSendMsg(hwndCreatorUser,
                                     UWM_CREATEMAINMENU,
                                     MPFROMP(Peer),
                                     MPFROMP(Menu));

    return 1;
}

int GFrame::ExecMainMenu(char Sub) {
    HWND hwnd;
    
    hwnd = WinWindowFromID(Peer->hwndFrame, FID_MENU);
    if (Sub != 0) {
        int count = LONGFROMMR(WinSendMsg(hwnd, MM_QUERYITEMCOUNT, 0, 0));
        SHORT id;
        char buf[256];
        int len;
        char srch[3] = "~x";

        srch[1] = (char)toupper(Sub);
        
        //puts("mainmenu");
        for (SHORT i = 0; i < count; i++) {
            id = SHORT1FROMMR(WinSendMsg(hwnd, MM_ITEMIDFROMPOSITION, MPFROMSHORT(i), 0));
            if (id == MIT_ERROR) {
                puts("error");
            } else {
                //printf("got %d %d\n", i, id);
                len = SHORT1FROMMR(WinSendMsg(hwnd, MM_QUERYITEMTEXT, MPFROM2SHORT((id), (sizeof(buf) - 1)), MPFROMP(buf)));
                buf[len] = 0;
                //puts(buf);
                srch[1] = (char)toupper(Sub);
                if (strstr(buf, srch) != 0) {
                    WinSendMsg(hwnd, MM_SELECTITEM, (MPFROM2SHORT(id, 0)), 0);
                    //printf("select %d %d\n", i, id);
                } else {
                    srch[1] = (char)tolower(Sub);
                    if (strstr(buf, srch) != 0) {
                        WinSendMsg(hwnd, MM_SELECTITEM, (MPFROM2SHORT(id, 0)), 0);
                        //printf("select %d %d\n", i, id);
                    }
                }
            }
        }
    } else {
        WinPostMsg(hwnd, MM_STARTMENUMODE, MPFROM2SHORT((Sub ? TRUE : FALSE), FALSE), 0);
    }
    return 1;
}

int GFrame::PopupMenu(const char *Name) {
    int id = GetMenuId(Name);

    if (id == -1)
        return 0;
            

    WinSendMsg(hwndCreatorUser,
               UWM_CREATEPOPUPMENU,
               MPFROMP(Peer),
               MPFROMLONG(id));
    return 1;
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

// GUI

GUI::GUI(int &argc, char **argv, int XSize, int YSize) {
    fArgc = argc;
    fArgv = argv;
    hab = WinInitialize(0);
    hmq = WinCreateMsgQueue(hab, 0);
    assert(0 == DosCreateMutexSem(0, &hmtxPMData, 0, 0));

    cxBorder = WinQuerySysValue(HWND_DESKTOP, SV_CXSIZEBORDER);
    cyBorder = WinQuerySysValue(HWND_DESKTOP, SV_CYSIZEBORDER);
    cxScrollBar = WinQuerySysValue(HWND_DESKTOP, SV_CXVSCROLL);
    cyScrollBar = WinQuerySysValue(HWND_DESKTOP, SV_CYHSCROLL);
    cxScreen = WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN);
    cyScreen = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);
    cyTitleBar = WinQuerySysValue(HWND_DESKTOP, SV_CYTITLEBAR);

    // edit window class
    WinRegisterClass(hab, szClient, (PFNWP)AVIOWndProc, CS_SIZEREDRAW, 4);
    //WinRegisterClass(hab, szSizer, (PFNWP)SizerWndProc, CS_SIZEREDRAW, 4);

    // worker object-window class
    WinRegisterClass(hab, szObject, (PFNWP)ObjectWndProc, 0, 4);

    // window that created windows in thr 1 for thr 2
    WinRegisterClass(hab, szCreator, (PFNWP)CreatorWndProc, 0, 0);

    hwndCreatorUser = WinCreateWindow(HWND_DESKTOP,
                                      szCreator,
                                      "FTEPM",
                                      0,
                                      0, 0, 0, 0,
                                      HWND_DESKTOP, HWND_TOP,
                                      0, NULL, NULL);

    assert(0 == DosCreateEventSem(0, &WorkerStarted, 0, 0));
    assert(0 == DosCreateEventSem(0, &StartInterface, 0, 0));

    _beginthread(WorkThread, FAKE_BEGINTHREAD_NULL PM_STACK_SIZE, 0);

    ULONG ulPostCount;
    DosWaitEventSem(WorkerStarted, SEM_INDEFINITE_WAIT);
    DosResetEventSem(WorkerStarted, &ulPostCount);
    
    gui = this;
}

GUI::~GUI() {
    WinDestroyMsgQueue(hmq);
    WinTerminate(hab);
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
    return ::ConGetEvent(EventMask, Event, WaitTime, Delete, view);
}

int GUI::ConPutEvent(const TEvent& Event) {
    EventBuf = Event;

    return 1;
}

int GUI::ConFlush() {
    return 1;
}

void GUI::ProcessEvent() {
    TEvent E;
    GView *v;

    if (frames == 0)
        return ;

    E.What = evNone;
    if (E.What == evNone) {
        if (ConGetEvent(evMouseDown | evCommand | evKeyDown, &E, 0, 1, &v) == -1)
            ;
    }
    if (E.What == evNone) {
        //DosBeep(500, 100);
        frames->Update();
        //DosBeep(1000, 100);
        if (ConGetEvent(evMouseDown | evCommand | evKeyDown, &E, -1, 1, &v) == -1)
            ;
    }
    if (E.What != evNone) {
        v = E.Msg.View;
        DispatchEvent(v->Parent, v, E);
    }
}

int GUI::Run() {
    QMSG qmsg;

    doLoop = 1;
    DosPostEventSem(StartInterface);

    while (doLoop != 0 && WinGetMsg(hab, &qmsg, NULLHANDLE, 0, 0))
        WinDispatchMsg(hab, &qmsg);

    return 1;
}

int GUI::ShowEntryScreen() {
    return 1;
}

int GUI::RunProgram(int mode, char *Command) {
    char FailBuf[256];
    char *Args;
    char *Prog;
    int rc;
    PID pid;
    ULONG sid;
    int ArgsSize;
    
    Prog = getenv("COMSPEC");

    ArgsSize = 3 + strlen(Command) + 1;
    
    Args = (char *)malloc(ArgsSize);
    if (Args == NULL) {
        return -1;
    }
    
    if (*Command != 0) {
        strlcpy(Args, "/c ", ArgsSize);
        strlcat(Args, Command, ArgsSize);
    } else {
        Args[0] = 0;
    }
    
    {
        STARTDATA sd;
        
        memset((void *)&sd, 0, sizeof(sd));
        sd.Length = sizeof(sd);
        sd.Related = SSF_RELATED_INDEPENDENT;
        sd.FgBg = SSF_FGBG_FORE;
        sd.TraceOpt = SSF_TRACEOPT_NONE;
        sd.PgmTitle = (Command && Command[0] != 0) ? Command : 0;
        sd.PgmName = Prog;
        sd.PgmInputs = Args;
        sd.TermQ = 0;
        sd.Environment = 0;
        sd.InheritOpt = SSF_INHERTOPT_PARENT;
        sd.SessionType = SSF_TYPE_DEFAULT;
        sd.IconFile = 0;
        sd.PgmHandle = 0;
        sd.PgmControl = SSF_CONTROL_VISIBLE;// | ((Command && Command[0] != 0) ? SSF_CONTROL_NOAUTOCLOSE : 0);
        sd.ObjectBuffer = FailBuf;
        sd.ObjectBuffLen = sizeof(FailBuf);
        rc = DosStartSession(&sd, &sid, &pid);
    }
    
    free(Args);
    
    return rc;
}

static int CreatePipeChild(ULONG *sid, PID *pid, HPIPE &hfPipe, char *Command) {
    static int PCount = 0;
    char szPipe[32];
    char FailBuf[256];
    char *Args;
    char *Prog;
#if 0
    int arglen = 0;
    RESULTCODES rc_code;
#endif
    ULONG ulAction;
    //ULONG ulNew;
    HPIPE hfChildPipe;
    HFILE hfNewStdOut = (HFILE)-1, hfNewStdErr = (HFILE)-1;
    HFILE hfStdOut = (HFILE)1, hfStdErr = (HFILE)2;
    int rc;
    
    sprintf(szPipe, "\\PIPE\\FTE%d\\CHILD%d", getpid(), PCount);
    PCount++;
    
    rc = DosCreateNPipe(szPipe, &hfPipe,
                        NP_NOINHERIT | NP_ACCESS_INBOUND,
                        NP_NOWAIT | NP_TYPE_BYTE | NP_READMODE_BYTE | 1,
                        0, 4096, 0);
    if (rc != 0)
        return -1;
    
    rc = DosConnectNPipe (hfPipe);
    if (rc != 0 && rc != ERROR_PIPE_NOT_CONNECTED) {
        DosClose(hfPipe);
        return -1;
    }
    
    rc = DosSetNPHState (hfPipe, NP_WAIT | NP_READMODE_BYTE);
    if (rc != 0) {
        DosClose(hfPipe);
        return -1;
    }
    
    rc = DosOpen (szPipe, &hfChildPipe, &ulAction, 0,
                  FILE_NORMAL,
                  OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW,
                  OPEN_ACCESS_WRITEONLY | OPEN_SHARE_DENYREADWRITE,
                  NULL);
    if (rc != 0) {
        DosClose (hfPipe);
        return -1;
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
    
#if 0
    Args = (char *)malloc(strlen(Prog) + 1
                          + 3 + strlen(Command) + 1
                          + 1);
    if (Args == NULL) {
        DosClose(hfPipe);
        return -1;
    }
    
    strcpy(Args, Prog);
    arglen = strlen(Args) + 1;
    strcpy(Args + arglen, "/c ");
    arglen += 3;
    strcpy(Args + arglen, Command);
    arglen += strlen(Command) + 1;
    Args[arglen] = '\0';
#else
    int ArgsSize = 3 + strlen(Command) + 1;
    Args = (char *)malloc(ArgsSize);
    if (Args == NULL) {
        DosClose(hfPipe);
        return -1;
    }
    
    strlcpy(Args, "/c ", ArgsSize);
    strlcat(Args, Command, ArgsSize);
#endif
    
    
#if 0
    rc = DosExecPgm(FailBuf, sizeof(FailBuf),
                    EXEC_ASYNCRESULT, // | EXEC_BACKGROUND,
                    Args,
                    0,
                    &rc_code,
                    Prog);
#else
    {
        STARTDATA sd;

        memset((void *)&sd, 0, sizeof(sd));
        sd.Length = sizeof(sd);
        sd.Related = SSF_RELATED_INDEPENDENT;
        sd.FgBg = SSF_FGBG_BACK;
        sd.TraceOpt = SSF_TRACEOPT_NONE;
        sd.PgmTitle = 0;
        sd.PgmName = Prog;
        sd.PgmInputs = Args;
        sd.TermQ = 0;
        sd.Environment = 0;
        sd.InheritOpt = SSF_INHERTOPT_PARENT;
        sd.SessionType = SSF_TYPE_DEFAULT;
        sd.IconFile = 0;
        sd.PgmHandle = 0;
        sd.PgmControl = SSF_CONTROL_INVISIBLE;
        sd.ObjectBuffer = FailBuf;
        sd.ObjectBuffLen = sizeof(FailBuf);
        rc = DosStartSession(&sd, sid, pid);
    }
#endif
    
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
        return -1;
    }
    
#if 0
    pid = rc_code.codeTerminate; // get pid when successful
    sid = 0;
#endif
    
    return 0;
}

static void _LNK_CONV PipeThread(void *p) {
    GPipe *pipe = (GPipe *)p;
    HAB hab;
    ULONG ulPostCount;
    ULONG used;
    PID pid;
    ULONG sid;
    HPIPE hfPipe;
#if 0
    RESULTCODES rc_code;
#endif
    int rc;
    
    hab = WinInitialize(0);
    
    rc = CreatePipeChild(&sid, &pid, hfPipe, pipe->Command);
    
    if (rc != 0) {
        DosRequestMutexSem(pipe->Access, SEM_INDEFINITE_WAIT);
        pipe->reading = 0;
        if (pipe->notify)
            WinPostMsg(frames->Active->Peer->hwndWorker, UWM_NOTIFY, MPFROMLONG(pipe->notify), MPFROMLONG(pipe->id));
        DosReleaseMutexSem(pipe->Access);
        WinTerminate(hab);
        return;
    }
//    fprintf(stderr, "Pipe: Begin: %d\n", pipe->id);
    while (1) {
        rc = DosRead(hfPipe, pipe->buffer, pipe->buflen, &used);
        if (rc < 0)
            used = 0;

        DosRequestMutexSem(pipe->Access, SEM_INDEFINITE_WAIT);
        pipe->bufused = used;
//        fprintf(stderr, "Pipe: fread: %d %d\n", pipe->id, pipe->bufused);
        DosResetEventSem(pipe->ResumeRead, &ulPostCount);
        if (pipe->bufused == 0)
            break;
        if (pipe->notify && pipe->stopped) {
            WinPostMsg(frames->Active->Peer->hwndWorker, UWM_NOTIFY, MPFROMLONG(pipe->notify), MPFROMLONG(pipe->id));
            pipe->stopped = 0;
        }
        DosReleaseMutexSem(pipe->Access);
        if (pipe->DoTerm)
            break;
        DosWaitEventSem(pipe->ResumeRead, SEM_INDEFINITE_WAIT);
        if (pipe->DoTerm)
            break;
    }
//    fprintf(stderr, "Pipe: pClose: %d\n", pipe->id);
    DosClose(hfPipe);
    //fprintf(stderr, "Pipe: pClose: %d\n", pipe->id);
#if 0
    rc = DosWaitChild(DCWA_PROCESS, DCWW_WAIT,
                      &rc_code,
                      &pid,
                      pid);
    pipe->RetCode = rc_code.codeResult;
#else
    //DosStopSession(STOP_SESSION_SPECIFIED, sid);
    pipe->RetCode = 0;
#endif
    pipe->reading = 0;
    if (pipe->notify)
        WinPostMsg(frames->Active->Peer->hwndWorker, UWM_NOTIFY, MPFROMLONG(pipe->notify), MPFROMLONG(pipe->id));
    DosReleaseMutexSem(pipe->Access);
    WinTerminate(hab);
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
            
            Pipes[i].tid = _beginthread(PipeThread,
                                        FAKE_BEGINTHREAD_NULL
                                        16384, &Pipes[i]);

            Pipes[i].used = 1;
//            fprintf(stderr, "Pipe Open: %d\n", i);
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
//    fprintf(stderr, "Pipe View: %d %08X\n", id, notify);
    Pipes[id].notify = notify;
    DosReleaseMutexSem(Pipes[id].Access);

    return 1;
}

int GUI::ReadPipe(int id, void *buffer, size_t len) {
    int l;
    //ULONG ulPostCount;
    
    if (id < 0 || id > MAX_PIPES)
        return -1;
    if (Pipes[id].used == 0)
        return -1;
//    DosQueryEventSem(Pipes[id].ResumeRead, &ulPostCount);
//    if (ulPostCount != 0)
//        return 0;
    DosRequestMutexSem(Pipes[id].Access, SEM_INDEFINITE_WAIT);
//    fprintf(stderr, "Pipe Read: Get %d %d\n", id, len);
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
//            fprintf(stderr, "Pipe Resume Read: %d\n", id);
            Pipes[id].stopped = 1;
            DosPostEventSem(Pipes[id].ResumeRead);
        }
    } else if (Pipes[id].reading == 0)
        l = -1;
    else {
        l = 0;
        //DosBeep(200, 200);
    }
//    fprintf(stderr, "Pipe Read: Got %d %d\n", id, l);
    DosReleaseMutexSem(Pipes[id].Access);
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
    DosCloseEventSem(Pipes[id].ResumeRead);
    DosCloseMutexSem(Pipes[id].Access);
//    fprintf(stderr, "Pipe Close: %d\n", id);
    Pipes[id].used = 0;

    return (Pipes[id].RetCode == 0);
}

int GUI::multiFrame() {
    return 1;
}

void DieError(int rc, const char *msg, ...) {
    va_list ap;
    char str[1024];
    
    va_start(ap, msg);
    vsprintf(str, msg, ap);
    va_end(ap);
    if (hab == 0)
        hab = WinInitialize(0);
    if (hmq == 0)
        hmq = WinCreateMsgQueue(hab, 0);
    WinMessageBox(HWND_DESKTOP, HWND_DESKTOP, str, "FTE", 0, MB_OK | MB_ERROR);
    WinDestroyMsgQueue(hmq);
    WinTerminate(hab);
    exit(rc);
}

char ConGetDrawChar(unsigned int index) {
    static const char tab[] = "\xDA\xBF\xC0\xD9\xC4\xB3\xC2\xC3\xB4\xC1\xC5\x1A\xFA\x04\xC4\x18\x19\xB1\xB0\x1B\x1A";

    assert(index < strlen(tab));

    return tab[index];
}
