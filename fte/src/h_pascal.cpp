/*    h_pascal.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "fte.h"

#ifdef CONFIG_HILIT_PASCAL

#include "c_bind.h"
#include "o_buflist.h"

#include <ctype.h>

#define hsPas_Normal       0
#define hsPas_Comment1     1
#define hsPas_Comment2     2
#define hsPas_Keyword      3
#define hsPas_String1      4
#define hsPas_String2      5

int Hilit_PASCAL(EBuffer *BF, int /*LN*/, PCell B, int Pos, int Width, ELine *Line, hlState &State, hsState *StateMap, int *ECol) {
    int j = 0;
    int firstnw = 0;
    HILIT_VARS(BF->Mode->fColorize->Colors, Line);

    C = 0;
    NC = 0;
    for(i = 0; i < Line->Count;) {
        if (*p != ' ' && *p != 9) firstnw++;
        IF_TAB() else {
            switch(State) {
            default:
            case hsPas_Normal:
                if (isalpha(*p) || *p == '_') {
                    j = 0;
                    while (((i + j) < Line->Count) &&
                           (isalnum(Line->Chars[i+j]) ||
                            (Line->Chars[i + j] == '_'))
                          ) j++;
                    if (BF->GetHilitWord(Color, &Line->Chars[i], j, 1)) {
                        //                        Color = hcC_Keyword;
                        State = hsPas_Keyword;
                    } else {
                        Color = CLR_Normal;
                        State = hsPas_Normal;
                    }
                    if (StateMap)
                        memset(StateMap + i, State, j);
                    if (B)
                        MoveMem(B, C - Pos, Width, Line->Chars + i, HILIT_CLRD(), j);
                    i += j;
                    len -= j;
                    p += j;
                    C += j;
                    State = hsPas_Normal;
                    continue;
                } else if ((len >= 2) && (*p == '(') && (*(p+1) == '*')) {
                    State = hsPas_Comment1;
                    Color = CLR_Comment;
                    ColorNext();
                    goto hilit;
                } else if (*p == '{') {
                    State = hsPas_Comment2;
		    Color = CLR_Comment;
		    goto hilit;
                } else if (*p == '$') {
		    Color = CLR_HexNumber;
		    ColorNext();
                    ColorNext();
                    while (len && isxdigit(*p)) ColorNext();
                    continue;
                } else if (isdigit(*p)) {
                    Color = CLR_Number;
                    ColorNext();
                    while (len && (isdigit(*p) || *p == 'e' || *p == 'E' || *p == '.')) ColorNext();
                    continue;
                } else if (*p == '\'') {
                    State = hsPas_String1;
                    Color = CLR_String;
                    goto hilit;
                } else if (*p == '"') {
                    State = hsPas_String2;
                    Color = CLR_String;
                    goto hilit;
                } else if (ispunct(*p) && *p != '_') {
                    Color = CLR_Punctuation;
                    goto hilit;
                }
                Color = CLR_Normal;
                goto hilit;
            case hsPas_Comment1:
                Color = CLR_Comment;
                if ((len >= 2) && (*p == '*') && (*(p+1) == ')')) {
                    ColorNext();
                    ColorNext();
                    State = hsPas_Normal;
                    continue;
                }
                goto hilit;
            case hsPas_Comment2:
                Color = CLR_Comment;
                if (*p == '}') {
                    ColorNext();
                    State = hsPas_Normal;
                    continue;
                }
                goto hilit;
            case hsPas_String1:
                Color = CLR_String;
                if (*p == '\'') {
                    ColorNext();
                    State = hsPas_Normal;
                    continue;
                }
                goto hilit;
            case hsPas_String2:
                Color = CLR_String;
                if (*p == '"') {
                    ColorNext();
                    State = hsPas_Normal;
                    continue;
                }
            hilit:
                ColorNext();
                continue;
            }
        }
    }
    switch(State) {
    case hsPas_String1:
    case hsPas_String2:
        State = hsPas_Normal;
        break;
    }
    *ECol = C;
    return 0;
}
#endif
