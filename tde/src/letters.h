/*
 * Editor:      TDE, the Thomson-Davis Editor
 * Filename:    letters.h
 * Compiled by: Byrial Jensen
 *
 * This file contains various defines of letters etc., collected to
 * to ease making versions for other languages.
 * It is used with changed versions of several of the TDE source files
 */

/* Response letters */

#define L_BLOCK         'B'     /* cp. block13 */
#define L_FILE          'F'

#define L_ABORT         'A'     /* cp. criterr_screen in criterr.h */
#if defined( __DOS16__ )
# define L_FAIL         'Q'     /* L_CONTINUE used by unix and djgpp is */
# define L_RETRY        'R'     /*  defined below (in find) */
#elif defined( __UNIX__ )
# define L_IGNORE       'I'
#endif

/*
 * dos file attributes
 */
#define L_DOS_ARCHIVE   'A'     /* cp. utils14 */
#define L_DOS_SYSTEM    'S'
#define L_DOS_HIDDEN    'H'
#define L_DOS_READ_ONLY 'R'

/*
 * unix file attributes
 */
#define L_UNIX_READ     'r'     /* cp. utils14 */
#define L_UNIX_WRITE    'w'
#define L_UNIX_EXECUTE  'x'

#define L_REPLACE       'R'     /* cp. find2 */
#define L_SKIP          'S'
#define L_EXIT          'E'

#define L_QUIT          'Q'     /* cp. find3 */
#define L_CONTINUE      'C'

#define L_ASCENDING     'A'     /* cp. utils4 */
#define L_DESCENDING    'D'

#define L_FORWARD       'F'     /* cp. utils5 */
#define L_BACKWARD      'B'

#define L_YES           'Y'     /* in multiple prompts */
#define L_NO            'N'

#define L_LEFT          'L'     /* cp. block18 */
#define L_RIGHT         'R'

#define L_BEGINNING     'B'     /* cp. diff_prompt3 */
#define L_CURRENT       'C'

#define L_OVERWRITE     'O'     /* cp. block7 */
#define L_APPEND        'A'


#define BLOCK14_LINE_SLOT  14   /* positions for data in messages */
#define BLOCK14_TOTAL_SLOT 25
#define BLOCK14_SUM_SLOT   25
#define UTILS13_NO_SLOT     7
#define GRAPHIC_SLOT        8   /* jmh 981129 */
#define ED7C_SLOT          10   /* jmh 030304 */


/*
 * Some mode letters in bottom screen line
 */
#define MODE_TRAILING   'T'
#define MODE_CONTROL_Z  'Z'
#define MODE_INSERT     'i'
#define MODE_OVERWRITE  'o'

#define REG_ALPHANUM    'a'     /* Regular expression predefined macros */
#define REG_WHITESPACE  'b'
#define REG_ALPHA       'c'
#define REG_DECIMAL     'd'
#define REG_HEX         'h'
#define REG_LOWER       'l'
#define REG_UPPER       'u'

#define TS_DAYMONTH     'd'     /* Time-stamp format characters */
#define TS_DAYWEEK      'D'
#define TS_ENTER        'e'
#define TS_12HOUR       'h'
#define TS_24HOUR       'H'
#define TS_MONTHNUM     'm'
#define TS_MONTHWORD    'M'
#define TS_MINUTES      'n'
#define TS_MERIDIAN     'p'     /* ie. am or pm */
#define TS_SECONDS      's'
#define TS_TAB          't'
#define TS_YEAR2        'y'
#define TS_YEAR         'Y'
#define TS_ZONE         'Z'

#define MAC_BackSpace   'b'     /* Macro literal escape characters */
#define MAC_CharLeft    'l'     /* In addition: 0 is MacroMark     */
#define MAC_CharRight   'r'     /*  and 1..3 are SetMark1..3       */
#define MAC_Rturn       'n'
#define MAC_Pseudo      'p'
#define MAC_Tab         't'

#if defined( __UNIX__ ) && !defined( PC_CHARS )
#define EOF_CHAR        '='
#else
#define EOF_CHAR        'Í'
#endif
#define EOL_TEXT        "EOL"           /* Should be three characters */

/*
 * Use this sign for window letters after the alphabet string
 * has been exhausted.
 */
#define LAST_WINDOWLETTER       '+'

/*
 * jmh 031116: output capture and search results indicators.
 */
#define OUTPUT          '|'
#define RESULTS         '='

#define CHECK           'x'     /* Indicate a checked option */


/*
 * Here comes positions for texts in the dirlist window.
 * All positions are relative to the window frame.
 */
#define DIR3_ROW   1   /* dir3: Directory : */
#define DIR3_COL   2
#define DIR4_ROW   2   /* dir4: File name : */
#define DIR4_COL   2
#define DIR5_ROW   3   /* dir5: File time : */
#define DIR5_COL   2
#define DIR6_ROW   4   /* dir6: File size : */
#define DIR6_COL   2
#define DIR7_ROW   4   /* dir7: File count: */
#define DIR8_COL  11   /* dir8: Cursor key move. Enter selects ... */
#define DIRA_COL  28   /* attribute, added by jmh 980527 */
#define DIRA_ROW   4

#define DIR10_ROW  1   /* dir10: Language : */
#define DIR10_COL  2
#define DIR11_ROW  2   /* dir11: Pattern  : */
#define DIR11_COL  2
#define DIR12_ROW  3   /* dir12: Case     : */
#define DIR12_COL  2
#define DIR13_ROW  3   /* dir13: Count :    */


/*
 * The characters used to draw the dirlist and menu frames
 * jmh 991019: now they're indices into graphic_char[][]
 */
#define HORIZONTAL_LINE   10
#define VERTICAL_LINE      0
#define CORNER_LEFT_UP     7
#define CORNER_RIGHT_UP    9
#define CORNER_LEFT_DOWN   1
#define CORNER_RIGHT_DOWN  3
#define LEFT_T             4
#define RIGHT_T            6
#define TOP_T              8
#define BOTTOM_T           2
#define INTERSECTION       5


/* Moved from tdestr.h: */
/*
 * characters used in tdeasm.c to display eol and column pointer in ruler
 */
#if defined( __UNIX__ ) && !defined( PC_CHARS )
 #define EOL_CHAR        0x1e           /* ^ as a control character */
 #define NEOL_CHAR       0x1c           /* \ as a control character */
 #define RULER_PTR       0x21
 #define RULER_FILL      0x2e
 #define RULER_TICK      0x2b
 #define LM_CHAR         0x6c
 #define RM_CHAR_RAG     0x3c
 #define RM_CHAR_JUS     0x7c
 #define PGR_CHAR        0x70
 #define RULER_LINE      '-'            /* popup ruler */
 #define RULER_EDGE      '|'
 #define RULER_CRNRA     '+'
 #define RULER_CRNRB     '+'
 /*
  * character used to separate vertical screens
  */
 #define VERTICAL_CHAR   0x7c
 /*
  * character used to separate line number from line text
  */
 #define HALF_SPACE      0x20
 /*
  * character used to indicate a pop-out menu item
  */
 #define POP_OUT         0x3e
#else
 #define EOL_CHAR        0x11
 #define NEOL_CHAR       0x10
 #define RULER_PTR       0x19
 #define RULER_FILL      0xf9
 #define RULER_TICK      0x04
 #define LM_CHAR         0xb4
 #define RM_CHAR_RAG     0x3c
 #define RM_CHAR_JUS     0xc3
 #define PGR_CHAR        0x14
 #define RULER_LINE      'Ä'            /* popup ruler */
 #define RULER_EDGE      '³'
 #define RULER_CRNRA     'Ú'
 #define RULER_CRNRB     'À'
 /*
  * character used to separate vertical screens
  */
 #define VERTICAL_CHAR   0xba
 /*
  * character used to separate line number from line text
  */
 #define HALF_SPACE      0xdd
 /*
  * character used to indicate a pop-out menu item
  */
 #define POP_OUT         0x10
#endif

/*
 * jmh 990410: status screen array co-ordinates.
 */
#define STAT_WIDTH      60      /* 991021: maximum width of display */

#define STAT_PREV        5
#define STAT_MARK        6
#define STAT_BLOCK      11
#define STAT_BREAK      13
#define STAT_LANG       15
#define STAT_REF        17
#define STAT_FTIME      19
#define STAT_TIME       20
#define STAT_COUNT      22

/*
 * jmh 990410: statistics screen array co-ordinates.
 */
#define STATS_LINE       5
#define STATS_WORD       5
#define STATS_CHARS     10
#define STATS_SPACE     10
#define STATS_SIZE      14
#define STATS_MEM       15
#define STATS_COUNT     17

/*
 * jmh 990430: maximum size of the help screens.
 */
#define HELP_HEIGHT     50
#define HELP_WIDTH      100

/*
 * jmh 991111: position of character zero in char_help.
 */
#define ZERO_COL        7
#define ZERO_ROW        4

/*
 * jmh 021028: position to display isearch string (length of find18a/b).
 */
#define ISEARCH         18
