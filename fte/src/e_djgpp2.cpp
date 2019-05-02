/*    e_djgpp2.cpp
 *
 *    Copyright (c) 1997, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 *  Contributed by Markus F.X.J. Oberhumer <markus.oberhumer@jk.uni-linz.ac.at>
 */

// djgpp2 specific routines

#include "c_bind.h"
#include "o_model.h"
#include "sysdep.h"

int EView::SysShowHelp(ExState &State, const char *word) {
    Msg(S_ERROR, "Not yet implemented");
    return 0;
}
