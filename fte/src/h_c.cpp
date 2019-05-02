/*    h_c.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "fte.h"

#ifdef CONFIG_HILIT_C

#include "c_bind.h"
#include "o_buflist.h"
#include "sysdep.h"

#include <ctype.h>

#define PRINTF(x) //printf x

#define ISNAME(x)  (isalnum(x) || ((x) == '_'))
#define ISUPPER(x, c)  ((x) == (c) || ((x) == (c + 'a' - 'A')))

enum {
    hsC_Normal,
    hsC_Comment,
    hsC_CommentL,
    hsC_Keyword = 4,
    hsC_String1 = 10,
    hsC_String2,
    hsC_CPP,
    hsC_CPP_Comm,
    hsC_CPP_String1,
    hsC_CPP_String2,
    hsC_CPP_ABrace,
    hsC_Tripplet
};

int Hilit_C(EBuffer *BF, int /*LN*/, PCell B, int Pos, int Width, ELine *Line, hlState &State, hsState *StateMap, int *ECol) {
    int j = 0;
    int firstnw = 0;
    HILIT_VARS(BF->Mode->fColorize->Colors, Line);
    int len1 = len;
    char *last = p + len1 - 1;
    int was_include = 0;

    for(i = 0; i < Line->Count;) {
        if (*p != ' ' && *p != '\t') firstnw++;
        IF_TAB() else {
            PRINTF(("STATE %d\n", State));
            switch(State) {
            default:
            case hsC_Tripplet:
            case hsC_Normal:
                if (ISUPPER(*p, 'L') && p[1] == '"') {
                    State = hsC_String2;
                    Color = CLR_String;
                    goto hilit2;
                } else if (ISUPPER(*p, 'L') && p[1] == '\'') {
                    State = hsC_String1;
                    Color = CLR_String;
                    goto hilit2;
                } else if (isalpha(*p) || *p == '_') {
                    for (j = 0; (((i + j) < Line->Count)
                                 && ISNAME(Line->Chars[i + j])); j++)
                        ;
                    if (BF->GetHilitWord(Color, &Line->Chars[i], j)) {
                        //Color = hcC_Keyword;
                        State = hsC_Keyword;
                    } else {
                        int x = i + j;
                        while (x < Line->Count && isspace(Line->Chars[x]))
                            x++;
                        PRINTF(("LABEL  %c %d\n", Line->Chars[x], x));
                        if (x < Line->Count && Line->Chars[x] == '(') {
                            Color = CLR_Function;
                        } else if (State == hsC_Normal
                                   && (x < Line->Count)
                                   && (Line->Chars[x] == ':'
                                       && (x == Line->Count - 1
                                           || Line->Chars[x + 1] != ':'))
                                   && firstnw == 1) {
                            Color = CLR_Label;
                            PRINTF(("___LABEL  %c %d\n", Line->Chars[x], x));
                        } else
                            Color = CLR_Normal;

                        State = hsC_Normal;
                    }
                    if (StateMap)
                        memset(StateMap + i, State, j);
                    if (B)
                        MoveMem(B, C - Pos, Width, Line->Chars + i, HILIT_CLRD(), j);
                    i += j;
                    len -= j;
                    p += j;
                    C += j;
                    State = hsC_Normal;
                    continue;
                } else if ((len >= 2) && (*p == '/') && (p[1] == '*')) {
                    State = hsC_Comment;
                    Color = CLR_Comment;
                    goto hilit2;
                } else if ((len >= 2) && (*p == '/') && (p[1] == '/')) {
                    State = hsC_CommentL;
                    Color = CLR_Comment;
                    goto hilit2;
                } else if (isdigit(*p)) {
                    // check if it is not floating point number 0.08!
                    if ((len >= 2) && (*p == '0') && p[1] != '.') {
                        if (ISUPPER(p[1], 'X')) {
                            Color = CLR_HexNumber;
                            ColorNext();
                            ColorNext();
                            while (len && isxdigit(*p)) ColorNext();
                        } else /* assume it's octal */ {
                            Color = CLR_Number;
                            ColorNext();
                            while (len && ('0' <= *p && *p <= '7')) ColorNext();
                            // if we hit a non-octal, stop hilighting it.
                            if (len && ('8' <= *p && *p <= '9'))
                            {
                                Color = CLR_Normal;
                                while (len && !isspace(*p)) ColorNext();
                                continue;
                            }
                        }
                    } else /* assume it's decimal/floating */ {
                        Color = CLR_Number;
                        ColorNext();
                        while (len && (isdigit(*p) || *p == 'e' || *p == 'E' || *p == '.')) ColorNext();
                        // if it ends with 'f', the number can't have more extras.
                        if (len && (ISUPPER(*p, 'F'))) {
                            ColorNext();
                            continue;
                        }
                    }
                    // allowed extras: u, l, ll, ul, ull, lu, llu
                    int colored_u = 0;
                    if (len && (ISUPPER(*p, 'U'))) {
                        ColorNext();
                        colored_u = 1;
                    }
                    if (len && (ISUPPER(*p, 'L'))) {
                        ColorNext();
                        if (len && (ISUPPER(*p, 'L'))) ColorNext();
                        if (! colored_u && len && (ISUPPER(*p, 'U'))) ColorNext();
                    }
                    continue;
                } else if (*p == '\'') {
                    State = hsC_String1;
                    Color = CLR_String;
                    goto hilit;
                } else if (*p == '"') {
                    State = hsC_String2;
                    Color = CLR_String;
                    goto hilit;
                } else if (firstnw == 1 && *p == '#') {
                    State = hsC_CPP;
                    Color = CLR_CPreprocessor;
                    goto hilit;
                } else if (*p == '?') {
                    State = hsC_Tripplet;
                    Color = CLR_Punctuation;
                    PRINTF(("----FOUND TRIPPLET\n"));
                    goto hilit;
                } else if (ispunct(*p) && *p != '_') {
                    Color = CLR_Punctuation;
                    PRINTF(("----FOUND PUNKT\n"));
                    goto hilit;
                }
                Color = CLR_Normal;
                goto hilit;
            case hsC_Comment:
                Color = CLR_Comment;
                if ((len >= 2) && (*p == '*') && (p[1] == '/')) {
                    ColorNext();
                    ColorNext();
                    State = hsC_Normal;
                    continue;
                }
                goto hilit;
            case hsC_CPP_Comm:
                Color = CLR_Comment;
                if ((len >= 2) && (*p == '*') && (p[1] == '/')) {
                    ColorNext();
                    ColorNext();
                    State = hsC_CPP;
                    continue;
                }
                goto hilit;
            case hsC_CPP_ABrace:
                Color = CLR_String;
                if (*p == '>') {
                    Color = CLR_CPreprocessor;
                    State = hsC_CPP;
                }
                goto hilit;
            case hsC_CommentL:
                Color = CLR_Comment;
                goto hilit;
            case hsC_String1:
                Color = CLR_String;
                if ((len >= 2) && (*p == '\\')) {
                    goto hilit2;
                } else if (*p == '\'') {
                    ColorNext();
                    State = hsC_Normal;
                    continue;
                }
                goto hilit;
            case hsC_String2:
                Color = CLR_String;
                if ((len >= 2) && (*p == '\\')) {
                    goto hilit2;
                } else if (*p == '"') {
                    ColorNext();
                    State = hsC_Normal;
                    continue;
                }
                goto hilit;
            case hsC_CPP_String1:
                Color = CLR_String;
                if ((len >= 2) && (*p == '\\')) {
                    goto hilit2;
                } else if (*p == '\'') {
                    ColorNext();
                    State = hsC_CPP;
                    continue;
                }
                goto hilit;
            case hsC_CPP_String2:
                Color = CLR_String;
                if ((len >= 2) && (*p == '\\')) {
                    goto hilit2;
                } else if (*p == '"') {
                    ColorNext();
                    State = hsC_CPP;
                    continue;
                }
                goto hilit;
            case hsC_CPP:
                if (ISNAME(*p)) {
                    j = 0;
                    Color = CLR_CPreprocessor;
                    while (((i + j) < Line->Count) &&
                           (ISNAME(Line->Chars[i+j])))
                        j++;
                    if (j == 7 && memcmp(Line->Chars + i, "include", 7) == 0)
                        was_include = 1;
                    if (StateMap)
                        memset(StateMap + i, State, j);
                    if (B)
                        MoveMem(B, C - Pos, Width, Line->Chars + i, HILIT_CLRD(), j);
                    i += j;
                    len -= j;
                    p += j;
                    C += j;
                    continue;
                } else if ((len >= 2) && (*p == '/') && (*(p+1) == '*')) {
                    State = hsC_CPP_Comm;
                    Color = CLR_Comment;
                    goto hilit2;
                } else if ((len >= 2) && (*p == '/') && (*(p+1) == '/')) {
                    State = hsC_CommentL;
                    Color = CLR_Comment;
                hilit2:
                    ColorNext();
                hilit:
                    ColorNext();
                    continue;
                } else if (isdigit(*p)) {
                    if ((len >= 2) && (*p == '0')) {
                        if (ISUPPER(p[1], 'X')) {
                            Color = CLR_HexNumber;
                            ColorNext();
                            ColorNext();
                            while (len && isxdigit(*p))
                                ColorNext();
                        } else /* assume it's octal */ {
                            Color = CLR_Number;
                            ColorNext();
                            while (len && ('0' <= *p && *p <= '7')) ColorNext();
                            // if we hit a non-octal, stop hilighting it.
                            if (len && ('8' <= *p && *p <= '9'))
                            {
                                Color = CLR_Normal;
                                while (len && !isspace(*p))
                                    ColorNext();
                                continue;
                            }
                        }
                    } else /* assume it's decimal/floating */ {
                        Color = CLR_Number;
                        ColorNext();
                        while (len && (isdigit(*p) || *p == 'e' || *p == 'E' || *p == '.'))
                            ColorNext();
                        // if it ends with 'f', the number can't have more extras.
                        if (len && (ISUPPER(*p, 'F')))
                            goto hilit;
                    }
                    // allowed extras: u, l, ll, ul, ull, lu, llu
                    int colored_u = 0;
                    if (len && (ISUPPER(*p, 'U'))) {
                        ColorNext();
                        colored_u = 1;
                    }
                    if (len && (ISUPPER(*p, 'L'))) {
                        ColorNext();
                        if (len && (ISUPPER(*p, 'L'))) ColorNext();
                        if (! colored_u && len && (ISUPPER(*p, 'U'))) ColorNext();
                    }
                    continue;
                } else if (*p == '\'') {
                    State = hsC_CPP_String1;
                    Color = CLR_String;
                    goto hilit;
                } else if (*p == '"') {
                    State = hsC_CPP_String2;
                    Color = CLR_String;
                    goto hilit;
                } else if (*p == '<' && was_include) {
                    ColorNext();
                    State = hsC_CPP_ABrace;
                    continue;
                } else {
                    Color = CLR_CPreprocessor;
                    goto hilit;
                }
            }
        }
    }
    if (State == hsC_CommentL)
        State = hsC_Normal;
    if ((len1 == 0) || (*last != '\\')) {
        switch(State) {
        case hsC_String1:
        case hsC_String2:
        case hsC_CPP:
        case hsC_CPP_String1:
        case hsC_CPP_String2:
            State = hsC_Normal;
            break;
        }
    }
    *ECol = C;
    return 0;
}

int IsState(hsState *Buf, hsState State, size_t Len) {

    for (;Len > 0; Buf++, --Len)
        if (*Buf != State)
            return 0;

    return 1;
}

int LookAt(EBuffer *B, int Row, unsigned int Pos, const char *What, hsState State, int NoWord, int CaseInsensitive) {
    size_t Len = strlen(What);

    if (Row < 0 || Row >= B->RCount) {
        PRINTF(("Row out of range: %d vs %d\n", Row, B->RCount));
        return 0;
    }
    char* pLine          = B->RLine(Row)->Chars;
    unsigned uLineLength = B->RLine(Row)->Count;
    Pos = B->CharOffset(B->RLine(Row), Pos);
    if (Pos + strlen(What) > uLineLength) return 0;
    if (NoWord && uLineLength > Pos + Len && ISNAME(pLine[Pos + Len]))
        return 0;

    PRINTF(("Check against [%c]\n", What));
    if ((CaseInsensitive && memicmp(pLine + Pos, What, Len) == 0)
        || (!CaseInsensitive && memcmp(pLine + Pos, What, Len) == 0)) {
        int StateLen;
        hsState *StateMap;
        if (B->GetMap(Row, &StateLen, &StateMap) == 0) return 0;
        if( IsState(StateMap + Pos, State, strlen(What) ) ) return 1;
    }
    return 0;
}

#ifdef CONFIG_INDENT_C

int C_Indent = 4;
int C_BraceOfs = 0;
int C_ParenDelta = -1;
int C_CaseOfs = 0;
int C_CaseDelta = 4;
int C_ClassOfs = 0;
int C_ClassDelta = 4;
int C_ColonOfs = 0;//-4;
int C_CommentOfs = 0;
int C_CommentDelta = 1;
int C_FirstLevelWidth = -1;
int C_FirstLevelIndent = 4;
int C_Continuation = 4;
int FunctionUsesContinuation = 0;

// this is global, unfortunately -- FIX !!!
int EBuffer::SetCIndentStyle(ExState &State) {
    if (State.GetIntParam(View, &C_Indent) == 0)
        return 0;
    if (State.GetIntParam(View, &C_BraceOfs) == 0)
        return 0;
    if (State.GetIntParam(View, &C_ParenDelta) == 0)
        return 0;
    if (State.GetIntParam(View, &C_CaseOfs) == 0)
        return 0;
    if (State.GetIntParam(View, &C_CaseDelta) == 0)
        return 0;
    if (State.GetIntParam(View, &C_ClassOfs) == 0)
        return 0;
    if (State.GetIntParam(View, &C_ClassDelta) == 0)
        return 0;
    if (State.GetIntParam(View, &C_ColonOfs) == 0)
        return 0;
    if (State.GetIntParam(View, &C_CommentOfs) == 0)
        return 0;
    if (State.GetIntParam(View, &C_CommentDelta) == 0)
        return 0;
    if (State.GetIntParam(View, &C_FirstLevelWidth) == 0)
        return 0;
    if (State.GetIntParam(View, &C_FirstLevelIndent) == 0)
        return 0;
    if (State.GetIntParam(View, &C_Continuation) == 0)
        return 0;
    return 1;
}

#define C_INDENT            C_Indent
#define C_BRACE_OFS         C_BraceOfs
#define C_PAREN_DELTA       C_ParenDelta
#define C_CASE_OFS          C_CaseOfs
#define C_CASE_DELTA        C_CaseDelta
#define C_CLASS_OFS         C_ClassOfs
#define C_CLASS_DELTA       C_ClassDelta
#define C_COLON_OFS         C_ColonOfs
#define C_COMMENT_OFS       C_CommentOfs
#define C_COMMENT_DELTA     C_CommentDelta
#define C_CONTINUATION      C_Continuation
#define C_FIRST_INDENT      C_FirstLevelIndent
#define C_FIRST_WIDTH       C_FirstLevelWidth

#define FIND_IF         0x0001
#define FIND_SEMICOLON  0x0002
#define FIND_COMMA      0x0004
#define FIND_COLON      0x0008
#define FIND_ELSE       0x0010
#define FIND_FOR        0x0020
#define FIND_WHILE      0x0040
#define FIND_ENDBLOCK   0x0080
//#define FIND_BEGINBLOCK 0x0100
#define FIND_CLASS      0x0200
#define FIND_CASE       0x0400
#define FIND_SWITCH     0x0800
#define FIND_QUESTION   0x1000

static int CheckLabel(EBuffer *B, int Line) {
    PELine L = B->RLine(Line);
    int P = B->CharOffset(L, B->LineIndented(Line));
    int Cnt = 0;

    PRINTF(("CHECKING LABEL\n"));
    if (Line > 0 && B->RLine(Line - 1)->StateE != hsC_Normal)
        return 0;

    while ((P < L->Count) &&
           (L->Chars[P] == ' ' || L->Chars[P] == 9)) P++;

    while (P < L->Count) {
        if (Cnt > 0)
            if (L->Chars[P] == ':' && (Cnt == 1 || L->Chars[P + 1] != ':')) {
                PRINTF(("CHECKING LABEL -> 1!\n"));
                return 1;
            }
        if (!ISNAME(L->Chars[P])) {
            PRINTF(("NONCHECKING LABEL -> 0\n"));
            return 0;
        }
        Cnt++;
        P++;
    }
    PRINTF(("CHECKING LABEL -> 0\n"));
    return 0;
}

static int SearchBackMatch(int Count, EBuffer *B, int Row, hsState State, const char *Open, const char *Close, int *OPos, int *OLine, int matchparens = 0, int bolOnly = 0) {
    char *P;
    int L;
    const size_t LOpen = strlen(Open);
    const size_t LClose = strlen(Close);
    int StateLen;
    hsState *StateMap;
    int CountX[3] = { 0, 0, 0 };
    int didMatch = 0;

    *OLine = Row;
    *OPos = 0;
    while (Row >= 0) {
        P = B->RLine(Row)->Chars;
        L = B->RLine(Row)->Count;
        StateMap = NULL;
        if (B->GetMap(Row, &StateLen, &StateMap) == 0) return -1;
        for (int Pos = (int)L - 1; Pos >= 0; Pos--) {
            if (P[Pos] != ' ' && P[Pos] != 9) {
                if (StateMap[Pos] == hsC_Normal) {
                    switch (P[Pos]) {
                    case '{': CountX[0]--; break;
                    case '}': CountX[0]++; break;
                    case '(': CountX[1]--; break;
                    case ')': CountX[1]++; break;
                    case '[': CountX[2]--; break;
                    case ']': CountX[2]++; break;
                    }
                }
                if (!matchparens ||
                    (CountX[0] == 0 && CountX[1] == 0 && CountX[2] == 0))
                {
                    if (LOpen + Pos <= L) {
                        if (IsState(StateMap + Pos, State, LOpen)) {
                            if (memcmp(P + Pos, Open, LOpen) == 0) Count++;
                            if (Count == 0) {
                                if (bolOnly)
                                    didMatch = 1;
                                else {
                                    *OPos = B->ScreenPos(B->RLine(Row), Pos);
                                    *OLine = Row;
                                    free(StateMap);
                                    return B->LineIndented(Row);
                                }
                            }
                        }
                        if (LClose + Pos <= L) {
                            if (IsState(StateMap + Pos, State, LClose)) {
                                if (memcmp(P + Pos, Close, LClose) == 0) Count--;
                            }
                        }
                    }
                }
            }
        }
        if (bolOnly && didMatch && CountX[1] == 0 && CountX[2] == 0) {
            *OPos = 0;
            *OLine = Row;
            free(StateMap);
            return B->LineIndented(Row);
        }
        if (StateMap) free(StateMap);
        Row--;
    }
    return -1;
}

static int FindPrevIndent(EBuffer *B, int &RowP, int &ColP, char &CharP, int Flags)
{
    //LOG << "Flags: " << hex << Flags << dec << ENDLINE;
    int StateLen;
    hsState *StateMap = 0;
    char *P;
    int L;
    int Count[4] = {
        0, // { }
        0, // ( )
        0, // [ ]
        0, // if/else (one if for each else)
    };

    assert(RowP >= 0 && RowP < B->RCount);
    L = B->RLine(RowP)->Count;
    if (ColP >= L)
        ColP = L - 1;
    assert(ColP >= -1 && ColP <= L);

    char BolChar = ' ';
    int BolCol = -1;
    int BolRow = -1;

    while (RowP >= 0) {

        P = B->RLine(RowP)->Chars;
        L = B->RLine(RowP)->Count;
        StateMap = NULL;
        if (B->GetMap(RowP, &StateLen, &StateMap) == 0)
        {
            //LOG << "Can't get state maps" << ENDLINE;
            return 0;
        }
        if (L > 0)
            while (ColP >= 0) {
            //LOG << "ColP: " << ColP << " State: " << (int)StateMap[ColP] << ENDLINE;
            if (StateMap[ColP] == hsC_Normal) {
                //LOG << "CharP: " << BinChar(P[ColP]) << " BolChar: " << BinChar(BolChar) <<
                //    " BolRow: " << BolRow <<
                //    " BolCol: " << BolCol <<
                //    ENDLINE;
                switch (CharP = P[ColP]) {
                case '{':
                    if (BolChar == ':' || BolChar == ',') {
                        CharP = BolChar;
                        ColP = BolCol;
                        RowP = BolRow;
                        free(StateMap);
                        return 1;
                    }
                    if (TEST_ZERO) {
                        free(StateMap);
                        return 1;
                    }
                    Count[0]--;
                    break;
                case '}':
                    if (BolChar == ':' || BolChar == ',') {
                        CharP = BolChar;
                        ColP = BolCol;
                        RowP = BolRow;
                        free(StateMap);
                        return 1;
                    }
                    if (BolChar == ';') {
                        CharP = BolChar;
                        ColP = BolCol;
                        RowP = BolRow;
                        free(StateMap);
                        return 1;
                    }
                    if (ColP == 0) { /* speed optimization */
                        free(StateMap);
                        return 1;
                    }
                    if (TEST_ZERO && (Flags & FIND_ENDBLOCK)) {
                        free(StateMap);
                        return 1;
                    }
                    Count[0]++;
                    break;
                case '(':
                    if (TEST_ZERO) {
                        free(StateMap);
                        return 1;
                    }
                    Count[1]--;
                    break;
                case ')':
                    Count[1]++;
                    break;
                case '[':
                    if (TEST_ZERO) {
                        free(StateMap);
                        return 1;
                    }
                    Count[2]--;
                    break;
                case ']':
                    Count[2]++;
                    break;
                case ':':
                    if (ColP >= 1 && P[ColP - 1] == ':') { // skip ::
                        ColP -= 2;
                        continue;
                    }
                    /* pass through (case:)*/
                case ',':
                case ';':
                    if (TEST_ZERO && BolChar == ' ') {
                        if ((CharP == ';' && (Flags & FIND_SEMICOLON))
                            || (CharP == ',' && (Flags & FIND_COMMA))
                            || (CharP == ':' && (Flags & FIND_COLON))) {
                            BolChar = CharP;
                            BolCol = ColP;
                            BolRow = RowP;
                            // this should be here
                            // if not say why ???
                            //free(StateMap);
                            //return 1;
                        }
                    }
                    if (BolChar == ',' && CharP == ':') {
                        BolChar = ' ';
                        BolCol = -1;
                        BolRow = -1;
                        break;
                    }
                    if ((BolChar == ':' || BolChar == ',')
                        && (CharP == ';'/* || CharP == ','*/)) {
                        CharP = ':';
                        ColP = BolCol;
                        RowP = BolRow;
                        free(StateMap);
                        return 1;
                    }
                    break;
                case '?':
		    if ((Flags & FIND_QUESTION)) {
			PRINTF(("FOUND ?"));
			if (BolChar == ':' || BolChar == ',') {
			    BolChar = ' ';
			    BolCol = -1;
			    BolRow = -1;
			}
		    }
                    break;
                }
            } else if (StateMap[ColP] == hsC_Keyword && (BolChar == ' ' || BolChar == ':')) {
                if (L - ColP >= 2 &&
                    IsState(StateMap + ColP, hsC_Keyword, 2) &&
                    memcmp(P + ColP, "if", 2) == 0)
                {
                    //puts("\nif");
                    if (Count[3] > 0)
                        Count[3]--;
                    if (Flags & FIND_IF) {
                        if (TEST_ZERO) {
                            CharP = 'i';
                            free(StateMap);
                            return 1;
                        }
                    }
                }
                if (L - ColP >= 4 &&
                    IsState(StateMap + ColP, hsC_Keyword, 4) &&
                    memcmp(P + ColP, "else", 4) == 0)
                {
                    //puts("\nelse\x7");
                    if (Flags & FIND_ELSE) {
                        if (TEST_ZERO) {
                            CharP = 'e';
                            free(StateMap);
                            return 1;
                        }
                    }
                    Count[3]++;
                }
                if (TEST_ZERO) {

                    if ((Flags & FIND_FOR) &&
                        L - ColP >= 3 &&
                        IsState(StateMap + ColP, hsC_Keyword, 3) &&
                        memcmp(P + ColP, "for", 3) == 0)
                    {
                        CharP = 'f';
                        free(StateMap);
                        return 1;
                    }
                    if ((Flags & FIND_WHILE) &&
                        L - ColP >= 5 &&
                        IsState(StateMap + ColP, hsC_Keyword, 5) &&
                        memcmp(P + ColP, "while", 5) == 0)
                    {
                        CharP = 'w';
                        free(StateMap);
                        return 1;
                    }
                    if ((Flags & FIND_SWITCH) &&
                        L - ColP >= 6 &&
                        IsState(StateMap + ColP, hsC_Keyword, 6) &&
                        memcmp(P + ColP, "switch", 6) == 0)
                    {
                        CharP = 's';
                        free(StateMap);
                        return 1;
                    }
                    if (((Flags & FIND_CASE) || (BolChar == ':')) &&
                        (((L - ColP >= 4) &&
                         IsState(StateMap + ColP, hsC_Keyword, 4) &&
                         memcmp(P + ColP, "case", 4) == 0) ||
                        ((L - ColP >= 7) &&
                         IsState(StateMap + ColP, hsC_Keyword, 7) &&
                         memcmp(P + ColP, "default", 7) == 0)))
                    {
                        CharP = 'c';
                        if (BolChar == ':') {
                            CharP = BolChar;
                            ColP = BolCol;
                            RowP = BolRow;
                        }
                        free(StateMap);
                        return 1;
                    }
                    if (((Flags & FIND_CLASS) || (BolChar == ':')) &&
                        (L - ColP >= 5 &&
                         IsState(StateMap + ColP, hsC_Keyword, 5) &&
                         memcmp(P + ColP, "class", 5) == 0))
                    {
                        CharP = 'l';
                        if (BolChar == ':') {
                            CharP = BolChar;
                            ColP = BolCol;
                            RowP = BolRow;
                        }
                        free(StateMap);
                        return 1;
                    }
                    if (((Flags & FIND_CLASS) || (BolChar == ':')) &&
                        ((L - ColP >= 6 &&
                          IsState(StateMap + ColP, hsC_Keyword, 6) &&
                          memcmp(P + ColP, "public", 6) == 0) ||
                         ((L - ColP >= 7) &&
                          IsState(StateMap + ColP, hsC_Keyword, 7) &&
                          memcmp(P + ColP, "private", 7) == 0) ||
                         ((L - ColP >= 9) &&
                          IsState(StateMap + ColP, hsC_Keyword, 9) &&
                          memcmp(P + ColP, "protected", 9) == 0)))
                    {
                        CharP = 'p';
                        if (BolChar == ':') {
                            CharP = BolChar;
                            ColP = BolCol;
                            RowP = BolRow;
                        }
                        free(StateMap);
                        return 1;
                    }
                }
            }
            ColP--;
        }
        free(StateMap);
        if (BolChar != ' ' && BolChar != ':' && BolChar != ',') {
            CharP = BolChar;
            ColP = BolCol;
            return 1;
        }
        RowP--;
        if (RowP >= 0) {
            L = B->RLine(RowP)->Count;
            ColP = L - 1;
        }
    }
#undef TEST_ZERO
    return 0;
}

#define SKIP_FORWARD  0
#define SKIP_BACK     1
#define SKIP_MATCH    2
#define SKIP_LINE     4
#define SKIP_TOBOL    8

static int SkipWhite(EBuffer *B, int Bottom, int &Row, int &Col, int Flags) {
    char *P;
    int L;
    int StateLen;
    hsState *StateMap;
    int Count[3] = { 0, 0, 0 };

    assert (Row >= 0 && Row < B->RCount);
    L = B->RLine(Row)->Count;
    if (Col >= L)
        Col = L;
    assert (Col >= -1 && Col <= L) ;

    while (Row >= 0 && Row < B->RCount) {
        P = B->RLine(Row)->Chars;
        L = B->RLine(Row)->Count;
        StateMap = NULL;
        if (B->GetMap(Row, &StateLen, &StateMap) == 0)
            return 0;

        if (L > 0)
            for ( ; Col >= 0 && Col < L;
                  Col += ((Flags & SKIP_BACK) ? -1 : +1)) {
                if (P[Col] == ' ' || P[Col] == '\t')
                    continue;
                if (StateMap[Col] != hsC_Normal &&
                    StateMap[Col] != hsC_Keyword &&
                    StateMap[Col] != hsC_String1 &&
                    StateMap[Col] != hsC_String2)
                    continue;
                if (StateMap[Col] == hsC_Normal && (Flags & SKIP_MATCH)) {
                    switch (P[Col]) {
                    case '{': Count[0]--; continue;
                    case '}': Count[0]++; continue;
                    case '(': Count[1]--; continue;
                    case ')': Count[1]++; continue;
                    case '[': Count[2]--; continue;
                    case ']': Count[2]++; continue;
                    }
                }
                if (Count[0] == 0 && Count[1] == 0 && Count[2] == 0
                    && !(Flags & SKIP_TOBOL)) {
                    free(StateMap);
                    return 1;
                }
            }
        free(StateMap);
        if (Count[0] == 0 && Count[1] == 0 && Count[2] == 0 && (Flags & SKIP_TOBOL))
            return 1;
        if (Flags & SKIP_LINE) {
            return 1;
        }
        if (Flags & SKIP_BACK) {
            Row--;
            if (Row >= 0) {
                L = B->RLine(Row)->Count;
                Col = L - 1;
            }
        } else {
            if (Row + 1 >= Bottom)
                return 1;
            Row++;
            Col = 0;
        }
    }
    return 0;
}


static int IndentNormal(EBuffer *B, int Line, int /*StateLen*/, hsState * /*StateMap*/) {
    int I = 0;
    int Pos, L;

    if (LookAt(B, Line, 0, "case", hsC_Keyword) ||
        LookAt(B, Line, 0, "default", hsC_Keyword))
    {
        I = SearchBackMatch(-1, B, Line - 1, hsC_Normal, "{", "}", &Pos, &L) + C_CASE_OFS;
        return I;
    } else if (LookAt(B, Line, 0, "public:", hsC_Keyword, 0) ||
               LookAt(B, Line, 0, "private:", hsC_Keyword, 0) ||
               LookAt(B, Line, 0, "protected:", hsC_Keyword, 0))
    {
        I = SearchBackMatch(-1, B, Line - 1, hsC_Normal, "{", "}", &Pos, &L) + C_CLASS_OFS;
        return I;
    } else if (LookAt(B, Line, 0, "else", hsC_Keyword)) {
        I = SearchBackMatch(-1, B, Line - 1, hsC_Keyword, "if", "else", &Pos, &L, 1);
        return I;
    } else if (LookAt(B, Line, 0, "}", hsC_Normal, 0)) {
        I = SearchBackMatch(-1, B, Line - 1, hsC_Normal, "{", "}", &Pos, &L, 0, 1);
        return I;
    } else if (LookAt(B, Line, 0, ")", hsC_Normal, 0)) {
        I = SearchBackMatch(-1, B, Line - 1, hsC_Normal, "(", ")", &Pos, &L);
        if (C_PAREN_DELTA >= 0)
            return I + C_PAREN_DELTA;
        else
            return Pos;
    } else if (LookAt(B, Line, 0, "]", hsC_Normal, 0)) {
        I = SearchBackMatch(-1, B, Line - 1, hsC_Normal, "[", "]", &Pos, &L);
        if (C_PAREN_DELTA >= 0)
            return I + C_PAREN_DELTA;
        else
            return Pos;
    } else {
        char CharP = ' ';
        // char FirstCharP = ' ';
        int RowP = Line;
        int ColP = -1;
        int PrevRowP = RowP;
        int PrevColP = ColP;
        int FirstRowP;
        int FirstColP;
        int ContinuationIndent = 0;

        if (SkipWhite(B, Line, PrevRowP, PrevColP, SKIP_BACK) != 1)
            return 0;

        PrevColP++;
        //LOG << "PrevRowP=" << PrevRowP << ", PrevColP=" << PrevColP << ENDLINE;

        if (FindPrevIndent(B, RowP, ColP, CharP,
                           FIND_IF |
                           FIND_ELSE |
                           FIND_FOR |
                           FIND_WHILE |
                           FIND_SWITCH |
                           FIND_CASE |
                           //FIND_CLASS |
                           FIND_COLON |
                           FIND_SEMICOLON |
                           FIND_COMMA |
                           FIND_ENDBLOCK) != 1)
        {
            //LOG << "Found: " << ENDLINE;
            if (RowP != PrevRowP)
                ContinuationIndent = C_CONTINUATION;
            I = 0;
            if (LookAt(B, Line, 0, "{", hsC_Normal, 0)) {
                I += C_BRACE_OFS;
                ContinuationIndent = 0;
            }
            return I + ContinuationIndent;
        }

        FirstRowP = RowP;
        FirstColP = ColP;
        // FirstCharP = CharP;

        //LOG << "FirstRowP=" << FirstRowP << ", FirstColP=" << FirstColP <<
        //    ", CharP=" << BinChar(CharP) << ENDLINE;

        switch (CharP) {
        case 'c':
            I = B->LineIndented(RowP) + C_CONTINUATION;
            return I;

        case '(':
        case '[':
            if (C_PAREN_DELTA >= 0) {
                I = B->LineIndented(FirstRowP) + C_PAREN_DELTA;
            } else {
                ColP++;
                if (SkipWhite(B, Line, RowP, ColP, SKIP_FORWARD | SKIP_LINE) != 1)
                    return 0;
                if (ColP < B->LineChars(RowP) || !FunctionUsesContinuation) {
                    char strLeft[2] = { CharP, 0 };
                    char strRight[2] = { CharP == '(' ? ')' : ']', 0 };
                    I = SearchBackMatch(-1, B, Line - 1, hsC_Normal, strLeft, strRight, &Pos, &L);
                    I = Pos + 1;
                } else {
                    I = B->LineIndented(RowP) + C_CONTINUATION;
                }
            }
            return I;

        case '{':
            ColP++;
            if (((PrevRowP != RowP) ||
                 ((PrevRowP == RowP) && (PrevColP != ColP)))
                && FirstRowP != PrevRowP)
                ContinuationIndent = C_CONTINUATION;
            ColP--; ColP--;
            if (SkipWhite(B, Line, RowP, ColP, SKIP_BACK | SKIP_TOBOL | SKIP_MATCH) != 1)
                return 0;
            I = B->LineIndented(RowP);
            if (B->LineIndented(FirstRowP) <= C_FIRST_WIDTH)
                I += C_FIRST_INDENT;
            else
                I += C_INDENT;
            PRINTF(("'{' indent : Line=%d, RowP=%d, ColP=%d, CharP=%c\n", Line, RowP, ColP, CharP));

            if (LookAt(B, Line, 0, "{", hsC_Normal, 0))
                I -= C_BRACE_OFS;
            else
                I += ContinuationIndent;

            return I;
        case ',':
            I = B->LineIndented(FirstRowP);
            return I;
        case '}':
            ColP++;
            ColP++;
            /*---nobreak---*/
        case ';':
            ColP--;
            if (FindPrevIndent(B, RowP, ColP, CharP,
                               ((CharP == ',') ? FIND_COMMA | FIND_COLON :
                                //(CharP == ';') ? FIND_SEMICOLON | FIND_COLON :
                                FIND_SEMICOLON | FIND_COLON)) != 1)
            {
                if (FirstRowP != PrevRowP)
                    ContinuationIndent = C_CONTINUATION;
                I = 0;
                if (LookAt(B, Line, 0, "{", hsC_Normal, 0)) {
                    I += C_BRACE_OFS;
                    ContinuationIndent = 0;
                }
                return I + ContinuationIndent;
            }
            PRINTF(("';' Line=%d, RowP=%d, ColP=%d, CharP=%c\n", Line, RowP, ColP, CharP));

            switch (CharP) {
            case ',':
            case ';':
            case '{':
            case ':':
                ColP++;
                if (SkipWhite(B, Line, RowP, ColP, SKIP_FORWARD) != 1)
                    return 0;
                //ColP--;
                //if (SkipWhite(B, RowP, ColP, SKIP_BACK) != 1)
                //if (CharP == ':') {
                //    I -= C_COLON_OFS;
                //}
                PRINTF(("';' indent : Line=%d, RowP=%d, ColP=%d, CharP=%c   Ind=%d\n", Line, RowP, ColP, CharP, I));
                I = B->LineIndented(RowP);
                if (((PrevRowP != RowP) ||
                     ((PrevRowP == RowP) && (PrevColP != ColP)))
                    && FirstRowP != PrevRowP)
                    ContinuationIndent = C_CONTINUATION;

                if (LookAt(B, Line, 0, "{", hsC_Normal, 0)) {
                    //I -= C_BRACE_OFS;
                    ContinuationIndent = 0;
                }
                if (LookAt(B, Line, 0, "{", hsC_Normal, 0)
                    && LookAt(B, RowP, ColP, "{", hsC_Normal, 0))
                    I -= 0; //C_BRACE_OFS;
                else if (LookAt(B, Line, 0, "{", hsC_Normal, 0)
                         && !LookAt(B, RowP, ColP, "{", hsC_Normal, 0))
                    I += C_BRACE_OFS;
                else if (!LookAt(B, Line, 0, "{", hsC_Normal, 0)
                         && LookAt(B, RowP, ColP, "{", hsC_Normal, 0))
                    I -= C_BRACE_OFS;
                break;
            case '(':
                ColP++;
                if (SkipWhite(B, Line, RowP, ColP, SKIP_FORWARD | SKIP_LINE) != 1)
                    return 0;
                I = B->ScreenPos(B->RLine(RowP), ColP);
                break;
            default:
                I = B->LineIndented(RowP);
                break;
            }
            PRINTF(("';' -- indent : Line=%d, RowP=%d, ColP=%d, CharP=%c Ind2=%d\n", Line, RowP, ColP, CharP, I));

            //            else
            //            if (LookAt(B, Line, 0, "{", hsC_Normal, 0))
            //                I += C_INDENT - C_BRACE_OFS;

            return I + ContinuationIndent;
#if 1
        case ':':
            ColP--;
            PRINTF(("COL-- %d\n", ColP));
            if (FindPrevIndent(B, RowP, ColP, CharP,
                               FIND_SEMICOLON | FIND_COLON | FIND_QUESTION
                               | FIND_CLASS | FIND_CASE) != 1) {
                PRINTF(("FOUNPRE \n"));
                if (FirstRowP != PrevRowP)
                    ContinuationIndent = C_CONTINUATION;
                return 0 + ContinuationIndent;
            }

            PRINTF(("':' Line=%d, RowP=%d, ColP=%d, CharP=%c\n", Line, RowP, ColP, CharP));

            switch (CharP) {
            case ':':
                /*ColP++;
                if (SkipWhite(B, Line, RowP, ColP, SKIP_FORWARD) != 1)
                 return 0;
                 I = B->LineIndented(RowP);// - C_COLON_OFS;
                 PRINTF(("':' 0 indent : Line=%d, RowP=%d, ColP=%d, CharP=%c\n", Line, RowP, ColP, CharP));
                 break;*/
            case '{':
            case ';':
                ColP++;
                if (SkipWhite(B, Line, RowP, ColP, SKIP_FORWARD) != 1)
                    return 0;
                I = B->LineIndented(RowP);
                PRINTF(("!!! FirstRowP=%d, PrevRowP=%d, RowP=%d, I=%d\n", FirstRowP, PrevRowP, RowP, I));
                PRINTF(("!!! FirstColP=%d, PrevColP=%d, ColP=%d\n", FirstColP, PrevColP, ColP));
                if (CheckLabel(B, RowP)) {
                    PRINTF(("CHECKEDLABEL1\n"));
                    I -= C_COLON_OFS;
                } else if (PrevRowP == RowP && FirstRowP == PrevRowP && FirstColP + 1 == PrevColP)
                    I += C_CONTINUATION;
                if (LookAt(B, Line, 0, "{", hsC_Normal, 0)
                    && LookAt(B, RowP, ColP, "{", hsC_Normal, 0))
                    I -= 0;//C_BRACE_OFS;
                else if (LookAt(B, Line, 0, "{", hsC_Normal, 0)
                         && !LookAt(B, RowP, ColP, "{", hsC_Normal, 0))
                    I += C_BRACE_OFS;
                else if (!LookAt(B, Line, 0, "{", hsC_Normal, 0)
                         && LookAt(B, RowP, ColP, "{", hsC_Normal, 0))
                    I -= C_BRACE_OFS;
                PRINTF(("':' 1 indent : Line=%d, RowP=%d, ColP=%d, CharP=%c   BRACE %d\n", Line, RowP, ColP, CharP, I));
                break;
            case 'p':
                ColP++;
                //if (SkipWhite(B, Line, RowP, ColP, SKIP_FORWARD) != 1)
                //                    return 0;
                I = B->LineIndented(RowP) + C_CLASS_DELTA;
                //                if (FirstRowP == RowP) {
                //                    I += C_CLASS_DELTA;
                ///                if (LookAt(B, Line, 0, "{", hsC_Normal, 0)) {
                ///                    I += C_INDENT - C_BRACE_OFS;
                ///                }
                //                }
                PRINTF(("':' 2 indent : Line=%d, RowP=%d, ColP=%d, CharP=%c\n", Line, RowP, ColP, CharP));
                break;
            case 'l':
                ColP++;
                I = B->LineIndented(RowP) + C_BRACE_OFS;
                //C_CLASS_OFS + C_CLASS_DELTA;
                break;
            case 'c':
                ColP++;
                //                if (SkipWhite(B, Line, RowP, ColP, SKIP_FORWARD) != 1)
                //                    return 0;
                I = B->LineIndented(RowP) + C_CASE_DELTA;
                //                if (FirstRowP == RowP) {
                //                    I += C_CASE_DELTA;
                ///                if (LookAt(B, Line, 0, "{", hsC_Normal, 0)) {
                ///                        I += C_INDENT - C_BRACE_OFS;
                ///                }
                //                }

                PRINTF(("':' 3 indent : Line=%d, RowP=%d, ColP=%d, CharP=%c\n", Line, RowP, ColP, CharP));
                break;
            default:
                I = B->LineIndented(RowP);
                break;
            }
            if (((PrevRowP != RowP) ||
                 ((PrevRowP == RowP) && (PrevColP != ColP)))
                && FirstRowP != PrevRowP)
                ContinuationIndent = C_CONTINUATION;
            if (LookAt(B, Line, 0, "{", hsC_Normal, 0)) {
                //I -= C_INDENT - C_BRACE_OFS;
                ContinuationIndent = 0;
            }
            PRINTF(("':' -- indent : Line=%d, RowP=%d, ColP=%d, CharP=%c\n", Line, RowP, ColP, CharP));
            return I + ContinuationIndent;
#endif
        case 'i':
        case 's':
        case 'f':
        case 'e':
        case 'w':
            I = B->LineIndented(RowP);
            switch (CharP) {
            case 'i': ColP += 2; break; // if
            case 'f': ColP += 3; break; // for
            case 'e': ColP += 4; break; // else
            case 'w': ColP += 5; break; // while
            case 's': ColP += 6; break; // switch
            }
            PRINTF(("'ifews' -- indent 1: Line=%d, RowP=%d, ColP=%d, CharP=%c\n", Line, RowP, ColP, CharP));
            if (SkipWhite(B, Line, RowP, ColP,
                          SKIP_FORWARD | (CharP != 'e' ? SKIP_MATCH : 0)) != 1)
                return 0;
            if (RowP >= Line) {
                RowP = Line;
                ColP = -1;
            } else
                ColP--;
            if (SkipWhite(B, Line, RowP, ColP, SKIP_BACK) != 1)
                return 0;
            ColP++;
            PRINTF(("'ifews' -- indent 2: Line=%d, RowP=%d, ColP=%d, CharP=%c\n", Line, RowP, ColP, CharP));

            if (((PrevRowP != RowP) ||
                 ((PrevRowP == RowP) && (PrevColP != ColP)))
                && FirstRowP != PrevRowP)
                ContinuationIndent = C_CONTINUATION;

            I += C_INDENT;

            if (LookAt(B, Line, 0, "{", hsC_Normal, 0)) {
                I -= C_INDENT - C_BRACE_OFS;
                ContinuationIndent = 0;
            }

            return I + ContinuationIndent;

        // default: return 0;
        }
    }
    return 0;
}

static int IndentComment(EBuffer *B, int Row, int /*StateLen*/, hsState * /*StateMap*/) {
    int I = 0, R;

    //puts("Comment");

    if (Row > 0) {
        R = Row - 1;
        while (R >= 0) {
            if (B->RLine(R)->Count == 0) R--;
            else {
                I = B->LineIndented(R);
                break;
            }
        }
        if (B->RLine(Row - 1)->StateE == hsC_Comment)
            if (LookAt(B, Row - 1, I, "/*", hsC_Comment, 0)) I += C_COMMENT_DELTA;
        if (B->RLine(Row - 1)->StateE == hsC_CPP_Comm)
            if (LookAt(B, Row - 1, I, "/*", hsC_CPP_Comm, 0)) I += C_COMMENT_DELTA;
    }
    return I;
}

static int IndentCPP(EBuffer *B, int Line, int /*StateLen*/, hsState * /*StateMap*/) {
    if (LookAt(B, Line, 0, "#", hsC_CPP, 0))
        return 0;
    else
        return C_INDENT;
}

int Indent_C(EBuffer *B, int Line, int PosCursor) {
    int I;
    hsState *StateMap = NULL;
    int StateLen = 0;
    int OI;

    OI = I = B->LineIndented(Line);
    if (Line == 0) {
        I = 0;
    } else {
        if (I != 0) B->IndentLine(Line, 0);
        if (B->GetMap(Line, &StateLen, &StateMap) == 0) return 0;

        switch (B->RLine(Line - 1)->StateE) {
        case hsC_Comment:
        case hsC_CPP_Comm:
            I = IndentComment(B, Line, StateLen, StateMap);
            break;
        case hsC_CPP:
            /*case hsC_CPP_Comm:*/
        case hsC_CPP_String1:
        case hsC_CPP_String2:
        case hsC_CPP_ABrace:
            I = C_INDENT;
            break;
        default:
            if (StateLen > 0) {                 // line is not empty
                if (StateMap[0] == hsC_CPP || StateMap[0] == hsC_CPP_Comm ||
                    StateMap[0] == hsC_CPP_String1 || StateMap[0] == hsC_CPP_String2 ||
                    StateMap[0] == hsC_CPP_ABrace)
                {
                    I = IndentCPP(B, Line, StateLen, 0);
                } else {
                    I = IndentNormal(B, Line, StateLen, StateMap);
                    if ((StateMap[0] == hsC_Comment
                         || StateMap[0] == hsC_CommentL
                         || StateMap[0] == hsC_CPP_Comm)
                        && ((LookAt(B, Line, 0, "/*", hsC_Comment, 0)
                             || LookAt(B, Line, 0, "/*", hsC_CPP_Comm, 0)
                             || LookAt(B, Line, 0, "//", hsC_CommentL, 0))))
                    {
                        I += C_COMMENT_OFS;
                    } else if (CheckLabel(B, Line)) {
                        if (LookAt(B, Line, 0, "case", hsC_Keyword) ||
                            LookAt(B, Line, 0, "default", hsC_Keyword) ||
                            LookAt(B, Line, 0, "public:", hsC_Keyword, 0) ||
                            LookAt(B, Line, 0, "private:", hsC_Keyword, 0) ||
                            LookAt(B, Line, 0, "protected:", hsC_Keyword, 0))
                            ;
                        else
                            I += C_COLON_OFS;
                    } //else if (LookAt(B, Line, 0, "{", hsC_Normal, 0)) {
                    //    I -= C_INDENT - C_BRACE_OFS;
                    //}
                }
            } else {
                I = IndentNormal(B, Line, 0, NULL);
            }
            break;
        }
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
#endif // CONFIG_INDENT_C
#endif // CONFIG_HILIT_C
