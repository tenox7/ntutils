/*    i_search.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "i_search.h"

#ifdef CONFIG_I_SEARCH

#include "i_view.h"
#include "o_buflist.h"

#include <stdio.h>

static char PrevISearch[ExISearch::MAXISEARCH] = "";

ExISearch::ExISearch(EBuffer *B) :
    Orig(B->CP), // ?
    len(0),
    stacklen(0),
    Buffer(B),
    state(INoMatch),
    Direction(0)
{
    ISearchStr[0] = 0;
}

ExISearch::~ExISearch() {
    if (ISearchStr[0] != 0)
        strcpy(PrevISearch, ISearchStr);
}

void ExISearch::HandleEvent(TEvent &Event) {
    int Case = BFI(Buffer, BFI_MatchCase) ? 0 : SEARCH_NCASE;
    
    ExView::HandleEvent(Event);
    switch (Event.What) {
    case evKeyDown:
        SetState(IOk);
        switch (kbCode(Event.Key.Code)) {
        case kbEsc: 
            Buffer->SetPos(Orig.Col, Orig.Row);
            EndExec(0); 
            break;
        case kbEnter: EndExec(1); break;
        case kbBackSp:
            if (len > 0) {
                if (stacklen > 0) {
                    stacklen--;
                    if (Buffer->CenterPos(stack[stacklen].Col, stack[stacklen].Row) == 0) return;
                }
                len--;
                ISearchStr[len] = 0;
                if (len > 0 && Buffer->FindStr(ISearchStr, len, Case | Direction) == 0) {
                    SetState(INoMatch);
                }
            } else {
                if (Buffer->CenterPos(Orig.Col, Orig.Row) == 0) return;
            }
            break;
        case kbUp:
            Buffer->ScrollDown(1);
            break;
        case kbDown:
            Buffer->ScrollUp(1);
            break;
        case kbLeft:
            Buffer->ScrollRight(8);
            break;
        case kbRight:
            Buffer->ScrollLeft(8);
            break;
        case kbPgDn:
            Buffer->MovePageDown();
            break;
        case kbPgUp:
            Buffer->MovePageUp();
            break;
        case kbPgUp | kfCtrl:
            Buffer->MoveFileStart();
            break;
        case kbPgDn | kfCtrl:
            Buffer->MoveFileEnd();
            break;
        case kbHome:
            Buffer->MoveLineStart();
            break;
        case kbEnd:
            Buffer->MoveLineEnd();
            break;
        case kbTab | kfShift:
            Direction = SEARCH_BACK;
            if (len == 0) {
                strcpy(ISearchStr, PrevISearch);
                len = (int)strlen(ISearchStr);
                if (len == 0)
                    break;
            }
            if (Buffer->FindStr(ISearchStr, len, Case | Direction | SEARCH_NEXT) == 0) {
                Buffer->FindStr(ISearchStr, len, Case);
                SetState(INoPrev);
            }
            break;
        case kbTab:
            Direction = 0;
            if (len == 0) {
                strcpy(ISearchStr, PrevISearch);
                len = (int)strlen(ISearchStr);
                if (len == 0)
                    break;
            }
            if (Buffer->FindStr(ISearchStr, len, Case | Direction | SEARCH_NEXT) == 0) {
                Buffer->FindStr(ISearchStr, len, Case);
                SetState(INoNext);
            }
            break;
        case 'Q' | kfCtrl:
            Event.What = evKeyDown;
            Event.Key.Code = Win->GetChar(0);
        default:
            if (isAscii(Event.Key.Code) && (len < (int)MAXISEARCH)) {
                char Ch = (char) Event.Key.Code;
                
                stack[stacklen++] = Buffer->CP;
                ISearchStr[len++] = Ch;
                ISearchStr[len] = 0;
                if (Buffer->FindStr(ISearchStr, len, Case | Direction) == 0) {
                    SetState(INoMatch);
                    len--;
                    stacklen--;
                    ISearchStr[len] = 0;
                    Buffer->FindStr(ISearchStr, len, Case | Direction);
                } 
            }
            break;
        }
    }
}

void ExISearch::RepaintStatus() {
    TDrawBuffer B;
    char s[MAXISEARCH + 1];
    const char *p;
    int W, H;
    
    ConQuerySize(&W, &H);
    
    switch (state) {
    case INoMatch: p = " No Match. "; break;
    case INoPrev: p = " No Prev Match. "; break;
    case INoNext: p = " No Next Match. "; break;
    case IOk: default: p = ""; break;
    }
    
    sprintf(s, "ISearch [%s]%s", ISearchStr, p);
    MoveCh(B, ' ', 0x17, W);
    MoveStr(B, 0, W, s, 0x17, W);
    ConPutBox(0, H - 1, W, 1, B);
    ConSetCursorPos((int)strlen(s) - 1, H - 1);
    ConShowCursor();
}

void ExISearch::SetState(IState s) {
    state = s;
    RepaintView();
}

#endif
