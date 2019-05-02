/*    o_buflist.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */


#include "o_buflist.h"

#include "i_modelview.h"
#include "i_view.h"
#include "s_string.h"
#include "s_util.h"
#include "sysdep.h"

#include <stdio.h>

BufferView *BufferList = 0;

BufferView::BufferView(int createFlags, EModel **ARoot) :
    EList(createFlags, ARoot, "Buffers"),
    BList(0),
    BCount(0),
    SearchLen(0)
{
    ModelNo = 0; // trick
}

BufferView::~BufferView() {
    if (BList) {
        for (int i = 0; i < BCount; i++)
            if (BList[i])
                free(BList[i]);
        free(BList);
    }
    BufferList = 0;
}
    
EEventMap *BufferView::GetEventMap() {
    return FindEventMap("BUFFERS");
}

int BufferView::GetContext() {
    return CONTEXT_BUFFERS; 
}

void BufferView::DrawLine(PCell B, int Line, int Col, ChColor color, int Width) {
    if (Line < BCount)
        if (Col < int(strlen(BList[Line])))
            MoveStr(B, 0, Width, BList[Line] + Col, color, Width);
}

char* BufferView::FormatLine(int Line) {
    return strdup(BList[Line]);
}

void BufferView::UpdateList() {
    EModel *B = ActiveModel;
    int No;
    char s[512] = "";
    
    if (BList) {
        for (int i = 0; i < BCount; i++)
            if (BList[i])
                free(BList[i]);
        free(BList);
    }
    BList = 0;
    BCount = 0;
    while (B) {
        BCount++;
        B = B->Next;
        if (B == ActiveModel) break;
    }
    BList = (char **) malloc(sizeof(char *) * BCount);
    assert(BList != 0);
    B = ActiveModel;
    No = 0;
    while (B) {
        B->GetInfo(s, sizeof(s) - 1);
        BList[No++] = strdup(s);
        B = B->Next;
        if (B == ActiveModel) break;
        if (No >= BCount) break;
    }
    Count = BCount;
    NeedsUpdate = 1;
}
    
EModel *BufferView::GetBufferById(int No) {
    EModel *B;
    
    B = ActiveModel;
    while (B) {
        if (No == 0) {
            return B;
        }
        No--;
        B = B->Next;
        if (B == ActiveModel) break;
    }
    return 0;
}
    
int BufferView::ExecCommand(ExCommands Command, ExState &State) {

    switch (Command) {
    case ExCloseActivate:
        {
            EModel *B;

            CancelSearch();
            B = GetBufferById(Row);
            if (B && B != this) {
                View->SwitchToModel(B);
                delete this;
                return 1;
            }
        }
        return 0;
    case ExBufListFileClose:
        {
            EModel *B = GetBufferById(Row);

            CancelSearch();
            if (B && B != this && Count > 1) {
                if (B->ConfQuit(View->MView->Win)) {
                    View->DeleteModel(B);
                }
                UpdateList();
                return 1;
            }
        }
        return 0;
    case ExBufListFileSave:
        {
            EModel *B = GetBufferById(Row);

            if (B && B->GetContext() == CONTEXT_FILE)
                if (((EBuffer *)B)->Save())
                    return 1;
        }
        return 0;
        
    case ExActivateInOtherWindow:
        {
            EModel *B = GetBufferById(Row);

            CancelSearch();
            if (B) {
                View->Next->SwitchToModel(B);
                return 1;
            }
        }
        return 0;
    case ExBufListSearchCancel:
        CancelSearch();
        return 1;
    case ExBufListSearchNext:
        // Find next matching line
        if (SearchLen) {
            int i = Row + 1;
            i = getMatchingLine(i == BCount ? 0 : i, 1);
            // Never returns -1 since something already found before call
            Row = SearchPos[SearchLen] = i;
        }
        return 1;
    case ExBufListSearchPrev:
        // Find prev matching line
        if (SearchLen) {
            int i = Row - 1;
            i = getMatchingLine(i == -1 ? BCount - 1 : i, -1);
            // Never returns -1 since something already found before call
            Row = SearchPos[SearchLen] = i;
        }
        return 1;
    default:
        ;
    }
    return EList::ExecCommand(Command, State);
}

void BufferView::HandleEvent(TEvent &Event) {
    int resetSearch = 1;
    EModel::HandleEvent(Event);
    switch (Event.What) {
        case evKeyUp:
            resetSearch = 0;
            break;
        case evKeyDown:
            switch (kbCode(Event.Key.Code)) {
                case kbBackSp:
                    resetSearch = 0;
                    if (SearchLen > 0) {
                        SearchString[--SearchLen] = 0;
                        Row = SearchPos[SearchLen];
                        Msg(S_INFO, "Search: [%s]", SearchString);
                    } else
                        Msg(S_INFO, "");
                    break;
                case kbEsc:
                    Msg(S_INFO, "");
                    break;
                default:
                    resetSearch = 0;
                    if (isAscii(Event.Key.Code) && (SearchLen < ExISearch::MAXISEARCH)) {
                        char Ch = (char) Event.Key.Code;

                        SearchPos[SearchLen] = Row;
                        SearchString[SearchLen] = Ch;
                        SearchString[++SearchLen] = 0;
                        int i = getMatchingLine(Row, 1);
                        if (i == -1)
                            SearchString[--SearchLen] = 0;
                        else
                            Row = i;
                        Msg(S_INFO, "Search: [%s]", SearchString);
                    }
                    break;
            }
    }
    if (resetSearch)
        SearchLen = 0;
}

/**
 * Search for next line containing SearchString starting from line 'start'.
 * Direction should be 1 for ascending and -1 for descending.
 * Returns line found or -1 if none.
 */
int BufferView::getMatchingLine (int start, int direction) {
    int i = start;
    do {
        // Find SearchString at any place in string for line i
        for(int j = 0; BList[i][j]; j++)
            if (BList[i][j] == SearchString[0] && strnicmp(SearchString, BList[i]+j, SearchLen) == 0) {
                return i;
            }
        i += direction;
        if (i == BCount) i = 0; else if (i == -1) i = BCount - 1;
    } while (i != start);
    return -1;
}

int BufferView::Activate(int No) {
    EModel *B;

    CancelSearch();
    B = GetBufferById(No);
    if (B) {
        View->SwitchToModel(B);
        return 1;
    }
    return 0;
}

void BufferView::CancelSearch() {
    SearchLen = 0;
    Msg(S_INFO, "");
}

void BufferView::GetInfo(char *AInfo, size_t MaxLen) {
    snprintf(AInfo, MaxLen, "%2d %04d/%03d Buffers", ModelNo, Row + 1, Count);
}

void BufferView::GetTitle(char *ATitle, size_t MaxLen, char *ASTitle, size_t SMaxLen) {
    strlcpy(ATitle, "Buffers", MaxLen);
    strlcpy(ASTitle, "Buffers", SMaxLen);
}
