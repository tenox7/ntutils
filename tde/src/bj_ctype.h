/****************************  start of original comments  ***************/
/* Editor:      TDE, the Thomson-Davis Editor
 * Filename:    myctype.h
 * Compiled by: Byrial Jensen
 *
 * This file is a replacement for the standard header file ctype.h
 * and defines the same ctype macros as in ctype.h
 * This file make use of the chartypes table in file prompts2.h
 * The macro definitions are similar to those in ctype.h (at least for
 * my compiler: Turbo C 2.0) but there the _ctype table starts with EOF (-1),
 * here first char is \0, so here is no need to adjust indexes in chartypes.
 *
 * Note: The standard library function atol( ) uses ctype.h to find and
 * and pass thru leading space in its argument string before converting it to
 * a number. So to avoid including of the original 257-byte _ctype table
 * in the executable you can recompile atol( ) with the necessary changes
 * to use this header file instead of ctype.h
 */
/************************** end of original comments  ******************/

/*
 * Byrial, thank you for sending in your replacement ctype file.  I modified
 * your code to work in Linux as well as in DOS.  Although I'm not sure, but
 * I suspect that most, if not all, PC C compilers use a 257 byte look-up table.
 * The reason for using a 257 byte table is because ANSI C requires the ctype
 * functions to handle EOF as well as the 256 ASCII and extended ASCII
 * characters, see:
 *
 *  Brian W. Kernighan and Dennis M. Ritchie, _The C Programming
 *    Language_, 2nd edition, Prentice-Hall, Englewood Cliffs, New
 *    Jersey, 1988, Appendix B2, pp 248-249, ISBN 0-13-110362-8.
 *
 *  P. J. Plauger, _The Standard C Library_, Prentice-Hall, Englewood
 *    Cliffs, New Jersey, 1992, Chapter 2, "<ctype.h>", pp 25-46,
 *    ISBN 0-13-131509-9.
 *
 *  Samuel P. Harbison and Guy L. Steele, Jr., _C, A Reference Manual_,
 *    3rd edition, Prentice-Hall, Englewood Cliffs, New Jersey, 1991,
 *    Chapter 12, "Character Processing", pp 277-283, ISBN 0-13-110933-2.
 *
 *
 * Name:  TDE, the Thomson-Davis Editor
 * Date:  November 13, 1993, version 3.2
 *
 * This code is released into the public domain, Frank Davis.
 * You may distribute it freely.
 *
 *                             ctype in TDE
 *
 *  Instead of using the Standard C ctype library, we will use our own
 *  ctype functions.  Our ctype functions are prefaced with bj_ (for
 *  Byrial Jensen, who thought up our scheme.)  Our 257 byte tables are
 *  at the bottom of the prompts.c file.  Our table defines ctypes for
 *  characters in the range 128-255 (Standard C does not define this range),
 *  which include accented characters used in various alphabets and languages.
 *  So, we actually have two sets of ctype functions: Standard C and
 *  Byrial Jensen.
 *
 * jmh 980808: replaced (int)(c) with (unsigned char)(c)
 *             added BJ_alpha
 *
 * jmh 010607: since we're using our own ctype table and not the ANSI C one, why
 *             should we bother with EOF?
 */

#define BJ_cntrl    0x01  /* bit mask for control character */
#define BJ_upper    0x02  /* bit mask for uppercase letter */
#define BJ_lower    0x04  /* bit mask for lowercase letter */
#define BJ_digit    0x08  /* bit mask for digit */
#define BJ_xdigit   0x10  /* bit mask for hex digits */
#define BJ_space    0x20  /* bit mask for space */
#define BJ_punct    0x40  /* bit mask for printing characters except
                                          space, letter, or digit */
#define BJ_alpha    (BJ_upper | BJ_lower)

#define bj_isalnum(c)  (bj_ctype[(unsigned char)(c)] & (BJ_alpha | BJ_digit))
#define bj_isalpha(c)  (bj_ctype[(unsigned char)(c)] & BJ_alpha)
#define bj_iscntrl(c)  (bj_ctype[(unsigned char)(c)] & BJ_cntrl)
#define bj_isdigit(c)  (bj_ctype[(unsigned char)(c)] & BJ_digit)
#define bj_islower(c)  (bj_ctype[(unsigned char)(c)] & BJ_lower)
#define bj_ispunct(c)  (bj_ctype[(unsigned char)(c)] & BJ_punct)
#define bj_isspace(c)  (bj_ctype[(unsigned char)(c)] & BJ_space)
#define bj_isupper(c)  (bj_ctype[(unsigned char)(c)] & BJ_upper)
#define bj_isxdigit(c) (bj_ctype[(unsigned char)(c)] & BJ_xdigit)

/*
 * These two functions are (re)defined in bj_ctype.c
 */
int  bj_tolower( int c );
int  bj_toupper( int c );
