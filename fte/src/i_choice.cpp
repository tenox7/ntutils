/*    i_choice.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "i_choice.h"

#include "c_color.h"
#include "i_view.h"
#include "s_string.h"
#include "sysdep.h"

#include <ctype.h>
#include <stdio.h>

ExChoice::ExChoice(const char *ATitle, int NSel, va_list ap) :
    Title(ATitle),
    lTitle((int)Title.size()),
    NOpt(NSel),
    SOpt(NSel),
    lChoice(0),
    Cur(0),
    MouseCaptured(0)
{
    for (int i = 0; i < NSel; i++) {
        SOpt.push_back((va_arg(ap, char *)));
        lChoice += SOpt.back().GetCStrLen() + 1;
    }

    char *fmt = va_arg(ap, char *);
    vsnprintf(Prompt, sizeof(Prompt), fmt, ap);
}

ExChoice::~ExChoice()
{
}

int ExChoice::FindChoiceByPoint(int x, int y) {
    int W, H;

    Win->ConQuerySize(&W, &H);

    if (y != H - 1)
        return -1;

    int pos = W - lChoice;
    if (x < pos)
        return -1;

    for (int i = 0; i < NOpt; i++) {
        if (x > pos && x <= pos + SOpt[i].GetCStrLen())
            return i;
        pos += SOpt[i].GetCStrLen() + 1;
    }
    return -1;
}

void ExChoice::HandleEvent(TEvent &Event) {
    switch (Event.What) {
    case evKeyDown:
        switch (kbCode(Event.Key.Code)) {
        case kbTab | kfShift: /* fall */
        case kbLeft: if (Cur == -1) Cur = 0; Cur--; if (Cur < 0) Cur = NOpt - 1; Event.What = evNone; break;
        case kbTab: /* fall */
        case kbRight: if (Cur == -1) Cur = 0; Cur++; if (Cur >= NOpt) Cur = 0; Event.What = evNone; break;
        case kbHome: Cur = 0; Event.What = evNone; break;
        case kbEnd: Cur = NOpt - 1; Event.What = evNone; break;
        case kbEnter: if (Cur >= 0 && NOpt > 0) EndExec(Cur); Event.What = evNone; break;
        case kbEsc: EndExec(-1); Event.What = evNone; break;
        default:
            if (isAscii(Event.Key.Code)) {
		char s[3] = { '&', (char)(toupper((char)Event.Key.Code)), 0 };

                for (int i = 0; i < NOpt; ++i) {
                    if (strstr(SOpt[i].GetCStr(), s) != 0) {
                        Win->EndExec(i);
                        break;
                    }
                }
                Event.What = evNone;
            }
            break;
        }
        break;
#ifdef CONFIG_MOUSE
    case evMouseDown:
	if (!Win->CaptureMouse(1))
	    break;
	MouseCaptured = 1;
        Cur = FindChoiceByPoint(Event.Mouse.X, Event.Mouse.Y);
        Event.What = evNone;
        break;
    case evMouseMove:
        if (MouseCaptured)
            Cur = FindChoiceByPoint(Event.Mouse.X, Event.Mouse.Y);
        Event.What = evNone;
        break;
    case evMouseUp:
	if (!MouseCaptured)
	    break;
	Win->CaptureMouse(0);
        MouseCaptured = 0;
        Cur = FindChoiceByPoint(Event.Mouse.X, Event.Mouse.Y);
        Event.What = evNone;
        if (Cur >= 0 && Cur < NOpt && NOpt > 0)
            EndExec(Cur);
        else
            Cur = 0;
        break;
#endif
    }
}

void ExChoice::RepaintStatus() {
    TDrawBuffer B;
    int W, H;

    ConQuerySize(&W, &H);

    if (Cur != -1) {
        if (Cur >= NOpt) Cur = NOpt - 1;
        if (Cur < 0) Cur = 0;
    }

    MoveCh(B, ' ', hcChoice_Background, W);
    MoveStr(B, 0, W, Title.c_str(), hcChoice_Title, W);
    MoveChar(B, lTitle, W, ':', hcChoice_Background, 1);
    MoveStr(B, lTitle + 2, W, Prompt, hcChoice_Param, W);

    int pos = W - lChoice;
    for (int i = 0; i < NOpt; ++i) {
	TAttr color1, color2;
        if (i == Cur) {
            color1 = hcChoice_ActiveItem;
            color2 = hcChoice_ActiveChar;
        } else {
            color1 = hcChoice_NormalItem;
            color2 = hcChoice_NormalChar;
        }
        if (i == Cur)
            ConSetCursorPos(pos + 1, H - 1);
        MoveChar(B, pos, W, ConGetDrawChar(DCH_V), hcChoice_Background, 1);
        MoveCStr(B, pos + 1, W, SOpt[i].GetCStr(), color1, color2, W);
        pos += SOpt[i].GetCStrLen() + 1;
    }
    ConPutBox(0, H - 1, W, 1, B);
}
