/*    indent.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "e_buffer.h"

#ifdef CONFIG_INDENT_SIMPLE

/* place holder */

int Indent_SIMPLE(EBuffer *B, int Line, int /*PosCursor*/) {
    int Pos, Old;
    
    if (Line == 0) {
        Pos = 0;
    } else {
        if (B->RLine(Line - 1)->StateE == 0) {
            Pos = B->LineIndented(Line - 1);
        } else { // for comments, strings, etc, use same as prev line.
            Pos = B->LineIndented(Line - 1);
        }
    }

    Old = B->LineIndented(Line);
    if (Pos < 0)
        Pos = 0;
    if (Pos != Old)
        if (B->IndentLine(Line, Pos) == 0)
            return 0;;
    return 1;
}
#endif
