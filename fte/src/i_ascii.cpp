/*    i_ascii.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "i_ascii.h"

#ifdef CONFIG_I_ASCII

#include "c_color.h"

static int SPos = 0;
static int SLPos = 0;

ExASCII::ExASCII() :
    Pos(SPos),
    LPos(SLPos)
{
}

ExASCII::~ExASCII() {
    SPos = Pos;
    SLPos = LPos;
}

void ExASCII::HandleEvent(TEvent &Event) {
    int W, H;

    ConQuerySize(&W, &H);

    switch (Event.What) {
    case evKeyDown:
        switch(kbCode(Event.Key.Code)) {
        case kbLeft:           Pos--; Event.What = evNone; break;
        case kbRight:          Pos++; Event.What = evNone; break;
        case kbHome:           Pos = 0; Event.What = evNone; break;
        case kbEnd:            Pos = 255; Event.What = evNone; break;
        case kbLeft + kfCtrl:  Pos -= 16; Event.What = evNone; break;
        case kbRight + kfCtrl: Pos += 16; Event.What = evNone; break;
        case kbUp:             Pos -= W; LPos -= W; Event.What = evNone; break;
        case kbDown:           Pos += W; LPos += W; Event.What = evNone; break;
        case kbEsc:            EndExec(-1); Event.What = evNone; break;
        case kbEnter:          EndExec(Pos); Event.What = evNone; break;
        }
        break;
#if 0
    case evMouseDown:
        if (E.Mouse.X < XPos || E.Mouse.X >= XPos + 34 ||
            E.Mouse.Y < YPos || E.Mouse.Y >= YPos + 10)
        {
            abort = 2;
            break;
        }

        do {
            x = E.Mouse.X - XPos - 1;
            y = E.Mouse.Y - YPos - 1;
            if (x >= 0 && x < 32 &&
                y >= 0 && y < 8)
            {
                X = x;
                Y = y;
                if (X >= 32) X = 31;
                if (Y >= 8) Y = 7;
                if (X < 0) X = 0;
                if (Y < 0) Y = 0;
                frames->ConSetCursorPos(X + XPos + 1, Y + YPos + 1);
                sprintf(s, "0%03o %03d 0x%02X",
                        X + Y * 32, X + Y * 32, X + Y * 32);
                MoveStr(B, 0, 13, s, hcAsciiStatus, 13);
                frames->ConPutBox(XPos + 2, YPos + 9, 13, 1, B);
            }
            if (E.Mouse.Count == 2) {
                abort = 1;
                break;
            }
            gui->ConGetEvent(evMouse, &E, -1, 1);
            if (E.What == evMouseUp) break;
        } while (1);
        break;
#endif
    }
}

void ExASCII::RepaintStatus() {
    TDrawBuffer B;
    int W, H;

    ConQuerySize(&W, &H);

    if (Pos > 255) Pos = 255;
    if (Pos < 0) Pos = 0;
    if (LPos + W < Pos) LPos = Pos - W + 1;
    if (LPos > 255 - W) LPos = 255 - W + 1;
    if (LPos > Pos) LPos = Pos;
    if (LPos < 0) LPos = 0;

    for (int i = 0; i < W; i++)
        MoveCh(B + i, char(i + LPos), hcAsciiChars, 1);
    ConSetCursorPos(Pos - LPos, H - 1);
    ConShowCursor();
    ConPutBox(0, H - 1, W, 1, B);
}
#endif
