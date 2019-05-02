/*
 * New editor name:  TDE, the Thomson-Davis Editor.
 * Author:           Frank Davis
 * Date:             June 5, 1991
 *
 * This modification of Douglas Thomson's code is released into the
 * public domain, Frank Davis.  You may distribute it freely.
 *
 * Set up default dispatch table.
 *
 * If you want to change the default key assignments - it's really easy.  All
 * you have to do is set the appropriate array element to the desired function
 * and then recompile the source code.  The available functions are in the
 * define.h file.
 *
 * If you change the default key assignments, you should also change the help
 * screen to show what function the new keys are assigned to.
 *
 * The key code returned by pressing a function key is added to 256.  This is
 * done because it makes it easy to allow the ASCII and Extended ASCII
 * characters to get thru as normal characters and to trap the function keys
 * because they are greater than 256.  See function getfunc( ) in query.c.
 *
 * jmh 980525: made it a C file.
 * jmh 980727: added a function to create default two-key combinations.
 * jmh 020816: totally modified the key layout.  Keys are based on scan
 *              codes, also allowing the viewer keys to be mapped directly.
 */

#include "tdestr.h"
#include "define.h"
#include "tdefunc.h"


KEY_FUNC key_func = {
{ /* Unshifted keys */
     RestoreLine,               /* Esc */
     0,                         /* 1 */
     0,                         /* 2 */
     0,                         /* 3 */
     0,                         /* 4 */
     0,                         /* 5 */
     0,                         /* 6 */
     0,                         /* 7 */
     0,                         /* 8 */
     0,                         /* 9 */
     0,                         /* 0 */
     0,                         /* - */
     Status,                    /* = */
     BackSpace,                 /* Backspace */
     Tab,                       /* Tab */
     Quit,                      /* q */
     0,                         /* w */
     PanDn,                     /* e */
     Repeat,                    /* r */
     0,                         /* t */
     PanUp,                     /* y */
     HalfScreenUp,              /* u */
     0,                         /* i */
     0,                         /* o */
     0,                         /* p */
     HalfScreenLeft,            /* [ */
     HalfScreenRight,           /* ] */
     Rturn,                     /* Enter */
     0,                         /* (Control) */
     0,                         /* a */
     UserScreen,                /* s */
     HalfScreenDown,            /* d */
     ScreenDown,                /* f */
     JumpToPosition,            /* g */
     PanLeft,                   /* h */
     PanDn,                     /* j */
     PanUp,                     /* k */
     PanRight,                  /* l */
     0,                         /* ; */
     TwoCharKey,                /* ' */
     PreviousPosition,          /* ` */
     0,                         /* (LeftShift) */
     0,                         /* \ */
     0,                         /* z */
     0,                         /* x */
     0,                         /* c */
     ToggleReadOnly,            /* v */
     ScreenUp,                  /* b */
     RepeatRegXForward,         /* n */
     TwoCharKey,                /* m */
     RepeatFindBackward,        /* , */
     RepeatFindForward,         /* . */
     RegXForward,               /* / */
     0,                         /* (RightShift) */
     0,                         /* Grey* */
     0,                         /* (Alt) */
     ScreenDown,                /* SpaceBar */
     0,                         /* (CapsLock) */
     Help,                      /* F1 */
     Save,                      /* F2 */
     Quit,                      /* F3 */
     File,                      /* F4 */
     RepeatFindForward,         /* F5 */
     RepeatFindBackward,        /* F6 */
     RepeatRegXForward,         /* F7 */
     SplitVertical,             /* F8 */
     SplitHorizontal,           /* F9 */
     NextWindow,                /* F10 */
     Rturn,                     /* GreyEnter */
     0,                         /* Grey/ */
     BegOfLine,                 /* Home */
     LineUp,                    /* Up */
     ScreenUp,                  /* PgUp */
     ScrollUpLine,              /* Grey- */
     CharLeft,                  /* Left */
     CenterWindow,              /* Center (KeyPad5) */
     CharRight,                 /* Right */
     ScrollDnLine,              /* Grey+ */
     EndOfLine,                 /* End */
     LineDown,                  /* Down */
     ScreenDown,                /* PgDn */
     ToggleOverWrite,           /* Ins */
     DeleteChar,                /* Del */
     0,                         /* (PrtSc) */
     0,                         /* (F11Alt) */
     0,                         /* Left\ */
     RepeatDiff,                /* F11 */
     RepeatGrep,                /* F12 */
},
{ /* Shift keys */
     SetBreakPoint,             /* Shift+Esc */
     Shell,                     /* ! */
     0,                         /* @ */
     0,                         /* # */
     0,                         /* $ */
     0,                         /* % */
     0,                         /* ^ */
     0,                         /* & */
     0,                         /* * */
     0,                         /* ( */
     0,                         /* ) */
     0,                         /* _ */
     0,                         /* + */
     BackSpace,                 /* Shift+Backspace */
     BackTab,                   /* Shift+Tab */
     QuitAll,                   /* Q */
     0,                         /* W */
     EditFile,                  /* E */
     RedrawScreen,              /* R */
     0,                         /* T */
     0,                         /* Y */
     0,                         /* U */
     0,                         /* I */
     0,                         /* O */
     0,                         /* P */
     ScreenLeft,                /* { */
     ScreenRight,               /* } */
     NextLine,                  /* Shift+Enter */
     0,                         /* (Shift+Control) */
     0,                         /* A */
     0,                         /* S */
     0,                         /* D */
     EndOfFile,                 /* F */
     EndOfFile,                 /* G */
     Help,                      /* H */
     0,                         /* J */
     0,                         /* K */
     0,                         /* L */
     TwoCharKey,                /* : */
     0,                         /* " */
     0,                         /* ~ */
     0,                         /* (Shift+LeftShift) */
     0,                         /* | */
     0,                         /* Z */
     0,                         /* X */
     0,                         /* C */
     0,                         /* V */
     TopOfFile,                 /* B */
     RepeatRegXBackward,        /* N */
     0,                         /* M */
     FindBackward,              /* < */
     FindForward,               /* > */
     RegXBackward,              /* ? */
     0,                         /* (Shift+RightShift) */
     0,                         /* Shift+Grey* */
     0,                         /* (Shift+Alt) */
     0,                         /* Shift+SpaceBar */
     0,                         /* (Shift+CapsLock) */
     SaveMacro,                 /* Shift+F1 */
     SaveAs,                    /* Shift+F2 */
     LoadMacro,                 /* Shift+F3 */
     EditFile,                  /* Shift+F4 */
     FindForward,               /* Shift+F5 */
     FindBackward,              /* Shift+F6 */
     RegXForward,               /* Shift+F7 */
     ReplaceString,             /* Shift+F8 */
     SizeWindow,                /* Shift+F9 */
     PreviousWindow,            /* Shift+F10 */
     Statistics,                /* Shift+GreyEnter */
     0,                         /* Shift+Grey/ */
     StartOfLine,               /* Shift+Home */
     LineUp,                    /* Shift+Up */
     HalfScreenUp,              /* Shift+PgUp */
     ScrollUpLine,              /* Shift+Grey- */
#if defined( __UNIX__ )
     WordLeft,                  /* Shift+Left */
     CenterWindow,              /* Shift+Center */
     WordRight,                 /* Shift+Right */
#else
     StreamCharLeft,            /* Shift+Left */
     CenterWindow,              /* Shift+Center */
     StreamCharRight,           /* Shift+Right */
#endif
     ScrollDnLine,              /* Shift+Grey+ */
     EndOfLine,                 /* Shift+End */
     LineDown,                  /* Shift+Down */
     HalfScreenDown,            /* Shift+PgDn */
     PasteFromClipboard,        /* Shift+Ins */
     CutToClipboard,            /* Shift+Del */
     0,                         /* (Shift+PrtSc) */
     0,                         /* (Alternative Shift+F11) */
     0,                         /* Left| */
     DefineDiff,                /* Shift+F11 */
     DefineGrep,                /* Shift+F12 */
},
{ /* Ctrl keys */
     ClearAllMacros,            /* Ctrl+Esc */
     0,                         /* Ctrl+1 */
     DateTimeStamp,             /* Ctrl+2 */
     0,                         /* Ctrl+3 */
     0,                         /* Ctrl+4 */
     0,                         /* Ctrl+5 */
#if 0 //defined( __UNIX__ )
     UnMarkBlock,               /* Ctrl+6 */
#else
     RedrawScreen,              /* Ctrl+6 */
#endif
     0,                         /* Ctrl+7 */
     0,                         /* Ctrl+8 */
     0,                         /* Ctrl+9 */
     0,                         /* Ctrl+0 */
     SplitLine,                 /* Ctrl+- */
     PopupRuler,                /* Ctrl+= */
     WordDeleteBack,            /* Ctrl+Backspace */
     SetTabs,                   /* Ctrl+Tab */
#if 0 //defined( __UNIX__ )
     ToggleEol,                 /* Ctrl+Q */
     ToggleSync,                /* Ctrl+W */
     BlockExpandTabs,           /* Ctrl+E */
     RecordMacro,               /* Ctrl+R */
     ToggleTabInflate,          /* Ctrl+T */
     DeleteLine,                /* Ctrl+Y */
     Undo,                      /* Ctrl+U */
     Tab,                       /* Ctrl+I */
     OverlayBlock,              /* Ctrl+O */
     RedrawScreen,              /* Ctrl+P */
     AbortCommand,              /* Ctrl+[ */
     ParenBalance,              /* Ctrl+] */
     BegNextLine,               /* Ctrl+Enter */
     0,                         /* (Ctrl+Control) */
     AddLine,                   /* Ctrl+A */
     SortBoxBlock,              /* Ctrl+S */
     DeleteLine,                /* Ctrl+D */
     FillBlock,                 /* Ctrl+F */
     DeleteBlock,               /* Ctrl+G */
     BackSpace,                 /* Ctrl+H */
     MoveBlock,                 /* Ctrl+J */
     TwoCharKey,                /* Ctrl+K */
     MarkLine,                  /* Ctrl+L */
     0,                         /* Ctrl+; */
     0,                         /* Ctrl+' */
     0,                         /* Ctrl+` */
     0,                         /* (Ctrl+LeftShift) */
     PullDown,                  /* Ctrl+\ */
     ToggleTrailing,            /* Ctrl+Z */
     MarkStream,                /* Ctrl+X */
     CopyBlock,                 /* Ctrl+C */
     ToggleOverWrite,           /* Ctrl+V */
     MarkBox,                   /* Ctrl+B */
     NumberBlock,               /* Ctrl+N */
     Rturn,                     /* Ctrl+M */
#else
     FileAll,                   /* Ctrl+Q */
     ScrollUpLine,              /* Ctrl+W */
     LineUp,                    /* Ctrl+E */
     ScreenUp,                  /* Ctrl+R */
     WordDelete,                /* Ctrl+T */
     DeleteLine,                /* Ctrl+Y */
     RetrieveLine,              /* Ctrl+U */
     Tab,                       /* Ctrl+I */
     0,                         /* Ctrl+O */
     Pause,                     /* Ctrl+P */
     AbortCommand,              /* Ctrl+[ */
     ParenBalance,              /* Ctrl+] */
     BegNextLine,               /* Ctrl+Enter */
     0,                         /* (Ctrl+Control) */
     WordLeft,                  /* Ctrl+A */
     CharLeft,                  /* Ctrl+S */
     CharRight,                 /* Ctrl+D */
     WordRight,                 /* Ctrl+F */
     DeleteChar,                /* Ctrl+G */
     BackSpace,                 /* Ctrl+H */
     Help,                      /* Ctrl+J */
     TwoCharKey,                /* Ctrl+K */
     ToggleSyntax,              /* Ctrl+L */
     0,                         /* Ctrl+; */
     0,                         /* Ctrl+' */
     TitleWindow,               /* Ctrl+` */
     0,                         /* (Ctrl+LeftShift) */
     PullDown,                  /* Ctrl+\ */
     ScrollDnLine,              /* Ctrl+Z */
     LineDown,                  /* Ctrl+X */
     ScreenDown,                /* Ctrl+C */
     ToggleOverWrite,           /* Ctrl+V */
     FormatText,                /* Ctrl+B */
     AddLine,                   /* Ctrl+N */
     Rturn,                     /* Ctrl+M */
#endif
     0,                         /* Ctrl+, */
     0,                         /* Ctrl+. */
     ToggleCWD,                 /* Ctrl+/ */
     0,                         /* (Ctrl+RightShift) */
     0,                         /* Ctrl+Grey* */
     0,                         /* (Ctrl+Alt) */
     PseudoMacro,               /* Ctrl+SpaceBar */
     0,                         /* (Ctrl+CapsLock) */
     ToggleSync,                /* Ctrl+F1 */
     ToggleEol,                 /* Ctrl+F2 */
     ToggleCRLF,                /* Ctrl+F3 */
     ToggleTrailing,            /* Ctrl+F4 */
     ToggleSearchCase,          /* Ctrl+F5 */
     SetMargins,                /* Ctrl+F6 */
     NextBrowse,                /* Ctrl+F7 */
     PrevBrowse,                /* Ctrl+F8 */
     ZoomWindow,                /* Ctrl+F9 */
     NextHiddenWindow,          /* Ctrl+F10 */
     Status,                    /* Ctrl+GreyEnter */
     0,                         /* Ctrl+Grey/ */
     TopOfFile,                 /* Ctrl+Home */
     ScrollUpLine,              /* Ctrl+Up */
     TopOfScreen,               /* Ctrl+PgUp */
     PanUp,                     /* Ctrl+Grey- */
     WordLeft,                  /* Ctrl+Left */
     CenterLine,                /* Ctrl+Center */
     WordRight,                 /* Ctrl+Right */
     PanDn,                     /* Ctrl+Grey+ */
     EndOfFile,                 /* Ctrl+End */
     ScrollDnLine,              /* Ctrl+Down */
     BotOfScreen,               /* Ctrl+PgDn */
     CopyToClipboard,           /* Ctrl+Ins */
     StreamDeleteChar,          /* Ctrl+Del */
     0,                         /* Ctrl+PrtSc */
     0,                         /* (Ctrl+F11Alt) */
     0,                         /* Ctrl+Left\ */
     GotoWindow,                /* Ctrl+F11 */
     DefineGrep,                /* Ctrl+F12 */
},
{ /* Shift+Ctrl keys */
     0,                         /* Shift+Ctrl+Esc */
     0,                         /* Shift+Ctrl+1 */
     0,                         /* Shift+Ctrl+2 */
     0,                         /* Shift+Ctrl+3 */
     0,                         /* Shift+Ctrl+4 */
     0,                         /* Shift+Ctrl+5 */
     0,                         /* Shift+Ctrl+6 */
     0,                         /* Shift+Ctrl+7 */
     0,                         /* Shift+Ctrl+8 */
     0,                         /* Shift+Ctrl+9 */
     0,                         /* Shift+Ctrl+0 */
     EraseBegOfLine,            /* Shift+Ctrl+- */
     0,                         /* Shift+Ctrl+= */
     0,                         /* Shift+Ctrl+Backspace */
     DynamicTabSize,            /* Shift+Ctrl+Tab */
     QuitAll,                   /* Shift+Ctrl+Q */
     0,                         /* Shift+Ctrl+W */
     BlockUnComment,            /* Shift+Ctrl+E */
     ISearchBackward,           /* Shift+Ctrl+R */
     Transpose,                 /* Shift+Ctrl+T */
     0,                         /* Shift+Ctrl+Y */
     UserScreen,                /* Shift+Ctrl+U */
     ToggleSmartTabs,           /* Shift+Ctrl+I */
     0,                         /* Shift+Ctrl+O */
     Shell,                     /* Shift+Ctrl+P */
     0,                         /* Shift+Ctrl+[ */
     0,                         /* Shift+Ctrl+] */
     EndNextLine,               /* Shift+Ctrl+Enter */
     0,                         /* (Shift+Ctrl+Control) */
     0,                         /* Shift+Ctrl+A */
     ISearchForward,            /* Shift+Ctrl+S */
     ChangeCurDir,              /* Shift+Ctrl+D */
     FillBlockDown,             /* Shift+Ctrl+F */
     0,                         /* Shift+Ctrl+G */
     BalanceHorizontal,         /* Shift+Ctrl+H */
     0,                         /* Shift+Ctrl+J */
     0,                         /* Shift+Ctrl+K */
     BlockLineComment,          /* Shift+Ctrl+L */
     0,                         /* Shift+Ctrl+; */
     0,                         /* Shift+Ctrl+' */
     0,                         /* Shift+Ctrl+` */
     0,                         /* (Shift+Ctrl+LeftShift) */
     0,                         /* Shift+Ctrl+\ */
     0,                         /* Shift+Ctrl+Z */
     0,                         /* Shift+Ctrl+X */
     BlockBlockComment,         /* Shift+Ctrl+C */
     BalanceVertical,           /* Shift+Ctrl+V */
     FormatParagraph,           /* Shift+Ctrl+B */
     0,                         /* Shift+Ctrl+N */
     ClearAllMacros,            /* Shift+Ctrl+M */
     0,                         /* Shift+Ctrl+, */
     0,                         /* Shift+Ctrl+. */
     0,                         /* Shift+Ctrl+/ */
     0,                         /* (Shift+Ctrl+RightShift) */
     0,                         /* Shift+Ctrl+Grey* */
     0,                         /* (Shift+Ctrl+Alt) */
     0,                         /* Shift+Ctrl+SpaceBar */
     0,                         /* (Shift+Ctrl+CapsLock) */
     ContextHelp,               /* Shift+Ctrl+F1 */
     SaveTo,                    /* Shift+Ctrl+F2 */
     CloseWindow,               /* Shift+Ctrl+F3 */
     ScratchWindow,             /* Shift+Ctrl+F4 */
     DefineSearch,              /* Shift+Ctrl+F5 */
     RepeatSearch,              /* Shift+Ctrl+F6 */
     0,                         /* Shift+Ctrl+F7 */
     MakeVertical,              /* Shift+Ctrl+F8 */
     MakeHorizontal,            /* Shift+Ctrl+F9 */
     PrevHiddenWindow,          /* Shift+Ctrl+F10 */
     0,                         /* Shift+Ctrl+GreyEnter */
     0,                         /* Shift+Ctrl+Grey/ */
     0,                         /* Shift+Ctrl+Home */
     0,                         /* Shift+Ctrl+Up */
     0,                         /* Shift+Ctrl+PgUp */
     0,                         /* Shift+Ctrl+Grey- */
     0,                         /* Shift+Ctrl+Left */
     0,                         /* Shift+Ctrl+Center */
     0,                         /* Shift+Ctrl+Right */
     0,                         /* Shift+Ctrl+Grey+ */
     0,                         /* Shift+Ctrl+End */
     0,                         /* Shift+Ctrl+Down */
     0,                         /* Shift+Ctrl+PgDn */
     KopyToClipboard,           /* Shift+Ctrl+Ins */
     0,                         /* Shift+Ctrl+Del */
     0,                         /* Shift+Ctrl+PrtSc */
     0,                         /* (Shift+Ctrl+F11Alt) */
     0,                         /* Shift+Ctrl+Left\ */
     MakeHalfVertical,          /* Shift+Ctrl+F11 */
     MakeHalfHorizontal,        /* Shift+Ctrl+F12 */
},
{ /* Alt keys */
     0,                         /* Alt+Esc */
     SetMark1,                  /* Alt+1 */
     SetMark2,                  /* Alt+2 */
     SetMark3,                  /* Alt+3 */
     0,                         /* Alt+4 */
     0,                         /* Alt+5 */
     0,                         /* Alt+6 */
     0,                         /* Alt+7 */
     0,                         /* Alt+8 */
     0,                         /* Alt+9 */
     0,                         /* Alt+0 */
     DelEndOfLine,              /* Alt+- */
     DuplicateLine,             /* Alt+= */
     Undo,                      /* Alt+Backspace */
     ToggleSmartTabs,           /* Alt+Tab */
     Quit,                      /* Alt+Q */
     BlockToFile,               /* Alt+W */
     BlockExpandTabs,           /* Alt+E */
     ToggleRuler,               /* Alt+R */
     BlockTrimTrailing,         /* Alt+T */
     RetrieveLine,              /* Alt+Y */
     UnMarkBlock,               /* Alt+U */
     ToggleIndent,              /* Alt+I */
     OverlayBlock,              /* Alt+O */
     PrintBlock,                /* Alt+P */
     BlockBegin,                /* Alt+[ */
     BlockEnd,                  /* Alt+] */
     PseudoMacro,               /* Alt+Enter */
     0,                         /* (Alt+Control) */
     AddLine,                   /* Alt+A */
     SortBoxBlock,              /* Alt+S */
     DeleteLine,                /* Alt+D */
     FillBlock,                 /* Alt+F */
     DeleteBlock,               /* Alt+G */
     Help,                      /* Alt+H */
     JoinLine,                  /* Alt+J */
     KopyBlock,                 /* Alt+K */
     MarkLine,                  /* Alt+L */
     WordEndLeft,               /* Alt+; */
     WordEndRight,              /* Alt+' */
     PreviousPosition,          /* Alt+` */
     0,                         /* (Alt+LeftShift) */
     ToggleReadOnly,            /* Alt+\ */
     ToggleZ,                   /* Alt+Z */
     MarkStream,                /* Alt+X */
     CopyBlock,                 /* Alt+C */
     ToggleWordWrap,            /* Alt+V */
     MarkBox,                   /* Alt+B */
     NumberBlock,               /* Alt+N */
     MoveBlock,                 /* Alt+M */
     BlockUpperCase,            /* Alt+, */
     BlockLowerCase,            /* Alt+. */
     BlockStripHiBit,           /* Alt+/ */
     0,                         /* (Alt+RightShift) */
     0,                         /* Alt+Grey* */
     0,                         /* (Alt+Alt) */
     0,                         /* Alt+SpaceBar */
     0,                         /* (Alt+CapsLock) */
     DirList,                   /* Alt+F1 */
     FileAttributes,            /* Alt+F2 */
     RecordMacro,               /* Alt+F3 */
     EditNextFile,              /* Alt+F4 */
     NextDirtyLine,             /* Alt+F5 */
     PrevDirtyLine,             /* Alt+F6 */
     RepeatRegXBackward,        /* Alt+F7 */
     LeftJustify,               /* Alt+F8 */
     RightJustify,              /* Alt+F9 */
     CenterJustify,             /* Alt+F10 */
     0,                         /* Alt+GreyEnter */
     0,                         /* Alt+Grey/ */
     TopLine,                   /* Alt+Home */
     PanUp,                     /* Alt+Up */
     ScreenLeft,                /* Alt+PgUp */
     0,                         /* Alt+Grey- */
     PanLeft,                   /* Alt+Left */
     0,                         /* Alt+Center */
     PanRight,                  /* Alt+Right */
     SumBlock,                  /* Alt+Grey+ */
     BottomLine,                /* Alt+End */
     PanDn,                     /* Alt+Down */
     ScreenRight,               /* Alt+PgDn */
     ToggleDraw,                /* Alt+Ins */
     0,                         /* Alt+Del */
     0,                         /* Alt+PrtSc */
     0,                         /* (Alt+F11Alt) */
     0,                         /* Alt+Left\ */
     ReadConfig,                /* Alt+F11 */
     RepeatRegXForward,         /* Alt+F12 */
},
{ /* Shift+Alt keys */
     0,                         /* Shift+Alt+Esc */
     GotoMark1,                 /* Shift+Alt+1 */
     GotoMark2,                 /* Shift+Alt+2 */
     GotoMark3,                 /* Shift+Alt+3 */
     0,                         /* Shift+Alt+4 */
     0,                         /* Shift+Alt+5 */
     0,                         /* Shift+Alt+6 */
     0,                         /* Shift+Alt+7 */
     0,                         /* Shift+Alt+8 */
     0,                         /* Shift+Alt+9 */
     0,                         /* Shift+Alt+0 */
     DelBegOfLine,              /* Shift+Alt+- */
     SetDirectory,              /* Shift+Alt+= */
     Redo,                      /* Shift+Alt+Backspace */
     0,                         /* Shift+Alt+Tab */
     DefineGrep,                /* Shift+Alt+Q */
     SaveWorkspace,             /* Shift+Alt+W */
     Execute,                   /* Shift+Alt+E */
     Repeat,                    /* Shift+Alt+R */
     ToggleTabInflate,          /* Shift+Alt+T */
     ToggleUndoMove,            /* Shift+Alt+Y */
     ToggleUndoGroup,           /* Shift+Alt+U */
     BlockIndentTabs,           /* Shift+Alt+I */
     0,                         /* Shift+Alt+O */
     0,                         /* Shift+Alt+P */
     MarkBegin,                 /* Shift+Alt+[ */
     MarkEnd,                   /* Shift+Alt+] */
     0,                         /* Shift+Alt+Enter */
     0,                         /* (Shift+Alt+Control) */
     0,                         /* Shift+Alt+A */
     SwapBlock,                 /* Shift+Alt+S */
     RepeatDiff,                /* Shift+Alt+D */
     StampFormat,               /* Shift+Alt+F */
     ToggleGraphicChars,        /* Shift+Alt+G */
     CharacterSet,              /* Shift+Alt+H */
     JumpToPosition,            /* Shift+Alt+J */
     0,                         /* Shift+Alt+K */
     SyntaxSelect,              /* Shift+Alt+L */
     StringEndLeft,             /* Shift+Alt+; */
     StringEndRight,            /* Shift+Alt+' */
     0,                         /* Shift+Alt+` */
     0,                         /* (Shift+Alt+LeftShift) */
     0,                         /* Shift+Alt+\ */
     0,                         /* Shift+Alt+Z */
     ToggleCursorCross,         /* Shift+Alt+X */
     BlockCompressTabs,         /* Shift+Alt+C */
     0,                         /* Shift+Alt+V */
     BorderBlock,               /* Shift+Alt+B */
     ToggleLineNumbers,         /* Shift+Alt+N */
     MacroMark,                 /* Shift+Alt+M */
     BlockRot13,                /* Shift+Alt+, */
     BlockFixUUE,               /* Shift+Alt+. */
     BlockEmailReply,           /* Shift+Alt+/ */
     0,                         /* (Shift+Alt+RightShift) */
     0,                         /* Shift+Alt+Grey* */
     0,                         /* (Shift+Alt+Alt) */
     0,                         /* Shift+Alt+SpaceBar */
     0,                         /* (Shift+Alt+CapsLock) */
     0,                         /* Shift+Alt+F1 */
     SaveUntouched,             /* Shift+Alt+F2 */
     0,                         /* Shift+Alt+F3 */
     Revert,                    /* Shift+Alt+F4 */
     0,                         /* Shift+Alt+F5 */
     0,                         /* Shift+Alt+F6 */
     RegXBackward,              /* Shift+Alt+F7 */
     BlockLeftJustify,          /* Shift+Alt+F8 */
     BlockRightJustify,         /* Shift+Alt+F9 */
     BlockCenterJustify,        /* Shift+Alt+F10 */
     0,                         /* Shift+Alt+GreyEnter */
     0,                         /* Shift+Alt+Grey/ */
     0,                         /* Shift+Alt+Home */
     0,                         /* Shift+Alt+Up */
     HalfScreenLeft,            /* Shift+Alt+PgUp */
     0,                         /* Shift+Alt+Grey- */
     0,                         /* Shift+Alt+Left */
     0,                         /* Shift+Alt+Center */
     0,                         /* Shift+Alt+Right */
     0,                         /* Shift+Alt+Grey+ */
     0,                         /* Shift+Alt+End */
     0,                         /* Shift+Alt+Down */
     HalfScreenRight,           /* Shift+Alt+PgDn */
     InsertFile,                /* Shift+Alt+Ins */
     0,                         /* Shift+Alt+Del */
     0,                         /* (Shift+Alt+PrtSc) */
     0,                         /* (Shift+Alt+F11Alt) */
     0,                         /* Shift+Alt+Left\ */
     SplitHalfVertical,         /* Shift+Alt+F11 */
     SplitHalfHorizontal,       /* Shift+Alt+F12 */
},
{ /* Ctrl+Alt keys */
     0,                         /* Ctrl+Alt+Esc */
     0,                         /* Ctrl+Alt+1 */
     0,                         /* Ctrl+Alt+2 */
     0,                         /* Ctrl+Alt+3 */
     0,                         /* Ctrl+Alt+4 */
     0,                         /* Ctrl+Alt+5 */
     0,                         /* Ctrl+Alt+6 */
     0,                         /* Ctrl+Alt+7 */
     0,                         /* Ctrl+Alt+8 */
     0,                         /* Ctrl+Alt+9 */
     0,                         /* Ctrl+Alt+0 */
     0,                         /* Ctrl+Alt+- */
     0,                         /* Ctrl+Alt+= */
     0,                         /* Ctrl+Alt+Backspace */
     0,                         /* Ctrl+Alt+Tab */
     ToggleQuickEdit,           /* Ctrl+Alt+Q */
     0,                         /* Ctrl+Alt+W */
     0,                         /* Ctrl+Alt+E */
     0,                         /* Ctrl+Alt+R */
     0,                         /* Ctrl+Alt+T */
     0,                         /* Ctrl+Alt+Y */
     0,                         /* Ctrl+Alt+U */
     0,                         /* Ctrl+Alt+I */
     0,                         /* Ctrl+Alt+O */
     0,                         /* Ctrl+Alt+P */
     MoveMark,                  /* Ctrl+Alt+[ */
     0,                         /* Ctrl+Alt+] */
     0,                         /* Ctrl+Alt+Enter */
     0,                         /* (Ctrl+Alt+Control) */
     0,                         /* Ctrl+Alt+A */
     0,                         /* Ctrl+Alt+S */
     0,                         /* Ctrl+Alt+D */
     FillBlockPattern,          /* Ctrl+Alt+F */
     0,                         /* Ctrl+Alt+G */
     0,                         /* Ctrl+Alt+H */
     0,                         /* Ctrl+Alt+J */
     0,                         /* Ctrl+Alt+K */
     0,                         /* Ctrl+Alt+L */
     0,                         /* Ctrl+Alt+; */
     0,                         /* Ctrl+Alt+' */
     BlockInvertCase,           /* Ctrl+Alt+` */
     0,                         /* (Ctrl+Alt+LeftShift) */
     0,                         /* Ctrl+Alt+\ */
     0,                         /* Ctrl+Alt+Z */
     0,                         /* Ctrl+Alt+X */
     0,                         /* Ctrl+Alt+C */
     0,                         /* Ctrl+Alt+V */
     BorderBlockEx,             /* Ctrl+Alt+B */
     0,                         /* Ctrl+Alt+N */
     0,                         /* Ctrl+Alt+M */
     0,                         /* Ctrl+Alt+, */
     BlockCapitalise,           /* Ctrl+Alt+. */
     0,                         /* Ctrl+Alt+/ */
     0,                         /* (Ctrl+Alt+RightShift) */
     0,                         /* Ctrl+Alt+Grey* */
     0,                         /* (Ctrl+Alt+Alt) */
     0,                         /* Ctrl+Alt+SpaceBar */
     0,                         /* (Ctrl+Alt+CapsLock) */
     0,                         /* Ctrl+Alt+F1 */
     SaveAll,                   /* Ctrl+Alt+F2 */
     0,                         /* Ctrl+Alt+F3 */
     0,                         /* Ctrl+Alt+F4 */
     0,                         /* Ctrl+Alt+F5 */
     0,                         /* Ctrl+Alt+F6 */
     0,                         /* Ctrl+Alt+F7 */
     0,                         /* Ctrl+Alt+F8 */
     0,                         /* Ctrl+Alt+F9 */
     0,                         /* Ctrl+Alt+F10 */
     0,                         /* Ctrl+Alt+GreyEnter */
     0,                         /* Ctrl+Alt+Grey/ */
     0,                         /* Ctrl+Alt+Home */
     0,                         /* Ctrl+Alt+Up */
     0,                         /* Ctrl+Alt+PgUp */
     0,                         /* Ctrl+Alt+Grey- */
     0,                         /* Ctrl+Alt+Left */
     0,                         /* Ctrl+Alt+Center */
     0,                         /* Ctrl+Alt+Right */
     0,                         /* Ctrl+Alt+Grey+ */
     0,                         /* Ctrl+Alt+End */
     0,                         /* Ctrl+Alt+Down */
     0,                         /* Ctrl+Alt+PgDn */
     0,                         /* Ctrl+Alt+Ins */
     0,                         /* Ctrl+Alt+Del */
     0,                         /* (Ctrl+Alt+PrtSc) */
     0,                         /* (Ctrl+Alt+F11Alt) */
     0,                         /* Ctrl+Alt+Left\ */
     0,                         /* Ctrl+Alt+F11 */
     0,                         /* Ctrl+Alt+F12 */
},
{ /* Shift+Ctrl+Alt keys */
     0,                         /* Shift+Ctrl+Alt+Esc */
     0,                         /* Shift+Ctrl+Alt+1 */
     0,                         /* Shift+Ctrl+Alt+2 */
     0,                         /* Shift+Ctrl+Alt+3 */
     0,                         /* Shift+Ctrl+Alt+4 */
     0,                         /* Shift+Ctrl+Alt+5 */
     0,                         /* Shift+Ctrl+Alt+6 */
     0,                         /* Shift+Ctrl+Alt+7 */
     0,                         /* Shift+Ctrl+Alt+8 */
     0,                         /* Shift+Ctrl+Alt+9 */
     0,                         /* Shift+Ctrl+Alt+0 */
     0,                         /* Shift+Ctrl+Alt+- */
     0,                         /* Shift+Ctrl+Alt+= */
     0,                         /* Shift+Ctrl+Alt+Backspace */
     0,                         /* Shift+Ctrl+Alt+Tab */
     0,                         /* Shift+Ctrl+Alt+Q */
     0,                         /* Shift+Ctrl+Alt+W */
     0,                         /* Shift+Ctrl+Alt+E */
     0,                         /* Shift+Ctrl+Alt+R */
     0,                         /* Shift+Ctrl+Alt+T */
     0,                         /* Shift+Ctrl+Alt+Y */
     0,                         /* Shift+Ctrl+Alt+U */
     0,                         /* Shift+Ctrl+Alt+I */
     0,                         /* Shift+Ctrl+Alt+O */
     0,                         /* Shift+Ctrl+Alt+P */
     0,                         /* Shift+Ctrl+Alt+[ */
     0,                         /* Shift+Ctrl+Alt+] */
     0,                         /* Shift+Ctrl+Alt+Enter */
     0,                         /* (Shift+Ctrl+Alt+Control) */
     0,                         /* Shift+Ctrl+Alt+A */
     0,                         /* Shift+Ctrl+Alt+S */
     0,                         /* Shift+Ctrl+Alt+D */
     0,                         /* Shift+Ctrl+Alt+F */
     0,                         /* Shift+Ctrl+Alt+G */
     0,                         /* Shift+Ctrl+Alt+H */
     0,                         /* Shift+Ctrl+Alt+J */
     0,                         /* Shift+Ctrl+Alt+K */
     0,                         /* Shift+Ctrl+Alt+L */
     0,                         /* Shift+Ctrl+Alt+; */
     0,                         /* Shift+Ctrl+Alt+' */
     0,                         /* Shift+Ctrl+Alt+` */
     0,                         /* (Shift+Ctrl+Alt+LeftShift) */
     0,                         /* Shift+Ctrl+Alt+\ */
     0,                         /* Shift+Ctrl+Alt+Z */
     0,                         /* Shift+Ctrl+Alt+X */
     0,                         /* Shift+Ctrl+Alt+C */
     0,                         /* Shift+Ctrl+Alt+V */
     0,                         /* Shift+Ctrl+Alt+B */
     0,                         /* Shift+Ctrl+Alt+N */
     0,                         /* Shift+Ctrl+Alt+M */
     0,                         /* Shift+Ctrl+Alt+, */
     0,                         /* Shift+Ctrl+Alt+. */
     0,                         /* Shift+Ctrl+Alt+/ */
     0,                         /* (Shift+Ctrl+Alt+RightShift) */
     0,                         /* Shift+Ctrl+Alt+Grey* */
     0,                         /* (Shift+Ctrl+Alt+Alt) */
     0,                         /* Shift+Ctrl+Alt+SpaceBar */
     0,                         /* (Shift+Ctrl+Alt+CapsLock) */
     0,                         /* Shift+Ctrl+Alt+F1 */
     0,                         /* Shift+Ctrl+Alt+F2 */
     0,                         /* Shift+Ctrl+Alt+F3 */
     0,                         /* Shift+Ctrl+Alt+F4 */
     0,                         /* Shift+Ctrl+Alt+F5 */
     0,                         /* Shift+Ctrl+Alt+F6 */
     0,                         /* Shift+Ctrl+Alt+F7 */
     0,                         /* Shift+Ctrl+Alt+F8 */
     0,                         /* Shift+Ctrl+Alt+F9 */
     0,                         /* Shift+Ctrl+Alt+F10 */
     0,                         /* Shift+Ctrl+Alt+GreyEnter */
     0,                         /* Shift+Ctrl+Alt+Grey/ */
     0,                         /* Shift+Ctrl+Alt+Home */
     0,                         /* Shift+Ctrl+Alt+Up */
     0,                         /* Shift+Ctrl+Alt+PgUp */
     0,                         /* Shift+Ctrl+Alt+Grey- */
     0,                         /* Shift+Ctrl+Alt+Left */
     0,                         /* Shift+Ctrl+Alt+Center */
     0,                         /* Shift+Ctrl+Alt+Right */
     0,                         /* Shift+Ctrl+Alt+Grey+ */
     0,                         /* Shift+Ctrl+Alt+End */
     0,                         /* Shift+Ctrl+Alt+Down */
     0,                         /* Shift+Ctrl+Alt+PgDn */
     0,                         /* Shift+Ctrl+Alt+Ins */
     0,                         /* Shift+Ctrl+Alt+Del */
     0,                         /* (Shift+Ctrl+Alt+PrtSc) */
     0,                         /* (Shift+Ctrl+Alt+F11Alt) */
     0,                         /* Shift+Ctrl+Alt+Left\ */
     0,                         /* Shift+Ctrl+Alt+F11 */
     0,                         /* Shift+Ctrl+Alt+F12 */
}
};


/*
 * Default macro definitions for the above PlayBack keys.
 */
#define F(func) func | _FUNCTION
static long win_one[] = { F(GotoWindow), '1' }; /* :X */
#undef F

static MACRO colon_X = { { 1, 1, 1 }, 0, 2, { 0 } };

/*
 * jmh 991025: allocate memory for the macros to avoid clearing problems.
 *             Assume more than one key and memory is available.
 * jmh 991108: Can I tell if a pointer references static memory or
 *              allocated memory?
 */
static MACRO *allocate_macro( MACRO *mac, long *keys )
{
MACRO *new_mac;
int len;
int rc = OK;

   len = mac->len * sizeof(long);
   mac->key.keys = my_malloc( len, &rc );
   memcpy( mac->key.keys, keys, len );
   new_mac = my_malloc( sizeof(MACRO), &rc );
   memcpy( new_mac, mac, sizeof(MACRO) );

   return( new_mac );
}


/*
 * Name:    default_twokeys
 * Class:   initialization (only called in initialize())
 * Purpose: set-up default two-key assignments
 * Author:  Jason Hood
 * Date:    July 27, 1998
 * Notes:   Since I dynamically allocate two-keys now, they can no longer
 *           be stored directly in the executable. This function is called
 *           during initialization to create default two-key combinations.
 *          Requires the parent key to be TwoCharKey in above array.
 *          Assumes there's enough heap, since it's done before the config
 *           file. If an error _is_ generated, then TDE16 probably needs to
 *           go to the large model.
 *
 * 990428:  add the viewer macros/two-keys.
 */
extern TREE key_tree;           /* global.c (common.h is not included here) */
extern TREE *cfg_key_tree;      /* config.c */
extern MACRO *macro[MODIFIERS][MAX_KEYS]; /* global.c */

void default_twokeys( void )
{
TREE *twokey;

   cfg_key_tree = &key_tree;

   add_twokey( CREATE_TWOKEY( _CTRL+_K, _I ), BlockIndentN );
   add_twokey( CREATE_TWOKEY( _CTRL+_K, _U ), BlockUndentN );
   add_twokey( CREATE_TWOKEY( _CTRL+_K, _CTRL+_I ), BlockIndent );
   add_twokey( CREATE_TWOKEY( _CTRL+_K, _CTRL+_U ), BlockUndent );
#if defined( __UNIX__ )
   add_twokey( CREATE_TWOKEY( _CTRL+_K, _Q ), Quit );
   add_twokey( CREATE_TWOKEY( _CTRL+_K, _N ), ReadConfig );
   add_twokey( CREATE_TWOKEY( _CTRL+_K, _B ), MarkBegin );
   add_twokey( CREATE_TWOKEY( _CTRL+_K, _K ), MarkEnd );
   add_twokey( CREATE_TWOKEY( _CTRL+_K, _CTRL+_B ), BlockBegin );
   add_twokey( CREATE_TWOKEY( _CTRL+_K, _CTRL+_K ), BlockEnd );
   add_twokey( CREATE_TWOKEY( _CTRL+_K, _SHIFT+_1 ), SetMark1 );
   add_twokey( CREATE_TWOKEY( _CTRL+_K, _SHIFT+_2 ), SetMark2 );
   add_twokey( CREATE_TWOKEY( _CTRL+_K, _SHIFT+_3 ), SetMark3 );
   add_twokey( CREATE_TWOKEY( _CTRL+_K, _1 ), GotoMark1 );
   add_twokey( CREATE_TWOKEY( _CTRL+_K, _2 ), GotoMark2 );
   add_twokey( CREATE_TWOKEY( _CTRL+_K, _3 ), GotoMark3 );
   add_twokey( CREATE_TWOKEY( _CTRL+_K, _M ), MacroMark );
   add_twokey( CREATE_TWOKEY( _CTRL+_K, _P ), Pause );
#endif

   add_twokey( CREATE_TWOKEY( _M, _1 ), SetMark1 );
   add_twokey( CREATE_TWOKEY( _M, _2 ), SetMark2 );
   add_twokey( CREATE_TWOKEY( _M, _3 ), SetMark3 );
   add_twokey( CREATE_TWOKEY( _APOSTROPHE, _APOSTROPHE ), PreviousPosition );
   add_twokey( CREATE_TWOKEY( _APOSTROPHE, _1 ),  GotoMark1 );
   add_twokey( CREATE_TWOKEY( _APOSTROPHE, _2 ),  GotoMark2 );
   add_twokey( CREATE_TWOKEY( _APOSTROPHE, _3 ),  GotoMark3 );
   add_twokey( CREATE_TWOKEY( _APOSTROPHE, _SHIFT+_6 ),  TopOfFile );
   add_twokey( CREATE_TWOKEY( _APOSTROPHE, _SHIFT+_4 ),  EndOfFile );
   add_twokey( CREATE_TWOKEY( _SHIFT+_SEMICOLON, _N ),  NextHiddenWindow );
   add_twokey( CREATE_TWOKEY( _SHIFT+_SEMICOLON, _P ),  PrevHiddenWindow );
   add_twokey( CREATE_TWOKEY( _SHIFT+_SEMICOLON, _E ),  EditFile );
   add_twokey( CREATE_TWOKEY( _SHIFT+_SEMICOLON, _F ),  Status );
   add_twokey( CREATE_TWOKEY( _SHIFT+_SEMICOLON, _Q ),  Quit );
   add_twokey( CREATE_TWOKEY( _SHIFT+_SEMICOLON, _SHIFT+_Q ),  QuitAll );
   add_twokey( CREATE_TWOKEY( _SHIFT+_SEMICOLON, _X ),  GotoWindow );
   add_twokey( CREATE_TWOKEY( _SHIFT+_SEMICOLON, _SHIFT+_X ),  PlayBack );
   twokey = search_tree( CREATE_TWOKEY( _SHIFT+_SEMICOLON, _SHIFT+_X ),
                         key_tree.right );
   twokey->macro = allocate_macro( &colon_X, win_one );
}
