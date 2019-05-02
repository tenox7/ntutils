/*    h_ipf.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "fte.h"

#ifdef CONFIG_HILIT_IPF

#include "c_bind.h"
#include "o_buflist.h"

#include <ctype.h>

#define hsIPF_Normal  0
#define hsIPF_Symbol  1
#define hsIPF_Tag     2
#define hsIPF_Control 3
#define hsIPF_String  4

int Hilit_IPF(EBuffer *BF, int /*LN*/, PCell B, int Pos, int Width, ELine *Line, hlState &State, hsState *StateMap, int *ECol) {
    HILIT_VARS(BF->Mode->fColorize->Colors, Line);
    int j;

    C = 0;
    NC = 0;
    for (i = 0; i < Line->Count;) {
        IF_TAB() else {
            switch (State) {
            case hsIPF_Normal:
                Color = CLR_Normal;
                if (i == 0 && *p == '.') {
                    State = hsIPF_Control;
                    Color = CLR_Control;
                } else if (*p == ':') {
                    State = hsIPF_Tag;
                    Color = CLR_Tag;
                } else if (*p == '&') {
                    State = hsIPF_Symbol;
                    Color = CLR_Symbol;
                }
                goto hilit;
            case hsIPF_Tag:
                Color = CLR_Tag;
                if (isalpha(*p) || *p == '_') {
                    j = 0;
                    while (((i + j) < Line->Count) &&
                           (isalnum(Line->Chars[i+j]) ||
                            (Line->Chars[i + j] == '_'))
                          ) j++;
                    if (BF->GetHilitWord(Color, &Line->Chars[i], j, 1)) {
                    }
                    if (StateMap)
                        memset(StateMap + i, State, j);
                    if (B)
                        MoveMem(B, C - Pos, Width, Line->Chars + i, HILIT_CLRD(), j);
                    i += j;
                    len -= j;
                    p += j;
                    C += j;
                    Color = CLR_Tag;
                    continue;
                } else if (*p == '\'') {
                    State = hsIPF_String;
                    Color = CLR_String;
                    goto hilit;
                } else if (*p == '.') {
                    ColorNext();
                    State = hsIPF_Normal;
                    continue;
                }
                goto hilit;
            case hsIPF_String:
                Color = CLR_String;
                if (*p == '\'') {
                    ColorNext();
                    State = hsIPF_Tag;
                    continue;
                }
                goto hilit;
            case hsIPF_Symbol:
                Color = CLR_Symbol;
                if (*p == '.') {
                    ColorNext();
                    State = hsIPF_Normal;
                    continue;
                }
                goto hilit;
            case hsIPF_Control:
                State = hsIPF_Control;
                Color = CLR_Control;
                goto hilit;
            default:
                State = hsIPF_Normal;
                Color = CLR_Normal;
            hilit:
                ColorNext();
                continue;
            }
        }
    }
    if (State == hsIPF_Symbol || State == hsIPF_Control || State == hsIPF_String)
        State = hsIPF_Normal;
    *ECol = C;
    return 0;
}
#endif
