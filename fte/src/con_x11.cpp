/*    con_x11.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 *    I18N & XMB support added by kabi@users.sf.net
 */

#include "c_config.h"
#include "con_i18n.h"
#include "gui.h"
#include "s_files.h"
#include "s_string.h"
#include "s_util.h"
#include "sysdep.h"

#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <X11/Xos.h>
#ifdef USE_XTINIT
#include <X11/Intrinsic.h>
#endif

#ifdef HPUX
#include <X11R5/X11/HPkeysym.h>
#else
#ifdef HAVE_HPKEYSYM
#include <HPkeysym.h>
#else
#define XK_ClearLine            0x1000FF6F
#define XK_InsertLine           0x1000FF70
#define XK_DeleteLine           0x1000FF71
#define XK_InsertChar           0x1000FF72
#define XK_DeleteChar           0x1000FF73
#define XK_BackTab              0x1000FF74
#define XK_KP_BackTab           0x1000FF75
#endif // HPKEYSYM
#endif // HPUX

#ifdef WINHCLX
#include <X11/XlibXtra.h>    /* HCL - HCLXlibInit */
#endif

//#undef CONFIG_X11_XICON
#ifdef CONFIG_X11_XICON
#include <X11/xpm.h>

#define ICON_COUNT 4
#include "icons/fte16x16.xpm"
#include "icons/fte32x32.xpm"
#include "icons/fte48x48.xpm"
#include "icons/fte64x64.xpm"
#endif // CONFIG_X11_XICON

#ifdef WINNT
#include <winsock.h>
#define NO_PIPES
#define NO_SIGNALS
#else
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#endif

#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#if defined(AIX)
#include <strings.h>
#endif

#include <stdio.h>
#include <stdarg.h>

#define MAX_SCRWIDTH 255
#define MAX_SCRHEIGHT 255
#define MIN_SCRWIDTH 20
#define MIN_SCRHEIGHT 6

#define SELECTION_INCR_LIMIT 0x1000
#define SELECTION_XFER_LIMIT 0x1000
#define SELECTION_MAX_AGE 10

class ColorXGC;

// times in miliseconds
static const unsigned int MouseAutoDelay = 40;
static const unsigned int MouseAutoRepeat = 200;
static const long MouseMultiClick = 300;

static int setUserPosition;
static int initX, initY;
static int ScreenCols = 80;
static int ScreenRows = 40;
static int CursorX;
static int CursorY;
static int CursorVisible = 1;
static int CursorStart, CursorEnd;
static unsigned long CursorLastTime;
// Cursor flashing interval, in msecs
static const unsigned int CursorFlashInterval = 300;
static TCell *ScreenBuffer;
static int Refresh;

// res_name can be set with -name switch
static char res_name[20] = "fte";
static char res_class[] = "Fte";

static Display *display;
static Atom wm_protocols;
static Atom wm_delete_window;
static Atom XA_CLIPBOARD;
static Atom proptype_targets;
static Atom proptype_text;
static Atom proptype_compound_text;
static Atom proptype_utf8_string;
static Atom proptype_incr;
static Window win;
static Atom prop_selection;
static XSizeHints size_hints;
// program now contains both modes if available
// some older Xservers don't like XmbDraw...
static XFontStruct* font_struct;
static ColorXGC* colorXGC;
#ifdef CONFIG_X11_XMB
static int useXMB = true;
static XFontSet font_set;
static int FontCYD;
#else
static int useXMB;
#endif
static i18n_context_t* i18n_ctx;
static int useI18n = true;
static int FontCX, FontCY;
static XColor Colors[16];
static char winTitle[256] = "FTE";
static char winSTitle[256] = "FTE";

static unsigned char* CurSelectionData[3];
static int CurSelectionLen[3];
static int CurSelectionOwn[3];
static Time now;

struct IncrementalSelectionInfo {
    IncrementalSelectionInfo *next;
    unsigned char *data;
    int len;
    int pos;
    Atom requestor;
    Atom property;
    Atom type;
    time_t lastUse;
};

static IncrementalSelectionInfo *incrementalSelections;
static Bool gotXError;

static void SendSelection(XEvent *notify, Atom property, Atom type, unsigned char *data, size_t len, Bool privateData);

static int ErrorHandler(Display *, XErrorEvent *ee) {
    gotXError = True;

    return 1;
}

static Atom GetXClip(int clipboard) {
    switch (clipboard) {
    case 1: return XA_PRIMARY;
    case 2: return XA_SECONDARY;
    default: return XA_CLIPBOARD;
    }
}

static int GetFTEClip(Atom clip) {
    if (clip == XA_CLIPBOARD)
        return 0;
    if (clip == XA_PRIMARY)
        return 1;
    if (clip == XA_SECONDARY)
        return 2;
    return -1;
}

static const struct {
    unsigned short r, g, b;
} dcolors[] = {
    {   0,   0,   0 },  //     black
    {   0,   0, 160 },  // darkBlue
    {   0, 160,   0 },  // darkGreen
    {   0, 160, 160 },  // darkCyan
    { 160,   0,   0 },  // darkRed
    { 160,   0, 160 },  // darkMagenta
    { 160, 160,   0 },  // darkYellow
    { 204, 204, 204 },  // paleGray
    { 160, 160, 160 },  // darkGray
    {   0,   0, 255 },  //     blue
    {   0, 255,   0 },  //     green
    {   0, 255, 255 },  //     cyan
    { 255,   0,   0 },  //     red
    { 255,   0, 255 },  //     magenta
    { 255, 255,   0 },  //     yellow
    { 255, 255, 255 },  //     white
};

static void SetColor(int i) {
    assert (0 <= i && i <= 15);

    if (RGBColorValid[i]) {
        Colors[i].blue  = (unsigned short)((RGBColor[i].b << 8) | RGBColor[i].b);
        Colors[i].green = (unsigned short)((RGBColor[i].g << 8) | RGBColor[i].g);
        Colors[i].red   = (unsigned short)((RGBColor[i].r << 8) | RGBColor[i].r);
    } else {
        Colors[i].blue  = (unsigned short)((dcolors[i].b << 8) | dcolors[i].b);
        Colors[i].green = (unsigned short)((dcolors[i].g << 8) | dcolors[i].g);
        Colors[i].red   = (unsigned short)((dcolors[i].r << 8) | dcolors[i].r);
    }

    Colors[i].flags = DoRed | DoGreen | DoBlue;
}

static int InitXColors(Colormap colormap) {
    long d = 0x7FFFFFFF, d1;
    XColor clr;
    long d_red, d_green, d_blue;
    long u_red, u_green, u_blue;

    for (int i = 0; i < 16; ++i) {
        SetColor(i);
        if (XAllocColor(display, colormap, &Colors[i]) == 0) {
            SetColor(i);
            unsigned long pix = 0xFFFFFFFF;
            int num = DisplayCells(display, DefaultScreen(display));
            for (int j = 0; j < num; ++j) {
                clr.pixel = j;
                XQueryColor(display, colormap, &clr);

                d_red = (clr.red - Colors[i].red) >> 3;
                d_green = (clr.green - Colors[i].green) >> 3;
                d_blue = (clr.blue - Colors[i].blue) >> 3;

                //fprintf(stderr, "%d:%d dr:%d, dg:%d, db:%d\n", i, j, d_red, d_green, d_blue);

                u_red = d_red / 100 * d_red * 3;
                u_green = d_green / 100 * d_green * 4;
                u_blue = d_blue / 100 * d_blue * 2;

                //fprintf(stderr, "%d:%d dr:%u, dg:%u, db:%u\n", i, j, u_red, u_green, u_blue);

                d1 = u_red + u_blue + u_green;

                if (d1 < 0)
                    d1 = -d1;
                if (pix == ~0UL || d1 < d) {
                    pix = j;
                    d = d1;
                }
            }
            if (pix == 0xFFFFFFFF) {
                fprintf(stderr, "Color search failed for #%04X%04X%04X\n",
                        Colors[i].red, Colors[i].green, Colors[i].blue);
            }
            clr.pixel = pix;
            XQueryColor(display, colormap, &clr);
            Colors[i] = clr;
            if (XAllocColor(display, colormap, &Colors[i]) == 0) {
                fprintf(stderr, "Color alloc failed for #%04X%04X%04X\n",
                        Colors[i].red, Colors[i].green, Colors[i].blue);
            }
            /*colormap = XCreateColormap(display, win, DefaultVisual(display, screen), AllocNone);
             for (i = 0; i < 16; i++) {
             SetColor(i);
             XAllocColor(display, colormap, &Colors[i]);
             }
             XSetWindowColormap(display, win, colormap);
             return 0;*/
        }
    }
    /* clear backgroud to background color when the windows is obscured
     * its looks more pleasant before the redraw event does its job */
    XSetWindowBackground(display, win,
                         ((Colors[0].blue & 0xff00) << 8)
                         | ((Colors[0].green & 0xff00))
                         | ((Colors[0].red & 0xff00) >> 8));
    return 1;
}

class ColorXGC {
    unsigned long mask;
    XGCValues gcv;
    GC GCs[256];
    Region reg;

    void clear()
    {
        if (reg)
            XDestroyRegion(reg);
        for (unsigned i = 0; i < 256; ++i)
            if (GCs[i])
                XFreeGC(display, GCs[i]);
    }

public:
    ColorXGC() : mask(GCForeground | GCBackground), reg(0)
    {
        if (!useXMB) {
            gcv.font = font_struct->fid;
            mask |= GCFont;
        }
        memset(&GCs, 0, sizeof(GCs));
    }

    ~ColorXGC()
    {
        clear();
    }

    GC& GetGC(unsigned i)
    {
        assert(i < FTE_ARRAY_SIZE(GCs));
        if (!GCs[i]) {
            gcv.foreground = Colors[i % 16].pixel;
            gcv.background = Colors[i / 16].pixel;
            GCs[i] = XCreateGC(display, win, mask, &gcv);
            if (reg)
                XSetRegion(display, GCs[i], reg);
        }
        return GCs[i];
    }

    void SetRegion(Region r)
    {
        clear();
        memset(&GCs, 0, sizeof(GCs));
        reg = r;
    }
};

#ifdef CONFIG_X11_XMB
static int TryLoadFontset(const char *fs)
{
    char *def = NULL;
    char **miss = NULL;
    int nMiss = 0;

    if (font_set)
        return 0;

    if (!fs || !*fs)
        return 0;

    if (!(font_set = XCreateFontSet(display, fs, &miss, &nMiss, &def))) {
        fprintf(stderr, "XFTE Warning: unable to open font \"%s\":\n"
                " Missing count: %d\n", fs, nMiss);
        for(int i = 0; i < nMiss; i++)
            fprintf(stderr, "  %s\n", miss[i]);
        if (def != NULL)
            fprintf(stderr, " def_ret: %s\n", def);
    }
    //else fprintf(stderr, "fonts  %p  %d   %p\n", miss, nMiss, def);
    if (nMiss)
        XFreeStringList(miss);

    return (font_set != NULL);
}
#endif // CONFIG_X11_XMB

static int InitXFonts()
{
    char *fs = getenv("VIOFONT");
    if (!fs && WindowFont[0] != 0)
        fs = WindowFont;

    if (!useXMB) {
        font_struct = NULL;

        if (fs) {
            char *s = strchr(fs, ',');

            if (s)
                *s = 0;
            font_struct = XLoadQueryFont(display, fs);
        }

        if (!font_struct
            && !(font_struct = XLoadQueryFont(display, "8x13"))
            && !(font_struct = XLoadQueryFont(display, "fixed")))
            return 0;

        FontCX = font_struct->max_bounds.width;
        FontCY = font_struct->max_bounds.ascent + font_struct->max_bounds.descent;
    }
#ifdef CONFIG_X11_XMB
    else {
        if (!TryLoadFontset(getenv("VIOFONT"))
            && !TryLoadFontset(WindowFont)
            && !TryLoadFontset("-misc-*-r-normal-*")
            && !TryLoadFontset("*fixed*"))
            return 0;

        XFontSetExtents *xE = XExtentsOfFontSet(font_set);

        FontCX = xE->max_logical_extent.width;
        FontCY = xE->max_logical_extent.height;
        // handle descending (comes in negative form)
        FontCYD = -(xE->max_logical_extent.y);
        // printf("Font X:%d\tY:%d\tD:%d\n", FontCX, FontCY, FontCYD);
    }
#endif // CONFIG_X11_XMB

    return 1;
}

static int SetupXWindow(int argc, char **argv)
{
#ifdef WINHCLX
    HCLXlibInit(); /* HCL - Initialize the X DLL */
#endif

#ifdef USE_XTINIT
    XtAppContext app_context;
    XtToolkitInitialize();
    app_context = XtCreateApplicationContext();
    if (( display = XtOpenDisplay(app_context, NULL, argv[0], "xfte",
                            NULL, 0, &argc, argv)) == NULL)
       DieError(1, "%s:  Can't open display\n", argv[0]);
#else
    const char *ds = getenv("DISPLAY");
    if (!ds)
        DieError(1, "$DISPLAY not set? This version of fte must be run under X11.");
    if ((display = XOpenDisplay(ds)) == NULL)
        DieError(1, "XFTE Fatal: could not open display: %s!", ds);
#endif
    Colormap colormap = DefaultColormap(display, DefaultScreen(display));

    XSetWindowAttributes wattr;
    wattr.win_gravity = NorthWestGravity;
    wattr.bit_gravity = NorthWestGravity;
    wattr.save_under = False;
    wattr.backing_store = NotUseful;
    wattr.background_pixel = BlackPixel(display, DefaultScreen(display));

    // this is correct behavior
    if (initX < 0)
        initX = DisplayWidth(display, DefaultScreen(display)) + initX;
    if (initY < 0)
        initY = DisplayHeight(display, DefaultScreen(display)) + initY;

    win = XCreateWindow(display, DefaultRootWindow(display),
                        initX, initY,
                        // ScreenCols * FontCX, ScreenRows * FontCY, 0,
                        // at this moment we don't know the exact size
                        // but we need to open a window - so pick up 1 x 1
                        1, 1, 0,
                        CopyFromParent, InputOutput, CopyFromParent,
                        CWBackingStore | CWBackPixel | CWSaveUnder |
                        CWBitGravity | CWWinGravity,
                        &wattr);

    unsigned long mask = 0;
    i18n_ctx = (useI18n) ? i18n_open(display, win, &mask) : 0;

    if (!InitXFonts())
        DieError(1, "XFTE Fatal: could not open any font!");

    /* >KeyReleaseMask shouldn't be set for correct key mapping */
    /* we set it anyway, but not pass to XmbLookupString -- mark */
    mask |= ExposureMask | StructureNotifyMask | VisibilityChangeMask |
        FocusChangeMask | KeyPressMask | KeyReleaseMask |
        ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | PropertyChangeMask;
    XSelectInput(display, win, mask);

    wm_protocols = XInternAtom(display, "WM_PROTOCOLS", False);
    assert(wm_protocols != None);
    wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
    assert(wm_delete_window != None);
    prop_selection = XInternAtom(display, "fte_clip", False);
    assert(prop_selection != None);
    XA_CLIPBOARD = XInternAtom(display, "CLIPBOARD", False);
    assert(XA_CLIPBOARD != None);
    proptype_targets = XInternAtom(display, "TARGETS", False);
    assert(proptype_targets != None);
    proptype_text = XInternAtom(display, "TEXT", False);
    assert(proptype_text != None);
    proptype_compound_text = XInternAtom(display, "COMPOUND_TEXT", False);
    assert(proptype_compound_text != None);
    proptype_utf8_string = XInternAtom(display, "UTF8_STRING", False);
    assert(proptype_utf8_string != None);
    proptype_incr = XInternAtom(display, "INCR", False);
    assert(proptype_incr != None);

    size_hints.flags = PResizeInc | PMinSize | PBaseSize;
    size_hints.width_inc = FontCX;
    size_hints.height_inc = FontCY;
    size_hints.min_width = MIN_SCRWIDTH * FontCX;
    size_hints.min_height = MIN_SCRHEIGHT * FontCY;
    size_hints.base_width = 0;
    size_hints.base_height = 0;
    if (setUserPosition)
        size_hints.flags |= USPosition;
    XSetStandardProperties(display, win, winTitle, winTitle, 0, NULL, 0, &size_hints);

    XClassHint class_hints;
    class_hints.res_name = res_name;
    class_hints.res_class = res_class;
    XSetClassHint(display, win, &class_hints);

    XSetWMProtocols(display, win, &wm_delete_window, 1);

    if (!InitXColors(colormap))
        return 0;
    colorXGC = new ColorXGC();

    XWMHints wm_hints;
    wm_hints.flags = InputHint | StateHint;
    wm_hints.input = True;
    wm_hints.initial_state = NormalState;

#ifdef CONFIG_X11_XICON
    // Set icons using _NET_WM_ICON property
    XpmAttributes attributes;
    attributes.valuemask = 0;//XpmColormap | XpmDepth | XpmCloseness;
    //attributes.colormap = colormap;
    //attributes.depth = DefaultDepth(display, DefaultScreen(display));
    //attributes.closeness = 40000;
    //attributes.exactColors = False;

    // Set icon using WMHints
    if (XpmCreatePixmapFromData(display, win, const_cast<char**>(fte16x16_xpm),
                                &wm_hints.icon_pixmap, &wm_hints.icon_mask,
                                &attributes) == XpmSuccess)
        wm_hints.flags |= IconPixmapHint | IconMaskHint;

    static const char * const *xpmData[ICON_COUNT] = { fte16x16_xpm, ftepm, fte48x48_xpm, fte64x64_xpm };
    XpmImage xpmImage[ICON_COUNT];
    CARD32 *xpmColors[ICON_COUNT] = { NULL, NULL, NULL, NULL };
    int i, iconBufferSize = 0;
    unsigned int j;

    // Load icons as XpmImage instances and create their colormaps
    for (i = 0; i < ICON_COUNT; i++) {
        XpmImage &xpm = xpmImage[i];
        CARD32 *&colors = xpmColors[i];
        if (XpmCreateXpmImageFromData(const_cast<char**>(xpmData[i]), &xpm,
                                      NULL) != XpmSuccess)
            break;
        iconBufferSize += 2 + xpm.width * xpm.height;
        if (!(colors = (CARD32 *)malloc(xpm.ncolors * sizeof(CARD32)))) {
            // Need to clear here as cleanup at the end checks for colors[i] to see if XPM was loaded
            XpmFreeXpmImage(&xpm);
            break;
        }
        // Decode all colors
        for (j = 0; j < xpm.ncolors; j++) {
            XColor xc;
            char *c = xpm.colorTable[j].c_color;
            if (c == NULL) c = xpm.colorTable[j].g_color;
            if (c == NULL) c = xpm.colorTable[j].g4_color;
            if (c == NULL) c = xpm.colorTable[j].m_color;
            if (c == NULL) c = xpm.colorTable[j].symbolic;
            if (c == NULL)
                // Unknown color
                colors[j] = 0;
            else if (strcmp(c, "None") == 0)
                // No color - see thru
                colors[j] = 0;
            else if (XParseColor(display, colormap, c, &xc)) {
                // Color parsed successfully
                ((char *)(colors + j))[0] = (char)(xc.blue >> 8);
                ((char *)(colors + j))[1] = (char)(xc.green >> 8);
                ((char *)(colors + j))[2] = (char)(xc.red >> 8);
                ((char *)(colors + j))[3] = (char)0xff;
            } else
                // Color parsing failed
                colors[j] = 0;
        }
    }
    if (i == ICON_COUNT) {
        // Everything OK, can create property
        // XChangeProperty takes 32-bit entities in long array
        // (which is 64bit on x86_64)
        long *iconBuffer = (long *)malloc(iconBufferSize * sizeof(long) + 16);
        if (iconBuffer) {
            long *b = iconBuffer;
            for (i = 0; i < ICON_COUNT; i++) {
                XpmImage &xpm = xpmImage[i];
                CARD32 *&colors = xpmColors[i];
                *b++ = xpm.width; *b++ = xpm.height;
                for (j = 0; j < xpm.width * xpm.height; j++)
                    *b++ = colors[xpm.data[j]];
            }
            Atom at = XInternAtom(display, "_NET_WM_ICON", False);
            if (at != None)
                XChangeProperty(display, win, at,
                                XA_CARDINAL, 32, PropModeReplace,
                                (unsigned char *)iconBuffer, iconBufferSize);
            free(iconBuffer);
        }
    }
    // Cleanup
    for (i = 0; i < ICON_COUNT; i++) {
        if (xpmColors[i]) {
            free(xpmColors[i]);
            XpmFreeXpmImage(xpmImage + i);
        }
    }
#endif // CONFIG_X11_XICON
    XSetWMHints(display, win, &wm_hints);
    XResizeWindow(display, win, ScreenCols * FontCX, ScreenRows * FontCY);
    XMapRaised(display, win); // -> Expose
    return 1;
}

#define CursorXYPos(x, y) (ScreenBuffer + ((x) + ((y) * ScreenCols)))
static void DebugShowArea(int X, int Y, int W, int H, int clr)
{
    return;
    fprintf(stderr, "Draw %02d X:%2d Y:%2d  W:%2d x H:%2d\n", clr, X, Y, W, H);
    XFillRectangle(display, win, colorXGC->GetGC(clr),
                   X * FontCX, Y * FontCY, W * FontCX,
                   H * FontCY / ((clr == 13) ? 2 : 1));
    XEvent e;
    while (XCheckTypedWindowEvent(display, win, GraphicsExpose, &e))
        XNextEvent(display, &e);
    usleep(2000);
}

static void DrawCursor(int Show) {
    if (CursorVisible) {
        TCell *Cell = CursorXYPos(CursorX, CursorY);

        // Check if cursor is on or off due to flashing
        if (CursorBlink)
            Show &= (CursorLastTime % (CursorFlashInterval * 2)) > CursorFlashInterval;
        int attr = Cell->GetAttr() ^ (Show ? 0xff : 0);
        char ch = (char) Cell->GetChar();
        if (!useXMB)
            XDrawImageString(display, win, colorXGC->GetGC(attr),
                             CursorX * FontCX,
                             font_struct->max_bounds.ascent + CursorY * FontCY,
                             &ch, 1);
#ifdef CONFIG_X11_XMB
        else
            XmbDrawImageString(display, win, font_set, colorXGC->GetGC(attr),
                               CursorX * FontCX, FontCYD + CursorY * FontCY,
                               &ch, 1);
#endif
#if 0
        if (Show) {
            int cs = (CursorStart * FontCY + FontCY / 2) / 100;
            int ce = (CursorEnd   * FontCY + FontCY / 2) / 100;
            XFillRectangle (display, win, GCs[p[1]],
                            CursorX * FontCX, CursorY * FontCY + cs,
                            FontCX, ce - cs);
        }
#endif
    }
}

int ConInit(int XSize, int YSize) {
    if (XSize != -1)
        ScreenCols = XSize;
    if (YSize != -1)
        ScreenRows = YSize;
    if (!(ScreenBuffer = new TCell[ScreenCols * ScreenRows]))
        return 0;
#ifndef NO_SIGNALS
    signal(SIGALRM, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
#endif
    return 1;
}

int ConDone() {
    delete[] ScreenBuffer;
    ScreenBuffer = 0;
    return 1;
}

int ConSuspend() {
    return 1;
}

int ConContinue() {
    return 1;
}

int ConClear() {
    TDrawBuffer B;
    MoveCh(B, ' ', 0x07, ScreenCols);

    return ConPutLine(0, 0, ScreenCols, ScreenRows, B);
}

int ConSetTitle(const char *Title, const char *STitle) {
    char buf[sizeof(winTitle)] = {0};

    JustFileName(Title, buf, sizeof(buf));
    if (buf[0] == '\0') // if there is no filename, try the directory name.
        JustLastDirectory(Title, buf, sizeof(buf));

    snprintf(winTitle, sizeof(winTitle), "FTE - %s%s%s",
             buf, buf[0] ? " - " : "", Title);

    strlcpy(winSTitle, STitle, sizeof(winSTitle));
    XSetStandardProperties(display, win, winTitle, winSTitle, 0, NULL, 0, NULL);

    return 1;
}

int ConGetTitle(char *Title, size_t MaxLen, char *STitle, size_t SMaxLen) {
    strlcpy(Title, winTitle, MaxLen);
    strlcpy(STitle, winSTitle, SMaxLen);

    return 1;
}

int ConPutBox(int X, int Y, int W, int H, PCell Cell)
{
    assert(X >= 0 && W >= 0 && Y >= 0 && H >= 0);

    if (X >= ScreenCols || Y >= ScreenRows) {
        //fprintf(stderr, "%d %d  %d %d %d %d\n", ScreenCols, ScreenRows, X, Y, W, H);
        return 0;
    }

    //XClearArea(display, win, X, Y, W * FontCX, H * FontCY, False);
    //DebugShowArea(X, Y, W, H, 13);
    //fprintf(stderr, "%d %d  %d %d %d %d\n", ScreenCols, ScreenRows, X, Y, W, H);
    if (W > ScreenCols)
        W = ScreenCols;

    if (H > ScreenRows)
        H = ScreenRows;

    for (int i = 0; i < H; ++i) {
        char temp[ScreenCols + 1];
        TCell* pCell = CursorXYPos(X, Y + i);
        int x = 0, l;
        while (x < W) {
            if (!Refresh && Cell[x] == pCell[x]) {
                x++;
                continue;
            }

            TAttr attr = Cell[x].GetAttr();
            for (l = 0; (x + l) < W && l < (int)sizeof(temp) - 1; ++l) {
                const unsigned p = x + l;
                if (attr != Cell[p].GetAttr())
                    break;

                if (!Refresh && Cell[p] == pCell[p])
                    break;
                // find larges not yet printed string with same attributes
                pCell[p] = Cell[p];
                char& ch = temp[l];
                switch (Cell[p].GetChar()) {
                // remap needs to be done in upper layer
                case '\t': ch = (char)3; break;  // HT
                //case '\n': ch = (char)9; break;  // NL
                //case '\r': ch = (char)5; break;  // CR
                default: ch = Cell[p].GetChar();
                }
            }

            if (!useXMB)
                XDrawImageString(display, win, colorXGC->GetGC(attr),
                                 (X + x) * FontCX, font_struct->max_bounds.ascent +
                                 (Y + i) * FontCY,
                                 temp, l);
#ifdef CONFIG_X11_XMB
            else
                XmbDrawImageString(display, win, font_set,
                                   colorXGC->GetGC(attr),
                                   (X + x) * FontCX, FontCYD + (Y + i) * FontCY,
                                   temp, l);
#endif
            //DebugShowArea(x, Y + i, l, 1, 13);
            //temp[l] = 0; printf("%s\n", temp);
            x += l;
        }

	if (i + Y == CursorY)
            DrawCursor(1);

	Cell += W;
    }

    return 1;
}

int ConGetBox(int X, int Y, int W, int H, PCell Cell) {
    for (int i = 0; i < H; Cell += W, ++i)
        memcpy(Cell, CursorXYPos(X, Y + i), W * sizeof(TCell));

    return 1;
}

int ConPutLine(int X, int Y, int W, int H, PCell Cell) {
    for (int i = 0; i < H; ++i)
        if (ConPutBox(X, Y + i, W, 1, Cell) != 0)
            return 0;

    return 1;
}

int ConSetBox(int X, int Y, int W, int H, TCell Cell) {
    TDrawBuffer B;

    for (int i = 0; i < W; i++)
        B[i] = Cell;
    ConPutLine(X, Y, W, H, B);

    return 1;
}

int ConScroll(int Way, int X, int Y, int W, int H, TAttr Fill, int Count) {
    int l;

    TCell Cell(' ', Fill);
    DrawCursor(0);

    if (Way == csUp) {
        DebugShowArea(X, (Y + Count), W, (H - Count), 14);
        XCopyArea(display, win, win, colorXGC->GetGC(0),
                  X * FontCX, (Y + Count) * FontCY,
                  W * FontCX, (H - Count) * FontCY,
                  X * FontCX, Y * FontCY);

	for (l = 0; l < H - Count; ++l)
            memcpy(CursorXYPos(X, Y + l), CursorXYPos(X, Y + l + Count), W * sizeof(TCell));
        //l = H - Count;
        //fprintf(stderr, "X:%d   Y:%d   W:%d   H:%d  c:%d\n", X, Y, W, H, Count);
        //ConGetBox(0, Y + Count, ScreenCols, H - Count, CursorXYPos(0, Y));
        //if (Count > 1 && ConSetBox(X, Y + l, W, Count, Cell) == -1)
        //    return -1;
    } else if (Way == csDown) {
        DebugShowArea(X, Y, W, (H - Count), 15);
        XCopyArea(display, win, win, colorXGC->GetGC(0),
                  X * FontCX, Y * FontCY,
                  W * FontCX, (H - Count) * FontCY,
                  X * FontCX, (Y + Count) * FontCY);

	for (l = H - 1; l >= Count; --l)
            memcpy(CursorXYPos(X, Y + l), CursorXYPos(X, Y + l - Count), W * sizeof(TCell));

        //if (Count > 1 && ConSetBox(X, Y, W, Count, Cell) == -1)
        //    return -1;
    }

    DrawCursor(1);

    return 1;
}

int ConSetSize(int X, int Y) {
    TCell* NewBuffer = new TCell[X * Y];
    int MX = (X < ScreenCols) ? X : ScreenCols;
    int MY = (Y < ScreenRows) ? Y : ScreenRows;

    for (int i = 0; i < MY; i++)
        memcpy(NewBuffer + X * i, CursorXYPos(0, i), MX * sizeof(TCell));

    delete[] ScreenBuffer;
    ScreenBuffer = NewBuffer;
    ScreenCols = X;
    ScreenRows = Y;
    //ConPutBox(0, 0, ScreenCols, ScreenRows, (PCell) ScreenBuffer);
    //if (Refresh == 0)
    //    XResizeWindow(display, win, ScreenCols * FontCX, ScreenRows * FontCY);
    return 1;
}

int ConQuerySize(int *X, int *Y) {
    *X = ScreenCols;
    *Y = ScreenRows;

    return 1;
}

int ConSetCursorPos(int X, int Y) {
    DrawCursor(0);
    CursorX = X;
    CursorY = Y;
    DrawCursor(1);

    return 1;
}

int ConQueryCursorPos(int *X, int *Y) {
    *X = CursorX;
    *Y = CursorY;

    return 1;
}

int ConShowCursor() {
    CursorVisible = 1;
    DrawCursor(1);

    return 1;
}

int ConHideCursor() {
    DrawCursor(0);
    CursorVisible = 0;

    return 1;
}

int ConCursorVisible() {
    return CursorVisible;
}
int ConSetCursorSize(int Start, int End) {
    CursorStart = Start;
    CursorEnd = End;
    DrawCursor(CursorVisible);

    return 1;
}

int ConSetMousePos(int /*X*/, int /*Y*/) {
    return 1;
}

static int LastMouseX = -1, LastMouseY = -1;

int ConQueryMousePos(int *X, int *Y) {
    if (X) *X = LastMouseX;
    if (Y) *Y = LastMouseY;

    return 1;
}

int ConShowMouse() {
    printf("Show\n");
    return 1;
}

int ConHideMouse() {
    printf("Hide\n");
    return 1;
}

int ConMouseVisible() {
    return 1;
}

int ConQueryMouseButtons(int *ButtonCount) {
    *ButtonCount = 3;

    return 1;
}

#if 0
static void UpdateWindow1(Region reg) {
    Refresh = 1;
    colorXGC->SetRegion(reg);

    //XRectangle rect;
    //XClipBox(reg, &rect);

    //ConPutBox(rect.x, rect.y, rect.width/FontCX + 1, rect.height/FontCY + 1,
    //          (PCell) CursorXYPos(rect.x, rect.y));
    ConPutBox(0, 0, ScreenCols, ScreenRows, CursorXYPos(0, 0));
    colorXGC->SetRegion(0);
    Refresh = 0;
}
#endif

static void UpdateWindow(int xx, int yy, int ww, int hh) {
    if (xx + ww > ScreenCols)
        ww = ScreenCols - xx;

    if (yy + hh > ScreenRows)
        hh = ScreenRows - yy;

    //DebugShowArea(xx, yy, ww, hh, 14);

    Refresh = 1;
    for (int i = 0; i < hh; i++)
        ConPutBox(xx, yy + i, ww, 1, CursorXYPos(xx, yy + i));
    Refresh = 0;
}

static void ResizeWindow(int ww, int hh) {
    int ox = ScreenCols;
    int oy = ScreenRows;
    ww /= FontCX; if (ww < 4) ww = 4;
    hh /= FontCY; if (hh < 2) hh = 2;
    if (ScreenCols != ww || ScreenRows != hh) {
        Refresh = 0;
        ConSetSize(ww, hh);
        Refresh = 1;
#if 1
        if (ox < ScreenCols)
            UpdateWindow(ox, 0, (ScreenCols - ox), ScreenRows);
        if (oy < ScreenRows)
            UpdateWindow(0, oy, ScreenCols, (ScreenRows - oy));
#endif
        //UpdateWindow(0, 0, ScreenCols, ScreenRows);
        Refresh = 0;
    }
}

static const struct {
    int keysym;
    int keycode;
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
    { XK_Delete,         kbDel },
    { XK_Insert,         kbIns },
    { XK_KP_Delete,      kbDel | kfGray },
    { XK_KP_Insert,      kbIns | kfGray },
    { XK_KP_Enter,       kbEnter | kfGray },
    { XK_KP_Add,         '+' | kfGray },
    { XK_KP_Subtract,    '-' | kfGray },
    { XK_KP_Multiply,    '*' | kfGray },
    { XK_KP_Divide,      '/' | kfGray },
    { XK_KP_Begin,       kbPgUp | kfGray | kfCtrl },
    { XK_KP_Home,        kbHome | kfGray },
    { XK_KP_Up,          kbUp | kfGray },
    { XK_KP_Prior,       kbPgUp | kfGray },
    { XK_KP_Left,        kbLeft | kfGray },
    { XK_KP_Right,       kbRight | kfGray },
    { XK_KP_End,         kbEnd | kfGray },
    { XK_KP_Down,        kbDown | kfGray },
    { XK_KP_Next,        kbPgDn| kfGray },
    { XF86XK_Back,       kbLeft | kfAlt },
    { XF86XK_Forward,    kbRight | kfAlt },
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
    // HP keysyms
    { XK_ClearLine,      kbDel | kfShift | kfGray },
    { XK_InsertLine,     kbIns | kfCtrl | kfGray },
    { XK_DeleteLine,     kbIns | kfShift | kfGray },
    { XK_InsertChar,     kbIns | kfGray },
    { XK_DeleteChar,     kbDel | kfGray },
    { XK_BackTab,        kbTab | kfShift },
    { XK_KP_BackTab,     kbTab | kfShift },
    { 0,                 0 }
};

static void ConvertKeyToEvent(KeySym key, KeySym key1, char */*keyname*/, char */*keyname1*/, int etype, int state, TEvent *Event) {
    unsigned int myState = 0;

    switch (etype) {
    case KeyPress:   Event->What = evKeyDown; break;
    case KeyRelease: Event->What = evKeyUp; break;
    default:         Event->What = evNone; return;
    }

    if (state & ShiftMask) myState |= kfShift;
    if (state & ControlMask) myState |= kfCtrl;
    //if (state & Mod2Mask) myState |= kfAlt; // NumLock
    if (state & (Mod1Mask | Mod3Mask | Mod4Mask | Mod5Mask)) myState |= kfAlt;

    /* modified kabi@users.sf.net
     * for old method
     * if (!KeyAnalyze((etype == KeyPress), state, &key, &key1))
     *     return;
     */

    //printf("key: %d ; %d ; %d\n", (int)key, (int)key1, state);
    if (key < 256 || (key1 < 256 && (myState == kfAlt || myState == (kfAlt | kfShift)))) {
        if (myState & kfAlt)
            key = key1;
        if (myState == kfShift)
            myState = 0;
        if (myState & (kfAlt | kfCtrl))
            if ((key >= 'a') && (key <= 'z'))
                key += ('A' - 'a');
        if ((myState & kfCtrl) && key < 32)
            key += 64;
        Event->Key.Code = key | myState;
        return;
    } else {
        for (size_t i = 0; i < FTE_ARRAY_SIZE(key_table); ++i) {
            long k;

            if ((int) key1 == key_table[i].keysym) {
                k = key_table[i].keycode;
                if (k < 256)
                    if (myState == kfShift)
                        myState = 0;
                Event->Key.Code = k | myState;
                return;
            }
        }
    }
    //printf("Unknown key: %ld %s %d %d\n", key, keyname, etype, state);
    Event->What = evNone;
}

static TEvent LastMouseEvent = { evNone };

#define TM_DIFF(x,y) ((long)(((long)(x) < (long)(y)) ? ((long)(y) - (long)(x)) : ((long)(x) - (long)(y))))

static void ConvertClickToEvent(int type, int xx, int yy, int button, int state,
                                TEvent *Event, Time mtime) {
    static unsigned long LastClickTime = 0;
    static short LastClickCount = 0;
    static unsigned long LastClick = 0;
    unsigned int myState = 0;
    unsigned long CurTime = mtime;

    //printf("Mouse x:%d y:%d  %d\n", xx, yy, type);
    if (type == MotionNotify) Event->What = evMouseMove;
    else if (type == ButtonPress) Event->What = evMouseDown;
    else Event->What = evMouseUp;
    Event->Mouse.X = xx / FontCX;
    Event->Mouse.Y = yy / FontCY;
    if (Event->What == evMouseMove)
        if (LastMouseX == Event->Mouse.X
            && LastMouseY == Event->Mouse.Y) {
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
        case Button4:
        case Button5:
            if (type == ButtonPress) {
                Event->What = evCommand;
                Event->Msg.Param1 = (state & ShiftMask) ? 1 : 3;
                // fix core to use count
                Event->Msg.Command =
                    (button == Button4) ? cmVScrollUp : cmVScrollDown;
            }
            return;
        }
    }
    Event->Mouse.Count = 1;
    if (state & ShiftMask) myState |= kfShift;
    if (state & ControlMask) myState |= kfCtrl;
    if (state & (Mod1Mask | Mod3Mask | Mod4Mask | Mod5Mask)) myState |= kfAlt;
    //if (state & Mod2Mask) myState |= kfAlt;
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
    LastMouseEvent = *Event;
}

static void ProcessXEvents(TEvent *Event) {
    XEvent event;

    Event->What = evNone;

    XNextEvent(display, &event);
#if 0
    // debug - print event name
    static const char * const event_names[] = {
        "",
        "",
        "KeyPress",
        "KeyRelease",
        "ButtonPress",
        "ButtonRelease",
        "MotionNotify",
        "EnterNotify",
        "LeaveNotify",
        "FocusIn",
        "FocusOut",
        "KeymapNotify",
        "Expose",
        "GraphicsExpose",
        "NoExpose",
        "VisibilityNotify",
        "CreateNotify",
        "DestroyNotify",
        "UnmapNotify",
        "MapNotify",
        "MapRequest",
        "ReparentNotify",
        "ConfigureNotify",
        "ConfigureRequest",
        "GravityNotify",
        "ResizeRequest",
        "CirculateNotify",
        "CirculateRequest",
        "PropertyNotify",
        "SelectionClear",
        "SelectionRequest",
        "SelectionNotify",
        "ColormapNotify",
        "ClientMessage",
        "MappingNotify"
    };
    fprintf(stderr, "event  %d -  %s\n", event.type, event_names[event.type]);
#endif
    if (XFilterEvent(&event, None))
        return;

    if (event.xany.window != win) {
        if (event.type == PropertyNotify && event.xproperty.state == PropertyDelete) {
            // Property change on different window - try to find matching incremental selection request
            IncrementalSelectionInfo *isi, *prev_isi = NULL;

            for (isi = incrementalSelections; isi; prev_isi = isi, isi = isi->next) {
                if (isi->requestor == event.xproperty.window && isi->property == event.xproperty.atom) {
                    // Found selection request - send more data
                    int send = isi->len - isi->pos;

                    send = send < SELECTION_XFER_LIMIT ? send : SELECTION_XFER_LIMIT;
                    XChangeProperty(display, isi->requestor, isi->property, isi->type, 8, PropModeAppend, isi->data + isi->pos, send);
                    isi->pos += send;
                    isi->lastUse = time(NULL);

                    if (send == 0) {
                        // Was sent - remove from memory
                        if (prev_isi)
                            prev_isi->next = isi->next;
                        else
                            incrementalSelections = isi->next;
                        XFree(isi->data);
                        delete isi;
                    }
                    break;
                }
            }
        }
        return;
    }

    switch (event.type) {
    case MappingNotify:
        XRefreshKeyboardMapping(&event.xmapping);
        break;
    case Expose:
    case GraphicsExpose:
        {
            XRectangle rect;
#if 0
#define MAXREGS 5
            // idea here is to create limited set of intersection free bounding boxes
            // though it would need some more thing about combining them together smartly
            Region region[MAXREGS];
            memset(region, 0, sizeof(region));
            //state = XEventsQueued(display, QueuedAfterReading); fprintf(stderr, "Events Expose %d\n", state);
            do {
                rect.x = (short) event.xexpose.x / FontCX;
                rect.y = (short) event.xexpose.y / FontCY;
                rect.width = (short) event.xexpose.width / FontCX + 1;
                rect.height= (short) event.xexpose.height / FontCY + 1;

                fprintf(stderr, "Region   %hd %hd %hd %hd\n",
                        rect.x, rect.y, rect.width, rect.height);

                for (int i = 0; i < MAXREGS; ++i)
                    if (!region[i] || (i == (MAXREGS - 1))
                        || XRectInRegion(region[i], rect.x, rect.y,
                                         rect.width, rect.height)) {
                        if (!region[i])
                            region[i] = XCreateRegion();
                        XUnionRectWithRegion(&rect, region[i], region[i]);
                        fprintf(stderr, "-> region %d\n", i);
                        break;
                    }

            } while (XCheckTypedWindowEvent(display, win, event.type, &event));

            for (int j = 0; region[j] && j < MAXREGS; ++j) {
                XClipBox(region[j], &rect);
                XDestroyRegion(region[j]);
                UpdateWindow(rect.x, rect.y, rect.width + 1, rect.height + 1);
            }
#else
            Region region = XCreateRegion();
            //state = XEventsQueued(display, QueuedAfterReading); fprintf(stderr, "Events Expose %d\n", state);
            //XFlush(display);
            //XSync(display, 0);
            //int cnt = 0;
            const int maxx = ScreenCols * FontCX;
            const int maxy = ScreenRows * FontCY;
            do {
                if (event.xexpose.x < maxx && event.xexpose.y < maxy) {
                    rect.x = (short) event.xexpose.x;
                    rect.y = (short) event.xexpose.y;
                    rect.width = (short) event.xexpose.width;
                    rect.height= (short) event.xexpose.height;
                    XUnionRectWithRegion(&rect, region, region);
                }
                //fprintf(stderr, "EXPOSE %d:%d   x:%3d y:%3d  w:%3d h:%3d\n", cnt++, event.xexpose.count,
                //	event.xexpose.x / FontCX, event.xexpose.y / FontCY,
                //	event.xexpose.width / FontCX + 1, event.xexpose.height / FontCY + 1);
            } while (XCheckTypedWindowEvent(display, win, event.type, &event));

            // get clipping bounding box for all Exposed areas
            // this seems to be much faster the using clipped regions for drawing
            XClipBox(region, &rect);
            XDestroyRegion(region);
            UpdateWindow(rect.x / FontCX, rect.y / FontCY,
                         rect.width / FontCX + 2, rect.height / FontCY + 2);
            //UpdateWindow1(region);
#endif
        }
        break;
    case ConfigureNotify:
        while (XCheckTypedWindowEvent(display, win, event.type, &event))
            XSync(display, 0); // wait for final resize
        ResizeWindow(event.xconfigure.width, event.xconfigure.height);
        Event->What = evCommand;
        Event->Msg.Command = cmResize;
        break;
    case ButtonPress:
    case ButtonRelease:
        now = event.xbutton.time;
        ConvertClickToEvent(event.type, event.xbutton.x, event.xbutton.y,
                            event.xbutton.button, event.xbutton.state,
                            Event, event.xmotion.time);
        break;
    case FocusIn:
        if (i18n_ctx) i18n_focus_in(i18n_ctx);
        break;
    case FocusOut:
        if (i18n_ctx) i18n_focus_out(i18n_ctx);
        break;
    case KeyPress:
        // case KeyRelease:
        {
            char keyName[32];
            char keyName1[32];
            KeySym key, key1;
            XEvent event1 = event;
            event1.xkey.state &= ~(ShiftMask | ControlMask | Mod1Mask /* | Mod2Mask*/ | Mod3Mask | Mod4Mask | Mod5Mask);
            now = event.xkey.time;

            if (!i18n_ctx || event.type == KeyRelease) {
                XLookupString(&event.xkey, keyName, sizeof(keyName), &key, 0);
            } else {
                i18n_lookup_sym(i18n_ctx, &event.xkey, keyName, sizeof(keyName), &key);
                if (!key)
                    break;
            }
            XLookupString(&event1.xkey, keyName1, sizeof(keyName1), &key1, 0);
            //fprintf(stderr, "event.state = %d %s %08X\n", event.xkey.state, keyName, (int)key);
            //fprintf(stderr, "keyev.state = %d %s %08X\n", event1.xkey.state, keyName1, (int)key1);
            //key1 = XLookupKeysym(&event1.xkey, 0);
            ConvertKeyToEvent(key, key1, keyName, keyName1,
                              event.type, event.xkey.state, Event);
        }
        break;
    case MotionNotify:
        now = event.xmotion.time;
        ConvertClickToEvent(event.type, event.xmotion.x, event.xmotion.y,
                            0, event.xmotion.state, Event, event.xmotion.time);
        break;
    case ClientMessage:
        if (event.xclient.message_type == wm_protocols
            && event.xclient.format == 32
            && (Atom)event.xclient.data.l[0] == wm_delete_window)
        {
            Event->What = evCommand;
            Event->Msg.Command = cmClose;
        }
        break;
    case SelectionClear:
        {
            int clip = GetFTEClip(event.xselectionclear.selection);
            if (clip >= 0) {
                Window owner = XGetSelectionOwner(display, GetXClip(clip));
                if (owner != win) {
                    if (CurSelectionData[clip] != NULL)
                        free(CurSelectionData[clip]);
                    CurSelectionData[clip] = NULL;
                    CurSelectionLen[clip] = 0;
                    CurSelectionOwn[clip] = 0;
                }
            }
        }
        break;
    case SelectionRequest:
        {
            // SelectionRequest:
            //   owner     - selection owner (should be fte window)
            //   selection - selection (clipboard)
            //   target    - target type to which the selection should be converted
            //   property  - target property - place data to this property of requestor's window, set correct type
            //   requestor - selection requestor
            //   time      - request time - owner should provide selection if it owned it at this time
            //  Note: Old clients use None property - in this case the right property is stored in target field.
            //  On error refuse request by sending notification with None property.
            //
            // SelectionNotify:
            //   requestor -
            //   selection - should be copied from request
            //   target    - -- copy --
            //   property  - -- copy -- or None if on error - request could not be fulfilled
            //   time      - -- copy --

            static unsigned char empty[] = "";
            XEvent notify;
            Bool notifySent = False;
            int clip = GetFTEClip(event.xselectionrequest.selection);

            notify.type = SelectionNotify;
            notify.xselection.requestor = event.xselectionrequest.requestor;
            notify.xselection.selection = event.xselectionrequest.selection;
            notify.xselection.target = event.xselectionrequest.target;
            notify.xselection.time = event.xselectionrequest.time;
            // Prefill for "unknown/bad request" case
            notify.xselection.property = None;

            if (clip >= 0) {
                if (event.xselectionrequest.target == proptype_targets) {
                    // Return targets - to what types data can be rendered
                    Atom type_list[] = {
                        XA_STRING,
                        proptype_text
#ifdef CONFIG_X11_XMB
                        , proptype_compound_text
#ifdef X_HAVE_UTF8_STRING
                        , proptype_utf8_string
#endif
#endif
                    };

                    XChangeProperty(display,
                                    event.xselectionrequest.requestor,
                                    event.xselectionrequest.property,
                                    XA_ATOM,
                                    32, PropModeReplace,
                                    (unsigned char *)&type_list,
                                    FTE_ARRAY_SIZE(type_list));
                    notify.xselection.property = event.xselectionrequest.property;
#ifdef CONFIG_X11_XMB
                } else if (event.xselectionrequest.target == XA_STRING) {
#else
                } else if (event.xselectionrequest.target == XA_STRING || event.xselectionrequest.target == proptype_text) {
#endif
                    // No conversion, just the string we have (in fact we should convert to ISO Latin-1)
                    SendSelection(&notify, event.xselectionrequest.property, XA_STRING,
                                  (CurSelectionData[clip] ? CurSelectionData[clip] : empty), CurSelectionLen[clip], False);
                    notifySent = True;
#ifdef CONFIG_X11_XMB
                } else {
                    // Convert to requested type
                    XTextProperty text_property;
                    char *text_list[1] = {(char *)(CurSelectionData[clip] ? CurSelectionData[clip] : empty)};

                    int style =
                        event.xselectionrequest.target == XA_STRING ? XStringStyle :
                        event.xselectionrequest.target == proptype_text ? XStdICCTextStyle :
                        event.xselectionrequest.target == proptype_compound_text ? XCompoundTextStyle :
#ifdef X_HAVE_UTF8_STRING
                        event.xselectionrequest.target == proptype_utf8_string ? XUTF8StringStyle :
#endif
                        -1;

                    if (style != -1) {
                        // Can convert
                        if (XmbTextListToTextProperty(display, text_list, 1, (XICCEncodingStyle) style,
                                                      &text_property) == Success) {
                            if (text_property.format == 8) {
                                // SendSelection supports only 8-bit data (should be always, just safety check)
                                SendSelection(&notify, event.xselectionrequest.property, text_property.encoding,
                                              text_property.value, text_property.nitems, True);
                                notifySent = True;
                            } else {
                                // Bad format - just cleanup
                                XFree(text_property.value);
                            }
                        }
                    }
#endif
                }
            }

            if (!notifySent) XSendEvent(display, notify.xselection.requestor, False, 0L, &notify);

            // Now clean too old incremental selections
            IncrementalSelectionInfo *isi = incrementalSelections, *prev_isi = NULL;
            time_t tnow = time(NULL);

            while (isi) {
                if (isi->lastUse + SELECTION_MAX_AGE < tnow) {
                    IncrementalSelectionInfo *next_isi = isi->next;
                    if (prev_isi) prev_isi->next = isi->next; else incrementalSelections = isi->next;
                    XFree(isi->data);
                    delete isi;
                    isi = next_isi;
                } else {
                    prev_isi = isi;
                    isi = isi->next;
                }
            }
        }
        break;
    }
}

static void FlashCursor ()
{
    struct timeval tv;
    unsigned long OldTime = CursorLastTime;

    if (!CursorBlink || gettimeofday(&tv, NULL) != 0)
        return;

    CursorLastTime = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    if (OldTime / CursorFlashInterval != CursorLastTime / CursorFlashInterval)
        DrawCursor(CursorVisible);
}

static TEvent Pending = { evNone };

int ConGetEvent(TEventMask EventMask, TEvent *Event, int WaitTime, int Delete) {
    static TEvent Queued = { evNone };
    int rc;

    FlashCursor();

    Event->What = evNone;
    if (Queued.What != evNone) {
        *Event = Queued;

        if (Delete)
            Queued.What = evNone;

        if (Event->What & EventMask)
            return 1;

        Queued.What = evNone;
    }

    Event->What = evNone;
    if (Pending.What != evNone) {
        *Event = Pending;

        if (Delete)
            Pending.What = evNone;

        if (Event->What & EventMask)
            return 1;

        Pending.What = evNone;
    }

    // We can't sleep for too much since we have to flash the cursor
    if (CursorBlink
        && ((WaitTime == -1) || (WaitTime > (int)CursorFlashInterval)))
        WaitTime = CursorFlashInterval;

    Event->What = evNone;

    while (Event->What == evNone) {
        while (XPending(display) > 0) {
            FlashCursor();
            ProcessXEvents(Event);
            if (Event->What != evNone) {
                while ((Event->What == evMouseMove) && (Queued.What == evNone)) {
                    while ((rc = XPending(display)) > 0) {
                        ProcessXEvents(&Queued);
                        if (Queued.What != evMouseMove)
                            break;
                        *Event = Queued;
                        Queued.What = evNone;
                    }
                    if (rc <= 0)
                        break;
                }
            }
            if (!Delete)
                Pending = *Event;
            if (Event->What & EventMask)
                return 1;

            Pending.What = evNone;
            Event->What = evNone;
        }

        if ((WaitTime == -1 || WaitTime > (int)MouseAutoDelay)
            && (LastMouseEvent.What == evMouseAuto) && (EventMask & evMouse)) {

            if ((rc = WaitFdPipeEvent(Event, ConnectionNumber(display), -1,
                                      MouseAutoDelay)) == FD_PIPE_TIMEOUT) {
                *Event = LastMouseEvent;
                return 1;
            }
        } else if ((WaitTime == -1 || WaitTime > (int)MouseAutoRepeat)
                   && (LastMouseEvent.What == evMouseDown || LastMouseEvent.What == evMouseMove)
                   && (LastMouseEvent.Mouse.Buttons) && (EventMask & evMouse)) {

            if ((rc = WaitFdPipeEvent(Event, ConnectionNumber(display), -1,
                                      MouseAutoRepeat)) == FD_PIPE_TIMEOUT) {
                LastMouseEvent.What = evMouseAuto;
                *Event = LastMouseEvent;
                return 1;
            }
        } else
            rc = WaitFdPipeEvent(Event, ConnectionNumber(display), -1,
                                 (WaitTime < 1000) ? WaitTime : 1001);

        if (rc == FD_PIPE_TIMEOUT || rc == FD_PIPE_ERROR)
            return 0;
        // pipe event has evNotify
    }

    return 1;
}

int ConPutEvent(const TEvent& Event) {
    Pending = Event;

    return 1;
}

int ConFlush() {
    XFlush(display);

    return 1;
}

int ConGrabEvents(TEventMask /*EventMask*/) {
    return 1;
}

static int WaitForXEvent(int eventType, XEvent *event) {
    time_t time_started = time(NULL);
    while (!XCheckTypedWindowEvent(display, win, eventType, event)) {
        usleep(1000);
        time_t tnow = time(NULL);

        if (time_started > tnow)
            time_started = tnow;

        if (tnow - time_started > 5)
            return 0;
    }

    return 1;
}

static void SendSelection(XEvent *notify, Atom property, Atom type, unsigned char *data, size_t len, Bool privateData) {
    int (*oldHandler)(Display *, XErrorEvent *);
    size_t i, send;

    // Install error handler
    oldHandler = XSetErrorHandler(ErrorHandler);
    gotXError = False;

    if (len < SELECTION_INCR_LIMIT) {
        // Send fully - set property by appending smaller chunks
        for (i = 0; !gotXError && i < len; i += SELECTION_XFER_LIMIT) {
            send = len - i;
            send = send < SELECTION_XFER_LIMIT ? send : SELECTION_XFER_LIMIT;
            XChangeProperty(display, notify->xselection.requestor, property,
                            type, 8, PropModeReplace, data + i, (int)send);
        }
        if (!gotXError) notify->xselection.property = property;
        XSendEvent(display, notify->xselection.requestor, False, 0L, notify);
    } else {
        // Send incrementally
        IncrementalSelectionInfo *isi = new IncrementalSelectionInfo;

        isi->next = incrementalSelections;
        isi->len = (int)len;
        isi->pos = 0;
        isi->requestor = notify->xselection.requestor;
        isi->property = property;
        isi->type = type;
        isi->lastUse = time(NULL);
        if (privateData) {
            // Private data - use directly
            isi->data = data;
            // Mark data non-private so XFree() at the end won't remove it
            privateData = False;
        } else {
            // Non-private data - need to make copy
            isi->data = (unsigned char *)malloc(len);
            if (isi->data != NULL) memcpy(isi->data, data, len);
        }
        if (isi->data != NULL) {
            // Data ready - put to list and send response
            incrementalSelections = isi;

            // Request receiving requestor's property changes
            XSelectInput(display, notify->xselection.requestor, PropertyChangeMask);

            // Send total size
            XChangeProperty(display, notify->xselection.requestor, property,
                            proptype_incr, 32, PropModeReplace, (unsigned char *)&len, 1);

            notify->xselection.property = property;
        }
        // Send also in case of error - with None property
        XSendEvent(display, notify->xselection.requestor, False, 0L, notify);
    }

    // Restore error handler
    XSetErrorHandler(oldHandler);

    // Cleanup
    if (privateData)
        XFree(data);
}

static int ConvertSelection(Atom selection, Atom type, int *len, char **data) {
    XEvent event;
    Atom actual_type;
    int actual_format, retval;
    unsigned long nitems, bytes_after;
    unsigned char *d;

    // Make sure property does not exist
    XDeleteProperty(display, win, prop_selection);

    // Request clipboard data
    XConvertSelection(display, selection, type, prop_selection, win, now);

    // Wait for SelectionNotify
    if (!WaitForXEvent(SelectionNotify, &event) || event.xselection.property != prop_selection)
        return 0;

    // Consume event sent when property was set by selection owner
    WaitForXEvent(PropertyNotify, &event);

    // Check the value - size, type etc.
    retval = XGetWindowProperty(display, win, prop_selection, 0, 0, False, AnyPropertyType,
                                &actual_type, &actual_format, &nitems, &bytes_after, &d);
    XFree(d);
    if (retval != Success)
        return 0;

    if (actual_type == proptype_incr) {
        // Incremental data
        size_t pos, buffer_len;
        unsigned char *buffer;

        // Get selection length and allocate buffer
        XGetWindowProperty(display, win, prop_selection, 0, 8, True, proptype_incr,
                           &actual_type, &actual_format, &nitems, &bytes_after, &d);
        buffer_len = *(int *)d;
        buffer = (unsigned char *)malloc(buffer_len);
        XFree(d);
        // Cannot exit right now if data == NULL since we need to complete the handshake

        // Now read data
        pos = 0;
        while (1) {
            // Wait for new value notification
            do {
                if (!WaitForXEvent(PropertyNotify, &event)) {
                    free(buffer);
                    return 0;
                }
            } while (event.xproperty.state != PropertyNewValue);

            // Get value size
            XGetWindowProperty(display, win, prop_selection, 0, 0, False, type,
                               &actual_type, &actual_format, &nitems, &bytes_after, &d);
            XFree(d);
            // Get value and delete property
            XGetWindowProperty(display, win, prop_selection, 0, nitems + bytes_after, True, type,
                               &actual_type, &actual_format, &nitems, &bytes_after, &d);

            if (nitems && buffer) {
                // Data received and have buffer
                if (nitems > (unsigned int)(buffer_len - pos)) {
                    // More data than expected - realloc buffer
                    size_t new_len = pos + nitems;
                    unsigned char *new_buffer = (unsigned char *)malloc(new_len);
                    if (new_buffer) memcpy(new_buffer, buffer, buffer_len);
                    free(buffer);
                    buffer = new_buffer;
                    buffer_len = new_len;
                }
                if (buffer) memcpy(buffer + pos, d, nitems);
                pos += nitems;
            }
            XFree(d);
            if (nitems == 0) {
                // No more data - done
                if (!buffer)
                    // No buffer - failed
                    return 0;

                // Buffer OK - exit loop and continue to data conversion
                nitems = pos;
                d = buffer;
                break;
            }
        }
    } else
        // Obtain the data from property
        if (XGetWindowProperty(display, win, prop_selection, 0, nitems + bytes_after,
                               True, type, &actual_type, &actual_format, &nitems,
                               &bytes_after, &d) != Success)
            return 0;

    // Now convert data to char string (uses nitems and d)
    if (actual_type == XA_STRING) {
        // String - propagate directly out of this function
        // This propagation is not safe since it expects XFree() to be the same as free().
        // Rather we should make a copy of the received data. The similar applies to data
        // propagated from INCR branch above - they are allocated by malloc() but get freed
        // by XFree() after Xmb conversion below.
        *data = (char *)d;
        *len = (int)nitems;
    } else {
#ifdef CONFIG_X11_XMB
        // Convert data to char * string
        XTextProperty text;
        char **list;
        int list_count;

        text.value = d;
        text.encoding = actual_type;
        text.format = actual_format;
        text.nitems = nitems;

        *data = NULL; // NULL indicates failure
        retval = XmbTextPropertyToTextList(display, &text, &list, &list_count);
        XFree(d);
        if (retval >= 0) {
            // Conversion OK - now we'll concat all the strings together
            int i;

            // Get total length first
            *len = 0;
            for (i = 0; i < list_count; i++) {
                *len += (int)strlen(list[i]);
            }
            // Allocate
            *data = (char *)malloc(*len + 1);
            if (*data != NULL) {
                // Concat strings
                char *s = *data;
                for (i = 0; i < list_count; i++) {
                    strcpy(s, list[i]);
                    s += strlen(s);
                }
            }
            // Cleanup
            XFreeStringList(list);
        }
        if (!*data)
            return 0; // failed
#else
        return 0;
#endif
    }
    // OK
    return 1;
}

int GetXSelection(int *len, char **data, int clipboard) {
    if (CurSelectionOwn[clipboard]) {
        if (!(*data = (char *) malloc(CurSelectionLen[clipboard])))
            return 0;
        memcpy(*data, CurSelectionData[clipboard], CurSelectionLen[clipboard]);
        *len = CurSelectionLen[clipboard];
        return 1;
    }

    Atom clip = GetXClip(clipboard);
    if (XGetSelectionOwner(display, clip) != None) {
        // Get data - try various formats
#ifdef CONFIG_X11_XMB
#ifdef X_HAVE_UTF8_STRING
        if (ConvertSelection(clip, proptype_utf8_string, len, data))
            return 1;
#endif
        if (ConvertSelection(clip, proptype_compound_text, len, data))
            return 1;
#endif
        return ConvertSelection(clip, XA_STRING, len, data);
    }
    *data = XFetchBytes(display, len);

    return (*data != NULL);
}

int SetXSelection(int len, char *data, int clipboard) {
    Atom clip = GetXClip(clipboard);

    if (CurSelectionData[clipboard])
        free(CurSelectionData[clipboard]);

    // We need CurSelectionData zero-terminated so XmbTextListToTextProperty can work
    CurSelectionData[clipboard] = (unsigned char *)malloc(len + 1);

    if (!CurSelectionData[clipboard]) {
        CurSelectionLen[clipboard] = 0;
        return 0;
    }

    CurSelectionLen[clipboard] = len;
    memcpy(CurSelectionData[clipboard], data, CurSelectionLen[clipboard]);
    CurSelectionData[clipboard][len] = 0;

    if (CurSelectionLen[clipboard] < 64 * 1024)
        XStoreBytes(display, data, len);

    XSetSelectionOwner(display, clip, win, CurrentTime);

    if (XGetSelectionOwner(display, clip) == win)
        CurSelectionOwn[clipboard] = 1;

    return 1;
}

GUI::GUI(int &argc, char **argv, int XSize, int YSize) {
    int o = 1;

    for (int c = 1; c < argc; c++) {
        if (strcmp(argv[c], "-font") == 0) {
            if (c + 1 < argc)
                strlcpy(WindowFont, argv[++c], 63);// ugly
        } else if (strcmp(argv[c], "-geometry") == 0) {
            if (c + 1 < argc) {

                XParseGeometry(argv[++c], &initX, &initY,
                               (unsigned*) &ScreenCols,
                               (unsigned*) &ScreenRows);
                if (ScreenCols > MAX_SCRWIDTH)
                    ScreenCols = MAX_SCRWIDTH;
                else if (ScreenCols < MIN_SCRWIDTH)
                    ScreenCols = MIN_SCRWIDTH;
                if (ScreenRows > MAX_SCRHEIGHT)
                    ScreenRows = MAX_SCRHEIGHT;
                else if (ScreenRows < MIN_SCRHEIGHT)
                    ScreenRows = MIN_SCRHEIGHT;
                setUserPosition = 1;
            }
        } else if ((strcmp(argv[c], "-noxmb") == 0)
                   || (strcmp(argv[c], "--noxmb") == 0))
            useXMB = 0;
        else if ((strcmp(argv[c], "-noi18n") == 0)
                 || (strcmp(argv[c], "--noi18n") == 0))
            useI18n = 0;
        else if (strcmp(argv[c], "-name") == 0) {
            if (c + 1 < argc)
                strlcpy(res_name, argv [++c], sizeof(res_name));
        } else
            argv[o++] = argv[c];
    }
    argc = o;
    argv[argc] = 0;

    if (::ConInit(XSize, YSize) && SetupXWindow(argc, argv))
        gui = this;
    else
        gui = NULL;
    fArgc = argc;
    fArgv = argv;
}

GUI::~GUI() {
    i18n_destroy(&i18n_ctx);
    delete colorXGC;
    colorXGC = 0;
#ifdef CONFIG_X11_XMB
    if (font_set)
        XFreeFontSet(display, font_set);
#endif
    if (font_struct)
        XFreeFont(display, font_struct);
    XDestroyWindow(display, win);
    XCloseDisplay(display);

    for (int i=0; i<3; i++) {
        if (CurSelectionData[i] != NULL) {
            free(CurSelectionData[i]);
        }
    }

    ::ConDone();
}

int GUI::ConSuspend() {
    return ::ConSuspend();
}

int GUI::ConContinue() {
    return ::ConContinue();
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

char ConGetDrawChar(unsigned int idx) {
    static const char *tab = NULL;
    static size_t len = 0;

    if (!tab) {
        tab = GetGUICharacters("X11","\x0D\x0C\x0E\x0B\x12\x19\x18\x15\x16\x17\x0f>\x1F\x01\x12\x01\x01 \x02\x01\x01");
        len = strlen(tab);
    }
    assert(idx < len);

    return tab[idx];
}
