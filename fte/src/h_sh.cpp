/*    h_sh.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "fte.h"

#ifdef CONFIG_HILIT_SH

#include "c_bind.h"
#include "o_buflist.h"
#include "sysdep.h"

#include <ctype.h>

enum {
    hsSH_Normal,
    hsSH_SQuote,
    hsSH_DQuote,
    hsSH_BQuote,
    hsSH_DBQuote,
    hsSH_Control,
    hsSH_Keyword,
    hsSH_Comment,
    hsSH_Variable,
    hsSH_EOF,
    hsSH_InOpt
};

#define MAXSEOF 100
static char seof[MAXSEOF];

int Hilit_SH(EBuffer *BF, int /*LN*/, PCell B, int Pos, int Width, ELine *Line, hlState &State, hsState *StateMap, int *ECol) {
    HILIT_VARS(BF->Mode->fColorize->Colors, Line);
    int CommandStr = 0;

    int isEOF = 0;
    for (i = 0; i < Line->Count;) {
        if (State == hsSH_EOF && 0 == i)
        {
            //printf("i=%d, len=%d, strlen(seof)=%d, seof=%s, Line-Chars=%s\n",
            //           i, len, strlen(seof), seof, Line->Chars);

            // Skip past any leading tabs.
            char* iseof = Line->Chars;
            size_t len_left = len;
            while (*iseof == '\t') ++iseof, --len_left;

            isEOF = strlen(seof) == len_left &&
                strncmp(seof, iseof, len_left) == 0;
        }
        IF_TAB() else {
            int j = 1;

            if (!isspace(*p))
                CommandStr++;
            Color = CLR_Normal;
            switch (State) {
            case hsSH_Normal:
                if (CommandStr == 1 && len > 2 && *p == '.' && isspace(p[1])) {
                    Color = CLR_Keyword;
                } else if (isalpha(*p) || *p == '_'
                           || ((CommandStr == 1)
                               && (*p == '/' || *p == '.'))) {
                    while (len > j
			   && (isalnum(p[j]) || strchr("_-[]$", p[j]) != NULL
                               || ((CommandStr == 1)
                                   && (p[j] == '/' || p[j] == '.'))))
                        j++;
                    if (p[j] == '=')
                        Color = CLR_Variable;
                    else if (p[j] == '*' || p[j] == ')')
                        Color = CLR_Normal;
                    else {
                        if (!BF->GetHilitWord(Color, p, j, 0)) {
                            // Color for good match is set by this function
                            Color = (CommandStr == 1) ? CLR_Command : CLR_Normal;
                            //printf("Command %d %c%c\n",
                            //CommandStr,
                            //Line->Chars[i],Line->Chars[i+1]);
			} else {
                            if (i > 0 && p[-1] != ';' && p[-1] != '('
                                && !isspace(p[-1]))
                                Color = CLR_Normal;
                            else {
                                int s;
                                switch(j) {
                                case 2:
                                    s = strncmp(p, "in", j);
                                    break;
                                case 3:
                                    s = strncmp(p, "for", j);
                                    break;
                                case 4:
                                    s = strncmp(p, "read", j);
                                    break;
                                case 5:
                                    s = strncmp(p, "unset", j);
                                    break;
                                case 6:
                                    s = strncmp(p, "export", j);
                                    break;
                                default:
                                    s = 1;
                                    break;
                                }
                                if (s)
                                    CommandStr = 0;
			    }
                        }
                    }
                    break;
                } else if (*p == '[' || *p == ']' ||
                           (CommandStr == 1 && *p == '!')) {
                    CommandStr = 0;
                    Color = CLR_Keyword; //Colors[CLR_Control];
                    //static a=0;
                    //if (!a) {for(int i=0;i<COUNT_CLR;i++)
                    //printf("%d   %d\n", i, Colors[i]);
                    //a++;}
                    //same state
                } else if (*p == '\'') {
                    State = hsSH_SQuote;
                    Color = CLR_String;
                } else if (*p == '"') {
                    State = hsSH_DQuote;
                    Color = CLR_String;
                } else if ( len >= 2 && *p == '\\' && p[1] == '\'' ) {
                    Color = CLR_String;
                    ColorNext();
                } else if ( len >= 2 && *p == '\\' && p[1] == '"' ) {
                    Color = CLR_String;
                    ColorNext();
                } else if ( len >= 2 && *p == '\\' && p[1] == '`' ) {
                    Color = CLR_Command;
                    ColorNext();
                } else if (*p == '`') {
                    State = hsSH_BQuote;
                    Color = CLR_Command;
                } else if (*p == '~') {
                    Color = CLR_Variable;
                } else if (*p == '$') {
                    State = hsSH_Variable;
                    Color = CLR_Variable;
                } else if (*p == '#') {
                    State = hsSH_Comment;
                    Color = CLR_Comment;
                    //} else if (isdigit(*p)) {
                    //Color = CLR_Number;
                    //while (len > 0 && (isdigit(*p)))
                    //ColorNext();
                    //continue;
                } else if (len > 3 && *p == '<' && p[1] == '<') {

                    if (p[2] == '<')
                    {
                        Color = CLR_Control;
                        ColorNext();
                        ColorNext();
                        ColorNext();
                    }
                    else
                    {
                        // !!! this is a hack, doesn't work properly -- Mark

                        char *s = seof;

                        j++;
                        if (p[2] == '-') j++;
                        Color = CLR_Control;
                        while (len > j && (isspace(p[j]) || p[j] == '\'' || p[j] == '"'))
                            j++;
                        if( p[j] == '\\' ) j++;
                        while (len > j && !(isspace(p[j]) || p[j] == '\'' || p[j] == '"'))
                            *s++ = p[j++];
                        if (len > j && (p[j] == '\'' || p[j] == '"')) j++;
                        *s = 0;
                        State = hsSH_EOF;
                        break;
                    }
                } else if (*p == '=' || *p == '\\' || *p == '>' ||
                           *p == '<' || *p == '!' /*|| *p == ':'*/) {
                    Color = CLR_Control;
                } else if (strchr(";|&(){}", *p) != NULL) {
                    CommandStr = 0;
                    Color = CLR_Control;
                }
                break;
            case hsSH_SQuote:
                Color = CLR_String;
                if ((len >= 2) && (*p == '\\'))
                    j++;
                else if (*p == '\'')
                    State = hsSH_Normal;
                break;
            case hsSH_DQuote:
                Color = CLR_String;
                if ((len >= 2) && (*p == '\\'))
                    j++;
                else if (*p == '"')
                    State = hsSH_Normal;
                else if (*p == '`') {
                    Color = CLR_Command;
                    State = hsSH_DBQuote;
                }
                break;
            case hsSH_BQuote:
                Color = CLR_Command;
                if ((len >= 2) && (*p == '\\'))
                    j++;
                else if (*p == '`')
                    State = hsSH_Normal;
                break;
            case hsSH_DBQuote:
                Color = CLR_Command;
                if ((len >= 2) && (*p == '\\'))
                    j++;
                else if (*p == '`')
                    State = hsSH_DQuote;
                break;
            case hsSH_Variable:
                Color = CLR_Variable;
                State = hsSH_Normal;
                if (!isdigit(*p)) {
                    int b = 1;
                    if (*p == '{')
                        b = 2;
                    else if (*p == '[')
                        b = 3;
                    while (b && len > 0 &&
                           (isalnum(*p) ||
                            (strchr("{}[]_", *p) != NULL) ||
                            (b == 2 && (strchr("#%:-=?+/", *p) != NULL)) ||
                            (b == 1 && (strchr("*@#?-$!", *p) != NULL) /*|| (b == 0, 1) ????? */)
                           )
                          )
                    { // !!!!! ?????
                        // in ${...}, once we hit a :, %, or #, anything goes.
                        // Ideally, we'd do normal processing inside as well,
                        // but that'll take much, much longer to figure out
                        // how to do.
                        if (b == 2 && strchr("#%:", *p) != NULL)
                        {
                            UntilMatchBrace('{',ColorNext());
                            break;
                        }
                        if (b == 2 && *p == '}')
                            b = 0;
                        else if (b == 3 && *p == ']')
                            b = 0;
                        ColorNext();
                    }
                    continue;
                }
                break;
            case hsSH_Comment:
                Color = CLR_Comment;
                break;
            case hsSH_EOF:
                Color = CLR_String;

                if (isEOF)
                {
                    Color = CLR_Control;
                    State = hsSH_Normal;
                    j += len - 1;
                }
                break;
            default:
                State = hsSH_Normal;
                Color = CLR_Normal;
            }

            if (StateMap)
                memset(StateMap + i, State, j);
            if (B)
                MoveMem(B, C - Pos, Width, p, HILIT_CLRD(), j);
            i += j;
            len -= j;
            p += j;
            C += j;
        }
    }
    if (State == hsSH_Comment || State == hsSH_Variable)
        State = hsSH_Normal;
    *ECol = C;
    return 0;
}
#endif
