/*    h_plain.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "fte.h"
#include "c_bind.h"
#include "o_buflist.h"

#include <ctype.h>

#define hsPLAIN_Normal 0

int Hilit_Plain(EBuffer *BF, int /*LN*/, PCell B, int Pos, int Width, ELine* Line, hlState& State, hsState *StateMap, int *ECol) {
    HILIT_VARS(BF->Mode->fColorize->Colors, Line);

#ifdef CONFIG_WORD_HILIT
    int j = 0;
    
    if (BF->Mode->fColorize->Keywords.TotalCount > 0 ||
        BF->GetWordCount() > 0)
    { /* words have to be hilited, go slow */
        for(i = 0; i < Line->Count;) {
            IF_TAB() else {
                if (isalpha(*p) || (*p == '_')) {
                    j = 0;
                    while (((i + j) < Line->Count) &&
                           (isalnum(Line->Chars[i+j]) ||
                            (Line->Chars[i + j] == '_'))
                          ) j++;
                    if (BF->GetHilitWord(Color, Line->Chars + i, j, 1)) ;
                    else {
                        Color = CLR_Normal;
                        State = hsPLAIN_Normal;
                    }
                    if (StateMap)
                        memset(StateMap + i, State, j);
                    if (B)
                        MoveMem(B, C - Pos, Width, Line->Chars + i, HILIT_CLRD(), j);
                    i += j;
                    len -= j;
                    p += j;
                    C += j;
                    State = hsPLAIN_Normal;
                    Color = CLR_Normal;
                    continue;
                }
                ColorNext();
                continue;
            }
        }
    } else
#endif // CONFIG_WORD_HILIT
    if (ExpandTabs) { /* use slow mode */
        for (i = 0; i < Line->Count;) {
            IF_TAB() else {
                ColorNext();
            }
        }
    } else { /* fast mode */
        if (Pos < Line->Count) {
            if (Pos + Width < Line->Count) {
                if (B) 
                    MoveMem(B, 0, Width, Line->Chars + Pos, HILIT_CLRD(), Width);
                if (StateMap)
                    memset(StateMap, State, Line->Count);
            } else {
                if (B) 
                    MoveMem(B, 0, Width, Line->Chars + Pos, HILIT_CLRD(), Line->Count - Pos);
                if (StateMap)
                    memset(StateMap, State, Line->Count);
            }
        }
        C = Line->Count;
    }
    *ECol = C;
    State = 0;
    return 0;
}

int Indent_Plain(EBuffer *B, int Line, int PosCursor) {
    int OI = B->LineIndented(Line);
    B->IndentLine(Line, B->LineIndented(Line - 1));
    if (PosCursor) {
        int I = B->LineIndented(Line);
        int X = B->CP.Col;

        X = X - OI + I;
        if (X < I) X = I;
        if (X < 0) X = 0;
        B->SetPosR(X, Line);
    }
    return 1;
}
