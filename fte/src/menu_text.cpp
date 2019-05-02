/*    menu_text.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "c_color.h"
#include "c_mode.h"
#include "gui.h"
#include "sysdep.h"

#include <ctype.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>

class UpMenu {
public:
    class UpMenu *up;
    int id;
    int vert;
    int x, y, w, h;
};

static int GetHOfsItem(int id, int cur) {
    int pos = 2;
    
    for (unsigned i = 0; i < Menus[id].Count; i++) {
        if ((int)i == cur) return pos;
        if (Menus[id].Items[i].Name)
            pos += (int)CStrLen(Menus[id].Items[i].Name) + 2;
        else
            pos++;
    }
    return -1;
}

static int GetHPosItem(int id, int X) {
    int pos = 1;
    unsigned i;
    size_t len;
    
    for (i = 0; i < Menus[id].Count; i++) {
        if (Menus[id].Items[i].Name) {
            len = CStrLen(Menus[id].Items[i].Name);
            if (X >= pos && X <= pos + (int)len + 1) return i;
            pos += (int)len + 2;
        } else 
            pos++;
    }
    return -1;
}

static int DrawHMenu(int x, int y, int id, int active) {
    int pos = 1;
    TDrawBuffer B;
    unsigned i;
    size_t len;
    TAttr color1, color2;
    int Cols, Rows;
    
    ConQuerySize(&Cols, &Rows);
    
    MoveChar(B, 0, Cols, ' ', hcMenu_Background, Cols);
    if (id != -1) {
        for (i = 0; i < Menus[id].Count; i++) {
            if ((int)i == active) {
                color1 = hcMenu_ActiveItem;
                color2 = hcMenu_ActiveChar;
            } else {
                color1 = hcMenu_NormalItem;
                color2 = hcMenu_NormalChar;
            }

            if (Menus[id].Items[i].Name) {
                len = CStrLen(Menus[id].Items[i].Name);
                MoveChar(B, pos, Cols, ' ', color1, len + 2);
                MoveCStr(B, pos + 1, Cols, Menus[id].Items[i].Name, color1, color2, len);
                pos += (int)len + 2;
            } else {
                MoveChar(B, pos, Cols, ConGetDrawChar(DCH_V), hcMenu_Background, 1);
                pos++;
            }
        }
    }
    ConPutBox(x, y, Cols - x, 1, B);
    return 1;
}

static int GetVPosItem(int id, int w, int X, int Y) {
    if (Y <= 0 || Y > (int)Menus[id].Count) return -1;
    if (Menus[id].Items[Y - 1].Name == 0) return -1;
    if (X <= 0 || X >= w - 1) return -1;
    return Y - 1;
}

static int GetVSize(int id, int &X, int &Y) {
    size_t xsize = 0;
    size_t len;

    Y = Menus[id].Count;
    for (int i = 0; i < Y; i++) {
        len = 0;
        if (Menus[id].Items[i].Name)
            len = CStrLen(Menus[id].Items[i].Name);
        if (len > xsize)
            xsize = len;
    }
    X = (int)xsize;
    return 0;
}


static int DrawVMenu(int x, int y, int id, int active) {
    TDrawBuffer B;
    TAttr color1, color2;
    int w, h;
    
    if (id == -1) return -1;
    
    GetVSize(id, w, h);
    w += 4;
    h += 2;

    MoveChar(B, 0, w, ConGetDrawChar(DCH_H), hcMenu_Background, w);
    MoveCh(B, ConGetDrawChar(DCH_C1), hcMenu_Background, 1);
    MoveCh(B + w - 1, ConGetDrawChar(DCH_C2), hcMenu_Background, 1);
    ConPutBox(x, y, w, 1, B);
    for (unsigned i = 0; i < Menus[id].Count; ++i) {
        if ((int)i == active) {
            color1 = hcMenu_ActiveItem;
            color2 = hcMenu_ActiveChar;
        } else {
            color1 = hcMenu_NormalItem;
            color2 = hcMenu_NormalChar;
        }
        if (Menus[id].Items[i].Name) {
            char name[128];

            strcpy(name, Menus[id].Items[i].Name);
            char* arg = strchr(name, '\t');
            if (arg)
                *arg++ = 0;

            size_t len = CStrLen(name);
            size_t len2 = (arg) ? CStrLen(arg) : 0;

            MoveChar(B, 0, w, ' ', color1, w);
            MoveCh(B, ConGetDrawChar(DCH_V), hcMenu_Background, 1);
            MoveCh(B + w - 1, ConGetDrawChar(DCH_V), hcMenu_Background, 1);

            MoveCStr(B, 2, (int)len + 2, Menus[id].Items[i].Name, color1, color2, len);
            if (arg)
                MoveCStr(B, w - (int)len2 - 2, w + 4, arg, color1, color2, len2);

            if (Menus[id].Items[i].SubMenu != -1)
                MoveCh(B + w - 2, ConGetDrawChar(DCH_RPTR), color1, 1);
        } else {
            MoveChar(B, 0, w, ConGetDrawChar(DCH_H), hcMenu_Background, w);
            //for (int i = 0; i <= DCH_ARIGHT; i++) MoveCh(B + i, ConGetDrawChar(i), hcMenu_Background, 1);
            MoveCh(B, ConGetDrawChar(DCH_M2), hcMenu_Background, 1);
            MoveCh(B + w - 1, ConGetDrawChar(DCH_M3), hcMenu_Background, 1);
        }
        ConPutBox(x, y + i + 1, w, 1, B);
    }
    MoveChar(B, 0, w, ConGetDrawChar(DCH_H), hcMenu_Background, w);
    MoveCh(B, ConGetDrawChar(DCH_C3), hcMenu_Background, 1);
    MoveCh(B + w - 1, ConGetDrawChar(DCH_C4), hcMenu_Background, 1);
    ConPutBox(x, y + Menus[id].Count + 1, w, 1, B);
    return 1;
}

int ExecVertMenu(int x, int y, int id, TEvent &E, UpMenu *up) {
    int cur = 0;
    int abort;
    int w, h;
    PCell c;
    PCell SaveC = 0;
    int SaveX, SaveY, SaveW, SaveH;
    UpMenu here;
    int dovert = 0;
    int rx;
    int Cols, Rows;
    
    ConQuerySize(&Cols, &Rows);
    
    here.up = up;
    
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    GetVSize(id, w, h);
    w += 4;
    h += 2;
    
    if (w > Cols) w = Cols;
    if (h > Rows) h = Rows;
    
    if (x + w > Cols) {
        if (up && up->x == 0 && up->y == 0 && up->h == 1) {
            x = Cols - w;
        } else {
            if (up)
                x = up->x - w + 1;
            else
                x = x - w + 1;
        }
    }
    if (y + h > Rows) {
        if (up)
            y = y - h + 3;
        else {
            y = y - h + 1;
        }
    }
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    
    here.x = x;
    here.y = y;
    here.w = w;
    here.h = h;
    here.id = id;
    here.vert = 1;
    
    c = (PCell) malloc(w * h * sizeof(TCell));
    if (c)
        ConGetBox(x, y, w, h, c);
    
    SaveC = c;
    SaveX = x;
    SaveY = y;
    SaveW = w;
    SaveH = h;
    
#ifdef CONFIG_MOUSE
    int wasmouse = 0;
    if (E.What == evMouseMove || E.What == evMouseDown) {
    }
    if (E.What & evMouse) {
        cur = GetVPosItem(id, w, E.Mouse.X - x, E.Mouse.Y - y);
        dovert = 0;
        wasmouse = 1;
        E.What = evNone;
    }
#endif
    abort = -2;
    while (abort == -2) {
        DrawVMenu(x, y, id, cur);
        if (dovert) {
            if (cur != -1) {
                if (Menus[id].Items[cur].SubMenu != -1) {
                    rx = ExecVertMenu(x + w - 1, y + cur,
                                      Menus[id].Items[cur].SubMenu, E, &here);
                    if (rx == 1) {
                        abort = 1;
                        continue;
                    } else if (rx == -3) {
                        abort = -3;
                        break;
                    } else
                        abort = -2;

                }
            }
        }
        ConHideCursor();
        do {
            ConGetEvent(evCommand | evMouseDown | evMouseMove | evMouseUp | evKeyDown | evNotify, &E, -1, 1);
            if (E.What & evNotify)
                gui->DispatchEvent(frames, frames->Active, E);
        } while (E.What & evNotify);
        if (E.What & evMouse) {
            //fprintf(stderr, "Mouse: %d %d %d\n", E.What, E.Mouse.X, E.Mouse.Y);
        }
        dovert = 0;
        switch (E.What) {
        case evCommand:
            if (E.Msg.Command == cmResize) abort = -3;
            break;
        case evKeyDown:
            switch (kbCode(E.Key.Code)) {
            case kbPgDn:
            case kbEnd: cur = Menus[id].Count;
            case kbUp: 
                {
                    int xcur = cur;
                    
                    do {
                        cur--;
                        if (cur < 0) cur = Menus[id].Count - 1;
                    } while (cur != xcur && Menus[id].Items[cur].Name == 0);
                }
                break;
            case kbPgUp:
            case kbHome: cur = -1;
            case kbDown: 
                {
                    int xcur = cur;
                    do {
                        cur++;
                        if (cur >= Menus[id].Count) cur = 0;
                    } while (cur != xcur && Menus[id].Items[cur].Name == 0);
                }
                break;
            case kbEsc: abort = -1; break;
            case kbEnter:
                if (cur != -1) {
                    if (Menus[id].Items[cur].SubMenu < 0) {
                        E.What = evCommand;
                        E.Msg.View = frames->Active;
                        E.Msg.Command = Menus[id].Items[cur].Cmd;
                        abort = 1;
                    } else {
                        dovert = 1;
                    }
                }
                break;
            case kbLeft:
            case kbRight:
                gui->ConPutEvent(E);
                abort = -1;
                break;
            default:
                if (isAscii(E.Key.Code)) {
                    char cc;
                    int i;
                    
                    cc = char(toupper(char(E.Key.Code & 0xFF)));
                    
                    for (i = 0; i < Menus[id].Count; i++) {
                        if (Menus[id].Items[i].Name) {
                            char *o = strchr(Menus[id].Items[i].Name, '&');
                            if (o)
                                if (toupper(o[1]) == cc) {
                                    cur = i;
                                    if (cur != -1) {
                                        if (Menus[id].Items[cur].SubMenu == -1) {
                                            E.What = evCommand;
                                            E.Msg.View = frames->Active;
                                            E.Msg.Command = Menus[id].Items[cur].Cmd;
                                            abort = 1;
                                        } else {
                                            dovert = 1;
                                        }
                                    }
                                    break;
                                }
                        }
                    }
                }
            }
            break;
#ifdef CONFIG_MOUSE
        case evMouseDown:
            if (E.Mouse.X >= x && E.Mouse.Y >= y &&
                E.Mouse.X < x + w && E.Mouse.Y < y + h) 
            {
                cur = GetVPosItem(id, w, E.Mouse.X - x, E.Mouse.Y - y);
            } else {
                if (up) 
                    gui->ConPutEvent(E);
                abort = -1;
            }
            wasmouse = 1;
            dovert = 1;
            break;
        case evMouseMove:
            if (E.Mouse.Buttons)  {
                dovert = 1;
                if (E.Mouse.X >= x && E.Mouse.Y >= y &&
                    E.Mouse.X < x + w && E.Mouse.Y < y + h)
                {
                    cur = GetVPosItem(id, w, E.Mouse.X - x, E.Mouse.Y - y);
                } else {
                    UpMenu *p = up;
                    int first = 1;
                    
                    if (wasmouse) {
                        while (p) {
                            if (E.Mouse.X >= p->x && E.Mouse.Y >= p->y &&
                                E.Mouse.X < p->x + p->w && E.Mouse.Y < p->y + p->h)
                            {
                                if (first == 1) {
                                    if (p->vert) {
                                        int i = GetVPosItem(p->id, p->w, E.Mouse.X - p->x, E.Mouse.Y - p->y);
                                        if (i != -1)
                                            if (Menus[p->id].Items[i].SubMenu == id) break;
                                    } else {
                                        int i = GetHPosItem(p->id, E.Mouse.X);
                                        if (i != -1)
                                            if (Menus[p->id].Items[i].SubMenu == id) break;
                                    }
                                    first = 0;
                                }
                                gui->ConPutEvent(E);
                                abort = -1;
                                break;
                            }
                            first = 0;
                            p = p->up;
                        }
                        cur = -1;
                    } else
                        cur = -1;
                }
            }
            break;
        case evMouseUp:
            if (E.Mouse.X >= x && E.Mouse.Y >= y &&
                E.Mouse.X < x + w && E.Mouse.Y < y + h)
            {
                cur = GetVPosItem(id, w, E.Mouse.X - x, E.Mouse.Y - y);
            }
            if (cur == -1) {
                if (up) {
                    UpMenu *p = up;
                    cur = 0;
                    if (E.Mouse.X >= p->x && E.Mouse.Y >= p->y &&
                        E.Mouse.X < p->x + p->w && E.Mouse.Y < p->y + p->h)
                    {
                        if (p->vert) {
                            int i = GetVPosItem(p->id, p->w, E.Mouse.X - p->x, E.Mouse.Y - p->y);
                            if (i != -1)
                                if (Menus[p->id].Items[i].SubMenu == id) break;
                        } else {
                            int i = GetHPosItem(p->id, E.Mouse.X);
                            if (i != -1)
                                if (Menus[p->id].Items[i].SubMenu == id) break;
                        }
                        abort = -1;
                    }
                } else
                    abort = -1;
                if (E.Mouse.X >= x && E.Mouse.Y >= y &&
                    E.Mouse.X < x + w && E.Mouse.Y < y + h);
                else {
                    gui->ConPutEvent(E);
                    abort = -3;
                }
            } else {
                if (Menus[id].Items[cur].Name != 0 &&
                    Menus[id].Items[cur].SubMenu == -1)
                {
                    E.What = evCommand;
                    E.Msg.View = frames->Active;
                    E.Msg.Command = Menus[id].Items[cur].Cmd;
                    //fprintf(stderr, "Command set = %d %d %d\n", id, cur, Menus[id].Items[cur].Cmd);
                    abort = 1;
                }
            }
            break;
#endif
        }
    }
    if (SaveC) {
        ConPutBox(SaveX, SaveY, SaveW, SaveH, SaveC);
        free(SaveC);
        SaveC = 0;
    }
    ConShowCursor();
    if (up && abort == -3) return -3;
    return (abort == 1) ? 1 : -1;
}

int ExecMainMenu(TEvent &E, char sub) {
    int cur = 0;
    int id = GetMenuId(frames->Menu);
    int abort;
    int dovert = 1;
    int rx;
    static UpMenu top = { 0, 0, 0, 0, 0, 0, 1 };
    TCell topline[ConMaxCols];
    int Cols, Rows;
    
    ConQuerySize(&Cols, &Rows);
    
    top.x = 0;
    top.y = 0;
    top.h = 1;
    top.w = Cols;
    top.id = id;
    top.vert = 0;
    
    ConGetBox(0, 0, Cols, 1, topline);
    
    if (sub != 0) {
        for (int i = 0; i < Menus[id].Count; i++) {
            if (Menus[id].Items[i].Name) {
                char *o = strchr(Menus[id].Items[i].Name, '&');
                if (o && toupper(o[1]) == toupper(sub)) {
                    cur = i;
                    break;
                }
            }
        }
    }

#ifdef CONFIG_MOUSE
    if (E.What == evMouseDown) {
        cur = GetHPosItem(id, E.Mouse.X);
        dovert = 1;
    }
#endif
    abort = -2;
    while (abort == -2) {
        DrawHMenu(0, 0, id, cur);
        if (dovert) {
            if (cur != -1) {
                if (Menus[id].Items[cur].SubMenu != -1) {
                    rx = ExecVertMenu(GetHOfsItem(id, cur) - 2, 1,
                                      Menus[id].Items[cur].SubMenu, E, &top);
                    if (rx == 1) {
                        abort = 1;
                        continue;
                    } else if (rx == -3) {
                        abort = -1;
                        break;
                    } else
                        abort = -2;
                }
            }
        }
        ConHideCursor();
        do {
            ConGetEvent(evCommand | evMouseDown | evMouseMove | evMouseUp | evKeyDown | evNotify, &E, -1, 1);
            if (E.What & evNotify)
                gui->DispatchEvent(frames, frames->Active, E);
        } while (E.What & evNotify);
        dovert = 0;
        switch (E.What) {
        case evCommand:
            if (E.Msg.Command == cmResize) abort = -1;
            break;
        case evKeyDown:
            switch (kbCode(E.Key.Code)) {
            case kbEnd: cur = Menus[id].Count;
            case kbLeft:
                dovert = 1;
                {
                    int x = cur;
                    do {
                        cur--;
                        if (cur < 0) cur = Menus[id].Count - 1;
                    } while (cur != x && Menus[id].Items[cur].Name == 0);
                }
                break;
            case kbHome: cur = -1;
            case kbRight:
                dovert = 1;
                {
                    int x = cur;
                    do {
                        cur++;
                        if (cur >= Menus[id].Count) cur = 0;
                    } while (cur != x && Menus[id].Items[cur].Name == 0);
                }
                break;
            case kbEsc: abort = -1; dovert = 0; break;
            case kbEnter:
                if (cur != -1) {
                    if (Menus[id].Items[cur].SubMenu == -1) {
                        E.What = evCommand;
                        E.Msg.View = frames->Active;
                        E.Msg.Command = Menus[id].Items[cur].Cmd;
                        abort = 1;
                    } else {
                        dovert = 1;
                    }
                }
                break;
            default:
                if (isAscii(E.Key.Code)) {
                    char cc;
                    int i;
                    
                    cc = char(toupper(char(E.Key.Code & 0xFF)));
                    
                    for (i = 0; i < Menus[id].Count; i++) {
                        if (Menus[id].Items[i].Name) {
                            char *o = strchr(Menus[id].Items[i].Name, '&');
                            if (o)
                                if (toupper(o[1]) == cc) {
                                    cur = i;
                                    if (cur != -1) {
                                        if (Menus[id].Items[cur].SubMenu == -1) {
                                            E.What = evCommand;
                                            E.Msg.View = frames->Active;
                                            E.Msg.Command = Menus[id].Items[cur].Cmd;
                                            abort = 1;
                                        } else {
                                            dovert = 1;
                                        }
                                    }
                                    break;
                                }
                        }
                    }
                }
                break;
            }
            break;
#ifdef CONFIG_MOUSE
        case evMouseDown:
            if (E.Mouse.Y == 0) {
                int oldcur = cur;
                cur = GetHPosItem(id, E.Mouse.X);
                if (cur == oldcur) {
                    abort = -1;
                }
            } else {
                cur = -1;
                abort = -1;
            }
            dovert = 1;
            break;
        case evMouseMove:
            if (E.Mouse.Buttons) {
                if (E.Mouse.Y == 0)
                    cur = GetHPosItem(id, E.Mouse.X);
                else
                    cur = -1;
                dovert = 1;
            }
            break;
        case evMouseUp:
            if (E.Mouse.Y == 0)
                cur = GetHPosItem(id, E.Mouse.X);
            if (cur == -1)
                abort = -1;
            else {
                if (Menus[id].Items[cur].Name != 0 &&
                    Menus[id].Items[cur].SubMenu == -1) 
                {
                    E.What = evCommand;
                    E.Msg.View = frames->Active;
                    E.Msg.Command = Menus[id].Items[cur].Cmd;
                    abort = 1;
                }
            }
            break;
#endif
        }
    }
    DrawHMenu(0, 0, id, -1);
    ConPutBox(0, 0, Cols, 1, topline);
    ConShowCursor();
    return (abort == 1) ? 1 : -1;
}

void GFrame::DrawMenuBar() {
    DrawHMenu(0, 0, GetMenuId(Menu), -1);
}

extern TEvent NextEvent;

int GFrame::PopupMenu(const char *Name) {
    NextEvent.What = evCommand;
    NextEvent.Msg.Command = cmPopupMenu;
    NextEvent.Msg.Param1 = GetMenuId(Name);
    return 0;
}
