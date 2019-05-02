/*    i_ascii.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef I_COMPLETE_H
#define I_COMPLETE_H

#include "i_oview.h"
#include "e_buffer.h"

#include <ctype.h>

// maximum words which will be presented to the user
#define MAXCOMPLETEWORDS 300

class ExComplete: public ExViewNext {
    EPoint Orig;
    EBuffer *Buffer;
    int WordsLast;
    char **Words;
    char *WordBegin;
    char *WordContinue;
    int WordPos;
    size_t WordFixed;
    size_t WordFixedCount;

    int RefreshComplete();
    int CheckASCII(char c) const { return (isalnum(c) || (c == '_') || (c == '.')); }
    void FixedUpdate(int add);

public:

    ExComplete(EBuffer *B);
    virtual ~ExComplete();

    virtual void HandleEvent(TEvent &Event);
    virtual void RepaintStatus();

    bool IsSimpleCase();
    int DoCompleteWord();
};

#endif // I_COMPLETE_H
