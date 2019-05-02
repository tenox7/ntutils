/*    h_make.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "fte.h"

#ifdef CONFIG_HILIT_MAKE

#include "c_bind.h"
#include "o_buflist.h"

#define hsMAKE_Normal  0
#define hsMAKE_Comment 1
#define hsMAKE_DotCmd  2
#define hsMAKE_Command 3

int Hilit_MAKE(EBuffer *BF, int /*LN*/, PCell B, int Pos, int Width, ELine *Line, hlState &State, hsState *StateMap, int *ECol) {
    HILIT_VARS(BF->Mode->fColorize->Colors, Line);
    int len1 = len;
    char *last = p + len1 - 1;
    
    for(i = 0; i < Line->Count;) {
        if (i == 0 && *p == 9) {
            State = hsMAKE_Command;
            Color = CLR_Command;
        }
        IF_TAB() else {
            if (i == 0) {
                if (*p == '.') {
                    State = hsMAKE_DotCmd;
                    Color = CLR_Directive;
                    goto hilit;
                } else if (*p == '#') {
                    State = hsMAKE_Comment;
                    Color = CLR_Comment;
                    goto hilit;
                }
            }
            switch(State) {
            case hsMAKE_Comment:
                Color = CLR_Comment;
                goto hilit;

            case hsMAKE_DotCmd:
                Color = CLR_Directive;
                goto hilit;
                
            case hsMAKE_Command:
                Color = CLR_Command;
                goto hilit;
                
            default:
                State = hsMAKE_Normal;
                Color = CLR_Normal;
            hilit:
                ColorNext();
                continue;
            }
        }
    }
    if((len1 == 0) || (*last != '\\')) {
        if (State == hsMAKE_Comment || State == hsMAKE_DotCmd || State == hsMAKE_Command)
            State = hsMAKE_Normal;
    }
    *ECol = C;
    return 0;
}

#endif
