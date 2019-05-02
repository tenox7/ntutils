/*
 * This file contains all the strings required to parse the configuration and
 * syntax highlighting files.
 */

#include "tdestr.h"
#include "config.h"
#include "syntax.h"
#include "define.h"

/*
 * Group the list of available keys alphabetically.
 * jmh 991028: use the key defines.
 * jmh 020818: only store the key name; the prefixes can be tested separately.
 * jmh 020903: removed the shift prefix from the viewer keys (eg: "1" & "!" are
 *              equivalent, use "s+1" or "s+!" if that's what you mean).
 * jmh 021008: renamed backspace to bkspace, tab to tabkey.
 */

const CONFIG_DEFS valid_keys[AVAIL_KEYS] = {
/* available key     index into file   */
   { " ",           _SPACEBAR       },
   { "!",           _1              },
   { "\"",          _APOSTROPHE     },
   { "#",           _3              },
   { "$",           _4              },
   { "%",           _5              },
   { "&",           _7              },
   { "\'",          _APOSTROPHE     },
   { "(",           _9              },
   { ")",           _0              },
   { "*",           _8              },
   { "+",           _EQUALS         },
   { ",",           _COMMA          },
   { "-",           _MINUS          },
   { ".",           _PERIOD         },
   { "/",           _SLASH          },
   { "0",           _0              },
   { "1",           _1              },
   { "2",           _2              },
   { "3",           _3              },
   { "4",           _4              },
   { "5",           _5              },
   { "6",           _6              },
   { "7",           _7              },
   { "8",           _8              },
   { "9",           _9              },
   { ":",           _SEMICOLON      },
   { ";",           _SEMICOLON      },
   { "<",           _COMMA          },
   { "=",           _EQUALS         },
   { ">",           _PERIOD         },
   { "?",           _SLASH          },
   { "@",           _2              },
#ifndef __TURBOC__                              /* It seems Borland (BC3.1) */
   { "[",           _LBRACKET       },          /* converts to uppercase in */
   { "\\",          _BACKSLASH      },          /* stricmp, meaning these   */
   { "]",           _RBRACKET       },          /* have to go after the     */
   { "^",           _6              },          /* letters (jmh)            */
   { "_",           _MINUS          },
   { "`",           _BACKQUOTE      },
#endif
   { "a",           _A              },
   { "b",           _B              },
   { "bkspace",     _BACKSPACE      },
   { "c",           _C              },
   { "center",      _CENTER         },
   { "d",           _D              },
   { "del",         _DEL            },
   { "down",        _DOWN           },
   { "e",           _E              },
   { "end",         _END            },
   { "enter",       _ENTER          },
   { "esc",         _ESC            },
   { "f",           _F              },
   { "f1",          _F1             },
   { "f10",         _F10            },
   { "f11",         _F11            },
   { "f12",         _F12            },
   { "f2",          _F2             },
   { "f3",          _F3             },
   { "f4",          _F4             },
   { "f5",          _F5             },
   { "f6",          _F6             },
   { "f7",          _F7             },
   { "f8",          _F8             },
   { "f9",          _F9             },
   { "g",           _G              },
   { "grey*",       _GREY_STAR      },
   { "grey+",       _GREY_PLUS      },
   { "grey-",       _GREY_MINUS     },
   { "grey/",       _GREY_SLASH     },
   { "greydel",     _DEL            },
   { "greydown",    _DOWN           },
   { "greyend",     _END            },
   { "greyenter",   _GREY_ENTER     },
   { "greyhome",    _HOME           },
   { "greyins",     _INS            },
   { "greyleft",    _LEFT           },
   { "greypgdn",    _PGDN           },
   { "greypgup",    _PGUP           },
   { "greyright",   _RIGHT          },
   { "greyup",      _UP             },
   { "h",           _H              },
   { "home",        _HOME           },
   { "i",           _I              },
   { "ins",         _INS            },
   { "j",           _J              },
   { "k",           _K              },
   { "l",           _L              },
   { "left",        _LEFT           },
   { "left\\",      _LEFT_BACKSLASH },
   { "left|",       _LEFT_BACKSLASH },
   { "m",           _M              },
   { "n",           _N              },
   { "o",           _O              },
   { "p",           _P              },
   { "pgdn",        _PGDN           },
   { "pgup",        _PGUP           },
   { "prtsc",       _PRTSC          },
   { "q",           _Q              },
   { "quote",       _APOSTROPHE     },
   { "r",           _R              },
   { "right",       _RIGHT          },
   { "s",           _S              },
   { "semicolon",   _SEMICOLON      },
   { "space",       _SPACEBAR       },
   { "t",           _T              },
   { "tabkey",      _TAB            },
   { "u",           _U              },
   { "up",          _UP             },
   { "v",           _V              },
   { "w",           _W              },
   { "x",           _X              },
   { "y",           _Y              },
   { "z",           _Z              },
#ifdef __TURBOC__
   { "[",           _LBRACKET       },
   { "\\",          _BACKSLASH      },
   { "]",           _RBRACKET       },
   { "^",           _6              },
   { "_",           _MINUS          },
   { "`",           _BACKQUOTE      },
#endif
   { "{",           _LBRACKET       },
   { "|",           _BACKSLASH      },
   { "}",           _RBRACKET       },
   { "~",           _BACKQUOTE      },
};


/*
 * list of keys to use when creating the config file for macros (jmh 020904)
 */
const char * const cfg_key[MAX_KEYS] = {
   "esc",
   "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "backspace",
   "tab", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "enter",
   ""/* ctrl */, "a", "s", "d", "f", "g", "h", "j", "k", "l", "semicolon", "'",
   "`", ""/* lshift */, "\\", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/",
   ""/* rshift */, "grey*", ""/* alt */, "space", ""/* capslock */,
   "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10",
   "greyenter", "grey/", "home", "up", "pgup", "grey-", "left",
   "center", "right", "grey+", "end", "down", "pgdn", "ins", "del",
   "prtsc", ""/* fake f11 */,  "left\\", "f11", "f12",
};


/*
 * sorted alphabetic list of functions that keys may be assigned to.
 *  add 1 to NUM_FUNCS so users may use either Macro or Playback to
 *  define macros.  Macro and Playback are both assigned to 126.
 *
 * jmh 980723: replaced the numbers with the defines.
 * jmh 980726: add another one to NUM_FUNCS due to my two-key reorganizing.
 * jmh 991121: add another one to NUM_FUNCS due to AbortCommand.
 * jmh 991125: created the f macro.
 * jmh 020923: added CopyWord/CopyString as aliases for (Beg)NextLine.
 */

#define f( x ) { #x, x },

const CONFIG_DEFS valid_func[CFG_FUNCS+1] = {
   f( AbortCommand                   )
   f( About                          )
   f( AddLine                        )
   f( BackSpace                      )
   f( BackTab                        )
   f( BalanceHorizontal              )
   f( BalanceVertical                )
   f( BegNextLine                    )
   f( BegOfLine                      )
   f( BlockBegin                     )
   f( BlockBlockComment              )
   f( BlockCapitalise                )
   f( BlockCenterJustify             )
   f( BlockCompressTabs              )
   f( BlockEmailReply                )
   f( BlockEnd                       )
   f( BlockExpandTabs                )
   f( BlockFixUUE                    )
   f( BlockIndent                    )
   f( BlockIndentN                   )
   f( BlockIndentTabs                )
   f( BlockInvertCase                )
   f( BlockLeftJustify               )
   f( BlockLineComment               )
   f( BlockLowerCase                 )
   f( BlockRightJustify              )
   f( BlockRot13                     )
   f( BlockStripHiBit                )
   f( BlockToFile                    )
   f( BlockTrimTrailing              )
   f( BlockUnComment                 )
   f( BlockUndent                    )
   f( BlockUndentN                   )
   f( BlockUpperCase                 )
   f( BorderBlock                    )
   f( BorderBlockEx                  )
   f( BotOfScreen                    )
   f( BottomLine                     )
   f( CenterJustify                  )
   f( CenterLine                     )
   f( CenterWindow                   )
   f( ChangeCurDir                   )
   f( CharacterSet                   )
   f( CharLeft                       )
   f( CharRight                      )
   f( ClearAllMacros                 )
   f( CloseWindow                    )
   f( ContextHelp                    )
   f( CopyBlock                      )
   { "CopyString",       BegNextLine },
   f( CopyToClipboard                )
   { "CopyWord",         NextLine    },
   f( CutToClipboard                 )
   f( DateTimeStamp                  )
   f( DefineDiff                     )
   f( DefineGrep                     )
   f( DefineSearch                   )
   f( DelBegOfLine                   )
   f( DelEndOfLine                   )
   f( DeleteBlock                    )
   f( DeleteChar                     )
   f( DeleteLine                     )
   f( DirList                        )
   f( DuplicateLine                  )
   f( DynamicTabSize                 )
   f( EditFile                       )
   f( EditNextFile                   )
   f( EndNextLine                    )
   f( EndOfFile                      )
   f( EndOfLine                      )
   f( EraseBegOfLine                 )
   f( Execute                        )
   f( File                           )
   f( FileAll                        )
   f( FileAttributes                 )
   f( FillBlock                      )
   f( FillBlockDown                  )
   f( FillBlockPattern               )
   f( FindBackward                   )
   f( FindForward                    )
   f( FormatParagraph                )
   f( FormatText                     )
   f( GotoMark1                      )
   f( GotoMark2                      )
   f( GotoMark3                      )
   f( GotoWindow                     )
   f( HalfScreenDown                 )
   f( HalfScreenLeft                 )
   f( HalfScreenRight                )
   f( HalfScreenUp                   )
   f( Help                           )
   f( InsertFile                     )
   f( ISearchBackward                )
   f( ISearchForward                 )
   f( JoinLine                       )
   f( JumpToPosition                 )
   f( KopyBlock                      )
   f( KopyToClipboard                )
   f( LeftJustify                    )
   f( LineDown                       )
   f( LineUp                         )
   f( LoadMacro                      )
   { "Macro",            PlayBack    },
   f( MacroMark                      )
   f( MakeHalfHorizontal             )
   f( MakeHalfVertical               )
   f( MakeHorizontal                 )
   f( MakeVertical                   )
   f( MarkBegin                      )
   f( MarkBox                        )
   f( MarkEnd                        )
   f( MarkLine                       )
   f( MarkStream                     )
   f( MoveBlock                      )
   f( MoveMark                       )
   f( NextBrowse                     )
   f( NextDirtyLine                  )
   f( NextHiddenWindow               )
   f( NextLine                       )
   f( NextWindow                     )
   { "Null",             0           },
   f( NumberBlock                    )
   f( OverlayBlock                   )
   f( PanDn                          )
   f( PanLeft                        )
   f( PanRight                       )
   f( PanUp                          )
   f( ParenBalance                   )
   f( PasteFromClipboard             )
   f( Pause                          )
   f( PlayBack                       )
   f( PopupRuler                     )
   f( PrevBrowse                     )
   f( PrevDirtyLine                  )
   f( PrevHiddenWindow               )
   f( PreviousPosition               )
   f( PreviousWindow                 )
   f( PrintBlock                     )
   f( PseudoMacro                    )
   f( PullDown                       )
   f( Quit                           )
   f( QuitAll                        )
   f( ReadConfig                     )
   f( RecordMacro                    )
   f( Redo                           )
   f( RedrawScreen                   )
   f( RegXBackward                   )
   f( RegXForward                    )
   f( Repeat                         )
   f( RepeatDiff                     )
   f( RepeatFindBackward             )
   f( RepeatFindForward              )
   f( RepeatGrep                     )
   f( RepeatRegXBackward             )
   f( RepeatRegXForward              )
   f( RepeatSearch                   )
   f( ReplaceString                  )
   f( RestoreLine                    )
   f( RetrieveLine                   )
   f( Revert                         )
   f( RightJustify                   )
   f( Rturn                          )
   f( Save                           )
   f( SaveAll                        )
   f( SaveAs                         )
   f( SaveMacro                      )
   f( SaveTo                         )
   f( SaveUntouched                  )
   f( SaveWorkspace                  )
   f( ScratchWindow                  )
   f( ScreenDown                     )
   f( ScreenLeft                     )
   f( ScreenRight                    )
   f( ScreenUp                       )
   f( ScrollDnLine                   )
   f( ScrollUpLine                   )
   f( SetBreakPoint                  )
   f( SetDirectory                   )
   f( SetMargins                     )
   f( SetMark1                       )
   f( SetMark2                       )
   f( SetMark3                       )
   f( SetTabs                        )
   f( Shell                          )
   f( SizeWindow                     )
   f( SortBoxBlock                   )
   f( SplitHalfHorizontal            )
   f( SplitHalfVertical              )
   f( SplitHorizontal                )
   f( SplitLine                      )
   f( SplitVertical                  )
   f( StampFormat                    )
   f( StartOfLine                    )
   f( Statistics                     )
   f( Status                         )
   f( StreamCharLeft                 )
   f( StreamCharRight                )
   f( StreamDeleteChar               )
   f( StringEndLeft                  )
   f( StringEndRight                 )
   f( StringLeft                     )
   f( StringRight                    )
   f( SumBlock                       )
   f( SwapBlock                      )
   f( SyntaxSelect                   )
   f( Tab                            )
   f( TitleWindow                    )
   f( ToggleCRLF                     )
   f( ToggleCursorCross              )
   f( ToggleCWD                      )
   f( ToggleDraw                     )
   f( ToggleEol                      )
   f( ToggleGraphicChars             )
   f( ToggleIndent                   )
   f( ToggleLineNumbers              )
   f( ToggleOverWrite                )
   f( ToggleQuickEdit                )
   f( ToggleReadOnly                 )
   f( ToggleRuler                    )
   f( ToggleSearchCase               )
   f( ToggleSmartTabs                )
   f( ToggleSync                     )
   f( ToggleSyntax                   )
   f( ToggleTabInflate               )
   f( ToggleTrailing                 )
   f( ToggleUndoGroup                )
   f( ToggleUndoMove                 )
   f( ToggleWordWrap                 )
   f( ToggleZ                        )
   f( TopLine                        )
   f( TopOfFile                      )
   f( TopOfScreen                    )
   f( Transpose                      )
   f( TwoCharKey                     )
   f( Undo                           )
   f( UnMarkBlock                    )
   f( UserScreen                     )
   f( WordDelete                     )
   f( WordDeleteBack                 )
   f( WordEndLeft                    )
   f( WordEndRight                   )
   f( WordLeft                       )
   f( WordRight                      )
   f( ZoomWindow                     )
};

/*
 * jmh 020923: function strings to use when creating the config file for macros
 */
const char * const func_str[NUM_FUNCS] = {
   "Null", "Help", "Rturn", "NextLine", "BegNextLine", "LineDown", "LineUp",
   "CharRight", "CharLeft", "PanRight", "PanLeft", "WordRight", "WordLeft",
   "ScreenDown", "ScreenUp", "EndOfFile", "TopOfFile", "BotOfScreen",
   "TopOfScreen", "EndOfLine", "BegOfLine", "JumpToPosition", "CenterWindow",
   "CenterLine", "ScreenRight", "ScreenLeft", "ScrollDnLine", "ScrollUpLine",
   "PanUp", "PanDn", "ToggleOverWrite", "ToggleSmartTabs", "ToggleIndent",
   "ToggleWordWrap", "ToggleCRLF", "ToggleTrailing", "ToggleZ", "ToggleEol",
   "ToggleSync", "ToggleRuler", "ToggleTabInflate", "SetTabs", "SetMargins",
   "FormatParagraph", "FormatText", "LeftJustify", "RightJustify",
   "CenterJustify", "Tab", "BackTab", "ParenBalance", "BackSpace", "DeleteChar",
   "StreamDeleteChar", "DeleteLine", "DelEndOfLine", "WordDelete", "AddLine",
   "SplitLine", "JoinLine", "DuplicateLine", "RestoreLine", "RetrieveLine",
   "ToggleSearchCase", "FindForward", "FindBackward", "RepeatFindForward",
   "RepeatFindBackward", "ReplaceString", "DefineDiff", "RepeatDiff", "MarkBox",
   "MarkLine", "MarkStream", "UnMarkBlock", "FillBlock", "NumberBlock",
   "CopyBlock", "KopyBlock", "MoveBlock", "OverlayBlock", "DeleteBlock",
   "SwapBlock", "BlockToFile", "PrintBlock", "BlockExpandTabs",
   "BlockCompressTabs", "BlockIndentTabs", "BlockTrimTrailing",
   "BlockUpperCase", "BlockLowerCase", "BlockRot13", "BlockFixUUE",
   "BlockEmailReply", "BlockStripHiBit", "SortBoxBlock", "DateTimeStamp",
   "EditFile", "DirList", "File", "Save", "SaveAs", "FileAttributes",
   "EditNextFile", "DefineGrep", "RepeatGrep", "RedrawScreen", "SizeWindow",
   "SplitHorizontal", "SplitVertical", "NextWindow", "PreviousWindow",
   "ZoomWindow", "NextHiddenWindow", "SetMark1", "SetMark2", "SetMark3",
   "GotoMark1", "GotoMark2", "GotoMark3", "RecordMacro", "Macro" /* PlayBack */,
   "SaveMacro", "LoadMacro", "ClearAllMacros", "Pause", "Quit", "NextDirtyLine",
   "PrevDirtyLine", "RegXForward", "RegXBackward", "RepeatRegXForward",
   "RepeatRegXBackward", "PullDown", "ReadConfig", "PrevHiddenWindow", "Shell",
   "StringRight", "StringLeft", "UserScreen", "QuitAll", "BlockBegin",
   "BlockEnd", "WordDeleteBack", "PreviousPosition", "FileAll", "ToggleSyntax",
   "SyntaxSelect", "Transpose", "InsertFile", "WordEndRight", "WordEndLeft",
   "StringEndRight", "StringEndLeft", "StampFormat", "PseudoMacro", "MacroMark",
   "ToggleCursorCross", "ToggleGraphicChars", "Repeat", "BorderBlock",
   "MarkBegin", "MarkEnd", "BlockLeftJustify", "BlockRightJustify",
   "BlockCenterJustify", "BlockIndentN", "BlockUndentN", "BlockIndent",
   "BlockUndent", "SetBreakPoint", "ChangeCurDir", "Status", "ToggleReadOnly",
   "GotoWindow", "BlockInvertCase", "DefineSearch", "RepeatSearch",
   "ToggleDraw", "TopLine", "BottomLine", "ToggleLineNumbers", "CharacterSet",
   "SumBlock", "Undo", "Redo", "ToggleUndoGroup", "ToggleUndoMove",
   "Statistics", "BlockCapitalise", "SaveWorkspace", "CopyToClipboard",
   "KopyToClipboard", "CutToClipboard", "PasteFromClipboard", "HalfScreenDown",
   "HalfScreenUp", "HalfScreenRight", "HalfScreenLeft", "DelBegOfLine",
   "EraseBegOfLine", "SetDirectory", "ISearchForward", "ISearchBackward",
   "ToggleCWD", "ScratchWindow", "MakeHorizontal", "MakeVertical",
   "BlockBlockComment", "BlockLineComment", "BlockUnComment", "StreamCharLeft",
   "StreamCharRight", "EndNextLine", "DynamicTabSize", "FillBlockDown",
   "FillBlockPattern", "BorderBlockEx", "SaveTo", "SaveUntouched", "Revert",
   "SplitHalfHorizontal", "SplitHalfVertical", "MakeHalfHorizontal",
   "MakeHalfVertical", "TitleWindow", "Execute", "NextBrowse", "PrevBrowse",
   "SaveAll", "MoveMark", "StartOfLine", "About", "ContextHelp", "PopupRuler",
   "BalanceHorizontal", "BalanceVertical", "CloseWindow", "ToggleQuickEdit"
};


/*
 * list of editor modes sorted alphabetically
 * jmh 020911: use the f macro
 */
const CONFIG_DEFS valid_modes[NUM_MODES] = {
   f( AutoSaveWorkspace     )
   f( Backups               )
   f( CaseConvert           )
   f( CaseIgnore            )
   f( CaseMatch             )
   f( CharDef               )
   f( ControlZ              )
   f( CursorCross           )
   f( CursorStyle           )
   f( DirSort               )
   f( DisplayCWD            )                   /* added by jmh 030226 */
   f( DisplayEndOfLine      )
   f( EndOfLineStyle        )
   f( FrameSpace            )                   /* added by jmh 991022 */
   f( FrameStyle            )                   /* added by jmh 991019 */
   f( HelpFile              )                   /* added by jmh 050711 */
   f( HelpTopic             )                   /* ditto */
   f( IndentMode            )
   f( InflateTabs           )
   f( InitialCaseMode       )
   f( InsertMode            )
   f( JustifyRightMargin    )
   f( KeyName               )                   /* added by jmh 020831 */
   f( LeftMargin            )
   f( LineNumbers           )                   /* added by jmh 991108 */
   f( LTabSize              )
   f( Menu                  )                   /* added by jmh 050722 */
   f( ParagraphMargin       )
   f( PTabSize              )
   f( QuickEdit             )                   /* added by jmh 060219 */
   f( RightMargin           )
   f( Ruler                 )
   f( Scancode              )                   /* added by jmh 020903 */
   f( Shadow                )
   f( SmartTabMode          )
   f( TimeStamp             )
   f( TrackPath             )                   /* added by jmh 021021 */
   f( TrimTrailingBlanks    )
   f( UndoGroup             )                   /* added by jmh 991120 */
   f( UndoMove              )                   /* added by jmh 010520 */
   f( UserMenu              )                   /* added by jmh 031129 */
   f( WordWrapMode          )
};

#undef f


/*
 * List of valid syntax highlighting features, sorted alphabetically.
 */
const CONFIG_DEFS valid_syntax[SHL_NUM_FEATURES] = {
   { "background",   SHL_BACKGROUND  },
   { "bad",          SHL_BAD         },
   { "binary",       SHL_BINARY      },
   { "case",         SHL_CASE        },
   { "character",    SHL_CHARACTER   },
   { "comment",      SHL_COMMENT     },
   { "escape",       SHL_ESCAPE      },
   { "function",     SHL_FUNCTION    },
   { "hex",          SHL_HEX         },
   { "InflateTabs",  SHL_INFLATETABS },
   { "innumber",     SHL_INNUMBER    },
   { "integer",      SHL_INTEGER     },
   { "inword",       SHL_INWORD      },
   { "keyword",      SHL_KEYWORD     },
   { "language",     SHL_LANGUAGE    },
   { "LTabSize",     SHL_LTABSIZE    },
   { "normal",       SHL_NORMAL      },
   { "octal",        SHL_OCTAL       },
   { "pattern",      SHL_PATTERN     },
   { "preprocessor", SHL_PREPRO      },
   { "PseudoMacro",  SHL_MACRO       },
   { "PTabSize",     SHL_PTABSIZE    },
   { "real",         SHL_REAL        },
   { "startword",    SHL_STARTWORD   },
   { "string",       SHL_STRING      },
   { "symbol",       SHL_SYMBOL      },
   { "UserMenu",     SHL_MENU        }
};


/*
 * List of additional features for string, character and preprocessor.
 */
const CONFIG_DEFS valid_string[2] = {
   { "newline",  SHL_NEWLINE  },
   { "spanline", SHL_SPANLINE }
};


const CONFIG_DEFS case_modes[2] = {
   { "ignore", IGNORE },
   { "match",  MATCH  }
};


/*
 * list of modes for the macros.
 * key / 2 is the index; key % 2 is the value.
 * jmh 990502: removed the aliases to avoid naming conflicts.
 * jmh 050709: added the flags (key & 0x8000 contains mask)
 */
const CONFIG_DEFS valid_macro_modes[NUM_MACRO_MODES] = {
  { "FixedTabs",  2                   },
  { "Indent",     5                   },
  { "Insert",     1                   },
  { "NeedBlock",  0x8000 | NEEDBLOCK  },
  { "NeedBox",    0x8000 | NEEDBOX    },
  { "NeedLine",   0x8000 | NEEDLINE   },
  { "NeedStream", 0x8000 | NEEDSTREAM },
  { "NoIndent",   4                   },
  { "NoWarn",     0x8000 | NOWARN     },
  { "NoWrap",     0x8000 | NOWRAP     },
  { "Overwrite",  0                   },
  { "SmartTabs",  3                   },
  { "UsesBlock",  0x8000 | USESBLOCK  },
  { "UsesDialog", 0x8000 | USESDIALOG },
  { "Wrap",       0x8000 | WRAP       }
};


/*
 * list of color fields sorted alphabetically
 */
const CONFIG_DEFS valid_colors[NUM_COLORS*2+1] = {
   { "background",        255                },      /* added by jmh 990420 */
   { "co80_Blocks",       Block_color        },
   { "co80_BreakPoint",   BP_color           },      /* added by jmh 031027 */
   { "co80_Cross",        Cross_color        },
   { "co80_CurLine",      Curl_color         },
   { "co80_CWD",          CWD_color          },      /* added by jmh 031028 */
   { "co80_Dialog",       Dialog_color       },      /* added by jmh 031027 */
   { "co80_DirtyLine",    Dirty_color        },
   { "co80_Disabled",     Disabled_color     },      /* added by jmh 031129 */
   { "co80_EditLabel",    EditLabel_color    },      /* added by jmh 050817 */
   { "co80_EndOfFile",    EOF_color          },
   { "co80_FileHeader",   Head_color         },
   { "co80_HelpScreen",   Help_color         },
   { "co80_HilitedFile",  Hilited_file_color },
   { "co80_Menu",         Menu_color         },      /* added by jmh 031027 */
   { "co80_MenuDisabled", Menu_dis_color     },      /* added by jmh 031129 */
   { "co80_MenuHeader",   Menu_header_color  },      /* added by jmh 031027 */
   { "co80_MenuItem",     Menu_item_color    },      /* added by jmh 031027 */
   { "co80_MenuItemBad",  Menu_nitem_color   },      /* added by jmh 031027 */
   { "co80_MenuSelected", Menu_sel_color     },      /* added by jmh 031027 */
   { "co80_MessageLine",  Message_color      },
   { "co80_ModeLine",     Mode_color         },
   { "co80_Overscan",     Overscan_color     },
   { "co80_Ruler",        Ruler_color        },
   { "co80_RulerPointer", Pointer_color      },
   { "co80_SpecialMode",  Special_color      },      /* added by jmh 020923 */
   { "co80_Swap",         Swap_color         },      /* added by jmh 031027 */
   { "co80_Text",         Text_color         },
   { "co80_Wrapped",      Diag_color         },
   { "mono_Blocks",       Block_color        },
   { "mono_BreakPoint",   BP_color           },
   { "mono_Cross",        Cross_color        },
   { "mono_CurLine",      Curl_color         },
   { "mono_CWD",          CWD_color          },
   { "mono_Dialog",       Dialog_color       },
   { "mono_DirtyLine",    Dirty_color        },
   { "mono_Disabled",     Disabled_color     },
   { "mono_EditLabel",    EditLabel_color    },
   { "mono_EndOfFile",    EOF_color          },
   { "mono_FileHeader",   Head_color         },
   { "mono_HelpScreen",   Help_color         },
   { "mono_HilitedFile",  Hilited_file_color },
   { "mono_Menu",         Menu_color         },
   { "mono_MenuDisabled", Menu_dis_color     },
   { "mono_MenuHeader",   Menu_header_color  },
   { "mono_MenuItem",     Menu_item_color    },
   { "mono_MenuItemBad",  Menu_nitem_color   },
   { "mono_MenuSelected", Menu_sel_color     },
   { "mono_MessageLine",  Message_color      },
   { "mono_ModeLine",     Mode_color         },
   { "mono_Overscan",     Overscan_color     },
   { "mono_Ruler",        Ruler_color        },
   { "mono_RulerPointer", Pointer_color      },
   { "mono_SpecialMode",  Special_color      },
   { "mono_Swap",         Swap_color         },
   { "mono_Text",         Text_color         },
   { "mono_Wrapped",      Diag_color         },
};



const CONFIG_DEFS off_on[2] = {
   { "Off",   0 },
   { "On",    1 }
};


const CONFIG_DEFS valid_z[2] = {
   { "No_Z",     0 },
   { "Write_Z",  1 }
};


const CONFIG_DEFS valid_cursor[3] = {
   { "Large",   LARGE_CURSOR  },
   { "Medium",  MEDIUM_CURSOR },
   { "Small",   SMALL_CURSOR  }
};


const CONFIG_DEFS valid_crlf[4] = {
   { "Binary",  BINARY },
   { "CRLF",    CRLF   },
   { "LF",      LF     },
   { "Native",  NATIVE }
};


const CONFIG_DEFS valid_wraps[3] = {
   { "DynamicWrap",   2 },
   { "FixedWrap",     1 },
   { "Off",           0 }
};


const CONFIG_DEFS valid_tabs[3] = {
   { "Off",   0 },
   { "On",    1 },
   { "Real",  2 }
};


const CONFIG_DEFS valid_eol[3] = {
   { "Extend", 2 },
   { "Off",    0 },
   { "On",     1 }
};


const CONFIG_DEFS valid_dir_sort[2] = {
   { "Extension", SORT_EXT  },
   { "Filename",  SORT_NAME }
};


const CONFIG_DEFS valid_pairs[8] = {
   { "Pair1",  0 },
   { "Pair2",  1 },
   { "Pair3",  2 },
   { "Pair4",  3 },
   { "Pair5",  4 },
   { "Pair6",  5 },
   { "Pair7",  6 },
   { "Pair8",  7 }
};

#if defined( __UNIX__ )
const CONFIG_DEFS valid_curse[8] = {
   { "COLOR_BLACK",    0 },
   { "COLOR_BLUE",     4 },
   { "COLOR_CYAN",     6 },
   { "COLOR_GREEN",    2 },
   { "COLOR_MAGENTA",  5 },
   { "COLOR_RED",      1 },
   { "COLOR_WHITE",    7 },
   { "COLOR_YELLOW",   3 }
};
#endif

/*
 * jmh 990420: Allow colors to be defined by name.
 * jmh 991111: Added mono names (added with 256 to distinguish from color).
 */
const CONFIG_DEFS valid_color[21] = {
   { "Black",           0       },
   { "Blue",            1       },
   { "Bold",           15 + 256 },
   { "Bright_Blue",     9       },
   { "Bright_Cyan",    11       },
   { "Bright_Green",   10       },
   { "Bright_Magenta", 13       },
   { "Bright_Red",     12       },
   { "Brown",           6       },
   { "Cyan",            3       },
   { "Dark_Grey",       8       },
   { "Green",           2       },
   { "Grey",            7       },
   { "Magenta",         5       },
   { "Normal",          7 + 256 },
   { "Red",             4       },
   { "Reverse",       112 + 256 },
   { "Standout",      127 + 256 },
   { "Underline",       1 + 256 },
   { "White",          15       },
   { "Yellow",         14       }
};

/*
 * jmh 991022: Valid frame styles.
 */
const CONFIG_DEFS valid_frame[4] = {
   { "ASCII",   0 },
   { "Combine", 3 },
   { "Double",  2 },
   { "Single",  1 }
};

/*
 * jmh 050722: Valid menu commands.
 */
const CONFIG_DEFS valid_menu[VALID_MENU_DEFS+1] = {
   { "Clear",   MENU_CLEAR   },
   { "Header",  MENU_HEADER  },
   { "Item",    MENU_ITEM    },
   { "PopItem", MENU_POPITEM },
   { "Popout",  MENU_POPOUT  }
};
