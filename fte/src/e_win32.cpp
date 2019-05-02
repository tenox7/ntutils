/*    e_win32.cpp
 *
 *    Copyright (c) 1997, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

// Win32 (NT) specific routines

#include "fte.h"
#include "c_bind.h"
#include "i_view.h"
#include "c_history.h"
#include "s_util.h"
#include "i_modelview.h"

// Silence warnings about redefinition, we don't need this here anyway.
#undef SEARCH_ALL

#include <windows.h>

int EView::SysShowHelp(ExState &State, const char *word) {
    char file[MAXPATH] = "";

    if (State.GetStrParam(this, file, sizeof(file) - 1) == 0)
        if (MView->Win->GetStr("Help file",
                               sizeof(file) - 1, file, HIST_DEFAULT) == 0)
            return 0;

    char wordAsk[64] = "";
    if (word == 0) {
        if (State.GetStrParam(this, wordAsk, sizeof(wordAsk) - 1) == 0)
            if (MView->Win->GetStr("Keyword",
                                   sizeof(wordAsk) - 1, wordAsk, HIST_DEFAULT) == 0)
                return 0;
        word = wordAsk;
    }

    //** Start WinHelp,
    if (!WinHelp(0, file, HELP_KEY, (DWORD)word)) {
        Msg(S_ERROR, "Failed to start WinHelp!");
        return 0;
    }
    return 1;
}
