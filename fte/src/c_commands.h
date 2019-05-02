/*    c_commands.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef C_COMMANDS_H
#define C_COMMANDS_H

enum ExCommands {
    ExNop,
    ExFail,

    //<cmd_cursor> Cursor Movement

    //& <A HREF="modes.html#ms.CursorTroughTabs">CursorTroughTabs</A>
    
    ExMoveDown,
    /// Move cursor to next line.
    ExMoveUp,
    /// Move cursor to previous line
    ExMoveLeft,
    /// Move cursor to previous column.
    ExMoveRight,
    /// Move cursor to next column.
    ExMovePrev,
    /// Move cursor to previous character. Moves to end of the previous
    /// line if cursor is at the beginning of line.
    ExMoveNext,
    /// Move cursor to next character. Moves to the beginning of next
    /// line if cursor is at the end of line.
    ExMoveWordLeft,
    /// Move cursor to the beginning of the word on the left.
    ExMoveWordRight,
    /// Move cursor to the beginning of the word on the right.
    ExMoveWordPrev,
    /// Move cursor to the beginning of the previous word.
    ExMoveWordNext,
    /// Move cursor to the beginning of the next word.
    ExMoveWordEndLeft,
    /// Move cursor to the end of the previous word.
    ExMoveWordEndRight,
    /// Move cursor to the end of the word on the right.
    ExMoveWordEndPrev,
    /// Move cursor to the end of the previous word.
    ExMoveWordEndNext,
    /// Move cursor to the end of the next word.
    ExMoveWordOrCapLeft,
    /// Move cursor to the beginning of the word or capital letter on the right.
    ExMoveWordOrCapRight,
    /// Move cursor to the beginning of the word or capital letter on the left.
    ExMoveWordOrCapPrev,
    /// Move cursor to the beginning of the previous word or to previous
    /// capital letter.
    ExMoveWordOrCapNext,
    /// Move cursor to the beginning of the next word or to next capital letter.
    ExMoveWordOrCapEndLeft,
    /// Move cursor to the end of the word or capitals on the left.
    ExMoveWordOrCapEndRight,
    /// Move cursor to the end of the word or capitals on the right.
    ExMoveWordOrCapEndPrev,
    /// Move cursor to the end of the previous word or capitals.
    ExMoveWordOrCapEndNext,
    /// Move cursor to the end of the next word or capitals.
    ExMoveLineStart,
    /// Move cursor to the beginning of line.
    ExMoveLineEnd,
    /// Move cursor to the end of line.
    ExMovePageStart,
    /// Move cursor to the first line on current page.
    ExMovePageEnd,
    /// Move cursor to the last line on currently page.
    ExMovePageUp,
    /// Display previous page.
    ExMovePageDown,
    /// Display next page.
    ExMoveFileStart,
    /// Move cursor to the beginning of file.
    ExMoveFileEnd,
    /// Move cursor to the end of file.
    ExMovePageLeft,
    /// Scroll horizontally to display page on the left.
    ExMovePageRight,
    /// Scroll horizontally to display page on the right.
    ExMoveBlockStart,
    /// Move cursor to the beginning of block.
    ExMoveBlockEnd,
    /// Move cursor to end beginning of block.
    ExMoveFirstNonWhite,
    /// Move cursor to the first non-blank character on line.
    ExMoveLastNonWhite,
    /// Move cursor to the last non-blank character on line.
    ExMovePrevEqualIndent,
    /// Move cursor to the previous line with equal indentation.
    ExMoveNextEqualIndent,
    /// Move cursor to the next line with equal indentation.
    ExMovePrevTab,
    /// Move cursor to the previous tab position.
    ExMoveNextTab,
    /// Move cursor to the next tab position.
    ExMoveTabStart,
    /// When cursor is on the tab characters, moves it to the beginning
    /// of the tab.
    ExMoveTabEnd,
    /// When cursor is on the tab characters, moves it to the end
    /// of the tab.
    ExMoveLineTop,
    /// Scroll the file to make the current line appear on the top of the window.
    ExMoveLineCenter,
    /// Scroll the file to make the current line appear on the center of the window.
    ExMoveLineBottom,
    /// Scroll the file to make the current line appear on the bottom of the window.
    ExScrollLeft,
    /// Scroll screen left.
    ExScrollRight,
    /// Scroll screen right.
    ExScrollDown,
    /// Scroll screen down.
    ExScrollUp,
    /// Scroll screen up.
    ExMoveFoldTop,
    /// Move to the beginning of current fold.
    ExMoveFoldPrev,
    /// Move to the beginning of previous fold.
    ExMoveFoldNext,
    /// Move to the beginning of next fold.
    ExMoveBeginOrNonWhite,
    /// Move to beginning of line, or to first non blank character
    ExMoveBeginLinePageFile,
    /// Move to the beginning of line. If there already, move to the beginning
    /// page. If there already, move to the beginning of file.
    ExMoveEndLinePageFile,
    /// Move to the end of line. If there already, move to the end
    /// page. If there already, move to the end of file.
    ExMoveToLine,
    /// Move to line number given as argument
    ExMoveToColumn,
    /// Move to column given as argument
    ExMoveSavedPosCol,
    /// Move to column from saved position
    ExMoveSavedPosRow,
    /// Move to line from saved position
    ExMoveSavedPos,
    /// Move to saved position
    ExSavePos,
    /// Save current cursor position
    ExMovePrevPos,
    /// Move to last cursor position
    // ExCursorPush,
    // ExCursorPop,

    
    //<cmd_delete> Deleting Text
    ExKillLine,
    /// Delete current line. If the line is the last line in the file,
    /// only the text is deleted.
    ExKillChar,
    /// Delete character under (after) cursor.
    ExKillCharPrev,
    /// Delete character before cursor.
    ExKillWord,
    /// Delete the word after cursor.
    ExKillWordPrev,
    /// Delete the word before cursor.
    ExKillWordOrCap,
    /// Delete word or capitals after cursor.
    ExKillWordOrCapPrev,
    /// Delete word or capitals before cursor.
    ExKillToLineStart,
    /// Delete characters to the beginning of line.
    ExKillToLineEnd,
    /// Delete characters to the end of line.
    ExKillBlock,
    /// Delete block.
    ExKillBlockOrChar,
    /// If block is marked, delete it, otherwise delete character under cursor.
    ExKillBlockOrCharPrev,
    /// If block is marked, delete it, otherwise delete character before cursor.
    ExDelete,
    /// Delete character under (after) cursor.
    //& <A HREF="modes.html#ms.DeleteKillTab">DeleteKillTab</A>
    //& <A HREF="modes.html#ms.DeleteKillBlock">DeleteKillBlock</A>
    ExBackSpace,
    /// Delete character before cursor.
    //& <A HREF="modes.html#ms.BackSpKillTab">BackSpKillTab</A>
    //& <A HREF="modes.html#ms.BackSpKillBlock">BackSpKillBlock</A>

    //<cmd_line> Line Commands
    ExLineInsert,
    /// Insert a new line before the current one.
    ExLineAdd,
    /// Add a new line after the current one.
    ExLineSplit,
    /// Split current line after cursor position
    ExLineJoin,
    /// Join current line with next one. If cursor is positioned beyond
    /// the end of line, the current line is first padded with whitespace.
    ExLineNew,
    /// Append a new line and move to the beginning of new line.
    ExLineIndent,
    /// Reindent current line.
    ExLineTrim,
    /// Trim whitespace at the end of current line.
    ExLineDuplicate,
    /// Duplicate the current line.
    ExLineCenter,
    /// Center the current line

    //<cmd_block> Block Commands
    ExBlockBegin,
    /// Set block beginning to current position.
    ExBlockEnd,
    /// Set block end to current position.
    ExBlockUnmark,
    /// Unmark block.
    ExBlockCut,
    /// Cut selected block to clipboard.
    ExBlockCopy,
    /// Copy selected block to clipboard.
    ExBlockCutAppend,
    /// Cut selected block and append it to clipboard.
    ExBlockCopyAppend,
    /// Append selected block to clipboard.
    ExBlockClear,
    /// Clear selected block
    ExBlockPaste,
    /// Paste clipboard to current position.
    ExBlockKill,
    /// Delete selected text.
    ExBlockIndent,
    /// Indent block by 1 character.
    ExBlockUnindent,
    /// Unindent block by 1 character.
    ExBlockMarkStream,
    /// Start/stop marking stream block.
    ExBlockMarkLine,
    /// Start/stop marking line block.
    ExBlockMarkColumn,
    /// Start/stop marking column block.
    ExBlockExtendBegin,
    /// Start extending selected block.
    ExBlockExtendEnd,
    /// Stop extending selected block.
    ExBlockReIndent,
    /// Reindent entire block (C/REXX mode)
    ExBlockSelectWord,
    /// Select word under cursor as block.
    ExBlockSelectLine,
    /// Select current line as block.
    ExBlockSelectPara,
    /// Select current paragraph (delimited by blank lines) as block.
    ExBlockPasteStream,
    /// Paste clipboard to current position as stream block.
    ExBlockPasteLine,
    /// Paste clipboard to current position as line block.
    ExBlockPasteColumn,
    /// Paste clipboard to current position as column block.
    ExBlockPrint,
    /// Print a block to configured device.
    ExBlockRead,
    /// Read block from file.
    ExBlockReadStream,
    /// Read block from file as stream block
    ExBlockReadLine,
    /// Read block from file as line block
    ExBlockReadColumn,
    /// Read block from file as column block
    ExBlockWrite,
    /// Write marked block to file.
    ExBlockSort,
    /// Sorts the marked block in ascending order.
    ///
    //\ If mode setting MatchCase is set, characters will be compared case
    //\ sensitively.
    ///
    //\ When block is marked in <A HREF="modes.html#ec.BlockMarkStream">
    //\ Stream</A> or <A HREF="#ec.BlockMarkLine">Line</A> mode,
    //\ the entire lines in marked block will be compared.
    ///
    //\ When block is marked in <A HREF="#ec.BlockMarkColumn">Column</A>
    //\ mode, only characters within marked columns will be compared.
    ExBlockSortReverse,
    /// Sorts the marked block in descending order.
    //^ <A HREF="#ec.BlockSort">BlockSort</A>
    ExBlockUnTab,
    /// Remove tabs from marked lines.
    ExBlockEnTab,
    /// Generate and optimize tabs in marked lines.
    ExBlockMarkFunction,
    /// Mark current function as block.
    ExBlockTrim,
    /// Trim end-of-line whitespace

    //<cmd_edit> Text Editing and Insertion
    ExUndo,
    /// Undo last operation
    ExRedo,
    /// Redo last undone operation.

    //<cmd_fold> Folding Text
    ExFoldCreate,
    /// Create fold
    ExFoldCreateByRegexp,
    /// Create folds at lines matching a regular expression
    ExFoldCreateAtRoutines,
    /// Create folds at lines matching RoutineRx
    ExFoldDestroy,
    /// Destroy fold at current line
    ExFoldDestroyAll,
    /// Destroy all folds in the file
    ExFoldPromote,
    /// Promote fold to outer level
    ExFoldDemote,
    /// Demote fold to inner level
    ExFoldOpen,
    /// Open fold at current line
    ExFoldOpenNested,
    /// Open fold and nested folds
    ExFoldClose,
    /// Close current fold
    ExFoldOpenAll,
    /// Open all folds in the file
    ExFoldCloseAll,
    /// Close all folds in the file
    ExFoldToggleOpenClose,
    /// Toggle open/close current fold.

    //<cmd_bookmark>Bookmarks
    ExPlaceBookmark,
    /// Place a file-local bookmark.
    ExRemoveBookmark,
    /// Place a file-local bookmark.
    ExGotoBookmark,
    /// Go to file-local bookmark location.
    ExPlaceGlobalBookmark,
    /// Place global (persistent) bookmark.
    ExRemoveGlobalBookmark,
    /// Remove global bookmark.
    ExGotoGlobalBookmark,
    /// Go to global bookmark location.
    ExPushGlobalBookmark,
    /// Push global bookmark (named as #<num>) to stack.
    ExPopGlobalBookmark,
    /// Pop global bookmark from stack.

    //<cmd_trans> Character Translation
    ExCharCaseUp,
    /// Convert current character to uppercase
    ExCharCaseDown,
    /// Convert current character to lowercase
    ExCharCaseToggle,
    /// Toggle case of current character
    ExCharTrans,
    /// Translate current character (like perl/sed)
    ExLineCaseUp,
    /// Convert current line to uppercase
    ExLineCaseDown,
    /// Convert current line to lowercase
    ExLineCaseToggle,
    /// Toggle case of current line
    ExLineTrans,
    /// Translate characters on current line
    ExBlockCaseUp,
    /// Convert characters in selected block to uppercase
    ExBlockCaseDown,
    /// Convert characters in selected block to lowercase
    ExBlockCaseToggle,
    /// Toggle case of characters in selected block
    ExBlockTrans,
    /// Translate characters in selected block.
    
    ExInsertString,
    /// Insert argument string at cursor position
    ExInsertSpace,
    /// Insert space
    ExInsertChar,
    /// Insert character argument at cursor position
    ExTypeChar,
    /// Insert character at cursor position (expanding abbreviations)
    ExInsertTab,
    /// Insert tab character at cursor position
    ExInsertSpacesToTab,
    /// Insert appropriate number of spaces to simulate a tab.
    ExSelfInsert,
    /// Insert typed character
    ExWrapPara,
    /// Wrap current paragraph
    ExInsPrevLineChar,
    /// Insert character in previous line above cursor
    ExInsPrevLineToEol,
    /// Insert previous line from cursor to end of line
    ExCompleteWord,
    /// Complete current word to last word starting with the
    /// same prefix.
    
    ExFilePrev,
    /// Switch to previous file in ring.
    ExFileNext,
    /// Switch to next file in ring.
    ExFileLast,
    /// Exchange last two files in ring.
    ExSwitchTo,
    /// Switch to numbered buffer given as argument

    //<cmd_file> File Commands
    ExFileOpen,
    /// Open file
    ExFileOpenInMode,
    /// Open file in specified mode
    ExFileReload,
    /// Reload current file
    ExFileSave,
    /// Save current file
    ExFileSaveAll,
    /// Save all modified files
    ExFileSaveAs,
    /// Rename Save current file
    ExFileWriteTo,
    /// Write current file into another file
    ExFilePrint,
    /// Print current file
    ExFileClose,
    /// Close current file
    ExFileCloseAll,
    /// Close all open files
    ExFileTrim,
    /// Trim end-of-line whitespace

    //<cmd_directory> Directory Commands
    ExDirOpen,
    /// Open directory browser
    ExDirGoUp,
    /// Change to parent directory
    ExDirGoDown,
    /// Change to currently selected directory
    ExDirGoRoot,
    /// Change to root directory
    ExDirGoto,
    /// Change to directory given as argument
    ExDirSearchCancel,
    /// Cancel search
    ExDirSearchNext,
    /// Find next matching file
    ExDirSearchPrev,
    /// Find previous matching file

    //<cmd_search> Search and Replace
    ExIncrementalSearch,
    /// Incremental search
    ExFind,
    /// Find
    ExFindReplace,
    /// Find and replace
    ExFindRepeat,
    /// Repeat last find/replace operation
    ExFindRepeatOnce,
    /// Repeat last find/replace operation only once
    ExFindRepeatReverse,
    /// Repeat last find/replace operation in reverse
    ExMatchBracket,
    /// Find matching bracket ([{<>}])
    ExHilitWord,
    /// Highlight current word everywhere in the file
    ExSearchWordPrev,
    /// Search for previous occurence of word under cursor
    ExSearchWordNext,
    /// Search for next occurence of word under cursor
    ExHilitMatchBracket,
    /// Highlight matching bracket
    ExSearch,
    ExSearchB,
    ExSearchRx,
    ExSearchAgain,
    ExSearchAgainB,
    ExSearchReplace,
    ExSearchReplaceB,
    ExSearchReplaceRx,

    //<cmd_window> Window Commands
    ExWinHSplit,
    /// Split window horizontally
    ExWinNext,
    /// Switch to next (bottom) window
    ExWinPrev,
    /// Switcn to previous (top) window.
    ExWinClose,
    /// Close current window
    ExWinZoom,
    /// Delete all windows except for current one
    ExWinResize,
    /// Resize current window (+n,-n given as argument)
    ExViewBuffers,
    /// View currently open buffers
    ExListRoutines,
    /// Display routines in current source file
    ExExitEditor,
    /// Exit FTE.
    ExShowEntryScreen,
    /// View external program output if available

    //<cmd_compile> Compiler Support
    ExCompile,
    /// Ask for compile command and run compiler
    ExRunCompiler,
    /// Run configured compile command
    ExViewMessages,
    /// View compiler output
    ExCompileNextError,
    /// Switch to next compiler error
    ExCompilePrevError,
    /// Switch to previous compiler error
    ExRunProgram,
    /// Run external program

    //<cmd_cvs> CVS Support
    ExCvs,
    /// Ask for CVS options and run CVS
    ExRunCvs,
    /// Run configured CVS command
    ExViewCvs,
    /// View CVS output
    ExClearCvsMessages,
    /// Clear CVS messages
    ExCvsDiff,
    /// Ask for CVS diff options and run CVS
    ExRunCvsDiff,
    /// Run configured CVS diff command
    ExViewCvsDiff,
    /// View CVS diff output
    ExCvsCommit,
    /// Ask for CVS commit options and run CVS
    ExRunCvsCommit,
    /// Run configured CVS commit command
    ExViewCvsLog,
    /// View CVS log

    //<cmd_svn> SVN Support
    ExSvn,
    /// Ask for SVN options and run SVN
    ExRunSvn,
    /// Run configured SVN command
    ExViewSvn,
    /// View SVN output
    ExClearSvnMessages,
    /// Clear SVN messages
    ExSvnDiff,
    /// Ask for SVN diff options and run SVN
    ExRunSvnDiff,
    /// Run configured SVN diff command
    ExViewSvnDiff,
    /// View SVN diff output
    ExSvnCommit,
    /// Ask for SVN commit options and run SVN
    ExRunSvnCommit,
    /// Run configured SVN commit command
    ExViewSvnLog,
    /// View SVN log

    //<cmd_tags> TAGS Commands
    /// fte supports TAGS files generated by programs like ctags.
    ExTagFind,
    /// Find word argumen in tag files.
    ExTagFindWord,
    /// Find word under cursor in tag files.
    ExTagNext,
    /// Switch to next occurance of tag
    ExTagPrev,
    /// Switch to previous occurance of tag
    ExTagPop,
    /// Pop saved position from tag stack
    ExTagLoad,
    /// Load tag file and merge with current tags
    ExTagClear,
    /// Clear loaded tags
    ExTagGoto,
    ///

    //<cmd_option> Option commands
    ExToggleAutoIndent,
    ///
    ExToggleInsert,
    ///
    ExToggleExpandTabs,
    ///
    ExToggleShowTabs,
    ///
    ExToggleUndo,
    ///
    ExToggleReadOnly,
    ///
    ExToggleKeepBackups,
    ///
    ExToggleMatchCase,
    ///
    ExToggleBackSpKillTab,
    ///
    ExToggleDeleteKillTab,
    ///
    ExToggleSpaceTabs,
    ///
    ExToggleIndentWithTabs,
    ///
    ExToggleBackSpUnindents,
    ///
    ExToggleWordWrap,
    ///
    ExToggleTrim,
    ///
    ExToggleShowMarkers,
    ///
    ExToggleHilitTags,
    ///
    ExToggleShowBookmarks,
    ///
    ExToggleMakeBackups,
    ///
    ExSetLeftMargin,
    ///
    ExSetRightMargin,
    ///
    ExToggleSysClipboard,
    ///
    ExSetPrintDevice,
    ///
    ExChangeTabSize,
    ///
    ExChangeLeftMargin,
    ///
    ExChangeRightMargin,
    ///


    //<cmd_other> Other commands
    ExShowPosition,
    /// Show internal position information on status line
    ExShowVersion,
    /// Show editor version information
    ExShowKey,
    /// Wait for keypress and display modifiers+key pressed
    ExWinRefresh,
    /// Refresh display

    ExMainMenu,
    /// Activate main menu
    ExShowMenu,
    /// Popup menu specified as argument
    ExLocalMenu,
    /// Popup context menu

    ExChangeMode,
    /// Change active mode for current buffer
    ExChangeKeys,
    /// Change keybindings for current buffer
    ExChangeFlags,
    /// Change option flags for current buffer

    ExCancel,
    ///
    ExActivate,
    ///
    ExRescan,
    ///
    ExCloseActivate,
    ///
    ExActivateInOtherWindow,
    ///
    ExDeleteFile,
    ///

    ExASCIITable,
    /// Display ASCII selector in status line.
    ExDesktopSave,
    /// Save desktop
    ExClipClear,
    /// Clear clipboard
    ExDesktopSaveAs,
    /// Save desktop under a new name
    ExChildClose,
    ///

    ExBufListFileSave,
    /// Save currently selected file in buffer list
    ExBufListFileClose,
    /// Close currently selected file in buffer list
    ExBufListSearchCancel,
    /// Cancel search
    ExBufListSearchNext,
    /// Next match in search
    ExBufListSearchPrev,
    /// Previous match in search

    ExViewModeMap,
    /// View current mode keybindings
    ExClearMessages,
    /// Clear compiler messages


    ExIndentFunction,
    /// Indent current function
    ExMoveFunctionPrev,
    /// Move cursor to previous function
    ExMoveFunctionNext,
    /// Move cursor to next function
    ExInsertDate,
    /// Insert date at cursor
    ExInsertUid,
    /// Insert user name at cursor

    ExFrameNew,
    ///
    ExFrameClose,
    ///
    ExFrameNext,
    ///
    ExFramePrev,
    ///

    ExBufferViewNext,
    ///
    ExBufferViewPrev,
    ///

    ExShowHelpWord,
    /// Show context help on keyword.
    ExShowHelp,
    /// Show help for FTE.
    ExConfigRecompile,
    /// Recompile editor configuration

    ExSetCIndentStyle,
    /// Set C indentation style parameters
    /// Has the following parameters:
    ///
    /// C_Indent = 4;
    /// C_BraceOfs = 0;
    /// C_ParenDelta = -1;
    /// C_CaseOfs = 0;
    /// C_CaseDelta = 4;
    /// C_ClassOfs = 0;
    /// C_ClassDelta = 4;
    /// C_ColonOfs = -4;
    /// C_CommentOfs = 0;
    /// C_CommentDelta = 1;
    /// C_FirstLevelWidth = -1;
    /// C_FirstLevelIndent = 4;
    /// C_Continuation = 4;
    ExSetIndentWithTabs,
    /// Set value of indent-with-tabs to argument
    ExRunProgramAsync,

    ExListMark,
    /// Mark single line in list
    ExListUnmark,
    /// Unmark single line in list
    ExListToggleMark,
    /// Toggle marking of single line in list
    ExListMarkAll,
    /// Mark all lines in list
    ExListUnmarkAll,
    /// Unmark all lines in list
    ExListToggleMarkAll,
    /// Toggle marking of all lines in list

    ExBlockPasteOver
    /// Delete content's of selection and paste clipboard to current position
};

#endif // C_COMMANDS_H
