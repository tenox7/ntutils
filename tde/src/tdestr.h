/*
 * New editor name:  tde, the Thomson-Davis Editor.
 * Author:           Frank Davis
 * Date:             June 5, 1991
 *
 * This modification of Douglas Thomson's code is released into the
 * public domain, Frank Davis.  You may distribute it freely.
 *
 * This file contains define's and structure declarations common to all
 * editor modules.  It should be included in every source code module.
 *
 * I'm so stupid, I can't keep up with which declarations are in which
 * file.  I decided to put all typedefs, structs, and defines in one file.
 * If I don't, I end up defining a typedef one way in one file and a
 * completely different way in another file.
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

#include <assert.h>
#if defined( __DOS16__ )
 #if defined( __MSC__ )
  #include <malloc.h>          /* for memory allocation */
 #else
  #include <alloc.h>           /* for memory allocation */
 #endif
#endif

/*
 * in unix (POSIX?), getch( ) is in the curses package.  in Linux, ncurses
 *  seems to be the best of the worst curses.
 */
#if defined( __UNIX__ )
   #include <dirent.h>             /* walking down directories */
   #include <limits.h>             /* NAME_MAX and PATH_MAX, POSIX stuff */
   #include <malloc.h>             /* for memory allocation */
   #include <ncurses.h>
   #include <unistd.h>             /* chdir, getcwd, etc... */
   #include <sys/types.h>
   #include <sys/stat.h>
   #include <fcntl.h>              /* creat */
   #define fattr_t mode_t          /* attribute type for unix */
   #define ftime_t time_t          /* timestamp type for unix */
   #ifndef stricmp
   # define stricmp strcasecmp
   #endif
#else
   #include <io.h>                 /* isatty() (DOS16 - I hope MSC has it) */
                                   /* setmode (_setmode for MSC) */
   #include <fcntl.h>              /* O_BINARY (_O_BINARY for MSC) */
   #define fattr_t unsigned int    /* attribute type for dos/djgpp/Win32 */
# if defined( __WIN32__ )
   #define ftime_t FILETIME        /* timestamp type for Win32 */
# else
   #define ftime_t unsigned long   /* timestamp type for dos/djgpp */
# endif
#endif

#if defined( __DJGPP__ )
   #include <sys/farptr.h>
   #include <go32.h>
   #include <dpmi.h>
   #include <dir.h>
   #undef DIRECTORY             /* this define interferes with the lister */
   #include <unistd.h>          /* for isatty() */
   #include <limits.h>
   #undef NAME_MAX              /* defined as 12, yet it should also handle */
   #define NAME_MAX 255         /*  LFN which can be this */
#endif

#if defined( __WIN32__ )
   #include <windows.h>
   #ifdef PATH_MAX              /* MinGW defines this in limits.h */
   # undef PATH_MAX
   #endif
   #define PATH_MAX MAX_PATH
   #define NAME_MAX MAX_PATH
   #undef ERROR                 /* fix this!! */
   #undef DELETE                /* fix this!! */
   #undef TEXT                  /* fix this!! */
   #ifdef IGNORE
   # undef IGNORE               /* MinGW32 */
   #endif
   #ifdef SEARCH_ALL
   # undef SEARCH_ALL           /* Visual Studio */
   #endif

   #ifndef ENABLE_QUICK_EDIT    /* undocumented console modes */
   # define ENABLE_QUICK_EDIT      0x0040
   #endif
   #ifndef ENABLE_EXTENDED_FLAGS
   # define ENABLE_EXTENDED_FLAGS  0x0080
   #endif
#endif

#ifndef PATH_MAX
# define PATH_MAX 100           /* maximum file size 66+1+12+1+just-in-case */
#endif
#ifndef NAME_MAX
# define NAME_MAX 12
#endif



#include "bj_ctype.h"
#include "letters.h"
#include "filmatch.h"


/*
 * defines for the inline assembler.
 */
#if defined( __MSC__ )
   #define  ASSEMBLE   _asm
#else
   #define  ASSEMBLE   asm
#endif


#if defined( __UNIX__ )
   #define my_stdprn stderr
   #define STDFILE "/dev/tty"
#else
# if defined( __WIN32__ )
   #define my_stdprn stderr
# else
   #define my_stdprn stdprn
# endif
   #define STDFILE "CON"
#endif

/*
 * based on code contributed by "The Big Boss" <intruder@link.hacktic.nl>
 *  let's look for a config file in the current directory or in the user's
 *  home directory, or the executable's directory.
 */
#if defined( __UNIX__ )
# define CONFIGFILE      ".tdecfg"
# define HELPFILE        ".tdehlp"
# define SYNTAXFILE      ".tdeshl"
# define WORKSPACEFILE   ".tdewsp"
#else
# define CONFIGFILE      "tde.cfg"
# define HELPFILE        "tde.hlp"
# define SYNTAXFILE      "tde.shl"
# define WORKSPACEFILE   "tde.wsp"
#endif


/*
 * jmh 020817: If this is the first line of a file, then it is a valid
 *              workspace file.
 */
#define WORKSPACE_SIG    "TDE Workspace File"


/*
 * based on code by chen.
 *  in both DOS and UNIX, let's assume the biggest screen is 132x60.
 * jmh - Browsing through Ralf Brown's Interrupt List, I see that the
 *       widest screen is 160 and the longest is 75.
 * jmh 991019: 80 lines is now the biggest (6-line font, 480 scan lines).
 */
#define MAX_COLS           160  /* widest  screen ever used */
#define MAX_LINES           80  /* highest screen ever used */

#define BUFF_SIZE         1042  /* buffer size for lines */
#define MAX_LINE_LENGTH   1040  /* longest line allowed in file */
#define MAX_TAB_SIZE        80  /* largest tab size allowed (jmh 021105) */
#define NO_MARKERS           4  /* maximum no. of markers (inc. previous) */
#if defined( __DOS16__ )
#define UNDO_STACK_LEN     200  /* number of lines in undo stack */
#define MAX_UNDOS          200  /* number of undos to keep */
#else
#define UNDO_STACK_LEN       0
#define MAX_UNDOS            0
#endif

/*
 * when we read in a file, lets try to match the size of the read buffer
 *   with some multiple of a hardware or software cache that may be present.
 */
#define READ_LENGTH             1024
#define DEFAULT_BIN_LENGTH      64


#define REGX_SIZE               200     /* maximum number of nodes in nfa */


/*
 * general defines.
 */
#ifndef ERROR
# define ERROR            (-1)  /* abnormal termination */
#endif
#ifndef OK
# define OK                  0  /* normal termination */
#endif
#ifndef TRUE
# define TRUE                1  /* logical true */
#endif
#ifndef FALSE
# define FALSE               0  /* logical false */
#endif

#define MAX_KEYS            88  /* number of physical keys recognised by TDE */
#define MODIFIERS            8  /* number of states of those keys */
#define AVAIL_KEYS         117  /* number of config keys */
#define STROKE_LIMIT      1024  /* number of key strokes in playback buffer */


#define STACK_UNDERFLOW      1  /* code for underflowing macro stack */
#define STACK_OVERFLOW       2  /* code for overflowing macro stack */
#define SAS_P               20  /* number of sas pointers to tokens */
#define NUM_FUNCS          239

#define NUM_COLORS          28  /* number of color fields in TDE */
#define SHL_NUM_COLORS      14  /* number of colors for syntax highlighting */
#define SHL_NUM_FEATURES    27  /* number of customizable features */


/*
 * Function flags (jmh 031129).  These are used to indicate a function is
 * available in read-only mode and to help the menu determine invalid fns.
 */
#define F_MODIFY         1      /* function modifies the file */
#define F_BOX            2      /* function requires BOX block */
#define F_LINE           4      /* function requires LINE block */
#define F_STREAM         8      /* function requires STREAM block */
#define F_BSAFE         16      /* block function does not modify its file */

#define F_BLOCK         (F_BOX | F_LINE | F_STREAM)


/*
 * special cases for keyboard mapping -  see define.h
 */
#define RTURN           _ENTER        /* Return key */
#define ESC             _ESC          /* Escape key */


/*
 * The following defines are used by the "error" function to indicate
 *  how serious the error is.
 */
#define WARNING         1    /* user must acknowledge, editor continues */
#define INFO            2    /* display message, acknowledge, continue */


/*
 * define the type of block marked by user and block actions
 */
#define NOTMARKED        0      /* block type undefined */
#define BOX              1      /* block marked by row and column */
#define LINE             2      /* block marked by begin and end lines */
#define STREAM           3      /* block marked by begin and end characters */

#define MOVE             1
#define DELETE           2
#define COPY             3
#define KOPY             4
#define FILL             5
#define OVERLAY          6
#define NUMBER           7
#define SWAP             8
#define BORDER           9      /* added by jmh 980731 */
#define JUSTIFY         10      /* added by jmh 980810 */
#define SUM             11      /* added by jmh 991112 */

#define LEFT            1
#define RIGHT           2

#define ASCENDING       1
#define DESCENDING      2


/*
 * three types of ways to update windows
 * jmh 991124: added a RULER type, treat as a mask.
 */
#define LOCAL           1
#define NOT_LOCAL       2
#define GLOBAL          3       /* LOCAL | NOT_LOCAL */
#define RULER           4

/*
 * Additional flags to indicate read-only status from command line.
 */
#define O_READ_ONLY     4
#define O_READ_WRITE    8
#define CMDLINE        16       /* jmh 030303 */

#define CURLINE         1
#define NOTCURLINE      2


/*
 * search/replace flags.
 */
#define BOYER_MOORE     0
#define REG_EXPRESSION  1

#define CLR_SEARCH      0
#define WRAPPED         1
#define SEARCHING       2
#define REPLACING       3
#define NFA_GAVE_UP     4
#define CHANGED        12
#define SORTING        13

#define IGNORE          1
#define MATCH           2

#define PROMPT          1
#define NOPROMPT        2

#define FORWARD         1
#define BACKWARD        2

#define BEGIN           1
#define END             2

#define BEGINNING       1
#define CURRENT         2

/*
 * jmh 991006: flags to indicate what type of search is being performed
 */
#define SEARCH_REGX     1       /* TRUE = regx,      FALSE = text        */
#define SEARCH_BACK     2       /* TRUE = backward,  FALSE = forward     */
#define SEARCH_BLOCK    4       /* TRUE = block,     FALSE = file        */
#define SEARCH_BEGIN    8       /* TRUE = beginning, FALSE = current     */
#define SEARCH_I       16       /* TRUE = isearch,   FALSE = normal      */
#define SEARCH_ALL     32       /* search all loaded files               */
#define SEARCH_RESULTS 64       /* all matching lines go in a new window */


/*
 * additional messages included with the search ones.
 */
#define DIFFING         5
#define NEXTKEY         6
#define UNDO_GROUP      7       /* 7 is off,  8 is on */
#define UNDO_MOVE       9       /* 9 is off, 10 is on */
#define TALLYING       11


/*
 * get_response flags
 */
#define R_PROMPT   1            /* display responses */
#define R_MACRO    2            /* accept response from a macro */
#define R_DEFAULT  4            /* accept the first response as a default */
#define R_ABORT    8            /* cancel query */
#define R_NOMACRO  (R_PROMPT | R_DEFAULT | R_ABORT)
#define R_ALL      (R_MACRO  | R_NOMACRO)


/*
 * word wrap flag.
 */
#define NO_WRAP         0
#define FIXED_WRAP      1
#define DYNAMIC_WRAP    2


/*
 * used in interrupt 0x21 function xx for checking file status
 */
#define EXIST           0
#define WRITE           2
#define READ            4
#define READ_WRITE      6

#define NORMAL          0x00
#define READ_ONLY       0x01
#define HIDDEN          0x02
#define SYSTEM          0x04
#define VOLUME_LABEL    0x08
#define SUBDIRECTORY    0x10
#define ARCHIVE         0x20

/*
 * critical error def's
 */
#define RETRY           1
#define ABORT           2
#define FAIL            3


/*
 * flags used for opening files to write either in binary or text mode.
 * crlf is for writing files in text mode - Operating System converts
 * lf to crlf automatically on output.  in binary mode, lf is not translated.
 */
#define CRLF            0
#define LF              1
#define BINARY          2
#define TEXT            3
#define NATIVE          4

#define OVERWRITE       1
#define APPEND          2


/*
 * cursor sizes (modified by jmh 990404)
 */
#define SMALL_CURSOR    0
#define MEDIUM_CURSOR   1
#define LARGE_CURSOR    2

#define CURSES_INVISBL  0
#define CURSES_SMALL    1
#define CURSES_LARGE    2


/*
 * possible answers to various questions - see get_yn, get_ynaq and get_oa
 */
#define A_YES           1
#define A_NO            2
#define A_ALWAYS        3
#define A_QUIT          4
#define A_ABORT         5
#define A_OVERWRITE     6
#define A_APPEND        7

#define VIDEO_INT    0x10

#define COLOR_80        3
#define MONO_80         7

#define VGA             3
#define EGA             2
#define CGA             1
#define MDA             0


#define  MAJOR          10      /* number of pull-down main menu options */
#define  MAJOR_MAX      20      /* maximum number of menu options */


#define SAS_DELIMITERS  " \n"


/*
 * values for directory sorting (jmh 980805)
 */
#define SORT_NAME       0
#define SORT_EXT        1


/*
 * cursor direction (jmh 981129)
 */
#define CUR_RIGHT       0
#define CUR_LEFT        1
#define CUR_DOWN        2
#define CUR_UP          3


/*
 * directory matching (jmh 990425)
 */
#define NO_DIRS         0
#define MATCH_DIRS      1
#define ALL_DIRS        2


/*
 * number of graphic sets available (jmh 991019)
 */
#define GRAPHIC_SETS    6


/*
 * flags for reduce string (jmh 021105)
 */
/* BEGIN & END are defined from the search */
#define MIDDLE          3


/*
 * macro flag bitmasks (jmh 050709)
 */
#define WRAP            0x01    /* searches should wrap */
#define NOWRAP          0x02    /* searches should stop at EOF */
#define NOWARN          0x04    /* don't display warning messages */
#define NEEDBOX         0x08    /* box block required */
#define NEEDLINE        0x10    /* line block required */
#define NEEDSTREAM      0x20    /* stream block required */
#define USESBLOCK       0x40    /* preserve current block */
#define USESDIALOG      0x80    /* preserve dialog settings */

#define NEEDBLOCK       (NEEDBOX | NEEDLINE | NEEDSTREAM)


/*
 * let's explicitly treat characters as unsigned.
 */
typedef unsigned char text_t;
typedef unsigned char *text_ptr;

/*
 * I'm sick of all the tests for the character type, so let's define it - jmh
 */
#if defined( __UNIX__ )
# define Char chtype
#elif defined( __WIN32__ )
# define Char CHAR_INFO
#else
# define Char unsigned short
#endif
#define DISPLAY_BUFF Char display_buff[MAX_COLS]
#define SAVE_LINE( line )       save_screen_line( 0, line, display_buff )
#define RESTORE_LINE( line ) restore_screen_line( 0, line, display_buff )


/*
 * Create a few key macros.
 * KEY_IDX is used by key_func and macro arrays: key_func[KEY_IDX(key)]
 */
#define SHIFT(key) ((int)key >> 9)
#define KEY(key)   ((int)key & 255)
#define KEY_IDX(key) SHIFT(key)][KEY(key)
#define CREATE_TWOKEY(parent, child) (((long)(parent) << 16) | (child))
#define PARENT_KEY(key) (int)((key) >> 16)
#define CHILD_KEY(key)  (int)((key) & 0xffff)


/*
 * "mem_info" contains struct defs for a block of memory.
 */
typedef struct {
   char *base;                  /* pointer to the actual memory of the block */
   unsigned size;               /* size of the block */
   unsigned space;              /* largest free space */
   char *free;                  /* pointer to current or first free space */
} mem_info;

#define MEM_GROUP 10            /* allocate this many at a time */
/*
 * The initial size of the block. I've added 12 because Borland uses paragraph
 * allocation, with a four-byte overhead. I hope MSC doesn't use more.
 */
#define MEM_CHUNK 64012u
#define MEM_MIN_CHUNK 2000      /* don't create a block less than this */


/*
 * "s_line_list" contains struct defs for a node of text.
 */
typedef struct s_line_list {
   text_ptr line;                       /* pointer to line */
   int      len;                        /* length of line */
   long     type;                       /* bit mask - see below */
   struct s_line_list *next;            /* next line in doubly linked list */
   struct s_line_list *prev;            /* prev line in doubly linked list */
} line_list_struc;

typedef line_list_struc *line_list_ptr;

/*
 * Bit masks for the type of a line. Syntax highlighting flags are in syntax.h.
 */
#define DIRTY             1     /* the line has been modified              */
#define EQUAL    0x80000000L    /* the line sorts the same as the previous */


typedef struct {
   char *minor_name;
   long minor_func;
   struct s_menu_str *pop_out;  /* added by jmh 010624 */
   int  disabled;               /* added by jmh 031129 */
   char *line;                  /* the line that is displayed (jmh 031130) */
} MINOR_STR;

typedef struct s_menu_str {
   MINOR_STR *minor;
   int  minor_cnt;
   int  width;                  /* the width of the menu     | these four  */
   int  first;                  /* the first valid selection |  added by   */
   int  last;                   /* the last valid selection  |  jmh 980809 */
   int  current;                /* the current selection */
   int  checked;                /* added by jmh 031129 */
} MENU_STR;

typedef struct {
   char *major_name;
   MENU_STR menu;
} MENU[MAJOR_MAX];

typedef struct s_menu_list {    /* jmh 050722: use a linked list to maintain */
   struct s_menu_list *next;    /*             the popout menus              */
   MENU_STR popout;
} MENU_LIST;


struct vcfg {
   int color;
   unsigned cols;
   unsigned rows;
};


/* one array for mono and another for color */
typedef int TDE_COLORS[2][NUM_COLORS];


typedef unsigned char KEY_FUNC[MODIFIERS][MAX_KEYS];


/*
 * structure used in directory list.
 */
typedef struct {
   char   *fname;               /* file name (jmh 020924: pointer not array) */
   long    fsize;               /* file size in bytes */
   fattr_t fattr;               /* file attribute */
   ftime_t ftime;               /* file time (jmh 031202) */
} FTYPE;


/*
 * structure used in language list.
 */
typedef struct {
   char *parent;                /* parent language */
   char *pattern;               /* file pattern */
   int   icase;                 /* case sensitivity */
} LTYPE;


/*
 * structure for an item in a list dialog (jmh 060913).
 */
typedef struct {
   char *name;                  /* text to display */
   int   color;                 /* color to use */
   void *data;                  /* associated data */
} LIST;


/*
 * stuff we need to know about how to display a directory listing.
 */
typedef struct {
   int  row;                    /* absolute row to display dir box */
   int  col;                    /* absolute column to display dir box */
   int  wid;                    /* absolute number of columns in dir box */
   int  hgt;                    /* absolute number of rows in dir box */
   int  max_cols;               /* number of columns of files in box */
   int  max_lines;              /* number of lines of files in box */
   int  len;                    /* length of highlight bar */
   int  cnt;                    /* file count */
   int  cols;                   /* logical number of columns in list */
   int  lines;                  /* logical number of rows in list */
   int  prow;                   /* logical number of rows in partial row */
   int  vcols;                  /* number of virtual columns, if any */
   int  nfiles;                 /* number of files on screen */
   int  avail;                  /* number of available slots for files */
   int  select;                 /* current file under cursor */
   int  flist_col[5];           /* offset from col to display each column */
} DIRECTORY;


typedef struct {
   int  pattern_length;                 /* number of chars in pattern */
   int  search_defined;                 /* search pattern defined? */
   text_t pattern[MAX_COLS];            /* search pattern */
   int  forward_md2;                    /* forward mini-delta2 */
   int  backward_md2;                   /* backward mini-delta2 */
   char skip_forward[256];              /* boyer array for forward searches */
   char skip_backward[256];             /* boyer array for back searches */
} boyer_moore_type;


/*
 * "mode_infos" contain the editor mode variables.  The configuration
 *  utility modifies this structure to customize the start-up tde
 *  configuration
 */
typedef struct {
   int  color_scheme;           /* color to start out with */
   int  sync;                   /* sync the cursor movement command? */
   int  sync_sem;               /* sync the cursor movement command? */
   int  record;                 /* are we recording keystrokes? */
   int  insert;                 /* in insert mode? */
   int  indent;                 /* in auto-indent mode? */
   int  ptab_size;              /* physical tab stops */
   int  ltab_size;              /* logical tab stops */
   int  smart_tab;              /* smart tab mode on or off? */
   int  inflate_tabs;           /* inflate tabs?  T or F or Real */
   int  search_case;            /* consider case? IGNORE or MATCH */
   int  enh_kbd;                /* type of keyboard */
   int  cursor_size;            /* insert cursor big or small? */
   int  control_z;              /* write ^Z - t or f */
   int  crlf;                   /* <cr><lf> toggle CRLF or LF */
   int  trailing;               /* remove trailing space? T or F */
   int  show_eol;               /* show lf at eol? T or F or Extend */
   int  word_wrap;              /* in word wrap mode? */
   int  left_margin;            /* left margin */
   int  parg_margin;            /* column for 1st word in paragraph */
   int  right_margin;           /* right margin */
   int  right_justify;          /* boolean, justify right margin?  T or F */
   int  format_sem;             /* format semaphore */
   int  undo_max_lines;         /* max number of lines in undo stack */
   int  undo_max;               /* max number of undos to keep */
   int  undo_group;             /* group or individual undo?  T or F */
   int  undo_move;              /* undo movement? T or F */
   int  do_backups;             /* create backup or ".bak" files? T or F */
   int  ruler;                  /* show ruler at top of window? T or F */
   char stamp[MAX_COLS];        /* format for date and time stamp */
   int  cursor_cross;           /* is the cursor cross on? T or F */
   int  dir_sort;               /* sort directories by name or extension? */
   int  graphics;               /* numbers translated to graphics? +ve yes */
   int  cur_dir;                /* cursor update direction */
   int  draw;                   /* draw mode? - jmh 991018 */
   int  line_numbers;           /* display line numbers? - jmh 991108 */
   int  auto_save_wksp;         /* auto. save workspace? - jmh 020911 */
   int  track_path;             /* set cwd to current file? - jmh 021021 */
   int  display_cwd;            /* display the cwd? - jmh 030226 */
   char helpfile[PATH_MAX];     /* name of help file - jmh 050711 */
   char helptopic[MAX_COLS];    /* regx pattern to search above */
} mode_infos;


/*
 * "displays" contain all the status information about what attributes are
 *  used for what purposes, which attribute is currently set, and so on.
 * The editor only knows about one physical screen.
 *
 * jmh 991023: even though nlines says it is lines on display device, it is
 *              used as the bottom line of the window. It's annoyed me once
 *              too often, so now it is what it says it is and end_line is
 *              the last available line. To be specific:
 *                  nlines    = height of display
 *                  mode_line = nlines - 1
 *                  end_line  = mode_line - 1
 *
 * jmh 031028: removed the individual colors, using a pointer into the
 *              colours array instead (also see define.h).
 *
 */
typedef struct {
   int nlines;                  /* lines on display device */
   int ncols;                   /* columns on display device */
   int mode_line;               /* line to display editor modes - fmd */
   int end_line;                /* last line available to windows - jmh */
   int *color;                  /* pointer to mono or color settings */
   int old_overscan;            /* color of old overscan   */
   int adapter;                 /* VGA, EGA, CGA, or MDA? */
   int curses_cursor;           /* what state is CURSES cursor? */
                                /* original cursor size for DOS */
   int cursor[3];               /* cursor sizes (jmh 990404) */
   int overw_cursor;            /* hi part = top scan line, lo part = bottom */
   int insert_cursor;           /* hi part = top scan line, lo part = bottom */
   int frame_style;             /* style to draw frames (jmh 991022) */
   int frame_space;             /* draw a space around frame? (jmh 991022) */
   int output_space;            /* flag for s_output() */
   int shadow;                  /* give dialogs a shadow? (jmh 991023) */
   int shadow_width;            /* width of right edge of shadow (jmh 991023) */
} displays;


/*
 * "TDE_WIN" contains all the information that is unique to a given
 *  window.
 */
typedef struct s_tdewin {
   struct s_file_infos *file_info;       /* file in window */
   line_list_ptr ll;            /* pointer to current node in linked list */
   int  ccol;                   /* column cursor logically in */
   int  rcol;                   /* column cursor actually in */
   int  bcol;                   /* base column to start display */
   int  cline;                  /* line cursor logically in */
   long rline;                  /* real line cursor in */
   long bin_offset;             /* offset, in bytes from beg of bin file */
   int  top;                    /* top line of window    - jmh 991109 */
   int  left;                   /* left column of window - jmh 991109 */
   int  top_line;               /* top line in window */
   int  bottom_line;            /* bottom line in window */
   int  start_col;              /* starting column on physical screen */
   int  end_col;                /* ending column on physical screen */
   int  vertical;               /* boolean. is window split vertically? */
   int  page;                   /* no. of lines to scroll for one page */
   int  visible;                /* window hidden or visible */
   int  letter;                 /* window letter */
   int  ruler;                  /* show ruler in this window? */
   int  syntax;                 /* is syntax highlighting enabled? */
   int  cur_count;              /* number of "current" lines to display */
   char *title;                 /* title to display instead of filename */
   struct s_tdewin *next;       /* next window in doubly linked list */
   struct s_tdewin *prev;       /* previous window in doubly linked list */
} TDE_WIN;


#if defined( __DOS16__ )
/*
 * struct for find first and find next functions.  see DOS technical ref
 * manuals for more info on DTA structures.
 */
typedef struct {
   char          reserved[21];
   unsigned char attrib;
   ftime_t       time;
   long          size;
   char          name[13];
} DTA;
#endif

typedef struct {
#if defined( __UNIX__ )
   DIR  *find_info;
#elif defined( __DJGPP__ )
   struct ffblk find_info;
#elif defined( __WIN32__ )
   HANDLE find_info;
#else
   DTA  find_info;
#endif
   char stem[PATH_MAX];                 /* Path to search */
   char pattern[NAME_MAX];              /* Wildcard pattern to search */
   int  dirs;                           /* TRUE to find directories/sizes */
} FFIND;


/*
 * marker structure used to save file positions.
 */
typedef struct {
   long rline;                  /* real line of marker */
   int  cline;                  /* screen line of marker */
   int  rcol;                   /* real column of marker */
   int  ccol;                   /* logical column of marker */
   int  bcol;                   /* base column of marker */
   int  marked;                 /* boolean:  has this marker been set? */
} MARKER;


/*
 * structure for recording and playing back keystrokes.
 */
typedef struct {
  int mode[3];                  /* insert, tab and indent modes */
  int flag;                     /* wrap, warning, block and dialog flags */
  int len;                      /* number of keys in the macro */
  union {
     long key;                  /* one key, which may be text or function */
     long *keys;                /* an array of keys */
  } key;
} MACRO;


/*
 * tree structure for pseudo-macros and two-key functions and macros.
 * jmh 990430: did away with the union.
 */
typedef struct s_tree {
   long key;                    /* combination or two-key code */
                                /* > 0x10000 is a two-key */
   MACRO *macro;                /* pointer to the macro */
   int   func;                  /* function assigned to two-key */
   struct s_tree *left;         /* pointer to the one less than this */
   struct s_tree *right;        /* pointer to the one greater than this */
} TREE;


/*
 * structure for macro block information.
 */
typedef struct {
   struct s_file_infos *file;
   int  marked;
   int  type;
   int  bc;
   long br;
   int  ec;
   long er;
} BLOCK;

/*
 * structure for the local macro stack in macro processor.
 */
typedef struct s_macro_stack {
   MACRO *macro;                /* macro we were executing */
   int    pos;                  /* position to begin execution in macro */
   MARKER mark;                 /* macro marker */
   BLOCK  block;                /* block prior to macro */
   struct s_macro_stack *prev;  /* previous macro on the stack */
} MACRO_STACK;


/*
 * structure for the undo.
 */
typedef struct s_undo {
   long line;                   /* line number (rline) */
   int  col;                    /* column position (rcol) */
   int  op;                     /* operation or function */
   int  len;                    /* length or count */
   text_ptr text;               /* characters or line */
   int  dirty;                  /* line dirty flag before change */
   int  mod;                    /* modified file flag before change */
   int  group;                  /* number of undos within the group */
   struct s_undo *prev;
   struct s_undo *next;
} UNDO;

#define U_UNDO  1024            /* distinguish between op. and function */
#define U_INS    (1 | U_UNDO)
#define U_OVR    (2 | U_UNDO)
#define U_DEL    (4 | U_UNDO)
#define U_CHAR   (8 | U_UNDO)
#define U_STR   (16 | U_UNDO)
#define U_LINE  (32 | U_UNDO)
#define U_SPACE (64 | U_UNDO)


/*
 * "option_infos" contains all the command line options that may
 *  apply to more than one file (jmh 030730).
 */
typedef struct {
   char *language;              /* syntax highlighting language to use */
   int  read_only;              /* read-only file */
   int  scratch;                /* load file as scratch - jmh 030730 */
   int  tab_size;               /* size of tabs - jmh 010528 */
   int  file_mode;              /* mode to open files, TEXT or BINARY */
   int  file_chunk;             /* if opened BINARY, bytes per line */
   int  macro;                  /* key to use for the startup macro */
   int  all;                    /* load all files immediately - jmh 040715 */
} option_infos;


/*
 * "status_infos" contains all the editor status information that is
 *  global to the entire editor (i.e. not dependent on the file or
 *  window)
 */
typedef struct {
   mem_info *memory;            /* array of memory blocks (sorted by base) */
   int  *mem_size;              /* array of memory blocks (sorted by size) */
   int  mem_avail;              /* count of above */
   int  mem_num;                /* actual blocks allocated */
   int  mem_cur;                /* current block to allocate from */
   TDE_WIN *current_window;     /* current active window */
   struct s_file_infos *current_file; /* current active file */
   struct s_file_infos *file_list; /* all active files */
   TDE_WIN *window_list;        /* all active windows */
   struct s_language *language_list; /* all loaded languages */
   int  window_count;           /* number of windows - displayed and hidden */
   int  file_count;             /* number of files currently open */
   int  scratch_count;          /* number of scratch files */
   int  arg;                    /* current argument pointer */
   int  argc;                   /* argument count */
   char **argv;                 /* argument variables - same as *argv[] */
   long jump_to;                /* line to jump to */
   int  jump_col;               /* column to jump to - jmh 990921 */
   long jump_off;               /* offset to jump to - jmh 990921 */
   int  viewer_mode;            /* start in viewer mode */
   int  sas_defined;            /* has search and seize been defined */
   int  sas_search_type;        /* if defined, Boyer-Moore or reg expression? */
   int  sas_arg;                /* current argument pointer */
   int  sas_argc;               /* argument count */
   char **sas_argv;             /* argument variables - same as *argv[] */
   char sas_tokens[PATH_MAX];   /* storage for file tokens */
   char *sas_arg_pointers[SAS_P]; /* array of tokens in sas_tokens */
   long sas_rline;              /* line number of sas find */
   int  sas_rcol;               /* column number of sas find */
   line_list_ptr sas_ll;        /* linked list node pointing to line */
   int  marked;                 /* has block been marked? */
   struct s_file_infos *marked_file;  /* pointer to file w/ marked block */
   char rw_name[PATH_MAX];      /* name of last file read or written */
   FileList *dta;               /* finding files to be edited */
   FileList *sas_dta;           /* finding files to be searched */
   char *sas_path;              /* path of files being searched */
   text_t subst[MAX_COLS];      /* last substitute text */
   int  replace_flag;           /* prompt or noprompt b4 replacing */
   int  replace_defined;        /* replace defined ? */
   int  overlap;                /* overlap between pages for page up etc */
   line_list_ptr buff_node;     /* address of node in line_buff */
   int  copied;                 /* has line been copied to file? */
   text_t line_buff[BUFF_SIZE*2+8]; /* for currently edited line */
   text_t tabout_buff[BUFF_SIZE+8]; /* for expanding tabs */
   int  line_buff_len;          /* length of line in the line_buff */
   int  tabout_buff_len;        /* length of line in the tabout_buff */
   int  viewer_key;             /* are viewer keys in effect? (jmh 990430) */
   int  command;                /* function of key just entered */
   long key_pressed;            /* key pressed by user */
   MACRO *key_macro;            /* macro assigned to key */
   int  last_command;           /* previous function (jmh 990507) */
   int  command_count;          /* consecutive function count (jmh 990507) */
   int  wrapped;                /* is the wrapped message on mode line? */
   int  stop;                   /* stop indicator */
   volatile int control_break;  /* control break pressed? (volatile by jmh) */
   int  macro_executing;        /* flag set if macro is executing */
   long recording_key;          /* key we are assigning keystrokes to */
   int  stroke_count;           /* free keys in stroke buffer */
   MACRO *current_macro;        /* macro currently executing */
   MACRO *rec_macro;            /* macro currently recording */
   int  macro_next;             /* pointer to next key in macro */
   MARKER macro_mark;           /* special marker for macros */
   MACRO_STACK *mstack;         /* pointer to stacked macros */
   int  screen_display;         /* screen display off or on? */
   int  input_redir;            /* has stdin been redirected? */
   int  output_redir;           /* has stdout been redirected? */
   int  cur_line;               /* displaying the current line? */
   int  search;                 /* various search flags - jmh 991006 */
   const char *errmsg;          /* error message on exit - jmh 010528 */
} status_infos;


/*
 * "file_infos" contain all the information unique to a given file
 */
typedef struct s_file_infos {
   line_list_ptr line_list;     /* pointer to first node in linked list */
   line_list_ptr line_list_end; /* pointer to last node in linked list */
   line_list_ptr undo_top;      /* pointer to top node in undo stack */
   line_list_ptr undo_bot;      /* pointer to last node in undo stack */
   int  undo_lines;             /* number of lines in undo stack */
   UNDO *undo;                  /* pointer to first item to undo */
   UNDO *redo;                  /* pointer to first item to redo */
   UNDO *undo_last;             /* undo to remove when full */
   int  undo_count;             /* number of items in undo */
   MARKER marker[NO_MARKERS];   /* number of file markers */
   long length;                 /* number of lines in file */
   int  len_len;                /* character length of above - jmh 991108 */
   int  modified;               /* file has been modified since save? */
   int  dirty;                  /* file in window modified? */
   int  new_file;               /* is current file new? */
   int  backed_up;              /* has ".bak" file been created? */
   int  crlf;                   /* when read, did lines end CRLF or LF? */
   int  binary_len;             /* if BINARY, length of lines (jmh 020724) */
   int  read_only;              /* can the file be modified? (jmh 990428) */
   int  scratch;                /* don't prompt to save (jmh 030226) */
   char file_name[PATH_MAX];    /* name of current file being edited */
   char backup_fname[PATH_MAX]; /* name of backup of current file */
   char *fname;                 /* the name within file_name (jmh 021021) */
   fattr_t file_attrib;         /* file attributes */
   ftime_t ftime;               /* file (modified) timestamp */
   int  block_type;             /* block type - line or box */
   line_list_ptr block_start;   /* beginning block position */
   line_list_ptr block_end;     /* ending block position */
   int  block_bc;               /* beginning column */
   long block_br;               /* beginning row */
   int  block_ec;               /* ending column */
   long block_er;               /* ending row */
   long break_point;            /* line number of break point */
   int  ptab_size;              /* physical tab stops */
   int  ltab_size;              /* logical tab stops */
   int  inflate_tabs;           /* inflate tabs?  T or F or Real */
   int  file_no;                /* file number */
   int  ref_count;              /* no. of windows referring to file */
   int  next_letter;            /* next window letter */
   struct s_language *syntax;   /* syntax highlighting information */
   struct s_file_infos *next;   /* next file in doubly linked list */
   struct s_file_infos *prev;   /* previous file in doubly linked list */
} file_infos;


/*
 * structure for languages/files that have syntax highlighting.
 */
typedef struct s_language {
   text_ptr *keyword[128];      /* hash table of keywords/identifiers */
   TREE     key_tree;           /* pseudo-macros and two-keys */
   MENU_STR menu;               /* User > Language pop-out menu */
   int    inflate_tabs;         /* language-specific tab settings */
   int    ptab_size;
   int    ltab_size;
   struct s_syntax_info *info;  /* syntax information */
   struct s_language *parent;   /* parent language */
   struct s_language *next;     /* next language in the list */
   char   name[1];              /* the name of the language (must */
                                /*  be last! allocated dynamically) */
} LANGUAGE;


/*
 * structure for syntax highlighting features.
 */
typedef struct s_syntax_info {
   text_t comment[2][5];                /* one line comment */
   text_t comstart[2][5];               /* start of multi-line comment */
   text_t comend[2][5];                 /* end of multi-line comment */
   text_t function;                     /* character for function calls */
   text_t strstart;                     /* start of string */
   text_t strend;                       /* end of string */
   int    strnewl;                      /* can string contain newlines? */
   text_t strspan;                      /* character for string line-spanning */
   text_t charstart;                    /* start of character */
   text_t charend;                      /* end of character */
   int    charnewl;                     /* can char. contain newlines? */
   text_t charspan;                     /* character for char. line-spanning */
   int    charnum;                      /* number of characters allowed */
   text_t escape;                       /* escaped character indicator */
   text_t binary[3];                    /* binary prefix or suffix */
   text_t octal[3];                     /* octal prefix or suffix */
   text_t hex[3];                       /* hexadecimal prefix or suffix */
   text_t prepro[2];                    /* preprocessor indicator */
   text_t prepspan;                     /* character for prep. line-spanning */
   int    icase;                        /* ignore case? */
   unsigned char identifier[256];       /* identifier flags */
} syntax_info;


#if defined( __DOS16__ )
/*
 * structure for critical error handler.  check the flag for errors on
 * each I/O call.
 * jmh 021103: limited to DOS16 only.
 */
typedef struct {
   int  flag;                   /* 0 = no error, -1 = error */
   int  code;                   /* error code from di */
   int  rw;                     /* read or write operation? */
   int  drive;                  /* drive number of error */
   int  extended;               /* extended error code, func 0x59 */
   int  class;                  /* error class */
   int  action;                 /* suggested action from DOS */
   int  locus;                  /* locus of error: drive, network, etc... */
   int  dattr;                  /* device attribute, 0 = drive, 1 = serial */
   char dname[10];              /* device name */
} CEH;
#define CEH_OK    ceh.flag = OK
#define CEH_ERROR (ceh.flag == ERROR)
#else
#define CEH_OK    (void)0
#define CEH_ERROR (0)
#endif


/*
 * structure for sort strings
 */
typedef struct {
   text_ptr order_array;
   text_ptr pivot_ptr;
   int  direction;
   int  block_len;
   int  pivot_len;
   int  bc;
   int  ec;
} SORT;


typedef struct {
   text_t ignore[256];
   text_t match[256];
} SORT_ORDER;


/*
 * structure for diff.  let's only diff two windows at a time.
 */
typedef struct {
   int       defined;           /* initially FALSE */
   int       leading;           /* skip leading spaces t/f */
   int       all_space;         /* skip all space t/f */
   int       blank_lines;       /* skip blank lines t/f */
   int       ignore_eol;        /* skip end of line t/f */
   TDE_WIN   *w1;               /* pointer to first diff window */
   TDE_WIN   *w2;               /* pointer to second diff window */
   line_list_ptr  d1;           /* pointer to text in window 1 */
   line_list_ptr  d2;           /* pointer to text in window 2 */
   long      rline1;            /* line number in window 1 */
   long      rline2;            /* line number in window 2 */
   long      bin_offset1;       /* binary offset in window 1 */
   long      bin_offset2;       /* binary offset in window 2 */
} DIFF;


/*
 * structures for regular expression searches.
 */
typedef struct {
   int  pattern_length;                 /* number of chars in pattern */
   int  search_defined;                 /* search pattern defined? */
   int  node_count;                     /* number of nodes in the nfa */
   text_t pattern[MAX_COLS];            /* search pattern */
} REGX_INFO;


typedef struct {
   int  node_type[REGX_SIZE];
   int  term_type[REGX_SIZE];
   int  c[REGX_SIZE];
   char *class[REGX_SIZE];
   int  next1[REGX_SIZE];
   int  next2[REGX_SIZE];
} NFA_TYPE;


/*
 * jmh 030331: a remnant from a previous regx parser?
 */
/*
typedef struct parse_tree {
   int  node_type;
   int  leaf_char;
   char *leaf_string;
   struct parse_tree *r_pt;
   struct parse_tree *l_pt;
} PARSE_TREE;
*/


/*
 * Configuration and syntax highlighting file parsing structure.
 */
typedef struct {
   const char * const key;           /* key name */
   int  key_index;                   /* key value */
} CONFIG_DEFS;


/*
 * History structure (jmh 990424).
 */
typedef struct s_history {
   int  len;                    /* length of string */
   struct s_history *prev;      /* previous string in history list */
   struct s_history *next;      /* next string in history list */
   char str[1];                 /* must be last! allocated dynamically */
} HISTORY;


/*
 * Dialog structure (jmh 031115).
 */
typedef struct {
   int  x, y;                   /* position of item or size of dialog */
   char *text;                  /* label or edit field text */
   int  n;                      /* checkbox value or edit field width */
   HISTORY *hist;               /* history for edit, enabled state of cb */
} DIALOG;

typedef int (*DLG_PROC)( int, char * );
