/*    i_choice.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef I_CHOICE_H
#define I_CHOICE_H

#include "i_oview.h"

#include <stdarg.h>

class ExChoice: public ExViewNext {
    struct option {
        StlString str;
        int len;

        option() : len(0) {}
        option(const char *s) : str(s), len((int)CStrLen(s)) {}
        const char* GetCStr() { return str.c_str(); };
        int GetCStrLen() { return len; }
    };
    StlString Title;
    int lTitle;
    char Prompt[160];
    int NOpt;
    StlVector<option> SOpt;
    int lChoice;
    int Cur;
    int MouseCaptured;

    int FindChoiceByPoint(int x, int y);

public:

    ExChoice(const char *ATitle, int NSel, va_list ap /* choices, format, args */);
    virtual ~ExChoice();

    virtual void HandleEvent(TEvent &Event);
    virtual void RepaintStatus();
};

#endif // I_CHOICE_H
