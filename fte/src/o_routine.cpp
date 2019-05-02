/*    o_routine.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "o_routine.h"

#ifdef CONFIG_OBJ_ROUTINE

#include "i_view.h"
#include "o_buflist.h"
#include "s_string.h"
#include "sysdep.h"

#include <stdio.h>

RoutineView::RoutineView(int createFlags, EModel **ARoot, EBuffer *AB) :
    EList(createFlags, ARoot, "Routines"),
    Buffer(AB)
{
    if (Buffer->rlst.Count == 0)
        Buffer->ScanForRoutines();

    int Row = Buffer->VToR(Buffer->CP.Row);
    for (int i = Buffer->rlst.Count - 1; i >= 0; --i)
        if (Row >= Buffer->rlst.Lines[i]) {
            Row = i;
            break;
        }

    char CTitle[256];
    snprintf(CTitle, sizeof(CTitle), "Routines %s: %d", Buffer->FileName, Buffer->rlst.Count);
    SetTitle(CTitle);
};

RoutineView::~RoutineView() {
    Buffer->Routines = 0;
}

EEventMap *RoutineView::GetEventMap() {
    return FindEventMap("ROUTINES");
}

int RoutineView::ExecCommand(ExCommands Command, ExState &State) {
    switch (Command) {
    case ExRescan:
        Buffer->ScanForRoutines();
        UpdateList();
        return 1;
    case ExActivateInOtherWindow:
        if (Row < Buffer->rlst.Count) {
            View->Next->SwitchToModel(Buffer);
            Buffer->CenterPosR(0, Buffer->rlst.Lines[Row]);
            return 1;
        }
        return 0;
    case ExCloseActivate:
        return 0;
    default:
        ;
    }
    return EList::ExecCommand(Command, State);
}
    
void RoutineView::DrawLine(PCell B, int Line, int Col, ChColor color, int Width) {
    if ((int)Buffer->RLine(Buffer->rlst.Lines[Line])->Count > Col) {
        char str[1024];
        size_t len;

        len = UnTabStr(str, sizeof(str),
                       Buffer->RLine(Buffer->rlst.Lines[Line])->Chars,
                       Buffer->RLine(Buffer->rlst.Lines[Line])->Count);

        if ((int)len > Col)
            MoveStr(B, 0, Width, str + Col, color, len - Col);
    }
}

char* RoutineView::FormatLine(int Line) {
    PELine L = Buffer->RLine(Buffer->rlst.Lines[Line]);
    
    char *p = (char *) malloc(L->Count + 1);
    if (p) {
        memcpy(p, L->Chars, L->Count);
        p[L->Count] = 0;
    }
    return p;
}

int RoutineView::Activate(int No) {
    if (No >= Buffer->rlst.Count)
	return 0;

    View->SwitchToModel(Buffer);
    Buffer->CenterPosR(0, Buffer->rlst.Lines[No]);
    return 1;
}

void RoutineView::RescanList() {
    Buffer->ScanForRoutines();
    UpdateList();
    NeedsRedraw = 1;
}

void RoutineView::UpdateList() {
    Count = Buffer->rlst.Count;
}

int RoutineView::GetContext() {
    return CONTEXT_ROUTINES;
}

void RoutineView::GetName(char *AName, size_t MaxLen) {
    strlcpy(AName, "Routines", MaxLen);
}

void RoutineView::GetInfo(char *AInfo, size_t MaxLen) {
    snprintf(AInfo, MaxLen, "%2d %04d/%03d Routines (%s)",
	    ModelNo, Row + 1, Count, Buffer->FileName);
}

void RoutineView::GetTitle(char *ATitle, size_t MaxLen, char *ASTitle, size_t SMaxLen) {
    snprintf(ATitle, MaxLen, "Routines: %s", Buffer->FileName);
    strlcpy(ASTitle, "Routines", SMaxLen);
}

#endif // CONFIG_OBJ_ROUTINE
