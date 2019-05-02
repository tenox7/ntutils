/*
 * These functions compile a regular expression into an NFA and recognize
 * a pattern.
 *
 * Regular expressions are often used to describe a pattern that might
 * match several different or similar words.  An example of a regular
 * expression subset is the wild card characters '*' and '?' used to list
 * files in a directory.
 *
 * Might as well read the books and papers written by those who first wrote
 * the various [a-z]?greps.  Ken Thompson was the author of grep.  In one
 * weekend, Al Aho wrote egrep and fgrep.  Incidentally, when Dr. Aho was
 * the guest speaker at the Distinguished Lecture Series at Georgia Tech,
 * he personally autographed my copy of his book  _Compilers:  Principles,
 * Techniques, and Tools_, aka the "Dragon Book."
 *
 * See:
 *
 *   Ken Thompson, "Regular Expression Search Algorithm."  _Communications
 *      of the ACM_, 11 (No. 6): 419-422, 1968.
 *
 *   Alfred V. Aho, Ravi Sethi, and Jeffrey D. Ullman, _Compilers: Principles,
 *      Techniques, and Tools_, Addison-Wesley, Reading, Mass., 1986,
 *      Chapter 3, "Lexical Analysis", pp 83-158.  ISBN 0-201-10088-6.
 *
 *   Robert Sedgewick, _Algorithms in C_, Addison-Wesley, Reading, Mass.,
 *      1990, Chapter 20, "Pattern Matching", pp. 293-303, and Chapter 21,
 *      "Parsing", pp. 305-317.  ISBN 0-201-51425-7.
 *
 *   Andrew Hume, "A Tale of Two Greps."  _Software--Practice and Experience_,
 *      18 (No. 11): 1063-1072, 1988.
 *
 *
 *                         Regular Expressions in TDE
 *
 * The core of the regular expression search algorithm in TDE is based on
 * the nondeterministic finite-state machine in Dr. Segdewick's book.
 * Additional parsing, operator precedence, and nfa construction and
 * simulation info is from Dr. Aho's book.  The nfa node naming convention
 * and additional nfa construction guidelines are from Ken Thompson's paper.
 * I'm much too stupid to compile a regular expression into assembly language
 * as suggested in Ken Thompson's paper, but I did get the idea to do the
 * next best thing and added C library functions as NNODE terminal types.
 *
 * The local global NFA is builded with two arrays and a deque.  The
 * first array, next1, in the NFA is used to hold the path for lambda
 * transitions.  The second array, next2, is used to hold alternate paths.
 * Next1 can hold all types of nodes, but it gets priority for lambda
 * transitions.  The deque is a circular array where future states are queued
 * in the head and present states are pushed in the tail.
 *
 * jmh: To be consistent with other regular expression implementations, the
 * longest match should be returned, not the shortest which the above does.
 * Fortunately, returning the longest match is a trivial change, simply
 * continue scanning if the deque is not empty.  However, other implementations
 * have "greedy" operators - it is the operator itself that matches as much as
 * possible, rather than the entire expression.  As far as the overall match is
 * concerned, this is just semantics, but it makes quite a difference when
 * capturing the substrings used by replacement.  Unfortunately, the deque
 * method prevents the reliable capture of substrings, nor does it allow the
 * operators to determine their own greediness.  So instead of using a deque to
 * store states, a recursive function is used to test one state; if it fails,
 * the scan continues with the other state.  Greediness is controlled by simply
 * choosing which state is done first.
 *
 * Searching for regular expressions in TDE is not very fast.  The worst
 * case search, .*x, where x is any word or letter, is probably the slowest
 * of any implementation with a regular expression search function.  However,
 * TDE can handle a large regular expression subset.
 *
 * In version 3.1, I added BOW and EOW (beginning of word and end of word.)
 *
 * In version 3.2, the macro letters were moved to letters.h per Byrial Jensen.
 * We also use the bj_isxxx( ) functions to test whether a letter falls into
 * a predefined macro class.
 *
 * jmh 050729: added BOB and EOB (beginning & end of bracket/backreference).
 *              This is used by the replacement to substitute bracketed
 *              expressions and for backreferences.
 * jmh 050803: added BLANK, separate from WHITESPACE.
 * jmh 050811: added BOS and EOS (beginning & end of string).
 * jmh 050817: added a closing lookahead assertion.
 * jmh 050908: replaced the deque with a recursive function.  This was
 *              necessary to allow for separate greedy and non-greedy operators
 *              and to correctly obtain backreferences.
 *
 * New editor name:  TDE, the Thomson-Davis Editor.
 * Author:           Frank Davis
 * Date:             June 5, 1991, version 1.0
 * Date:             July 29, 1991, version 1.1
 * Date:             October 5, 1991, version 1.2
 * Date:             January 20, 1992, version 1.3
 * Date:             February 17, 1992, version 1.4
 * Date:             April 1, 1992, version 1.5
 * Date:             June 5, 1992, version 2.0
 * Date:             October 31, 1992, version 2.1
 * Date:             April 1, 1993, version 2.2
 * Date:             June 5, 1993, version 3.0
 * Date:             August 29, 1993, version 3.1
 * Date:             November 13, 1993, version 3.2
 * Date:             June 5, 1994, version 4.0
 * Date:             December 5, 1998, version 5.0 (jmh)
 *
 * This code is released into the public domain, Frank Davis.
 * You may use and distribute it freely.
 */

#include "tdestr.h"
#include "common.h"
#include "tdefunc.h"
#include "define.h"

#ifndef min
 #define min(a,b)       (((a) < (b)) ? (a) : (b))
#endif

/*
 * types of nodes in the NFA.
 *
 * let's use the node naming convention in Ken Thompson's reg ex paper.
 */
#define CNODE           0
#define NNODE           1

/*
 * types of terminals in NNODEs.
 *
 * starting with simple ASCII terminals, it's easy to build in other types of
 *  terminals, e.g. wild chars, BOL, EOL, and character classes.  actually,
 *  we can easily build ANSI C ctype library function NNODEs.
 */
#define STRAIGHT_ASCII  0
#define IGNORE_ASCII    1
#define WILD            2
#define CLASS           3
#define NOTCLASS        4
#define BLANK           5
#define WHITESPACE      6
#define ALPHA           7
#define UPPER           8
#define LOWER           9
#define ALPHANUM        10
#define DECIMAL         11
#define HEX             12
#define BOL             13
#define EOL             14
#define BOW             15
#define EOW             16
#define BOS             17
#define EOS             18
#define BOB             19
#define EOB             20
#define BACKREF         21


/*
 * types of terminals in CNODEs.
 */
#define START           0
#define ACCEPT          1
#define OR_NODE         2
#define JUXTA           3
#define CLOSURE         4
#define ZERO_OR_ONE     5
#define ASSERT          6
#define ASSERTNOT       7


static int  lookahead;
static int  regx_rc;
static int  parser_state;
static char class_bits[32];
static int  bit[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
static int  c1;
static int  c2;
static int  ttype;

static NFA_TYPE  *nfa_pointer;

       int  nfa_status;
static int  search_len;
static int  search_col;
static text_ptr search_string;

extern int  found_len;                  /* defined in findrep.c */
extern int  block_replace_offset;
extern long last_replace_line;
extern int  add_search_line( line_list_ptr, long, int );

static int  subs_found;
       int  subs_pos[10];
       int  subs_len[10];


/*
 * Name:    find_regx
 * Purpose: To set up and find a regular expression.
 * Date:    June 5, 1993
 * Passed:  window:  pointer to current window
 * Notes:   Keep the regular expression until changed.
 */
int  find_regx( TDE_WIN *window )
{
int  direction;
int  new_string;
register TDE_WIN *win;  /* put window pointer in a register */
int  rc;
char answer[MAX_COLS+2];

   switch (g_status.command) {
      case RegXForward :
         direction  = FORWARD;
         new_string = TRUE;
         break;
      case RegXBackward :
         direction  = BACKWARD;
         new_string = TRUE;
         break;
      case RepeatRegXForward :
         direction  = FORWARD;
         new_string =  regx.search_defined != OK ? TRUE : FALSE;
         break;
      case RepeatRegXBackward :
         direction  = BACKWARD;
         new_string =  regx.search_defined != OK ? TRUE : FALSE;
         break;
      default :
         assert( FALSE );
         return( ERROR );
   }
   win = window;
   if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
      return( ERROR );

   /*
    * get search text, using previous as default
    */
   rc = OK;
   if (new_string == TRUE) {
      *answer = '\0';
      if (regx.search_defined == OK) {

         assert( strlen( (char *)regx.pattern ) < (size_t)g_display.ncols );

         strcpy( answer, (char *)regx.pattern );
      }

      /*
       * jmh 991006: if there was an error processing the pattern ask again.
       */
      do {
         /*
          * string to find:
          */
         if (get_name( reg1, win->bottom_line, answer, &h_find ) <= 0)
            return( ERROR );

         strcpy( (char *)regx.pattern, answer );
         rc = regx.search_defined = build_nfa( );
      } while (regx.search_defined == ERROR);
   }
   g_status.search = SEARCH_REGX;
   if (direction == BACKWARD)
      g_status.search |= SEARCH_BACK;
   rc = perform_search( win );
   return( rc );
}


/*
 * Name:    regx_search_forward
 * Purpose: search forward for pattern using nfa
 * Date:    June 5, 1993
 * Passed:  ll:     pointer to current node in linked list
 *          rline:  pointer to real line counter
 *          col:    column in ll->line to begin search
 * Returns: position in file if found or NULL if not found
 * Notes:   run the nfa machine on each character in each line
 *
 * jmh 010704: fixed bug when on TOF.
 * jmh 031030: optimise the BOL search (don't check each character).
 */
line_list_ptr regx_search_forward( line_list_ptr ll, long *rline, int *col )
{
file_infos *file;
int bc, ec;
int anchor;

   if (ll->next == NULL)
      return( NULL );

   if (g_status.command == DefineGrep  ||  g_status.command == RepeatGrep) {
      nfa_pointer = &sas_nfa;
   } else {
      nfa_pointer = &nfa;
   }
   nfa_status = OK;
   search_string = ll->line;
   search_len = ll->len;
   search_col = *col;
   file = g_status.current_file;
   if ((g_status.search & SEARCH_BLOCK)  &&
       (file->block_type == BOX  ||
        (file->block_type == STREAM  &&
         (*rline == file->block_br ||
          (*rline == file->block_er && file->block_ec != -1))))) {
      bc = file->block_bc;
      ec = (file->block_ec == -1) ? search_len : file->block_ec;
      if (last_replace_line == *rline) {
         ec += block_replace_offset;
         block_replace_offset = 0;
         last_replace_line = 0;
      }
      if (file->inflate_tabs) {
         bc = entab_adjust_rcol( search_string, search_len, bc,
                                 file->ptab_size );
         if (file->block_ec != -1)
            ec = entab_adjust_rcol( search_string, search_len, ec,
                                    file->ptab_size );
      }
      if (bc > search_len)
         bc = search_len;
      if (ec >= search_len)
         ec = search_len - 1;

      if (file->block_type == STREAM) {
         if (*rline == file->block_br) {
            if (search_col < bc)
               search_col = bc;
         }
         if (*rline == file->block_er) {
            if (search_col > ec + 1)
               return( NULL );
            search_len = ec + 1;
            if (*rline != file->block_br)
               bc = 0;
         }
      } else {
         if (search_col < bc)
            search_col = bc;
         search_len = ec + 1;
      }
      search_string += bc;
      search_len    -= bc;
      search_col    -= bc;
   } else
      bc = 0;

   anchor = nfa_pointer->next1[0];
   anchor = (nfa_pointer->node_type[anchor] == NNODE &&
             nfa_pointer->term_type[anchor] == BOL);
   while (!g_status.control_break) {
      if (anchor) {
         if (nfa_match( ) != ERROR) {
            if (g_status.search & SEARCH_RESULTS) {
               if (!add_search_line( ll, *rline, 0 ))
                  return( NULL );
            } else {
               *col = bc;
               return( ll );
            }
         }
      } else {
         for (; search_col <= search_len; search_col++) {
            if (nfa_match( ) != ERROR) {
               if (g_status.search & SEARCH_RESULTS) {
                  if (!add_search_line( ll, *rline, search_col ))
                     return( NULL );
                  break;
               } else {
                  *col = search_col + bc;
                  return( ll );
               }
            }
         }
      }
      ll = ll->next;
      if (ll->len == EOF)
         return( NULL );
      ++*rline;
      search_string = ll->line;
      search_len = ll->len;
      search_col = 0;
      if ((g_status.search & SEARCH_BLOCK)  &&
          (file->block_type == BOX  ||
           (file->block_type == STREAM  &&
            *rline == file->block_er && file->block_ec != -1))) {
         bc = file->block_bc;
         ec = file->block_ec;
         if (file->inflate_tabs) {
            bc = entab_adjust_rcol( search_string, search_len, bc,
                                    file->ptab_size );
            ec = entab_adjust_rcol( search_string, search_len, ec,
                                    file->ptab_size );
         }
         if (bc > search_len)
            bc = search_len;
         if (ec >= search_len)
            ec = search_len - 1;

         if (file->block_type == STREAM)
            search_len = ec + 1;
         else {
            search_string += bc;
            search_len     = ec - bc + 1;
         }
      }
   }
   return( NULL );
}


/*
 * Name:    regx_search_backward
 * Purpose: search backward for pattern using regular expression
 * Date:    June 5, 1993
 * Passed:  ll:     pointer to current node in linked list
 *          rline:  pointer to real line counter
 *          col:    column in ll->line to begin search
 * Returns: position in file if found or NULL if not found
 * Notes:   run the nfa machine on each character in each line
 */
line_list_ptr regx_search_backward( line_list_ptr ll, long *rline, int *col )
{
file_infos *file;
int  bc, ec;
int  anchor;

   if (ll == NULL || ll->len == EOF)
      return( NULL );

   nfa_pointer = &nfa;

   search_string = ll->line;
   search_len = ll->len;
   search_col = *col;
   file = g_status.current_file;
   if ((g_status.search & SEARCH_BLOCK)  &&
       (file->block_type == BOX  ||
        (file->block_type == STREAM  &&
         (*rline == file->block_br ||
          (*rline == file->block_er && file->block_ec != -1))))) {
      bc = file->block_bc;
      ec = (file->block_ec == -1) ? search_len : file->block_ec;
      if (file->inflate_tabs) {
         bc = entab_adjust_rcol( search_string, search_len, bc,
                                 file->ptab_size );
         if (file->block_ec != -1)
            ec = entab_adjust_rcol( search_string, search_len, ec,
                                    file->ptab_size );
      }
      if (bc > search_len)
         bc = search_len;
      if (ec >= search_len)
         ec = search_len - 1;

      if (file->block_type == STREAM) {
         if (*rline == file->block_er) {
            search_len = ec + 1;
            if (*rline != file->block_br)
               bc = 0;
            if (search_col > search_len)
               search_col = search_len;
         }
         if (*rline == file->block_br) {
            if (search_col < bc)
               return( NULL );
         }
      } else {
         search_len = ec + 1;
         if (search_col > search_len)
            search_col = search_len;
      }
      search_string += bc;
      search_len    -= bc;
      search_col    -= bc;
   } else
      bc = 0;

   anchor = nfa.next1[0];
   anchor = (nfa.node_type[anchor] == NNODE && nfa.term_type[anchor] == BOL);
   while (!g_status.control_break) {
      if (anchor) {
         if (search_col >= 0) {
            search_col = 0;
            if (nfa_match( ) != ERROR) {
               if (g_status.search & SEARCH_RESULTS) {
                  if (!add_search_line( ll, *rline, 0 ))
                     return( NULL );
               } else {
                  *col = bc;
                  return( ll );
               }
            }
         }
      } else {
         for (; search_col >= 0; search_col--) {
            if (nfa_match( ) != ERROR) {
               if (g_status.search & SEARCH_RESULTS) {
                  if (!add_search_line( ll, *rline, search_col ))
                     return( NULL );
                  break;
               } else {
                  *col = search_col + bc;
                  return( ll );
               }
            }
         }
      }
      ll = ll->prev;
      if (ll->len == EOF)
         return( NULL );
      --*rline;
      search_string = ll->line;
      search_col = search_len = ll->len;
      if ((g_status.search & SEARCH_BLOCK)  &&
          (file->block_type == BOX  ||
           (file->block_type == STREAM  &&  *rline == file->block_br))) {
         bc = file->block_bc;
         ec = (file->block_ec == -1) ? 0 : file->block_ec;
         if (file->inflate_tabs) {
            bc = entab_adjust_rcol( search_string, search_len, bc,
                                    file->ptab_size );
            ec = entab_adjust_rcol( search_string, search_len, ec,
                                    file->ptab_size );
         }
         if (bc > search_len)
            bc = search_len;
         if (ec >= search_len)
            ec = search_len - 1;

         if (file->block_type == BOX)
            search_len = ec + 1;
         search_string += bc;
         search_len    -= bc;
         search_col     = search_len;
      }
   }
   return( NULL );
}


/******************************  NFA Recognizer  *********************/

//#define SHOW_NFA
//#define SHOW_SUBS
//#define SHOW_STATE

/*
 * Name:    nfa_match
 * Purpose: try to recognize a pattern
 * Date:    June 5, 1993
 * Passed:  none, but modifies local global nfa recognizer.
 * Returns: length of recognized pattern if found or ERROR if not found.
 */
int  nfa_match1( int state, int j );
int  nfa_match( void )
{
int  len;

   memset( subs_pos, 0, sizeof(subs_pos) );
   memset( subs_len, 0, sizeof(subs_len) );
   sort.order_array = (mode.search_case == IGNORE) ? sort_order.ignore
                                                   : sort_order.match;

   len = nfa_match1( nfa_pointer->next1[0], search_col );
   if (len != ERROR)
      subs_len[0] = found_len = len;

#if defined( SHOW_SUBS )
   if (subs_found > 1) {
# if defined( SHOW_STATE )
   putchar( '\n' );
# endif
   fputs( "Subs:", stdout );
   for (int i = 1; i < subs_found; ++i)
      printf( " %d:%d", subs_pos[i], subs_len[i] );
   putchar( '\n' );
# if defined( SHOW_STATE ) || defined( SHOW_NFA )
   putchar( '\n' );
# endif
   }
#endif

   return( len );
}


int  nfa_match1( int state, int j )
{
register int c;
int  i;
int  n1, n2;
int  rc;

   c =  j < search_len  ?  search_string[j]  :  EOF;

   while (state) {
#if defined( SHOW_STATE )
      printf( "%d: ", state );
#endif
      n1 = nfa_pointer->next1[state];
      if (nfa_pointer->node_type[state] == NNODE) {
         rc = OK;
         switch (nfa_pointer->term_type[state]) {
            case STRAIGHT_ASCII :
               if (c != EOF  &&  nfa_pointer->c[state] == c)
                  break;
               return( ERROR );
            case IGNORE_ASCII :
               if (c != EOF  &&  nfa_pointer->c[state] == bj_tolower( c ))
                  break;
               return( ERROR );
            case WILD :
               if (c != EOF)
                  break;
               return( ERROR );
            case CLASS :
               if (c != EOF  &&  nfa_pointer->class[state][c/8] & bit[c%8])
                  break;
               return( ERROR );
            case NOTCLASS :
               if (c != EOF  &&  !(nfa_pointer->class[state][c/8] & bit[c%8]))
                  break;
               return( ERROR );
            case BLANK :
               if (c == ' '  ||  c == '\t')
                  break;
               return( ERROR );
            case WHITESPACE :
               if (c != EOF  &&  !bj_isspace( c ) == nfa_pointer->c[state])
                  break;
               return( ERROR );
            case ALPHA :
               if (c != EOF  &&  !bj_isalpha( c ) == nfa_pointer->c[state])
                  break;
               return( ERROR );
            case UPPER :
               if (c != EOF  &&  !bj_isupper( c ) == nfa_pointer->c[state])
                  break;
               return( ERROR );
            case LOWER :
               if (c != EOF  &&  !bj_islower( c ) == nfa_pointer->c[state])
                  break;
               return( ERROR );
            case ALPHANUM :
               if (c != EOF  &&  !bj_isalnum( c ) == nfa_pointer->c[state])
                  break;
               return( ERROR );
            case DECIMAL :
               if (c != EOF  &&  !bj_isdigit( c ) == nfa_pointer->c[state])
                  break;
               return( ERROR );
            case HEX :
               if (c != EOF  &&  !bj_isxdigit( c ) == nfa_pointer->c[state])
                  break;
               return( ERROR );
            case BOL :
               if (j == 0) {
                  rc = TRUE;
                  break;
               }
               return( ERROR );
            case EOL :
               if (j == search_len) {
                  rc = TRUE;
                  break;
               }
               return( ERROR );
            case BOW :
               if (c != EOF  &&  !myiswhitespc( c )) {
                  if (j == 0  ||  myiswhitespc( search_string[j-1] )) {
                     rc = TRUE;
                     break;
                  }
               }
               return( ERROR );
            case EOW :
               if (c == EOF) {
                  if (search_len > 0) {
                     if (!myiswhitespc( search_string[search_len-1] )) {
                        rc = TRUE;
                        break;
                     }
                  }
               } else {
                  if (myiswhitespc( c )) {
                     if (j > 0  &&  !myiswhitespc( search_string[j-1] )) {
                        rc = TRUE;
                        break;
                     }
                  }
               }
               return( ERROR );
            case BOS :
               if (c != EOF  &&  !bj_isspace( c )) {
                  if (j == 0  ||  bj_isspace( search_string[j-1] )) {
                     rc = TRUE;
                     break;
                  }
               }
               return( ERROR );
            case EOS :
               if (c == EOF) {
                  if (search_len > 0) {
                     if (!bj_isspace( search_string[search_len-1] )) {
                        rc = TRUE;
                        break;
                     }
                  }
               } else {
                  if (bj_isspace( c )) {
                     if (j > 0  &&  !bj_isspace( search_string[j-1] )) {
                        rc = TRUE;
                        break;
                     }
                  }
               }
               return( ERROR );
            case BOB :
               i = nfa_pointer->c[state];
               if (i < 10)
                  subs_pos[i] = j - search_col;
               rc = TRUE;
               break;
            case EOB :
               i = nfa_pointer->c[state];
               if (i < 10)
                  subs_len[i] = j - search_col - subs_pos[i];
               rc = TRUE;
               break;
            case BACKREF :
               i = nfa_pointer->c[state];
               if (subs_len[i] > 0  &&  subs_len[i] <= search_len - j) {
                  if (my_memcmp( search_string + j,
                                 search_string + search_col + subs_pos[i],
                                 subs_len[i] ) == 0) {
                     j += subs_len[i] - 1;
                     break;
                  }
               }
               return( ERROR );
            default :
               assert( FALSE );
               return( ERROR );
         }
         if (rc == OK) {
            ++j;
            c =  j < search_len  ?  search_string[j]  :  EOF;
         }
         state = n1;
      } else {
         assert( nfa_pointer->node_type[state] == CNODE );

         if (nfa_pointer->term_type[state] == ASSERT) {
            if (nfa_match1( n1, j ) != ERROR)
               break;
            return( ERROR );
         }
         if (nfa_pointer->term_type[state] == ASSERTNOT) {
            if (nfa_match1( n1, j ) == ERROR)
               break;
            return( ERROR );
         }

         n2 = nfa_pointer->next2[state];
         if (n1 == n2) {
            state = n1;
            continue;
         }
         /*
          * special case for greedy .* - it will always match everything,
          * so go straight to the end and work backwards.
          */
         if (n2 == state - 1  &&  nfa_pointer->node_type[n2] == NNODE  &&
                                  nfa_pointer->term_type[n2] == WILD) {
            rc = ERROR;
            for (i = search_len; i >= j; --i)
               if ((rc = nfa_match1( n1, i )) != ERROR)
                  break;
         } else {
            rc = nfa_match1( n2, j );
            if (rc == ERROR) {
               state = n1;
               continue;
            }
         }
         return( rc );
      }
   }

   return( j - search_col );
}


/***************************  NFA Compiler  **************************/


/*
 * Name:    build_nfa
 * Purpose: initialize nfa and call the compiler
 * Date:    June 5, 1993
 * Passed:  none, but looks at local global pattern.
 * Returns: whether or not an ERROR occured
 * Notes:   sets up the initial variables for the compiler.  builds
 *          initial and acceptor states for the nfa after compiler finishes.
 */
int  build_nfa( void )
{
   regx_rc = OK;

   init_nfa( );
   subs_found = 1;              /* 0 is always found */
   lookahead = 0;
   parser_state = 1;

   nfa.next1[0] = expression( );
   if (regx_rc == OK) {
      emit_cnode( 0, START, nfa.next1[0], nfa.next1[0] );
      emit_cnode( parser_state, ACCEPT, 0, 0 );
      regx.node_count = parser_state + 2;
   }

#if defined( SHOW_NFA )
   {
   int  i, n;
   static char* ntype[] = { "CNODE", "NNODE" };
   static char* ttype[2][22] = {
      { "START ", "ACCEPT", "OR    ", "JUXTA ", "CLOSE ",
        "OPTION", "ASSERT", "ASERT!" },
      { "ASCII ", "IASCII", "WILD  ", "CLASS ", "!CLASS",
        "BLANK ", "SPACE ", "ALPHA ", "UPPER ", "LOWER ",
        "ALNUM ", "DECI  ", "HEX   ", "BOL   ", "EOL   ",
        "BOW   ", "EOW   ", "BOS   ", "EOS   ",
        "BOB   ", "EOB   ", "BACKRF" } };

      for (i = 0; nfa.term_type[i] != 0 || i == 0; i++) {
         n = nfa.node_type[i];
         printf( "%2d: %s, %s, n1 = %2d, n2 = %2d",
                 i, ntype[n], ttype[n][nfa.term_type[i]],
                 nfa.next1[i], nfa.next2[i] );
         if (n == NNODE) {
            printf( ", c = " );
            if (nfa.c[i] < 32)
               printf( "\\%c", nfa.c[i] + '0' );
            else
               putchar( nfa.c[i] );
         }
         putchar( '\n' );
      }
      putchar( '\n' );
   }
#endif

   if (g_status.command == DefineGrep) {
      memcpy( &sas_nfa, &nfa, sizeof(NFA_TYPE) );
      memcpy( &sas_regx, &regx, sizeof(REGX_INFO) );
   }
   return( regx_rc );
}


/*
 * Name:    expression
 * Purpose: break reg ex into terms and expressions
 * Date:    June 5, 1993
 * Passed:  none, but looks at local global pattern.
 * Returns: none
 * Notes:   because recursion can go fairly deep, almost all variables
 *          were made local global.  no need to save most of this stuff
 *          on the stack.
 *
 * jmh 050707: fixed a bug with '?' followed by bracketed alternatives.
 */
int  expression( void )
{
int t1;
int t2;
int r;

   t1 = term( );
   r = t1;
   if (regx.pattern[lookahead] == '|') {
      lookahead++;
      parser_state++;
      r = t2 = parser_state;
      parser_state++;
      emit_cnode( t2, OR_NODE, expression( ), t1 );
      emit_cnode( t2-1, JUXTA, parser_state, parser_state );

      /*
       * the OR_NODE seems to need a path from the previous node
       */
      if (nfa.node_type[t1] == CNODE)
         t1 = min( nfa.next1[t1], nfa.next2[t1] );
      nfa.next1[t1-1] = t2;
      if (nfa.node_type[t1-1] == NNODE)
         nfa.next2[t1-1] = t2;
      else if (nfa.term_type[t1-1] == ZERO_OR_ONE)
         nfa.next1[t1-2] = nfa.next2[t1-2] = t2;
   }
   return( r );
}


/*
 * Name:    term
 * Purpose: break reg ex into factors and terms
 * Date:    June 5, 1993
 * Passed:  none, but looks at local global pattern.
 * Returns: none
 */
int  term( void )
{
int r;

   r = factor( );
   if (regx.pattern[lookahead] == '('  ||  letter( regx.pattern[lookahead] ))
      term( );
   else if (Kleene_star( regx.pattern[lookahead] ))
      regx_error( reg11 );
   return( r );
}


/*
 * Name:    factor
 * Purpose: add NNODEs and CNODEs to local global nfa
 * Date:    June 5, 1993
 * Passed:  none
 * Returns: none, but modifies local global nfa.
 * Notes:   this function does most of the compiling.  it recognizes all
 *          NNODEs, predefined macros, escape characters, and operators.
 */
int  factor( void )
{
int  t1;
int  t2;
int  r;
int  c;
int  sub = 0;
int  bq = ERROR;

   t2 = t1 = parser_state;
   c = regx.pattern[lookahead];
   if (c == '(') {
      if (regx.pattern[++lookahead] == '?') {
         c = regx.pattern[++lookahead];
         if (c != ':'  &&  c != '='  &&  c != '!') {
            /*
             * unrecognized character after (?
             */
            regx_error( reg13 );
            return( 0 );
         }
         ++lookahead;
         bq = TRUE;
         if (c != ':') {
            emit_cnode( parser_state, (c == '!') ? ASSERTNOT : ASSERT,
                        parser_state+1, parser_state+1 );
            parser_state++;
            bq++;
         }
      } else {
         sub = subs_found++;
         emit_nnode( parser_state, BOB, sub, parser_state+1, parser_state+1 );
         parser_state++;
         t1 = parser_state;
         bq = FALSE;
      }
      t2 = expression( );
      if (regx.pattern[lookahead] == ')') {
         if (bq) {
            if (!Kleene_star( regx.pattern[lookahead+1] )) {
               /*
                * close parens seem to need a JUXTA node to gather all reg ex's
                *  to a common point.
                */
               emit_cnode(parser_state, JUXTA, parser_state+1, parser_state+1);
               parser_state++;
            }
            if (bq > TRUE  &&  regx.pattern[lookahead+1] != '\0')
               /*
                * assertion must be last
                */
               regx_error( reg14 );
         }
         lookahead++;
      } else

         /*
          * unmatched open parens
          */
         regx_error( reg2 );
   } else if (letter( c )) {
      switch (c) {
         case ']' :

            /*
             * unmatched close bracket
             */
            regx_error( reg9 );
            break;
         case '.' :
            ttype = WILD;
            break;
         case ',' :
            ttype = BLANK;
            break;
         case '^' :
            ttype = BOL;
            break;
         case '$' :
            ttype = EOL;
            break;
         case '<' :
            ttype = BOW;
            if (regx.pattern[lookahead+1] == '<') {
               ++lookahead;
               ttype = BOS;
            }
            break;
         case '>' :
            ttype = EOW;
            if (regx.pattern[lookahead+1] == '>') {
               ++lookahead;
               ttype = EOS;
            }
            break;
         case '\\' :
            c = regx.pattern[++lookahead];
            ttype = mode.search_case == IGNORE ? IGNORE_ASCII : STRAIGHT_ASCII;
            if (c != '\0') {
               if (c >= '1' && c <= '9') {
                  c -= '0';
                  if (c >= subs_found)
                     regx_error( reg12 );
                  ttype = BACKREF;
               } else if (c != ':')
                  c = escape_char( c );

               /*
                * predefined unix-like macros.
                */
               else {
                  c = regx.pattern[++lookahead];
                  switch (bj_tolower( c )) {
                     case REG_ALPHANUM :   ttype = ALPHANUM;   break;
                     case REG_WHITESPACE : ttype = WHITESPACE; break;
                     case REG_ALPHA :      ttype = ALPHA;      break;
                     case REG_DECIMAL :    ttype = DECIMAL;    break;
                     case REG_HEX :        ttype = HEX;        break;
                     case REG_LOWER :      ttype = LOWER;      break;
                     case REG_UPPER :      ttype = UPPER;      break;
                     default :
                        c = ':';
                        break;
                  }
                  if (ttype != IGNORE_ASCII && ttype != STRAIGHT_ASCII)
                     c = !bj_islower( c );
               }
            } else
               regx_error( reg4 );
            break;

         case '[' :
            memset( class_bits, 0, 32 );
            c = regx.pattern[++lookahead];
            if (c != '\0') {
               if (c == '^') {
                  ++lookahead;
                  ttype = NOTCLASS;
               } else
                  ttype = CLASS;

               c1 = regx.pattern[lookahead];
               if (c1 != '\0')
               do {
                  c2 = 0;
                  if (c1 == '\\'  &&  regx.pattern[lookahead+1] != '\0') {
                     c1 = regx.pattern[++lookahead];
                     if (c1 != ':')
                        c1 = escape_char( c1 );
                     else {
                        c1 = regx.pattern[++lookahead];
                        switch (bj_tolower( c1 )) {
                           case REG_ALPHANUM :   c2 = BJ_alpha|BJ_digit; break;
                           case REG_WHITESPACE : c2 = BJ_space;   break;
                           case REG_ALPHA :      c2 = BJ_alpha;   break;
                           case REG_DECIMAL :    c2 = BJ_digit;   break;
                           case REG_HEX :        c2 = BJ_xdigit;  break;
                           case REG_LOWER :      c2 = BJ_lower;   break;
                           case REG_UPPER :      c2 = BJ_upper;   break;
                           default :
                              --lookahead;
                              break;
                        }
                        if (c2)
                           c1 = !bj_islower( c1 );
                     }
                  }
                  if (c2) {
                     for (c = 0; c <= 255; c++)
                        if (!(bj_ctype[c] & c2) == c1)
                           class_bits[c/8] |= bit[c%8];
                  } else {
                     c2 = c1;
                     if (regx.pattern[++lookahead] == '-') {
                        c2 = regx.pattern[++lookahead];
                        if (c2 != '\0') {
                           if (c2 == '\\' && regx.pattern[lookahead+1] != '\0')
                              c2 = escape_char( regx.pattern[++lookahead] );
                           ++lookahead;

                           /*
                            * just in case the hi for the range is given first,
                            *  switch c1 and c2,  e.g. [9-0].
                            */
                           if (c2 < c1) {
                              c  = c2;
                              c2 = c1;
                              c1 = c;
                           }

                        } else
                           regx_error( reg10 );
                     }
                     if (mode.search_case == IGNORE) {
                        for (; c1 <= c2; c1++) {
                           c = bj_tolower( c1 );
                           class_bits[c/8] |= bit[c%8];
                           c = bj_toupper( c1 );
                           class_bits[c/8] |= bit[c%8];
                        }
                     } else
                        for (c = c1; c <= c2; c++)
                           class_bits[c/8] |= bit[c%8];
                  }
                  c1 = regx.pattern[lookahead];
               } while (c1 != '\0'  &&  c1 != ']');

               if (c1 == '\0')
                  regx_error( reg5 );
            } else
               regx_error( reg6 );
            break;
         default :
            if (mode.search_case == IGNORE) {
               c = bj_tolower( c );
               ttype = IGNORE_ASCII;
            } else
               ttype = STRAIGHT_ASCII;
      }
      emit_nnode( parser_state, ttype, c, parser_state+1, parser_state+1 );
      if (ttype == CLASS  ||  ttype == NOTCLASS) {
         nfa.class[parser_state] = malloc( 32 );
         if (nfa.class[parser_state] != NULL)
            memcpy( nfa.class[parser_state], class_bits, 32 );
         else
            regx_error( reg7 );
      }
      t2 = parser_state;
      lookahead++;
      parser_state++;
      if (ttype >= BOL && ttype <= EOS && Kleene_star(regx.pattern[lookahead]))
         regx_error( reg8 );
   } else if (c == '\0')
      return( 0 );
   else {
      if (Kleene_star( c ))
         regx_error( reg8 );
      else if (c == ')')
         regx_error( reg3 );
      else
         regx_error( reg2 );
   }

   c = regx.pattern[lookahead];
   switch (c) {
      case '*' :
         emit_cnode( parser_state, CLOSURE, parser_state+1, t2 );
         r = parser_state;
         if (nfa.node_type[t1] == CNODE)
            t1 = min( nfa.next1[t1], nfa.next2[t1] );
         nfa.next1[t1-1] = parser_state;
         if (nfa.node_type[t1-1] == NNODE)
            nfa.next2[t1-1] = parser_state;
         parser_state++;
         lookahead++;
         break;

      case '+' :
         emit_cnode( parser_state, CLOSURE, parser_state+1, t2 );
         r = t2;
         parser_state++;
         lookahead++;
         break;

      case '?' :
         emit_cnode( parser_state, JUXTA, parser_state+2, parser_state+2 );
         parser_state++;
         r = parser_state;
         emit_cnode( parser_state, ZERO_OR_ONE, parser_state+1, t2 );
         if (nfa.node_type[t1] == CNODE)
            t1 = min( nfa.next1[t1], nfa.next2[t1] );
         nfa.next1[t1-1] = parser_state;
         if (nfa.node_type[t1-1] == NNODE)
            nfa.next2[t1-1] = parser_state;
         parser_state++;
         lookahead++;
         break;

      default  :
         r = t2;
         break;
   }
   if (regx.pattern[lookahead] == '?') {
      /*
       * swap the previous node's next1 and next2 to implement non-greedy
       */
      nfa.next2[parser_state-1] = nfa.next1[parser_state-1];
      nfa.next1[parser_state-1] = t2;
      lookahead++;
   }

   if (!bq) {
      emit_nnode( parser_state, EOB, sub, parser_state+1, parser_state+1 );
      parser_state++;
   }

   return( r );
}


/*
 * Name:    escape_char
 * Purpose: recognize escape and C escape sequences
 * Date:    June 5, 1993
 * Passed:  let:  letter to escape
 * Returns: escaped letter
 *
 * jmh 991006: added \e for ESC, \f for form-feed and \v for vertical tab
 */
int  escape_char( int let )
{
   switch (let) {
      case '0' : let = 0x00; break;
      case 'a' : let = 0x07; break;
      case 'b' : let = 0x08; break;
      case 'e' : let = 0x1b; break;
      case 'f' : let = 0x0c; break;
      case 'n' : let = 0x0a; break;
      case 'r' : let = 0x0d; break;
      case 't' : let = 0x09; break;
      case 'v' : let = 0x0b; break;
   }
   return( let );
}


/*
 * Name:    emit_cnode
 * Purpose: add a null node to our pattern matching machine
 * Date:    June 5, 1993
 * Passed:  index:  current node in nfa
 *          ttype:  terminal type - CLOSURE, OR, JUXTA, etc...
 *          n1:     pointer to next state, path for lambda transitions
 *          n2:     pointer to other next state, usually a NNODE
 * Returns: none, but modifies local global nfa.
 */
void emit_cnode( int index, int ttype, int n1, int n2 )
{
   assert( index >= 0);
   assert( index < REGX_SIZE );

   nfa.node_type[index] = CNODE;
   nfa.term_type[index] = ttype;
   nfa.c[index] = 0;
   nfa.next1[index] = n1;
   nfa.next2[index] = n2;
}


/*
 * Name:    emit_nnode
 * Purpose: add a to our pattern matching machine
 * Date:    June 5, 1993
 * Passed:  index:  current node in nfa
 *          ttype:  terminal type - EOL, ASCII, etc...
 *          c:      letter this node recognizes
 *          n1:     pointer to next state
 *          n2:     pointer to other next state, which can be same as n1
 * Returns: none, but modifies local global nfa.
 */
void emit_nnode( int index, int ttype, int c, int n1, int n2 )
{
   assert( index >= 0);
   assert( index < REGX_SIZE );

   nfa.node_type[index] = NNODE;
   nfa.term_type[index] = ttype;
   nfa.c[index] = c;
   nfa.next1[index] = n1;
   nfa.next2[index] = n2;
}


/*
 * Name:    init_nfa
 * Purpose: set local global nfa to NULL state
 * Date:    June 5, 1993
 * Passed:  none
 */
void init_nfa( void )
{
int  i;

   for (i = 0; i < REGX_SIZE; i++) {
      nfa.node_type[i] = NNODE;
      nfa.term_type[i] = 0;
      nfa.c[i] = 0;
      nfa.next1[i] = 0;
      nfa.next2[i] = 0;
      if (nfa.class[i] != NULL)
         free( nfa.class[i] );
      nfa.class[i] = NULL;
   }
}


/*
 * Name:    regx_error
 * Purpose: display reg ex error message and set reg ex error code
 * Date:    June 5, 1993
 * Passed:  line:  line to display error
 * Returns: none, but sets reg ex return code to error.
 *
 * jmh 010528: If regx_error_line is zero, assume this is called due to the
 *              the command line grep and set g_status.errmsg, instead.
 * jmh 031130: remove regx_error_line, use g_status.current_window instead.
 */
void regx_error( const char *line )
{
   if (g_status.current_window == NULL)
      g_status.errmsg = line;
   else
      error( WARNING, (g_status.command == RegXForward ||
                       g_status.command == RegXBackward)
                       ? g_status.current_window->bottom_line
                       : g_display.end_line, line );
   regx_rc = ERROR;
}


/*
 * Name:    separator
 * Purpose: determine if character is a reg ex separator
 * Date:    June 5, 1993
 * Passed:  let:  letter to look at
 * Returns: whether or not 'let' is a separator
 */
int  separator( int let )
{
   return( let == '\0'  ||  let == ')'  ||  let == '|' );
}


/*
 * Name:    Kleene_star
 * Purpose: determine if character is a reg ex operator
 * Date:    June 5, 1993
 * Passed:  let:  letter to look at
 * Returns: whether or not 'let' is a letter
 */
int  Kleene_star( int let )
{
   return( let == '*'  ||  let == '+'  ||  let == '?' );
}


/*
 * Name:    letter
 * Purpose: determine if character is a recognized reg ex character
 * Date:    June 5, 1993
 * Passed:  let:  letter to look at
 * Returns: whether or not 'let' is a letter.
 */
int  letter( int let )
{
   return( !separator( let )  &&  !Kleene_star( let ) );
}
