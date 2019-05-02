/*
 * New editor name:  TDE, the Thomson-Davis Editor.
 * Author:           Frank Davis
 * Date:             June 5, 1991
 *
 * This modification of Douglas Thomson's code is released into the
 * public domain, Frank Davis.  You may distribute it freely.
 *
 * This file defines all functions in the editor.
 */

#define   Help                        1
#define   Rturn                       2
#define   NextLine                    3
#define   BegNextLine                 4
#define   LineDown                    5
#define   LineUp                      6
#define   CharRight                   7
#define   CharLeft                    8
#define   PanRight                    9
#define   PanLeft                    10
#define   WordRight                  11
#define   WordLeft                   12
#define   ScreenDown                 13
#define   ScreenUp                   14
#define   EndOfFile                  15
#define   TopOfFile                  16
#define   BotOfScreen                17
#define   TopOfScreen                18
#define   EndOfLine                  19
#define   BegOfLine                  20
#define   JumpToPosition             21
#define   CenterWindow               22
#define   CenterLine                 23
#define   ScreenRight                24         /* renamed by jmh 020906 */
#define   ScreenLeft                 25         /* ditto */
#define   ScrollDnLine               26
#define   ScrollUpLine               27
#define   PanUp                      28
#define   PanDn                      29
#define   ToggleOverWrite            30
#define   ToggleSmartTabs            31
#define   ToggleIndent               32
#define   ToggleWordWrap             33
#define   ToggleCRLF                 34
#define   ToggleTrailing             35
#define   ToggleZ                    36
#define   ToggleEol                  37
#define   ToggleSync                 38
#define   ToggleRuler                39
#define   ToggleTabInflate           40
#define   SetTabs                    41
#define   SetMargins                 42
#define   FormatParagraph            43
#define   FormatText                 44
#define   LeftJustify                45
#define   RightJustify               46
#define   CenterJustify              47
#define   Tab                        48
#define   BackTab                    49
#define   ParenBalance               50
#define   BackSpace                  51
#define   DeleteChar                 52
#define   StreamDeleteChar           53
#define   DeleteLine                 54
#define   DelEndOfLine               55
#define   WordDelete                 56
#define   AddLine                    57
#define   SplitLine                  58
#define   JoinLine                   59
#define   DuplicateLine              60
#define   RestoreLine                61         /* renamed by jmh 010521 */
#define   RetrieveLine               62         /* renamed by jmh 010521 */
#define   ToggleSearchCase           63
#define   FindForward                64
#define   FindBackward               65
#define   RepeatFindForward          66
#define   RepeatFindBackward         67
#define   ReplaceString              68
#define   DefineDiff                 69
#define   RepeatDiff                 70
#define   MarkBox                    71
#define   MarkLine                   72
#define   MarkStream                 73
#define   UnMarkBlock                74
#define   FillBlock                  75
#define   NumberBlock                76
#define   CopyBlock                  77
#define   KopyBlock                  78
#define   MoveBlock                  79
#define   OverlayBlock               80
#define   DeleteBlock                81
#define   SwapBlock                  82
#define   BlockToFile                83
#define   PrintBlock                 84
#define   BlockExpandTabs            85
#define   BlockCompressTabs          86
#define   BlockIndentTabs            87
#define   BlockTrimTrailing          88
#define   BlockUpperCase             89
#define   BlockLowerCase             90
#define   BlockRot13                 91
#define   BlockFixUUE                92
#define   BlockEmailReply            93
#define   BlockStripHiBit            94
#define   SortBoxBlock               95
#define   DateTimeStamp              96
#define   EditFile                   97
#define   DirList                    98
#define   File                       99
#define   Save                      100
#define   SaveAs                    101
#define   FileAttributes            102
#define   EditNextFile              103
#define   DefineGrep                104
#define   RepeatGrep                105
#define   RedrawScreen              106
#define   SizeWindow                107
#define   SplitHorizontal           108
#define   SplitVertical             109
#define   NextWindow                110
#define   PreviousWindow            111
#define   ZoomWindow                112
#define   NextHiddenWindow          113
#define   SetMark1                  114
#define   SetMark2                  115
#define   SetMark3                  116
#define   GotoMark1                 117
#define   GotoMark2                 118
#define   GotoMark3                 119
#define   RecordMacro               120
#define   PlayBack                  121
#define   SaveMacro                 122
#define   LoadMacro                 123
#define   ClearAllMacros            124
#define   Pause                     125
#define   Quit                      126
#define   NextDirtyLine             127
#define   PrevDirtyLine             128
#define   RegXForward               129         /* renamed by jmh 990915 */
#define   RegXBackward              130         /* added by jmh 990915 */
#define   RepeatRegXForward         131         /* renamed by jmh 990915 */
#define   RepeatRegXBackward        132         /* renamed by jmh 990915 */
#define   PullDown                  133
#define   ReadConfig                134
/*
 * The following functions were added by jmh
 */
#define   PrevHiddenWindow          135         /* 961124 */
#define   Shell                     136         /* 961127 */
#define   StringRight               137
#define   StringLeft                138
#define   UserScreen                139         /* 961228 */
#define   QuitAll                   140         /* 961229 */
#define   BlockBegin                141         /* 970809 */
#define   BlockEnd                  142         /* 970809 */
#define   WordDeleteBack            143         /* 970810 */
#define   PreviousPosition          144         /* 970913 */
#define   FileAll                   145         /* 970821 */
#define   ToggleSyntax              146         /* 970830 */
#define   SyntaxSelect              147         /* 970830 */
#define   Transpose                 148         /* 970911 */
#define   InsertFile                149         /* 980124 */
#define   WordEndRight              150         /* 980521 */
#define   WordEndLeft               151         /* 980521 */
#define   StringEndRight            152         /* 980521 */
#define   StringEndLeft             153         /* 980521 */
#define   StampFormat               154         /* 980521 */
#define   PseudoMacro               155         /* 980718 */
#define   MacroMark                 156         /* 980718 */
#define   ToggleCursorCross         157         /* 980724 */
#define   ToggleGraphicChars        158         /* 980724 */
#define   Repeat                    159         /* 980726 */
#define   BorderBlock               160         /* 980731 */
#define   MarkBegin                 161         /* 980728 */
#define   MarkEnd                   162         /* 980728 */
#define   BlockLeftJustify          163         /* 980810 */
#define   BlockRightJustify         164         /* 980810 */
#define   BlockCenterJustify        165         /* 980810 */
#define   BlockIndentN              166         /* 980811 */
#define   BlockUndentN              167         /* 980811 */
#define   BlockIndent               168         /* 980811 */
#define   BlockUndent               169         /* 980811 */
#define   SetBreakPoint             170         /* 980815 */
#define   ChangeCurDir              171         /* 981129 */
#define   Status                    172         /* 990410 */
#define   ToggleReadOnly            173         /* 990428 */
#define   GotoWindow                174         /* 990502 */
#define   BlockInvertCase           175         /* 990915 */
#define   DefineSearch              176         /* 990923 */
#define   RepeatSearch              177         /* 990923 */
#define   ToggleDraw                178         /* 991018 */
#define   TopLine                   179         /* 991025 */
#define   BottomLine                180         /* 991025 */
#define   ToggleLineNumbers         181         /* 991108 */
#define   CharacterSet              182         /* 991110 */
#define   SumBlock                  183         /* 991112 */
#define   Undo                      184         /* 991120 */
#define   Redo                      185         /* 991120 */
#define   ToggleUndoGroup           186         /* 991120 */
#define   ToggleUndoMove            187         /* 010520 */
#define   Statistics                188         /* 010605 */
#define   BlockCapitalise           189         /* 010624 */
#define   SaveWorkspace             190         /* 020722 */
#define   CopyToClipboard           191         /* 020826 */
#define   KopyToClipboard           192         /* 020826 */
#define   CutToClipboard            193         /* 020826 */
#define   PasteFromClipboard        194         /* 020826 */
#define   HalfScreenDown            195         /* 020906 */
#define   HalfScreenUp              196         /* 020906 */
#define   HalfScreenRight           197         /* 020906 */
#define   HalfScreenLeft            198         /* 020906 */
#define   DelBegOfLine              199         /* 020911 */
#define   EraseBegOfLine            200         /* 020911 */
#define   SetDirectory              201         /* 021021 */
#define   ISearchForward            202         /* 021028 */
#define   ISearchBackward           203         /* 021028 */
#define   ToggleCWD                 204         /* 030226 */
#define   ScratchWindow             205         /* 030226 */
#define   MakeHorizontal            206         /* 030226 */
#define   MakeVertical              207         /* 030226 */
#define   BlockBlockComment         208         /* 030302 */
#define   BlockLineComment          209         /* 030302 */
#define   BlockUnComment            210         /* 030302 */
#define   StreamCharLeft            211         /* 030302 */
#define   StreamCharRight           212         /* 030302 */
#define   EndNextLine               213         /* 030303 */
#define   DynamicTabSize            214         /* 030304 */
#define   FillBlockDown             215         /* 030305 */
#define   FillBlockPattern          216         /* 030305 */
#define   BorderBlockEx             217         /* 030305 */
#define   SaveTo                    218         /* 030318 */
#define   SaveUntouched             219         /* 030318 */
#define   Revert                    220         /* 030318 */
#define   SplitHalfHorizontal       221         /* 030323 */
#define   SplitHalfVertical         222         /* 030323 */
#define   MakeHalfHorizontal        223         /* 030323 */
#define   MakeHalfVertical          224         /* 030323 */
#define   TitleWindow               225         /* 030331 */
#define   Execute                   226         /* 031026 */
#define   NextBrowse                227         /* 031116 */
#define   PrevBrowse                228         /* 031116 */
#define   SaveAll                   229         /* 040715 */
#define   MoveMark                  230         /* 050710 */
#define   StartOfLine               231         /* 050710 */
#define   About                     232         /* 050710 */
#define   ContextHelp               233         /* 050710 */
#define   PopupRuler                234         /* 050710 */
#define   BalanceHorizontal         235         /* 050724 */
#define   BalanceVertical           236         /* 050724 */
#define   CloseWindow               237         /* 050727 */
#define   ToggleQuickEdit           238         /* 060219 */

/*
 * TwoCharKey is not an editor function, just a key function (ie. you can
 * press it, but it's not part of do_it[]).
 */
#define   TwoCharKey                NUM_FUNCS

/*
 * jmh 991121: made AbortCommand similar to TwoCharKey.
 */
#define   AbortCommand              (NUM_FUNCS+1)

/*
 * WordWrap must be the last function. It is neither a key function nor an
 * editor function, but a helper function.
 */
#define   WordWrap                  (NUM_FUNCS+2)


/*
 * jmh 031028: index of the color settings
 */
#define   Color( col ) g_display.color[col##_color]
#define   Head_color           0   /* file header color */
#define   Mode_color           1   /* mode line color - footer */
#define   Diag_color           2   /* color for diagnostics in mode line */
#define   Special_color        3   /* color of special mode (rec. macro, etc) */
#define   CWD_color            4   /* color of the current directory */
#define   Message_color        5   /* color of editor messages */
#define   Text_color           6   /* text area color - normal text */
#define   Dirty_color          7   /* text area color - modified line */
#define   Curl_color           8   /* color of cursor line */
#define   EOF_color            9   /* color for end of file line */
#define   Block_color         10   /* hilited area of blocked region */
#define   Swap_color          11   /* hilited area of swap blocked region */
#define   Cross_color         12   /* color of cursor cross (XOR mask) */
#define   Ruler_color         13   /* color of ruler line */
#define   Pointer_color       14   /* color of ruler pointer */
#define   BP_color            15   /* color of break point */
#define   Help_color          16   /* color of help screen */
#define   Dialog_color        17   /* color of dialogs */
#define   EditLabel_color     18   /* color of selected edit field's label */
#define   Disabled_color      19   /* color of disabled dialog items */
#define   Hilited_file_color  20   /* color of current file in dir list */
#define   Menu_header_color   21   /* color of menu headings */
#define   Menu_sel_color      22   /* color of selected menu heading */
#define   Menu_color          23   /* color of pull-down menu */
#define   Menu_dis_color      24   /* color of disabled menu item */
#define   Menu_item_color     25   /* color of selected menu item */
#define   Menu_nitem_color    26   /* color of selected disabled menu item */
#define   Overscan_color      27   /* color of overscan color */

/*
 *  These are not functions.  They redefine the Control code sequence.
 *  jmh 020816: modified the entire sequence. All potential keys are
 *               available. The sequence is the same as the scan codes,
 *               but starting from 0, with 256 to separate from the
 *               character codes, and 512, 1024 & 2048 bit masks to
 *               indicate Shift, Ctrl and Alt modifiers.
 */
#define    _ESC                       256
#define    _1                         257
#define    _2                         258
#define    _3                         259
#define    _4                         260
#define    _5                         261
#define    _6                         262
#define    _7                         263
#define    _8                         264
#define    _9                         265
#define    _0                         266
#define    _MINUS                     267
#define    _EQUALS                    268
#define    _BACKSPACE                 269
#define    _TAB                       270
#define    _Q                         271
#define    _W                         272
#define    _E                         273
#define    _R                         274
#define    _T                         275
#define    _Y                         276
#define    _U                         277
#define    _I                         278
#define    _O                         279
#define    _P                         280
#define    _LBRACKET                  281
#define    _RBRACKET                  282
#define    _ENTER                     283
#define    _CONTROLKEY                284         /* only for the menu */
#define    _A                         285
#define    _S                         286
#define    _D                         287
#define    _F                         288
#define    _G                         289
#define    _H                         290
#define    _J                         291
#define    _K                         292
#define    _L                         293
#define    _SEMICOLON                 294
#define    _APOSTROPHE                295
#define    _BACKQUOTE                 296
#define    _SHIFTKEY                  297         /* only for the menu */
#define    _BACKSLASH                 298
#define    _Z                         299
#define    _X                         300
#define    _C                         301
#define    _V                         302
#define    _B                         303
#define    _N                         304
#define    _M                         305
#define    _COMMA                     306
#define    _PERIOD                    307
#define    _SLASH                     308
#define    _CONTROL_BREAK             309         /* re-mapped right shift */
#define    _GREY_STAR                 310
#define    _ALTKEY                    311         /* only for the menu */
#define    _SPACEBAR                  312
#define    _CAPSLOCK                  313         /* unused */
#define    _F1                        314
#define    _F2                        315
#define    _F3                        316
#define    _F4                        317
#define    _F5                        318
#define    _F6                        319
#define    _F7                        320
#define    _F8                        321
#define    _F9                        322
#define    _F10                       323
#define    _GREY_ENTER                324         /* re-mapped NUMLOCK */
#define    _GREY_SLASH                325         /* re-mapped SCROLLOCK */
#define    _HOME                      326
#define    _UP                        327
#define    _PGUP                      328
#define    _GREY_MINUS                329
#define    _LEFT                      330
#define    _CENTER                    331
#define    _RIGHT                     332
#define    _GREY_PLUS                 333
#define    _END                       334
#define    _DOWN                      335
#define    _PGDN                      336
#define    _INS                       337
#define    _DEL                       338
#define    _PRTSC                     339
#define    _FAKE_F11                  340         /* unused */
#define    _LEFT_BACKSLASH            341         /* 102-key keyboards */
#define    _F11                       342
#define    _F12                       343

#define    _SHIFT                     512
#define    _CTRL                     1024
#define    _ALT                      2048
#define    _FUNCTION                 4096         /* a function, not a key */
