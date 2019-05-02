/*
 * Editor:      TDE, the Thomson-Davis Editor
 * Filename:    myctype.c
 * Compiled by: Byrial Jensen
 *
 * This file redefines the two standard library files tolower and toupper
 *
 * jmh 980808: Modified to work with signed char.
 *             The original function was
 *                if (c & 0xff00)
 *                   return c;
 *                else
 *                   return ...
 *             which ignores negative numbers, which means characters above
 *             127 are ignored (since char is usually signed by default),
 *             which rather defeats the entire purpose.
 */


#include "tdestr.h"
#include "common.h"


int  bj_tolower( int c )
{
   c &= 0xff;
   return( bj_isupper( c ) ? (int)upper_lower[c] : c );
}


int  bj_toupper( int c )
{
   c &= 0xff;
   return( bj_islower( c ) ? (int)upper_lower[c] : c );
}


/*
 * Provide a functional version of bj_isspace for use with word movement
 * Added by jmh, 28 July, 1997
 */
int  bj_isspc( int c )
{
   return( bj_isspace( c ) );
}


/*
 * Provide a functional inverse version for use with word movement.
 * jmh 991022: Made global, renamed from not_bjisspc.
 */
int  bj_isnotspc( int c )
{
   return( !bj_isspace( c ) );
}
