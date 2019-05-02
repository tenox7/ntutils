/*    c_mode.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef C_MODE_H
#define C_MODE_H

#define CMD_EXT 0x1000  // max 4096 internal commands, check cfte.cpp

enum Context_e {
    CONTEXT_NONE,
    CONTEXT_FILE,
    CONTEXT_DIRECTORY,
    CONTEXT_MESSAGES,
    CONTEXT_SHELL,
    CONTEXT_INPUT,
    CONTEXT_CHOICE,
    CONTEXT_LIST,
    CONTEXT_CHAR,
    CONTEXT_BUFFERS,
    CONTEXT_ROUTINES, // 10
    CONTEXT_MAPVIEW,
    CONTEXT_CVSBASE,
    CONTEXT_CVS,
    CONTEXT_CVSDIFF,
    CONTEXT_SVNBASE,
    CONTEXT_SVN,
    CONTEXT_SVNDIFF
};

typedef unsigned char ChColor;
//typedef int ChColor;

enum Hilit_e {
    HILIT_PLAIN,
    HILIT_C,
    HILIT_HTML,
    HILIT_MAKE,
    HILIT_REXX,
    HILIT_DIFF,
    HILIT_IPF,
    HILIT_PERL,
    HILIT_MERGE,
    HILIT_ADA,
    HILIT_MSG, // 10
    HILIT_SH,
    HILIT_PASCAL,
    HILIT_TEX,
    HILIT_FTE,
    HILIT_CATBS,
    HILIT_SIMPLE // 16
};

enum Indent_{
    INDENT_PLAIN,
    INDENT_C,
    INDENT_REXX,
    INDENT_SIMPLE
};

#define BFI_AutoIndent          0
#define BFI_Insert              1
#define BFI_DrawOn              2
#define BFI_HilitOn             3
#define BFI_ExpandTabs          4
#define BFI_Trim                5
#define BFI_TabSize             6

#define BFI_Colorizer           7
#define BFI_IndentMode          8

#define BFI_ShowTabs            9



#define BFI_HardMode           15
#define BFI_Undo               16
#define BFI_ReadOnly           17
#define BFI_AutoSave           18
#define BFI_KeepBackups        19
#define BFI_MatchCase          22
#define BFI_BackSpKillTab      23
#define BFI_DeleteKillTab      24
#define BFI_BackSpUnindents    25
#define BFI_SpaceTabs          26
#define BFI_IndentWithTabs     27
#define BFI_SeeThruSel         30
#define BFI_ShowMarkers        32
#define BFI_CursorThroughTabs  33
#define BFI_MultiLineHilit     35

#define BFI_WordWrap           31
#define BFI_LeftMargin         28
#define BFI_RightMargin        29


#define BFI_LineChar           10
#define BFI_StripChar          11
#define BFI_AddLF              12
#define BFI_AddCR              13
#define BFI_ForceNewLine       14
#define BFI_LoadMargin         20
#define BFI_SaveFolds          34

#define BFI_UndoLimit          21

#define BFI_AutoHilitParen     36
#define BFI_Abbreviations      37
#define BFI_BackSpKillBlock    38
#define BFI_DeleteKillBlock    39
#define BFI_PersistentBlocks   40
#define BFI_InsertKillBlock    41
#define BFI_EventMap           42
#define BFI_UndoMoves          43
#define BFI_DetectLineSep      44
#define BFI_TrimOnSave         45
#define BFI_SaveBookmarks      46
#define BFI_HilitTags          47
#define BFI_ShowBookmarks      48
#define BFI_MakeBackups        49

#define BFI_COUNT              50

#define BFS_RoutineRegexp       (0 | 256)
#define BFS_DefFindOpt          (1 | 256)
#define BFS_DefFindReplaceOpt   (2 | 256)
#define BFS_CommentStart        (3 | 256)
#define BFS_CommentEnd          (4 | 256)
#define BFS_FileNameRx          (5 | 256)
#define BFS_FirstLineRx         (6 | 256)
#define BFS_CompileCommand      (7 | 256)

#define BFS_COUNT               8

#define BFS_WordChars           (100 | 256) // ext
#define BFS_CapitalChars        (101 | 256)

#define BFI(y,x) ((y)->Flags.num[(x) & 0xFF])
#define BFI_SET(y,x,v) ((y)->Flags.num[(x) & 0xFF]=(v))
#define BFS(y,x) ((y)->Flags.str[(x) & 0xFF])

#define WSETBIT(x,y,z) \
    ((x)[(unsigned char)(y) >> 3] = char((z) ? \
    ((x)[(unsigned char)(y) >> 3] |  (1 << ((unsigned char)(y) & 0x7))) : \
    ((x)[(unsigned char)(y) >> 3] & ~(1 << ((unsigned char)(y) & 0x7)))))

#define WGETBIT(x,y) \
    (((x)[(unsigned char)(y) / 8] &  (1 << ((unsigned char)(y) % 8))) ? 1 : 0)

struct EBufferFlags {
    int num[BFI_COUNT];
    char *str[BFS_COUNT];
    char WordChars[32];
    char CapitalChars[32];
};

extern EBufferFlags DefaultBufferFlags;

/* globals */
#define FLAG_C_Indent            1
#define FLAG_C_BraceOfs          2
#define FLAG_REXX_Indent         3
#define FLAG_ScreenSizeX         6
#define FLAG_ScreenSizeY         7
#define FLAG_CursorInsertStart   8
#define FLAG_CursorInsertEnd     9
#define FLAG_CursorOverStart    10
#define FLAG_CursorOverEnd      11
#define FLAG_SysClipboard       12
#define FLAG_ShowHScroll        13
#define FLAG_ShowVScroll        14
#define FLAG_ScrollBarWidth     15
#define FLAG_SelectPathname     16
#define FLAG_C_CaseOfs          18
#define FLAG_DefaultModeName    19
#define FLAG_CompletionFilter   20
#define FLAG_ShowMenuBar        22
#define FLAG_C_CaseDelta        23
#define FLAG_C_ClassOfs         24
#define FLAG_C_ClassDelta       25
#define FLAG_C_ColonOfs         26
#define FLAG_C_CommentOfs       27
#define FLAG_C_CommentDelta     28
#define FLAG_OpenAfterClose     30
#define FLAG_PrintDevice        31
#define FLAG_CompileCommand     32
#define FLAG_REXX_Do_Offset     33
#define FLAG_KeepHistory        34
#define FLAG_LoadDesktopOnEntry 35
#define FLAG_SaveDesktopOnExit  36
#define FLAG_WindowFont         37
#define FLAG_KeepMessages       38
#define FLAG_ScrollBorderX      39
#define FLAG_ScrollBorderY      40
#define FLAG_ScrollJumpX        41
#define FLAG_ScrollJumpY        42
#define FLAG_ShowToolBar        43
#define FLAG_GUIDialogs         44
#define FLAG_PMDisableAccel     45
#define FLAG_SevenBit           46
#define FLAG_WeirdScroll        47
#define FLAG_LoadDesktopMode    48
#define FLAG_HelpCommand        49
#define FLAG_C_FirstLevelIndent 50
#define FLAG_C_FirstLevelWidth  51
#define FLAG_C_Continuation     52
#define FLAG_C_ParenDelta       53
#define FLAG_FunctionUsesContinuation 54
#define FLAG_IgnoreBufferList   55
#define FLAG_GUICharacters      56
#define FLAG_CvsCommand         57
#define FLAG_CvsLogMode         58
#define FLAG_ReassignModelIds   59
#define FLAG_RecheckReadOnly    60
#define FLAG_XShellCommand      61
#define FLAG_RGBColor		62
#define FLAG_CursorBlink        63
#define FLAG_SvnCommand         64
#define FLAG_SvnLogMode         65
#define FLAG_ShowTildeFilesInDirList 66

#define EM_MENUS 2
#define EM_MainMenu 0
#define EM_LocalMenu 1

#define COL_SyntaxParser 1

enum Color_e {
    CLR_Normal,
    CLR_Keyword,
    CLR_String,
    CLR_Comment,
    CLR_CPreprocessor,
    CLR_Regexp,
    CLR_Header,
    CLR_Quotes,
    CLR_Number,
    CLR_HexNumber,
    CLR_OctalNumber, // 10
    CLR_FloatNumber,
    CLR_Function,
    CLR_Command,
    CLR_Tag,
    CLR_Punctuation,
    CLR_New,
    CLR_Old,
    CLR_Changed,
    CLR_Control,
    CLR_Separator, // 20
    CLR_Variable,
    CLR_Symbol,
    CLR_Directive,
    CLR_Label,
    CLR_Special,
    CLR_QuoteDelim,
    CLR_RegexpDelim,

    COUNT_CLR
};

#define MATCH_MUST_BOL     0x0001
#define MATCH_MUST_BOLW    0x0002
#define MATCH_MUST_EOL     0x0004
#define MATCH_MUST_EOLW    0x0008
#define MATCH_NO_CASE      0x0010
#define MATCH_SET          0x0020
#define MATCH_NOTSET       0x0040
#define MATCH_QUOTECH      0x0100
#define MATCH_QUOTEEOL     0x0200
#define MATCH_NOGRAB       0x0400
#define MATCH_NEGATE       0x0800
#define MATCH_TAGASNEXT    0x1000
#define MATCH_REGEXP       0x2000

#define ACTION_NXSTATE     0x0001

#define STATE_NOCASE       0x0001
#define STATE_TAGASNEXT    0x0002
#define STATE_NOGRAB       0x0004

enum MacroVariable {
    mvFilePath = 1,  /* directory + name + extension */
    mvFileName,      /* name + extension */
    mvFileDirectory, /* directory + '/' */
    mvFileBaseName,  /* without the last extension */
    mvFileExtension, /* the last one */
    mvCurDirectory,
    mvCurRow,
    mvCurCol,
    mvChar,
    mvWord,
    mvLine,
    mvFTEVer
};

#endif // C_MODE_H
