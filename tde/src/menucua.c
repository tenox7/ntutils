/*
 * New editor name:  TDE, the Thomson-Davis Editor.
 * Author:           Eric Pement
 * Date:             5 February, 2005
 *
 * This file was contributed by Eric Pement to provide more of a
 * CUA-style menu system.  To use this instead of the default:
 *
 *      rename menu.c menutde.c
 *      rename menucua.c menu.c
 *
 * and, if you've already compiled:
 *
 *      delete <dir>/menu.o*
 */

#include "tdestr.h"
#include "define.h"


#define Begin( name )      static MINOR_STR name[] = { \
                              { NULL, ERROR-1, NULL, FALSE, NULL },
#define Item( name, func ) { "\000" name, func | _FUNCTION, NULL, FALSE, NULL },
#define SubM( name, num )  { "\000" name, 0,&popout_default[num].popout, FALSE, NULL },
#define Separator          { NULL,  ERROR, NULL, FALSE, NULL },
#define SepLabel( label )  { "\000" label, ERROR, NULL, FALSE, NULL },
#define End                Separator };


Begin( file_special )
   Item( "Save all",            SaveAll             )
   Item( "Save to...",          SaveTo              )
   Item( "Save untouched",      SaveUntouched       )
   Item( "Save Workspace",      SaveWorkspace       )
   Item( "Revert",              Revert              )
   Item( "Print",               PrintBlock          )
End

Begin( block_conversion )
   Item( "Align left",          BlockLeftJustify    )
   Item( "Align right",         BlockRightJustify   )
   Item( "Align center",        BlockCenterJustify  )
   Item( "Strip hi bit",        BlockStripHiBit     )
   Item( "Rot13",               BlockRot13          )
   Item( "Fix UUE prob",        BlockFixUUE         )
End

Begin( block_comment )
   Item( "Block",               BlockBlockComment   )
   Item( "Line",                BlockLineComment    )
   Item( "Remove",              BlockUnComment      )
End

Begin( block_border )
   Item( "Standard",            BorderBlock         )
   Item( "Enhanced",            BorderBlockEx       )
End

Begin( block_fill )
   Item( "Character",           FillBlock           )
   Item( "Down",                FillBlockDown       )
   Item( "Pattern",             FillBlockPattern    )
End

Begin( word_tabs )
   Item( "Set tabs",            SetTabs             )
   Item( "Smart tabs",          ToggleSmartTabs     )
   Item( "Inflate tabs",        ToggleTabInflate    )
   Item( "Dynamic size",        DynamicTabSize      )
End

Begin( word_indent )
   Item( "Indent spaces",       BlockIndentN        )
   Item( "Undent spaces",       BlockUndentN        )
   Item( "Indent",              BlockIndent         )
   Item( "Undent",              BlockUndent         )
End

Begin( macro_file )
   Item( "Load",                LoadMacro           )
   Item( "Save",                SaveMacro           )
   Item( "Clear",               ClearAllMacros      )
End

Begin( browse_results )
   Item( "Next Browse",         NextBrowse          )
   Item( "Prev Browse",         PrevBrowse          )
End

Begin( change_case )
   Item( "To upper",            BlockUpperCase      )
   Item( "To lower",            BlockLowerCase      )
   Item( "Invert case",         BlockInvertCase     )
   Item( "Title case",          BlockCapitalise     )
End


#define Popout( menu ) \
   { menu, sizeof(menu) / sizeof(*menu), 0, 0, 0, 0, 0 }

#define PopoutList( idx, menu ) { &popout_default[idx+1], Popout( menu ) },
#define PopoutLEnd( idx, menu ) { NULL, Popout( menu ) }

MENU_LIST popout_default[] = {
   PopoutList( 0, block_border     )
   PopoutList( 1, block_comment    )
   PopoutList( 2, block_conversion )
   PopoutList( 3, block_fill       )
   PopoutList( 4, browse_results   )
   PopoutList( 5, change_case      )
   PopoutList( 6, file_special     )
   PopoutList( 7, macro_file       )
   PopoutList( 8, word_indent      )
   PopoutLEnd( 9, word_tabs        )
};
MENU_LIST *popout_last = popout_default + sizeof(popout_default)
                                        / sizeof(*popout_default) - 1;

MENU_LIST *popout_menu = popout_default;


Begin( file_opts )
   Item( "New",                 ScratchWindow       )
   Item( "Open",                DirList             )
   Item( "Load Next",           EditNextFile        )
   Item( "File Find",           DefineGrep          )
   Item( "Insert File",         InsertFile          )
   Separator
   Item( "Save",                Save                )
   Item( "Save As...",          SaveAs              )
   Item( "Save & Close",        File                )
   SubM( "Special",             6                   )
   SepLabel( "Toggle" )
   Item( "Read only",           ToggleReadOnly      )
   Item( "Unix/DOS EOL",        ToggleCRLF          )
   Item( "Ctrl-Z at EOF",       ToggleZ             )
   Separator
   Item( "Status",              Status              )
   Item( "File attr",           FileAttributes      )
   Item( "Statistics",          Statistics          )
   Separator
   Item( "Close",               Quit                )
   Item( "Close all",           QuitAll             )
   Item( "Save all,Exit",       FileAll             )
End

Begin( edit_opts )
   Item( "Undo/redo",           RestoreLine         )
   Item( "Retrieve",            RetrieveLine        )
#if defined( __DJGPP__ ) || defined( __WIN32__ )
   Separator
   Item( "Copy",                CopyToClipboard     )
   Item( "Copy,keep marked",    KopyToClipboard     )
   Item( "Cut",                 CutToClipboard      )
   Item( "Paste",               PasteFromClipboard  )
#endif
   Separator
   Item( "Date/Time Stamp",     DateTimeStamp       )
   SepLabel( "Line" )
   Item( "Add blank",           AddLine             )
   Item( "Split",               SplitLine           )
   Item( "Join",                JoinLine            )
   Item( "Duplicate",           DuplicateLine       )
   Item( "Delete",              DeleteLine          )
   Item( "Del to EOL",          DelEndOfLine        )
   Item( "Del to BOL",          DelBegOfLine        )
   Item( "Wipe to BOL",         EraseBegOfLine      )
   SepLabel( "Toggle" )
   Item( "Cursor direction",    ChangeCurDir        )
   Item( "Keypad graphics",     ToggleGraphicChars  )
   Item( "Drawing mode",        ToggleDraw          )
   /*
   Item( "Group undo",          ToggleUndoGroup     )
   Item( "Undo movement",       ToggleUndoMove      )
   */
End

Begin( search_opts )
   Item( "Find fwd",            FindForward         )
   Item( "Find back",           FindBackward        )
   Item( "Find next",           RepeatFindForward   )
   Item( "Find prev",           RepeatFindBackward  )
   Item( "Increm fwd",          ISearchForward      )
   Item( "Increm back",         ISearchBackward     )
   Item( "Toggle case",         ToggleSearchCase    )
   /* Separator */ /* jmh 050721: makes it 26 lines */
   Item( "Regex fwd",           RegXForward         )
   Item( "Regex back",          RegXBackward        )
   Item( "Regex next",          RepeatRegXForward   )
   Item( "Regex prev",          RepeatRegXBackward  )
   Separator
   Item( "Advanced find",       DefineSearch        )
   Item( "Advanced next",       RepeatSearch        )
   SubM( "Browse results",      4                   )
   Separator
   Item( "Replace",             ReplaceString       )
   Separator
   Item( "Goto Line:Pos",       JumpToPosition      )
   Item( "Top of screen",       TopOfScreen         )
   Item( "Ctr of screen",       CenterWindow        )
   Item( "Bot of screen",       BotOfScreen         )
   Item( "Next dirty line",     NextDirtyLine       )
   Item( "Prev dirty line",     PrevDirtyLine       )
End

Begin( view_opts )
   Item( "Show win title",      TitleWindow         )
   Item( "Redraw screen",       RedrawScreen        )
   SepLabel( "Pan Motion" )
   Item( "Window left",         ScreenLeft          )
   Item( "Window right",        ScreenRight         )
   Item( "Window half lf",      HalfScreenLeft      )
   Item( "Window half rt",      HalfScreenRight     )
   Separator
   Item( "Half window up",      HalfScreenUp        )
   Item( "Half window dn",      HalfScreenDown      )
   SepLabel( "Toggle to see:" )
   Item( "Path to file",        ToggleCWD           )
   Item( "Ruler",               ToggleRuler         )
   Item( "TABs, EOL",           ToggleEol           )
   Item( "Line numbers",        ToggleLineNumbers   )
   Item( "Cursor cross",        ToggleCursorCross   )
   Item( "Highlight off",       ToggleSyntax        )
End

Begin( block_opts )
   Item( "Copy",                CopyBlock           )
   Item( "Copy (w/marks)",      KopyBlock           )
   Item( "Delete",              DeleteBlock         )
   Item( "Move",                MoveBlock           )
   Item( "Overlay",             OverlayBlock        )
   Item( "Swap",                SwapBlock           )
   Item( "Sort",                SortBoxBlock        )
   Item( "Write Block",         BlockToFile         )
   SubM( "Comment",             1                   )
   SubM( "Conversions",         2                   )
   Item( "Move mark",           MoveMark            )
   SepLabel( "Line only" )
   Item( "Expand Tabs",         BlockExpandTabs     )
   Item( "Compress Tab",        BlockCompressTabs   )
   Item( "Indent Tabs",         BlockIndentTabs     )
   Item( "Trim Trail",          BlockTrimTrailing   )
   Item( "E-mail '>'",          BlockEmailReply     )
   SepLabel( "Box only" )
   SubM( "Border",              0                   )
   SubM( "Fill",                3                   )
   Item( "Number",              NumberBlock         )
   Item( "Sum",                 SumBlock            )
End

Begin( text_opts )
   Item( "Format paragraph",    FormatParagraph     )
   Item( "Format forward",      FormatText          )
   Item( "Set margins",         SetMargins          )
   SepLabel( "Adjust line" )
   Item( "Flush left",          LeftJustify         )
   Item( "Flush right",         RightJustify        )
   Item( "Center",              CenterJustify       )
   Separator
   SubM( "Tabs",                9                   )
   SubM( "Change case",         5                   )
   SubM( "Indentation",         8                   )
   SepLabel( "Toggles " )
   Item( "Indent",              ToggleIndent        )
   Item( "Word wrap",           ToggleWordWrap      )
   Item( "Trim trailing",       ToggleTrailing      )
   SepLabel( "Move line" )
   Item( "To top",              TopLine             )
   Item( "To center",           CenterLine          )
   Item( "To bottom",           BottomLine          )
End

Begin( tools_opts )
   Item( "ASCII table",         CharacterSet        )
   Item( "Popup ruler",         PopupRuler          )
   Item( "Shell",               Shell               )
#if 1 //!defined( __UNIX__ )
   Item( "View console",        UserScreen          )
#endif
   Item( "Execute",             Execute             )
   SepLabel( "Macro" )
   Item( "Record/Stop",         RecordMacro         )
   Item( "Pseudo-macro",        PseudoMacro         )
   Item( "Repeat",              Repeat              )
   SubM( "File",                7                   )
   SepLabel( "Configure" )
   Item( "Date Format",         StampFormat         )
   Item( "Select Language",     SyntaxSelect        )
   Item( "Read Config",         ReadConfig          )
   Separator
   Item( "Set Breakpoint",      SetBreakPoint       )
End

Begin( window_opts )
   Item( "Split horizontal",    SplitHorizontal     )
   Item( "Split half horiz.",   SplitHalfHorizontal )
   Item( "Split vertical",      SplitVertical       )
   Item( "Split half vert.",    SplitHalfVertical   )
   Item( "Add horizontal",      MakeHorizontal      )
   Item( "Add half horiz.",     MakeHalfHorizontal  )
   Item( "Add vertical",        MakeVertical        )
   Item( "Add half vert.",      MakeHalfVertical    )
   Separator
   Item( "Balance horiz.",      BalanceHorizontal   )
   Item( "Balance vert.",       BalanceVertical     )
   Item( "Resize",              SizeWindow          )
   Item( "Zoom",                ZoomWindow          )
   Item( "Close",               CloseWindow         )
   Separator
   Item( "Next",                NextWindow          )
   Item( "Previous",            PreviousWindow      )
   Item( "Next Hidden",         NextHiddenWindow    )
   Item( "Prev Hidden",         PrevHiddenWindow    )
   Item( "Goto",                GotoWindow          )
   Separator
   Item( "Cursor sync",         ToggleSync          )
   Item( "Begin Diff",          DefineDiff          )
   Item( "Next Diff",           RepeatDiff          )
End

Begin( help_opts )
   Item( "Context Help",        ContextHelp         )
   Item( "Help Screen",         Help                )
   Separator
   Item( "About",               About               )
End

/* these are 3 lines that Eric didn't incorporate into the CUA menu

   Item( "Set directory",   SetDirectory      )   # have no use for this
   Item( "Top of File",       TopOfFile       )   # too common
   Item( "End of File",       EndOfFile       )   # too common

*/


/*
 * here's the main headings in the pull-down menu.  in TDESTR.H the number
 *  of headings is defined as MAJOR or 10.
 */

#define Menu( name, menu ) { "\000" name, Popout( menu ) },

MENU menu = {
   Menu( "File",      file_opts )
   Menu( "Edit",      edit_opts )
   Menu( "Search",  search_opts )
   Menu( "View",      view_opts )
   Menu( "Block",    block_opts )
   Menu( "Text",      text_opts )
   Menu( "Tools",    tools_opts )
   Menu( "Window",  window_opts )
     { "\0User",    { NULL, 0, 0, 0, 0, 0 } },
   Menu( "Help",      help_opts )
};


int  menu_cnt = MAJOR;
int  user_idx = MAJOR - 2;


/*
 * popup menu for selecting the new window in the make window functions.
 */
Begin( make_window )
   SepLabel( " Make window with..." )
   Separator
   Item( "Next file",           EditNextFile        )
   Item( "Next hidden window",  NextHiddenWindow    )
   Item( "Previous hidden",     PrevHiddenWindow    )
   Item( "Select window",       GotoWindow          )
   Item( "Load file",           EditFile            )
   Item( "Select file",         DirList             )
   Item( "Scratch window",      ScratchWindow       )
   Item( "Grep",                DefineGrep          )
   Item( "Next grep",           RepeatGrep          )
End

MENU_STR make_window_menu = Popout( make_window );


char *key_word[MAX_KEYS] = {
   "Esc",
   "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "Backspace",
   "Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "Enter",
   "Ctrl+", "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "'", "`",
   "Shift+", "\\", "Z", "X", "C", "V", "B", "N", "M", ",", ".", "/",
   "", /* RightShift remapped as Control+Break */
   "Grey*", "Alt+", "Space", "", /* CapsLock */
   "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
   "GreyEnter", "Grey/", /* remapped NumLock & ScrollLock */
   "Home", "Up", "PgUp", "Grey-", "Left", "Center", "Right", "Grey+",
   "End", "Down", "PgDn", "Insert", "Delete",
   "PrtSc", "" /* fake F11 */,  "Left\\", "F11", "F12",
};
