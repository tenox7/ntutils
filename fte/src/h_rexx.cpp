/*    h_rexx.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "fte.h"

#ifdef CONFIG_HILIT_REXX

#include "c_bind.h"
#include "o_buflist.h"
#include "sysdep.h"

#include <ctype.h>

#define hsREXX_Normal    0
#define hsREXX_Comment   1
#define hsREXX_String1   3
#define hsREXX_String2   4
#define hsREXX_Keyword   5
#define hsREXX_CommentL  6

int Hilit_REXX(EBuffer *BF, int /*LN*/, PCell B, int Pos, int Width, ELine *Line, hlState &State, hsState *StateMap, int *ECol) {
    int j = 0;
    int firstnw = 0;
    HILIT_VARS(BF->Mode->fColorize->Colors, Line);
    int wascall = 0;

    C = 0;
    NC = 0;
    for(i = 0; i < Line->Count;) {
        if (*p != ' ' && *p != 9) firstnw++;
        IF_TAB() else {
            switch(State) {
            case hsREXX_Comment:
                Color = CLR_Comment;
                if ((len >= 2) && (*p == '*') && (*(p+1) == '/')) {
                    ColorNext();
                set_normal:
                    ColorNext();
                    State = hsREXX_Normal;
                    continue;
                }
                goto hilit;
            case hsREXX_String1:
                Color = CLR_String;
                if (*p == '\'') {
                    goto set_normal;
                }
                goto hilit;
            case hsREXX_String2:
                Color = CLR_String;
                if (*p == '"') {
                    goto set_normal;
                }
                goto hilit;
            default:
            case hsREXX_Normal:
                if (isalpha(*p) || (*p == '_')) {

                    j = 0;
                    while (((i + j) < Line->Count) &&
                           (isalnum(Line->Chars[i+j]) ||
                            (Line->Chars[i + j] == '_'))
                          ) j++;
                    if (wascall) {
                        State = hsREXX_Normal;
                        Color = CLR_Function;
                        wascall = 0;
                    } else {
                        if (BF->GetHilitWord(Color, Line->Chars + i, j, 1))
                            State = hsREXX_Keyword;
                        else {
                            int x;
                            x = i + j;
                            if ((x < Line->Count) && (Line->Chars[x] == '(')) {
                                Color = CLR_Function;
                            } else {
                                Color = CLR_Normal;
                            }
                            State = hsREXX_Normal;
                        }
                    }
                    if (StateMap)
                        memset(StateMap + i, State, j);
                    if (B)
                        MoveMem(B, C - Pos, Width, Line->Chars + i, HILIT_CLRD(), j);
                    if (State == hsREXX_Keyword)
                        if (strnicmp(Line->Chars + i, "CALL", 4) == 0)
                            wascall = 1;
                    i += j;
                    len -= j;
                    p += j;
                    C += j;
                    State = hsREXX_Normal;
                    continue;
                } else if ((len >= 2) && ( (*p == '/') && (*(p+1) == '*') )) {
                    State = hsREXX_Comment;
                    Color = CLR_Comment;
                    //hilit2:
                    ColorNext();
                hilit:
                    ColorNext();
                    continue;
                } else if ((len >= 2) && (*p == '-') && (p[1] == '-')) {
                    State = hsREXX_CommentL;
                    Color = CLR_Comment;
                    ColorNext();
                    ColorNext();
                    continue;
                } else if (isdigit(*p)) {
                    Color = CLR_Number;
                    ColorNext();
                    while (len && isdigit(*p))
                            ColorNext();
                    continue;
                } else if (*p == '\'') {
                    State = hsREXX_String1;
                    Color = CLR_String;
                    goto hilit;
                } else if (*p == '"') {
                    State = hsREXX_String2;
                    Color = CLR_String;
                    goto hilit;
                } else if (ispunct(*p) && *p != '_') {
                    Color = CLR_Punctuation;
                    goto hilit;
                }
                Color = CLR_Normal;
                goto hilit;
            case hsREXX_CommentL:
                Color = CLR_Comment;
                goto hilit;
            }
        }
    }
    switch (State) {
    case hsREXX_String1:
    case hsREXX_String2:
    case hsREXX_CommentL:
        State = hsREXX_Normal;
        break;
    }
    *ECol = C;
    return 0;
}

#ifdef CONFIG_INDENT_REXX

int REXX_Base_Indent = 4;
int REXX_Do_Offset = 0;

#define REXX_BASE_INDENT       REXX_Base_Indent
#define REXX_DO_OFFSET         REXX_Do_Offset


static int Match(int Len, int Pos, hsState *StateMap, const char *Text, const char *String, hsState State) {
    int L = strlen(String);

    if (Pos + L <= Len)
        if (StateMap == NULL || IsState(StateMap + Pos, State, L))
            if (strnicmp(String, Text + Pos, L) == 0)
                return 1;
    return 0;
}

static int Match2(int Len, int Pos, hsState *StateMap, const char *Text, const char *String, hsState State) {
    int L = strlen(String);

    int i;
    for( i = 0; i < Pos; i++ )
        if( Text[i] != ' ' ) return 0;

    if (Pos + L <= Len)
        if (StateMap == NULL || IsState(StateMap + Pos, State, L))
            if (strnicmp(String, Text + Pos, L) == 0)
                return 1;
    return 0;
}

static int SearchMatch(int Count, EBuffer *B, int Row, int Ctx) {
    char *P;
    int L;
    int Pos;
    int StateLen;
    hsState *StateMap;
    int ICount = (Ctx == 2) ? Count : 0;

    Count = (Ctx == 2) ? 0 : Count;

    // Check all previous rows of the buffer for a matching "starting" keyword.
    // Count gives the "depth" we are in, must reach 0.
    // Ctx == 1 means we are looking for a mate for "end"
    // Ctx == 2 means we are looking for a mate for "else"
    // We try to find the indent of the line that "started" this "block" and return it.
    while (Row >= 0) {
        P = B->RLine(Row)->Chars;
        L = B->RLine(Row)->Count;
        StateMap = NULL;
        if (B->GetMap(Row, &StateLen, &StateMap) == 0) return -1;
        Pos = L - 1;
        if (L > 0) while (Pos >= 0) {
            if (isalpha(P[Pos])) {
                if( Ctx == 1 || Ctx == 3 ) {
                if (Match(L, Pos, StateMap, P, "do", hsREXX_Keyword) ||
                        Match(L, Pos, StateMap, P, "loop", hsREXX_Keyword) ||
                    Match(L, Pos, StateMap, P, "select", hsREXX_Keyword ))
                {
                    Count++;
                        if (Count == 0) {
                        if (StateMap)
                            free(StateMap);
                            if( Ctx == 3 )
                        return B->LineIndented(Row);
                            else
                                return B->LineIndented(Row) + REXX_BASE_INDENT;
                    }
                } else if (Match(L, Pos, StateMap, P, "end", hsREXX_Keyword)) {
                    Count--;
                }
                } else if( Ctx == 4 ) {
                    if (Match2(L, Pos, StateMap, P, "class", hsREXX_Keyword) )
                    {
                        if (StateMap)
                            free(StateMap);
                        return B->LineIndented(Row) + REXX_BASE_INDENT;
                    }
                }
                if (Ctx == 2 && Count == 0) {
                    if (Match(L, Pos, StateMap, P, "if", hsREXX_Keyword)) {
                        ICount++;
                        if (ICount == 0) {
                            if (StateMap)
                                free(StateMap);
                            return B->LineIndented(Row);
                        }
                    } else if (Match(L, Pos, StateMap, P, "else", hsREXX_Keyword)) {
                        ICount--;
                    }
                }
            }
            Pos--;
        }
        if (StateMap) free(StateMap);
        Row--;
    }
    return -1;
}

static int CheckLabel(EBuffer *B, int Line) {
    PELine L = B->RLine(Line);
    int P = B->CharOffset(L, B->LineIndented(Line));
    int Cnt = 0;

    if (Line > 0 && B->RLine(Line - 1)->StateE != hsREXX_Normal)
        return 0;

    while ((P < L->Count) &&
           (L->Chars[P] == ' ' || L->Chars[P] == 9)) P++;
    while (P < L->Count) {
        if (Cnt > 0)
            if (L->Chars[P] == ':') return 1;
        if (!(isalnum(L->Chars[P]) || (L->Chars[P] == '_'))) return 0;
        Cnt++;
        P++;
    }
    return 0;
}

static int SearchBackContext(EBuffer *B, int Row, char &ChFind) {
    char *P;
    int L;
    int Pos;
    int Count = -1;
    int StateLen;
    hsState *StateMap;
    int is_blank = 0;

    ChFind = '0';
    while (Row >= 0) {
        P = B->RLine(Row)->Chars;
        L = B->RLine(Row)->Count;
        StateMap = NULL;
        if (B->GetMap(Row, &StateLen, &StateMap) == 0) return 0;
        Pos = L - 1;
        if (L > 0) while (Pos >= 0) {
            if (CheckLabel(B, Row) == 1) {
                Count++;
                ChFind = 'p';
                if (Count == 0) {
                    if (StateMap)
                        free(StateMap);
                    return B->LineIndented(Row);
                }
            }
            if (isalpha(P[Pos]) && (Pos == 0 || !isalpha(P[Pos - 1]))) {
                if (Match(L, Pos, StateMap, P, "do", hsREXX_Keyword) || Match(L, Pos, StateMap, P, "loop", hsREXX_Keyword)) {
                    Count++;
                    ChFind = 'd';
                    //} else if (Match(L, Pos, StateMap, P, "procedure", hsREXX_Keyword)) {
                    //Count++;
                    //ChFind = 'p';
                } else if (Match(L, Pos, StateMap, P, "select", hsREXX_Keyword)) {
                    Count++;
                    ChFind = 's';
                } else if (Match(L, Pos, StateMap, P, "method", hsREXX_Keyword)) {
                    Count++;
                    ChFind = 'm';
                } else if (Match(L, Pos, StateMap, P, "properties", hsREXX_Keyword)) {
                    Count++;
                    ChFind = 'r';
                } else if (Match2(L, Pos, StateMap, P, "class", hsREXX_Keyword)) {
                    Count++;
                    ChFind = 'c';
                    if (StateMap)
                        free(StateMap);
                    return B->LineIndented(Row) + REXX_BASE_INDENT;
                } else if (Match(L, Pos, StateMap, P, "otherwise", hsREXX_Keyword) && Count == 0) {
                    //Count++;
                    ChFind = 'o';
                    if (StateMap)
                        free(StateMap);
                    return B->LineIndented(Row);
                } else if (Match(L, Pos, StateMap, P, "end", hsREXX_Keyword)) {
                    Count--;
                    ChFind = 'e';
                } else if (is_blank < 5 && Match(L, Pos, StateMap, P, "then", hsREXX_Keyword)) {
                    ChFind = 't';
                    if (StateMap)
                        free(StateMap);
                    return B->LineIndented(Row);
                } else if (is_blank < 5 && Match(L, Pos, StateMap, P, "else", hsREXX_Keyword)) {
                    ChFind = 'e';
                    if (StateMap)
                        free(StateMap);
                    return B->LineIndented(Row);
                } else {
                    is_blank++;
                    Pos--;
                    continue;
                }
                if (Count == 0) {
                    if (StateMap)
                        free(StateMap);
                    return B->LineIndented(Row);
                }
            }
            if (P[Pos] != ' ' && P[Pos] != 9) is_blank++;
            Pos--;
        }
        if (StateMap) free(StateMap);
        Row--;
    }
    return -1;
}

static int IndentComment(EBuffer *B, int Row, int /*StateLen*/, hsState * /*StateMap*/) {
    int I = 0;
    char ChFind;

    if (Row > 0) {
        I = SearchBackContext(B, Row - 1, ChFind);
        if (I != -1)
            switch (ChFind) {
            case 's':
            case 'd':
            case 'c':
            case 't':
            case 'e':
            case 'o':
                //        case 'r':
                //        case 'm':
                I += REXX_BASE_INDENT;
            }
        else
            I = 0;
        if (B->RLine(Row - 1)->StateE == hsREXX_Comment)
            //if (LookAt(B, Row - 1, I, "/*", hsREXX_Comment, 0))
                I += 1;
    }
    return I;
}


static int IndentNormal(EBuffer *B, int Line, int /*StateLen*/, hsState * /*StateMap*/) {
    int I = 0;

    if (CheckLabel(B, Line)) {
        return 0;
    } else if (LookAtNoCase(B, Line, 0, "end", hsREXX_Keyword)) {
        return SearchMatch(-1, B, Line - 1, 1);
    } else if (LookAtNoCase(B, Line, 0, "else", hsREXX_Keyword)) {
        return SearchMatch(-1, B, Line - 1, 2);
    } else if (LookAtNoCase(B, Line, 0, "catch", hsREXX_Keyword)) {
        return SearchMatch(-1, B, Line - 1, 3);
    } else if (LookAtNoCase(B, Line, 0, "method", hsREXX_Keyword) || LookAtNoCase(B, Line, 0, "properties", hsREXX_Keyword)) {
        return SearchMatch(-1, B, Line - 1, 4);
    } else {
        char ChFind;

        I = SearchBackContext(B, Line - 1, ChFind);
        if (I == -1)
            return 0;
        switch (ChFind) {
        case 'p':
            if (LookAtNoCase(B, Line, 0, "return", hsREXX_Keyword))
                return I;
            else
                return I + REXX_BASE_INDENT;
        case 's':
        case 'd':
        case 'c':
        case 'r':
        case 'm':
            return I + REXX_BASE_INDENT;
        case 't':
        case 'e':
        case 'o':
            if (LookAtNoCase(B, Line, 0, "do", hsREXX_Keyword))
                return I + REXX_DO_OFFSET;
            else
                return I + REXX_BASE_INDENT;
        default:
            return I;
        }
    }
}

int Indent_REXX(EBuffer *B, int Line, int PosCursor) {
    int I;
    hsState *StateMap = NULL;
    int StateLen = 0;
    int OI;

    OI = I = B->LineIndented(Line);
    if (I != 0) B->IndentLine(Line, 0);
    if (B->GetMap(Line, &StateLen, &StateMap) == 0) return 0;

    if (StateLen > 0) {                 // line is not empty
        if (StateMap[0] == hsREXX_Comment) {
            I = IndentComment(B, Line, StateLen, StateMap);
        } else {
            I = IndentNormal(B, Line, StateLen, StateMap);
        }
    } else {
        I = IndentNormal(B, Line, 0, NULL);
    }
    if (StateMap)
        free(StateMap);
    if (I >= 0)
        B->IndentLine(Line, I);
    else
        I = 0;
    if (PosCursor == 1) {
        int X = B->CP.Col;

        X = X - OI + I;
        if (X < I) X = I;
        if (X < 0) X = 0;
        if (X > B->LineLen(Line)) {
            X = B->LineLen(Line);
            if (X < I) X = I;
        }
        if (B->SetPosR(X, Line) == 0) return 0;
    } else if (PosCursor == 2) {
        if (B->SetPosR(I, Line) == 0) return 0;
    }
    return 1;
}
#endif
#endif
