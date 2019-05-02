/*    i_input.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef I_INPUT_H
#define I_INPUT_H

#include "i_oview.h"
#include "console.h"

typedef int (*Completer)(const char *Name, char *Completed, int Num);

class ExInput: public ExViewNext {
    StlString Prompt;
    size_t MaxLen;
    char *Line;
    char *MatchStr;
    char *CurStr;
    unsigned Pos;
    unsigned LPos;
    Completer Comp;
    int TabCount;
    int HistId;
    int CurItem;
    unsigned SelStart;
    unsigned SelEnd;

public:

    ExInput(const char *APrompt, const char *ALine, size_t AMaxLen, Completer AComp, int Select, int AHistId);
    virtual ~ExInput();

    virtual void HandleEvent(TEvent &Event);
    virtual void RepaintStatus();

    const char* GetLine() const { return Line; }
};

#endif // I_INPUT_H
