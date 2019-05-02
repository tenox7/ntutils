/*    h_msg.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "fte.h"

#ifdef CONFIG_HILIT_MSG

#include "c_bind.h"
#include "o_buflist.h"

#include <ctype.h>

#define hsMSG_Normal  0
#define hsMSG_Header  1
#define hsMSG_Quote   2
#define hsMSG_Tag     3
#define hsMSG_Control 4

int Hilit_MSG(EBuffer *BF, int /*LN*/, PCell B, int Pos, int Width, ELine* Line, hlState& State, hsState *StateMap, int *ECol) {
    HILIT_VARS(BF->Mode->fColorize->Colors, Line);
    int is_head = 0, is_quote = 0, is_space = 0, is_tag = 0, is_control = 0;
    
    if (Line->Count > 0) {
        if (State == hsMSG_Header) {
            if (Line->Chars[0] == ' ' || Line->Chars[0] == '\t') is_head = 1;
            else
                State = hsMSG_Normal;
        }
        if (State == hsMSG_Normal) {
            if (Line->Count >= 2 &&
                Line->Chars[0] == '-' &&
                Line->Chars[1] == '-' &&
                (Line->Count == 2 || Line->Chars[2] == ' '))
                is_tag = 1;
            else if (Line->Count >= 2 &&
                     Line->Chars[0] == '.' &&
                     Line->Chars[1] == '.' &&
                     (Line->Count == 2 || Line->Chars[2] == ' '))
                is_tag = 1;
            else if (Line->Count >= 3 &&
                     Line->Chars[0] == '-' &&
                     Line->Chars[1] == '-' &&
                     Line->Chars[2] == '-' &&
                     (Line->Count == 3 || Line->Chars[3] == ' '))
                is_tag = 1;
            else if (Line->Count >= 3 &&
                     Line->Chars[0] == '.' &&
                     Line->Chars[1] == '.' &&
                     Line->Chars[2] == '.' &&
                     (Line->Count == 3 || Line->Chars[3] == ' '))
                is_tag = 1;
            else if (Line->Count > 10 && memcmp(Line->Chars, " * Origin:", 10) == 0)
                is_control = 1;
            else if (Line->Count > 0 && Line->Chars[0] == '\x01')
                is_control = 1;
            else for (i = 0; i < Line->Count; i++) {
                if (i < 30 && Line->Chars[i] == ':' && i < Line->Count - 1 && Line->Chars[i+1] == ' ' && !is_space) { is_head = 1; break; }
                else if (i < 5 && Line->Chars[i] == '>') { is_quote = 1; break; }
                else if (Line->Chars[i] == '<' ||
                         (Line->Chars[i] == ' ' && i > 0) ||
                         Line->Chars[i] == '\t') break;
                else if (Line->Chars[i] == ' ' || Line->Chars[i] == '\t')
                    is_space = 0;
            }
        }   
    }
    if (is_head) {
        State = hsMSG_Header;
        Color = CLR_Header;
    } else if (is_quote) {
        State = hsMSG_Quote;
        Color = CLR_Quotes;
    } else if (is_tag) {
        State = hsMSG_Tag;
        Color = CLR_Tag;
    } else if (is_control) {
        State = hsMSG_Control;
        Color = CLR_Control;
    } else {
        State = hsMSG_Normal;
        Color = CLR_Normal;
    }

    ChColor DefColor = Color;
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
                        Color = DefColor;
                    }
                    if (StateMap)
                        memset(StateMap + i, State, j);
                    if (B)
                        MoveMem(B, C - Pos, Width, Line->Chars + i, HILIT_CLRD(), j);
                    i += j;
                    len -= j;
                    p += j;
                    C += j;
                    Color = DefColor;
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
                    MoveMem(B, 0, Width, Line->Chars + Pos, Color, Width);
                if (StateMap)
                    memset(StateMap, State, Line->Count);
            } else {
                if (B)
                    MoveMem(B, 0, Width, Line->Chars + Pos, Color, Line->Count - Pos);
                if (StateMap)
                    memset(StateMap, State, Line->Count);
            }
        }
        C = Line->Count;
    }
    if (State != hsMSG_Header) 
        State = hsMSG_Normal;
    *ECol = C;
    return 0;
}

#endif
