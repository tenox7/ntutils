/*
 * Editor name:      tde, the Thomson-Davis Editor.
 * Filename:         syntax.h
 * Author:           Jason Hood
 * Date:             January 24, 1998
 *
 * Syntax highlighting definitions.
 */

/*
 * Bit masks for the syntax highlighting type of a line. The suffixes are:
 *   _START  :  starts somewhere, finishes at eol
 *   _WHOLE  :  the entire line
 *   _END    :  starts at bol, finishes somewhere
 * Note that 1 is for the DIRTY flag.
 */
#define COM_START       0x0002
#define COM_WHOLE       0x0004
#define COM_END         0x0008
#define STR_START       0x0010
#define STR_WHOLE       0x0020
#define STR_END         0x0040
#define CHR_START       0x0080
#define CHR_WHOLE       0x0100
#define CHR_END         0x0200
/*
 * Preprocessor is handled a little differently.
 */
#define PREPRO          0x0400  /* the line is for the preprocessor    */
#define PREPRO_START    0x0800  /* the first line for the preprocessor */
#define PREPRO_END      0x1000  /* the last line for the preprocessor  */
/*
 * Strings and characters need additional flags to indicate whether it is good
 * or bad, depending on the spanning characteristics.
 */
#define START_VALID     0x2000
#define WHOLE_VALID     0x4000
#define END_VALID       0x8000
/*
 * jmh 011122: flag to indicate which multi-line comment we're in.
 */
#define COM_NUMBER     0x10000L

/*
 * The colors for each syntax component.
 */
enum {
   COL_NORMAL,                  /* color for everything not below and spaces */
   COL_BAD,                     /* something's not right */
   COL_KEYWORD,                 /* default color */
   COL_COMMENT,                 /* color for both comment types */
   COL_FUNCTION,
   COL_STRING,
   COL_CHARACTER,
   COL_INTEGER,
   COL_BINARY,
   COL_OCTAL,
   COL_HEX,
   COL_REAL,                    /* ie. floating point */
   COL_PREPRO,                  /* preprocessor */
   COL_SYMBOL                   /* everything that's not alphanumeric */
};

/*
 * Indices for the syntax highlighting file. Add 256 to these to distinguish
 * between explicit color settings.
 */
enum {
   SHL_NORMAL = 256,
   SHL_BAD,
   SHL_KEYWORD,
   SHL_COMMENT,
   SHL_FUNCTION,
   SHL_STRING,
   SHL_CHARACTER,
   SHL_INTEGER,
   SHL_BINARY,
   SHL_OCTAL,
   SHL_HEX,
   SHL_REAL,
   SHL_PREPRO,
   SHL_SYMBOL,
   SHL_LANGUAGE,
   SHL_PATTERN,
   SHL_CASE,
   SHL_NEWLINE,
   SHL_SPANLINE,
   SHL_ESCAPE,
   SHL_STARTWORD,
   SHL_INWORD,
   SHL_INNUMBER,
   SHL_MACRO,
   SHL_BACKGROUND,
   SHL_MENU,
   SHL_INFLATETABS,
   SHL_PTABSIZE,
   SHL_LTABSIZE
};


/*
 * Defines for identifiers and numbers (for identifier[]).
 */
#define ID_STARTWORD  1
#define ID_INWORD     2
#define ID_DIGIT      4
#define ID_BINARY     8
#define ID_OCTAL     16
#define ID_DECIMAL   32
#define ID_HEX       64
/*
 * Additional defines for numbers (not part of identifier[]).
 */
#define ID_POINT    128
#define ID_EXPONENT 256
#define ID_REAL     (ID_POINT | ID_EXPONENT)


/*
 * Defines for number (the first character of the base).
 */
#define PREFIX 1
#define SUFFIX 2


/* This macro computes a hash value for a word, which is used for looking
 * the word up in the keyword[] table.  The word is known to be at least
 * one character long and terminated with a '\0', so word[1] is guaranteed to
 * be valid and consistent. Originally by Steve Kirkendall, tweaked by jmh.
 * Note: uses only 5 bits to provide an implicit case conversion.
 */
#define KWHASH(word)    (((word)[0] & 0x1f) ^ (((word)[1] & 0x0f) << 3) \
                         ^ ((word)[1] ? (((word)[2] & 0x1f) << 2) : 0))
