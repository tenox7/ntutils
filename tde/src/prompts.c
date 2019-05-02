/*
 * New editor name:  TDE, the Thomson-Davis Editor.
 * Author:           Frank Davis
 * Date:             June 5, 1992
 *
 * This file contains all user prompts in TDE.  Prompts were gathered into
 *  one file to make the job of translating English into other languages
 *  as easy as possible.
 *
 * jmh 980525: made it a C file.
 * jmh 980809: moved the help displays and menu into separate files.
 * jmh 030325: since I haven't been sent any translations, make the strings
 *              constant arrays for a slight optimisation.
 */

#include "tdestr.h"
#include "bj_ctype.h"


const char cb[]      = "Control-Break pressed";

const char eof_text[2][12] = { "Top of File",   /* Maximum of 11 characters */
                               "End of File" };

/*
 * block.c
 */
const char block1[]  = "a block is already defined in another file";

const char ltol[]    = "Error: line would be too long";
const char block2[]  = "can only fill box blocks";
const char block3a[] = "can only number box blocks";
const char block3b[] = "cannot swap stream blocks";
const char block4[]  = "not enough memory for block";
const char block5[]  = "cannot overlay stream blocks";

const char * const block6[][2]  = {
    { "Save block to: ",        "Save file to: "        },
    { "writing block to '",     "writing file to '"     },
    { "appending block to '",   "appending file to '"   },
    { "could not write block",  "could not write file"  },
    { "could not append block", "could not append file" }
};
const char block7[]  = "File exists. Overwrite or Append?";

const char block13[] = "Print file or block?";
const char block14[] = /* cf. BLOCK14_LINE_SLOT & BLOCK14_SUM_SLOT */
      "Printing line         of         Press Control-Break to cancel.";

const char block15[]  = "Enter character to fill block (ESC to exit): ";
/* jmh 030305 */
const char block15a[] = "Enter pattern to fill block (ESC to exit): ";

const char block20[]  = "can only expand tabs in line blocks";
const char block20a[] = "can only compress tabs in line blocks";

const char block21[] = "can only trim trailing space in line blocks";

const char block23[] = "cannot sort stream blocks";
const char block24[] = "block not marked";

const char block25[] = "can only e-mail reply in line blocks";

/* jmh 980731: added following three. */
const char block26[] = "can only border box blocks";
const char block27[] = "Border style (F1 = help): ";
const char block28[] = "invalid style";
const char block28a[] = "box not big enough for border";  /* jmh 030305 */

const char block29[] = "cannot justify stream blocks"; /* jmh 980810 */
const char block30[] = "cannot indent stream blocks";  /* jmh 980811 */
const char block31[] = "Indentation level: ";          /* jmh 980811 */

/* jmh 991029 */
const char block32[]  =
      "Select swap lines with Ctrl+Up/Down, Up/Down, Home/End, Enter/Esc.";
const char block33a[] =
      "Select swap box with Ctrl+Left/Right, Left/Right, Home/End, Enter/Esc.";
const char block33b[] =
      "Select swap box with Ctrl+Up, Up/Down, Enter/Esc."; /* jmh 991108 */

const char block34[] = "can only sum box blocks"; /* jmh 991112 */
const char block35[] = "The total is %ld";        /* ditto */

const char block36[] = "No block comment defined.";            /* jmh 030304 */
const char block37[] = "No line comment defined.";             /* ditto */
const char block38[] = "cannot line comment stream blocks";    /* jmh 030302 */


/*
 * config.c
 *
 * jmh 980721: added config0
 */
const char config0[]  = ".  line:  ";
const char config1[]  = "Setting without value";
const char config2[]  = "Two-key cannot be TwoCharKey";
const char config3[]  = "Unrecognized function";
const char config6[]  = "Color number out of range";
const char config6a[] = "Unrecognized color name";     /* added by jmh 990420 */
const char config7[]  = "Off/On error";
const char config8[]  = "Tab error";
const char config9[]  = "Left margin error";
const char config10[] = "Paragraph margin error";
const char config11[] = "Right margin error";
const char config12[] = "CRLF or LF error";
const char config13[] = "Word wrap error";
const char config14[] = "Cursor size error";
const char config15[] = "Control Z error";
const char config16[] = "Inflate tabs error";
const char config17[] = "EOL display error";
const char config18[] = "Initial Case Mode error";
const char config19[] = "Unknown mode";
const char config20[] = "Unrecognized editor setting";
const char config21[] = "Unterminated quote";
const char config22[] = "Unrecognized key";
const char config23[] = "No more room in macro buffer";
#if 0 //defined( __UNIX__ )
const char config24[] = "Error parsing color pair";
#endif
const char config25[] = "Invalid or missing pseudo-macro combination";
const char config26[] = "Unexpected end of file";
const char config27[] = "Directory sort error";
const char config28[] = "Second key of two-key missing";
const char config29[] = "Invalid frame style";
const char config30[] = "Scancode requires pairs of keys";
const char config31[] = "Unrecognized key or function";        /* jmh 021025 */

const char config32[] = " <Empty>";       /* User menu has no items */
const char config33[] = "\x0Language";    /* User sub-menu label */
const char config34[] = "Too many menus";
const char config35[] = "Menu item not in a menu";


/*
 * diff
 */
const char diff_prompt0[] = "Two visible windows required for diff'ing";
const char diff_prompt1[] = "DIFF windows are the same"; /* jmh 031202 */
const char diff_prompt4[] =
      "DIFF:  Diffed until EOF(s).  No diff's were found";
const char diff_prompt5[] = "DIFF windows not defined";
const char diff_prompt6[] = "' is not visible";   /* see win21a */


/*
 * dir list
 */
const char dir1[]  = "Search path or pattern : ";
const char dir2[]  = "Invalid path or file name";

const char dir3[]  = "Directory : ";
const char dir4[]  = "File name : ";
const char dir5[]  = "File time : ";
const char dir5a[] = "%2d %0M %Y   %2H:%0n:%0s";
const char dir6[]  = "File size : ";
const char dir7[]  = "File count : ";
const char dir8[]  = "Cursor keys move.  Enter selects file or new directory";

const char dir10[] = "Language : ";
const char dir11[] = "Pattern  : ";
const char dir12[] = "Case     : ";
const char dir13[] = "Count : ";
const char dir14[][8] = { "inherit",
                          "ignore ",
                          "match  " };


/*
 * ed.c
 */
const char ed1[]  = "line too long to add";
const char ed2[]  = "cannot insert more characters";
const char ed3[]  = "no more room to add";

const char ed4[]  = "cannot combine lines";

const char ed5[]  = "cannot duplicate line";

/* added by jmh 030304. cf. ED7C_SLOT */
const char ed7c[] =
      "Tab size:           (Right/Down increase, Left/Up decrease)";
const char ed8a[]  = "logical tab size too long";
const char ed8b[]  = "physical tab size too long";

const char ed10[] = "Left margin out of range";
const char ed12[] = "Right margin out of range";
const char ed14[] = "Paragraph margin out of range";

const char ed15[]  = "File name to edit : ";
const char ed15a[] = "File name to insert : ";         /* added by jmh */
const char ed15b[] = "File name, path or pattern : ";  /* added by jmh 021024 */

const char ed16[] = "Macro execution halted:  Internal stack overflow";
const char ed17[] = "Macro execution halted:  Internal stack underflow";

const char ed18[] = "BINARY line length (0 for text) : "; /* jmh 981127 */

const char ed19[] = "Repeat count: ";                  /* added by jmh 980726 */
const char ed20[] = "Press the key to be repeated.";   /* ditto */

const char ed21[] = "Press the key for cursor update direction."; /*jmh 981129*/

const char paused1[]  = "Paused:";
const char paused1a[] = "  ";  /* jmh 031114: blank extra main15 characters */
const char paused2[]  = "  Press ESC to halt macro   ";

/*
 * jmh 010528 - display following error message before exiting.
 *              (Invalid regx is handled directly in regx.c.)
 */
const char ed22[]  = "Missing pattern or file for grep";
const char ed23[]  = "Unknown option";
const char ed23a[] = "Invalid binary length";          /* added by jmh 021024 */
const char ed23b[] = "Invalid tab size";               /* ditto */
const char ed23c[] = "Missing macro";                  /* ditto */

const char ed24[] = "Save workspace to: ";             /* added by jmh 020802 */
const char ed25[] = "Invalid workspace file.";         /* added by jmh 020817 */

const char ed26[] = "Cannot resize a top window to display CWD."; /*jmh 030226*/

const char ed27a[] = "Context help file '";
const char ed27b[] = "' not available";


/*
 * findrep.c
 */
const char find1[]  = "Prompt before replace?";
const char find2[]  = "(R)eplace  (S)kip  (E)xit";
const char find3[]  = "Search has wrapped.  Continue or quit?";
const char find4[]  = "String to find: ";
const char find5a[] = "string \"";
const char find5b[] = "\" not found";
const char find8[]  = "string not found";
const char find6[]  = "find pattern not defined";

const char find7[][11] = { "          ",
                           "wrapped...",
                           "searching ",
                           "replacing ",
                           "nfa choked",
                           "diffing...",
                           "Next Key..",
                           "undo one  ",    /* these two strings */
                           "undo group",    /*  must be consecutive! */
                           "undo place",    /* these two strings */
                           "undo move ",    /*  must be consecutive! */
                           "tallying..",
                           "changed...",
                           "sorting..." };


const char find11[] = "Position (line:col or +offset) : ";
const char find12[] = "must be in the range 1 - ";

const char find13[] = "Search forward or backward?";

const char find17[] = "Results window cannot append to itself"; /* jmh 031120 */

const char find18a[] = "ISearch (ignore): ";           /* added by jmh 021028 */
const char find18b[] = "ISearch (match):  ";           /* ditto */
const char find18c[] = "- not found";                  /* ditto */


/*
 * hwind.c
 */
const char file_win[] = "F=   W="; /* changed from file_win_mem by jmh 981130 */
const char mem_eq[]   = "m=";

const char tabs[]      = "Tabs=";
const char smart[2]    = "FS";          /* jmh 030328: characters,  */
const char tab_mode[3] = "DIR";         /*              not strings */

const char indent_mode[][7] = { "      ",
                                "Indent" };

const char case_mode[][7] = { "Ignore",
                              "Match " };

const char sync_mode[][5] = { "    ",
                              "Sync" };

const char ww_mode[][3] = { "  ",
                            "FW",
                            "DW" };

char graphic_mode[] = "Graphic ?";      /* cf. GRAPHIC_SLOT */

const char * const cur_dir_mode[] = {
      "",                               /* Right - not actually displayed */
      "Left",                           /* these three strings should not */
      "Down",                           /* be any longer than the memory  */
      "Up" };                           /* display (8 characters)         */

const char draw_mode[] = "Draw ";       /* needs to be 5 characters */

/*
 * Incidentally, these need to be 4 characters wide.  The previous file mode
 * should leave no residue in the lite bar, when toggling thru the modes.
 */
const char crlf_mode[][5] = { "crlf",
                              "lf  ",
                              "BIN " };

const char ruler_help[] =
      "Arrow keys move ruler, Ctrl+Arrows/Home/End/PgUp/PgDn move window";
const char ruler_bad[]  = "Two lines are required for the popup ruler";


/*
 * main.c
 */
const char main2[]   = "Warning: ";
const char main3[]   = " : press a key";

const char main4[]   = "Out of memory";
const char main4a[]  = "Not enough base memory.";      /* jmh 011124 */

const char main6[]   = "File is write protected.  Overwrite anyway?";

const char main7a[]  = "File '";
const char main7b[]  = "' not found or error loading file";
const char main9[]   = "error reading file '";
const char main10b[] = "' too big";

const char main11[]  = "Press the key that will play back this recording : ";
const char main12[]  = "Cannot assign a recording to this ";
const char main12a[] = "key";
const char main12b[] = "combination";
const char main13[]  = "No more room in recording buffer";
const char main14[]  = "Overwrite recording?";

const char main15[]  = "Recording";
const char main16[]  = "  Avail strokes =      ";
const char main17[]  = "Macro requires a block to be marked"; /* jmh 050709 */
const char main18[]  = "Macro requires a different type of block"; /* ditto */
const char main19[]  = "Name for macro file name: ";
const char main20a[] = "; TDE global %s\n\n";
const char main20b[] = "; TDE \"%s\" %s\n\n";
const char main20c[] = "macros, written %H:%0n, %d %M, %Y";
const char main21[]  = "Search path or file name for macro file : ";
/* jmh 010604 */
const char main21a[] = "Search path or file name for config file : ";

/* Added by jmh */
const char main22[]  = "Quit- and file-all are disabled during redirection";

/* added by jmh 030320 */
const char main23[]  = "Discard changes and reload";     /* Revert */
const char main23a[] = "File no longer exists";
const char main24[]  = "File has been modified. Reload"; /* Shell - visible */
const char main25b[] = "' has been modified. Reload";    /* Shell - hidden */


/*
 * regx.c
 */
const char reg1[]   = "Regular expression search (F1 = help): ";
const char reg2[]   = "unmatched open paren";
const char reg3[]   = "unmatched close paren";
const char reg4[]   = "char \'\\\' at end of string is not escaped";
const char reg5[]   = "class is not defined properly";
const char reg6[]   = "unmatched open bracket";
const char reg7[]   = "out of heap for class definition";
const char reg8[]   = "operator *, +, or ? error";
const char reg9[]   = "unmatched close bracket";
const char reg10[]  = "incomplete range in character class";
const char reg11[]  = "can't parse two operators in a row";
const char reg12[]  = "backreference not yet defined";
const char reg13[]  = "unrecognized character after (?";
const char reg14[]  = "assertion must be last";


/*
 * syntax.c
 */
const char syntax1[]   = "Unrecognized syntax setting";
const char syntax2[]   = "Expecting \"pattern\" after \"language\"";
const char syntax3[]   = "Expecting a color setting";
/*
 * jmh 990421: syntax4 used to be "Expecting a color", but with the
 *             new color parsing routine, it was made redundant.
 */
const char syntax4[]   = "Syntax macro requires two-key";
const char syntax5[]   = "Setting is in the wrong place";
const char syntax6[]   = "Case setting not recognized";
const char syntax7[]   = "Expecting one non-alphanumeric character";
const char syntax8a[]  = "Only one or two characters allowed";
const char syntax8b[]  = "Only one to four characters allowed"; /* jmh 011122 */
const char syntax9[]   = "Only one character allowed";
const char syntax10[]  = "Number requires a prefix or suffix";
const char syntax11[]  = "Number cannot be both prefix and suffix";
const char syntax12[]  = "Syntax highlighting language: ";
const char syntax13a[] = "Language '";
const char syntax13b[] = "' not found";
const char syntax14a[] = "Only two multi-line comments allowed"; /*jmh 011123*/
const char syntax14b[] = "Only two line comments allowed";       /*jmh 011123*/


/*
 * utils.c
 */
const char utils4[]   = "Sort Ascending or Descending?";

const char utils6[]   = "Saving '";
const char utils7b[]  = "' is Read Only";
const char utils7c[]  = "' is marked read-only. Save anyway?"; /* jmh 021023 */
const char utils7d[]  = "' is marked scratch. Save anyway?";   /* jmh 030730 */
const char utils8[]   = "cannot write to '";
const char utils9[]   = "New file name: ";
const char utils10[]  = "Overwrite existing file?";

const char utils12[]  = "Abandon changes?";
const char utils12a[] = "Abandon all files?";  /* jmh */

/* jmh 010523. cf. UTILS13_NO_SLOT */
      char utils13[]  = "Marker   not set in this file";

#if defined( __UNIX__ )
const char utils15[]  = "New file attributes not set.  Wrong owner, perhaps";
#else
const char utils15[]  = "New file attributes not set";
#endif

const char utils16[]  = "No dirty lines found";
const char utils18a[] = "No browser window";     /* jmh 031116 */
const char utils18b[] = "No browse lines found"; /* jmh 031117 */

const char utils19[]  = "Command requires a file";
const char utils20[]  = "Enter parameter for command : "; /* jmh 031029 */


const char * const time_ampm[2] = { "am", "pm" };

/*
 * following three added by jmh 980521
 */
const char utils17[]  = "Enter new stamp format (F1 = help) : ";

const char * const months[2][12] = {
   { "January", "February", "March",     "April",   "May",      "June",
     "July",    "August",   "September", "October", "November", "December"
   },
   { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
   }
};
const int  longest_month = 9;   /* jmh 010624 - length of longest month name */

const char * const days[2][7] = {
   { "Sunday",   "Monday", "Tuesday", "Wednesday",
     "Thursday", "Friday", "Saturday"
   },
   { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" }
};
const int  longest_day = 9;     /* jmh 010624 - length of longest day name */

/*
 * status strings added by jmh 990410
 */
const char status0[] = "Status";       /* jmh 991021 */
const char * const status1a[] = { "Marker %d is not set.",
                                  "Marker %d is set at %ld:%d." };
const char * const status1b[] = { "The previous position is not set.",
                                  "The previous position is at %ld:%d." };
const char * const status_block[] = { "", "Box", "Line", "Stream" };
const char * const status2[] = { "No block is marked.",
                                 "%s block is marked at %ld:%d - %ld:%d",
                                 "Lines %ld through %ld are marked",
                                 "%s block is marked from %ld:%d to %ld:%d",
                                 "%s block is marked from %ld:%d to %ld",
                                 "%s block is marked in another file." };
const char * const status2a[] = { "lines", "line", "columns", "column" };
const char * const status2b[] = { "(%ld %s, %d %s).",
                                  "(%ld lines)." }; /* not used for 1 line */
const char * const status3[] = { "No breakpoint is set.",
                                 "The breakpoint is set at line %ld." };
const char * const status4[] = { "No syntax highlighting information.",
                                 "\"%s\" syntax highlighting." };
const char * const status5[] = { "1 window references this file.",
                                 "%d windows reference this file." };
const char stat_time[] = "%Y-%0m-%0d %0H:%0n:%0s";
const char status6[] = "File time:    %s";             /* jmh 030320 */
const char status7[] = "Current time: %s";

/*
 * statistics strings added by jmh 010605
 */
const char stats0[] = "Statistics";
const char * const stats1[3][3] = { { "Lines",  "Words", "Strings" },
                                    { "Blank", "Al-num",   "Chars" },
                                    { "Total", "Symbol",   "Space" } };
const char * const stats[] = {  "min",
                                "max",
                                "avg",
                               "%age" };
const char stats4[] = "File size:   %*ld (%s)";
const char stats5[] = "Memory size: %ld";

const char * const eol_mode[] = { "CR+LF",       /* jmh 991111 */
                                  "LF",
                                  "Binary" };

const char pipe_file[]    = "pipe";             /* jmh 030226 */
const char scratch_file[] = "scratch";          /* jmh 030226 */


/*
 * window.c
 */
const char win1[]  = "move cursor up first";
const char win1a[] = "move cursor down first"; /* added by jmh 991109 */
const char win1b[] = "window too small";       /* jmh 030323 */

const char win2[] = "move cursor right first";
const char win3[] = "move cursor left first";

const char win4[] =
      "Use cursor keys to change window size.  Press Return when done.";
const char win5[] = "too many windows";
const char win6[] = "cannot resize top window";

const char win7[] = "cannot close current window";

const char win8[]  = "Invalid path or file name";
const char win9[]  = "No more files to load";
const char win9b[] = "Pattern not found";       /* added by jmh 031116 */
const char win9a[] = "Use the <Grep> key for more files";      /* jmh */

const char win18[]  = "Gathering file names..."; /* jmh 070501 */
const char win19[]  = "Searching:  ";

const char win20[] = "Goto window: ";           /* added by jmh 990502 */
const char win21a[] = "window '";
const char win21b[] = "' does not exist";
const char win21c[] = "' is visible";   /* added by jmh 030401 */
const char win22a[] = "file name '";
const char win22b[] = "' not matched";

const char win23[] = "Window title: ";  /* added by jmh 030331 */


/*
 * wordwrap.c
 */
const char ww1[]  = "line would be too long";
const char ww2[]  = "line too long to format";


/************************************************************************/
/* Editor:      TDE, the Thomson-Davis Editor
 * Filename:    prompts2.h
 * Compiled by: Byrial Jensen
 *
 * This file contains various prompts and other strings, collected to
 * to ease making versions for other languages.
 * Here is: What is "forgotten" in prompts.h
 *          A string with the alphabet used for window names
 *          Tables used by functions in myctype.c and macros in myctype.h
 * It is used with changed versions of several of the TDE source files
 */


/*
 * These letters are used to differentiate windows in the same file.
 *  Some users may prefer the sequence of their alphabet, instead of
 *  this English sequence.
 */
const char windowletters[] =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";


/*
 * I'll give the English ASCII and Extended-ASCII keyboards a try, Frank.
 * jmh 030403: added Latin-1 and made it the Linux default.
 */
#if defined( __UNIX__ )  &&  !defined( PC_CHARS )
#define CHAR_SET   LATIN_1
#elif 0
#define CHAR_SET   LATIN_3
#else
#define CHAR_SET   CP437
#endif


/*
 * First some defines which only purpose is to make the next table
 * more compact and easy to see, Byrial.
 */
#define  S   BJ_space
#define  D   (BJ_digit | BJ_xdigit)     /* bit mask for digit */
#define  U   BJ_upper
#define  L   BJ_lower
#define  H   (BJ_xdigit | U)            /* bit mask for uppercase hex digit */
#define  h   (BJ_xdigit | L)            /* bit mask for lowercase hex digit */
#define  C   BJ_cntrl
#define  P   BJ_punct
#define  s   (S | C)                    /* counts both for space and control */

/*
 * State corresponding uppercase letter for each lowercase letter,
 * and corresponding lowercase letter for each uppercase letter.
 * If the right corresponding letter does not exist in the character set
 * state best replacement, possible the letter self in wrong case.
 * Byrial Jensen.
 */

#if CHAR_SET==CP437

char bj_ctype[256] = {

    C,C,C,C, C,C,C,C,  C,s,s,s, s,s,C,C,  /* 00h-0Fh    0- 15 */
    C,C,C,C, C,C,C,C,  C,C,C,C, C,C,C,C,  /* 10h-1Fh   16- 31 */
    S,P,P,P, P,P,P,P,  P,P,P,P, P,P,P,P,  /* 20h-2Fh   32- 47 */
    D,D,D,D, D,D,D,D,  D,D,P,P, P,P,P,P,  /* 30h-3Fh   48- 63 */

    P,H,H,H, H,H,H,U,  U,U,U,U, U,U,U,U,  /* 40h-4Fh   64- 79 */
    U,U,U,U, U,U,U,U,  U,U,U,P, P,P,P,P,  /* 50h-5Fh   80- 95 */
    P,h,h,h, h,h,h,L,  L,L,L,L, L,L,L,L,  /* 60h-6Fh   96-111 */
    L,L,L,L, L,L,L,L,  L,L,L,P, P,P,P,C,  /* 70h-7Fh  112-127 */

    U,L,L,L, L,L,L,L,  L,L,L,L, L,L,U,U,  /* 80h-8Fh  128-143 */
    U,L,U,L, L,L,L,L,  L,U,U,P, P,P,P,P,  /* 90h-9Fh  144-159 */
    L,L,L,L, L,U,P,P,  P,P,P,P, P,P,P,P,  /* A0h-AFh  160-175 */
    P,P,P,P, P,P,P,P,  P,P,P,P, P,P,P,P,  /* B0H-BFh  176-191 */

    P,P,P,P, P,P,P,P,  P,P,P,P, P,P,P,P,  /* C0h-CFh  192-207 */
    P,P,P,P, P,P,P,P,  P,P,P,P, P,P,P,P,  /* D0h-DFh  208-223 */
    L,L,U,L, U,L,L,L,  U,L,U,L, L,L,P,P,  /* E0h-EFh  224-239 */
    P,P,P,P, P,P,P,P,  P,P,P,P, P,P,P,P   /* F0h-FFh  240-255 */
};

text_t upper_lower[256] = {

     0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,
     0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,
     0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,
     0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,

     0 ,'a','b','c', 'd','e','f','g',  'h','i','j','k', 'l','m','n','o',
    'p','q','r','s', 't','u','v','w',  'x','y','z', 0 ,  0 , 0 , 0 , 0 ,
     0 ,'A','B','C', 'D','E','F','G',  'H','I','J','K', 'L','M','N','O',
    'P','Q','R','S', 'T','U','V','W',  'X','Y','Z', 0 ,  0 , 0 , 0 , 0 ,

    'á','ö','ê','A', 'é','A','è','Ä',  'E','E','E','I', 'I','I','Ñ','Ü',
    'Ç','í','ë','O', 'ô','O','U','U',  'Y','î','Å', 0 ,  0 , 0 , 0 , 0 ,
    'A','I','O','U', '•','§', 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,
     0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,

     0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,
     0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,
    '‡','·','‚','„', 'Â','‰','Ê','Á',  'Ì','È','Ï','Î', 'Í','Ë', 0 , 0 ,
     0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0
};

#elif CHAR_SET==LATIN_1

char bj_ctype[256] = {

    C,C,C,C, C,C,C,C,  C,s,s,s, s,s,C,C,  /* 00h-0Fh    0- 15 */
    C,C,C,C, C,C,C,C,  C,C,C,C, C,C,C,C,  /* 10h-1Fh   16- 31 */
    S,P,P,P, P,P,P,P,  P,P,P,P, P,P,P,P,  /* 20h-2Fh   32- 47 */
    D,D,D,D, D,D,D,D,  D,D,P,P, P,P,P,P,  /* 30h-3Fh   48- 63 */

    P,H,H,H, H,H,H,U,  U,U,U,U, U,U,U,U,  /* 40h-4Fh   64- 79 */
    U,U,U,U, U,U,U,U,  U,U,U,P, P,P,P,P,  /* 50h-5Fh   80- 95 */
    P,h,h,h, h,h,h,L,  L,L,L,L, L,L,L,L,  /* 60h-6Fh   96-111 */
    L,L,L,L, L,L,L,L,  L,L,L,P, P,P,P,C,  /* 70h-7Fh  112-127 */

    0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  /* 80h-8Fh  128-143 */
    0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  /* 90h-9Fh  144-159 */
    P,P,P,P, P,P,P,P,  P,P,P,P, P,P,P,P,  /* A0h-AFh  160-175 */
    P,P,P,P, P,P,P,P,  P,P,P,P, P,P,P,P,  /* B0H-BFh  176-191 */

    U,U,U,U, U,U,U,U,  U,U,U,U, U,U,U,U,  /* C0h-CFh  192-207 */
    U,U,U,U, U,U,U,P,  U,U,U,U, U,U,U,L,  /* D0h-DFh  208-223 */
    L,L,L,L, L,L,L,L,  L,L,L,L, L,L,L,L,  /* E0h-EFh  224-239 */
    L,L,L,L, L,L,L,P,  L,L,L,L, L,L,L,L,  /* F0h-FFh  240-255 */
};

text_t upper_lower[256] = {

     0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,
     0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,
     0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,
     0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,

     0 ,'a','b','c', 'd','e','f','g',  'h','i','j','k', 'l','m','n','o',
    'p','q','r','s', 't','u','v','w',  'x','y','z', 0 ,  0 , 0 , 0 , 0 ,
     0 ,'A','B','C', 'D','E','F','G',  'H','I','J','K', 'L','M','N','O',
    'P','Q','R','S', 'T','U','V','W',  'X','Y','Z', 0 ,  0 , 0 , 0 , 0 ,

     0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,
     0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,
     0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,
     0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,

    '‡','·','‚','„', '‰','Â','Ê','Á',  'Ë','È','Í','Î', 'Ï','Ì','Ó','Ô',
    '','Ò','Ú','Û', 'Ù','ı','ˆ', 0 ,  '¯','˘','˙','˚', '¸','˝','˛','ﬂ',
    '¿','¡','¬','√', 'ƒ','≈','∆','«',  '»','…',' ','À', 'Ã','Õ','Œ','œ',
    '–','—','“','”', '‘','’','÷', 0 ,  'ÿ','Ÿ','⁄','€', '‹','›','ﬁ','Y',
};

#else

/*
 * This is Byrial's definition for ISO 8859-3 (Latin-3).
 */
char bj_ctype[256] = {

    C,C,C,C, C,C,C,C,  C,s,s,s, s,s,C,C,  /* 00h-0Fh    0- 15 */
    C,C,C,C, C,C,C,C,  C,C,C,C, C,C,C,C,  /* 10h-1Fh   16- 31 */
    S,P,P,P, P,P,P,P,  P,P,P,P, P,P,P,P,  /* 20h-2Fh   32- 47 */
    D,D,D,D, D,D,D,D,  D,D,P,P, P,P,P,P,  /* 30h-3Fh   48- 63 */

    P,H,H,H, H,H,H,U,  U,U,U,U, U,U,U,U,  /* 40h-4Fh   64- 79 */
    U,U,U,U, U,U,U,U,  U,U,U,P, P,P,P,P,  /* 50h-5Fh   80- 95 */
    P,h,h,h, h,h,h,L,  L,L,L,L, L,L,L,L,  /* 60h-6Fh   96-111 */
    L,L,L,L, L,L,L,L,  L,L,L,P, P,P,P,C,  /* 70h-7Fh  112-127 */

    0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  /* 80h-8Fh  128-143 */
    0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  /* 90h-9Fh  144-159 */
    s,U,P,P, P,0,U,P,  P,U,U,U, U,C,0,U,  /* A0h-AFh  160-175 */
    P,L,P,P, P,P,L,P,  P,L,L,L, L,P,0,L,  /* B0H-BFh  176-191 */

    U,U,U,0, U,U,U,U,  U,U,U,U, U,U,U,U,  /* C0h-CFh  192-207 */
    0,U,U,U, U,U,U,P,  U,U,U,U, U,U,U,U,  /* D0h-DFh  208-223 */
    L,L,L,0, L,L,L,L,  L,L,L,L, L,L,L,L,  /* E0h-EFh  224-239 */
    0,L,L,L, L,L,L,P,  L,L,L,L, L,L,L,P   /* F0h-FFh  240-255 */
};

text_t upper_lower[256] = {

     0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,
     0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,
     0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,
     0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,

     0 ,'a','b','c', 'd','e','f','g',  'h','i','j','k', 'l','m','n','o',
    'p','q','r','s', 't','u','v','w',  'x','y','z', 0 ,  0 , 0 , 0 , 0 ,
     0 ,'A','B','C', 'D','E','F','G',  'H','I','J','K', 'L','M','N','O',
    'P','Q','R','S', 'T','U','V','W',  'X','Y','Z', 0 ,  0 , 0 , 0 , 0 ,

     0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,
     0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,   0 , 0 , 0 , 0 ,  0 , 0 , 0 , 0 ,
     0 ,'±', 0 , 0 ,  0 , 0 ,'∂', 0 ,   0 ,'π','∫','ª', 'º', 0 , 0 ,'ø',
     0 ,'°', 0 , 0 ,  0 , 0 ,'¶', 0 ,   0 ,'©','™','´', '¨', 0 , 0 ,'Ø',

    '‡','·','‚', 0 , '‰','Â','Ê','Á',  'Ë','È','Í','Î', 'Ï','Ì','Ó','Ô',
     0 ,'Ò','Ú','Û', 'Ù','ı','ˆ', 0 ,  '¯','˘','˙','˚', '¸','˝','˛','ﬂ',
    '¿','¡','¬', 0 , 'ƒ','≈','∆','«',  '»','…',' ','À', 'Ã','Õ','Œ','œ',
     0 ,'—','“','”', '‘','’','÷', 0 ,  'ÿ','Ÿ','⁄','€', '‹','›','ﬁ', 0
};

#endif

#undef S        /* These letters where only defined to make the */
#undef D        /* above table as compact as possible.          */
#undef U        /* Now they can be undef'ed, Byrial             */
#undef L
#undef H
#undef h
#undef C
#undef P
#undef s


/*
 *   those who use special accented characters as part of normal character set
 *     in text do not particularly care for a straight ASCII sort sequence.
 */

#if CHAR_SET==LATIN_1

SORT_ORDER sort_order = {
               /* ignore case */
   { '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07',
     '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d', '\x0e', '\x0f',
     '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
     '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f',
     ' ', '!', '\"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',',
     '-', '.', '/', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
     ':', ';', '<', '=', '>', '?',
     '@', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
     'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '[',
    '\\', ']', '^', '_', '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
     'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
     'x', 'y', 'z', '{', '|', '}', '~', '', 'Ä', 'Å', 'Ç', 'É', 'Ñ', 'Ö',
     'Ü', 'á', 'à', 'â', 'ä', 'ã', 'å', 'ç', 'é', 'è', 'ê', 'ë', 'í', 'ì',
     'î', 'ï', 'ñ', 'ó', 'ò', 'ô', 'ö', 'õ', 'ú', 'ù', 'û', 'ü', '†', '°',
     '¢', '£', '§', '•', '¶', 'ß', '®', '©', '™', '´', '¨', '≠', 'Æ', 'Ø',
     '∞', '±', '≤', '≥', '¥', 'µ', '∂', '∑', '∏', 'π', '∫', 'ª', 'º', 'Ω',
     'æ', 'ø', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'c', 'e', 'e', 'e', 'e',
     'i', 'i', 'i', 'i', '–', 'n', 'o', 'o', 'o', 'o', 'o', '◊', 'o', 'u',
     'u', 'u', 'u', 'y', '˛', 'ﬂ', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'c',
     'e', 'e', 'e', 'e', 'i', 'i', 'i', 'i', '', 'n', 'o', 'o', 'o', 'o',
     'o', '˜', 'o', 'u', 'u', 'u', 'u', 'y', '˛', 'y' },
               /* match case */
   { '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07',
     '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d', '\x0e', '\x0f',
     '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
     '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f',
     ' ', '!', '\"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',',
     '-', '.', '/', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
     ':', ';', '<', '=', '>', '?',
     '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
     'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[',
    '\\', ']', '^', '_', '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
     'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
     'x', 'y', 'z', '{', '|', '}', '~', '', 'Ä', 'Å', 'Ç', 'É', 'Ñ', 'Ö',
     'Ü', 'á', 'à', 'â', 'ä', 'ã', 'å', 'ç', 'é', 'è', 'ê', 'ë', 'í', 'ì',
     'î', 'ï', 'ñ', 'ó', 'ò', 'ô', 'ö', 'õ', 'ú', 'ù', 'û', 'ü', '†', '°',
     '¢', '£', '§', '•', '¶', 'ß', '®', '©', '™', '´', '¨', '≠', 'Æ', 'Ø',
     '∞', '±', '≤', '≥', '¥', 'µ', '∂', '∑', '∏', 'π', '∫', 'ª', 'º', 'Ω',
     'æ', 'ø', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'C', 'E', 'E', 'E', 'E',
     'I', 'I', 'I', 'I', '–', 'N', 'O', 'O', 'O', 'O', 'O', '◊', 'O', 'U',
     'U', 'U', 'U', 'Y', 'ﬁ', 'ﬂ', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'c',
     'e', 'e', 'e', 'e', 'i', 'i', 'i', 'i', '', 'n', 'o', 'o', 'o', 'o',
     'o', '˜', 'o', 'u', 'u', 'u', 'u', 'y', '˛', 'y' },
};

#else

/*
 *   this modified ASCII sorting sequence for special accent characters is
 *     useful with English, Esperanto, and French.
 */
SORT_ORDER sort_order = {
               /* ignore case */
   { '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07',
     '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d', '\x0e', '\x0f',
     '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
     '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f',
     ' ', '!', '\"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',',
     '-', '.', '/', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
     ':', ';', '<', '=', '>', '?',
     '@', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
     'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '[',
    '\\', ']', '^', '_', '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
     'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
     'x', 'y', 'z', '{', '|', '}', '~', '', 'c', 'u', 'e', 'a', 'a', 'a',
     'a', 'c', 'e', 'e', 'e', 'i', 'i', 'i', 'a', 'a', 'e', 'a', 'a', 'o',
     'o', 'o', 'u', 'u', 'y', 'o', 'u', 'õ', 'ú', 'ù', 'û', 'ü', 'a', 'i',
     'o', 'u', 'n', 'n', '¶', 'ß', '®', '©', '™', '´', '¨', '≠', 'Æ', 'Ø',
     '∞', '±', '≤', '≥', '¥', 'µ', '∂', '∑', '∏', 'π', '∫', 'ª', 'º', 'Ω',
     'æ', 'ø', '¿', '¡', '¬', '√', 'ƒ', '≈', '∆', '«', '»', '…', ' ', 'À',
     'Ã', 'Õ', 'Œ', 'œ', '–', '—', '“', '”', '‘', '’', '÷', '◊', 'ÿ', 'Ÿ',
     '⁄', '€', '‹', '›', 'ﬁ', 'ﬂ', '‡', '·', '‚', '„', 'Â', 'Â', 'Ê', 'Á',
     'Ì', 'È', 'Ï', 'Î', 'Ï', 'Ì', 'Ó', 'Ô', '', 'Ò', 'Ú', 'Û', 'Ù', 'ı',
     'ˆ', '˜', '¯', '˘', '˙', '˚', '¸', '˝', '˛', 'ˇ' },
               /* match case */
   { '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07',
     '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d', '\x0e', '\x0f',
     '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
     '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f',
     ' ', '!', '\"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',',
     '-', '.', '/', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
     ':', ';', '<', '=', '>', '?',
     '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
     'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[',
    '\\', ']', '^', '_', '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
     'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
     'x', 'y', 'z', '{', '|', '}', '~', '', 'C', 'u', 'e', 'a', 'a', 'a',
     'a', 'c', 'e', 'e', 'e', 'i', 'i', 'i', 'A', 'A', 'E', 'a', 'A', 'o',
     'o', 'o', 'u', 'u', 'y', 'O', 'U', 'õ', 'ú', 'ù', 'û', 'ü', 'a', 'i',
     'o', 'u', 'n', 'N', '¶', 'ß', '®', '©', '™', '´', '¨', '≠', 'Æ', 'Ø',
     '∞', '±', '≤', '≥', '¥', 'µ', '∂', '∑', '∏', 'π', '∫', 'ª', 'º', 'Ω',
     'æ', 'ø', '¿', '¡', '¬', '√', 'ƒ', '≈', '∆', '«', '»', '…', ' ', 'À',
     'Ã', 'Õ', 'Œ', 'œ', '–', '—', '“', '”', '‘', '’', '÷', '◊', 'ÿ', 'Ÿ',
     '⁄', '€', '‹', '›', 'ﬁ', 'ﬂ', '‡', '·', '‚', '„', '‰', 'Â', 'Ê', 'Á',
     'Ë', 'È', 'Í', 'Î', 'Ï', 'Ì', 'Ó', 'Ô', '', 'Ò', 'Ú', 'Û', 'Ù', 'ı',
     'ˆ', '˜', '¯', '˘', '˙', '˚', '¸', '˝', '˛', 'ˇ' },
};

#endif
