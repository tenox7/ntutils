/*    i_key.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "i_key.h"
#include "sysdep.h"

ExKey::ExKey(const char *APrompt) :
    Prompt(APrompt)
{
}

ExKey::~ExKey() {
}

void ExKey::HandleEvent(TEvent &Event) {
    switch (Event.What) {
    case evKeyDown:
        Key = Event.Key.Code;
        if (!(Key & kfModifier)) // not ctrl,alt,shift, ....
            EndExec(1);
        Event.What = evNone;
        break;
    }
}

void ExKey::RepaintStatus() {
    TDrawBuffer B;
    int W, H;

    ConQuerySize(&W, &H);

    MoveCh(B, ' ', 0x17, W);
    ConPutBox(0, H - 1, W, 1, B);
}
