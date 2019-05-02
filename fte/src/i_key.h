/*    i_key.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef I_KEY_H
#define I_KEY_H

#include "i_oview.h"

class ExKey: public ExViewNext {
    StlString Prompt;
    TKeyCode Key;
    char ch;

public:

    ExKey(const char *APrompt);
    virtual ~ExKey();

    virtual void HandleEvent(TEvent &Event);
    virtual void RepaintStatus();

    TKeyCode GetKey() const { return Key; }
};

#endif // I_KEY_H
