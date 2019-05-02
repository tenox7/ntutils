/*    e_regex.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef E_REGEX_H
#define E_REGEX_H

#include <stddef.h>

/*
 * Operator:
 *
 * ^            Match the beginning of line
 * $            Match the end of line
 * .            Match any character
 * [ ]          Match characters in set
 * [^ ]         Match characters not in set
 * ?            Match previous pattern 0 or 1 times (greedy)
 * |            Match previous or next pattern
 * @            Match previous pattern 0 or more times (non-greedy)
 * #            Match previous pattern 1 or more times (non-greedy)
 * *            Match previous pattern 0 or more times (greedy)
 * +            Match previous pattern 1 or more times (greedy)
 * { }          Group characters to form one pattern
 * ( )          Group and remember
 * \            Quote next character (only of not a-z)
 * <            Match beginning of a word
 * >            Match end of a word
 * \x##         Match character with ASCII code ## (hex)
 * \N###        Match ascii code ### (dec)
 * \o###        Match ascii code
 * \a           Match \a
 * \b           Match \b
 * \f           Match \f
 * \n           Match 0x0a (lf)
 * \r           Match 0x0d (cr)
 * \t           Match 0x09 (tab)
 * \v           Match \v
 * \e           Match escape (^E)
 * \s           Match whitespace (cr/lf/tab/space)
 * \S           Match nonwhitespace (!\S)
 * \w           Match word character
 * \W           Match non-word character
 * \d           Match digit character
 * \D           Match non-digit character
 * \U           Match uppercase
 * \L           Match lowercase
 * \C           Match case sensitively from here on
 * \c           Match case ingnore from here on
 */
// *INDENT-OFF*
#define RE_NOTHING         0  // nothing
#define RE_JUMP            1  // jump to
#define RE_BREAK           2  // break |
#define RE_ATBOL           3  // match at beginning of line
#define RE_ATEOL           4  // match at end of line
#define RE_ATBOW           5  // match beginning of word
#define RE_ATEOW           6  // match end of word
#define RE_CASE            7  // match case sensitively from here
#define RE_NCASE           8  // ignore case from here.
#define RE_END            31  // end of regexp

#define RE_ANY      (32 +  1) // match any character
#define RE_INSET    (32 +  2) // match if in set
#define RE_NOTINSET (32 +  3) // match if not in set
#define RE_CHAR     (32 +  4) // match character string
#define RE_WSPACE   (32 +  5) // match whitespace
#define RE_NWSPACE  (32 +  6) // match whitespace
#define RE_UPPER    (32 +  7) // match uppercase
#define RE_LOWER    (32 +  8) // match lowercase
#define RE_DIGIT    (32 +  9) // match digit
#define RE_NDIGIT   (32 + 10) // match non-digit
#define RE_WORD     (32 + 11) // match word
#define RE_NWORD    (32 + 12) // match non-word

#define RE_GROUP         256  // grouping
#define RE_OPEN          512  // open (
#define RE_CLOSE        1024  // close )
#define RE_MEM          2048  // store () match

#define RE_BRANCH       4096
#define RE_GREEDY       2048  // do a greedy match (as much as possible)

#define NSEXPS            64  // for replace only 0-9

#define RX_CASE            1  // matchcase
// *INDENT-ON*

struct RxNode {
    short fWhat;
    short fLen;
    RxNode *fPrev;
    RxNode *fNext;
    union {
        char *fChar;
        RxNode *fPtr;
    };
};

struct RxMatchRes {
    int Open[NSEXPS];    // -1 = not matched
    int Close[NSEXPS];
};

RxNode *RxCompile(const char *Regexp);
int RxExecMatch(RxNode *Regexp, const char *Data, size_t Len, const char *Start, RxMatchRes *Match, unsigned int RxOpt = RX_CASE);
int RxExec(RxNode *Regexp, const char *Data, size_t Len, const char *Start, RxMatchRes *Match, unsigned int RxOpt = RX_CASE);
int RxReplace(const char *rep, const char *Src, size_t len, RxMatchRes match, char **Dest, size_t *Dlen);
void RxFree(RxNode *Node);

#endif // E_REGEX_H
