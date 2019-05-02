/*    h_perl.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

/*
 * Perl Mode
 *
 * TODO:
 *    here documents, formats
 *    OS/2 EXTPROC...,
 *    UNIX #! starts hilit ?
 *    POD highlighting (need two keyword sets).
 *    some tab handling (in & foo, etc -- if allowed)/
 */

#include "fte.h"

#ifdef CONFIG_HILIT_PERL

#include "c_bind.h"
#include "o_buflist.h"

#include <ctype.h>

#define X_BIT     0x80     /* set if last was number, var, */
#define X_MASK    0x7F
#define X_NOT(state) (!((state) & X_BIT))

#define kwd(x) (isalnum(x) || (x) == '_')

#define IS_OBRACE(x) \
    ((x) == '(' || (x) == '{' || (x) == '<' || (x) == '[')

#define NUM_BRACE(x) \
    ( \
    (x) == '(' ? 0U : \
    (x) == '{' ? 1U : \
    (x) == '<' ? 2U : \
    (x) == '[' ? 3U : 0U \
    )

#define GET_BRACE(x) \
    ( \
    (x) == 0 ? '(' : \
    (x) == 1 ? '{' : \
    (x) == 2 ? '<' : \
    (x) == 3 ? '[' : 0 \
    )

#define IS_MBRACE(y,x) \
    ( \
    ((y) == '(' && (x) == ')') || \
    ((y) == '{' && (x) == '}') ||\
    ((y) == '<' && (x) == '>') ||\
    ((y) == '[' && (x) == ']') \
    )

#define QCHAR(state) ((char)(((state) >> 8) & 0xFF))
#define QSET(state, ch) ((unsigned short)((unsigned short)(state) | (((unsigned short)(ch)) << 8)))

#define hsPerl_Punct        0
#define hsPerl_Comment      1
#define hsPerl_Normal      30
#define hsPerl_Keyword      4
#define hsPerl_String1     10
#define hsPerl_String2     11
#define hsPerl_StringBk    22
#define hsPerl_Variable    23
#define hsPerl_Number      24
#define hsPerl_Function    25
#define hsPerl_RegexpM     26
#define hsPerl_RegexpS1    28
#define hsPerl_RegexpS2    29
#define hsPerl_Docs        31
#define hsPerl_Data        32
#define hsPerl_RegexpS3    33

#define hsPerl_Quote1Op    35
#define hsPerl_Quote1      36
#define hsPerl_Quote1M     37

#define hsPerl_Regexp1Op   38
#define hsPerl_Regexp1     39
#define hsPerl_Regexp1M    40

#define hsPerl_Regexp2Op   41
#define hsPerl_Regexp2     42
#define hsPerl_Regexp2M    43

#define hsPerl_HereDoc     44 // hack (eod not detected properly)

enum PerlOps
{
    opQ,
    opQQ,
    opQW,
    opQX,
    opQR,
    opM,
    opS,
    opTR
};
/*#define opQ  1
#define opQQ 2
#define opQW 3
#define opQX 4
#define opM  5
#define opS  6
#define opTR 7*/

int Hilit_PERL(EBuffer *BF, int /*LN*/, PCell B, int Pos, int Width, ELine *Line, hlState &State, hsState *StateMap, int *ECol) {
    int j;
    HILIT_VARS(BF->Mode->fColorize->Colors, Line);
    int firstnw = 0;
    int op;
    int setHereDoc = 0;
    int inSub = 0;
#define MAXSEOF 100
    static char hereDocKey[MAXSEOF];

    C = 0;
    NC = 0;
    int isEOHereDoc = 0;
    if ((State & X_MASK) == hsPerl_HereDoc)
    {
	isEOHereDoc =
	    (strlen(hereDocKey) == (size_t)len &&
	     strncmp(hereDocKey, Line->Chars, len) == 0)
	    ||
	    (strlen(hereDocKey) == (size_t)len - 1 &&
             Line->Chars[len - 1] == '\r' &&
	     strncmp(hereDocKey, Line->Chars, len - 1) == 0
	    );
        if (isEOHereDoc) State = hsPerl_Normal | (State & X_BIT);
    }
    for(i = 0; i < Line->Count;) {
        if (*p != ' ' && *p != 9) firstnw++;
        if (*p == '{' && inSub)
            inSub = 0;
        IF_TAB() else {
            //         printf("State = %d pos = %d", State, i); fflush(stdout);
            switch (State & X_MASK) {
            default:
            case hsPerl_Normal:
                if (i == 0 && X_NOT(State) && len == 8 &&
                           p[0] == '_' &&
                           p[1] == '_' &&
                           p[2] == 'D' &&
                           p[3] == 'A' &&
                           p[4] == 'T' &&
                           p[5] == 'A' &&
                           p[6] == '_' &&
                           p[7] == '_')
                {
                    State = hsPerl_Data;
                    Color = CLR_Comment;
                hilit6:
                    ColorNext();
                hilit5:
                    ColorNext();
                hilit4:
                    ColorNext();
                //hilit3:
                    ColorNext();
                hilit2:
                    ColorNext();
                hilit:
                    ColorNext();
                    continue;
                } else if (
                           i == 0 && X_NOT(State) && len == 7 &&
                           p[0] == '_' &&
                           p[1] == '_' &&
                           p[2] == 'E' &&
                           p[3] == 'N' &&
                           p[4] == 'D' &&
                           p[5] == '_' &&
                           p[6] == '_')
                {
                    State = hsPerl_Data;
                    Color = CLR_Comment;

                } else if (
                           i == 0 && X_NOT(State) && (*p == '=') && len > 5 &&
                           (
                            strncmp(p+1, "begin", 5) == 0
                           )
                          )
                {
                    State = hsPerl_Docs;
                    Color = CLR_Comment;
                    goto hilit6;
                } else if (
                           i == 0 && X_NOT(State) && (*p == '=') && len > 4 &&
                           (
                            strncmp(p+1, "head", 4) == 0 ||
                            strncmp(p+1, "item", 4) == 0 ||
                            strncmp(p+1, "over", 4) == 0 ||
                            strncmp(p+1, "back", 4) == 0
                           )
                          )
                {
                    State = hsPerl_Docs;
                    Color = CLR_Comment;
                    goto hilit5;
                } else if (
                           i == 0 && X_NOT(State) && (*p == '=') && len > 3 &&
                           (
                            strncmp(p+1, "pod",  3) == 0 ||
                            strncmp(p+1, "for",  3) == 0 ||
                            strncmp(p+1, "end",  3) == 0 ||
                            0
                           )
                          )
                {
                    State = hsPerl_Docs;
                    Color = CLR_Comment;
                    goto hilit4;
                } else if (isalpha(*p) || *p == '_') {
                    op = -1;

                    j = 0;
                    while (((i + j) < Line->Count) &&
                           (isalnum(Line->Chars[i+j]) ||
                            (Line->Chars[i + j] == '_' || Line->Chars[i + j] == '\''))
                           ) j++;
                    if (BF->GetHilitWord(Color, &Line->Chars[i], j)) {
                        //Color = hcPERL_Keyword;
                        State = hsPerl_Keyword;
                        if (strncmp(p, "sub", 3) == 0)
                        {
                            inSub = 1;
                        }
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
                        State = hsPerl_Normal;
                    }
                    if (j == 1) {
                        if (*p == 'q') op = opQ;
                        else if (*p == 's' || *p == 'y') op = opS;
                        else if (*p == 'm') op = opM;
                    } else if (j >= 2) {
                        if (*p == 'q') {
                            if (p[1] == 'q') op = opQQ;
                            else if (p[1] == 'w') op = opQW;
                            else if (p[1] == 'r') op = opQR;
                            else if (p[1] == 'x') op = opQX;
                        } else if (*p == 't' && p[1] == 'r') op = opTR;
                        if (op != -1 && j > 2 && p[2] == '\'')
			    j = 2;
			else if (op != -1 && kwd(p[2]))
                            op = -1;
                    }
                    if (StateMap)
                        memset(StateMap + i, State, j);
                    if (B)
                        MoveMem(B, C - Pos, Width, Line->Chars + i, HILIT_CLRD(), j);
                    i += j;
                    len -= j;
                    p += j;
                    C += j;

                    switch (op) {
                    case opQ:
                        State = hsPerl_Quote1Op;   // q{} operator
                        Color = CLR_Punctuation;
                        continue;

                    case opQQ:
                    case opQW:
                    case opQX:
                        State = hsPerl_Quote1Op;   // qq{} qx{} qw{} operators
                        Color = CLR_Punctuation;
                        continue;

                    case opM:
                    case opQR:
                        State = hsPerl_Regexp1Op;   // m{} qr{} operator
                        Color = CLR_Punctuation;
                        continue;

                    case opTR:
                        State = hsPerl_Regexp2Op;   // tr{} operators
                        Color = CLR_RegexpDelim;
                        continue;

                    case opS:
                        State = hsPerl_Regexp2Op;   // s{}{} operator
                        Color = CLR_Punctuation;
                        continue;

                    default:
                        State = hsPerl_Normal;
                        continue;
                    }
                } else if (len >= 2 && ((*p == '-' && p[1] == '-') || (*p == '+' && p[1] == '+'))) {
                    hlState s = State;
                    State = hsPerl_Punct;
                    Color = CLR_Punctuation;
                    ColorNext();
                    ColorNext();
                    State = s;
                    continue;
                } else if (inSub && *p == '(') {
                    State = hsPerl_Punct;
                    Color = CLR_Punctuation;
                    ColorNext();
                    Color = CLR_Variable;
                    while (*p != ')' && len)
                        ColorNext();
                    State = hsPerl_Normal;
                    continue;
                } else if (len >= 2 && *p == '&' && (p[1] == '&' || isspace(p[1]))) {
                    State = hsPerl_Punct;
                    Color = CLR_Punctuation;
                    ColorNext();
                    ColorNext();
                    State = hsPerl_Normal;
                    continue;
                } else if (*p == '&' && (len < 2 || p[1] != '&') && X_NOT(State)) {
                    State = hsPerl_Function;
                    Color = CLR_Function;
                    ColorNext();
                    while ((len > 0) && (*p == '$' ||
                                         *p == '@' ||
                                         *p == '*' ||
                                         *p == '%' ||
                                         *p == '\\'))
                        ColorNext();
                    while ((len > 0) && (isalnum(*p) ||
                                         *p == '_' ||
                                         *p == '\''))
                        ColorNext();
                    State = hsPerl_Normal | X_BIT;
                    continue;
                } else if ((*p == '$' || *p == '*') && (len > 1) &&
                           (p[1] == '"')) {
                    State = hsPerl_Variable;
                    Color = CLR_Variable;
                    ColorNext();
                    ColorNext();
                    State = hsPerl_Normal | X_BIT;
                    continue;
                } else if (*p == '*' && len > 1 && p[1] == '*') {
                    hlState s = State;
                    State = hsPerl_Punct;
                    Color = CLR_Punctuation;
                    ColorNext();
                    ColorNext();
                    State = s;
                    continue;
                } else if (*p == '$' || *p == '@' || *p == '\\' ||
                           (len > 2 && (*p == '%' || *p == '*') && X_NOT(State))
                          ) {
                    char var_type = *p;
                    State = hsPerl_Variable;
                    Color = CLR_Variable;
                    ColorNext();
                    while ((len > 0) && ((*p == ' ') || (*p == '\t'))) {
                        IF_TAB() else
                            ColorNext();
                    }
                    /*while ((len > 0) && (*p == '$' ||
                                         *p == '@' ||
                                         *p == '*' ||
                                         *p == '%' ||
                                         *p == '\\'))
                        ColorNext();*/
                    char first = *p;
                    if (len > 0 && *p != ' ' && *p != '\t' && *p != '"')
                        ColorNext();
                    // the following are one-character-ONLY
                    if (
                        (var_type == '$' && strchr("_&`'+*.!/|,\\\";#%=-~:?<>()[]", first) != NULL) ||
                        (var_type == '@' && strchr("_", first) != NULL)
                       )
                    {
                        // nothing.
                    }
                    // the following are one-or-two-characters-ONLY
                    else if (first == '^')
                    {
                        if (len > 0 && isalpha(*p))
                            ColorNext();
                    }
                    else if (first == '{')
                    {
                        UntilMatchBrace(first, ColorNext());
                    }
                    else
                    {
                        while ((len > 0) && (isalnum(*p) ||
                                             (first == '{' && (*p == '^' || *p == '}')) ||
                                             *p == '_' ||
                                             *p == '\'')
                              )
                            ColorNext();
                    }
                    State = hsPerl_Normal | X_BIT;
                    continue;
                } else if ((len >= 2) && (*p == '0') && (*(p+1) == 'x')) {
                    State = hsPerl_Number;
                    Color = CLR_Number;
                    ColorNext();
                    ColorNext();
                    while (len && (isxdigit(*p) || *p == '_')) ColorNext();
                    //                    if (len && (toupper(*p) == 'U')) ColorNext();
                    //                    if (len && (toupper(*p) == 'L')) ColorNext();
                    State = hsPerl_Normal | X_BIT;
                    continue;
                } else if (isdigit(*p)) {
                    State = hsPerl_Number;
                    Color = CLR_Number;
                    ColorNext();
                    while (len && (isdigit(*p) || (*p == 'e' || *p == 'E' || *p == '_'))) ColorNext();
                    //                    if (len && (toupper(*p) == 'U')) ColorNext();
                    //                    if (len && (toupper(*p) == 'L')) ColorNext();
                    State = hsPerl_Normal | X_BIT;
                    continue;
                } else if (*p == '\'') {
                    State = QSET(hsPerl_String1, '\'');
                    Color = CLR_String;
                    goto hilit;
                } else if (*p == '"') {
                    State = QSET(hsPerl_String2, '"');
                    Color = CLR_String;
                    goto hilit;
                } else if (*p == '<' && len > 2 && p[1] == '<') {
                    int offset = 2;
                    while (p[offset] == '"' ||
                           p[offset] == '\'' ||
                           isspace(p[offset]) ||
                           0)
                    {
                        ++offset;
                    }
                    if (p[offset] == '_' || (toupper(p[offset]) >= 'A' && toupper(p[offset]) <= 'Z'))
                    {
                        int hereDocKeyLen;
                        setHereDoc++;
                        for (hereDocKeyLen = 0;
                             hereDocKeyLen < len && (
                                                     p[offset + hereDocKeyLen] == '_' ||
                                                     (toupper(p[offset + hereDocKeyLen]) >= 'A' && toupper(p[offset + hereDocKeyLen]) <= 'Z')
                                                    );
                             ++hereDocKeyLen)
                        {
                            hereDocKey[hereDocKeyLen] = p[offset + hereDocKeyLen];
                        }
                        hereDocKey[hereDocKeyLen] = '\0';
                        State = hsPerl_Punct;
                        Color = CLR_Punctuation;
                        ColorNext();
                        State = hsPerl_Normal;
                        continue;
                    }
                } else if (*p == '`') {
                    State = QSET(hsPerl_StringBk, '`');
                    Color = CLR_String;
                    goto hilit;
                } else if (*p == '#') {
                    State = hsPerl_Comment | (State & X_BIT);
                    continue;
                } else if (X_NOT(State) && *p == '/') {
                    State = QSET(hsPerl_Regexp1, '/');
                    Color = CLR_RegexpDelim;
                    goto hilit;
                } else if (X_NOT(State) &&
                           *p == '-' &&
                           len >= 2 &&
                           isalpha(p[1])
                          ) {
                    Color = CLR_Normal; // default.
                    if (strchr("wrxoRWXOezsfdlpSbctugkTB", p[1]) != NULL) {
                        Color = CLR_Punctuation; // new default.
                        if (len > 2) {
                            switch(p[2]) {
                            case '_': // there may be others...
                                Color = CLR_Normal;
                                break;
                            default:
                                if (isalnum(p[2]))
                                    Color = CLR_Normal;
                                break;
                            }
                        }
                    }
                    ColorNext();
                    ColorNext();
                    State = hsPerl_Normal;
                    continue;
                } else if (*p == ')' || *p == ']') {
                    State = hsPerl_Punct;
                    Color = CLR_Punctuation;
                    ColorNext();
                    State = hsPerl_Normal | X_BIT;
                    continue;
                } else if (ispunct(*p)) {
                    State = hsPerl_Punct;
                    Color = CLR_Punctuation;
                    ColorNext();
                    State = hsPerl_Normal;
                    continue;
                }
                Color = CLR_Normal;
                goto hilit;
            case hsPerl_Quote1Op:
                if (*p != ' ' && !kwd(*p)) {
                    if (IS_OBRACE(*p))
                    {
                        State = QSET(hsPerl_Quote1M,
                                     (1U << 2) | NUM_BRACE(*p));
                    }
                    else
                    {
                        State = QSET(hsPerl_Quote1, *p);
                    }
                    Color = CLR_QuoteDelim;
                    goto hilit;
                } else if (kwd(*p)) {
                    State = hsPerl_Normal | X_BIT;
                    continue;
                }
                Color = CLR_Punctuation;
                goto hilit;
            case hsPerl_Quote1:
                Color = CLR_String;
                if ((len >= 2) && (*p == '\\')) {
                    goto hilit2;
                } else if (*p == QCHAR(State)) {
                    Color = CLR_QuoteDelim;
                    ColorNext();
                    State = hsPerl_Normal | X_BIT;
                    continue;
                }
                goto hilit;
            case hsPerl_Quote1M:
                Color = CLR_String;
                if ((len >= 2) && (*p == '\\')) {
                    goto hilit2;
                } else if (GET_BRACE(QCHAR(State) & 3) == *p) {
                    State += 1 << (2 + 8);
                    goto hilit;
                } else if (IS_MBRACE(GET_BRACE(QCHAR(State) & 3), *p)) {
                    State -= 1 << (2 + 8);
                    if ((QCHAR(State) >> 2) == 0) {
                        Color = CLR_QuoteDelim;
                        ColorNext();
                        State = hsPerl_Normal | X_BIT;
                    } else
                        goto hilit;
                    continue;
                }
                goto hilit;
            case hsPerl_Regexp1Op:
                if (*p != ' ') { // && !kwd(*p)) {
                    if (IS_OBRACE(*p))
                        State = QSET(hsPerl_Regexp1M,
                                     (1U << 2) | NUM_BRACE(*p));
                    else
                        State = QSET(hsPerl_Regexp1, *p);
                    Color = CLR_RegexpDelim;
                    goto hilit;
                } else if (kwd(*p)) {
                    State = hsPerl_Normal | X_BIT;
                    continue;
                }
                Color = CLR_Regexp;
                goto hilit;
            case hsPerl_Regexp1:
                Color = CLR_Regexp;
                if ((len >= 2) && (*p == '\\')) {
                    goto hilit2;
                } else if (*p == QCHAR(State)) {
                    Color = CLR_RegexpDelim;
                    ColorNext();
                    Color = CLR_Punctuation;
                    while (len > 0 && isalpha(*p))
                        ColorNext();
                    State = hsPerl_Normal | X_BIT;
                    continue;
                }
                goto hilit;
            case hsPerl_Regexp1M:
                Color = CLR_Regexp;
                if ((len >= 2) && (*p == '\\')) {
                    goto hilit2;
                } else if (GET_BRACE(QCHAR(State) & 3) == *p) {
                    State += 1 << (2 + 8);
                    goto hilit;
                } else if (IS_MBRACE(GET_BRACE(QCHAR(State) & 3), *p)) {
                    State -= 1 << (2 + 8);
                    if ((QCHAR(State) >> 2) == 0) {
                        Color = CLR_RegexpDelim;
                        ColorNext();
                        Color = CLR_Punctuation;
                        while (len > 0 && isalpha(*p))
                            ColorNext();
                        State = hsPerl_Normal | X_BIT;
                    } else
                        goto hilit;
                    continue;
                }
                goto hilit;
            case hsPerl_Regexp2Op:
                if (*p != ' ') { // && !kwd(*p)) {
                    if (IS_OBRACE(*p))
                        State = QSET(hsPerl_Regexp2M,
                                     (1U << 2) | NUM_BRACE(*p));
                    else
                        State = QSET(hsPerl_Regexp2, *p);
                    Color = CLR_RegexpDelim;
                    goto hilit;
                } else if (kwd(*p)) {
                    State = hsPerl_Normal | X_BIT;
                    continue;
                }
                Color = CLR_Regexp;
                goto hilit;
            case hsPerl_Regexp2:
                Color = CLR_Regexp;
                if ((len >= 2) && (*p == '\\')) {
                    goto hilit2;
                } else if (*p == QCHAR(State)) {
                    Color = CLR_RegexpDelim;
                    ColorNext();
                    /*State = hsPerl_Normal | X_BIT;*/
                    State = QSET(hsPerl_Regexp1, QCHAR(State));
                    continue;
                }
                goto hilit;
            case hsPerl_Regexp2M:
                Color = CLR_Regexp;
                if ((len >= 2) && (*p == '\\')) {
                    goto hilit2;
                } else if (GET_BRACE(QCHAR(State) & 3) == *p) {
                    State += 1 << (2 + 8);
                    goto hilit;
                } else if (IS_MBRACE(GET_BRACE(QCHAR(State) & 3), *p)) {
                    State -= 1 << (2 + 8);
                    if ((QCHAR(State) >> 2) == 0) {
                        Color = CLR_RegexpDelim;
                        ColorNext();
                        /*State = hsPerl_Normal | X_BIT;*/
                        State = hsPerl_Regexp1Op;
                    } else
                        goto hilit;
                    continue;
                }
                goto hilit;
            case hsPerl_Data:
                Color = CLR_Comment;
                goto hilit;
            case hsPerl_HereDoc:
                if (!isEOHereDoc)
                {
                    Color = CLR_String;
                    goto hilit;
                }
                Color = CLR_Punctuation;
                setHereDoc = QCHAR(State);
                while (len > 0)
                    ColorNext();
                State = hsPerl_Normal | (State & X_BIT);
                continue;
            case hsPerl_Docs:
                Color = CLR_Comment;
                if (i == 0 && *p == '=' && len > 3 &&
                    p[1] == 'c' && p[2] == 'u' && p[3] == 't')
                {
                    ColorNext();
                    ColorNext();
                    ColorNext();
                    ColorNext();
                    State = hsPerl_Normal;
                    Color = CLR_Normal;
                    continue;
                }
                goto hilit;
            case hsPerl_Comment:
                Color = CLR_Comment;
                goto hilit;
            case hsPerl_String1:
                Color = CLR_String;
                if ((len >= 2) && (*p == '\\')) {
                    goto hilit2;
                } else if (*p == QCHAR(State)) {
                    ColorNext();
                    State = hsPerl_Normal | X_BIT;
                    continue;
                }
                goto hilit;
            case hsPerl_String2:
                Color = CLR_String;
                if ((len >= 2) && (*p == '\\')) {
                    goto hilit2;
                } else if (*p == QCHAR(State)) {
                    ColorNext();
                    State = hsPerl_Normal | X_BIT;
                    continue;
                }
                goto hilit;
            case hsPerl_StringBk:
                Color = CLR_String;
                if ((len >= 2) && (*p == '\\')) {
                    goto hilit2;
                } else if (*p == QCHAR(State)) {
                    ColorNext();
                    State = hsPerl_Normal | X_BIT;
                    continue;
                }
                goto hilit;
            }
        }
    }
    if ((State & X_MASK) == hsPerl_Comment)
        State = hsPerl_Normal | (State & X_BIT);
    if (setHereDoc)
        State = QSET(hsPerl_HereDoc | (State & X_BIT), setHereDoc - 1);
    *ECol = C;
    return 0;
}
#endif // CONFIG_HILIT_PERL
