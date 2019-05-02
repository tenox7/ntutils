/*    e_cmds.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "egui.h"
#include "i_modelview.h"
#include "o_buflist.h"
#include "s_util.h"

int EBuffer::MoveLeft() {
    if (CP.Col == 0 || !VCount) return 0;
    SetPos(CP.Col - 1, CP.Row, tmLeft);
    return 1;
}

int EBuffer::MoveRight() {
    if (!VCount) return 0;
    SetPos(CP.Col + 1, CP.Row, tmRight);
    return 1;
}

int EBuffer::MoveUp() {
    if (CP.Row == 0) return 0;
    SetPos(CP.Col, CP.Row - 1, tmLeft);
    return 1;
}

int EBuffer::MoveDown() {
    if (CP.Row + 1 >= VCount) return 0;
    SetPos(CP.Col, CP.Row + 1, tmLeft);
    return 1;
}

int EBuffer::MovePrev() {
    if (MoveLeft()) return 1;
    if (MoveUp() && MoveLineEnd()) return 1;
    return 0;
}

int EBuffer::MoveNext() {
    if (CP.Col < LineLen())
        if (MoveRight()) return 1;
    if (MoveDown() && MoveLineStart()) return 1;
    return 0;
}


int EBuffer::MoveWordLeftX(int start) {
    if (CP.Col > 0) {
        int wS = start, wE = 1 - start;
        PELine L = VLine(CP.Row);
        int C, P;
        
        C = CP.Col;
        P = CharOffset(L, C);
        
        if (P > L->Count) P = L->Count;
        if (P > 0) {
            while ((P > 0) && (WGETBIT(Flags.WordChars, L->Chars[P - 1]) == wE)) P--;
            while ((P > 0) && (WGETBIT(Flags.WordChars, L->Chars[P - 1]) == wS)) P--;
            C = ScreenPos(L, P);
            return SetPos(C, CP.Row);
        } else return 0;
    } else return 0;
}


int EBuffer::MoveWordRightX(int start) {
    PELine L = VLine(CP.Row);
    int C, P;
    int wS = start, wE = 1 - start;
    
    C = CP.Col;
    P = CharOffset(L, C);
    
    if (P >= L->Count) return 0;
    
    while ((P < L->Count) && (WGETBIT(Flags.WordChars, L->Chars[P]) == wS)) P++;
    while ((P < L->Count) && (WGETBIT(Flags.WordChars, L->Chars[P]) == wE)) P++;
    C = ScreenPos(L, P);
    return SetPos(C, CP.Row);
}

int EBuffer::MoveWordLeft() {
    return MoveWordLeftX(1);
}

int EBuffer::MoveWordRight() {
    return MoveWordRightX(1);
}

int EBuffer::MoveWordPrev() {
    if (MoveWordLeft()) return 1;
    if (MoveUp() && MoveLineEnd()) return 1;
    return 0;
}

int EBuffer::MoveWordNext() {
    if (MoveWordRight()) return 1;
    if (MoveDown() && MoveLineStart()) return 1;
    return 0;
}

int EBuffer::MoveWordEndLeft() {
    return MoveWordLeftX(0);
}

int EBuffer::MoveWordEndRight() {
    return MoveWordRightX(0);
}

int EBuffer::MoveWordEndPrev() {
    if (MoveWordEndLeft()) return 1;
    if (MoveUp() && MoveLineEnd()) return 1;
    return 0;
}

int EBuffer::MoveWordEndNext() {
    if (MoveWordEndRight()) return 1;
    if (MoveDown() && MoveLineStart()) return 1;
    return 0;
}

int EBuffer::MoveWordOrCapLeft() {
    if (CP.Col > 0) {
        PELine L = VLine(CP.Row);
        int C, P;
        
        C = CP.Col;
        P = CharOffset(L, C);
        
        if (P > L->Count) P = L->Count;
        if (P > 0) {
            while ((P > 0) && (WGETBIT(Flags.WordChars, L->Chars[P - 1]) == 0)) P--;
            while ((P > 0) && (WGETBIT(Flags.WordChars, L->Chars[P - 1]) == 1) && (WGETBIT(Flags.CapitalChars, L->Chars[P - 1]) == 0)) P--;
            while ((P > 0) && (WGETBIT(Flags.CapitalChars, L->Chars[P - 1]) == 1)) P--;
            C = ScreenPos(L, P);
            return SetPos(C, CP.Row);
        } else return 0;
    } else return 0;
}

int EBuffer::MoveWordOrCapRight() {
    PELine L = VLine(CP.Row);
    int C, P;
    
    C = CP.Col;
    P = CharOffset(L, C);
    
    if (P >= L->Count) return 0;
    
    while ((P < L->Count) && (WGETBIT(Flags.CapitalChars, L->Chars[P]) == 1)) P++;
    while ((P < L->Count) && (WGETBIT(Flags.WordChars, L->Chars[P]) == 1) && (WGETBIT(Flags.CapitalChars, L->Chars[P]) == 0)) P++;
    while ((P < L->Count) && (WGETBIT(Flags.WordChars, L->Chars[P]) == 0)) P++;
    C = ScreenPos(L, P);
    return SetPos(C, CP.Row);
}

int EBuffer::MoveWordOrCapPrev() {
    if (MoveWordOrCapLeft()) return 1;
    if (MoveUp() && MoveLineEnd()) return 1;
    return 0;
}

int EBuffer::MoveWordOrCapNext() {
    if (MoveWordOrCapRight()) return 1;
    if (MoveDown() && MoveLineStart()) return 1;
    return 0;
}

int EBuffer::MoveWordOrCapEndLeft() {
    if (CP.Col > 0) {
        PELine L = VLine(CP.Row);
        int C, P;

        C = CP.Col;
        P = CharOffset(L, C);
        
        if (P > L->Count) P = L->Count;
        if (P > 0) {
            while ((P > 0) && (WGETBIT(Flags.WordChars, L->Chars[P - 1]) == 1) && (WGETBIT(Flags.CapitalChars, L->Chars[P - 1]) == 0)) P--;
            while ((P > 0) && (WGETBIT(Flags.CapitalChars, L->Chars[P - 1]) == 1)) P--;
            while ((P > 0) && (WGETBIT(Flags.WordChars, L->Chars[P - 1]) == 0)) P--;
            C = ScreenPos(L, P);
            return SetPos(C, CP.Row);
        } else return 0;
    } else return 0;
}

int EBuffer::MoveWordOrCapEndRight() {
    PELine L = VLine(CP.Row);
    int C, P;
    
    C = CP.Col;
    P = CharOffset(L, C);
    
    if (P >= L->Count) return 0;
    
    while ((P < L->Count) && (WGETBIT(Flags.WordChars, L->Chars[P]) == 0)) P++;
    while ((P < L->Count) && (WGETBIT(Flags.CapitalChars, L->Chars[P]) == 1)) P++;
    while ((P < L->Count) && (WGETBIT(Flags.WordChars, L->Chars[P]) == 1) && (WGETBIT(Flags.CapitalChars, L->Chars[P]) == 0)) P++;
    C = ScreenPos(L, P);
    return SetPos(C, CP.Row);
}

int EBuffer::MoveWordOrCapEndPrev() {
    if (MoveWordOrCapEndLeft()) return 1;
    if (MoveUp() && MoveLineEnd()) return 1;
    return 0;
}

int EBuffer::MoveWordOrCapEndNext() {
    if (MoveWordOrCapEndRight()) return 1;
    if (MoveDown() && MoveLineStart()) return 1;
    return 0;
}

int EBuffer::MoveLineStart() {
    SetPos(0, CP.Row);
    return 1;
}

int EBuffer::MoveLineEnd() {
    SetPos(LineLen(VToR(CP.Row)), CP.Row);
    return 1;
}

int EBuffer::MovePageUp() {
    return ScrollDown(GetVPort()->Rows);
}

int EBuffer::MovePageDown() {
    return ScrollUp(GetVPort()->Rows);
}

int EBuffer::MovePageLeft() {
    return ScrollRight(GetVPort()->Cols);
}

int EBuffer::MovePageRight() {
    return ScrollRight(GetVPort()->Cols);
}

int EBuffer::MovePageStart() {
    SetPos(CP.Col, GetVPort()->TP.Row, tmLeft);
    return 1;
}

int EBuffer::MovePageEnd() {
    SetNearPos(CP.Col, 
               GetVPort()->TP.Row +
               GetVPort()->Rows - 1, tmLeft);
    return 1;
}

int EBuffer::MoveFileStart() {
    SetPos(0, 0);
    return 1;
}

int EBuffer::MoveFileEnd() {
    SetPos(LineLen(VToR(VCount - 1)), VCount - 1);
    return 1;
}

int EBuffer::MoveBlockStart() {
    if (BB.Col == -1 && BB.Row == -1)
        return 0;
    assert(BB.Col >= 0 && BB.Row >= 0 && BB.Row < RCount);
    if (SetPosR(BB.Col, BB.Row))
        return 1;
    return 0;
}

int EBuffer::MoveBlockEnd() {
    if (BE.Col == -1 && BE.Row == -1)
        return 0;
    assert(BE.Col >= 0 && BE.Row >= 0 && BE.Row < RCount);
    if (SetPosR(BE.Col, BE.Row))
        return 1;
    return 0;
}

int EBuffer::MoveFirstNonWhite() {
    int C = 0, P = 0;
    PELine L = VLine(CP.Row);
    
    while (C < L->Count) {
        if (L->Chars[C] == ' ') P++;
        else if (L->Chars[C] == 9) P = NextTab(P, BFI(this, BFI_TabSize));
        else break;
        C++;
    }
    if (SetPos(P, CP.Row) == 0) return 0;
    return 1;
}

int EBuffer::MoveLastNonWhite() {
    int C = LineLen(), P;
    PELine L = VLine(CP.Row);
    
    while (C > 0) {
        if (L->Chars[C - 1] == ' ' || L->Chars[C - 1] == 9) C--;
        else break;
    }
    P = ScreenPos(VLine(CP.Row), C);
    if (SetPos(P, CP.Row) == 0) return 0;
    return 1;
}

int EBuffer::MovePrevEqualIndent() {
    int L = VToR(CP.Row);
    int I = LineIndented(L);
    
    while (--L >= 0)
        if ((RLine(L)->Count > 0) && (LineIndented(L) == I))
            return SetPosR(I, L);
    return 0;
}

int EBuffer::MoveNextEqualIndent() {
    int L = VToR(CP.Row);
    int I = LineIndented(L);
    
    while (L++ < RCount - 1)
        if ((RLine(L)->Count > 0) && (LineIndented(L) == I))
            return SetPosR(I, L);
    return 0;
}

int EBuffer::MoveNextTab() {
    int P = CP.Col;
    
    P = NextTab(P, BFI(this, BFI_TabSize));
    return SetPos(P, CP.Row);
}

int EBuffer::MovePrevTab() {
    int P = CP.Col;
    
    if (P > 0) {
        P = ((P - 1) / BFI(this, BFI_TabSize)) * BFI(this, BFI_TabSize);
        return SetPos(P, CP.Row);
    } else return 0;
}

int EBuffer::MoveLineTop() {
    if (View)
        if (GetVPort()->SetTop(GetVPort()->TP.Col, CP.Row) == 0) return 0;
    return 1;
}

int EBuffer::MoveLineCenter() {
    if (View) {
        int Row = CP.Row - GetVPort()->Rows / 2;
        
        if (Row < 0) Row = 0;
        if (GetVPort()->SetTop(GetVPort()->TP.Col, Row) == 0) return 0;
    }
    return 1;
}

int EBuffer::MoveLineBottom() {
    if (View) {
        int Row = CP.Row - GetVPort()->Rows + 1;
        
        if (Row < 0) Row = 0;
        if (GetVPort()->SetTop(GetVPort()->TP.Col, Row) == 0) return 0;
    }
    return 1;
}

int EBuffer::MovePrevPos() {
    if (PrevPos.Col == -1 || PrevPos.Row == -1) return 0;
    if (SetPosR(PrevPos.Col, PrevPos.Row) == 0) return 0;
    return 1;
}

int EBuffer::MoveSavedPosCol() {
    if (SavedPos.Col == -1) return 0;
    if (SetPos(SavedPos.Col, CP.Row) == 0) return 0;
    return 1;
}

int EBuffer::MoveSavedPosRow() {
    if (SavedPos.Row == -1) return 0;
    if (SetPosR(CP.Col, SavedPos.Row) == 0) return 0;
    return 1;
}

int EBuffer::MoveSavedPos() {
    if (SavedPos.Col == -1 || SavedPos.Row == -1) return 0;
    if (SetPosR(SavedPos.Col, SavedPos.Row) == 0) return 0;
    return 1;
}

int EBuffer::SavePos() {
    SavedPos = CP;
    SavedPos.Row = VToR(CP.Row);
    return 1;
}

int EBuffer::MoveTabStart() {
    PELine X = VLine(CP.Row);
    int C = CharOffset(X, CP.Col);
    
    if (C < X->Count)
        if (X->Chars[C] == 9)
            return SetPos(ScreenPos(X, C), CP.Row);
    return 1;
}

int EBuffer::MoveTabEnd() {
    PELine X = VLine(CP.Row);
    int C = CharOffset(X, CP.Col);
    
    if (C < X->Count)
        if (X->Chars[C] == 9)
            if (ScreenPos(X, C) < CP.Col)
                return SetPos(ScreenPos(X, C + 1), CP.Row);
    return 1;
}

int EBuffer::ScrollLeft(int Cols) {
    int C = GetVPort()->TP.Col;
    if (SetNearPos(CP.Col + Cols, CP.Row, tmLeft) == 0) return 0;
    if (GetVPort()->SetTop(C + Cols, GetVPort()->TP.Row) == 0) return 0;
    return 1;
}

int EBuffer::ScrollRight(int Cols) {  
    int C = GetVPort()->TP.Col;
    if (SetNearPos(CP.Col - Cols, CP.Row, tmLeft) == 0) return 0;
    if (GetVPort()->SetTop(C - Cols, GetVPort()->TP.Row) == 0) return 0;
    return 1;
}

int EBuffer::ScrollDown(int Lines) {
    int L = GetVPort()->TP.Row;
    if (SetNearPos(CP.Col, CP.Row - Lines, tmLeft) == 0) return 0;
    if (GetVPort()->SetTop(GetVPort()->TP.Col, L - Lines) == 0) return 0;
    return 1;
}

int EBuffer::ScrollUp(int Lines) {
    int L = GetVPort()->TP.Row;
    if (SetNearPos(CP.Col, CP.Row + Lines, tmLeft) == 0) return 0;
    if (GetVPort()->SetTop(GetVPort()->TP.Col, L + Lines) == 0) return 0;
    return 1;
}

int EBuffer::MoveBeginOrNonWhite() {
    if (CP.Col == 0)
        return MoveFirstNonWhite();

    return MoveLineStart();
}

int EBuffer::MoveBeginLinePageFile() {
    int L = GetVPort()->TP.Row;

    if (CP.Col == 0 && CP.Row == L)
        return MoveFileStart();
    else if (CP.Col == 0)
        return MovePageStart();

    return MoveLineStart();
}

int EBuffer::MoveEndLinePageFile() {
    int L = GetVPort()->TP.Row + GetVPort()->Rows - 1;
    int Len = LineLen();

    if (CP.Col == Len && CP.Row == L)
        return MoveFileEnd();
    else if (CP.Col == Len)
        if (MovePageEnd() == 0)
            return 0;
    return MoveLineEnd();
}

int EBuffer::KillLine() {
    int Y = VToR(CP.Row);
    
    if (Y == RCount - 1) {
        if (DelText(Y, 0, LineLen())) return 1;
    } else 
        if (DelLine(Y)) return 1;
    return 0;
}

int EBuffer::KillChar() {
    int Y = VToR(CP.Row);
    if (CP.Col < LineLen()) {
        if (DelText(Y, CP.Col, 1)) return 1;
    } else if (LineJoin()) return 1;
    return 0;
}

int EBuffer::KillCharPrev() {
    if (CP.Col == 0) {
        if (CP.Row > 0)
            if (ExposeRow(VToR(CP.Row) - 1) == 0) return 0;
        if (!MoveUp()) return 0;
        if (!MoveLineEnd()) return 0;
        if (LineJoin()) return 1;
    } else {
        if (!MovePrev()) return 0;
        if (DelText(CP.Row, CP.Col, 1)) return 1;
    } 
    return 0;
}

int EBuffer::KillWord() {
    int Y = VToR(CP.Row);
    if (CP.Col >= LineLen()) {
        if (KillChar() == 0) return 0;
    } else {
        PELine L = RLine(Y);
        int P = CharOffset(L, CP.Col);
        int C;
        int Class = ChClassK(L->Chars[P]);
        
        while ((P < L->Count) && (ChClassK(L->Chars[P]) == Class)) P++;
        C = ScreenPos(L, P);
        if (DelText(Y, CP.Col, C - CP.Col) == 0) return 0;
    }
    return 1;
}

int EBuffer::KillWordPrev() {
    int Y = VToR(CP.Row);
    
    if (CP.Col == 0) {
        if (KillCharPrev() == 0) return 0;
    } else if (CP.Col > LineLen()) {
        if (SetPos(LineLen(), CP.Row) == 0) return 0;
    } else {
        PELine L = RLine(Y);
        int P = CharOffset(L, CP.Col);
        int C;
        int Class = ChClassK(L->Chars[P - 1]);
        
        while ((P > 0) && (ChClassK(L->Chars[P - 1]) == Class)) P--;
        C = ScreenPos(L, P);
        if (DelText(Y, C, CP.Col - C) == 0) return 0;
        if (SetPos(C, CP.Row) == 0) return 0;
    }
    return 1;
}

int EBuffer::KillWordOrCap() {
    int Y = VToR(CP.Row);
    if (CP.Col >= LineLen()) {
        if (KillChar() == 0) return 0;
    } else {
        PELine L = VLine(CP.Row);
        int P = CharOffset(L, CP.Col);
        int C;
        int Class = ChClassK(L->Chars[P]);

        if (Class == 1) {
            if (WGETBIT(Flags.CapitalChars, L->Chars[P]) == 1)
                while ((P < L->Count) && (WGETBIT(Flags.CapitalChars, L->Chars[P]) == 1)) P++;
            while ((P < L->Count) && (WGETBIT(Flags.WordChars, L->Chars[P]) == 1) && (WGETBIT(Flags.CapitalChars, L->Chars[P]) == 0)) P++;
        } else while ((P < L->Count) && (ChClassK(L->Chars[P]) == Class)) P++;
        C = ScreenPos(L, P);
        if (DelText(Y, CP.Col, C - CP.Col) == 0) return 0;
    }
    return 1;
}

int EBuffer::KillWordOrCapPrev() {
    int Y = VToR(CP.Row);
    
    if (CP.Col == 0) {
        if (KillCharPrev() == 0) return 0;
    } else if (CP.Col > LineLen()) {
        if (SetPos(LineLen(), CP.Row) == 0) return 0;
    } else {
        PELine L = RLine(Y);
        int P = CharOffset(L, CP.Col);
        int C;
        int Class = ChClassK(L->Chars[P - 1]);

        if (Class == 1) {
            if (WGETBIT(Flags.CapitalChars, L->Chars[P - 1]) == 0)
                while ((P > 0) && (WGETBIT(Flags.WordChars, L->Chars[P - 1]) == 1) && (WGETBIT(Flags.CapitalChars, L->Chars[P - 1]) == 0)) P--;
            while ((P > 0) && (WGETBIT(Flags.CapitalChars, L->Chars[P - 1]) == 1)) P--;
        } else while ((P > 0) && (ChClassK(L->Chars[P - 1]) == Class)) P--;
        C = ScreenPos(L, P);
        if (DelText(Y, C, CP.Col - C) == 0) return 0;
        if (SetPos(C, CP.Row) == 0) return 0;
    }
    return 1;
}

int EBuffer::KillToLineStart() {
    int Y = VToR(CP.Row);
    if (DelText(Y, 0, CP.Col) == 0) return 0;
    if (MoveLineStart() == 0) return 0;
    return 1;
}

int EBuffer::KillToLineEnd() {
    int Y = VToR(CP.Row);
    if (DelText(Y, CP.Col, LineLen() - CP.Col)) return 1;
    return 0;
}

int EBuffer::KillBlock() {
    return BlockKill();
}

int EBuffer::KillBlockOrChar() {
    if (CheckBlock() == 0)
        return KillChar();
    else
        return BlockKill();
}

int EBuffer::KillBlockOrCharPrev() {
    if (CheckBlock() == 0)
        return KillCharPrev();
    else
        return BlockKill();
}

int EBuffer::BackSpace() {
    int Y = VToR(CP.Row);
    
    if (CheckBlock() == 1 && BFI(this, BFI_BackSpKillBlock)) {
        if (BlockKill() == 0)
            return 0;
    } else if (BFI(this, BFI_WordWrap) == 2 && CP.Row > 0 && !IsLineBlank(Y - 1) &&
               CP.Col <= BFI(this, BFI_LeftMargin) && CP.Col <= LineIndented(Y))
    {
        if (SetPos(LineLen(Y - 1), CP.Row - 1) == 0) return 0;
    } else if (CP.Col == 0) {
        if (CP.Row > 0)
            if (ExposeRow(VToR(CP.Row) - 1) == 0) return 0;
        if (MoveUp() == 0) return 0;
        if (MoveLineEnd() == 0) return 0;
        if (LineJoin() == 0) return 0;
    } else {
        if (BFI(this, BFI_BackSpUnindents) && (LineIndented(Y) >= CP.Col)) {
            int C = CP.Col, C1 = 0;
            int L = VToR(CP.Row);
            
            C1 = C;
            while (L > 0 && (IsLineBlank(L - 1) || (C1 = LineIndented(L - 1)) >= C)) L--;
            if (L == 0) C1 = 0;
            if (C1 == C) C1--;
            if (C1 < 0) C1 = 0;
            if (C1 > C) C1 = C;
            if (SetPos(C1, CP.Row) == 0) return 0;
            if (C > LineIndented(Y)) return 0;
            if (DelText(Y, C1, C - C1) == 0) return 0;
        } else if (BFI(this, BFI_BackSpKillTab)) {
            int P;
            int C = CP.Col, C1;
            
            P = CharOffset(RLine(Y), C - 1);
            C1 = ScreenPos(RLine(Y), P);
            if (SetPos(C1, CP.Row) == 0) return 0;
            if (DelText(Y, C1, C - C1) == 0) return 0;
        } else {
            if (MovePrev() == 0) return 0;
            if (DelText(Y, CP.Col, 1) == 0) return 0;
        } 
    }
#ifdef CONFIG_WORDWRAP
    if (BFI(this, BFI_WordWrap) == 2) {
        if (DoWrap(0) == 0) return 0;
    }
#endif
    if (BFI(this, BFI_Trim)) {
        Y = VToR(CP.Row);
        if (TrimLine(Y) == 0) return 0;
    }
    return 1;
}

int EBuffer::Delete() {
    int Y = VToR(CP.Row);
    if (CheckBlock() == 1 && BFI(this, BFI_DeleteKillBlock)) {
        if (BlockKill() == 0)
            return 0;
    } else if (CP.Col < LineLen()) {
        if (BFI(this, BFI_DeleteKillTab)) {
            int P;
            int C = CP.Col, C1;
            
            P = CharOffset(RLine(Y), C);
            C1 = ScreenPos(RLine(Y), P + 1);
            if (DelText(Y, C, C1 - C) == 0) return 0;
        } else 
            if (DelText(Y, CP.Col, 1) == 0) return 0;
    } else 
        if (LineJoin() == 0) return 0;
#ifdef CONFIG_WORDWRAP
    if (BFI(this, BFI_WordWrap) == 2) {
        if (DoWrap(0) == 0) return 0;
        if (CP.Col >= LineLen(Y))
            if (CP.Row + 1 < VCount) {
                if (SetPos(BFI(this, BFI_LeftMargin), CP.Row + 1) == 0) return 0;
            }
    }
#endif
    if (BFI(this, BFI_Trim))
        if (TrimLine(VToR(CP.Row)) == 0)
            return 0;
    return 1;
}

int EBuffer::LineInsert() {
    if (InsLine(VToR(CP.Row), 0)) return 1;
    return 0;
}

int EBuffer::LineAdd() {
    if (InsLine(VToR(CP.Row), 1) && MoveDown()) return 1;
    return 0;
}

int EBuffer::LineSplit() {
    if (SplitLine(VToR(CP.Row), CP.Col) == 0) return 0;
    if (BFI(this, BFI_Trim))
	if (TrimLine(VToR(CP.Row)) == 0) return 0;
    return 1;
}

int EBuffer::LineJoin() {
    if (JoinLine(VToR(CP.Row), CP.Col)) return 1;
    return 0;
}

int EBuffer::LineNew() {
    if (SplitLine(VToR(CP.Row), CP.Col) == 0)
        return 0;
    
    if (!MoveDown())
        return 0;
    
    if (CP.Col > 0) {
        
        if (!MoveLineStart())
            return 0;
        
        //int Indent = LineIndented(VToR(CP.Row));
        
        if (!LineIndent())
            return 0;
        
        //if (Indent > 0)
        //  if (InsText(Row, C, Indent, 0) == 0)
        //    return 0;
        
        if (BFI(this, BFI_Trim))
            if (TrimLine(VToR(CP.Row - 1)) == 0)
                return 0;
    }
    return 1;
}

int EBuffer::LineIndent() {
    int rc = 1;

    if (BFI(this, BFI_AutoIndent)) {
        int L = VToR(CP.Row);
        
        switch (BFI(this, BFI_IndentMode)) {
#ifdef CONFIG_INDENT_C
        case INDENT_C: rc = Indent_C(this, L, 1); break;
#endif
#ifdef CONFIG_INDENT_REXX
        case INDENT_REXX: rc = Indent_REXX(this, L, 1); break;
#endif
#ifdef CONFIG_INDENT_SIMPLE
        case INDENT_SIMPLE: rc = Indent_SIMPLE(this, L, 1); break;
#endif
        default: rc = Indent_Plain(this, L, 1); break;
        }
    }
    if (rc == 0) return 0;
    if (BFI(this, BFI_Trim))
        if (TrimLine(VToR(CP.Row)) == 0) return 0;
    return 1;
}

int EBuffer::LineLen() {
    return LineLen(VToR(CP.Row));
}

int EBuffer::LineCount() {
    assert(1 == 0);
    return RCount;
}

int EBuffer::CLine() {
    assert(1 == 0);
    return VToR(CP.Row);
}

int EBuffer::CColumn() {
    return CP.Col;
}

int EBuffer::InsertChar(char aCh) {
    return InsertString(&aCh, 1);
}

int EBuffer::TypeChar(char aCh) { // does abbrev expansion if appropriate
    if (BFI(this, BFI_InsertKillBlock) == 1)
        if (CheckBlock() == 1)
            if (BlockKill() == 0)
                return 0;
#ifdef CONFIG_ABBREV    //fprintf(stderr, "TypeChar\n");
    if (ChClass(aCh) == 0 && BFI(this, BFI_Abbreviations) == 1) {
        PELine L = VLine(CP.Row);
        int C, P, P1, C1, Len, R;
        char Str[256];
        EAbbrev *ab;
        
        R = VToR(CP.Row);
        C = CP.Col;
        P = CharOffset(L, C);
        if (P >= 0 && P <= L->Count) {
            //fprintf(stderr, "TypeChar 1\n");
            P1 = P;
            C1 = ScreenPos(L, P);
            while ((P > 0) && ((ChClass(L->Chars[P - 1]) == 1) || (L->Chars[P - 1] == '_'))) P--;
            Len = P1 - P;
            C = ScreenPos(L, P);
            assert(C1 - C == Len);
	    if (Len > 0 && Len < int (sizeof(Str))) {
                //fprintf(stderr, "TypeChar 2\n");
                memcpy(Str, L->Chars + P, Len);
                Str[Len] = 0;
                ab = Mode->FindAbbrev(Str);
                if (ab) {
                    //fprintf(stderr, "TypeChar 3\n");
                    if (ab->Replace != 0) {
                        //fprintf(stderr, "TypeChar 4\n");
                        if (DelText(R, C, C1 - C) == 0)
                            return 0;
                        if (ab->Replace) {
                            //fprintf(stderr, "TypeChar 5 %s <- %s\n", ab->Replace, ab->Match);
                            Len = strlen(ab->Replace);
                            if (InsText(R, C, Len, ab->Replace) == 0)
                                return 0;
                            if (SetPos(C + Len, CP.Row) == 0)
                                return 0;
                        } else {
                            if (SetPos(C, CP.Row) == 0)
                                return 0;
                        }
                    } else {
                        if (((EGUI *)gui)->ExecMacro(View->MView->Win, ab->Cmd) == 0)
                            return 0;
                    }
                }
            }
        }
    }
#endif
    return InsertString(&aCh, 1);
}

int EBuffer::InsertString(const char *aStr, size_t aCount) {
    int P;
    int C, L;
    int Y = VToR(CP.Row);
    
    if (BFI(this, BFI_InsertKillBlock) == 1)
        if (CheckBlock() == 1)
            if (BlockKill() == 0)
                return 0;
    
    if (BFI(this, BFI_Insert) == 0)
        if (CP.Col < LineLen())
            if (KillChar() == 0)
                return 0;
    if (InsText(Y, CP.Col, aCount, aStr) == 0)
        return 0;
    C = CP.Col;
    L = VToR(CP.Row);
    P = CharOffset(RLine(L), C);
    P += aCount;
    C = ScreenPos(RLine(L), P);
    if (SetPos(C, CP.Row) == 0)
        return 0;
    if (BFI(this, BFI_Trim))
        if (TrimLine(L) == 0)
            return 0;
#ifdef CONFIG_WORDWRAP
    if (BFI(this, BFI_WordWrap) == 2) {
        if (DoWrap(0) == 0) return 0;
    } else if (BFI(this, BFI_WordWrap) == 1) {
        int P, C = CP.Col;
        PELine LP;
        int L;
        
        if (C > BFI(this, BFI_RightMargin)) {
            L = CP.Row;
            
            C = BFI(this, BFI_RightMargin);
            P = CharOffset(LP = RLine(L), C);
            while ((C > BFI(this, BFI_LeftMargin)) &&
                   ((LP->Chars[P] != ' ') &&
                    (LP->Chars[P] != 9)))
                C = ScreenPos(LP, --P);
            
            if (P <= BFI(this, BFI_LeftMargin)) {
                C = BFI(this, BFI_RightMargin);
            } else
                C = ScreenPos(LP, P);
            if (SplitLine(L, C) == 0) return 0;
            IndentLine(L + 1, BFI(this, BFI_LeftMargin));
            if (SetPos(CP.Col - C - 1 + BFI(this, BFI_LeftMargin), CP.Row + 1) == 0) return 0;
        }
    }
#endif
    return 1;
}

int EBuffer::InsertSpacesToTab(int TSize) {
    int P = CP.Col, P1;
    
    if (BFI(this, BFI_InsertKillBlock) == 1)
        if (CheckBlock() == 1)
            if (BlockKill() == 0)
                return 0;
    
    if (TSize <= 0)
        TSize = BFI(this, BFI_TabSize);
    
    P1 = NextTab(P, TSize);
    if (BFI(this, BFI_Insert) == 0) {
        if (CP.Col < LineLen())
            if (DelText(VToR(CP.Row), CP.Col, P1 - P) == 0) return 0;
    }
    if (InsText(VToR(CP.Row), CP.Col, P1 - P, 0) == 0) return 0;
    if (SetPos(P1, CP.Row) == 0) return 0;
    return 1;
}

int EBuffer::InsertTab() {
    return (BFI(this, BFI_SpaceTabs)) ?
	InsertSpacesToTab(BFI(this, BFI_TabSize)) : InsertChar(9);
}

int EBuffer::InsertSpace() {
    return TypeChar(32);
}

int EBuffer::LineIndented(int Row, const char *indentchars) {
    ELine *l;
    
    if (Row < 0) return 0;
    if (Row >= RCount) return 0;
    l = RLine(Row);
    return ScreenPos(l, LineIndentedCharCount(l, indentchars));
}

int EBuffer::LineIndentedCharCount(ELine *l, const char *indentchars) {
    char *PC;
    int CC, i;

    if (! l)
        return 0;
    if (! indentchars)
        indentchars = " \t";
    CC = l->Count;
    PC = l->Chars;
    for(i = 0; i < CC; i++) {
        if (! strchr(indentchars, PC[i]))
            break;
    }
    return i;
}

int EBuffer::IndentLine(int Row, int Indent) {
    int I, C;
    int Ind = Indent;
    
    if (Row < 0) return 0;
    if (Row >= RCount) return 0;
    if (Indent < 0) Indent = 0;
    I = LineIndented(Row);
    if (Indent != I) {
        if (I > 0) 
            if (DelText(Row, 0, I) == 0) return 0;
        if (Indent > 0) {
            C = 0;
            if (BFI(this, BFI_IndentWithTabs)) {
                char ch = 9;
                
                while (BFI(this, BFI_TabSize) <= Indent) {
                    if (InsText(Row, C, 1, &ch) == 0) return 0;
                    Indent -= BFI(this, BFI_TabSize);
                    C += BFI(this, BFI_TabSize);
                }
            }
            if (Indent > 0)
                if (InsText(Row, C, Indent, 0) == 0) return 0;
        }
    }
    return Ind - I;
}

#ifdef CONFIG_UNDOREDO
int EBuffer::CanUndo() {
    if (BFI(this, BFI_Undo) == 0) return 0;
    if (US.Num == 0 || US.UndoPtr == 0) return 0;
    return 1;
}

int EBuffer::CanRedo() {
    if (BFI(this, BFI_Undo) == 0) return 0;
    if (US.Num == 0 || US.UndoPtr == US.Num) return 0;
    return 1;
}
#endif

int EBuffer::IsLineBlank(int Row) {
    PELine X = RLine(Row);
    int P;
    
    for (P = 0; P < X->Count; P++) 
        if (X->Chars[P] != ' ' && X->Chars[P] != 9)
            return 0;
    return 1;
}

#ifdef CONFIG_WORDWRAP
#define WFAIL(x) return 0	/*do { puts(#x "\x7"); return -1; } while (0) */

int EBuffer::DoWrap(int WrapAll) {
    int L, Len, C, P, Ind;
    PELine LP;
    int Left = BFI(this, BFI_LeftMargin), Right = BFI(this, BFI_RightMargin);
    int FirstParaLine;
    int NoChange = 0, NoChangeX = 0;
    
    if (Left >= Right) return 0;
    
    L = VToR(CP.Row);
    
    FirstParaLine = 0;
    if (L > 0)
        if (IsLineBlank(L - 1)) FirstParaLine = L;
    
    while (L < RCount) {
        NoChange = 1;
        
        if (VToR(CP.Row) != L || L != FirstParaLine) {
            if (VToR(CP.Row) == L)
                if (CP.Col <= LineIndented(L))
                    if (SetPos(Left, CP.Row) == 0) WFAIL(1);
            Ind = IndentLine(L, Left);
            if (VToR(CP.Row) == L)
                if (SetPos((CP.Col + Ind > 0) ? CP.Col + Ind : 0, CP.Row) == 0) WFAIL(2);
            NoChange = 0;
        }
        Len = LineLen(L);
        
        if (IsLineBlank(L)) break;
        
        if (Len < Right) {
            int firstwordbeg = -1;
            int firstwordend = -1;
            int X;
            PELine lp;
            
            if (L < RCount - 1) {
                IndentLine(L + 1, 0);
                if ((ScreenPos(RLine(L + 1), RLine(L + 1)->Count) == 0) ||
                    (RLine(L + 1)->Chars[0] == '>') || (RLine(L + 1)->Chars[0] == '<')) break;
            } else
                break;
            if (L + 1 >= RCount) break;
            
            lp = RLine(L + 1);
            for (X = 0; X < lp->Count; X++) {
                if (firstwordbeg == -1 && 
                    ((lp->Chars[X] != ' ') && (lp->Chars[X] != '\t')))
                {
                    firstwordbeg = X;
                } else if (firstwordend == -1 &&
                           ((lp->Chars[X] == ' ' || lp->Chars[X] == '\t')))
                {
                    firstwordend = X - 1;
                }
            }
            if (firstwordbeg != -1)
                if (firstwordend == -1)
                    firstwordend = lp->Count;
            
            if (firstwordend == -1) break;
            if (Right - Len > firstwordend - firstwordbeg) {
                if (JoinLine(L, Len + 1) == 0) WFAIL(3);
                NoChange = 0;
                continue;
            } else
                IndentLine(L + 1, Left);
        } else if (Len > Right) {
            C = Right;
            P = CharOffset(LP = RLine(L), C);
            while ((C > Left) && 
                   ((LP->Chars[P] != ' ') &&
                    (LP->Chars[P] != 9)))
                C = ScreenPos(LP, --P);
            
            if (P <= Left) {
                L++;
                continue;
            }
            C = ScreenPos(LP, P);
            if (SplitLine(L, C) == 0) WFAIL(4);
            IndentLine(L + 1, Left);
            if (L < RCount - 2 && LineLen(L + 1) == Left) {
                if (!IsLineBlank(L + 2)) {
                    if (JoinLine(L + 1, Left) == 0) WFAIL(5);
                }
            }
            if (L == VToR(CP.Row) && CP.Col > C) {
                if (SetPos(Left + CP.Col - C - 1, CP.Row + 1) == 0) WFAIL(6);
            }
            NoChange = 0;
            L++;
            continue;
        }
        if (WrapAll == 0)
            if (NoChangeX) {
                //printf("\n\nBreak OUT = %d\n\x7", L);
                break;
            }
        L++;
        NoChangeX = NoChange;
    }
    if (WrapAll == 1)
        if (SetPosR(Left,
                    (L < RCount - 2) ? (L + 2) :
                    (L < RCount - 1) ? (L + 1) :
                    (RCount - 1)) == 0) WFAIL(7);
    return 1;
}

int EBuffer::WrapPara() {
    while (VToR(CP.Row) < RCount - 1 && IsLineBlank(VToR(CP.Row)))
        if (SetPos(CP.Col, CP.Row + 1) == 0) return 0;
    return DoWrap(1);
}
#endif

int EBuffer::LineCenter() {
    if (LineTrim() == 0)
        return 0;
    int ind = LineIndented(VToR(CP.Row));
    int left = BFI(this, BFI_LeftMargin);
    int right = BFI(this, BFI_RightMargin);
    int len = LineLen();

    //int chs = len - ind;
    int newind = left + ((right - left) - (len - ind)) / 2;
    if (newind < left)
        newind = left;
    return IndentLine(VToR(CP.Row), newind);
}

int EBuffer::InsPrevLineChar() {
    int L = VToR(CP.Row);
    int C = CP.Col, P;
    
    if (L > 0) {
        L--;
        if (C < LineLen(L)) {
            P = CharOffset(RLine(L), C);
            return InsertChar(RLine(L)->Chars[P]);
        }
    }
    return 0;
}

int EBuffer::InsPrevLineToEol() {
    int L = VToR(CP.Row);
    int C = CP.Col, P;
    int Len;
    
    if (L > 0) {
        L--;
        P = CharOffset(RLine(L), C);
        Len = RLine(L)->Count - P;
        if (Len > 0)
            return InsertString(RLine(L)->Chars + P, Len);
    }
    return 0;
}

int EBuffer::LineDuplicate() {
    int Y = VToR(CP.Row);
    if (InsLine(Y, 1) == 0) return 0;
    if (InsChars(Y + 1, 0, RLine(Y)->Count, RLine(Y)->Chars) == 0) return 0;
    return 1;
}

int EBuffer::TrimLine(int Row) {
    PELine L = RLine(Row);
    int P, X, E;
    
    if (L->Count == 0) return 1;
    P = L->Count;
    while ((P > 0) && ((L->Chars[P - 1] == ' ') || (L->Chars[P - 1] == 9)))
        P--;
    X = ScreenPos(L, P);
    E = ScreenPos(L, L->Count);
    if (E - X > 0) 
        if (DelText(Row, X, E - X, 1) == 0) return 0;
    return 1;
}

int EBuffer::LineTrim() {
    return TrimLine(VToR(CP.Row));
}

int EBuffer::FileTrim() {
    for (int L = 0; L < RCount; L++)
        if (TrimLine(L) == 0)
            return 0;
    return 1;
}

int EBuffer::BlockTrim() {
    EPoint B, E;
    int L;
    
    AutoExtend = 0;
    if (CheckBlock() == 0) return 0;
    if (RCount <= 0) return 0;
    B = BB;
    E = BE;
    Draw(B.Row, E.Row);
    for (L = B.Row; L <= E.Row; L++) {
        switch (BlockMode) {
        case bmStream:
            if (L < E.Row || E.Col != 0)
                if (TrimLine(L) == 0)
                    return 0;
            break;
        case bmLine:
        case bmColumn:
            if (L < E.Row)
                if (TrimLine(L) == 0)
                    return 0;
            break;
        }
    }
    return 1;
}

#define TOGGLE(x) \
    Flags.num[BFI_##x] = (Flags.num[BFI_##x]) ? 0 : 1; \
    /*Msg(INFO, #x " is now %s.", Flags.num[BFI_##x] ? "ON" : "OFF");*/ \
    return 1;

#define TOGGLE_R(x) \
    Flags.num[BFI_##x] = (Flags.num[BFI_##x]) ? 0 : 1; \
    /*Msg(INFO, #x " is now %s.", Flags.num[BFI_##x] ? "ON" : "OFF");*/ \
    FullRedraw(); \
    return 1;


int EBuffer::ToggleAutoIndent() { TOGGLE(AutoIndent); }
int EBuffer::ToggleInsert() { TOGGLE(Insert); }
int EBuffer::ToggleExpandTabs() { TOGGLE_R(ExpandTabs); }
int EBuffer::ToggleShowTabs() { TOGGLE_R(ShowTabs); }
int EBuffer::ToggleUndo() {
#ifdef CONFIG_UNDOREDO
    FreeUndo();
    TOGGLE(Undo);
#endif
}
int EBuffer::ToggleReadOnly() { TOGGLE(ReadOnly); }
int EBuffer::ToggleKeepBackups() { TOGGLE(KeepBackups); }
int EBuffer::ToggleMatchCase() { TOGGLE(MatchCase); }
int EBuffer::ToggleBackSpKillTab() { TOGGLE(BackSpKillTab); }
int EBuffer::ToggleDeleteKillTab() { TOGGLE(DeleteKillTab); }
int EBuffer::ToggleSpaceTabs() { TOGGLE(SpaceTabs); }
int EBuffer::ToggleIndentWithTabs() { TOGGLE(IndentWithTabs); }
int EBuffer::ToggleBackSpUnindents() { TOGGLE(BackSpUnindents); }
int EBuffer::ToggleTrim() { TOGGLE(Trim); }
int EBuffer::ToggleShowMarkers() { TOGGLE_R(ShowMarkers); }
int EBuffer::ToggleHilitTags() { TOGGLE_R(HilitTags); }
int EBuffer::ToggleShowBookmarks() { TOGGLE_R(ShowBookmarks); }
int EBuffer::ToggleMakeBackups() { TOGGLE(MakeBackups); }

int EBuffer::ToggleWordWrap() { 
    BFI(this, BFI_WordWrap) = (BFI(this, BFI_WordWrap) + 1) % 3;
    /*Msg(INFO,
        "WordWrap is now %s.",
        (BFI(this, BFI_WordWrap) == 2) ? "AUTO" :
       (BFI(this, BFI_WordWrap) == 1) ? "ON" : "OFF"); */
    return 1;
}

int EBuffer::SetLeftMargin() {
    BFI(this, BFI_LeftMargin) = CP.Col;
    Msg(S_INFO, "LeftMargin set to %d.", BFI(this, BFI_LeftMargin) + 1);
    return 1;
}

int EBuffer::SetRightMargin() {
    BFI(this, BFI_RightMargin) = CP.Col;
    Msg(S_INFO, "RightMargin set to %d.", BFI(this, BFI_RightMargin) + 1);
    return 1;
}

int EBuffer::ChangeMode(const char *AMode) {
    if (FindMode(AMode) != 0) {
        Mode = FindMode(AMode);
        Flags = Mode->Flags;
#ifdef CONFIG_SYNTAX_HILIT
	HilitProc = 0;
        if (Mode && Mode->fColorize)
            HilitProc = GetHilitProc(Mode->fColorize->SyntaxParser);
#endif
	FullRedraw();
        return 1;
    }
    Msg(S_ERROR, "Mode '%s' not found.", AMode);
    return 0;
}

int EBuffer::ChangeKeys(const char *AMode) {
    if (FindMode(AMode) != 0) {
        Mode = FindMode(AMode);
#ifdef CONFIG_SYNTAX_HILIT
        HilitProc = 0;
        if (Mode && Mode->fColorize)
            HilitProc = GetHilitProc(Mode->fColorize->SyntaxParser);
#endif
	FullRedraw();
        return 1;
    }
    Msg(S_ERROR, "Mode '%s' not found.", AMode);
    return 0;
}

int EBuffer::ChangeFlags(const char *AMode) {
    if (FindMode(AMode) != 0) {
        EMode *XMode;
        XMode = FindMode(AMode);
        Flags = XMode->Flags;
#ifdef CONFIG_SYNTAX_HILIT
        HilitProc = 0;
        if (Mode && Mode->fColorize)
            HilitProc = GetHilitProc(Mode->fColorize->SyntaxParser);
#endif
	FullRedraw();
        return 1;
    }
    Msg(S_ERROR, "Mode '%s' not found.", AMode);
    return 0;
}
