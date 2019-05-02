/*******************  start of original comments  ********************/
/*
 * Written by Douglas Thomson (1989/1990)
 *
 * This source code is released into the public domain.
 */

/*
 * Name:    dte - Doug's Text Editor program - find/replace module
 * Purpose: This file contains the functions relating to finding text
 *           and replacing text.
 *          It also contains the code for moving the cursor to various
 *           other positions in the file.
 * File:    findrep.c
 * Author:  Douglas Thomson
 * System:  this file is intended to be system-independent
 * Date:    October 1, 1989
 */
/*********************  end of original comments   ********************/

/*
 * The search and replace routines have been EXTENSIVELY rewritten.  The
 * "brute force" search algorithm has been replaced by the Boyer-Moore
 * string search algorithm.  This search algorithm really speeds up search
 * and replace operations.
 *
 *                  Sketch of the BM pattern matching algorithm:
 *
 *    while (not found) {
 *       fast skip loop;    <===  does most of the work and should be FAST
 *       slow match loop;   <===  standard "brute force" string compare
 *       if (found) then
 *          break;
 *       shift function;    <===  mismatch, shift and go to fast skip loop
 *    }
 *
 * See:
 *
 *   Robert S. Boyer and J Strother Moore, "A fast string searching
 *     algorithm."  _Communications of the ACM_ 20 (No. 10): 762-772, 1977.
 *
 * See also:
 *
 *   Zvi Galil, "On Improving the Worst Case Running Time of the Boyer-
 *    Moore String Matching Algorithm."  _Communications of the ACM_
 *    22 (No. 9): 505-508, 1979.
 *
 *   R. Nigel Horspool, "Practical fast searching in strings." _Software-
 *    Practice and Experience_  10 (No. 3): 501-506, 1980.
 *
 *   Alberto Apostolico and Raffaele Giancarlo, "The Boyer-Moore-Galil
 *    String Searching Strategies Revisited."  _SIAM Journal on Computing_
 *    15 (No. 1): 98-105, 1986.
 *
 *   Andrew Hume and Daniel Sunday, "Fast String Searching."  _Software-
 *    Practice and Experience_ 21 (No. 11): 1221-1248, November 1991.
 *
 *   Timo Raita, "Tuning the Boyer-Moore-Horspool String Searching Algorithm."
 *    _Software-Practice and Experience_ 22 (No. 10): 879-884, October 1992.
 *
 *
 *                            Boyer-Moore in TDE
 *
 * Hume and Sunday demonstrated in their paper that using a simple shift
 * function after a mismatch gives one of the fastest search times with the
 * Boyer-Moore algorithm.  When searching normal text for small patterns
 * (patterns less than 30 characters or so), Hume and Sunday found that the
 * faster the algorithm can get back into the fast skip loop with the
 * largest shift value then the faster the search.  Some algorithms can
 * generate a large shift value, but they can't get back into the skip loop
 * very fast.  Hume and Sunday give a simple method for computing a shift
 * constant that, more often than not, is equal to the length of the search
 * pattern.  They refer to the constant as mini-Sunday delta2 or md2.  From
 * the end of the string, md2 is the first leftmost occurrence of the
 * rightmost character.  Hume and Sunday also found that using a simple string
 * compare as the match function gave better performance than using the C
 * library memcmp( ) function.  The average number of compares in the slow
 * loop was slightly above 1.  Calling the memcmp( ) function to compare 1
 * character slows down the algorithm.  See the Hume and Sunday paper for an
 * excellent discussion on fast string searching.  Incidentally, Hume and
 * Sunday describe an even faster, tuned Boyer-Moore search function.
 *
 * The Boyer-Moore search algorithm in TDE was rearranged so that it is now
 * almost identical to the original version Boyer-Moore described in CACM.
 * The simple shift function in TDE was replaced by the mini-delta2 shift
 * function in the Hume and Sunday paper.
 *
 * I am not very fond of WordStar/TURBO x style search and replace prompting,
 * which is used in the DTE 5.1 editor.  Once the search pattern has been
 * defined, one only needs to press a key to search forwards or backwards.
 * The forward or backward search key may be pressed at any time in any file
 * once the pattern has been entered.  Also, the search case may be toggled
 * at any time in any file once the pattern has been entered.
 *
 * Thanks to Tom Waters, twaters@relay.nswc.navy.mil, for finding a bug when
 * building the ignore skip index array in TDE 1.3 and previous versions.
 * BTW, I added assertions to those functions that build the skip index
 * array to make certain that everything is everything.
 *
 * Thanks also to David Merrill, u09606@uicvm.uic.edu, for recommending a
 * change and the source code for the solution to what can be an annoying
 * feature in the find function.  Pressing <FindForward> before defining
 * the pattern would cause TDE to display an error message.  Since we already
 * know that the search pattern is undefined, we can easily prompt for
 * the search pattern.  I usually tolerate such features until I get tired
 * of being annoyed with error messages and finally write a solution.
 * Dave, thanks for the solution and the easy-to-implement code.  Although
 * such changes are small, they really make for a more comfortable editor.
 *
 *
 *                         Incremental Searching
 *
 * Incremental (or interactive) searching finds the search string as you type
 * it in. TDE's isearch is based on Emacs (as described to me by David J
 * Hughes). The default keys are Shift+Ctrl+S for forward searching and
 * Shift+Ctrl+R for backward; however, within isearch, Ctrl+S and Ctrl+R can
 * be used. Pressing these keys again will immediately start a search with the
 * previous find string (not necessarily isearch); otherwise they will
 * continue the search with the current string. LineUp and LineDown will cycle
 * through the find history. BackSpace can be used to remove entered
 * characters, returning to the previous position. CopyWord (ie. NextLine),
 * WordRight, WordEndRight and Ctrl+W will append the remainder of the word to
 * the search string; likewise with the String functions and Shift+Ctrl+W. If
 * no search string has been entered, the complete word or string will be
 * entered. ToggleSearchCase will do the obvious. Escape will stop the search
 * at the current position; AbortCommand and Ctrl+G will restore the original
 * position before the search was started. Enter/Rturn will stop the search
 * when entering characters or using the ISearch functions to continue it;
 * otherwise the search will be continued. The previous position is set to the
 * original position, not to the last matching instance; if aborted, it is set
 * to the aborted position.
 *
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
 * This modification of Douglas Thomson's code is released into the
 * public domain, Frank Davis.   You may distribute it freely.
 */

#include "tdestr.h"
#include "common.h"
#include "tdefunc.h"
#include "define.h"


/*
 * jmh 991009: when text is replaced in a forward BOX replace, need to
 *             adjust the end of the block to reflect the change.
 */
       int  block_replace_offset = 0;
       long last_replace_line = 0;

extern int  nfa_status;                 /* regx.c */

       int  search_type = ERROR;        /* the search being repeated */
static file_infos *first_file;          /* first file when searching all */
static TDE_WIN    *first_win;           /* first win when searching all */

       TDE_WIN    *results_window = NULL;/* search results window,   */
       file_infos *results_file;         /*  file                    */
       file_infos *search_file;          /*  and file being searched */

       long found_rline;                /* for highlighting */
       int  found_rcol;
       int  found_len;
       int  found_vlen;                 /* virtual length, for tabs */

static text_ptr subst_text;             /* for regx substitution */
static int  subst_len;
static int  subst_max;
extern int  subs_pos[10];
extern int  subs_len[10];


#define CopyWord   NextLine
#define CopyString BegNextLine

static int  icopy_word( TDE_WIN *, int, int );

int  add_search_line( line_list_ptr, long, int );


/*
 * Name:     ask_replace
 * Purpose:  Ask user if he wants to (r)eplace, (s)kip, or (e)xit.
 * Date:     June 5, 1991
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen
 * Returns:  TRUE if user wants to replace, ERROR otherwise.
 * Passed:   window:   pointer to current window
 *           finished: TRUE if user pressed ESC or (e)xit.
 *
 * jmh 980813: center prompt column above cursor, rather than screen.
 */
int  ask_replace( TDE_WIN *window, int *finished )
{
int  prompt_line;
int  c;
int  rc;
DISPLAY_BUFF;

   prompt_line = window->cline - 1;
   rc = strlen( find2 );
   c  = window->ccol - (rc >> 1);
   if (c < 0)
      c = 0;
   else if (c > g_display.ncols - rc)
      c = g_display.ncols - rc;

   SAVE_LINE( prompt_line );
   /*
    * (R)eplace  (S)kip  (E)xit - default replace
    */
   s_output( find2, prompt_line, c, Color( Message ) );

#if defined( __UNIX__ )
   xygoto( window->ccol, window->cline );
   refresh( );
#endif

   rc = get_response( NULL, 0, R_DEFAULT | R_ABORT,
                      3, L_REPLACE, OK, L_SKIP, ERROR, L_EXIT, L_EXIT );
   RESTORE_LINE( prompt_line );
   if (rc == L_EXIT) {
      *finished = TRUE;
      rc = ERROR;
   }
   return( rc );
}


/*
 * Name:     ask_wrap_replace
 * Purpose:  After a wrap, ask user if he wants to (q)uit or (c)ontinue.
 * Date:     June 5, 1991
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen
 * Returns:  TRUE if user wants to continue, ERROR otherwise.
 * Passed:   window:   pointer to current window
 *           finished: ERROR if user pressed ESC or (q)uit.
 *
 * jmh 991026: allow response from a macro.
 * jmh 050709: check macro flag.
 */
int  ask_wrap_replace( TDE_WIN *window, int *finished )
{
int  rc = -2;

   if (g_status.search & SEARCH_ALL)
      window = first_win;

   if (g_status.macro_executing) {
      if (g_status.current_macro->flag & WRAP)
         rc = OK;
      else if (g_status.current_macro->flag & NOWRAP)
         rc = ERROR;
   }
   if (rc == -2) {
      /*
       * search has wrapped. continue or quit? - default continue
       */
      rc = get_response( find3, window->bottom_line, R_NOMACRO,
                         2, L_CONTINUE, OK, L_QUIT, ERROR );
   }
   if (rc == ERROR)
      *finished = ERROR;
   return( rc );
}


/*
 * Name:    subst_add
 * Purpose: add a string to the regx substitution
 * Author:  Jason Hood
 * Date:    July 29, 2005
 * Passed:  line:  character(s) to add
 *          len:   length of above
 * Returns: TRUE if string added, FALSE if no memory
 */
int  subst_add( text_ptr line, int len )
{
int  new_len;
text_ptr temp;
int  add;
int  rc = OK;

   new_len = subst_len + len;
   if (new_len > subst_max) {
      temp = my_realloc( subst_text, new_len + (add = 40), &rc );
      if (rc == ERROR) {
         rc = OK;
         temp = my_realloc( subst_text, new_len, &rc );
         if (rc == ERROR)
            return FALSE;
         add = 0;
      }
      subst_text = temp;
      subst_max = new_len + add;
   }
   memcpy( subst_text + subst_len, line, len );
   subst_len = new_len;
   return TRUE;
}


/*
 * Name:    do_replace
 * Purpose: To replace text once match has been found.
 * Date:    June 5, 1991
 * Passed:  window:     pointer to current window
 *          direction:  direction of search
 *
 * jmh 051018: don't deflate tabs (long-standing bug - inflate mode could
 *              never replace a tab)
 */
void do_replace( TDE_WIN *window, int direction )
{
int  old_len;           /* length of original text */
int  new_len;           /* length of replacement text */
int  rcol;
register int net_change;
text_ptr source;        /* source of block move */
text_ptr dest;          /* destination of block move */
file_infos *file;
int  tabs;
int  i;
int  rc;

   file = window->file_info;
   tabs = file->inflate_tabs;
   rcol = (tabs) ? entab_adjust_rcol( window->ll->line, window->ll->len,
                                      window->rcol, file->ptab_size )
                 : window->rcol;
   g_status.copied = FALSE;
   copy_line( window->ll, window, FALSE );

   old_len = found_len;
   new_len = strlen( (char *)g_status.subst );
   if (g_status.search & SEARCH_REGX) {
      subst_len = 0;
      for (i = 0; i < new_len; ++i) {
         if (g_status.subst[i] != '\\'  ||  i == new_len-1)
            rc = subst_add( g_status.subst + i, 1 );
         else {
            text_t c = g_status.subst[i+1];
            if ((c == '+' || c == '-' || c == '^')  &&  i+2 < new_len)
               c = g_status.subst[i+2];
            if (c == '&'  ||  (c >= '1' && c <= '9')) {
               int s = (c == '&') ? 0 : c - '0';
               rc = TRUE;
               if (subs_len[s]) {
                  net_change = subst_len;
                  rc = subst_add( g_status.line_buff + rcol + subs_pos[s],
                                  subs_len[s] );
                  if (rc) {
                     source = subst_text + net_change;
                     switch (g_status.subst[i+1]) {
                        case '+': upper_case( source, subs_len[s] ); ++i; break;
                        case '-': lower_case( source, subs_len[s] ); ++i; break;
                        case '^': capitalise( source, subs_len[s] ); ++i; break;
                     }
                  }
               }
            } else {
               c = escape_char( g_status.subst[i+1] );
               rc = subst_add( &c, 1 );
            }
            ++i;
         }
         if (!rc) {
            /*
             * out of memory
             */
            error( WARNING, window->bottom_line, main4 );
            g_status.copied = FALSE;
            return;
         }
      }
      new_len = subst_len;
   }
   net_change = new_len - old_len;

   /*
    * move the text to either make room for the extra replacement text
    *  or to close up the gap left
    */

   if (net_change != 0) {
      source = g_status.line_buff + rcol + old_len;
      dest  = source + net_change;

      assert( g_status.line_buff_len - rcol - old_len >= 0 );
      assert( g_status.line_buff_len - rcol - old_len < MAX_LINE_LENGTH );

      shift_tabbed_block( file );

      memmove( dest, source, g_status.line_buff_len - rcol - old_len );
      g_status.line_buff_len += net_change;

      shift_block( file, window->rline, rcol + old_len, net_change );

      if ((g_status.search & SEARCH_BLOCK) && direction == FORWARD  &&
          file->block_type == BOX && file->block_br != file->block_er) {
         block_replace_offset = net_change;
         last_replace_line = window->rline;
      }
   }

   /*
    * insert the replacement text
    */

   assert( rcol >= 0 );
   assert( rcol < MAX_LINE_LENGTH );
   assert( g_status.line_buff_len >= 0 );
   assert( g_status.line_buff_len >= rcol );

   memcpy( g_status.line_buff + rcol,
           (g_status.search & SEARCH_REGX) ? subst_text : g_status.subst,
           new_len );

   file->modified = TRUE;
   un_copy_line( window->ll, window, FALSE, FALSE );
   window->ll->type |= DIRTY;

   if (direction == FORWARD) {
      if (tabs)
         window->rcol = detab_adjust_rcol( window->ll->line, rcol + new_len - 1,
                                           file->ptab_size );
      else
         window->rcol += new_len - 1;
      if ((g_status.search & SEARCH_REGX)  &&
          (found_len == 0 || *regx.pattern == '$' || *regx.pattern == '>'))
         ++window->rcol;
   }

   /*
    * jmh (22/11/97) - rcol can become -1, which appears to be safe.
    */
   assert( window->rcol >= -1 );
   show_avail_mem( );
}


/*
 * Name:    find_string
 * Purpose: To set up and perform a find operation.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   Keep the search string and boyer-moore stuff until changed.
 */
int  find_string( TDE_WIN *window )
{
int  direction;
int  new_string;
register TDE_WIN *win;  /* put window pointer in a register */
int  rc;
char answer[MAX_COLS+2];

   switch (g_status.command) {
      case FindForward :
         direction  = FORWARD;
         new_string = TRUE;
         break;
      case FindBackward :
         direction  = BACKWARD;
         new_string = TRUE;
         break;
      case RepeatFindForward :
         direction  = FORWARD;
         new_string =  bm.search_defined != OK ? TRUE : FALSE;
         break;
      case RepeatFindBackward :
         direction  = BACKWARD;
         new_string =  bm.search_defined != OK ? TRUE : FALSE;
         break;
      default :
         direction  = 0;
         new_string = 0;
         assert( FALSE );
         return( ERROR );
   }
   win = window;
   if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
      return( ERROR );

   /*
    * get search text, using previous as default
    */
   if (new_string == TRUE) {
      strcpy( answer, (char *)bm.pattern );

      /*
       * string to find:
       */
      if (get_name( find4, win->bottom_line, answer, &h_find ) <= 0)
         return( ERROR );
      bm.search_defined = OK;
      strcpy( (char *)bm.pattern, answer );
      build_boyer_array( );
   }
   g_status.search = (direction == BACKWARD) ? SEARCH_BACK : 0;
   rc = perform_search( win );
   return( rc );
}


/*
 * Name:    define_search
 * Purpose: to setup and perform a search
 * Author:  Jason Hood
 * Date:    September 23, 1999
 * Passed:  window:  pointer to current window
 * Notes:   prompt for direction and start position.  If the string contains
 *           regx characters perform a regx search, otherwise a text search.
 *           Start the string with a quote (") to force text search.
 *          If searching block, make sure it's in the current file.
 */
int  define_search( TDE_WIN *window )
{
int  type;
register TDE_WIN *win;  /* put window pointer in a register */
int  block;
int  rc;
char *answer;

   win = window;
   if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
      return( ERROR );

   check_block( NULL );
   if (g_status.marked && g_status.marked_file == win->file_info)
      block = TRUE;
   else
      block = CB_S_Block = FALSE;
   find_dialog[IDC_S_BLOCK].hist = (block) ? NULL : (HISTORY*)ERROR;

   rc = do_dialog( find_dialog, find_proc );
   if (rc == ERROR)
      return( ERROR );

   answer = get_dlg_text( EF_S_Pattern );
   if (*answer == '\0')
      return( ERROR );

   if (CB_S_RegX) {
      /* pattern is copied and successfully compiled in find_proc */
      regx.search_defined = OK;
      type = SEARCH_REGX;
   } else {
      bm.search_defined = OK;
      strcpy( (char *)bm.pattern, answer );
      build_boyer_array( );
      type = 0;
   }
   if (CB_S_Backward) type |= SEARCH_BACK;
   if (CB_S_Begin)    type |= SEARCH_BEGIN;
   if (CB_S_Block)    type |= SEARCH_BLOCK;
   if (CB_S_All)      type |= SEARCH_ALL;
   if (CB_S_Results) {
      type |= SEARCH_RESULTS;
      results_window = NULL;
   }

   g_status.search = search_type = type;
   rc = perform_search( window );
   if (!(search_type & SEARCH_ALL) || !(search_type & SEARCH_RESULTS))
      search_type &= ~SEARCH_BEGIN;

   g_status.search = 0;
   return( rc );
}


/*
 * Name:    find_proc
 * Purpose: dialog callback for DefineSearch
 * Author:  Jason Hood
 * Date:    November 30, 2003
 * Notes:   if using RegX, check the pattern compiles successfully;
 *          since Block and All searches are complementary, turning on one
 *           will turn off the other;
 *          results for all files will always be from the beginning, so turn
 *           Beginning on and disable it.
 */
int  find_proc( int id, char* text )
{
static int  begin = ERROR;      /* remember the old Beginning state */
int  other;
int  rc = OK;

   if (id == IDE_S_PATTERN) {
      if (CB_S_RegX && *text != '\0') {
         strcpy( (char *)regx.pattern, text );
         if (build_nfa( ) == ERROR) {
            regx.search_defined = FALSE;
            rc = ERROR;
         }
      }
   } else {
      if (id == IDC_S_BLOCK || id == IDC_S_ALL) {
         other = IDC_S_BLOCK + IDC_S_ALL - id;
         if (find_dialog[id].n && find_dialog[other].n)
            check_box( other );
      }
      if (id == IDC_S_RESULTS || id == IDC_S_ALL) {
         other = IDC_S_RESULTS + IDC_S_ALL - id;
         if (find_dialog[id].n) {
            if (find_dialog[other].n) {
               begin = CB_S_Begin;
               if (!begin)
                  check_box( IDC_S_BEGIN );
               check_box_enabled( IDC_S_BEGIN, FALSE );
            }
         } else if (find_dialog[other].n && begin != ERROR) {
            check_box_enabled( IDC_S_BEGIN, TRUE );
            if (!begin)
               check_box( IDC_S_BEGIN );
            begin = ERROR;
         }
      }
   }

   return( rc );
}


/*
 * Name:    repeat_search
 * Purpose: to repeat the search defined above
 * Author:  Jason Hood
 * Date:    September 23, 1999
 * Passed:  window:  pointer to current window
 * Notes:   uses the other search functions to do all the work.
 *          if repeating results across all files use the dialog to verify.
 */
int  repeat_search( TDE_WIN *window )
{
line_list_ptr temp;
int  rc;

   if (/*search_type == ERROR  ||*/ /* (ERROR is -1 so both bits are defined) */
       ((search_type & SEARCH_ALL) && (search_type & SEARCH_RESULTS)))
      return( define_search( window ) );

   if (un_copy_line( window->ll, window, TRUE, TRUE ) == ERROR)
      return( ERROR );

   /*
    * if repeating a search with a results window, add a blank
    *  line before the new results and move the cursor there
    */
   if ((search_type & SEARCH_RESULTS) && results_window != NULL) {
      if (results_window == window) {
         /*
          * results window cannot append to itself
          */
         error( WARNING, window->bottom_line, find17 );
         return( ERROR );
      }
      rc = OK;
      temp = new_line( 0, &rc );
      if (rc == OK) {
         insert_node( results_file, results_file->line_list_end, temp );
         if (results_window->rline == results_file->length)
            ++results_window->rline;
         else
            move_to_line( results_window, results_file->length + 1, TRUE );
         check_cline( results_window, (results_window->bottom_line + 1
                                       + results_window->top_line) / 2 );
      }
   }

   g_status.search = search_type;
   rc = perform_search( window );

   g_status.search = 0;
   return( rc );
}


/*
 * Name:    isearch
 * Purpose: perform an incremental/interactive search
 * Author:  Jason Hood
 * Date:    October 28, 2002
 * Passed:  window:  pointer to current window
 * Notes:   search for text as characters are entered.
 */
int  isearch( TDE_WIN *window )
{
register TDE_WIN *win;
long key;
int  func = 0;
int  len = 0;
MARKER *prev, temp, cur;
MARKER pos[MAX_COLS];
int  fcol[MAX_COLS];
int  vlen[MAX_COLS];
HISTORY *h;
int  num = 0;
int  search, new_string = FALSE, display;
int  first = TRUE;
int  stop = FALSE;
int  rc = OK;
DISPLAY_BUFF;
int  scase = mode.search_case;
int  max_len = 66 - ISEARCH;    /* the position of the "wrapped" message */
unsigned char ch;

   win = window;
   if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
      return( ERROR );

   prev = &win->file_info->marker[0];
   temp = *prev;
   set_marker( win );
   cur = *prev;
   *prev = temp;
   SAVE_LINE( g_display.mode_line );
   s_output( mode.search_case == IGNORE ? find18a : find18b,
             g_display.mode_line, 0, Color( Help ) );

   g_status.search = (g_status.command == ISearchBackward)
                     ? (SEARCH_I | SEARCH_BACK) : SEARCH_I;
   bm.search_defined = OK;
   h = h_find.prev;
   new_string = TRUE;

   while (!stop) {
      search = display = FALSE;
      if (new_string) {
         num = 0;
         if (new_string == TRUE)
            strcpy( (char *)bm.pattern, h->str );
         build_boyer_array( );
         len = bm.pattern_length;
         eol_clear( ISEARCH, g_display.mode_line, Color( Help ) );
         if (len > max_len) {
            ch = bm.pattern[max_len];
            bm.pattern[max_len] = '\0';
            s_output( (char *)bm.pattern, g_display.mode_line, ISEARCH,
                      Color( Help ) );
            bm.pattern[max_len] = ch;
         } else
            s_output( (char *)bm.pattern, g_display.mode_line, ISEARCH,
                      Color( Help ) );
         refresh( );
      }
      key = getkey_macro( );
      if (g_status.wrapped) {
         g_status.wrapped = FALSE;
         eol_clear( (len > max_len) ? 67 : ISEARCH + len,
                    g_display.mode_line, Color( Help ) );
      }
      if (key < 256  &&  rc != ERROR) {
         if (first) {
            len = 0;
            eol_clear( ISEARCH, g_display.mode_line, Color( Help ) );
         }
         if (len < MAX_COLS - 1) {
            bm.pattern[len++] = (text_t)key;
            bm.pattern[len]   = '\0';
            build_boyer_array( );
            if (len > max_len)
               s_output( (char *)bm.pattern + len - max_len,
                         g_display.mode_line, ISEARCH, Color( Help ) );
            else
               c_output( (int)key, ISEARCH + len - 1, g_display.mode_line,
                         Color( Help ) );
            refresh( );
            search = TRUE + 1;
            new_string = FALSE;
            g_status.search |= SEARCH_BEGIN;
         }
      } else {
         switch (key) {
            case RTURN: case ESC: func = Rturn;           break;
            case _CTRL+_G:        func = AbortCommand;    break;
            case _CTRL+_S:        func = ISearchForward;  break;
            case _CTRL+_R:        func = ISearchBackward; break;
            case _CTRL+_W:        func = CopyWord;        break;
            case _SHIFT+_CTRL+_W: func = CopyString;      break;
            default:              func = getfunc( key );  break;
         }
         switch (func) {
            case AbortCommand:
               *prev = cur;
               goto_marker( win );
               rc = ERROR;
               stop = TRUE;
               break;

            case Rturn:
               if (key != ESC  &&  new_string  &&  len)
                  search = TRUE;
               else {
                  *prev = cur;
                  rc = OK;
                  stop = TRUE;
               }
               break;

            case ToggleSearchCase:
               mode.search_case = (mode.search_case == IGNORE) ? MATCH : IGNORE;
               build_boyer_array( );
               s_output( mode.search_case == IGNORE ? find18a : find18b,
                         g_display.mode_line, 0, Color( Help ) );
               refresh( );
               break;

            case ISearchForward:
               g_status.search = SEARCH_I;
               goto history;

            case ISearchBackward:
               g_status.search = SEARCH_I | SEARCH_BACK;

            history:
               if (len) {
                  search = TRUE;
                  new_string = FALSE;
               }
               break;

            case LineUp:
            case LineDown:
               h = (func == LineDown) ? h->next : h->prev;
               new_string = TRUE;
               break;

            case WordRight:
            case WordEndRight:
               func = CopyWord;
               goto copy;

            case StringRight:
            case StringEndRight:
               func = CopyString;

            case CopyWord:
            case CopyString:
            copy:
               len = icopy_word( win, (first) ? 0 : len, (func == CopyString) );
               new_string = TRUE + 1;
               display = TRUE;
               break;

            case BackSpace:
               if (len) {
                  if (num) {
                     if (rc == OK) {
                        temp = *prev;
                        *prev = pos[--num];
                        found_rline = prev->rline;
                        found_rcol  = fcol[num];
                        found_vlen  = vlen[num];
                        goto_marker( win );
                        *prev = temp;
                     } else
                        found_rline = win->rline;
                     display = TRUE;
                  }
                  rc = OK;
                  bm.pattern[--len] = '\0';
                  build_boyer_array( );
                  if (len > max_len)
                     s_output( (char *)bm.pattern + len - max_len,
                               g_display.mode_line, ISEARCH, Color( Help ) );
                  else
                     c_output( ' ', ISEARCH + len, g_display.mode_line,
                               Color( Help ) );
                  refresh( );
               }
               break;
         }
      }
      if (search) {
         found_rline = -1;
         fcol[num] = found_rcol;
         vlen[num] = found_vlen;
         if ((rc = perform_search( win )) == OK) {
            display = TRUE;
            if (search == TRUE + 1)
               pos[num++] = *prev;
         } else {
            s_output( find18c, g_display.mode_line,
                      (len > max_len) ? 67 : ISEARCH + len + 1, Color( Help ) );
            search = FALSE;
         }
      }
      if (display) {
         if (win->file_info->dirty) {
            display_current_window( win );
            win->file_info->dirty = FALSE;
         } else
            show_curl_line( win );
         show_ruler( win );
         show_ruler_pointer( win );
         show_line_col( win );
         xygoto( win->ccol, win->cline );
         refresh( );
      }
      first = FALSE;
   }
   add_to_history( (char *)bm.pattern, &h_find );

   RESTORE_LINE( g_display.mode_line );
   if (mode.search_case != scase) {
      mode.search_case = scase;
      toggle_search_case( win );
   }
   g_status.search = 0;
   return( rc );
}


/*
 * Name:    icopy_word
 * Purpose: copy a word/string for use with isearch
 * Author:  Jason Hood
 * Date:    October 29, 2002
 * Passed:  window:  pointer to current window
 *          len:     length of current pattern
 *          str:     non-zero to copy a string
 * Returns: new length
 * Notes:   adds the remainder of the current word/string to the search
 *           pattern, or appends the next.
 *          if len is zero, copy the complete word/string.
 *
 * 031120:  fix bug when searching backwards (rcol at start, not end).
 */
static int  icopy_word( TDE_WIN *window, int len, int str )
{
text_ptr line = window->ll->line;
int  llen = window->ll->len;
int  rcol;
int  (*space)( int );   /* Function to determine what a word is */
int  begin;

   rcol = window->rcol;
   if (window->file_info->inflate_tabs)
      rcol = entab_adjust_rcol( line, llen, rcol,
                                window->file_info->ptab_size );
   if (g_status.search & SEARCH_BACK)
       rcol += len;
   if (rcol >= llen)
      return( len );

   space = (str) ? bj_isspc : myiswhitespc;

   if (len == 0) {
      /*
       * no search string, find the start of the (next) word
       */
      while (space( line[rcol] )  &&  ++rcol < llen) ;
      if (rcol == llen)
         return( len );
      begin = rcol;
      while (--begin >= 0  &&  !space( line[begin] )) ;
      ++begin;
   } else {
      /*
       * if we're already at the end of the word,
       * find the start of the next.
       */
      begin = rcol;
      while (space( line[rcol] )  &&  ++rcol < llen) ;
      if (rcol == llen)
         return( len );
   }
   while (++rcol < llen  &&  !space( line[rcol] )) ;

   llen = rcol - begin;
   if (len + llen >= MAX_COLS)
      llen = MAX_COLS - len - 1;
   my_memcpy( bm.pattern + len, line + begin, llen );
   len += llen;
   bm.pattern[len] = '\0';

   if (window->file_info->inflate_tabs)
      rcol = detab_adjust_rcol( line, rcol, window->file_info->ptab_size );
   check_virtual_col( window, rcol, window->ccol + llen );

   return( len );
}


/*
 * Name:    perform_search
 * Purpose: do the search
 * Author:  Jason Hood
 * Date:    October 6, 1999
 * Passed:  window:  pointer to current window
 * Returns: OK if found, ERROR if not.
 * Notes:   ripped out of find_string() and modified to work with regx.
 *
 * jmh 021029: have isearch leave the cursor at the end of the match.
 * jmh 021211: isearch forward will leave the cursor at the end, backward
 *              will leave it at the start; also allows backward/forward to
 *              move between start/end of the match.
 */
int  perform_search( TDE_WIN *window )
{
long found_line;
line_list_ptr ll;
register TDE_WIN *win;  /* put window pointer in a register */
int  rc;
int  old_rcol;
int  rcol;
int  dummy;
int  tabs = 0;

   rc = OK;
   win = window;
   if (((g_status.search & SEARCH_REGX) && regx.search_defined == OK) ||
       bm.search_defined == OK) {
      old_rcol = win->rcol;
      if (win->file_info->inflate_tabs) {
         tabs = win->file_info->ptab_size;
         win->rcol = entab_adjust_rcol( win->ll->line, win->ll->len, win->rcol,
                                        tabs );
      }
      if (g_status.search & SEARCH_I) {
         if (g_status.search & SEARCH_BACK) {
            if (g_status.search & SEARCH_BEGIN)
               win->rcol += bm.pattern_length;
         } else {
            if (g_status.search & SEARCH_BEGIN)
               win->rcol -= bm.pattern_length;
            else
               --win->rcol;
         }
         g_status.search &= ~SEARCH_BEGIN;
      }

      update_line( win );
      show_search_message( SEARCHING );

      /*
       * let search results include the current line
       */
      if (g_status.search & SEARCH_RESULTS) {
         if (win->rline == 1)
            g_status.search |= SEARCH_BEGIN;
         else
            win->rcol = (g_status.search & SEARCH_BACK) ? MAX_LINE_LENGTH : -1;
      }

      first_win = win;
      search_file = first_file = win->file_info;
      if (g_status.search & SEARCH_BACK)
         ll = backward_search( win, &found_line, &rcol );
      else
         ll = forward_search( win, &found_line, &rcol );

      win->rcol = old_rcol;
      if (ll != NULL) {
         if (g_status.wrapped && g_status.macro_executing)
            rc = ask_wrap_replace( win, &dummy );
         if (rc == OK) {
            if (search_file != win->file_info) {
               win = find_file_window( search_file );
               change_window( window, win );
               g_status.wrapped = ERROR;
            }
            undo_move( win, 0 );
            set_marker( win );          /* remember the previous position */
            if ((g_status.search & SEARCH_I) &&
                !(g_status.search & SEARCH_BACK))
               rcol += bm.pattern_length;
            find_adjust( win, ll, found_line, rcol );
         } else
            g_status.wrapped = FALSE;

      } else if (g_status.wrapped && g_status.macro_executing &&
                 (g_status.current_macro->flag & NOWRAP))
         g_status.wrapped = FALSE;

      if ((g_status.search & SEARCH_REGX) && nfa_status == ERROR)
         g_status.wrapped = TRUE;
      else
         show_search_message(
            (g_status.wrapped == ERROR) ? CHANGED :
            (g_status.wrapped && !(g_status.search & SEARCH_RESULTS))
            ? WRAPPED : CLR_SEARCH );

      if (ll == NULL) {
         if ((g_status.search & SEARCH_RESULTS) && results_window != NULL)
            change_window( g_status.current_window, results_window );
         else {
            /*
             * string not found
             */
            if (!(g_status.search & SEARCH_I)) {
               combine_strings( line_out, find5a,
                                (g_status.search & SEARCH_REGX)
                                ? (char*)regx.pattern : (char*)bm.pattern,
                                find5b );
               error( WARNING, win->bottom_line, line_out );
            }
            rc = ERROR;
         }
      }
      if (!win->file_info->dirty)
         show_curl_line( win );

   } else {
      /*
       * find pattern not defined
       */
      error( WARNING, win->bottom_line, find6 );
      rc = ERROR;
   }
   return( rc );
}


/*
 * Name:    forward_search
 * Purpose: search forward for pattern
 * Passed:  window:  pointer to current window
 *          rline:   pointer to real line counter
 *          rcol:    pointer to real column variable
 * Returns: position in file if found or NULL if not found
 * Date:    June 5, 1991
 * Notes:   Start searching from cursor position to end of file.  If we hit
 *          end of file without a match, start searching from the beginning
 *          of file to cursor position.  (do wrapped searches)
 *
 * jmh 991006: incorporated boyer_moore and regx into one function.
 * jmh 991007: added block search and beginning position. The block should
 *              be defined before entry.
 * jmh 031126: added multi-file search (ignore non-file files; check for
 *              search_file != window->file_info).
 */
line_list_ptr forward_search( TDE_WIN *window, long *rline, int *rcol )
{
int  end;
register TDE_WIN *win;  /* put window pointer in a register */
file_infos *file;
line_list_ptr ll;
line_list_ptr (*search_func)( line_list_ptr, long *, int * );

   win = window;
   file = win->file_info;

   *rcol = win->rcol + 1;       /* assume a continuing search */
   *rline = win->rline;
   ll = win->ll;
   end = 0;

   if (g_status.search & SEARCH_BLOCK) {
      if ((g_status.search & SEARCH_BEGIN) ||
          win->rline < file->block_br  ||  win->rline > file->block_er) {
         *rcol = 0;
         *rline = file->block_br;
         ll = file->block_start;
         g_status.search |= SEARCH_BEGIN;
      }
      end = file->block_end->next->len;
      file->block_end->next->len = EOF;
   } else {
      if (g_status.search & SEARCH_BEGIN) {
         *rcol = 0;
         *rline = 1L;
         ll = file->line_list->next;
      }
   }
   search_func = (g_status.search & SEARCH_REGX) ? regx_search_forward :
                                                   search_forward;
   ll = search_func( ll, rline, rcol );

   if (g_status.search & SEARCH_BLOCK)
      file->block_end->next->len = end;

   if (ll == NULL) {
      if (g_status.search & SEARCH_ALL) {
         do {
            search_file = search_file->next;
            if (search_file == NULL)
               search_file = g_status.file_list;
            if (search_file == first_file)
               break;
            if (search_file->file_name[0] == '\0')
               continue;
            *rline = 1L;
            *rcol  = 0;
            ll = search_func( search_file->line_list->next, rline, rcol );
         } while (ll == NULL);
      }

      if (ll == NULL && first_win->rline > 0 &&
          !(g_status.search & SEARCH_BEGIN)) {

         if (g_status.search & SEARCH_ALL) {
            win = first_win;
            file = first_file;
         }

         if (g_status.search & SEARCH_RESULTS) {
            end = win->ll->len;
            win->ll->len = EOF;

         } else if (win->ll->next != NULL) {
            end = win->ll->next->len;
            win->ll->next->len = EOF;
         }

         /*
          * haven't found pattern yet - now search from beginning of file
          */
         g_status.wrapped = TRUE;

         *rcol = 0;
         if (g_status.search & SEARCH_BLOCK) {
            *rline = file->block_br;
            ll = file->block_start;
         } else {
            *rline = 1L;
            ll = file->line_list->next;
         }
         ll = search_func( ll, rline, rcol );

         if (ll == win->ll  &&  *rcol >= win->rcol)
            ll = NULL;

         if (g_status.search & SEARCH_RESULTS)
            win->ll->len = end;
         else if (win->ll->next != NULL)
            win->ll->next->len = end;
      }
   }
   flush_keyboard( );
   g_status.search &= ~SEARCH_BEGIN;

   return( ll );
}


/*
 * Name:    backward_search
 * Purpose: search backward for pattern
 * Passed:  window:  pointer to current window
 *          rline:   pointer current real line counter
 *          rcol:    pointer to real column
 * Returns: position in file if found or NULL if not found
 * Date:    June 5, 1991
 * Notes:   Start searching from cursor position to beginning of file. If we
 *          hit beginning of file without a match, start searching from the
 *          end of file to cursor position.  (do wrapped searches)
 *
 * jmh 991006: incorporated boyer_moore and regx into one function.
 * jmh 991007: added block search and beginning position. The block should
 *              be defined before entry.
 * jmh 031126: added multi-file search (ignore non-file files; check for
 *              search_file != window->file_info).
 */
line_list_ptr backward_search( TDE_WIN *window, long *rline, int *rcol )
{
int  i;
int  len;
int  end;
register TDE_WIN *win;  /* put window pointer in a register */
file_infos *file;
line_list_ptr ll;
line_list_ptr (*search_func)( line_list_ptr, long *, int * );

   win  = window;
   file = win->file_info;
   ll   = NULL;
   end  = 0;
   if (g_status.search & SEARCH_BLOCK) {
      if ((g_status.search & SEARCH_BEGIN) ||
          win->rline < file->block_br  ||  win->rline > file->block_er) {
         *rline = file->block_er;
         ll = file->block_end;
         *rcol = ll->len - 1;
         g_status.search |= SEARCH_BEGIN;
      }
      end = file->block_start->prev->len;
      file->block_start->prev->len = EOF;
   } else {
      if (g_status.search & SEARCH_BEGIN) {
         ll = file->line_list_end->prev;
         *rcol = ll->len - 1;
         *rline = file->length;
      }
   }
   if (ll == NULL) {
      *rline = win->rline;
      ll = win->ll;

      /*
       * see if cursor is on EOF line.  if so, move search start to
       * previous node.
       */
      if (ll->next != NULL) {
         i  = win->rcol - 1;
         if (!(g_status.search & SEARCH_REGX))
            i += bm.pattern_length - 1;
         len = ll->len;
         if (i >= len)
            i = len - 1;
      } else {
         ll = ll->prev;
         --*rline;
         i = ll->len - 1;
      }
      *rcol = i;
   }
   search_func = (g_status.search & SEARCH_REGX) ? regx_search_backward :
                                                   search_backward;
   ll = search_func( ll, rline, rcol );

   if (g_status.search & SEARCH_BLOCK)
      file->block_start->prev->len = end;

   if (ll == NULL) {
      if (g_status.search & SEARCH_ALL) {
         do {
            search_file = search_file->prev;
            if (search_file == NULL) {
               search_file = g_status.file_list;
               while (search_file->next != NULL)
                  search_file = search_file->next;
            }
            if (search_file == first_file)
               break;
            if (search_file->file_name[0] == '\0')
               continue;
            ll = search_file->line_list_end->prev;
            *rline = search_file->length;
            *rcol  = ll->len - 1;
            ll = search_func( ll, rline, rcol );
         } while (ll == NULL);
      }

      if (ll == NULL  &&  first_win->rline <= first_file->length  &&
          !(g_status.search & SEARCH_BEGIN)) {

         if (g_status.search & SEARCH_ALL) {
            win = first_win;
            file = first_file;
         }

         if (g_status.search & SEARCH_RESULTS) {
            end = win->ll->len;
            win->ll->len = EOF;

         } else if (win->ll->prev != NULL) {
            end = win->ll->prev->len;
            win->ll->prev->len = EOF;
         }

         /*
          * haven't found pattern yet - now search from end of file
          */
         g_status.wrapped = TRUE;
         if (g_status.search & SEARCH_BLOCK) {
            ll = file->block_end;
            *rline = file->block_er;
         } else {
            ll = file->line_list_end->prev;
            *rline = file->length;
         }
         *rcol = ll->len - 1;
         ll = search_func( ll, rline, rcol );

         if (ll == win->ll  &&  *rcol <= win->rcol)
            ll = NULL;

         if (g_status.search & SEARCH_RESULTS)
            win->ll->len = end;
         else if (win->ll->prev != NULL)
            win->ll->prev->len = end;
      }
   }
   flush_keyboard( );
   g_status.search &= ~SEARCH_BEGIN;

   return( ll );
}


/*
 * Name:    build_boyer_array
 * Purpose: To set up the boyer array for forward and backward searches.
 * Date:    June 5, 1991
 * Notes:   Set up one array for forward searches and another for backward
 *          searches.  If user decides to ignore case then fill in array
 *          with reverse case characters so both upper and lower case
 *          characters are defined.
 */
void build_boyer_array( void )
{
   /*
    * set up for forward search
    */
   if (g_status.command == DefineGrep  ||  g_status.command == RepeatGrep) {
      memcpy( bm.pattern, sas_bm.pattern, strlen( (char *)sas_bm.pattern )+1 );
      bm.search_defined = OK;
   }

   if (bm.search_defined == OK) {
      build_forward_skip( &bm );
      bm.forward_md2 = calculate_forward_md2( bm.pattern, bm.pattern_length );

      /*
       * set up for backward search
       */
      build_backward_skip( &bm );
      bm.backward_md2 = calculate_backward_md2( bm.pattern, bm.pattern_length );
   }

   /*
    * build an array for search and seize.
    */
   if (sas_bm.search_defined == OK) {
      build_forward_skip( &sas_bm );
      sas_bm.forward_md2 = calculate_forward_md2( sas_bm.pattern,
                                                  sas_bm.pattern_length );

      /*
       * set up for backward search
       */
      build_backward_skip( &sas_bm );
      sas_bm.backward_md2 = calculate_backward_md2( sas_bm.pattern,
                                                    sas_bm.pattern_length );
   }
}


/*
 * Name:    build_forward_skip
 * Purpose: build Boyer-Moore forward skip array
 * Date:    October 31, 1992
 * Passed:  bmp:  pointer to Boyer-Moore structure
 * Notes:   build standard Boyer-Moore forward skip array.
 *          Thanks to Tom Waters, twaters@relay.nswc.navy.mil, for finding a
 *           bug in TDE 1.3 when building the ignore skip index array.
 *
 * jmh 030326: use bj_isalpha() and upper_lower[] instead of 'Aa'..'Zz'.
 */
void build_forward_skip( boyer_moore_type *bmp )
{
register text_ptr p;
register int i;
int  c;

   i = bmp->pattern_length = (int)strlen( (char *)bmp->pattern );

   /*
    * set skip index of all characters to length of string
    */
   memset( bmp->skip_forward, i, 256 );
   i--;

   /*
    * for each character in string, set the skip index
    */
   for (p=bmp->pattern; i>=0; i--, p++) {

      assert( (char)i < bmp->skip_forward[*p] );

      bmp->skip_forward[*p] = (char)i;
      if (mode.search_case == IGNORE  &&  bj_isalpha( *p )) {
         c = upper_lower[*p];

         assert( (char)i < bmp->skip_forward[c] );

         bmp->skip_forward[c] = (char)i;
      }
   }
}


/*
 * Name:    build_backward_skip
 * Purpose: build Boyer-Moore backward skip array
 * Date:    October 31, 1992
 * Passed:  bmp:  pointer to Boyer-Moore structure
 * Notes:   build standard Boyer-Moore backward skip array.
 *          Thanks to Tom Waters, twaters@relay.nswc.navy.mil, for finding a
 *           bug in TDE 1.3 when building the ignore skip index array.
 *
 * jmh 030326: use bj_isalpha() and upper_lower[] instead of 'Aa'..'Zz'.
 */
void build_backward_skip( boyer_moore_type *bmp )
{
register text_ptr p;
register int i;
int  c;

   i = -bmp->pattern_length;
   memset( bmp->skip_backward, i, 256 );
   i++;
   for (p=bmp->pattern + bmp->pattern_length - 1; i<=0; i++, p--) {

      assert( (char)i > bmp->skip_backward[*p] );

      bmp->skip_backward[*p] = (char)i;
      if (mode.search_case == IGNORE  &&  bj_isalpha( *p )) {
         c = upper_lower[*p];

         assert( (char)i > bmp->skip_backward[c] );

         bmp->skip_backward[c] = (char)i;
      }
   }
}


/*
 * Name:    calculate_forward_md2
 * Purpose: Calculate mini delta2 for forward searches
 * Date:    October 31, 1992
 * Passed:  p:  pointer to pattern
 *          len: length of pattern
 * Notes:   Hume and Sunday (see above) demonstrate in their paper that
 *            using a simple shift function on mismatches with BM
 *            gives one of the fastest run times for general text searching.
 *          mini delta2 is, from the end of the string, the first leftmost
 *            occurrence of the rightmost character.  mini delta2 is 1 in
 *            the worst case.  in previous versions of TDE, the shift function
 *            was hard-coded as 1 -- the worst case.  typically, mini delta2
 *            will be the length of the search pattern.
 */
int  calculate_forward_md2( text_ptr p, int len )
{
int  last_c;
register int i;
register int md2;

   md2 = 1;
   if (len > 1) {
      i = len - 1;
      last_c = p[i];
      if (mode.search_case == IGNORE) {
         last_c = bj_tolower( last_c );
         while (--i >= 0  &&  last_c != bj_tolower( p[i] ))
            ++md2;
      } else
         while (--i >= 0  &&  last_c != p[i])
            ++md2;

      assert( md2 >= 1  &&  md2 <= len );
   }

   return( md2 );
}


/*
 * Name:    calculate_backward_md2
 * Purpose: Calculate mini delta2 for backward searches
 * Date:    October 31, 1992
 * Passed:  p:  pointer to pattern
 *          len: length of pattern
 * Notes:   the backward mini delta2 is, from the start of the string, the
 *            first rightmost occurrence of the leftmost character.  in the
 *            worst case, mini delta2 is -1.  typically, mini delta2 is the
 *            negative length of the search pattern.
 */
int  calculate_backward_md2( text_ptr p, int len )
{
int  first_c;
register int i;
register int md2;

   md2 = -1;
   if (len > 1) {
      i = 1;
      first_c = *p;
      if (mode.search_case == IGNORE) {
         first_c = bj_tolower( first_c );
         for (; i < len  &&  first_c != bj_tolower( p[i] ); i++)
            --md2;
      } else
         for (; i < len  &&  first_c != p[i]; i++)
            --md2;

      assert( md2 <= -1  &&  md2 >= -len );
   }

   return( md2 );
}


/*
 * Name:    search_forward
 * Purpose: search forward for pattern using boyer array
 * Passed:  ll:     pointer to current node in linked list
 *          rline:  pointer to real line counter
 *          offset: offset into ll->line to begin search
 * Returns: position in file if found or NULL if not found
 * Date:    January 8, 1992
 * Notes:   mini delta2 is the first leftmost occurrence of the rightmost
 *            character.
 *
 * jmh 991007: added block searching.
 */
line_list_ptr search_forward( line_list_ptr ll, long *rline, int *offset )
{
register int i;
text_ptr p;
text_ptr q;
int  mini_delta2;
unsigned int  mini_guard;
int  guard;
int  pat_len;
int  len_s;
text_ptr s;
char *skip;
boyer_moore_type *bmp;
file_infos *file;
int  bc, ec;

   if (ll->next == NULL)
      return( NULL );

   if (g_status.command == DefineGrep  ||  g_status.command == RepeatGrep)
      bmp = &sas_bm;
   else
      bmp = &bm;

   pat_len  = bmp->pattern_length;
   mini_delta2 = bmp->forward_md2;
   skip = bmp->skip_forward;
   p    = bmp->pattern;

   i = pat_len - 1;
   guard = -i;
   mini_guard = *p;
   if (mode.search_case != MATCH)
      mini_guard = bj_tolower( mini_guard );

   file = g_status.current_file;
   s = ll->line;
   len_s = ll->len;
   if ((g_status.search & SEARCH_BLOCK)  &&
       (file->block_type == BOX  ||
        (file->block_type == STREAM  &&
         (*rline == file->block_br ||
          (*rline == file->block_er && file->block_ec != -1))))) {
      bc = file->block_bc;
      ec = (file->block_ec == -1) ? len_s : file->block_ec;
      if (last_replace_line == *rline) {
         ec += block_replace_offset;
         block_replace_offset = 0;
         last_replace_line = 0;
      }
      if (file->inflate_tabs) {
         bc = entab_adjust_rcol( s, len_s, bc, file->ptab_size );
         if (file->block_ec != -1)
            ec = entab_adjust_rcol( s, len_s, ec, file->ptab_size );
      }
      if (bc > len_s)
         bc = len_s;
      if (ec >= len_s)
         ec = len_s - 1;

      if (file->block_type == STREAM) {
         if (*rline == file->block_br) {
            if (*offset < bc)
               *offset = bc;
         }
         if (*rline == file->block_er) {
            if (*offset > ec)
               return( NULL );
            len_s = ec + 1;
         }
      } else {
         if (*offset < bc)
            *offset = bc;
         len_s = ec + 1;
      }
   }
   s += *offset;
   len_s -= *offset;

   for (;;) {
      /*
       * Boyer-Moore fast skip loop.  check character count as we move
       *   down the line.
       */
      for (s+=i, len_s-=i; len_s > 0 && (i = skip[*s],i); s+=i, len_s-=i);
      if (len_s > 0) {
         /*
          * i == 0, possible match.  Boyer-Moore slow match loop.
          */

         if (mode.search_case == MATCH) {
            if (s[guard] != mini_guard)
               goto shift_func;

            q = s + 1 - pat_len;
            for (i=0; i < pat_len; i++)
               if (q[i] != p[i])
                  goto shift_func;
         } else {
            if ((unsigned int)bj_tolower( s[guard] ) != mini_guard)
               goto shift_func;

            q = s + 1 - pat_len;
            for (i=0; i < pat_len; i++)
               if (bj_tolower( q[i] ) != bj_tolower( p[i] ))
                  goto shift_func;
         }
         *offset = (int)(q - ll->line);

         assert( *offset <= ll->len - pat_len );

         if (g_status.search & SEARCH_RESULTS) {
            if (!add_search_line( ll, *rline, *offset ))
               return( NULL );
            len_s = 0;
         } else {
            found_len = pat_len;
            return( ll );
         }
      }
shift_func:
      if (len_s <= 0) {
         ll = ll->next;
         if (ll->len == EOF)
            return( NULL );
         ++*rline;
         i = pat_len - 1;
         s = ll->line;
         len_s = ll->len;
         if ((g_status.search & SEARCH_BLOCK)  &&
             (file->block_type == BOX  ||
              (file->block_type == STREAM  &&
               *rline == file->block_er && file->block_ec != -1))) {
            bc = file->block_bc;
            ec = file->block_ec;
            if (file->inflate_tabs) {
               bc = entab_adjust_rcol( s, len_s, bc, file->ptab_size );
               ec = entab_adjust_rcol( s, len_s, ec, file->ptab_size );
            }
            if (bc > len_s)
               bc = len_s;
            if (ec >= len_s)
               ec = len_s - 1;

            if (file->block_type == STREAM)
               len_s = ec + 1;
            else {
               s += bc;
               len_s = ec - bc + 1;
            }
         }
      } else
         i = mini_delta2;
   }
}


/*
 * Name:    search_backward
 * Purpose: search backward for pattern using boyer array
 * Passed:  ll:  pointer to node in linked list to start search
 *          rline:  pointer to real line counter
 *          offset:  offset into ll->line to start search
 * Returns: position in file if found else return NULL
 * Date:    January 8, 1992
 * Notes:   Start searching from cursor position to beginning of file.
 *          mini delta2 is the first rightmost occurrence of the leftmost char.
 */
line_list_ptr search_backward( line_list_ptr ll, long *rline, int *offset )
{
register int i;
text_ptr p;
int  mini_delta2;
int  pat_len;
int  len_s;
text_ptr s;
file_infos *file;
int  bc, ec;

   if (ll == NULL || ll->len == EOF)
      return( NULL );

   mini_delta2 = bm.backward_md2;
   pat_len  = bm.pattern_length;
   p = bm.pattern;

   i = 1 - pat_len;
   file = g_status.current_file;
   s = ll->line;
   len_s = ll->len;
   if ((g_status.search & SEARCH_BLOCK)  &&
       (file->block_type == BOX  ||
        (file->block_type == STREAM  &&
         (*rline == file->block_br ||
          (*rline == file->block_er && file->block_ec != -1))))) {
      bc = file->block_bc;
      ec = (file->block_ec == -1) ? len_s : file->block_ec;
      if (file->inflate_tabs) {
         bc = entab_adjust_rcol( s, len_s, bc, file->ptab_size );
         if (file->block_ec != -1)
            ec = entab_adjust_rcol( s, len_s, ec, file->ptab_size );
      }
      if (bc > len_s)
         bc = len_s;
      if (ec >= len_s)
         ec = len_s - 1;

      if (file->block_type == STREAM) {
         if (*rline == file->block_er) {
            if (*offset > ec)
               *offset = ec;
            len_s = *offset + 1;
         }
         if (*rline == file->block_br) {
            if (*offset < bc)
               return( NULL );
            len_s = *offset - bc + 1;
         }
         s += *offset;
      } else {
         if (*offset > ec)
            *offset = ec;
         s += *offset;
         len_s = *offset - bc + 1;
      }
   } else {
      if (s != NULL) {          /* jmh (23/11/97) - prevents an empty   */
         s += *offset;          /* last line from crashing when search- */
         len_s = *offset + 1;   /* ing for one character.               */
      } else
         len_s = 0;
   }

   for (;;) {

      /*
       * Boyer-Moore fast skip loop.  check character count as we move
       *   down the line.
       */
      for (s+=i, len_s+=i; len_s > 0 &&
              (i = bm.skip_backward[*s],i); s+=i, len_s+=i);
      if (len_s > 0) {
         /*
          * i == 0, possible match.  Boyer-Moore slow match loop.
          */
         if (mode.search_case == MATCH) {
            for (i=0; i < pat_len; i++)
               if (s[i] != p[i])
                 goto shift_func;
         } else {
            for (i=0; i < pat_len; i++)
               if (bj_tolower( s[i] ) != bj_tolower( p[i] ))
                  goto shift_func;
         }
         *offset = (int)(s - ll->line);

         assert( *offset <= ll->len - pat_len );

         if (g_status.search & SEARCH_RESULTS) {
            if (!add_search_line( ll, *rline, *offset ))
               return( NULL );
            len_s = 0;
         } else {
            found_len = pat_len;
            return( ll );
         }
      }
shift_func:
      if (len_s <= 0) {
         ll = ll->prev;
         if (ll->len == EOF)
            return( NULL );
         --*rline;
         i = 1 - pat_len;
         len_s = ll->len;
         s = ll->line + len_s - 1;
         if ((g_status.search & SEARCH_BLOCK)  &&
             (file->block_type == BOX  ||
              (file->block_type == STREAM  &&  *rline == file->block_br))) {
            bc = file->block_bc;
            ec = (file->block_ec == -1) ? 0 : file->block_ec;
            if (file->inflate_tabs) {
               bc = entab_adjust_rcol( s, len_s, bc, file->ptab_size );
               ec = entab_adjust_rcol( s, len_s, ec, file->ptab_size );
            }
            if (bc > len_s)
               bc = len_s;
            if (ec >= len_s)
               ec = len_s - 1;

            if (file->block_type == STREAM)
               len_s -= bc;
            else {
               s = ll->line + ec;
               len_s = ec - bc + 1;
            }
         }
      } else
         i = mini_delta2;
   }
}


/*
 * Name:    show_search_message
 * Purpose: display search status
 * Date:    January 8, 1992
 * Passed:  i:     index into message array
 * jmh 980816: included the non-search messages (diff and next key).
 *             removed color parameter.
 * jmh 991120: added the undo messages.
 */
void show_search_message( int i )
{
   /*
    *  0 = blank
    *  1 = wrapped...
    *  2 = searching
    *  3 = replacing
    *  4 = nfa choked
    *  5 = diffing...
    *  6 = Next Key..
    *  7 = undo one
    *  8 = undo group
    *  9 = undo place
    * 10 = undo move
    * 11 = tallying..
    * 12 = changed
    * 13 = sorting...
    */
   assert( i >= 0  &&  i <= 13);
   s_output( find7[i], g_display.mode_line, 67,
             (i == CLR_SEARCH) ? ((g_status.search & SEARCH_I)
                                   ? Color( Help )
                                   : Color( Mode ))
                               : Color( Diag ) );
   refresh( );
}


/*
 * Name:    bin_offset_adjust
 * Purpose: modify bin_offset for a different line
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 *          change:  relative position from current rline
 * Notes:   in binary mode, we keep an accurate count of the offset of the
 *          cursor from the beginning of the file.
 */
void bin_offset_adjust( TDE_WIN *window, long change )
{
line_list_ptr node;

   node = window->ll;
   if (change < 0) {
      do {
         node = node->prev;
         window->bin_offset -= node->len;
      } while (++change != 0);
   } else if (change > 0) {
      if (node->len == EOF)
         ++window->bin_offset;
      do {
         window->bin_offset += node->len;
         node = node->next;
      } while (--change != 0);
   }
   assert( window->bin_offset >= 0 );
}


/*
 * Name:    find_adjust
 * Purpose: place cursor on screen given a position in file - default cline
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 *          ll:      position anywhere in file
 *          rline:   real line number of ll in the file
 *          rcol:    real column of ll in the file
 * Notes:   ll could be anywhere in file.  Find the start of line that
 *          ll is on.  Determine if start of line is behind or ahead of
 *          current line.  Once that is done, it is easy to determine if ll
 *          is on screen.  Lastly, current cursor position becomes start of
 *          ll line - reposition and display.
 *
 * jmh 991125: call bin_offset_adjust().
 */
void find_adjust( TDE_WIN *window, line_list_ptr ll, long rline, int rcol )
{
long test_line;
file_infos *file;
register TDE_WIN *win;  /* put window pointer in a register */
int  tabs;

   win = window;
   file = win->file_info;

   /*
    * move the cursor to the line; if it's off-screen, center it.
    */
   test_line = rline - win->rline;
   if (test_line) {
      bin_offset_adjust( win, test_line );
      win->rline = rline;
      win->ll    = ll;

      if (labs( test_line ) >= g_display.end_line) {
         test_line = 0;
         file->dirty = LOCAL;
      }
      if (check_cline( win, win->cline + (int)test_line ))
         file->dirty = LOCAL;

      if (file->dirty == LOCAL) {
         if (g_status.command != ReplaceString  ||  win->visible)
            center_window( win );
      }
   }

   /*
    * given a real column, adjust for inflated tabs.
    */
   if (file->inflate_tabs)
      tabs = file->ptab_size;
   else {
      tabs = 0;
      found_vlen = found_len;
   }
   if ((g_status.search & SEARCH_I) && !(g_status.search & SEARCH_BACK)) {
      /*
       * forward isearch leaves rcol at the end of the match
       */
      found_rcol = rcol - found_len;
      if (tabs) {
         rcol = detab_adjust_rcol( ll->line, rcol, tabs );
         found_rcol = detab_adjust_rcol( ll->line, found_rcol, tabs );
         found_vlen = rcol - found_rcol;
      }
   } else {
      if (tabs) {
         found_vlen = detab_adjust_rcol( ll->line, rcol + found_len, tabs );
         rcol = detab_adjust_rcol( ll->line, rcol, tabs );
         found_vlen -= rcol;
      }
      found_rcol = rcol;
   }

   found_rline = rline;

   show_ruler_char( win );
   check_virtual_col( win, rcol, rcol );
}


/*
 * Name:    replace_string
 * Purpose: To set up and perform a replace operation.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 *
 * jmh 991008: added block replace, start position.
 * jmh 991010: if no prompting, remember cursor position for tabs.
 * jmh 031027: fixed long-standing bug if wrap occurred before being found.
 * jmh 031120: fixed my solution to the above;
 *             replace the match at the cursor first, not last.
 * jmh 031126: fix display bug after continuing the wrap, but then exiting;
 *             replace across all files.
 * jmh 050709: only return ERROR on control-break.
 */
int  replace_string( TDE_WIN *window )
{
int  block;
int  direction;
int  sub_len;
int  net_change;
int  finished;
int  use_prev_find;
int  rc;
int  rcol;
int  rcol_limit;
int  wrapped;
char *answer;
long found_line;
long line_limit;
line_list_ptr  ll;
TDE_WIN  wp;
TDE_WIN  old_wp;
register TDE_WIN *win;    /* put window pointer in a register */
line_list_ptr (*search_func)( TDE_WIN *, long *, int * );

   win = window;
   if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
      return( ERROR );

   check_block( NULL );
   if (g_status.marked && g_status.marked_file == win->file_info)
      block = TRUE;
   else
      block = CB_R_Block = FALSE;
   replace_dialog[IDC_R_BLOCK].hist = (block) ? NULL : (HISTORY*)ERROR;

   replace_dialog->n = IDE_R_PATTERN;  /* always start at the pattern */
   if (do_dialog( replace_dialog, replace_proc ) == ERROR)
      return( ERROR );

   answer = get_dlg_text( EF_R_Pattern );
   if (*answer == '\0')
      return( ERROR );

   if (CB_R_RegX) {
      regx.search_defined = OK;
      g_status.search = SEARCH_REGX;
      net_change = 0;
   } else {
      bm.search_defined = OK;
      strcpy( (char *)bm.pattern, answer );
      build_boyer_array( );
      g_status.search = 0;
      net_change = strlen( answer );
   }
   if (CB_R_Backward) g_status.search |= SEARCH_BACK;
   if (CB_R_Begin)    g_status.search |= SEARCH_BEGIN;
   if (CB_R_Block)    g_status.search |= SEARCH_BLOCK;
   if (CB_R_All)      g_status.search |= SEARCH_ALL;

   g_status.replace_flag = (CB_R_NPrompt) ? NOPROMPT : PROMPT;
   direction = (CB_R_Backward) ? BACKWARD : FORWARD;

   answer = get_dlg_text( EF_R_Replace );
   strcpy( (char *)g_status.subst, answer );
   sub_len = strlen( answer );
   if (!(g_status.search & SEARCH_REGX))
      net_change = sub_len - net_change;

   update_line( win );
   g_status.replace_defined = OK;

   rc = OK;
   if (g_status.replace_flag == NOPROMPT) {
      show_search_message( REPLACING );
      xygoto( win->ccol, win->cline );
   }
   finished = FALSE;
   use_prev_find = FALSE;
   line_limit = 0;
   rcol_limit = 0;
   wrapped = FALSE;
   dup_window_info( &wp, win );
   if (win->file_info->inflate_tabs)
      wp.rcol = entab_adjust_rcol( win->ll->line, win->ll->len, win->rcol,
                                   win->file_info->ptab_size );
   dup_window_info( &old_wp, &wp );
   if (direction == FORWARD) {
      search_func = forward_search;
      --wp.rcol;
   } else {
      search_func = backward_search;
      ++wp.rcol;
   }
   first_win = win;
   search_file = first_file = win->file_info;
   if ((ll = search_func( &wp, &found_line, &rcol )) != NULL  &&
       !g_status.control_break) {
      line_limit = found_line;
      rcol_limit = rcol;
      replace_and_display( &wp, ll, found_line, rcol, &finished, direction );
      if (finished == ERROR  &&  g_status.replace_flag == PROMPT)
         use_prev_find = TRUE;
   } else {
      /*
       * string not found
       */
      error( WARNING, win->bottom_line, find8 );
      finished = ERROR;
   }
   while (finished == FALSE) {
      if (g_status.replace_flag == PROMPT) {
         found_rline = -1;
         update_line( &wp );
      }
      dup_window_info( &old_wp, &wp );
      if ((ll = search_func( &wp, &found_line, &rcol )) != NULL  &&
          !g_status.control_break) {
         if (g_status.wrapped)
            wrapped = TRUE;
         if (wrapped) {
            if (direction == FORWARD) {
               if (found_line > line_limit  ||
                   (found_line == line_limit  &&  rcol >= rcol_limit)) {
                  finished = TRUE;
                  use_prev_find = TRUE;
               }
            } else {
               if (found_line < line_limit  ||
                   (found_line == line_limit  &&  rcol <= rcol_limit)) {
                  finished = TRUE;
                  use_prev_find = TRUE;
               }
            }
         }
         if (finished == FALSE) {
            rc = replace_and_display( &wp, ll, found_line, rcol, &finished,
                                      direction );
            if (rc == OK && search_file == first_file &&
                            found_line == line_limit && rcol < rcol_limit) {
               if (g_status.search & SEARCH_REGX)
                  net_change = sub_len - found_len;
               rcol_limit += net_change;
            }
            if (finished == ERROR  &&  g_status.replace_flag == PROMPT) {
               use_prev_find = TRUE;
               finished = TRUE;
            }
            rc = OK;
         }
      } else {
         if (g_status.control_break) {
            error( WARNING, wp.bottom_line, cb );
            rc = ERROR;
         }
         finished = TRUE;
      }
   }

   found_rline = -1;

   if (g_status.replace_flag == PROMPT) {
      if (finished != ERROR) {
         if (use_prev_find)
            dup_window_info( &wp, &old_wp );
         if (wp.file_info->inflate_tabs)
            wp.rcol = detab_adjust_rcol( wp.ll->line, wp.rcol,
                                         wp.file_info->ptab_size );
         win = g_status.current_window;
         dup_window_info( win, &wp );
         check_virtual_col( win, win->rcol, win->ccol );
      }
   } else {
      show_search_message( CLR_SEARCH );
      g_status.wrapped = FALSE;
   }
   show_curl_line( win );

   return( rc );
}


/*
 * Name:    replace_proc
 * Purpose: dialog callback for ReplaceString
 * Author:  Jason Hood
 * Date:    November 30, 2003
 * Notes:   if using RegX, check the pattern compiles successfully;
 *          since Block and All searches are complementary, turning on one
 *           will turn off the other.
 */
int  replace_proc( int id, char* text )
{
int  other;
int  rc = OK;

   if (id == IDE_R_PATTERN || id == 0) {
      if (CB_R_RegX) {
         if (id == 0)
            text = get_dlg_text( EF_R_Pattern );
         if (*text != '\0') {
            strcpy( (char *)regx.pattern, text );
            if (build_nfa( ) == ERROR) {
               regx.search_defined = FALSE;
               rc = IDE_R_PATTERN;
            }
         }
      }
   } else {
      if (id == IDC_R_BLOCK || id == IDC_R_ALL) {
         other = IDC_R_BLOCK + IDC_R_ALL - id;
         if (replace_dialog[id].n && replace_dialog[other].n)
            check_box( other );
      }
   }

   return( rc );
}


/*
 * Name:    replace_and_display
 * Purpose: To make room for replacement string
 * Date:    June 5, 1991
 * Passed:  window:            pointer to current window
 *          ll:                pointer to position of pattern in file
 *          rline:             pointer to real line counter
 *          rcol:              pointer to real column
 *          finished:          stop replacing on this occurrence?
 *          direction:         direction of search
 * Notes:   Show user where pattern_location is on screen if needed.
 *          Replace and display if needed.   Always ask the user if he
 *          wants to do wrapped replacing.
 *
 * jmh 980728: syntax check the change if window is invisible.
 * jmh 031126: test for match in different file.
 */
int  replace_and_display( TDE_WIN *window, line_list_ptr ll, long rline,
                          int rcol, int *finished, int direction )
{
register int rc;
file_infos *file;
register TDE_WIN *win;  /* put window pointer in a register */
int  visible;
int  changed;

   win = window;
   file = win->file_info;
   rc = OK;
   if (g_status.wrapped) {
      rc = ask_wrap_replace( win, finished );
      g_status.wrapped = FALSE;
      show_search_message( CLR_SEARCH );
   }
   if (rc == OK) {
      changed = (file != search_file);
      if (changed) {
         win = find_file_window( search_file );
         if (g_status.replace_flag == PROMPT)
            change_window( g_status.current_window, win );
         dup_window_info( window, win );
         win = window;
         file = search_file;
      }
      visible = win->visible;
      if (g_status.replace_flag == NOPROMPT)
         win->visible = FALSE;
      find_adjust( win, ll, rline, rcol );
      if (g_status.replace_flag == PROMPT) {
         show_ruler( win );
         show_ruler_pointer( win );
         if (file->dirty) {
            display_current_window( win );
            file->dirty = FALSE;
         } else
            show_curl_line( win );
         xygoto( win->ccol, win->cline );
         show_line_col( win );
         if (changed)
            show_search_message( CHANGED );
         rc = ask_replace( win, finished );
         if (changed)
            show_search_message( CLR_SEARCH );
      }
      if (rc == OK) {
         do_replace( win, direction );
         file->modified = TRUE;
         file->dirty = GLOBAL;
         if (g_status.replace_flag == PROMPT) {
            show_changed_line( win );
            file->dirty = FALSE;
         } else
            syntax_check_lines( win->ll, file->syntax );
      }
      if (file->inflate_tabs)
         win->rcol = entab_adjust_rcol( ll->line, ll->len, win->rcol,
                                        file->ptab_size );
      win->visible = visible;
   }
   return( rc );
}


/*
 * Name:    scan_forward
 * Purpose: To find the corresponding occurrence of target, ignoring
 *           embedded pairs of opp and target, searching forwards.
 * Date:    June 5, 1991
 * Passed:  win:    pointer to window containing current position
 *          rcol:   pointer to real column position
 *          target: the character to be found
 *          opp:    the opposite to target
 * Returns: OK if found (win updated with new position), ERROR otherwise
 */
int  scan_forward( TDE_WIN *win, int *rcol, char target, char opp )
{
line_list_ptr ll;
text_ptr s;
int  len;
int  count = 1;         /* number of unmatched opposites found */
register char c;

   ++*rcol;
   ll  = win->ll;
   s   = ll->line + *rcol;
   len = ll->len  - *rcol;
   for (;;) {
      while (--len >= 0) {
         c = *s++;
         if (c == target) {
            if (--count == 0) {
               *rcol = (int)(s - ll->line - 1);
               return( OK );
            }
         } else if (c == opp)
            count++;
      }
      if (!inc_line( win, FALSE ))
         break;
      ll  = win->ll;
      s   = ll->line;
      len = ll->len;
   }
   return( ERROR );
}


/*
 * Name:    scan_backward
 * Purpose: To find the corresponding occurrence of target, ignoring
 *           embedded pairs of opp and target, searching backwards.
 * Date:    June 5, 1991
 * Passed:  win:    pointer to window containing current position
 *          rcol:   pointer to real column position
 *          target: the character to be found
 *          opp:    the opposite to target
 * Returns: OK if found (win updated with new position), ERROR otherwise
 */
int  scan_backward( TDE_WIN *win, int *rcol, char target, char opp )
{
line_list_ptr ll;
text_ptr s;
int  len;
int  count = 1;         /* number of unmatched opposites found */
register char c;

   ll  = win->ll;
   len = *rcol;
   for (;;) {
      s = ll->line + len - 1;
      while (--len >= 0) {
         c = *s--;
         if (c == target) {
            if (--count == 0) {
               *rcol = (int)(s - ll->line + 1);
               return( OK );
            }
         } else if (c == opp)
            count++;
      }
      if (!dec_line( win, FALSE ))
         break;
      ll  = win->ll;
      len = ll->len;
   }
   return( ERROR );
}


/*
 * Name:    match_pair
 * Purpose: To find the corresponding pair to the character under the
 *           cursor.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   Searching is very simple-minded, and does not cope with things
 *          like brackets embedded within quoted strings.
 *
 * jmh 980614: added quote (") searching (prompting for direction) and syntax
 *             highlighting strings, characters and multi-line comments.
 *             Escapes are not recognized and if the two characters are
 *             different nesting will be performed, which may or may not be
 *             appropriate for the language. The search may also fail if the
 *             line contains NUL characters (ie. character number 0).
 *             For two-character multi-line comments, you should be on the
 *             first of the start and the second of the end.
 *
 * jmh 991125: corrected bug with failed two-character matching;
 *             renamed "one" to "opp", "two" to "tgt" (target) and put
 *              the target first in the scan_ functions.
 *             added the "<>" pair;
 *             created the pairs[] array.
 *
 * jmh 011123: rewrote all the pair finding (due to the new comment syntax, but
 *              multi-character comments still only use two characters).
 */
int  match_pair( TDE_WIN *window )
{
register TDE_WIN *win;  /* put window pointer in a register */
TDE_WIN w;
int  rc;
int  rcol;
text_ptr s;
int  len;
int  direction;
int  (*scan)( TDE_WIN *, int *, char, char );
char ch;
char tgt = 0,                   /* initialize a couple of variables */
     opp = 0;                   /* to remove warnings */
int  multi = FALSE;
syntax_info *info = NULL;
char comstart1,   comstart2 = 0,
     comend1 = 0, comend2;
static char pairs[9*2+1] = "()[]{}<>\"\"";
char *match;
int  single[2] = { FALSE, FALSE };
int  i, j;

   win = window;
   if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
      return( ERROR );

   /*
    * make sure the character under the cursor is one that has a
    *  matched pair.
    */
   rc   = direction = ERROR;
   s    = win->ll->line;
   len  = win->ll->len;
   rcol = win->rcol;
   if (win->file_info->inflate_tabs)
      rcol = entab_adjust_rcol( s, len, rcol, win->file_info->ptab_size );

   if (rcol < len) {
      /*
       * find the matching pair
       */
      i = 10;
      if (win->syntax) {
         info = win->file_info->syntax->info;
         if (info->strstart) {
            pairs[i++] = info->strstart;
            pairs[i++] = info->strend;
         }
         if (info->charstart) {
            pairs[i++] = info->charstart;
            pairs[i++] = info->charend;
         }
         if (info->comstart[0][0]) {
            if (!info->comstart[0][1] && !info->comend[0][1]) {
               pairs[i++] = info->comstart[0][0];
               pairs[i++] = info->comend[0][0];
               single[0] = TRUE;
            }
            if (info->comstart[1][0] &&
                !info->comstart[1][1] && !info->comend[1][1]) {
               pairs[i++] = info->comstart[1][0];
               pairs[i++] = info->comend[1][0];
               single[1] = TRUE;
            }
         }
      }
      pairs[i] = '\0';

      ch = *(s + rcol);
      match = pairs;
      do {
         if (ch == *match  &&  ch == *(match+1)) {
            tgt = ch;
            opp = '\0';
            /*
             * Search forward or backward
             */
            direction = get_response( find13, win->bottom_line, R_ALL, 2,
                                     L_FORWARD, FORWARD, L_BACKWARD, BACKWARD );
            break;
         }
         opp = ch;
         if (ch == *match) {
            tgt = *(match+1);
            direction = FORWARD;
            break;
         }
         if (ch == *(match+1)) {
            tgt = *match;
            direction = BACKWARD;
            break;
         }
         match += 2;
      } while (*match);

      if (!*match  &&  win->syntax) {
         for (i = 0; i < 2; ++i) {
            if (!single[i] && info->comstart[i][0]) {
               comstart1 = info->comstart[i][0];
               comstart2 = info->comstart[i][1];
               for (j = 0; info->comend[i][j]; ++j);
               if (j == 1)
                  j = 2;
               comend1 = info->comend[i][j-2];
               comend2 = info->comend[i][j-1];
               opp = ch;
               if (ch == comstart1 && (!comstart2 ||
                    (rcol+1 < len && s[rcol+1] == comstart2))) {
                  direction = FORWARD;
                  if (comstart2 || comend2)
                     opp = '\0';
                  if (comstart2)
                     ++rcol;
                  if (comend2) {
                     tgt = comend2;
                     multi = TRUE;
                  } else
                     tgt = comend1;
                  break;
               } else if ((ch == comend1 && !comend2) ||
                          (comend2 && ch == comend2 &&
                           rcol-1 >= 0 && s[rcol-1] == comend1)) {
                  direction = BACKWARD;
                  tgt = comstart1;
                  if (comstart2 || comend2)
                     opp = '\0';
                  if (comend2)
                     --rcol;
                  if (comstart2)
                     multi = TRUE;
                  break;
               }
            }
         }
      }

      if (direction != ERROR) {
         dup_window_info( &w, win );
         scan = (direction == FORWARD) ? scan_forward : scan_backward;
         rc = scan( &w, &rcol, tgt, opp );
         if (multi)
            while (rc == OK) {
               s = w.ll->line + rcol;
               if (direction == FORWARD) {
                  if (rcol-1 >= 0 && *(s-1) == comend1)
                     break;
               } else
                  if (rcol+1 < w.ll->len && *(s+1) == comstart2)
                     break;
               rc = scan( &w, &rcol, tgt, opp );
            }
         /*
          * now show the user what we have found
          */
         if (rc == OK) {
            undo_move( win, 0 );
            set_marker( win );          /* remember the previous position */
            if (w.file_info->inflate_tabs)
               rcol = detab_adjust_rcol( w.ll->line, rcol,
                                         w.file_info->ptab_size );
            w.rcol = rcol;
            move_display( win, &w );
         }
      }
   }
   return( rc );
}


/*
 * Name:    add_search_line
 * Purpose: add a line to the search results window
 * Author:  Jason Hood
 * Date:    November 16, 2003
 * Passed:  ll:    line to add
 *          rline: its line number
 *          rcol:  real column of the match
 * Returns: TRUE if line added, FALSE if not (search should stop)
 * Notes:   if results_window is NULL a new window is created, otherwise the
 *           line is appended to the current.
 *
 * 070501:  pad line numbers according to current file length and column
 *           numbers to two digits.
 */
int  add_search_line( line_list_ptr ll, long rline, int rcol )
{
line_list_ptr temp_ll;
text_t buf[MAX_LINE_LENGTH];
int  len, llen, flen;
TDE_WIN    *old_window;
file_infos *old_file;
int  rc = OK;
TDE_WIN w;

   if (g_status.wrapped) {
      g_status.wrapped = FALSE;
      if (ask_wrap_replace( g_status.current_window, &rc ) == ERROR)
         return( FALSE );
   }

   if (results_window == NULL) {
      old_window = g_status.current_window;     /* remember what we're  */
      old_file   = g_status.current_file;       /*  searching           */
      /*
       * don't display the results window until the search is done
       */
      if (attempt_edit_display( "", NOT_LOCAL ) == ERROR)
         return( FALSE );
      results_window = g_status.current_window;
      results_file   = g_status.current_file;
      g_status.current_window = old_window;
      g_status.current_file   = old_file;

      results_window->visible = (g_status.current_window == NULL);
   } else
      results_file->dirty = LOCAL;

   if (search_file->inflate_tabs)
      rcol = detab_adjust_rcol( ll->line, rcol, search_file->ptab_size );
   ++rcol;

   w.file_info = search_file;
   flen = strlen( relative_path( (char *)buf, &w, FALSE ) );
   len  = 4 + numlen( search_file->length )
          + (numlen( rcol ) > 2 ? numlen( rcol ) : 2);
   llen = ll->len;
   if (llen + flen + len > MAX_LINE_LENGTH)
      llen -= flen + len;
   len = flen + sprintf( (char *)buf + flen, ":%*ld:%2d: ",
                         numlen( search_file->length ), rline, rcol );
   memcpy( buf + len, ll->line, llen );
   temp_ll = new_line_text( buf, len + llen, 0, &rc );
   if (rc == OK) {
      insert_node( results_file, results_file->line_list_end, temp_ll );
      if (results_window->ll == results_file->line_list_end)
         results_window->ll = temp_ll;
   }

   return( rc == OK );
}
