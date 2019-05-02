/*    e_trans.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "c_history.h"
#include "i_modelview.h"
#include "i_view.h"
#include "o_buflist.h"
#include "s_util.h"

#include <ctype.h>

// FLAW: NULL characters can not be translated, need escaping
int ParseTrans(unsigned char *S, unsigned char *D, TransTable tab) {
    unsigned char Dest[512];
    unsigned char A, B;
    unsigned int i;

    if (S == 0 || D == 0)
        return 0;

    strncpy((char *)Dest, (char *)D, sizeof(Dest) - 1); Dest[sizeof(Dest) - 1] = 0;
    D = Dest;

    // no translation
    for (i = 0; i < 256; i++)
        tab[i] = (unsigned char)i;

    while (*S && *D) {
        if (S[0] && S[1] == '-' && S[2]) {
            if (S[0] <= S[2]) {
                A = (*S)++;
                if (S[0] >= S[2])
                    S += 2;
            } else {
                A = (*S)--;
                if (S[0] <= S[2])
                    S += 2;
            }
        } else {
            A = *S++;
        }
        if (D[0] && D[1] == '-' && D[2]) {
            if (D[0] <= D[2]) {
                B = (*D)++;
                if (D[0] >= D[2])
                    D += 2;
            } else {
                B = (*D)--;
                if (D[0] <= D[2])
                    D += 2;
            }
        } else {
            B = *D++;
        }
        tab[A] = B;
    }
    if (*S != *D) // one was too short
        return 0;
    return 1;
}

int MakeTrans(TransTable tab, int What) {
    int i;

    // no translation
    for (i = 0; i <= 255; i++)
        tab[i] = (unsigned char)i;

    switch (What) {
    case ccToggle:
    case ccUp:
        for (i = 33; i <= 255; i++)
            if (isalpha(i) && (toupper(i) != i))
                tab[i] = (unsigned char) toupper(i);
        if (What != ccToggle)
            break;
    case ccDown:
        for (i = 33; i <= 255; i++)
            if (isalpha(i) && (i == tab[i]) && (tolower(i) != i))
                tab[i] = (unsigned char) tolower(i);
        break;
    default:
        return 0;
    }
    return 1;
}

int EBuffer::BlockTrans(TransTable tab) {
    int L, I, B, E;
    PELine LL;

    if (CheckBlock() == 0) return 0;
    if (RCount == 0) return 0;

    for (L = BB.Row; L <= BE.Row; L++) {
        LL = RLine(L);
        B = 0;
        E = 0;
        switch (BlockMode) {
        case bmLine:
            if (L == BE.Row)
                E = 0;
            else
                E = LL->Count;
            break;
        case bmColumn:
            if (L == BE.Row)
                E = 0;
            else {
                B = CharOffset(LL, BB.Col);
                E = CharOffset(LL, BE.Col);
            }
            break;
        case bmStream:
            if (L == BB.Row && L == BE.Row) {
                B = CharOffset(LL, BB.Col);
                E = CharOffset(LL, BE.Col);
            } else if (L == BB.Row) {
                B = CharOffset(LL, BB.Col);
                E = LL->Count;
            } else if (L == BE.Row) {
                B = 0;
                E = CharOffset(LL, BE.Col);
            } else {
                B = 0;
                E = LL->Count;
            }
            break;
        }
        if (B > LL->Count)
            B = LL->Count;
        if (E > LL->Count)
            E = LL->Count;
        if (E > B) {
            if (ChgChars(L, B, E - B, 0) == 0) return 0;
            for (I = B; I < E; I++)
                LL->Chars[I] = tab[(unsigned char)LL->Chars[I]];
        }
    }
    Draw(BB.Row, BE.Row);
    return 1;
}

int EBuffer::CharTrans(TransTable tab) {
    PELine L = VLine(CP.Row);
    unsigned int P = CharOffset(L, CP.Col);

    if (P >= (unsigned int)L->Count) return 0;
    if (ChgChars(CP.Row, P, 1, 0) == 0) return 0;
    L->Chars[P] = tab[(unsigned char)L->Chars[P]];
    return 1;
}

int EBuffer::LineTrans(TransTable tab) {
    PELine L = VLine(CP.Row);
    int I;

    if (L->Count > 0) {
        if (ChgChars(CP.Row, 0, L->Count, 0) == 0) return 0;
        for (I = 0; I < L->Count; I++)
            L->Chars[I] = tab[(unsigned char)L->Chars[I]];
    }
    return 1;
}

int EBuffer::CharCaseUp() {
    TransTable tab;

    MakeTrans(tab, ccUp);
    return CharTrans(tab);
}

int EBuffer::CharCaseDown() {
    TransTable tab;

    MakeTrans(tab, ccDown);
    return CharTrans(tab);
}

int EBuffer::CharCaseToggle() {
    TransTable tab;

    MakeTrans(tab, ccToggle);
    return CharTrans(tab);
}

int EBuffer::LineCaseUp() {
    TransTable tab;

    MakeTrans(tab, ccUp);
    return LineTrans(tab);
}

int EBuffer::LineCaseDown() {
    TransTable tab;

    MakeTrans(tab, ccDown);
    return LineTrans(tab);
}

int EBuffer::LineCaseToggle()  {
    TransTable tab;

    MakeTrans(tab, ccToggle);
    return LineTrans(tab);
}

int EBuffer::BlockCaseUp() {
    TransTable tab;

    MakeTrans(tab, ccUp);
    return BlockTrans(tab);
}

int EBuffer::BlockCaseDown() {
    TransTable tab;

    MakeTrans(tab, ccDown);
    return BlockTrans(tab);
}

int EBuffer::BlockCaseToggle() {
    TransTable tab;

    MakeTrans(tab, ccToggle);
    return BlockTrans(tab);
}

int EBuffer::GetTrans(ExState &State, TransTable tab) {
    unsigned char TrS[512] = "";
    unsigned char TrD[512] = "";

    if (State.GetStrParam(View, (char *)TrS, sizeof(TrS)) == 0)
        if (View->MView->Win->GetStr("Trans From", sizeof(TrS), (char *)TrS, HIST_TRANS) == 0)
            return 0;
    if (State.GetStrParam(View, (char *)TrD, sizeof(TrD)) == 0)
        if (View->MView->Win->GetStr("Trans To", sizeof(TrS), (char *)TrD, HIST_TRANS) == 0)
            return 0;
    if (ParseTrans(TrS, TrD, tab) == 0) {
        Msg(S_ERROR, "Bad Trans Arguments %s %s.", TrS, TrD);
        return 0;
    }
    return 1;
}

int EBuffer::CharTrans(ExState &State) {
    TransTable tab;

    if (GetTrans(State, tab) == 0)
        return 0;
    return CharTrans(tab);
}

int EBuffer::LineTrans(ExState &State) {
    TransTable tab;

    if (GetTrans(State, tab) == 0)
        return 0;
    return LineTrans(tab);
}

int EBuffer::BlockTrans(ExState &State) {
    TransTable tab;

    if (GetTrans(State, tab) == 0)
        return 0;
    return BlockTrans(tab);
}
