/*    i_input.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "c_color.h"
#include "c_config.h"
#include "c_history.h"
#include "i_view.h"
#include "o_buflist.h"
#include "s_util.h"

#include <ctype.h>

ExInput::ExInput(const char *APrompt, const char *ALine, size_t ABufLen,
                 Completer AComp, int Select, int AHistId) :
    Prompt(APrompt),
    MaxLen(ABufLen - 1),
    Line(new char[MaxLen + 1]),
    MatchStr(new char[MaxLen + 1]),
    CurStr(new char[MaxLen + 1]),
    LPos(0),
    Comp(AComp),
    TabCount(0),
    HistId(AHistId),
    CurItem(0),
    SelStart(0),
    SelEnd(0)
{
    assert(ABufLen > 0);
    Line[MaxLen] = 0;
    strncpy(Line, ALine, MaxLen);
    Pos = strlen(Line);
    if (Select)
        SelEnd = Pos;
}

ExInput::~ExInput() {
    delete[] Line;
    delete[] MatchStr;
    delete[] CurStr;
}

void ExInput::HandleEvent(TEvent &Event) {
    switch (Event.What) {
    case evKeyDown:
        switch (kbCode(Event.Key.Code)) {
        case kbLeft:
            if (Pos > 0)
                Pos--;
        common:
            SelStart = SelEnd = 0;
            TabCount = 0;
            Event.What = evNone;
            break;
        case kbRight: Pos++; goto common;
        case kbLeft | kfCtrl:
            if (Pos > 0) {
                Pos--;
                while (Pos > 0) {
                    if (isalnum(Line[Pos]) && !isalnum(Line[Pos - 1]))
                        break;
                    Pos--;
                }
            }
            goto common;
        case kbRight | kfCtrl:
            while (Line[Pos])
                if (!isalnum(Line[Pos++]) && (isalnum(Line[Pos])))
                    break;
            goto common;
        case kbHome: Pos = 0; goto common;
        case kbEnd: Pos = strlen(Line); goto common;
        case kbEsc: EndExec(0); goto common;
        case kbEnter:
#ifdef CONFIG_HISTORY
            AddInputHistory(HistId, Line);
#endif
            EndExec(1);
            goto common;
        case kbBackSp | kfCtrl | kfShift:
            Pos = 0;
            Line[0] = 0;
            goto common;
        case kbBackSp | kfCtrl:
            if (Pos > 0) {
                if (Pos > strlen(Line)) {
                    Pos = strlen(Line);
                } else {
                    char Ch;

                    if (Pos > 0)
                        do {
                            Pos--;
                            memmove(Line + Pos, Line + Pos + 1, strlen(Line + Pos + 1) + 1);
                            if (Pos == 0) break;
                            Ch = Line[Pos - 1];
                        } while (Pos > 0 && Ch != '\\' && Ch != '/' && Ch != '.' && isalnum(Ch));
                }
            }
            goto common;
        case kbBackSp:
        case kbBackSp | kfShift:
            if (SelStart < SelEnd) {
                memmove(Line + SelStart, Line + SelEnd, strlen(Line + SelEnd) + 1);
                Pos = SelStart;
                SelStart = SelEnd = 0;
                break;
            }
            if (Pos <= 0) break;
            Pos--;
            if (Pos < strlen(Line))
                memmove(Line + Pos, Line + Pos + 1, strlen(Line + Pos + 1) + 1);
            TabCount = 0;
            Event.What = evNone;
            break;
        case kbDel:
            if (SelStart < SelEnd) {
                memmove(Line + SelStart, Line + SelEnd, strlen(Line + SelEnd) + 1);
                Pos = SelStart;
                SelStart = SelEnd = 0;
                break;
            }
            if (Pos < strlen(Line))
                memmove(Line + Pos, Line + Pos + 1, strlen(Line + Pos + 1) + 1);
            TabCount = 0;
            Event.What = evNone;
            break;
        case kbDel | kfCtrl:
            if (SelStart < SelEnd) {
                memmove(Line + SelStart, Line + SelEnd, strlen(Line + SelEnd) + 1);
                Pos = SelStart;
                SelStart = SelEnd = 0;
                break;
            }
            Line[Pos] = 0;
            goto common;
        case kbIns | kfShift:
        case 'V'   | kfCtrl:
            {
                if (SystemClipboard)
                    GetPMClip(0);

                if (SSBuffer == 0) break;
                if (SSBuffer->RCount == 0) break;

                if (SelStart < SelEnd) {
                    memmove(Line + SelStart, Line + SelEnd, strlen(Line + SelEnd) + 1);
                    Pos = SelStart;
                    SelStart = SelEnd = 0;
                }

                size_t len = SSBuffer->LineChars(0);
                if (strlen(Line) + len < MaxLen) {
                    memmove(Line + Pos + len, Line + Pos, strlen(Line + Pos) + 1);
                    memcpy(Line + Pos, SSBuffer->RLine(0)->Chars, len);
                    Pos += len;
                    TabCount = 0;
                    Event.What = evNone;
                }
            }
            break;
#ifdef CONFIG_HISTORY
        case kbUp:
            if (CurItem == 0)
                strcpy(CurStr, Line);
            CurItem += 2;
            /* fall */
        case kbDown:
            SelStart = SelEnd = 0;
            if (CurItem == 0) break;
            CurItem--;
            {
                int cnt = CountInputHistory(HistId);

                if (CurItem > cnt) CurItem = cnt;
                if (CurItem < 0) CurItem = 0;

                if (!CurItem || !GetInputHistory(HistId, Line, MaxLen, CurItem))
                    strcpy(Line, CurStr);
                Pos = strlen(Line);
            }
            Event.What = evNone;
            break;
#endif
        case kbTab | kfShift:
            TabCount -= 2;
            /* fall */
        case kbTab:
            if (Comp) {
                char *Str2 = (char*)alloca(MaxLen + 1);
                int n;

                TabCount++;
                if (TabCount < 1) TabCount = 1;
                if ((TabCount == 1) && (kbCode(Event.Key.Code) == kbTab))
                    strcpy(MatchStr, Line);

                n = Comp(MatchStr, Str2, TabCount);
                if ((n > 0) && (TabCount <= n)) {
                    strcpy(Line, Str2);
                    Pos = strlen(Line);
                } else if (TabCount > n) TabCount = n;
            }
            SelStart = SelEnd = 0;
            Event.What = evNone;
            break;
        case 'Q' | kfCtrl:
            Event.What = evKeyDown;
            Event.Key.Code = Win->GetChar(0);
        default:
            {
                char Ch;

                if (GetCharFromEvent(Event, &Ch) && (strlen(Line) < MaxLen)) {
                    if (SelStart < SelEnd) {
                        memmove(Line + SelStart, Line + SelEnd, strlen(Line + SelEnd) + 1);
                        Pos = SelStart;
                        SelStart = SelEnd = 0;
                    }
                    memmove(Line + Pos + 1, Line + Pos, strlen(Line + Pos) + 1);
                    Line[Pos++] = Ch;
                    TabCount = 0;
                    Event.What = evNone;
                }
            }
            break;
        }
        Event.What = evNone;
        break;
    }
}

void ExInput::RepaintStatus() {
    TDrawBuffer B;
    int W, H, FLen, FPos;

    ConQuerySize(&W, &H);

    FPos = Prompt.size() + 2;
    FLen = W - FPos;

    if (Pos > strlen(Line))
        Pos = strlen(Line);
    //if (Pos < 0) Pos = 0;
    if (LPos + FLen <= Pos) LPos = Pos - FLen + 1;
    if (Pos < LPos) LPos = Pos;

    MoveChar(B, 0, W, ' ', hcEntry_Field, W);
    MoveStr(B, 0, W, Prompt.c_str(), hcEntry_Prompt, FPos);
    MoveChar(B, FPos - 2, W, ':', hcEntry_Prompt, 1);
    MoveStr(B, FPos, W, Line + LPos, hcEntry_Field, FLen);
    MoveAttr(B, FPos + SelStart - LPos, W, hcEntry_Selection, SelEnd - SelStart);
    ConSetCursorPos(FPos + Pos - LPos, H - 1);
    ConPutBox(0, H - 1, W, 1, B);
    ConShowCursor();
}
