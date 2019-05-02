/*    h_ada.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "fte.h"

#ifdef CONFIG_HILIT_ADA

#include "c_bind.h"
#include "o_buflist.h"

#include <ctype.h>

#define hsAda_Normal       0
#define hsAda_Comment      1
#define hsAda_CommentL     2
#define hsAda_Keyword      4
#define hsAda_String1     10
#define hsAda_String2     11

int Hilit_ADA(EBuffer *BF, int /*LN*/, PCell B, int Pos, int Width, ELine *Line, hlState &State, hsState *StateMap, int *ECol) {
    int j = 0;
    int firstnw = 0;
    HILIT_VARS(BF->Mode->fColorize->Colors, Line);
    int len1 = len;
//    char *last = p + len1 - 1;

    C = 0;
    NC = 0;
    for(i = 0; i < Line->Count;) {
        if (*p != ' ' && *p != 9) firstnw++;
        IF_TAB() else {
            switch (State) {
            default:
            case hsAda_Normal:
                if (isalpha(*p) || *p == '_') {
                    j = 0;
                    while (((i + j) < Line->Count) &&
                           (isalnum(Line->Chars[i+j]) ||
                            (Line->Chars[i + j] == '_') ||
                            (Line->Chars[i + j] == '\''))
                          ) j++;
                    if (BF->GetHilitWord(Color, &Line->Chars[i], j, 1)) {
                        State = hsAda_Keyword;
                    } else {
                        int x;
                        x = i + j;
                        while ((x < Line->Count) &&
                               ((Line->Chars[x] == ' ') || (Line->Chars[x] == 9))) x++;
                        if ((x < Line->Count) && (Line->Chars[x] == '(')) {
                            Color = CLR_Function;
                        } else {
                            Color = CLR_Normal;
                        }
                        State = hsAda_Normal;
                    }
                    if (StateMap)
                        memset(StateMap + i, State, j);
                    if (B)
                        MoveMem(B, C - Pos, Width, Line->Chars + i, HILIT_CLRD(), j);
                    i += j;
                    len -= j;
                    p += j;
                    C += j;
                    State = hsAda_Normal;
                    continue;
                } else if ((len >= 2) && (*p == '-') && (*(p+1) == '-')) {
                    State = hsAda_CommentL;
                    Color = CLR_Comment;
                //hilit2:
                    ColorNext();
                hilit:
                    ColorNext();
                    continue;
                } else if (isdigit(*p)) {
                    Color = CLR_Number;
                    ColorNext();
                    while (len && (isdigit(*p) || *p == 'e' || *p == 'E' || *p == '.' || *p == '_')) ColorNext();
                    continue;
                } else if (*p == '\'') {
                    State = hsAda_String1;
                    Color = CLR_String;
                    goto hilit;
                } else if (*p == '"') {
                    State = hsAda_String2;
                    Color = CLR_String;
                    goto hilit;
                } else if (ispunct(*p) && *p != '_') {
                    Color = CLR_Punctuation;
                    goto hilit;
                }
                Color = CLR_Normal;
                goto hilit;
            case hsAda_CommentL:
                Color = CLR_Comment;
                goto hilit;
            case hsAda_String1:
                Color = CLR_String;
                if (*p == '\'') {
                    ColorNext();
                    State = hsAda_Normal;
                    continue;
                }
                goto hilit;
            case hsAda_String2:
                Color = CLR_String;
                if (*p == '"') {
                    ColorNext();
                    State = hsAda_Normal;
                    continue;
                }
                goto hilit;
            }
        }
    }
    if (State == hsAda_CommentL)
        State = hsAda_Normal;
    if ((len1 == 0)) {
        switch (State) {
        case hsAda_String1:
        case hsAda_String2:
            State = hsAda_Normal;
            break;
        }
    }
    *ECol = C;
    return 0;
}
#endif // CONFIG_HILIT_ADA
