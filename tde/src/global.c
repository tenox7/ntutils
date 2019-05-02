/*******************  start of original comments  ********************/
/*
 * Written by Douglas Thomson (1989/1990)
 *
 * This source code is released into the public domain.
 */
/*********************  end of original comments   ********************/

/*
 * New editor name:  TDE, the Thomson-Davis Editor.
 * Author:           Frank Davis
 * Date:             June 5, 1991
 *
 * This modification of Douglas Thomson's code is released into the
 * public domain, Frank Davis.  You may distribute it freely.
 *
 * See "tdestr.h" for a description of these variables. C requires global
 *  variables to be declared "extern" in all modules except one.  This file
 *  is included in ed.c and it should not be included in any other module.
 *
 * jmh 980525: made it a C file. ed.c now includes "common.h"
 */

#include "tdestr.h"
#include "tdefunc.h"
#include "define.h"


displays g_display;

status_infos g_status;

boyer_moore_type bm;

boyer_moore_type sas_bm;

MACRO *macro[MODIFIERS][MAX_KEYS];
TREE  key_tree = { 0x10000L, NULL, 0, NULL, NULL };

#if defined( __DOS16__ )
CEH ceh;
#endif

SORT sort;

DIFF diff;

REGX_INFO regx;

REGX_INFO sas_regx;

NFA_TYPE nfa;

NFA_TYPE sas_nfa;

char line_out[MAX_COLS+2];

#if defined( __UNIX__ )
chtype tde_color_table[256];
#endif

char ruler_line[MAX_LINE_LENGTH+MAX_COLS+2];
TDE_WIN ruler_win;


/*
 * Scancode mapping. This translates keyboard hardware scancodes to TDE
 * keycodes. Added by jmh 020903.
 */
int  scancode_map[MAX_KEYS] = {
   _ESC, _1, _2, _3, _4, _5, _6, _7, _8, _9, _0, _MINUS, _EQUALS, _BACKSPACE,
   _TAB, _Q, _W, _E, _R, _T, _Y, _U, _I, _O, _P, _LBRACKET, _RBRACKET, _ENTER,
   _CONTROLKEY, _A, _S, _D, _F, _G, _H, _J, _K, _L, _SEMICOLON, _APOSTROPHE,
   _BACKQUOTE, _SHIFTKEY, _BACKSLASH, _Z, _X, _C, _V, _B, _N, _M, _COMMA,
   _PERIOD, _SLASH, _CONTROL_BREAK, _GREY_STAR, _ALTKEY, _SPACEBAR, _CAPSLOCK,
   _F1, _F2, _F3, _F4, _F5, _F6, _F7, _F8, _F9, _F10, _GREY_ENTER, _GREY_SLASH,
   _HOME, _UP, _PGUP, _GREY_MINUS, _LEFT, _CENTER, _RIGHT, _GREY_PLUS, _END,
   _DOWN, _PGDN, _INS, _DEL, _PRTSC, _FAKE_F11, _LEFT_BACKSLASH, _F11, _F12,
};


/*
 * Graphic character sets. Added by jmh 980724.
 * jmh 991019: added ASCII.
 */
const char graphic_char[GRAPHIC_SETS][11] = {
   { '|', '+', '+', '+',
          '+', '+', '+',
          '+', '+', '+', '-' },

   { '³', 'À', 'Á', 'Ù',
          'Ã', 'Å', '´',
          'Ú', 'Â', '¿', 'Ä' },

   { 'º', 'È', 'Ê', '¼',
          'Ì', 'Î', '¹',
          'É', 'Ë', '»', 'Í' },

   { 'º', 'Ó', 'Ð', '½',
          'Ç', '×', '¶',
          'Ö', 'Ò', '·', 'Ä' },

   { '³', 'Ô', 'Ï', '¾',
          'Æ', 'Ø', 'µ',
          'Õ', 'Ñ', '¸', 'Í' },

   { '°', '±', 'Ü', '²', 'Þ', 'Û', 'Ý', '®', 'ß', '¯', 'þ' }
};


option_infos g_option_all = {
   NULL,                /* auto-detect syntax highlighting language */
   FALSE,               /* auto-detect read-only status */
   0,                   /* file is not scratch */
   -1,                  /* use global tab size */
   TEXT,                /* auto-detect file type */
   DEFAULT_BIN_LENGTH,  /* line length if binary */
   0,                   /* no startup macro */
   FALSE                /* load each file individually */
};
option_infos g_option;


mode_infos mode = {
   0,                   /* initial color scheme */
   FALSE,               /* sync mode? */
   TRUE,                /* sync semaphore must be TRUE - DO NOT CHANGE */
   FALSE,               /* macro recording mode must be FALSE - DO NOT CHANGE */
   TRUE,                /* insert mode */
   TRUE,                /* indent mode */
   8,                   /* tab size */
   8,                   /* tab size */
   TRUE,                /* default smart tab mode */
   FALSE,               /* inflate tabs? */
   IGNORE,              /* sort case */
   FALSE,               /* enhanced keyboard flag - set in console.c */
   SMALL_CURSOR | (MEDIUM_CURSOR << 2), /* jmh 990404 */
                        /* default cursor size == small insert, medium overw */
   FALSE,               /* default FALSE = do not write ^Z at end of file */
   NATIVE,              /* default write <cr><lf> at eol */
   TRUE,                /* default remove trailing space on edited lines */
   FALSE,               /* default show eol character is off */
   NO_WRAP,             /* default word wrap mode is off */
   0,                   /* default left margin - add 1 to margins for display */
   2,                   /* default paragraph begin */
   71,                  /* default right margin */
   FALSE,               /* default justify right margin */
   FALSE,               /* format paragraph/text semaphore - DO NOT CHANGE */
   UNDO_STACK_LEN,      /* number of undo lines to keep */
   MAX_UNDOS,           /* number of undos to keep */
   TRUE,                /* default undo group */
   FALSE,               /* default undo move */
   FALSE,               /* default backup mode */
   TRUE,                /* default ruler mode */
   "%H:%0n  %D, %d %M, %Y",     /* default time-stamp format */
                                /* eg: "17:18  Sunday, 31 May, 1998" */
   FALSE,               /* default cursor cross is off */
   SORT_NAME,           /* default filename sort for directory list */
#if defined( __UNIX__ ) && !defined( PC_CHARS )
   -1,                  /* default disabled ASCII graphic characters */
#else
   -2,                  /* default disabled single-line graphic characters */
#endif
   CUR_RIGHT,           /* cursor direction - DO NOT CHANGE */
   FALSE,               /* draw mode - DO NOT CHANGE */
   FALSE,               /* default line number display */
   TRUE,                /* default auto save workspace */
   FALSE,               /* default don't track path */
   FALSE,               /* default don't display cwd */
   "tde.txt",           /* default help file */
   "^<~>",              /* default help topic */
};


/*
 * Initialise the histories. jmh 990424.
 */
HISTORY h_file   = { 0, &h_file,   &h_file,   "" };
HISTORY h_find   = { 0, &h_find,   &h_find,   "" };
HISTORY h_border = { 0, &h_border, &h_border, "" };
HISTORY h_fill   = { 0, &h_fill,   &h_fill,   "" };
HISTORY h_stamp  = { 0, &h_stamp,  &h_stamp,  "" };
HISTORY h_line   = { 0, &h_line,   &h_line,   "" };
HISTORY h_lang   = { 0, &h_lang,   &h_lang,   "" };
HISTORY h_grep   = { 0, &h_grep,   &h_grep,   "" };
HISTORY h_win    = { 0, &h_win,    &h_win,    "" };
HISTORY h_exec   = { 0, &h_exec,   &h_exec,   "" };
HISTORY h_parm   = { 0, &h_parm,   &h_parm,   "" };


/*
 * Default color settings.  Incidentally, I'm color blind (mild red-green) and
 * the default colors look fine to me, Frank.
 * jmh 980804: supplied my own defaults, since I now display the credit
 *             screen before the config is loaded.
 */
/*
 * The following defines specify which video attributes give desired
 *  effects on different display devices.
 * REVERSE is supposed to be reverse video - a different background color,
 *  so that even a blank space can be identified.
 * HIGH is supposed to quickly draw the user's eye to the relevant part of
 *  the screen, either for a message or for matched text in find/replace.
 * NORMAL is supposed to be something pleasant to look at for the main
 *  body of the text.
 */
/*
 * jmh 990408: renamed from HERC_ to MONO_ and added UNIX values
 * jmh 991111: removed UNIX values, added STANDOUT, removed BOLD_UNDER.
 */

#define MONO_NORMAL     0x07
#define MONO_BOLD       0x0f
#define MONO_REVERSE    0x70
#define MONO_STANDOUT   0x7f
#define MONO_UNDER      0x01

#define COLOR_HEAD      0x4b    /* Bright Cyan   on Red     */
#define COLOR_BLOCK     0x71    /*        Blue   on Gray    */
#define COLOR_CROSS     0x10    /* actually a XOR mask, not a color */
#define COLOR_RULER     0x02    /*        Green  on Black   */
#define COLOR_POINTER   0x0a    /* Bright Green  on Black   */
#if 0                           /* change to 1 for Frank's default colors */
#define COLOR_MODE      0x17    /*        Gray   on Blue    */
#define COLOR_DIAG      0x0e    /*        Yellow on Black   */
#define COLOR_SPECIAL   0x97    /*        Gray   on Bright Blue */
#define COLOR_CWD       0x17    /*        Gray   on Blue    */
#define COLOR_TEXT      0x07    /*        Gray   on Black   */
#define COLOR_DIRTY     0x02    /*        Green  on Black   */
#define COLOR_CURL      0x0f    /*        White  on Black   */
#define COLOR_EOF       0x09    /* Bright Blue   on Black   */
#define COLOR_SWAP      0x07    /*        Gray   on Black   */
#define COLOR_MESSAGE   0x0f    /*        White  on Black   */
#define COLOR_BP        0x07    /*        Gray   on Black   */
#define COLOR_HELP      0x1a    /* Bright Green  on Blue    */
#define COLOR_DIALOG    0x1a    /* Bright Green  on Blue    */
#define COLOR_EDIT      0x1f    /*        White  on Blue    */
#define COLOR_DISABLED  0x12    /*        Green  on Blue    */
#define COLOR_HILITE    0x07    /*        Gray   on Black   */
#define COLOR_MENUHEAD  0x17    /*        Gray   on Blue    */
#define COLOR_MENUSEL   0x71    /*        Blue   on Gray    */
#define COLOR_POPUP     0x1a    /* Bright Green  on Blue    */
#define COLOR_MENUDIS   0x12    /*        Green  on Blue    */
#define COLOR_MENUITEM  0x07    /*        Gray   on Black   */
#define COLOR_MENUNITEM 0x08    /*   Dark Gray   on Black   */
#define COLOR_OVRS      0x00    /*                  Black   */
#else
#define COLOR_MODE      0x6f    /*        White  on Brown   */
#define COLOR_DIAG      0x03    /*        Cyan   on Black   */
#define COLOR_SPECIAL   0xe4    /*        Red    on Yellow  */
#define COLOR_CWD       0x6f    /*        White  on Brown   */
#define COLOR_TEXT      0x17    /*        Gray   on Blue    */
#define COLOR_DIRTY     0x1e    /*        Yellow on Blue    */
#define COLOR_CURL      0x1f    /*        White  on Blue    */
#define COLOR_EOF       0x12    /*        Green  on Blue    */
#define COLOR_SWAP      0x5f    /*        White  on Magenta */
#define COLOR_MESSAGE   0x4f    /*        White  on Red     */
#define COLOR_BP        0x5f    /*        White  on Magenta */
#define COLOR_HELP      0x30    /*        Black  on Cyan    */
#define COLOR_DIALOG    0x70    /*        Black  on Grey    */
#define COLOR_EDIT      0x79    /* Bright Blue   on Grey    */
#define COLOR_DISABLED  0x78    /*   Dark Grey   on Grey    */
#define COLOR_HILITE    0x5f    /*        White  on Magenta */
#define COLOR_MENUHEAD  0x30    /*        Black  on Cyan    */
#define COLOR_MENUSEL   0x3e    /*        Yellow on Cyan    */
#define COLOR_POPUP     0x70    /*        Black  on Grey    */
#define COLOR_MENUDIS   0x78    /*   Dark Grey   on Grey    */
#define COLOR_MENUITEM  0x2f    /*        White  on Green   */
#define COLOR_MENUNITEM 0x28    /*   Dark Grey   on Green   */
#define COLOR_OVRS      0x01    /*                  Blue    */
#endif

TDE_COLORS colour = {
   { MONO_REVERSE,  MONO_REVERSE,   MONO_NORMAL,     MONO_BOLD,
     MONO_NORMAL,   MONO_UNDER,     MONO_NORMAL,     MONO_BOLD,
     MONO_BOLD,     MONO_UNDER,     MONO_REVERSE,    MONO_STANDOUT,
     MONO_REVERSE,  MONO_REVERSE,   MONO_STANDOUT,   MONO_STANDOUT,
     MONO_NORMAL,   MONO_NORMAL,    MONO_BOLD,       MONO_UNDER,
     MONO_REVERSE,  MONO_REVERSE,   MONO_NORMAL,     MONO_NORMAL,
     MONO_UNDER,    MONO_REVERSE,   MONO_REVERSE,    0
   },
   { COLOR_HEAD,    COLOR_MODE,     COLOR_DIAG,      COLOR_SPECIAL,
     COLOR_CWD,     COLOR_MESSAGE,  COLOR_TEXT,      COLOR_DIRTY,
     COLOR_CURL,    COLOR_EOF,      COLOR_BLOCK,     COLOR_SWAP,
     COLOR_CROSS,   COLOR_RULER,    COLOR_POINTER,   COLOR_BP,
     COLOR_HELP,    COLOR_DIALOG,   COLOR_EDIT,      COLOR_DISABLED,
     COLOR_HILITE,  COLOR_MENUHEAD, COLOR_MENUSEL,   COLOR_POPUP,
     COLOR_MENUDIS, COLOR_MENUITEM, COLOR_MENUNITEM, COLOR_OVRS
   }
};


/*
 * Default syntax highlighting colors.
 */
int syntax_color[SHL_NUM_COLORS] = {
   30,          /* Normal       -        yellow on blue */
   28,          /* Bad          - bright red    on blue */
   31,          /* Keyword      -        white  on blue */
   23,          /* Comment      -        grey   on blue */
   26,          /* Function     - bright green  on blue */
   27,          /* String       - bright cyan   on blue */
   27,          /* Character    - bright cyan   on blue */
   27,          /* Integer      - bright cyan   on blue */
   27,          /* Binary       - bright cyan   on blue */
   27,          /* Octal        - bright cyan   on blue */
   27,          /* Hex          - bright cyan   on blue */
   27,          /* Real         - bright cyan   on blue */
   19,          /* Preprocessor -        cyan   on blue */
   31           /* Symbol       -        white  on blue */
};


/*
 * do_it is an array of pointers to functions that return int with an argument
 * that is a pointer to a window.  Is that right???
 */
int  (* const do_it[NUM_FUNCS])( TDE_WIN * ) = {
   insert_overwrite,                /*   regular text keys          0  */
   get_help,                        /*   Help                       1  */
   insert_newline,                  /*   Rturn                      2  */
   next_line,                       /*   NextLine                   3  */
   beg_next_line,                   /*   BegNextLine                4  */
   move_down,                       /*   LineDown                   5  */
   move_up,                         /*   LineUp                     6  */
   move_right,                      /*   CharRight                  7  */
   move_left,                       /*   CharLeft                   8  */
   pan_right,                       /*   PanRight                   9  */
   pan_left,                        /*   PanLeft                   10  */
   word_right,                      /*   WordRight                 11  */
   word_left,                       /*   WordLeft                  12  */
   page_down,                       /*   ScreenDown                13  */
   page_up,                         /*   ScreenUp                  14  */
   goto_end_file,                   /*   EndOfFile                 15  */
   goto_top_file,                   /*   TopOfFile                 16  */
   goto_bottom,                     /*   BotOfScreen               17  */
   goto_top,                        /*   TopOfScreen               18  */
   goto_eol,                        /*   EndOfLine                 19  */
   home,                            /*   BegOfLine                 20  */
   goto_position,                   /*   JumpToPosition            21  */
   center_window,                   /*   CenterWindow              22  */
   center_window,                   /*   CenterLine                23  */
   screen_right,                    /*   ScreenRight               24  */
   screen_left,                     /*   ScreenLeft                25  */
   scroll_down,                     /*   ScrollDnLine              26  */
   scroll_up,                       /*   ScrollUpLine              27  */
   pan_up,                          /*   PanUp                     28  */
   pan_down,                        /*   PanDn                     29  */
   toggle_overwrite,                /*   ToggleOverWrite           30  */
   toggle_smart_tabs,               /*   ToggleSmartTabs           31  */
   toggle_indent,                   /*   ToggleIndent              32  */
   toggle_ww,                       /*   ToggleWordWrap            33  */
   toggle_crlf,                     /*   ToggleCRLF                34  */
   toggle_trailing,                 /*   ToggleTrailing            35  */
   toggle_z,                        /*   ToggleZ                   36  */
   toggle_eol,                      /*   ToggleEol                 37  */
   toggle_sync,                     /*   ToggleSync                38  */
   toggle_ruler,                    /*   ToggleRuler               39  */
   toggle_tabinflate,               /*   ToggleTabInflate          40  */
   set_tabstop,                     /*   SetTabs                   41  */
   set_margins,                     /*   SetMargins                42  */
   format_paragraph,                /*   FormatParagraph           43  */
   format_paragraph,                /*   FormatText                44  */
   flush_left,                      /*   LeftJustify               45  */
   flush_right,                     /*   RightJustify              46  */
   flush_center,                    /*   CenterJustify             47  */
   tab_key,                         /*   Tab                       48  */
   backtab,                         /*   BackTab                   49  */
   match_pair,                      /*   ParenBalance              50  */
   back_space,                      /*   BackSpace                 51  */
   char_del_under,                  /*   DeleteChar                52  */
   char_del_under,                  /*   StreamDeleteChar          53  */
   line_kill,                       /*   DeleteLine                54  */
   eol_kill,                        /*   DelEndOfLine              55  */
   word_delete,                     /*   WordDelete                56  */
   insert_newline,                  /*   AddLine                   57  */
   insert_newline,                  /*   SplitLine                 58  */
   join_line,                       /*   JoinLine                  59  */
   dup_line,                        /*   DuplicateLine             60  */
   restore_line,                    /*   RestoreLine               61  */
   retrieve_line,                   /*   RetrieveLine              62  */
   toggle_search_case,              /*   ToggleSearchCase          63  */
   find_string,                     /*   FindForward               64  */
   find_string,                     /*   FindBackward              65  */
   find_string,                     /*   RepeatFindForward         66  */
   find_string,                     /*   RepeatFindBackward        67  */
   replace_string,                  /*   ReplaceString             68  */
   define_diff,                     /*   DefineDiff                69  */
   repeat_diff,                     /*   RepeatDiff                70  */
   mark_block,                      /*   MarkBlock                 71  */
   mark_block,                      /*   MarkLine                  72  */
   mark_block,                      /*   MarkStream                73  */
   unmark_block,                    /*   UnMarkBlock               74  */
   block_operation,                 /*   FillBlock                 75  */
   block_operation,                 /*   NumberBlock               76  */
   block_operation,                 /*   CopyBlock                 77  */
   block_operation,                 /*   KopyBlock                 78  */
   block_operation,                 /*   MoveBlock                 79  */
   block_operation,                 /*   OverlayBlock              80  */
   block_operation,                 /*   DeleteBlock               81  */
   block_operation,                 /*   SwapBlock                 82  */
   block_write,                     /*   BlockToFile               83  */
   block_print,                     /*   PrintBlock                84  */
   block_expand_tabs,               /*   BlockExpandTabs           85  */
   block_compress_tabs,             /*   BlockCompressTabs         86  */
   block_compress_tabs,             /*   BlockIndentTabs           87  */
   block_trim_trailing,             /*   BlockTrimTrailing         88  */
   block_convert_case,              /*   BlockUpperCase            89  */
   block_convert_case,              /*   BlockLowerCase            90  */
   block_convert_case,              /*   BlockRot13                91  */
   block_convert_case,              /*   BlockFixUUE               92  */
   block_email_reply,               /*   BlockEmailReply           93  */
   block_convert_case,              /*   BlockStripHiBit           94  */
   sort_box_block,                  /*   SortBoxBlock              95  */
   date_time_stamp,                 /*   DateTimeStamp             96  */
   edit_another_file,               /*   EditFile                  97  */
   dir_help,                        /*   DirList                   98  */
   file_file,                       /*   File                      99  */
   save_file,                       /*   Save                     100  */
   save_as_file,                    /*   SaveAs                   101  */
   change_fattr,                    /*   SetFileAttributes        102  */
   edit_next_file,                  /*   EditNextFile             103  */
   search_and_seize,                /*   DefineGrep               104  */
   search_and_seize,                /*   RepeatGrep               105  */
   redraw_screen,                   /*   RedrawScreen             106  */
   size_window,                     /*   SizeWindow               107  */
   split_horizontal,                /*   SplitHorizontal          108  */
   split_vertical,                  /*   SplitVertical            109  */
   next_window,                     /*   NextWindow               110  */
   prev_window,                     /*   PreviousWindow           111  */
   zoom_window,                     /*   ZoomWindow               112  */
   next_window,                     /*   NextHiddenWindow         113  */
   set_marker,                      /*   SetMark1                 114  */
   set_marker,                      /*   SetMark2                 115  */
   set_marker,                      /*   SetMark3                 116  */
   goto_marker,                     /*   GotoMark1                117  */
   goto_marker,                     /*   GotoMark2                118  */
   goto_marker,                     /*   GotoMark3                119  */
   record_on_off,                   /*   RecordMacro              120  */
   play_back,                       /*   PlayBack                 121  */
   save_strokes,                    /*   SaveMacro                122  */
   tdecfgfile,                      /*   LoadMacro                123  */
   clear_macros,                    /*   ClearAllMacros           124  */
   macro_pause,                     /*   Pause                    125  */
   quit,                            /*   Quit                     126  */
   find_dirty_line,                 /*   NextDirtyLine            127  */
   find_dirty_line,                 /*   PrevDirtyLine            128  */
   find_regx,                       /*   RegXForward              129  */
   find_regx,                       /*   RegXBackward             130  */
   find_regx,                       /*   RepeatRegXForward        131  */
   find_regx,                       /*   RepeatRegXBackward       132  */
   main_pull_down,                  /*   PullDown                 133  */
   tdecfgfile,                      /*   ReadConfig               134  */
   /* the following functions were added by jmh */
   prev_window,                     /*   PrevHiddenWindow         135  */
   shell,                           /*   Shell                    136  */
   word_right,                      /*   StringRight              137  */
   word_left,                       /*   StringLeft               138  */
   user_screen,                     /*   UserScreen               139  */
   quit_all,                        /*   QuitAll                  140  */
   goto_marker,                     /*   BlockBegin               141  */
   goto_marker,                     /*   BlockEnd                 142  */
   back_space,                      /*   WordDeleteBack           143  */
   goto_marker,                     /*   PreviousPosition         144  */
   file_all,                        /*   FileAll                  145  */
   syntax_toggle,                   /*   SyntaxToggle             146  */
   syntax_select,                   /*   SyntaxSelect             147  */
   transpose,                       /*   Transpose                148  */
   edit_another_file,               /*   InsertFile               149  */
   word_right,                      /*   WordEndRight             150  */
   word_left,                       /*   WordEndLeft              151  */
   word_right,                      /*   StringEndRight           152  */
   word_left,                       /*   StringEndLeft            153  */
   stamp_format,                    /*   StampFormat              154  */
   play_back,                       /*   PseudoMacro              155  */
   set_marker,                      /*   MacroMark                156  */
   toggle_cursor_cross,             /*   ToggleCursorCross        157  */
   toggle_graphic_chars,            /*   ToggleGraphicChars       158  */
   repeat,                          /*   Repeat                   159  */
   block_operation,                 /*   BorderBlock              160  */
   mark_block,                      /*   MarkBegin                161  */
   mark_block,                      /*   MarkEnd                  162  */
   block_operation,                 /*   BlockLeftJustify         163  */
   block_operation,                 /*   BlockRightJustify        164  */
   block_operation,                 /*   BlockCenterJustify       165  */
   block_indent,                    /*   BlockIndentN             166  */
   block_indent,                    /*   BlockUndentN             167  */
   block_indent,                    /*   BlockIndent              168  */
   block_indent,                    /*   BlockUndent              169  */
   set_break_point,                 /*   SetBreakPoint            170  */
   change_cur_dir,                  /*   ChangeCurDir             171  */
   show_status,                     /*   Status                   172  */
   toggle_read_only,                /*   ToggleReadOnly           173  */
   goto_window,                     /*   GotoWindow               174  */
   block_convert_case,              /*   BlockInvertCase          175  */
   define_search,                   /*   DefineSearch             176  */
   repeat_search,                   /*   RepeatSearch             177  */
   toggle_draw,                     /*   ToggleDraw               178  */
   topbot_line,                     /*   TopLine                  179  */
   topbot_line,                     /*   BottomLine               180  */
   toggle_line_numbers,             /*   ToggleLineNumbers        181  */
   get_help,                        /*   CharacterSet             182  */
   block_operation,                 /*   SumBlock                 183  */
   undo,                            /*   Undo                     184  */
   redo,                            /*   Redo                     185  */
   toggle_undo_group,               /*   ToggleUndoGroup          186  */
   toggle_undo_move,                /*   ToggleUndoMove           187  */
   show_statistics,                 /*   Statistics               188  */
   block_convert_case,              /*   BlockCapitalise          189  */
   save_workspace,                  /*   SaveWorkspace            190  */
   block_to_clipboard,              /*   CopyToClipboard          191  */
   block_to_clipboard,              /*   KopyToClipboard          192  */
   block_to_clipboard,              /*   CutToClipboard           193  */
   paste,                           /*   PasteFromClipboard       194  */
   page_down,                       /*   HalfScreenDown           195  */
   page_up,                         /*   HalfScreenUp             196  */
   screen_right,                    /*   HalfScreenRight          197  */
   screen_left,                     /*   HalfScreenLeft           198  */
   bol_kill,                        /*   DelBegOfLine             199  */
   bol_kill,                        /*   EraseBegOfLine           200  */
   set_path,                        /*   SetDirectory             201  */
   isearch,                         /*   ISearchForward           202  */
   isearch,                         /*   ISearchBackward          203  */
   toggle_cwd,                      /*   ToggleCWD                204  */
   edit_another_file,               /*   ScratchWindow            205  */
   make_horizontal,                 /*   MakeHorizontal           206  */
   make_vertical,                   /*   MakeVertical             207  */
   comment_block,                   /*   BlockBlockComment        208  */
   comment_block,                   /*   BlockLineComment         209  */
   uncomment_block,                 /*   BlockUnComment           210  */
   move_left,                       /*   StreamCharLeft           211  */
   move_right,                      /*   StreamCharRight          212  */
   goto_eol,                        /*   EndNextLine              213  */
   dynamic_tab_size,                /*   DynamicTabSize           214  */
   block_operation,                 /*   FillBlockDown            215  */
   block_operation,                 /*   FillBlockPattern         216  */
   block_operation,                 /*   BorderBlockEx            217  */
   block_write,                     /*   SaveTo                   218  */
   save_file,                       /*   SaveUntouched            219  */
   reload_file,                     /*   Revert                   220  */
   split_horizontal,                /*   SplitHalfHorizontal      221  */
   split_vertical,                  /*   SplitHalfVertical        222  */
   make_horizontal,                 /*   MakeHalfHorizontal       223  */
   make_vertical,                   /*   MakeHalfVertical         224  */
   title_window,                    /*   TitleWindow              225  */
   shell,                           /*   Execute                  226  */
   find_browse_line,                /*   NextBrowse               227  */
   find_browse_line,                /*   PrevBrowse               228  */
   save_all,                        /*   SaveAll                  229  */
   move_mark,                       /*   MoveMark                 230  */
   home,                            /*   StartOfLine              231  */
   get_help,                        /*   About                    232  */
   context_help,                    /*   ContextHelp              233  */
   popup_ruler,                     /*   PopupRuler               234  */
   balance_horizontal,              /*   BalanceHorizontal        235  */
   balance_vertical,                /*   BalanceVertical          236  */
   finish,                          /*   CloseWindow              237  */
   toggle_quickedit,                /*   ToggleQuickEdit          238  */
};


/*
 * func_flag is an array to indicate which functions can be used
 * in viewer mode, as well as assisting the menu.
 * jmh 991003: Most of the block functions are tested within their
 * own routine, since the current and marked windows could be different.
 * jmh 031129: renamed from read_only_func, due to extra flags.
 */
const char func_flag[NUM_FUNCS] = {
   F_MODIFY,                        /*   regular text keys          0  */
   0,                               /*   Help                       1  */
   F_MODIFY,                        /*   Rturn                      2  */
   0,                               /*   NextLine                   3  */
   0,                               /*   BegNextLine                4  */
   0,                               /*   LineDown                   5  */
   0,                               /*   LineUp                     6  */
   0,                               /*   CharRight                  7  */
   0,                               /*   CharLeft                   8  */
   0,                               /*   PanRight                   9  */
   0,                               /*   PanLeft                   10  */
   0,                               /*   WordRight                 11  */
   0,                               /*   WordLeft                  12  */
   0,                               /*   ScreenDown                13  */
   0,                               /*   ScreenUp                  14  */
   0,                               /*   EndOfFile                 15  */
   0,                               /*   TopOfFile                 16  */
   0,                               /*   BotOfScreen               17  */
   0,                               /*   TopOfScreen               18  */
   0,                               /*   EndOfLine                 19  */
   0,                               /*   BegOfLine                 20  */
   0,                               /*   JumpToPosition            21  */
   0,                               /*   CenterWindow              22  */
   0,                               /*   CenterLine                23  */
   0,                               /*   ScreenRight               24  */
   0,                               /*   ScreenLeft                25  */
   0,                               /*   ScrollDnLine              26  */
   0,                               /*   ScrollUpLine              27  */
   0,                               /*   PanUp                     28  */
   0,                               /*   PanDn                     29  */
   0,                               /*   ToggleOverWrite           30  */
   0,                               /*   ToggleSmartTabs           31  */
   0,                               /*   ToggleIndent              32  */
   0,                               /*   ToggleWordWrap            33  */
   0,                               /*   ToggleCRLF                34  */
   0,                               /*   ToggleTrailing            35  */
   0,                               /*   ToggleZ                   36  */
   0,                               /*   ToggleEol                 37  */
   0,                               /*   ToggleSync                38  */
   0,                               /*   ToggleRuler               39  */
   0,                               /*   ToggleTabInflate          40  */
   0,                               /*   SetTabs                   41  */
   0,                               /*   SetMargins                42  */
   F_MODIFY,                        /*   FormatParagraph           43  */
   F_MODIFY,                        /*   FormatText                44  */
   F_MODIFY,                        /*   LeftJustify               45  */
   F_MODIFY,                        /*   RightJustify              46  */
   F_MODIFY,                        /*   CenterJustify             47  */
   0,                               /*   Tab                       48  */
   0,                               /*   BackTab                   49  */
   0,                               /*   ParenBalance              50  */
   F_MODIFY,                        /*   BackSpace                 51  */
   F_MODIFY,                        /*   DeleteChar                52  */
   F_MODIFY,                        /*   StreamDeleteChar          53  */
   F_MODIFY,                        /*   DeleteLine                54  */
   F_MODIFY,                        /*   DelEndOfLine              55  */
   F_MODIFY,                        /*   WordDelete                56  */
   F_MODIFY,                        /*   AddLine                   57  */
   F_MODIFY,                        /*   SplitLine                 58  */
   F_MODIFY,                        /*   JoinLine                  59  */
   F_MODIFY,                        /*   DuplicateLine             60  */
   F_MODIFY,                        /*   RestoreLine               61  */
   F_MODIFY,                        /*   RetrieveLine              62  */
   0,                               /*   ToggleSearchCase          63  */
   0,                               /*   FindForward               64  */
   0,                               /*   FindBackward              65  */
   0,                               /*   RepeatFindForward         66  */
   0,                               /*   RepeatFindBackward        67  */
   F_MODIFY,                        /*   ReplaceString             68  */
   0,                               /*   DefineDiff                69  */
   0,                               /*   RepeatDiff                70  */
   0,                               /*   MarkBlock                 71  */
   0,                               /*   MarkLine                  72  */
   0,                               /*   MarkStream                73  */
   0,                               /*   UnMarkBlock               74  */
   F_BOX,                           /*   FillBlock                 75  */
   F_BOX,                           /*   NumberBlock               76  */
   F_MODIFY|F_BLOCK,                /*   CopyBlock                 77  */
   F_MODIFY|F_BLOCK,                /*   KopyBlock                 78  */
   F_MODIFY|F_BLOCK,                /*   MoveBlock                 79  */
   F_MODIFY|F_BOX|F_LINE,           /*   OverlayBlock              80  */
   F_BLOCK,                         /*   DeleteBlock               81  */
   F_MODIFY|F_BOX|F_LINE,           /*   SwapBlock                 82  */
   F_BLOCK|F_BSAFE,                 /*   BlockToFile               83  */
   0,                               /*   PrintBlock                84  */
   F_LINE,                          /*   BlockExpandTabs           85  */
   F_LINE,                          /*   BlockCompressTabs         86  */
   F_LINE,                          /*   BlockIndentTabs           87  */
   F_LINE,                          /*   BlockTrimTrailing         88  */
   F_BLOCK,                         /*   BlockUpperCase            89  */
   F_BLOCK,                         /*   BlockLowerCase            90  */
   F_BLOCK,                         /*   BlockRot13                91  */
   F_BLOCK,                         /*   BlockFixUUE               92  */
   F_LINE,                          /*   BlockEmailReply           93  */
   F_BLOCK,                         /*   BlockStripHiBit           94  */
   F_BOX|F_LINE,                    /*   SortBoxBlock              95  */
   0,                               /*   DateTimeStamp             96  */
   0,                               /*   EditFile                  97  */
   0,                               /*   DirList                   98  */
   0,                               /*   File                      99  */
   0,                               /*   Save                     100  */
   0,                               /*   SaveAs                   101  */
   0,                               /*   FileAttributes           102  */
   0,                               /*   EditNextFile             103  */
   0,                               /*   DefineGrep               104  */
   0,                               /*   RepeatGrep               105  */
   0,                               /*   RedrawScreen             106  */
   0,                               /*   SizeWindow               107  */
   0,                               /*   SplitHorizontal          108  */
   0,                               /*   SplitVertical            109  */
   0,                               /*   NextWindow               110  */
   0,                               /*   PreviousWindow           111  */
   0,                               /*   ZoomWindow               112  */
   0,                               /*   NextHiddenWindow         113  */
   0,                               /*   SetMark1                 114  */
   0,                               /*   SetMark2                 115  */
   0,                               /*   SetMark3                 116  */
   0,                               /*   GotoMark1                117  */
   0,                               /*   GotoMark2                118  */
   0,                               /*   GotoMark3                119  */
   0,                               /*   RecordMacro              120  */
   0,                               /*   PlayBack                 121  */
   0,                               /*   SaveMacro                122  */
   0,                               /*   LoadMacro                123  */
   0,                               /*   ClearAllMacros           124  */
   0,                               /*   Pause                    125  */
   0,                               /*   Quit                     126  */
   0,                               /*   NextDirtyLine            127  */
   0,                               /*   PrevDirtyLine            128  */
   0,                               /*   RegXForward              129  */
   0,                               /*   RegXBackward             130  */
   0,                               /*   RepeatRegXForward        131  */
   0,                               /*   RepeatRegXBackward       132  */
   0,                               /*   PullDown                 133  */
   0,                               /*   ReadConfig               134  */
   0,                               /*   PrevHiddenWindow         135  */
   0,                               /*   Shell                    136  */
   0,                               /*   StringRight              137  */
   0,                               /*   StringLeft               138  */
   0,                               /*   UserScreen               139  */
   0,                               /*   QuitAll                  140  */
   F_BLOCK|F_BSAFE,                 /*   BlockBegin               141  */
   F_BLOCK|F_BSAFE,                 /*   BlockEnd                 142  */
   F_MODIFY,                        /*   WordDeleteBack           143  */
   0,                               /*   PreviousPosition         144  */
   0,                               /*   FileAll                  145  */
   0,                               /*   SyntaxToggle             146  */
   0,                               /*   SyntaxSelect             147  */
   F_MODIFY,                        /*   Transpose                148  */
   F_MODIFY,                        /*   InsertFile               149  */
   0,                               /*   WordEndRight             150  */
   0,                               /*   WordEndLeft              151  */
   0,                               /*   StringEndRight           152  */
   0,                               /*   StringEndLeft            153  */
   0,                               /*   StampFormat              154  */
   F_MODIFY,                        /*   PseudoMacro              155  */
   0,                               /*   MacroMark                156  */
   0,                               /*   ToggleCursorCross        157  */
   0,                               /*   ToggleGraphicChars       158  */
   0,                               /*   Repeat                   159  */
   F_BOX,                           /*   BorderBlock              160  */
   0,                               /*   MarkBegin                161  */
   0,                               /*   MarkEnd                  162  */
   F_LINE|F_BOX,                    /*   BlockLeftJustify         163  */
   F_LINE|F_BOX,                    /*   BlockRightJustify        164  */
   F_LINE|F_BOX,                    /*   BlockCenterJustify       165  */
   F_LINE|F_BOX,                    /*   BlockIndentN             166  */
   F_LINE|F_BOX,                    /*   BlockUndentN             167  */
   F_LINE|F_BOX,                    /*   BlockIndent              168  */
   F_LINE|F_BOX,                    /*   BlockUndent              169  */
   0,                               /*   SetBreakPoint            170  */
   0,                               /*   ChangeCurDir             171  */
   0,                               /*   Status                   172  */
   0,                               /*   ToggleReadOnly           173  */
   0,                               /*   GotoWindow               174  */
   F_BLOCK,                         /*   BlockInvertCase          175  */
   0,                               /*   DefineSearch             176  */
   0,                               /*   RepeatSearch             177  */
   F_MODIFY,                        /*   ToggleDraw               178  */
   0,                               /*   TopLine                  179  */
   0,                               /*   BottomLine               180  */
   0,                               /*   ToggleLineNumbers        181  */
   0,                               /*   CharacterSet             182  */
   F_BOX,                           /*   SumBlock                 183  */
   F_MODIFY,                        /*   Undo                     184  */
   F_MODIFY,                        /*   Redo                     185  */
   0,                               /*   ToggleUndoGroup          186  */
   0,                               /*   ToggleUndoMove           187  */
   0,                               /*   Statistics               188  */
   F_BLOCK,                         /*   BlockCapitalise          189  */
   0,                               /*   SaveWorkspace            190  */
   F_BLOCK|F_BSAFE,                 /*   CopyToClipboard          191  */
   F_BLOCK|F_BSAFE,                 /*   KopyToClipboard          192  */
   F_BLOCK,                         /*   CutToClipboard           193  */
   F_MODIFY,                        /*   PasteFromClipboard       194  */
   0,                               /*   HalfScreenDown           195  */
   0,                               /*   HalfScreenUp             196  */
   0,                               /*   HalfScreenRight          197  */
   0,                               /*   HalfScreenLeft           198  */
   F_MODIFY,                        /*   DelBegOfLine             199  */
   F_MODIFY,                        /*   EraseBegOfLine           200  */
   0,                               /*   SetDirectory             201  */
   0,                               /*   ISearchForward           202  */
   0,                               /*   ISearchBackward          203  */
   0,                               /*   ToggleCWD                204  */
   0,                               /*   ScratchWindow            205  */
   0,                               /*   MakeHorizontal           206  */
   0,                               /*   MakeVertical             207  */
   F_BLOCK,                         /*   BlockBlockComment        208  */
   F_BLOCK,                         /*   BlockLineComment         209  */
   F_BLOCK,                         /*   BlockUnComment           210  */
   0,                               /*   StreamCharLeft           211  */
   0,                               /*   StreamCharRight          212  */
   0,                               /*   EndNextLine              213  */
   0,                               /*   DynamicTabSize           214  */
   F_BOX,                           /*   FillBlockDown            215  */
   F_BOX,                           /*   FillBlockPattern         216  */
   F_BOX,                           /*   BorderBlockEx            217  */
   0,                               /*   SaveTo                   218  */
   0,                               /*   SaveUntouched            219  */
   F_MODIFY,                        /*   Revert                   220  */
   0,                               /*   SplitHalfHorizontal      221  */
   0,                               /*   SplitHalfVertical        222  */
   0,                               /*   MakeHalfHorizontal       223  */
   0,                               /*   MakeHalfVertical         224  */
   0,                               /*   TitleWindow              225  */
   0,                               /*   Execute                  226  */
   0,                               /*   NextBrowse               227  */
   0,                               /*   PrevBrowse               228  */
   0,                               /*   SaveAll                  229  */
   F_BLOCK|F_BSAFE,                 /*   MoveMark                 230  */
   0,                               /*   StartOfLine              231  */
   0,                               /*   About                    232  */
   0,                               /*   ContextHelp              233  */
   0,                               /*   PopupRuler               234  */
   0,                               /*   BalanceHorizontal        235  */
   0,                               /*   BalanceVertical          236  */
   0,                               /*   CloseWindow              237  */
   0,                               /*   ToggleQuickEdit          238  */
};
