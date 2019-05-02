/*
 * New editor name:  TDE, the Thomson-Davis Editor.
 * Author:           Frank Davis and Jason Hood
 * Date:             August 9, 1998
 *
 * This file contains the pull-down menu and strings for the keys.
 */

#include "tdestr.h"
#include "define.h"

/*
 * simple pull-down menu stuff
 *
 * pull.c auto calculates the number of selections under each menu choice.
 *  end the selections with a NULL string and ERROR in the function list.
 *  pull.c also calculates the width of the pull menu.
 *
 * jmh - added some extra functions and spacing
 * jmh 980524: removed the borders, added the #defines.
 * jmh 980809: use sizeof to calculate the number of selections,
 *              removing the need for the last NULL.
 * jmh 991021: added an array of item names.
 * jmh 991025: added another menu - "Move";
 *             modified the method of creating key strings.
 * jmh 991104: added the two HorizontalScreen functions to "Move" (and
 *              removed them from the help screen).
 * jmh 010624: added pop-out menus, re-organised menus as a result.
 * jmh 031129: added Item & SubM macros;
 *             added User menu.
 * jmh 050710: added Help menu.
 * jmh 050722: to assist customisation start the name/label with a NUL to
 *              indicate static memory, rather than allocated; likewise
 *              the first line has ERROR-1 instead of ERROR.
 */

#define Begin( name )      static MINOR_STR name[] = { \
                              { NULL, ERROR-1, NULL, FALSE, NULL },
#define Item( name, func ) { "\000" name, func | _FUNCTION, NULL, FALSE, NULL },
#define SubM( name, num )  { "\000" name, 0,&popout_default[num].popout, FALSE, NULL },
#define Separator          { NULL,  ERROR, NULL, FALSE, NULL },
#define SepLabel( label )  { "\000" label, ERROR, NULL, FALSE, NULL },
#define End                Separator };


Begin( file_special )
   Item( "Save all",            SaveAll             )
   Item( "Save to",             SaveTo              )
   Item( "Save untouched",      SaveUntouched       )
   Item( "Revert",              Revert              )
   Item( "Insert File",         InsertFile          )
   Item( "Write Block",         BlockToFile         )
   Item( "Print",               PrintBlock          )
End

Begin( block_conversion )
   Item( "Upper Case",          BlockUpperCase      )
   Item( "Lower Case",          BlockLowerCase      )
   Item( "Invert Case",         BlockInvertCase     )
   Item( "Capitalise",          BlockCapitalise     )
   Separator
   Item( "Strip hi bit",        BlockStripHiBit     )
   Item( "Rot13",               BlockRot13          )
   Item( "Fix UUE prob",        BlockFixUUE         )
End

Begin( block_comment )
   Item( "Block",               BlockBlockComment   )
   Item( "Line",                BlockLineComment    )
   Item( "Remove",              BlockUnComment      )
End

#if defined( __DJGPP__ ) || defined( __WIN32__ )
Begin( block_clipboard )
   Item( "Copy",                CopyToClipboard     )
   Item( "Kopy",                KopyToClipboard     )
   Item( "Cut",                 CutToClipboard      )
   Item( "Paste",               PasteFromClipboard  )
End
#endif

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


#define Popout( menu ) \
   { menu, sizeof(menu) / sizeof(*menu), 0, 0, 0, 0, 0 }

#define PopoutList( idx, menu ) { &popout_default[idx+1], Popout( menu ) },
#define PopoutLEnd( idx, menu ) { NULL, Popout( menu ) }

MENU_LIST popout_default[] = {
   PopoutList( 0, block_conversion )
   PopoutList( 1, word_tabs        )
   PopoutList( 2, word_indent      )
   PopoutList( 3, macro_file       )
   PopoutList( 4, block_comment    )
   PopoutList( 5, block_border     )
   PopoutList( 6, block_fill       )
#if defined( __DJGPP__ ) || defined( __WIN32__ )
   PopoutList( 7, file_special     )
   PopoutLEnd( 8, block_clipboard  )
#else
   PopoutLEnd( 7, file_special     )
#endif
};
MENU_LIST *popout_last = popout_default + sizeof(popout_default)
                                        / sizeof(*popout_default) - 1;

MENU_LIST *popout_menu = popout_default;


Begin( file_opts )
   Item( "Directory",           DirList             )
   Item( "Load",                EditFile            )
   Item( "Load Next",           EditNextFile        )
   Item( "File Find",           DefineGrep          )
   Item( "Next File",           RepeatGrep          )
   Item( "Scratch",             ScratchWindow       )
   Item( "Save",                Save                )
   Item( "Save as",             SaveAs              )
   Item( "Save & Close",        File                )
   Item( "Close",               Quit                )
   Item( "File attr",           FileAttributes      )
   SubM( "Special",             7                   )
   Separator
   Item( "Select Language",     SyntaxSelect        )
   Item( "Read Config",         ReadConfig          )
   Item( "Status",              Status              )
   Item( "Statistics",          Statistics          )
   Item( "Set directory",       SetDirectory        )
   Separator
   Item( "Save Workspace",      SaveWorkspace       )
   Item( "Exit",                QuitAll             )
   Item( "Save and Exit",       FileAll             )
End

Begin( search_opts )
   Item( "Search",              DefineSearch        )
   Item( "Repeat",              RepeatSearch        )
   Item( "Replace",             ReplaceString       )
   Separator
   Item( "Find forward",        FindForward         )
   Item( "Find backward",       FindBackward        )
   Item( "Repeat >",            RepeatFindForward   )
   Item( "Repeat <",            RepeatFindBackward  )
   Item( "ISearch >",           ISearchForward      )
   Item( "ISearch <",           ISearchBackward     )
   Separator
   Item( "Regx forward",        RegXForward         )
   Item( "Regx backward",       RegXBackward        )
   Item( "Repeat >",            RepeatRegXForward   )
   Item( "Repeat <",            RepeatRegXBackward  )
   Separator
   Item( "Toggle Case",         ToggleSearchCase    )
   Separator
   Item( "Begin Diff",          DefineDiff          )
   Item( "Next Diff",           RepeatDiff          )
End

Begin( block_opts )
   Item( "Copy",                CopyBlock           )
   Item( "Kopy",                KopyBlock           )
   Item( "Delete",              DeleteBlock         )
   Item( "Move",                MoveBlock           )
   Item( "Overlay",             OverlayBlock        )
   Item( "Swap",                SwapBlock           )
   Item( "Sort",                SortBoxBlock        )
   SubM( "Comment",             4                   )
   SubM( "Conversions",         0                   )
#if defined( __DJGPP__ ) || defined( __WIN32__ )
   SubM( "Clipboard",    sizeof(popout_default) / sizeof(*popout_default) - 1 )
#endif
   Item( "Move Mark",           MoveMark            )
   SepLabel( "Line only" )
   Item( "Expand Tabs",         BlockExpandTabs     )
   Item( "Compress Tab",        BlockCompressTabs   )
   Item( "Indent Tabs",         BlockIndentTabs     )
   Item( "Trim Trail",          BlockTrimTrailing   )
   Item( "E-mail '>'",          BlockEmailReply     )
   SepLabel( "Box only" )
   SubM( "Border",              5                   )
   SubM( "Fill",                6                   )
   Item( "Number",              NumberBlock         )
   Item( "Sum",                 SumBlock            )
End

Begin( toggle_opts )
   Item( "Indent",              ToggleIndent        )
   Item( "Word wrap",           ToggleWordWrap      )
   Item( "Trim trailing",       ToggleTrailing      )
   Separator
   Item( "EOL display",         ToggleEol           )
   Item( "Ruler",               ToggleRuler         )
   Item( "Line numbers",        ToggleLineNumbers   )
   Item( "Cursor cross",        ToggleCursorCross   )
   Separator
   Item( "Cursor direction",    ChangeCurDir        )
   Item( "Graphic chars",       ToggleGraphicChars  )
   Item( "Drawing",             ToggleDraw          )
   Separator
   Item( "Read only",           ToggleReadOnly      )
   Item( "Line ending",         ToggleCRLF          )
   Item( "Control Z",           ToggleZ             )
   Item( "Cur. directory",      ToggleCWD           )
   Separator
   Item( "Syntax highlight",    ToggleSyntax        )
   Item( "Break point",         SetBreakPoint       )
   /*
   Item( "Group undo",          ToggleUndoGroup     )
   Item( "Undo movement",       ToggleUndoMove      )
   */
End

Begin( other_opts )
   Item( "Date Stamp",          DateTimeStamp       )
   Item( "Stamp Format",        StampFormat         )
   Item( "Character set",       CharacterSet        )
   Item( "Popup Ruler",         PopupRuler          )
   SepLabel( "Line" )
   Item( "Add",                 AddLine             )
   Item( "Split",               SplitLine           )
   Item( "Join",                JoinLine            )
   Item( "Duplicate",           DuplicateLine       )
   Item( "Delete",              DeleteLine          )
   Item( "Delete end",          DelEndOfLine        )
   Item( "Delete begin",        DelBegOfLine        )
   Item( "Erase begin",         EraseBegOfLine      )
   Item( "Retrieve",            RetrieveLine        )
   SepLabel( "Macro" )
   Item( "Record/Stop",         RecordMacro         )
   Item( "Pseudo-macro",        PseudoMacro         )
   Item( "Repeat",              Repeat              )
   SubM( "File",                3                   )
   Separator
   Item( "Shell",               Shell               )
   Item( "Execute",             Execute             )
#if 1 //!defined( __UNIX__ )
   Item( "User Screen",         UserScreen          )
#endif
End

Begin( word_opts )
   Item( "Format Paragraph",    FormatParagraph     )
   Item( "Format forward",      FormatText          )
   Item( "Margins",             SetMargins          )
   SepLabel( "Justify" )
   Item( "Left",                LeftJustify         )
   Item( "Right",               RightJustify        )
   Item( "Center",              CenterJustify       )
   SepLabel( "Block Justify" )
   Item( "Left",                BlockLeftJustify    )
   Item( "Right",               BlockRightJustify   )
   Item( "Center",              BlockCenterJustify  )
   Separator
   SubM( "Tabs",                1                   )
   SubM( "Indentation",         2                   )
End

Begin( move_opts )
   Item( "Top of File",         TopOfFile           )
   Item( "End of File",         EndOfFile           )
   Item( "Top line",            TopOfScreen         )
   Item( "Center line",         CenterWindow        )
   Item( "Bottom line",         BotOfScreen         )
   Item( "Line to top",         TopLine             )
   Item( "Line to center",      CenterLine          )
   Item( "Line to bottom",      BottomLine          )
   Item( "Window left",         ScreenLeft          )
   Item( "Window right",        ScreenRight         )
   Item( "Half window up",      HalfScreenUp        )
   Item( "Half window down",    HalfScreenDown      )
   Item( "Half window left",    HalfScreenLeft      )
   Item( "Half window right",   HalfScreenRight     )
   Separator
   Item( "Goto Position",       JumpToPosition      )
   Item( "Next Dirty Line",     NextDirtyLine       )
   Item( "Prev Dirty Line",     PrevDirtyLine       )
   Item( "Next Browse",         NextBrowse          )
   Item( "Prev Browse",         PrevBrowse          )
   Separator
   Item( "Cursor sync",         ToggleSync          )
End

Begin( window_opts )
   Item( "Split horizontal",    SplitHorizontal     )
   Item( "Split vertical",      SplitVertical       )
   Item( "Split half horiz.",   SplitHalfHorizontal )
   Item( "Split half vert.",    SplitHalfVertical   )
   Item( "Make horizontal",     MakeHorizontal      )
   Item( "Make vertical",       MakeVertical        )
   Item( "Make half horiz.",    MakeHalfHorizontal  )
   Item( "Make half vert.",     MakeHalfVertical    )
   Item( "Balance horiz.",      BalanceHorizontal   )
   Item( "Balance vert.",       BalanceVertical     )
   Item( "Size",                SizeWindow          )
   Item( "Zoom",                ZoomWindow          )
   Item( "Close",               CloseWindow         )
   Separator
   Item( "Next",                NextWindow          )
   Item( "Previous",            PreviousWindow      )
   Item( "Next Hidden",         NextHiddenWindow    )
   Item( "Prev Hidden",         PrevHiddenWindow    )
   Item( "Goto",                GotoWindow          )
   Separator
   Item( "Title",               TitleWindow         )
   Item( "Redraw Screen",       RedrawScreen        )
End

Begin( help_opts )
   Item( "Context Help",        ContextHelp         )
   Item( "Help Screen",         Help                )
   Separator
   Item( "About",               About               )
End


/*
 * here's the main headings in the pull-down menu.  in TDESTR.H the number
 *  of headings is defined as MAJOR or 10.
 */

#define Menu( name, menu ) { "\000" name, Popout( menu ) },

MENU menu = {
   Menu( "File",      file_opts )
   Menu( "Search",  search_opts )
   Menu( "Block",    block_opts )
   Menu( "Toggles", toggle_opts )
   Menu( "Other",    other_opts )
   Menu( "Word",      word_opts )
   Menu( "Move",      move_opts )
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
