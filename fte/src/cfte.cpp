/*    cfte.cpp
 *
 *    Copyright (c) 1994-1997, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "c_fconfig.h"
#include "c_hilit.h"
#include "c_mode.h"
#include "console.h"
#include "ftever.h"
#include "s_files.h"
#include "s_string.h"
#include "sysdep.h"

#include "c_commands.h"
#include "c_cmdtab.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


#define slen(s) ((s) ? (strlen(s) + 1) : 0)

struct ExMacroCFTE {
    char *Name;
};

static unsigned CMacrosCFTE = 0;
static ExMacroCFTE *MacrosCFTE = 0;

static FILE *output = 0;
static int lntotal = 0;
static long offset = -1;
static long pos = 0;
static char XTarget[MAXPATH] = "";
static char StartDir[MAXPATH] = "";
static bool preprocess_only = false;

struct CurPos {
    off_t sz;
    char *a;
    char *c;
    char *z;
    int line;
    const char *name; // filename
};

static void cleanup(int xerrno) {
    if (output)
        fclose(output);
    if (XTarget[0] != 0)
        unlink(XTarget);
    exit(xerrno);
}

static void Fail(CurPos &cp, const char *s, ...) {
    va_list ap;
    char msgbuf[1024];

    va_start(ap, s);
    vsprintf(msgbuf, s, ap);
    va_end(ap);

    fprintf(stderr, "%s:%d: Error: %s\n", cp.name, cp.line, msgbuf);
    cleanup(1);
}

static int LoadFile(const char *WhereName, const char *CfgName, int Level = 1);
static void DefineWord(const char *w);

static void PutObject(CurPos &cp, int xtag, size_t xlen, void *obj) {
    unsigned char tag = (unsigned char)xtag;
    unsigned short len = (unsigned short)xlen;
    unsigned char l[2];

    if (preprocess_only == false) {
	l[0] = (char)len;
	l[1] = (char)(len >> 8);

	if (fwrite(&tag, 1, 1, output) != 1
	    || fwrite(l, 2, 1, output) != 1
	    || fwrite(obj, 1, len, output) != len)
	    Fail(cp, "Disk full!");
    }
    pos += 1 + 2 + len;
    if (offset != -1 && pos >= offset)
        Fail(cp, "Error location found at %ld", pos);
}

static void PutNull(CurPos &cp, int xtag) {
    PutObject(cp, xtag, 0, 0);
}

static void PutString(CurPos &cp, int xtag, char *str) {
    PutObject(cp, xtag, slen(str), str);
}

static void PutNumber(CurPos &cp, int xtag, long num) {
    unsigned long l = num;
    unsigned char b[4];

    b[0] = (unsigned char)(l & 0xFF);
    b[1] = (unsigned char)((l >> 8) & 0xFF);
    b[2] = (unsigned char)((l >> 16) & 0xFF);
    b[3] = (unsigned char)((l >> 24) & 0xFF);
    PutObject(cp, xtag, 4, b);
}

int main(int argc, char **argv) {
    char Source[MAXPATH];
    char Target[MAXPATH];
    unsigned char b[4];
    unsigned long l;
    int n = 0;

    fprintf(stderr, PROG_CFTE " " VERSION "\n" COPYRIGHT "\n");
    if (argc < 2 || argc > 5) {
        fprintf(stderr, "Usage: " PROG_CFTE " [-o<offset>] [-p[reprocess]] "
#ifndef UNIX
                "config/"
#endif
                "main.fte [fte-new.cnf]\n");
        exit(1);
    }

    DefineWord("OS_"
#if defined(OS2)
               "OS2"
#elif defined(UNIX)
               "UNIX"
#elif defined(NT)
               "NT"
#elif defined(DOSP32)
               "DOS32"
#endif
              );

    // setup defaults
    strcpy(Source, "");
    strcpy(Target, "fte-new.cnf");
    preprocess_only = false;
    offset = -1;

    // parse arguments
    for (int i = 1; i < argc; i++) {
	if (argv[i][0] == '-') {
	    if ((strcmp(argv[i], "-p") == 0) || (strcmp(argv[i], "-preprocess") == 0))
		preprocess_only = true;
	    else if (strncmp(argv[i], "-o", 2) == 0) {
		char *p;

		p = argv[i];
		p += 2;
		offset = atol(p);
	    } else {
		fprintf(stderr, "Invalid option '%s'\n", argv[i]);
		exit(1);
	    }
	} else {
	    switch(n) {
	    case 0:
		strlcpy(Source, argv[i], sizeof(Source));
		break;

	    case 1:
                strlcpy(Target, argv[i], sizeof(Target));
		break;

	    default:
		fprintf(stderr, "Invalid option '%s'\n", argv[i]);
		exit(1);
	    }
            n++;
	}
    }

    if (n == 0) {
	fprintf(stderr, "No configuration file specified\n");
	exit(1);
    }

    JustDirectory(Target, XTarget, sizeof(XTarget));
    Slash(XTarget, 1);

    if (preprocess_only == false) {
	sprintf(XTarget + strlen(XTarget), "cfte%ld.tmp", (long)getpid());
	output = fopen(XTarget, "wb");
	if (output == 0) {
	    fprintf(stderr, "Cannot create '%s', errno=%d!\n", XTarget, errno);
	    cleanup(1);
	}

	b[0] = b[1] = b[2] = b[3] = 0;

	if (fwrite(b, sizeof(b), 1, output) != 1) {
	    fprintf(stderr, "Disk full!");
	    cleanup(1);
	}

	l = VERNUM;

	b[0] = (unsigned char)(l & 0xFF);
	b[1] = (unsigned char)((l >> 8) & 0xFF);
	b[2] = (unsigned char)((l >> 16) & 0xFF);
	b[3] = (unsigned char)((l >> 24) & 0xFF);

	if (fwrite(b, 4, 1, output) != 1) {
	    fprintf(stderr, "Disk full!");
	    cleanup(1);
	}
	pos = 2 * 4;

	fprintf(stderr, "Compiling to '%s'\n", Target);
    } else
    {
        pos = 2 * 4;
    }
    /*{
        char PrevDir[MAXPATH];
        sprintf(PrevDir, "%s/..", Target);
        ExpandPath(PrevDir, StartDir);
        Slash(StartDir, 1);
    }*/

    ExpandPath("."
#ifdef UNIX
               "."
#endif
               , StartDir, sizeof(StartDir));
    Slash(StartDir, 1);

    if (preprocess_only == false) {
        CurPos cp;
        char FSource[MAXPATH];

        if (ExpandPath(Source, FSource, sizeof(FSource)) != 0) {
            fprintf(stderr, "Could not expand path %s\n", Source);
            exit(1);
        }

        cp.sz = 0;
        cp.c = 0;
        cp.a = cp.c = 0;
        cp.z = cp.a + cp.sz;
        cp.line = 0;
        cp.name = "<cfte-start>";

        PutString(cp, CF_STRING, FSource);
    }

    if (LoadFile(StartDir, Source, 0) != 0) {
        fprintf(stderr, "\nCompile failed\n");
        cleanup(1);
    }

    if (preprocess_only == true)
        return 0;

    l = CONFIG_ID;
    b[0] = (unsigned char)(l & 0xFF);
    b[1] = (unsigned char)((l >> 8) & 0xFF);
    b[2] = (unsigned char)((l >> 16) & 0xFF);
    b[3] = (unsigned char)((l >> 24) & 0xFF);
    fseek(output, 0, SEEK_SET);
    fwrite(b, 4, 1, output);
    fclose(output);

    if (unlink(Target) != 0 && errno != ENOENT) {
        fprintf(stderr, "Remove of '%s' failed, result left in %s, errno=%d\n",
                Target, XTarget, errno);
        exit(1);
    }

    if (rename(XTarget, Target) != 0) {
        fprintf(stderr, "Rename of '%s' to '%s' failed, errno=%d\n",
                XTarget, Target, errno);
        exit(1);
    }

    fprintf(stderr, "\nDone.\n");
    return 0;
}

#define MODE_BFI(x) { #x, BFI_##x }
#define MODE_BFS(x) { #x, BFS_##x }
#define MODE_FLG(x) { #x, FLAG_##x }
#define EVENT_FLG(x) { #x, EM_##x }
#define COLORIZE_FLG(x) { #x, COL_##x }
#define HILIT_CLR(x) { #x, CLR_##x }

struct OrdLookup {
    const char *Name;
    int num;
};

static const OrdLookup mode_num[] = {
MODE_BFI(AutoIndent),
MODE_BFI(Insert),
MODE_BFI(DrawOn),
MODE_BFI(HilitOn),
MODE_BFI(ExpandTabs),
MODE_BFI(Trim),
MODE_BFI(TabSize),
MODE_BFI(ShowTabs),
MODE_BFI(LineChar),
MODE_BFI(StripChar),
MODE_BFI(AddLF),
MODE_BFI(AddCR),
MODE_BFI(ForceNewLine),
MODE_BFI(HardMode),
MODE_BFI(Undo),
MODE_BFI(ReadOnly),
MODE_BFI(AutoSave),
MODE_BFI(KeepBackups),
MODE_BFI(LoadMargin),
MODE_BFI(UndoLimit),
MODE_BFI(MatchCase),
MODE_BFI(BackSpKillTab),
MODE_BFI(DeleteKillTab),
MODE_BFI(BackSpUnindents),
MODE_BFI(SpaceTabs),
MODE_BFI(IndentWithTabs),
MODE_BFI(LeftMargin),
MODE_BFI(RightMargin),
MODE_BFI(SeeThruSel),
MODE_BFI(WordWrap),
MODE_BFI(ShowMarkers),
MODE_BFI(CursorThroughTabs),
MODE_BFI(SaveFolds),
MODE_BFI(MultiLineHilit),
MODE_BFI(AutoHilitParen),
MODE_BFI(Abbreviations),
MODE_BFI(BackSpKillBlock),
MODE_BFI(DeleteKillBlock),
MODE_BFI(PersistentBlocks),
MODE_BFI(InsertKillBlock),
MODE_BFI(UndoMoves),
MODE_BFI(DetectLineSep),
MODE_BFI(TrimOnSave),
MODE_BFI(SaveBookmarks),
MODE_BFI(HilitTags),
MODE_BFI(ShowBookmarks),
MODE_BFI(MakeBackups),
{ 0, 0 },
};

static const OrdLookup mode_string[] = {
MODE_BFI(Colorizer),
MODE_BFI(IndentMode),
MODE_BFS(RoutineRegexp),
MODE_BFS(DefFindOpt),
MODE_BFS(DefFindReplaceOpt),
MODE_BFS(CommentStart),
MODE_BFS(CommentEnd),
MODE_BFS(WordChars),
MODE_BFS(CapitalChars),
MODE_BFS(FileNameRx),
MODE_BFS(FirstLineRx),
MODE_BFS(CompileCommand),
MODE_BFI(EventMap),
{ 0, 0 },
};

static const OrdLookup global_num[] = {
#ifdef CONFIG_INDENT_C
MODE_FLG(C_Indent),
MODE_FLG(C_BraceOfs),
MODE_FLG(C_CaseOfs),
MODE_FLG(C_CaseDelta),
MODE_FLG(C_ClassOfs),
MODE_FLG(C_ClassDelta),
MODE_FLG(C_ColonOfs),
MODE_FLG(C_CommentOfs),
MODE_FLG(C_CommentDelta),
MODE_FLG(C_FirstLevelWidth),
MODE_FLG(C_FirstLevelIndent),
MODE_FLG(C_Continuation),
MODE_FLG(C_ParenDelta),
MODE_FLG(FunctionUsesContinuation),
#endif
#ifdef CONFIG_INDENT_REXX
MODE_FLG(REXX_Indent),
MODE_FLG(REXX_Do_Offset),
#endif
MODE_FLG(ScreenSizeX),
MODE_FLG(ScreenSizeY),
MODE_FLG(CursorInsertStart),
MODE_FLG(CursorInsertEnd),
MODE_FLG(CursorOverStart),
MODE_FLG(CursorOverEnd),
MODE_FLG(SysClipboard),
MODE_FLG(OpenAfterClose),
MODE_FLG(ShowVScroll),
MODE_FLG(ShowHScroll),
MODE_FLG(ScrollBarWidth),
MODE_FLG(SelectPathname),
MODE_FLG(ShowToolBar),
MODE_FLG(ShowMenuBar),
MODE_FLG(KeepHistory),
MODE_FLG(LoadDesktopOnEntry),
MODE_FLG(SaveDesktopOnExit),
MODE_FLG(KeepMessages),
MODE_FLG(ScrollBorderX),
MODE_FLG(ScrollBorderY),
MODE_FLG(ScrollJumpX),
MODE_FLG(ScrollJumpY),
MODE_FLG(GUIDialogs),
MODE_FLG(PMDisableAccel),
MODE_FLG(SevenBit),
MODE_FLG(WeirdScroll),
MODE_FLG(LoadDesktopMode),
MODE_FLG(IgnoreBufferList),
MODE_FLG(ReassignModelIds),
MODE_FLG(RecheckReadOnly),
MODE_FLG(CursorBlink),
MODE_FLG(ShowTildeFilesInDirList),
{ 0, 0 },
};

static const OrdLookup global_string[] = {
MODE_FLG(DefaultModeName),
MODE_FLG(CompletionFilter),
MODE_FLG(PrintDevice),
MODE_FLG(CompileCommand),
MODE_FLG(WindowFont),
MODE_FLG(HelpCommand),
MODE_FLG(GUICharacters),
MODE_FLG(CvsCommand),
MODE_FLG(CvsLogMode),
MODE_FLG(SvnCommand),
MODE_FLG(SvnLogMode),
MODE_FLG(XShellCommand),
MODE_FLG(RGBColor),
{ 0, 0 },
};

static const OrdLookup event_string[] = {
EVENT_FLG(MainMenu),
EVENT_FLG(LocalMenu),
{ 0, 0 },
};

static const OrdLookup colorize_string[] = {
COLORIZE_FLG(SyntaxParser),
{ 0, 0 },
};

static const OrdLookup hilit_colors[] = {
HILIT_CLR(Normal),
HILIT_CLR(Keyword),
HILIT_CLR(String),
HILIT_CLR(Comment),
HILIT_CLR(CPreprocessor),
HILIT_CLR(Regexp),
HILIT_CLR(Header),
HILIT_CLR(Quotes),
HILIT_CLR(Number),
HILIT_CLR(HexNumber),
HILIT_CLR(OctalNumber),
HILIT_CLR(FloatNumber),
HILIT_CLR(Function),
HILIT_CLR(Command),
HILIT_CLR(Tag),
HILIT_CLR(Punctuation),
HILIT_CLR(New),
HILIT_CLR(Old),
HILIT_CLR(Changed),
HILIT_CLR(Control),
HILIT_CLR(Separator),
HILIT_CLR(Variable),
HILIT_CLR(Symbol),
HILIT_CLR(Directive),
HILIT_CLR(Label),
HILIT_CLR(Special),
HILIT_CLR(QuoteDelim),
HILIT_CLR(RegexpDelim),
{ 0, 0 },
};

static int Lookup(const OrdLookup *where, char *what) {
    int i;

    for (i = 0; where[i].Name != 0; i++) {
        if (strcmp(what, where[i].Name) == 0)
            return where[i].num;
    }
//    fprintf(stderr, "\nBad name: %s (i = %d)\n", what, i);
    return -1;
}

#define P_EOF         0  // end of file
#define P_SYNTAX      1  // unknown
#define P_WORD        2  // a-zA-Z_
#define P_NUMBER      3  // 0-9
#define P_STRING      4  // "'`
#define P_ASSIGN      5  // =
#define P_EOS         6  // ;
#define P_KEYSPEC     7  // []
#define P_OPENBRACE   8  // {
#define P_CLOSEBRACE  9  // }
#define P_COLON      10  // :
#define P_COMMA      11  // ,
#define P_QUEST      12
#define P_VARIABLE   13  // $
#define P_DOT        14  // . (concat)

#define K_UNKNOWN     0
#define K_MODE        1
#define K_KEY         2
#define K_COLOR       3
#define K_KEYWORD     4
#define K_OBJECT      5
#define K_MENU        6
#define K_ITEM        7
#define K_SUBMENU     8
#define K_COMPILERX   9
#define K_EXTERN     10
#define K_INCLUDE    11
#define K_SUB        12
#define K_EVENTMAP   13
#define K_COLORIZE   14
#define K_ABBREV     15
#define K_HSTATE     16
#define K_HTRANS     17
#define K_HWORDS     18
#define K_SUBMENUCOND 19
#define K_HWTYPE     20
#define K_COLPALETTE 21
#define K_CVSIGNRX   22
#define K_SVNIGNRX   23

typedef char Word[64];

static const OrdLookup CfgKW[] = {
    { "mode", K_MODE },
    { "eventmap", K_EVENTMAP },
    { "key", K_KEY },
    { "color", K_COLOR },
    { "color_palette", K_COLPALETTE },
    { "keyword", K_KEYWORD },
    { "object", K_OBJECT },
    { "menu", K_MENU },
    { "item", K_ITEM },
    { "submenu", K_SUBMENU },
    { "CompileRx", K_COMPILERX },
    { "extern", K_EXTERN },
    { "include", K_INCLUDE },
    { "sub", K_SUB },
    { "colorize", K_COLORIZE },
    { "abbrev", K_ABBREV },
    { "h_state", K_HSTATE },
    { "h_trans", K_HTRANS },
    { "h_words", K_HWORDS },
    { "h_wtype", K_HWTYPE },
    { "submenucond", K_SUBMENUCOND },
    { "CvsIgnoreRx", K_CVSIGNRX },
    { "SvnIgnoreRx", K_SVNIGNRX },
    { 0, 0 },
};

static const OrdLookup CfgVar[] = {
    { "FilePath", mvFilePath },
    { "FileName", mvFileName },
    { "FileDirectory", mvFileDirectory },
    { "FileBaseName", mvFileBaseName },
    { "FileExtension", mvFileExtension },
    { "CurDirectory", mvCurDirectory },
    { "CurRow", mvCurRow, },
    { "CurCol", mvCurCol },
    { "Char", mvChar },
    { "Word", mvWord },
    { "Line", mvLine },
    { "FTEVer", mvFTEVer },
    { 0, 0 },
};

static char **words = 0;
static unsigned int wordCount = 0;

static int DefinedWord(const char *w) {
    if (words == 0 || wordCount == 0)
        return 0;
    for (unsigned int i = 0; i < wordCount; i++)
        if (strcmp(w, words[i]) == 0)
            return 1;
    return 0;
}

static void DefineWord(const char *w) {
    if (!w || !w[0])
        return ;
    if (!DefinedWord(w)) {
        words = (char **)realloc(words, sizeof (char *) * (wordCount + 1));
        assert(words != 0);
        words[wordCount] = strdup(w);
        assert(words[wordCount] != 0);
        wordCount++;
    }
}

static int colorCount;
static struct _color {
    char *colorName;
    char *colorValue;
} *colors;

static int DefineColor(char *name, char *value) {
    if (!name || !value)
        return 0;
    colors = (struct _color *)realloc(colors, sizeof (struct _color) * (colorCount + 1));
    assert(colors != 0);
    colors[colorCount].colorName = strdup(name);
    colors[colorCount].colorValue = strdup(value);
    assert(colors != NULL);
    assert(colors[colorCount].colorName != 0);
    assert(colors[colorCount].colorValue != 0);
    colorCount++;
    return 1;
}

static char *DefinedColor(char *name) {
    if (colors == 0 || colorCount == 0)
        return 0;
    for (int i = 0; i < colorCount; i++)
        if (strcmp(name, colors[i].colorName) == 0)
            return colors[i].colorValue;
    return 0;
}

static char *GetColor(CurPos &cp, char *name) {
    char *c;
    static char color[4];

    // add support for fore:back and remove it from fte itself
    if ((c = strchr(name, ' ')) != NULL) {
    } else if ((c = strchr(name, ':')) != NULL) {
        char clr[4];
        *c++ = 0;
        clr[0] = GetColor(cp, name)[0];
        clr[1] = ' ';
        clr[2] = GetColor(cp, c)[2];
        clr[3] = 0;

        memcpy(color, clr, sizeof(color));
        name = color;
    } else {
        char *p = DefinedColor(name);
        if (!p)
            Fail(cp, "Unknown symbolic color %s", name);
        name = p;
    }

    if (!isxdigit(name[0]) && name[1] != ' ' &&
        !isxdigit(name[2]) && name[3] != 0)
        Fail(cp, "Malformed color specification: %s", name);

    return name;
}

static int GetWord(CurPos &cp, char *w) {
    char *p = w;
    int len = 0;

    while (len < int(sizeof(Word)) && cp.c < cp.z &&
           ((*cp.c >= 'a' && *cp.c <= 'z') ||
            (*cp.c >= 'A' && *cp.c <= 'Z') ||
            (*cp.c >= '0' && *cp.c <= '9') ||
            (*cp.c == '_'))) {
        *p++ = *cp.c++;
        len++;
    }
    if (len == sizeof(Word))
        return -1;
    *p = 0;

    return 0;
}


static int Parse(CurPos &cp) {
    while (cp.c < cp.z) {
        switch (*cp.c) {
#ifndef UNIX
        case '\x1A': /* ^Z :-* */ return P_EOF;
#endif
        case '#':
            while (cp.c < cp.z && *cp.c != '\n') cp.c++;
            break;
        case '\n':
            cp.line++;
	    lntotal++;
        case ' ':
        case '\t':
        case '\r':
            cp.c++;
            break;
        case '=': return P_ASSIGN;
        case ';': return P_EOS;
        case ',': return P_COMMA;
        case ':': return P_COLON;
        case '.': return P_DOT;
        case '\'':
        case '"':
        case '`':
        case '/': return P_STRING;
        case '[': return P_KEYSPEC;
        case '{': return P_OPENBRACE;
        case '}': return P_CLOSEBRACE;
        case '?': return P_QUEST;
        case '$': return P_VARIABLE;
        case '-': case '+':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9': return P_NUMBER;
        default:
            if ((*cp.c >= 'a' && *cp.c <= 'z') ||
                (*cp.c >= 'A' && *cp.c <= 'Z') ||
                (*cp.c == '_'))
                return P_WORD;
            else
                return P_SYNTAX;
        }
    }
    return P_EOF;
}

static void GetOp(CurPos &cp, int what) {
    switch (what) {
    case P_COMMA:
    case P_OPENBRACE:
    case P_CLOSEBRACE:
    case P_ASSIGN:
    case P_EOS:
    case P_COLON:
    case P_QUEST:
    case P_VARIABLE:
    case P_DOT:
        cp.c++;
        break;
    }
}

static char *GetString(CurPos &cp) {
    char c = *cp.c;
    char *p = cp.c;
    char *d = cp.c;
    int n;

    if (c == '[') c = ']';

    cp.c++; // skip '"`
    while (cp.c < cp.z) {
        if (*cp.c == '\\') {
            if (c == '/')
                *p++ = *cp.c;
            cp.c++;
            if (cp.c == cp.z) return 0;
            if (c == '"') {
                switch (*cp.c) {
                case 'e': *cp.c = '\x1B'; break;
                case 't': *cp.c = '\t'; break;
                case 'r': *cp.c = '\r'; break;
                case 'n': *cp.c = '\n'; break;
                case 'b': *cp.c = '\b'; break;
                case 'v': *cp.c = '\v'; break;
                case 'a': *cp.c = '\a'; break;
                case 'x':
                    cp.c++;
                    if (cp.c == cp.z) return 0;
                    if (*cp.c >= '0' && *cp.c <= '9') n = *cp.c - '0';
                    else if (*cp.c >='a' && *cp.c <= 'f') n = *cp.c - 'a' + 10;
                    else if (*cp.c >='A' && *cp.c <= 'F') n = *cp.c - 'A' + 10;
                    else return 0;
                    cp.c++;
                    if (cp.c == cp.z) cp.c--;
                    else if (*cp.c >= '0' && *cp.c <= '9') n = n * 16 + *cp.c - '0';
                    else if (*cp.c >= 'a' && *cp.c <= 'f') n = n * 16 + *cp.c - 'a' + 10;
                    else if (*cp.c >= 'A' && *cp.c <= 'F') n = n * 16 + *cp.c - 'A' + 10;
                    else cp.c--;
                    *cp.c = (char)n;
                    break;
                }
            }
        } else if (*cp.c == c) {
            cp.c++;
            *p = 0;
            return d;
        } else if (*cp.c == '\n') {
            cp.line++;
            return 0;
        } else if (*cp.c == '\r') {
            cp.c++;
            if (cp.c == cp.z) return 0;
        }
        *p++ = *cp.c++;
    }
    return 0;
}

static int GetNumber(CurPos &cp) {
    int value = 0;
    int neg = 0;

    if (cp.c < cp.z && (*cp.c == '-' || *cp.c == '+')) {
        if (*cp.c == '-') neg = 1;
        cp.c++;
    }
    while (cp.c < cp.z && (*cp.c >= '0' && *cp.c <= '9')) {
        value = value * 10 + (*cp.c - '0');
        cp.c++;
    }
    return neg ? -value : value;
}

static int CmdNumCFTE(const char *Cmd) {

    for (size_t i = 0; i < FTE_ARRAY_SIZE(Command_Table); ++i)
        if (strcmp(Cmd, Command_Table[i].Name) == 0)
            return Command_Table[i].CmdId;
    for (unsigned i = 0; i < CMacrosCFTE; i++)
        if (MacrosCFTE[i].Name && (strcmp(Cmd, MacrosCFTE[i].Name)) == 0)
            return i | CMD_EXT;
    return 0; // Nop
}

static int NewCommandCFTE(const char *Name) {
    if (Name == 0)
        Name = "";
    MacrosCFTE = (ExMacroCFTE *) realloc(MacrosCFTE, sizeof(ExMacroCFTE) * (1 + CMacrosCFTE));
    MacrosCFTE[CMacrosCFTE].Name = strdup(Name);
    CMacrosCFTE++;
    return CMacrosCFTE - 1;
}

static int ParseCommands(CurPos &cp, char *Name) {
    //if (!Name)
    //    return 0;
    Word cmd;
    int p;
    long Cmd = NewCommandCFTE(Name) | CMD_EXT;

    long cnt;
    long ign = 0;

    PutNumber(cp, CF_INT, Cmd);
    GetOp(cp, P_OPENBRACE);
    cnt = 1;
    while (1) {
        p = Parse(cp);
        if (p == P_CLOSEBRACE) break;
        if (p == P_EOF) Fail(cp, "Unexpected EOF");

        if (p == P_DOT) {
            GetOp(cp, P_DOT);
            PutNull(cp, CF_CONCAT);
        } else if (p == P_NUMBER) {
            long num = GetNumber(cp);
            if (Parse(cp) != P_COLON) {
                PutNumber(cp, CF_INT, num);
            } else {
                cnt = num;
                GetOp(cp, P_COLON);
            }
        } else if (p == P_WORD) {
            long Command;

            if (GetWord(cp, cmd) == -1) Fail(cp, "Syntax error");
            Command = CmdNumCFTE(cmd);
            if (Command == 0)
                Fail(cp, "Unrecognised command: %s", cmd);
            PutNumber(cp, CF_COMMAND, Command);
            PutNumber(cp, CF_INT, cnt);
            PutNumber(cp, CF_INT, ign);
            ign = 0;
            cnt = 1;
        } else if (p == P_STRING) {
            char *s = GetString(cp);
            PutString(cp, CF_STRING, s);
        } else if (p == P_QUEST) {
            ign = 1;
            GetOp(cp, P_QUEST);
        } else if (p == P_VARIABLE) {
            GetOp(cp, P_VARIABLE);
            if (Parse(cp) != P_WORD) Fail(cp, "Syntax error (variable name expected)");
            Word w;
            if (GetWord(cp, w) != 0) Fail(cp, "Syntax error (bad variable name)");
            long var = Lookup(CfgVar, w);
            if (var == -1) Fail(cp, "Unrecognised variable");
            PutNumber(cp, CF_VARIABLE, var);
        } else if (p == P_EOS) {
            GetOp(cp, P_EOS);
            cnt = 1;
        } else
            Fail(cp, "Syntax error");
    }
    GetOp(cp, P_CLOSEBRACE);
    return 0;
}

static int ParseConfigFile(CurPos &cp) {
    Word w = "";
    char *s = 0;
    int p = 0;

    Word ObjName = "", UpMode = "";

    while (1) {
        p = Parse(cp);

        switch (p) {
        case P_WORD:
            if (GetWord(cp, w) != 0) Fail(cp, "Syntax error");
            switch (Lookup(CfgKW, w)) {
            case K_SUB:
                {
                    Word Name;

                    if (Parse(cp) != P_WORD) Fail(cp, "Syntax error");
                    if (GetWord(cp, Name) != 0) Fail(cp, "Syntax error");
                    PutString(cp, CF_SUB, Name);
                    if (Parse(cp) != P_OPENBRACE) Fail(cp, "'{' expected");
                    GetOp(cp, P_OPENBRACE);
                    if (ParseCommands(cp, strdup(Name)) == -1)
                        Fail(cp, "Parse failed");
                    PutNull(cp, CF_END);
                }
                break;
            case K_MENU:

                {
                    Word MenuName;
                    //int menu = -1, item = -1;

                    if (Parse(cp) != P_WORD) Fail(cp, "Syntax error");;
                    if (GetWord(cp, MenuName) != 0) Fail(cp, "Syntax error");;
                    if (Parse(cp) != P_OPENBRACE) Fail(cp, "'{' expected");
                    GetOp(cp, P_OPENBRACE);

                    PutString(cp, CF_MENU, MenuName);

                    while (1) {
                        p = Parse(cp);
                        if (p == P_CLOSEBRACE) break;
                        if (p == P_EOF) Fail(cp, "Unexpected EOF");
                        if (p != P_WORD) Fail(cp, "Syntax error");

                        if (GetWord(cp, w) != 0) Fail(cp, "Parse failed");
                        switch (Lookup(CfgKW, w)) {
                        case K_ITEM: // menu::item
                            switch (Parse(cp)) {
                            case P_EOS:
                                PutNull(cp, CF_ITEM);
                                break;
                            case P_STRING:
                                s = GetString(cp);
                                PutString(cp, CF_ITEM, s);
                                break;
                            default:
                                Fail(cp, "Syntax error");;
                            }
                            if (Parse(cp) == P_EOS) {
                                GetOp(cp, P_EOS);
                                break;
                            }
                            if (Parse(cp) != P_OPENBRACE)
                                Fail(cp, "'{' expected");

                            PutNull(cp, CF_MENUSUB);
                            if (ParseCommands(cp, 0) == -1)
                                Fail(cp, "Parse failed");
                            PutNull(cp, CF_END);
                            break;
                        case K_SUBMENU: // menu::submenu
                            if (Parse(cp) != P_STRING)
                                Fail(cp, "String expected");
                            s = GetString(cp);
                            if (Parse(cp) != P_COMMA)
                                Fail(cp, "',' expected");
                            GetOp(cp, P_COMMA);
                            if (Parse(cp) != P_WORD)
                                Fail(cp, "Syntax error");
                            if (GetWord(cp, w) == -1)
                                Fail(cp, "Parse failed");

                            PutString(cp, CF_SUBMENU, s);
                            PutString(cp, CF_STRING, w);
                            if (Parse(cp) != P_EOS)
                                Fail(cp, "';' expected");
                            GetOp(cp, P_EOS);
                            break;

                        case K_SUBMENUCOND: // menu::submenu
                            if (Parse(cp) != P_STRING)
                                Fail(cp, "String expected");
                            s = GetString(cp);
                            if (Parse(cp) != P_COMMA)
                                Fail(cp, "',' expected");
                            GetOp(cp, P_COMMA);
                            if (Parse(cp) != P_WORD)
                                Fail(cp, "Syntax error");
                            if (GetWord(cp, w) == -1)
                                Fail(cp, "Parse failed");

                            PutString(cp, CF_SUBMENUCOND, s);
                            PutString(cp, CF_STRING, w);
                            if (Parse(cp) != P_EOS)
                                Fail(cp, "';' expected");
                            GetOp(cp, P_EOS);
                            break;
                        default:  // menu::
                            Fail(cp, "Syntax error");
                        }
                    }
                    GetOp(cp, P_CLOSEBRACE);
                    PutNull(cp, CF_END);
                }
                break;
            case K_EVENTMAP:
                {
                    if (Parse(cp) != P_WORD) Fail(cp, "Syntax error");
                    if (GetWord(cp, ObjName) != 0) Fail(cp, "Parse failed");
                    PutString(cp, CF_EVENTMAP, ObjName);

                    UpMode[0] = 0;
                    if (Parse(cp) == P_COLON) {
                        GetOp(cp, P_COLON);
                        if (Parse(cp) != P_WORD) Fail(cp, "Syntax error");
                        if (GetWord(cp, UpMode) != 0) Fail(cp, "Parse failed");
                    }
                    PutString(cp, CF_PARENT, UpMode);
                    if (Parse(cp) != P_OPENBRACE) Fail(cp, "'{' expected");
                    GetOp(cp, P_OPENBRACE);

                    while (1) {
                        p = Parse(cp);
                        if (p == P_CLOSEBRACE) break;
                        if (p == P_EOF) Fail(cp, "Unexpected EOF");
                        if (p != P_WORD) Fail(cp, "Syntax error");

                        if (GetWord(cp, w) != 0) Fail(cp, "Parse failed");
                        switch (Lookup(CfgKW, w)) {
                        case K_KEY: // mode::key
                            if (Parse(cp) != P_KEYSPEC) Fail(cp, "'[' expected");
                            s = GetString(cp);
                            PutString(cp, CF_KEY, s);
                            if (Parse(cp) != P_OPENBRACE) Fail(cp, "'{' expected");
                            PutNull(cp, CF_KEYSUB);
                            if (ParseCommands(cp, 0) == -1) Fail(cp, "Parse failed");
                            PutNull(cp, CF_END);
                            break;

                        case K_ABBREV:
                            if (Parse(cp) != P_STRING) Fail(cp, "String expected");
                            s = GetString(cp);
                            PutString(cp, CF_ABBREV, s);
                            switch (Parse(cp)) {
                            case P_OPENBRACE:
                                PutNull(cp, CF_KEYSUB);
                                if (ParseCommands(cp, 0) == -1) Fail(cp, "Parse failed");
                                PutNull(cp, CF_END);
                                break;
                            case P_STRING:
                                s = GetString(cp);
                                PutString(cp, CF_STRING, s);
                                if (Parse(cp) != P_EOS) Fail(cp, "';' expected");
                                GetOp(cp, P_EOS);
                                break;
                            default:
                                Fail(cp, "Syntax error");
                            }
                            break;

                        default:  // mode::
                            if (Parse(cp) != P_ASSIGN) Fail(cp, "'=' expected");
                            GetOp(cp, P_ASSIGN);

                            switch (Parse(cp)) {
                                /*  case P_NUMBER:
                                 {
                                 long var;
                                 long num;

                                 num = GetNumber(cp);
                                 var = LookupEventNumber(w);
                                 if (var == -1) return -1;
                                 PutObj(cp, CF_SETVAR, sizeof(long), &var);
                                 PutObj(cp, CF_INT, sizeof(long), &num);
                                 }
                                 break;*/
                            case P_STRING:
                                {
                                    long var;

                                    s = GetString(cp);
                                    if (s == 0) Fail(cp, "String expected");
                                    var = Lookup(event_string, w);
                                    if (var == -1) Fail(cp, "event_string Lookup of '%s' failed", w);
                                    PutNumber(cp, CF_SETVAR, var);
                                    PutString(cp, CF_STRING, s);
                                }
                                break;
                            default:
                                return -1;
                            }
                            if (Parse(cp) != P_EOS) return -1;
                            GetOp(cp, P_EOS);
                            break;
                        }
                    }
                    GetOp(cp, P_CLOSEBRACE);
                    PutNull(cp, CF_END);
                }
                break;

            case K_COLORIZE:
                {
                    long LastState = -1;

                    if (Parse(cp) != P_WORD) Fail(cp, "Syntax error");
                    if (GetWord(cp, ObjName) != 0) Fail(cp, "Parse failed");
                    PutString(cp, CF_COLORIZE, ObjName);

                    UpMode[0] = 0;
                    if (Parse(cp) == P_COLON) {
                        GetOp(cp, P_COLON);
                        if (Parse(cp) != P_WORD) Fail(cp, "Syntax error");
                        if (GetWord(cp, UpMode) != 0) Fail(cp, "Parse failed");
                    }
                    PutString(cp, CF_PARENT, UpMode);
                    if (Parse(cp) != P_OPENBRACE) Fail(cp, "'{' expected");
                    GetOp(cp, P_OPENBRACE);

                    while (1) {
                        p = Parse(cp);
                        if (p == P_CLOSEBRACE) break;
                        if (p == P_EOF) Fail(cp, "Unexpected EOF");
                        if (p != P_WORD) Fail(cp, "Syntax error");

                        if (GetWord(cp, w) != 0) Fail(cp, "Parse failed");
                        switch (Lookup(CfgKW, w)) {
                        case K_COLOR: // mode::color
                            if (Parse(cp) != P_OPENBRACE) Fail(cp, "'{' expected");
                            GetOp(cp, P_OPENBRACE);
                            PutNull(cp, CF_COLOR);

                            while (1) {
                                char *sname, *svalue;
                                long cidx;

                                if (Parse(cp) == P_CLOSEBRACE) break;
                                if (Parse(cp) != P_OPENBRACE) Fail(cp, "'{' expected");
                                GetOp(cp, P_OPENBRACE);
                                if (Parse(cp) != P_STRING) Fail(cp, "String expected");
                                sname = GetString(cp);
                                if ((cidx = Lookup(hilit_colors, sname)) == -1)
                                    Fail(cp, "hilit_colors Lookup of '%s' failed", sname);
                                PutNumber(cp, CF_INT, cidx);
                                if (Parse(cp) != P_COMMA)
                                    Fail(cp, "',' expected");
                                GetOp(cp, P_COMMA);
                                if (Parse(cp) != P_STRING) Fail(cp, "String expected");
                                svalue = GetString(cp);
                                svalue = GetColor(cp, svalue);
                                PutString(cp, CF_STRING, svalue);
                                if (Parse(cp) != P_CLOSEBRACE) Fail(cp, "'}' expected");
                                GetOp(cp, P_CLOSEBRACE);
                                if (Parse(cp) != P_COMMA)
                                    break;
                                else
                                    GetOp(cp, P_COMMA);
                            }
                            if (Parse(cp) != P_CLOSEBRACE) Fail(cp, "'}' expected");
                            GetOp(cp, P_CLOSEBRACE);
                            if (Parse(cp) != P_EOS) Fail(cp, "';' expected");
                            GetOp(cp, P_EOS);
                            PutNull(cp, CF_END);
                            break;

                        case K_KEYWORD: // mode::keyword
                            {
                                char *colorstr, *kname;
                                //int color;

                                if (Parse(cp) != P_STRING) Fail(cp, "String expected");
                                colorstr = GetString(cp);
                                colorstr = GetColor(cp, colorstr);
                                if (Parse(cp) != P_OPENBRACE) Fail(cp, "'{' expected");
                                GetOp(cp, P_OPENBRACE);

                                PutString(cp, CF_KEYWORD, colorstr);

                                while (1) {
                                    if (Parse(cp) == P_CLOSEBRACE) break;
                                    if (Parse(cp) != P_STRING) Fail(cp, "String expected");
                                    kname = GetString(cp);

                                    if (strlen(kname) >= CK_MAXLEN) Fail(cp, "Keyword name is too long");

                                    PutString(cp, CF_STRING, kname);

                                    if (Parse(cp) != P_COMMA)
                                        break;
                                    else
                                        GetOp(cp, P_COMMA);
                                }
                            }
                            if (Parse(cp) != P_CLOSEBRACE) Fail(cp, "'}' expected");
                            GetOp(cp, P_CLOSEBRACE);
                            if (Parse(cp) != P_EOS) Fail(cp, "';' expected");
                            GetOp(cp, P_EOS);
                            PutNull(cp, CF_END);
                            break;

                        case K_HSTATE:
                            {
                                long stateno;
                                char *cname;
                                long cidx;

                                if (Parse(cp) != P_NUMBER) Fail(cp, "state index expected");
                                stateno = GetNumber(cp);
                                if (stateno != LastState + 1) Fail(cp, "invalid state index");

                                if (Parse(cp) != P_OPENBRACE) Fail(cp, "'{' expected");
                                GetOp(cp, P_OPENBRACE);
                                PutNumber(cp, CF_HSTATE, stateno);

                                if (Parse(cp) != P_STRING) Fail(cp, "String expected");
                                cname = GetString(cp);
                                if ((cidx = Lookup(hilit_colors, cname)) == -1)
                                    Fail(cp, "hilit_colors Lookup of '%s' failed", cname);
                                PutNumber(cp, CF_INT, cidx);
                                if (Parse(cp) != P_CLOSEBRACE) Fail(cp, "'}' expected");
                                GetOp(cp, P_CLOSEBRACE);
                                LastState = stateno;
                            }
                            break;

                        case K_HTRANS:
                            {
                                long next_state;
                                char *opts, *match;
                                long match_opts;
                                char *cname;
                                long cidx;

                                if (Parse(cp) != P_OPENBRACE) Fail(cp, "'{' expected");
                                GetOp(cp, P_OPENBRACE);

                                if (Parse(cp) != P_NUMBER) Fail(cp, "next_state index expected");
                                next_state = GetNumber(cp);
                                if (Parse(cp) != P_COMMA) Fail(cp, "',' expected");
                                GetOp(cp, P_COMMA);
                                if (Parse(cp) != P_STRING) Fail(cp, "match options expected");
                                opts = GetString(cp);
                                if (Parse(cp) != P_COMMA) Fail(cp, "',' expected");
                                GetOp(cp, P_COMMA);
                                if (Parse(cp) != P_STRING) Fail(cp, "match string expected");
                                match = GetString(cp);
                                PutNumber(cp, CF_HTRANS, next_state);
                                match_opts = 0;
                                if (strchr(opts, '^')) match_opts |= MATCH_MUST_BOL;
                                if (strchr(opts, '$')) match_opts |= MATCH_MUST_EOL;
                                //if (strchr(opts, 'b')) match_opts |= MATCH_MUST_BOLW;
                                //if (strchr(opts, 'e')) match_opts |= MATCH_MUST_EOLW;
                                if (strchr(opts, 'i')) match_opts |= MATCH_NO_CASE;
                                if (strchr(opts, 's')) match_opts |= MATCH_SET;
                                if (strchr(opts, 'S')) match_opts |= MATCH_NOTSET;
                                if (strchr(opts, '-')) match_opts |= MATCH_NOGRAB;
                                if (strchr(opts, '<')) match_opts |= MATCH_TAGASNEXT;
                                if (strchr(opts, '>')) match_opts &= ~MATCH_TAGASNEXT;
                                //if (strchr(opts, '!')) match_opts |= MATCH_NEGATE;
                                if (strchr(opts, 'q')) match_opts |= MATCH_QUOTECH;
                                if (strchr(opts, 'Q')) match_opts |= MATCH_QUOTEEOL;
                                if (strchr(opts, 'x')) match_opts |= MATCH_REGEXP;

                                if (Parse(cp) != P_COMMA) Fail(cp, "',' expected");
                                GetOp(cp, P_COMMA);
                                if (Parse(cp) != P_STRING) Fail(cp, "String expected");
                                cname = GetString(cp);
                                if ((cidx = Lookup(hilit_colors, cname)) == -1)
                                    Fail(cp, "hilit_colors Lookup of '%s' failed", cname);

                                PutNumber(cp, CF_INT, match_opts);
                                PutNumber(cp, CF_INT, cidx);
                                PutString(cp, match_opts & MATCH_REGEXP ? CF_REGEXP : CF_STRING, match);

                                if (Parse(cp) != P_CLOSEBRACE) Fail(cp, "'}' expected");
                                GetOp(cp, P_CLOSEBRACE);
                            }
                            break;

                        case K_HWTYPE:
                            if (Parse(cp) != P_OPENBRACE) Fail(cp, "'{' expected");
                            GetOp(cp, P_OPENBRACE);

                            {
                                long options = 0;
                                long nextKwdMatchedState;
                                long nextKwdNotMatchedState;
                                long nextKwdNoCharState;
                                char *opts;
                                char *wordChars;


                                if (Parse(cp) != P_NUMBER) Fail(cp, "next_state index expected");
                                nextKwdMatchedState = GetNumber(cp);

                                if (Parse(cp) != P_COMMA) Fail(cp, "',' expected");
                                GetOp(cp, P_COMMA);

                                if (Parse(cp) != P_NUMBER) Fail(cp, "next_state index expected");
                                nextKwdNotMatchedState = GetNumber(cp);

                                if (Parse(cp) != P_COMMA) Fail(cp, "',' expected");
                                GetOp(cp, P_COMMA);

                                if (Parse(cp) != P_NUMBER) Fail(cp, "next_state index expected");
                                nextKwdNoCharState = GetNumber(cp);

                                if (Parse(cp) != P_COMMA) Fail(cp, "',' expected");
                                GetOp(cp, P_COMMA);

                                if (Parse(cp) != P_STRING) Fail(cp, "String expected");
                                opts = GetString(cp);
                                if (strchr(opts, 'i')) options |= STATE_NOCASE;
                                if (strchr(opts, '<')) options |= STATE_TAGASNEXT;
                                if (strchr(opts, '>')) options &= ~STATE_TAGASNEXT;
                                if (strchr(opts, '-')) options |= STATE_NOGRAB;

                                if (Parse(cp) != P_COMMA) Fail(cp, "',' expected");
                                GetOp(cp, P_COMMA);

                                if (Parse(cp) != P_STRING) Fail(cp, "String expected");
                                wordChars = GetString(cp);

                                PutNull(cp, CF_HWTYPE);
                                PutNumber(cp, CF_INT, nextKwdMatchedState);
                                PutNumber(cp, CF_INT, nextKwdNotMatchedState);
                                PutNumber(cp, CF_INT, nextKwdNoCharState);
                                PutNumber(cp, CF_INT, options);
                                PutString(cp, CF_STRING, wordChars);
                            }
                            if (Parse(cp) != P_CLOSEBRACE) Fail(cp, "'}' expected");
                            GetOp(cp, P_CLOSEBRACE);
                            break;

                        case K_HWORDS:
                            {
                                char *colorstr, *kname;
                                //int color;

                                if (Parse(cp) != P_STRING) Fail(cp, "String expected");
                                colorstr = GetString(cp);
                                colorstr = GetColor(cp, colorstr);

                                if (Parse(cp) != P_OPENBRACE) Fail(cp, "'{' expected");
                                GetOp(cp, P_OPENBRACE);

                                PutString(cp, CF_HWORDS, colorstr);

                                while (1) {
                                    if (Parse(cp) == P_CLOSEBRACE) break;
                                    if (Parse(cp) != P_STRING) Fail(cp, "String expected");
                                    kname = GetString(cp);
                                    PutString(cp, CF_STRING, kname);

                                    if (Parse(cp) != P_COMMA)
                                        break;
                                    else
                                        GetOp(cp, P_COMMA);
                                }
                            }
                            if (Parse(cp) != P_CLOSEBRACE) Fail(cp, "'}' expected");
                            GetOp(cp, P_CLOSEBRACE);

                            PutNull(cp, CF_END);
                            break;

                        default:
                            if (Parse(cp) != P_ASSIGN) Fail(cp, "'=' expected");
                            GetOp(cp, P_ASSIGN);
                            switch (Parse(cp)) {
                                /*case P_NUMBER:
                                 {
                                 long var;
                                 long num;

                                 num = GetNumber(cp);
                                 var = LookupColorizeNumber(w);
                                 if (var == -1) return -1;
                                 PutObj(cp, CF_SETVAR, sizeof(long), &var);
                                 PutObj(cp, CF_INT, sizeof(long), &num);
                                 }
                                 break;*/
                            case P_STRING:
                                {
                                    long var;

                                    s = GetString(cp);
                                    if (s == 0) Fail(cp, "Parse failed");
                                    var = Lookup(colorize_string, w);
                                    if (var == -1)
                                        Fail(cp, "colorize_string Lookup of '%s' failed", w);
                                    PutNumber(cp, CF_SETVAR, var);
                                    PutString(cp, CF_STRING, s);
                                }
                                break;
                            default:
                                return -1;
                            }
                            if (Parse(cp) != P_EOS) Fail(cp, "';' expected");
                            GetOp(cp, P_EOS);
                            break;
                        }
                    }
                    GetOp(cp, P_CLOSEBRACE);
                    PutNull(cp, CF_END);
                }
                break;

            case K_MODE: // mode::
                {
                    if (Parse(cp) != P_WORD) Fail(cp, "Syntax error");
                    if (GetWord(cp, ObjName) != 0) Fail(cp, "Parse failed");
                    PutString(cp, CF_MODE, ObjName);

                    UpMode[0] = 0;
                    if (Parse(cp) == P_COLON) {
                        GetOp(cp, P_COLON);
                        if (Parse(cp) != P_WORD) Fail(cp, "Syntax error");
                        if (GetWord(cp, UpMode) != 0) Fail(cp, "Parse failed");
                    }
                    PutString(cp, CF_PARENT, UpMode);
                    if (Parse(cp) != P_OPENBRACE) Fail(cp, "'{' expected");
                    GetOp(cp, P_OPENBRACE);

                    while (1) {
                        p = Parse(cp);
                        if (p == P_CLOSEBRACE) break;
                        if (p == P_EOF) Fail(cp, "Unexpected EOF");
                        if (p != P_WORD) Fail(cp, "Syntax error");

                        if (GetWord(cp, w) != 0) Fail(cp, "Parse failed");
                        //switch (Lookup(CfgKW, w)) {
                        //default:  // mode::
                        if (Parse(cp) != P_ASSIGN) Fail(cp, "'=' expected");
                        GetOp(cp, P_ASSIGN);
                        switch (Parse(cp)) {
                        case P_NUMBER:
                            {
                                long var;
                                long num;

                                num = GetNumber(cp);
                                var = Lookup(mode_num, w);
                                if (var == -1)
                                    Fail(cp, "mode_num Lookup of '%s' failed", w);
                                PutNumber(cp, CF_SETVAR, var);
                                PutNumber(cp, CF_INT, num);
                            }
                            break;
                        case P_STRING:
                            {
                                long var;

                                s = GetString(cp);
                                if (s == 0) Fail(cp, "Parse failed");
                                var = Lookup(mode_string, w);
                                if (var == -1)
                                    Fail(cp, "mode_string Lookup of '%s' filed", w);
                                PutNumber(cp, CF_SETVAR, var);
                                PutString(cp, CF_STRING, s);
                            }
                            break;
                        default:
                            return -1;
                        }
                        if (Parse(cp) != P_EOS) Fail(cp, "';' expected");
                        GetOp(cp, P_EOS);
                        //    break;
                        //}
                    }
                    GetOp(cp, P_CLOSEBRACE);
                    PutNull(cp, CF_END);
                }
                break;
            case K_OBJECT:
                {
                    if (Parse(cp) != P_WORD) Fail(cp, "Syntax error");
                    if (GetWord(cp, ObjName) != 0) Fail(cp, "Parse failed");
                    if (Parse(cp) != P_OPENBRACE) Fail(cp, "'{' expected");
                    GetOp(cp, P_OPENBRACE);

                    PutString(cp, CF_OBJECT, ObjName);

                    while (1) {
                        p = Parse(cp);
                        if (p == P_CLOSEBRACE) break;
                        if (p == P_EOF) Fail(cp, "Unexpected EOF");
                        if (p != P_WORD) Fail(cp, "Syntax error");

                        if (GetWord(cp, w) != 0) Fail(cp, "Parse failed");
                        switch (Lookup(CfgKW, w)) {
                        case K_COLOR: // mode::color
                            if (Parse(cp) != P_OPENBRACE) Fail(cp, "'{' expected");
                            GetOp(cp, P_OPENBRACE);
                            PutNull(cp, CF_COLOR);

                            while (1) {
                                char *sname, *svalue;

                                if (Parse(cp) == P_CLOSEBRACE) break;
                                if (Parse(cp) != P_OPENBRACE) Fail(cp, "'{' expected");
                                GetOp(cp, P_OPENBRACE);
                                if (Parse(cp) != P_STRING) Fail(cp, "String expected");
                                sname = GetString(cp);
                                PutString(cp, CF_STRING, sname);
                                if (Parse(cp) != P_COMMA) Fail(cp, "',' expected");
                                GetOp(cp, P_COMMA);
                                if (Parse(cp) != P_STRING) Fail(cp, "String expected");
                                svalue = GetString(cp);
                                svalue = GetColor(cp, svalue);
                                PutString(cp, CF_STRING, svalue);
                                if (Parse(cp) != P_CLOSEBRACE) Fail(cp, "'}' expected");
                                GetOp(cp, P_CLOSEBRACE);
                                if (Parse(cp) != P_COMMA)
                                    break;
                                else
                                    GetOp(cp, P_COMMA);
                            }
                            if (Parse(cp) != P_CLOSEBRACE) Fail(cp, "'}' expected");
                            GetOp(cp, P_CLOSEBRACE);
                            if (Parse(cp) != P_EOS) Fail(cp, "';' expected");
                            GetOp(cp, P_EOS);
                            PutNull(cp, CF_END);
                            break;

                        case K_COMPILERX:
                            {
                                long file, line, msg;
                                char *regexp;

                                if (Parse(cp) != P_ASSIGN) Fail(cp, "'=' expected");
                                GetOp(cp, P_ASSIGN);
                                if (Parse(cp) != P_OPENBRACE) Fail(cp, "'{' expected");
                                GetOp(cp, P_OPENBRACE);
                                if (Parse(cp) != P_NUMBER) Fail(cp, "Number expected");
                                file = GetNumber(cp);
                                if (Parse(cp) != P_COMMA) Fail(cp, "',' expected");
                                GetOp(cp, P_COMMA);
                                if (Parse(cp) != P_NUMBER) Fail(cp, "Number expected");
                                line = GetNumber(cp);
                                if (Parse(cp) != P_COMMA) Fail(cp, "',' expected");
                                GetOp(cp, P_COMMA);
                                if (Parse(cp) != P_NUMBER) Fail(cp, "Number expected");
                                msg = GetNumber(cp);
                                if (Parse(cp) != P_COMMA) Fail(cp, "',' expected");
                                GetOp(cp, P_COMMA);
                                if (Parse(cp) != P_STRING) Fail(cp, "String expected");
                                regexp = GetString(cp);
                                if (Parse(cp) != P_CLOSEBRACE) Fail(cp, "'}' expected");
                                GetOp(cp, P_CLOSEBRACE);
                                PutNull(cp, CF_COMPRX);
                                PutNumber(cp, CF_INT, file);
                                PutNumber(cp, CF_INT, line);
                                PutNumber(cp, CF_INT, msg);
                                PutString(cp, CF_REGEXP, regexp);
                                if (Parse(cp) != P_EOS) Fail(cp, "';' expected");
                                GetOp(cp, P_EOS);
                            }
                            break;
                        case K_CVSIGNRX:
                            {
                                char *regexp;

                                if (Parse(cp) != P_ASSIGN) Fail(cp, "'=' expected");
                                GetOp(cp, P_ASSIGN);
                                if (Parse(cp) != P_STRING) Fail(cp, "String expected");
                                regexp = GetString(cp);
                                PutNull(cp, CF_CVSIGNRX);
                                PutString(cp, CF_REGEXP, regexp);
                                if (Parse(cp) != P_EOS) Fail(cp, "';' expected");
                                GetOp(cp, P_EOS);
                            }
                            break;
                        case K_SVNIGNRX:
                            {
                                char *regexp;

                                if (Parse(cp) != P_ASSIGN) Fail(cp, "'=' expected");
                                GetOp(cp, P_ASSIGN);
                                if (Parse(cp) != P_STRING) Fail(cp, "String expected");
                                regexp = GetString(cp);
                                PutNull(cp, CF_SVNIGNRX);
                                PutString(cp, CF_REGEXP, regexp);
                                if (Parse(cp) != P_EOS) Fail(cp, "';' expected");
                                GetOp(cp, P_EOS);
                            }
                            break;
                        default:  // mode::
                            if (Parse(cp) != P_ASSIGN) Fail(cp, "'=' expected");
                            GetOp(cp, P_ASSIGN);

                            switch (Parse(cp)) {
                            case P_NUMBER:
                                {
                                    long var;
                                    long num;

                                    num = GetNumber(cp);
                                    var = Lookup(global_num, w);
                                    if (var == -1)
                                        Fail(cp, "global_num Lookup of '%s' failed", w);
                                    PutNumber(cp, CF_SETVAR, var);
                                    PutNumber(cp, CF_INT, num);
                                }
                                break;
                            case P_STRING:
                                {
                                    long var;

                                    s = GetString(cp);
                                    if (s == 0) Fail(cp, "Parse failed");
                                    var = Lookup(global_string, w);
                                    if (var == -1) Fail(cp, "global_string Lookup of '%s' failed", w);
                                    PutNumber(cp, CF_SETVAR, var);
                                    PutString(cp, CF_STRING, s);
                                }
                                break;
                            default:
                                Fail(cp, "Syntax error");
                            }
                            if (Parse(cp) != P_EOS) Fail(cp, "';' expected");
                            GetOp(cp, P_EOS);
                            break;
                        }
                    }
                    GetOp(cp, P_CLOSEBRACE);
                    PutNull(cp, CF_END);
                }
                break;

            case K_COLPALETTE:
                {
                    if (Parse(cp) != P_OPENBRACE) Fail(cp, "'{' expected");
                    GetOp(cp, P_OPENBRACE);

                    while (1) {
                        char *sname, *svalue;

                        if (Parse(cp) == P_CLOSEBRACE) break;
                        if (Parse(cp) != P_OPENBRACE) Fail(cp, "'{' expected");
                        GetOp(cp, P_OPENBRACE);
                        if (Parse(cp) != P_STRING) Fail(cp, "String expected");
                        sname = GetString(cp);
                        if (Parse(cp) != P_COMMA) Fail(cp, "',' expected");
                        GetOp(cp, P_COMMA);
                        if (Parse(cp) != P_STRING) Fail(cp, "String expected");
                        svalue = GetString(cp);
                        svalue = GetColor(cp, svalue);
                        if (DefineColor(sname, svalue) != 1)
                            Fail(cp, "DefineColor failed\n");
                        if (Parse(cp) != P_CLOSEBRACE) Fail(cp, "'}' expected");
                        GetOp(cp, P_CLOSEBRACE);
                        if (Parse(cp) != P_COMMA)
                            break;
                        else
                            GetOp(cp, P_COMMA);
                    }
                    if (Parse(cp) != P_CLOSEBRACE) Fail(cp, "'}' expected");
                    GetOp(cp, P_CLOSEBRACE);
                }
                break;
            case K_INCLUDE:
                {
                    char *fn;

                    if (Parse(cp) != P_STRING) Fail(cp, "String expected");
                    fn = GetString(cp);

                    if (LoadFile(cp.name, fn) != 0) Fail(cp, "Include of file '%s' failed", fn);
                    if (Parse(cp) != P_EOS) Fail(cp, "';' expected");
                    GetOp(cp, P_EOS);
                }
                break;
            default:
                Fail(cp, "Syntax error");
            }
            break;
        case P_EOF: return 0;
        default:    Fail(cp, "Syntax error");
        }
    }
}

static int PreprocessConfigFile(CurPos &cp) {
    char *wipe = NULL;
    char *wipe_end = NULL;
    bool rem_active = false;
    bool string_open = false;

    while (cp.c < cp.z) {
	switch(*cp.c)
	{
	case '#':
	    if (string_open == true) break;

	    rem_active = true;
	    break;

	case '\\':
            cp.c++;
            break;

	case '"':
	case '\'':
	    if (rem_active == true) break;

	    string_open = !string_open;
	    break;

	case '%':
	    if (string_open == true) break;
	    if (rem_active == true) break;

	    wipe = cp.c;
            wipe_end = NULL;

            if (cp.c + 8 < cp.z && strncmp(cp.c, "%define(", 8) == 0) {
                Word w;
                cp.c += 8;

                while (cp.c < cp.z && *cp.c != ')') {
                    GetWord(cp, w);
                    //printf("define '%s'\n", w);
                    DefineWord(w);
                    if (cp.c < cp.z && *cp.c != ',' && *cp.c != ')' )
                        Fail(cp, "unexpected: %c", cp.c[0]);
                    if (cp.c < cp.z && *cp.c == ',')
                        cp.c++;
                }
		cp.c++;
/*            } else if (cp.c + 6 && strcmp(cp.c, "undef(", 6) == 0) {
                Word w;
                cp.c += 6;

                while (cp.c < cp.z && *cp.c != ')') {
                    GetWord(cp, w);
                    UndefWord(w);
                }*/
            } else if (cp.c + 4 < cp.z && strncmp(cp.c, "%if(", 4) == 0) {
                Word w;
                int wasWord = 0;
                cp.c += 4;

                while (cp.c < cp.z && *cp.c != ')') {
                    int neg = 0;
                    if (*cp.c == '!') {
                        cp.c++;
                        neg = 1;
                    }
                    GetWord(cp, w);
                    if (DefinedWord(w))
                        wasWord = 1;
                    if (neg)
                        wasWord = wasWord ? 0 : 1;
                    /*if (wasWord)
                        printf("yes '%s'\n", w);
                    else
                        printf("not '%s'\n", w);*/

                    if (cp.c < cp.z && *cp.c != ',' && *cp.c != ')' )
                        Fail(cp, "unexpected: %c", cp.c[0]);
                    if (cp.c < cp.z && *cp.c == ',')
                        cp.c++;
                }
                cp.c++;
                if (!wasWord) {
                    int nest = 1;
                    while (cp.c < cp.z) {
                        if (*cp.c == '\n') {
                            cp.line++;
                            lntotal++;
                        } else if (*cp.c == '%') {
                            if (cp.c + 6 < cp.z &&
                                strncmp(cp.c, "%endif", 6) == 0)
                            {
                                cp.c += 6;
                                if (--nest == 0)
                                    break;
                            }
                            if (cp.c + 3 < cp.z &&
                                strncmp(cp.c, "%if", 3) == 0)
                            {
                                cp.c += 3;
                                ++nest;
                            }
                        } else if (*cp.c == '#') {
                            // we really shouldn't process hashed % directives
				while (cp.c < cp.z && *cp.c != '\n' )
					cp.c++;

                            // workaround to make line numbering correct
                            cp.line++;
                            lntotal++;
                        }
                        cp.c++;
                    }
                }
            } else if (cp.c + 6 < cp.z && strncmp(cp.c, "%endif", 6) == 0) {
                cp.c += 6;
            }
            if (cp.c < cp.z && *cp.c != '\n' && *cp.c != '\r')
                Fail(cp, "syntax error %30.30s", cp.c);

	    wipe_end = cp.c;

            // wipe preprocessor macros with space
	    while (wipe < wipe_end)
	    {
                *wipe++ = ' ';
	    }

	    break;

        case '\n':
	    cp.line++;
	    rem_active = false;
            string_open = false;
	    break;

	default:;
	}

        cp.c++;
    }

    return 0;
}

static int LoadFile(const char *WhereName, const char *CfgName, int Level) {
    int fd, rc;
    char *buffer = 0;
    struct stat statbuf;
    CurPos cp;
    char last[MAXPATH];
    char Cfg[MAXPATH];

    //fprintf(stderr, "Loading file %s %s\n", WhereName, CfgName);

    JustDirectory(WhereName, last, sizeof(last));

    if (IsFullPath(CfgName)) {
        strlcpy(Cfg, CfgName, sizeof(Cfg));
    } else {
        // here we will try relative to a number of places.
        // 1. User's .fte directory.
        // 2. System's "local config" directory.
        // 3. /usr/share/fte (FHS compliant - from Gentoo)
        // 3. Initial file's directory.
        // 4. Current directory.
        // This means that a user's directory will always win out,
        // allowing a given user to always be able to override everything,
        // followed by a system standard to override anything.

        // #'s 1 and 2 are unix-only.
#ifdef UNIX
        // 1. User's .fte directory.
        char tmp[MAXPATH];
        sprintf(tmp, "~/.fte/%s", CfgName);
        ExpandPath(tmp, Cfg, sizeof(Cfg));
        //fprintf(stderr, "Looking for %s\n", Cfg);
        if (!FileExists(Cfg)) {
            // 2. try "local config".
            sprintf(tmp, "%slocalconfig/%s", StartDir, CfgName);
            ExpandPath(tmp, Cfg, sizeof(Cfg));
            //fprintf(stderr, "Looking for %s\n", Cfg);
            if (!FileExists(Cfg)) {
                // 3. /usr/share/fte
                sprintf(tmp, "/usr/share/fte/%s", CfgName);
                ExpandPath(tmp, Cfg, sizeof(Cfg));
                if (!FileExists(Cfg)) {
                    sprintf(tmp, "%sconfig/%s", StartDir, CfgName);
                    ExpandPath(tmp, Cfg, sizeof(Cfg));
                    //fprintf(stderr, "Looking for %s\n", Cfg);
                    if (!FileExists(Cfg)) {
                        sprintf(tmp, "./%s", CfgName);
                        ExpandPath(tmp, Cfg, sizeof(Cfg));
                        //fprintf(stderr, "Looking for %s\n", Cfg);
                        if (!FileExists(Cfg)) {
                            fprintf(stderr, "Cannot find '%s' in:\n"
                                    "\t~/.fte,\n""\t%slocalconfig,\n\t/usr/share/fte,\n"
                                    "\t%sconfig, or\n"
                                    "\t.",
                                    CfgName, StartDir, StartDir);
                        }
                    }
                }
            }
        }
#else // UNIX
        SlashDir(last);
        strlcat(last, CfgName, sizeof(last));
        ExpandPath(last, Cfg, sizeof(Cfg));
#endif // UNIX
    }
    // puts(Cfg);

    //fprintf(stderr, "Loading file %s\n", Cfg);
    if ((fd = open(Cfg, O_RDONLY | O_BINARY)) == -1) {
        fprintf(stderr, "Cannot open '%s', errno=%d\n", Cfg, errno);
        return -1;
    }
    if (fstat(fd, &statbuf) != 0) {
        close(fd);
        fprintf(stderr, "Cannot stat '%s', errno=%d\n", Cfg, errno);
        return -1;
    }
    buffer = (char *) malloc((size_t)statbuf.st_size+1);
    if (buffer == NULL) {
        close(fd);
        return -1;
    }

    buffer[statbuf.st_size] = 0; // add null to end of buffer, NOTE: allocated statbuf.st_size + 1

    if (read(fd, buffer, (size_t)statbuf.st_size) != statbuf.st_size) {
        close(fd);
        free(buffer);
        return -1;
    }
    close(fd);

    cp.sz = statbuf.st_size;
    cp.a = cp.c = buffer;
    cp.z = cp.a + cp.sz;
    cp.line = 1;
    cp.name = Cfg;

    // preprocess configuration file
    rc = PreprocessConfigFile(cp);
    if (rc == -1)
        Fail(cp, "Preprocess failed");

    if (preprocess_only == true)
        printf("%s", cp.a);

    // reset pointers
    cp.a = cp.c = buffer;
    cp.z = cp.a + cp.sz;
    cp.line = 1;

    rc = ParseConfigFile(cp);
    // puts("End Loading file");
    if (Level == 0)
        PutNull(cp, CF_EOF);

    if (rc == -1)
        Fail(cp, "Parse failed");

    free(buffer);
    return rc;
}
