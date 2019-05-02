/*    c_mode.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "c_bind.h"
#include "c_config.h"

#include <fcntl.h>

EBufferFlags DefaultBufferFlags = {
    {
        1,                  // AutoIndent
        1,                  // InsertOn
        0,                  // DrawOn
        1,                  // HilitOn
        1,                  // ExpandTabs
        0,                  // Trim
        8,                  // TabSize
        HILIT_PLAIN,        // HilitMode
        INDENT_PLAIN,       // IndentMode
        0,                  // ShowTab
        10,                 // LineChar
#if !defined(UNIX)
        13,                 // StripChar
#else
        -1,
#endif
        1,                  // AddLine
#if !defined(UNIX)
        1,                  // AddStrip
#else
        0,
#endif
        0,                  // ForceNewLine
        0,                  // HardMode
        1,                  // UndoRedo
        0,                  // ReadOnly
        0,                  // AutoSave
        1,                  // KeepBackups
        -1,                 // LoadMargin
        256,                // Max Undo/Redo Commands
        1,                  // MatchCase
        0,                  // BackKillTab
        0,                  // DelKillTab
        1,                  // BackSpUnindent
        0,                  // SpaceTabs
        1,                  // IndentWTabs
        1,                  // Wrap.LeftMargin
        72,                 // Wrap.RightMargin
        0,                  // See Thru Sel
        0,                  // WordWrap
        0,                  // ShowMarkers
        1,                  // CursorThroughTabs
        0,                  // Save Folds
        0,                  // MultiLineHilit
        0,                  // AutoHilitParen
        0,                  // Abbreviations
        0,                  // BackSpKillBlock
        0,                  // DeleteKillBlock
        1,                  // PersistentBlocks
        0,                  // InsertKillBlock
        0,                  // EventMap
        0,                  // UndoMoves
#ifdef UNIX
        0,                  // DetectLineSep
#else
        1,
#endif
        0,                  // trim on save
        0,                  // save bookmarks
        1,                  // HilitTags
        0,                  // ShowBookmarks
        1                   // MakeBackups
    },
    {
        0,                  // Routine Regexp
        0,                  // DefFindOpt
        0,                  // DefFindReplaceOpt
        0,                  // comment start (folds)
        0,                  // comment end (folds)
        0,                  // filename rx
        0,                  // firstline rx
        0                   // compile command
    }
};

EMode *GetModeForName(const char *FileName) {
    EMode *m;
    RxMatchRes RM;
    int fd;

    for (m = Modes; m; m = m->fNext)
	if (RxExec(m->MatchNameRx, FileName, strlen(FileName),
		   FileName, &RM) == 1)
                return m;

    if ((fd = open(FileName, O_RDONLY)) != -1) {
	char buf[81];
	int l = read(fd, buf, sizeof(buf) - 1);
        close(fd);
	if (l > 0) {
            int i;
	    for (i = 0; i < l && buf[i] != '\n'; ++i)
                ;

	    buf[i] = 0;
            for (m = Modes; m; m = m->fNext)
		if (RxExec(m->MatchLineRx, buf, i, buf, &RM) == 1)
		    return m;
	}
    }

    if ((m = FindMode(DefaultModeName)))
	return m;

    for (m = Modes; m && m->fNext; m = m->fNext)
        ;

    return m;
}
