/*    e_os2.cpp
 *
 *    Copyright (c) 1997, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

// Win32 (NT) specific routines

#include "c_bind.h"
#include "o_model.h"
#include "sysdep.h"

int EView::SysShowHelp(ExState &State, const char *word) {
    char file[MAXPATH] = "";
    char cmd[1024];

    if (State.GetStrParam(this, file, sizeof(file)) == 0)
	if (MView->Win->GetStr("Help file",
			       sizeof(file), file, HIST_DEFAULT) == 0)
            return 0;

    char wordAsk[64] = "";
    if (word == 0) {
        if (State.GetStrParam(this, wordAsk, sizeof(wordAsk)) == 0)
            if (MView->Win->GetStr("Keyword",
                                   sizeof(wordAsk), wordAsk, HIST_DEFAULT) == 0)
                return 0;
        word = wordAsk;
    }

    int i = snprintf(cmd, sizeof(cmd), "%s %s %s", HelpCommand, file, word);
    if (i > 0 &&  i < sizeof(cmd) && system(cmd) == 0)
        return 1;

    Msg(S_ERROR, "Failed to start view.exe!");
    return 0;
}
