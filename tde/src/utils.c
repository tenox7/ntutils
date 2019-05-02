/*******************  start of original comments  ********************/
/*
 * Written by Douglas Thomson (1989/1990)
 *
 * This source code is released into the public domain.
 */

/*
 * Name:    dte - Doug's Text Editor program - miscellaneous utilities
 * Purpose: This file contains miscellaneous functions that were required
 *           in more than one of the other files, or were thought to be
 *           likely to be used elsewhere in the future.
 * File:    utils.c
 * Author:  Douglas Thomson
 * System:  this file is intended to be system-independent
 * Date:    October 1, 1989
 */
/*********************  end of original comments  ********************/


/*
 * The utility routines have been EXTENSIVELY rewritten.  Update screens as
 * needed.  Most times, only one line has changed.  Just show changed line
 * in all windows if it is on screen.
 *
 * Support routines for text lines longer than screen width have been added.
 * Currently support lines as long as 1040 characters.
 *
 * In DTE, Doug chose to test whether characters are part of a word.  In TDE,
 * we will test whether characters are not part of a word.  The character
 * set not part of a word will not change as much as the characters that
 * are part of a word.  In most languages, human and computer, the delimiters
 * are much more common across languages than the tokens that make up a word.
 * Thanks to Pierre Jelenc, pcj1@columbia.edu, for recommending looking for
 * delimiters.
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
 * public domain, Frank Davis.  You may distribute it freely.
 */

#include "tdestr.h"
#include "common.h"
#include "define.h"
#include "tdefunc.h"
#if defined( __WIN32__ )
#include <process.h>
#endif


extern long swap_br, swap_er;           /* block.c */
extern int  swap_bc, swap_ec;

extern int  copied_rcol;                /* undo.c */
extern int  copied_mod;
extern int  copied_dirty;

extern long found_rline;                /* findrep.c */
extern int  found_rcol;
extern int  found_vlen;

       int  auto_reload = FALSE;        /* for file.c */

static int  ruler1 = -1, ruler2 = -1;   /* for the popup ruler */


/*
 * Name:    execute
 * Purpose: run a function
 * Author:  Jason Hood
 * Date:    November 21, 1999
 * Passed:  window:  window where function executes
 * Returns: depends on the function
 * Notes:   ignore non-viewer functions if the file is read-only.
 *          Uses g_status.command for the function, sets g_status.command_count.
 *          Don't keep a count of PullDown.
 */
int  execute( TDE_WIN *window )
{
int  rc;
int  func = g_status.command;

   if (func >= 0 && func < NUM_FUNCS &&
       (!window->file_info->read_only || !(func_flag[func] & F_MODIFY))) {
      if (func != PullDown) {
         if (func == g_status.last_command)
            ++g_status.command_count;
         else
            g_status.command_count = 0;
      }
      rc = (*do_it[func])( window );
      if (func != PullDown)
         g_status.last_command = func;

   } else
      rc = ERROR;

   return( rc );
}


/*
 * Name:    myiswhitespc
 * Purpose: To determine whether or not a character is *NOT* part of a "word".
 * Date:    July 4, 1992
 * Passed:  c: the character to be tested
 * Returns: TRUE if c is in the character set *NOT* part of a word
 * Notes:   The characters in the set not part of a word will not change as
 *           as much as the characters that are part of a word.  In most
 *           languages, human and computer, the delimiters are much more
 *           common than the tokens that make up a word.  For example,
 *           the set of punction characters don't change as much across
 *           languages, human and computer, as the characters that make
 *           up the alphabet, usually.  In other words, the delimiters
 *           are fairly constant across languages.
 */
int  myiswhitespc( int c )
{
   return( c == ' ' || (bj_ispunct( c ) && c != '_') || bj_iscntrl( c ) );
}


/*
 * Name:    myisnotwhitespc
 * Purpose: to determine whether or not a character is part of a word
 * Author:  Jason Hood
 * Date:    May 21, 1998
 * Passed:  c: the character to be tested
 * Returns: TRUE if c is in the character set part of a word
 * Notes:   used by the various word movement functions.
 * 991022:  decided to make this a global function;
 *          renamed from not_myiswhitespc.
 */
int  myisnotwhitespc( int c )
{
   /* return( !myiswhitespc( c ) ); */
   return( c == '_' || (c != ' ' && !bj_ispunct( c ) && !bj_iscntrl( c )) );
}


/*
 * Name:    check_virtual_col
 * Purpose: ensure integrity of rcol, ccol, and bcol
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 *          rcol: real column of cursor
 *          ccol: current or logical column of cursor
 * jmh 980724: update cursor cross
 * jmh 021210: when moving beyond the window, place ccol at the end of the
 *              window, instead of the start.
 */
void check_virtual_col( TDE_WIN *window, int rcol, int ccol )
{
register int bcol;
int  start_col;
int  end_col;
file_infos *file;

   file      = window->file_info;
   bcol      = window->bcol;
   start_col = window->start_col;
   end_col   = window->end_col;

   /*
    * is logical column past end of screen?
    */
   if (ccol > end_col) {
      ccol = rcol - bcol + start_col;
      if (ccol > end_col) {
         ccol = end_col;
         bcol = rcol - (ccol - start_col);
         file->dirty = LOCAL | RULER;
      }
   }

   /*
    * is logical column behind start of screen?
    */
   if (ccol < start_col) {
      if (bcol >= (start_col - ccol))
         bcol -= (start_col - ccol);
      ccol = start_col;
      file->dirty = LOCAL | RULER;
   }

   /*
    * is real column < base column?
    */
   if (rcol < bcol) {
      ccol = rcol + start_col;
      bcol = 0;
      if (ccol > end_col) {
         bcol = rcol;
         ccol = start_col;
      }
      file->dirty = LOCAL | RULER;
   }

   /*
    * current column + base column MUST equal real column
    */
   if ((ccol - start_col) + bcol != rcol) {
      if (bcol < 0 || bcol > rcol) {
         bcol = rcol;
         file->dirty = LOCAL | RULER;
      }
      ccol = rcol - bcol + start_col;
      if (ccol > end_col) {
         /*bcol = rcol;
         ccol = start_col;*/
         ccol = end_col;
         bcol = rcol - (ccol - start_col);
         file->dirty = LOCAL | RULER;
      }
   }

   /*
    * rcol CANNOT be negative
    */
   if (rcol < 0) {
      rcol = bcol = 0;
      ccol = start_col;
      file->dirty = LOCAL | RULER;
   }

   if (rcol >= MAX_LINE_LENGTH) {
      rcol = MAX_LINE_LENGTH - 1;
      bcol = rcol - (ccol - start_col);
      file->dirty = LOCAL | RULER;
   }

   assert( rcol >= 0 );
   assert( rcol < MAX_LINE_LENGTH );
   assert( bcol >= 0 );
   assert( bcol < MAX_LINE_LENGTH );
   assert( ccol >= start_col );
   assert( ccol <= end_col );

   window->bcol = bcol;
   window->ccol = ccol;
   window->rcol = rcol;

   /*
    * If the cursor cross is on, set LOCAL update. This is not as
    * efficient as it should be, but is a whole lot easier than
    * modifying every function that could move the cursor left or right.
    */
   if (mode.cursor_cross)
      file->dirty = LOCAL | RULER;
}


/*
 * Name:    check_cline
 * Purpose: ensure integrity of cline
 * Author:  Jason Hood
 * Date:    March 27, 2003
 * Passed:  window:  pointer to window
 *          cline:   new cline
 * Returns: TRUE if cline was outside the window (an update is required)
 * Notes:   also ensures TOF remains as the top line
 */
int  check_cline( TDE_WIN *window, int cline )
{
int  update = FALSE;

   if (cline < window->top_line) {
      cline  = window->top_line;
      update = TRUE;

   } else {
      if (cline > window->bottom_line) {
         cline  = window->bottom_line;
         update = TRUE;
      }
      if (cline > window->rline + window->top_line) {
         cline  = (int)window->rline + window->top_line;
         update = TRUE;
      }
   }
   window->cline = cline;

   return( update );
}


/*
 * Name:    new_line
 * Purpose: create a new line structure
 * Author:  Jason Hood
 * Date:    February 27, 2003
 * Passed:  type:  type of the line
 *          rc:    place to store error code (should be OK on entry)
 * Returns: the new line
 */
line_list_ptr new_line( long type, int *rc )
{
line_list_ptr ll;

   ll = (line_list_ptr)my_malloc( sizeof(line_list_struc), rc );
   if (*rc == OK) {
      ll->line = NULL;
      ll->len  = 0;
      ll->type = type;
   }
   return( ll );
}


/*
 * Name:    new_line_text
 * Purpose: create a new line with text
 * Author:  Jason Hood
 * Date:    February 27, 2003
 * Passed:  text:  the text of the line to copy
 *          len:   the length of the above
 *          type:  the type of the line
 *          rc:    pointer to return code (should be OK on entry)
 * Returns: pointer to the new line
 */
line_list_ptr new_line_text( text_ptr text, int len, long type, int *rc )
{
line_list_ptr ll;
text_ptr t;

   t  = (text_ptr)my_malloc( len, rc );
   ll = (line_list_ptr)my_malloc( sizeof(line_list_struc), rc );
   if (*rc == OK) {
      my_memcpy( t, text, len );
      ll->line = t;
      ll->len  = len;
      ll->type = type;
   } else {
      my_free( ll );
      my_free( t );
   }
   return( ll );
}


/*
 * Name:    copy_line
 * Purpose: To copy the cursor line, if necessary, into the current line
 *           buffer, so that changes can be made efficiently.
 * Date:    June 5, 1991
 * Passed:  ll: line to be copied to line buffer
 *          window: window containing the line
 *          tabs:   TRUE to detab
 * Notes:   See un_copy_line, the reverse operation.
 *          DO NOT use the C library string functions on text in
 *           g_status.line_buff, because Null characters are allowed as
 *           normal text in the file.
 */
void copy_line( line_list_ptr ll, TDE_WIN *window, int tabs )
{
register int len;

   assert( ll != NULL );

   len = ll->len;
   if (len != EOF) {
      if (g_status.copied == FALSE) {

         assert( len < MAX_LINE_LENGTH );

         my_memcpy( g_status.line_buff, ll->line, len );
         g_status.line_buff_len = len;

         g_status.copied    = TRUE;
         g_status.buff_node = ll;

         /*
          * jmh 021011: remember the original rcol position for RestoreLine
          * jmh 030225: likewise with modified.
          * jmh 030302: and dirty.
          */
         copied_rcol  = window->rcol;
         copied_mod   = window->file_info->modified;
         copied_dirty = (int)ll->type & DIRTY;
      }
      if (tabs)
         detab_linebuff( window->file_info->inflate_tabs,
                         window->file_info->ptab_size );
   }
}


/*
 * Name:    un_copy_line
 * Purpose: To copy the cursor line, if necessary, from the current line
 *           buffer, shifting the main text to make the right amount of
 *           room.
 * Date:    June 5, 1991
 * Passed:  test_line:     location in file to copy line buffer
 *          window:        pointer to current window
 *          del_trailing:  delete the trailing blanks at eol? TRUE or FALSE
 *          tabs:          TRUE to entab
 * Notes:   For some functions, trailing spaces should not be removed when
 *           returning the line buffer to the main text.  The JoinLine function
 *           is a good example.  We need to leave trailing space so when we
 *           join lines - the current line will extend at least up to
 *           the column of the cursor.  We need to leave trailing space
 *           during BOX block operations.
 *          See copy_line, the reverse operation.
 *
 * jmh 011203: only display the available memory if the window is visible.
 * jmh 031114: reset the buff_node pointer (for RestoreLine).
 */
int  un_copy_line( line_list_ptr ll, TDE_WIN *window, int del_trailing,
                   int tabs )
{
text_ptr p;
size_t len;     /* length of line buffer text */
size_t ll_len;  /* length of ll->line */
int  net_change;
int  rc;
text_ptr c;
file_infos *file;
TDE_WIN *wp;

   rc = OK;
   if (mode.do_backups == TRUE)
      rc = backup_file( window );

   if (g_status.copied == TRUE  &&  ll->len != EOF) {

      file = window->file_info;
      if (tabs)
         entab_linebuff( file->inflate_tabs, file->ptab_size );

      /*
       * if we are deleting the entire line, don't worry about
       *  deleting the trailing space, since we're deleting entire line.
       */
      if (g_status.command == DeleteLine)
         del_trailing = FALSE;

      if (del_trailing  &&  mode.trailing  &&  file->crlf != BINARY) {
         len = g_status.line_buff_len - 1;
         for (c = g_status.line_buff + len; (int)len >= 0; len--, c--) {
            if (*c != ' '  &&  (*c != '\t' || !file->inflate_tabs))
               break;
         }
         g_status.line_buff_len = len + 1;
         file->dirty = GLOBAL;
         if (window->visible == TRUE)
            show_changed_line( window );
      }
      len    = g_status.line_buff_len;
      ll_len = ll->len;

      assert( len    < MAX_LINE_LENGTH );
      assert( ll_len < MAX_LINE_LENGTH );

      net_change = len - ll_len;
      if (net_change != 0) {
         /*
          * let's malloc space for the new line before we free the old line.
          */
         p = my_malloc( len, &rc );
         if (rc == ERROR)
            error( WARNING, window->bottom_line, main4 );
         else {
            /*
             * free the space taken up by the old line.
             */
            my_free( ll->line );
            ll->line = p;
         }
      }

      if (rc != ERROR) {
         my_memcpy( ll->line, g_status.line_buff, len );
         ll->len = len;

         if (net_change != 0) {
            for (wp=g_status.window_list; wp != NULL; wp=wp->next) {
               if (wp->file_info == file && wp != window)
                  if (wp->rline > window->rline)
                     wp->bin_offset += net_change;
            }
         }

         file->modified = TRUE;
         if (window->visible)
            show_avail_mem( );
      }
   }
   g_status.copied = FALSE;
   g_status.buff_node = NULL;
   return( rc );
}


/*
 * jmh 980729: This function is currently unused.
 */
#if 0
/*
 * Name:    un_copy_tab_buffer
 * Purpose: To copy the tab buffer line the main text buffer
 * Date:    October 31, 1992
 * Passed:  line_number:  line number to copy line tab out buffer
 *          window:       pointer to current window
 */
int  un_copy_tab_buffer( line_list_ptr ll, TDE_WIN *window )
{
text_ptr p;
int  len;               /* length of current line buffer text */
int  net_change;
int  rc;
file_infos *file;
TDE_WIN *wp;

   rc = OK;
   file = window->file_info;
   /*
    * file has changed.  lets create the back_up if needed
    */
   if (mode.do_backups == TRUE) {
      window->file_info->modified = TRUE;
      rc = backup_file( window );
   }

   len = g_status.tabout_buff_len;

   assert( len >= 0 );
   assert( len < MAX_LINE_LENGTH );
   assert( ll->len >= 0 );
   assert( ll->len < MAX_LINE_LENGTH );

   /*
    * if the FAR heap has run out of space, then only part of the
    *  current line can be moved back into the FAR heap. Warn the user
    *  that some of the current line has been lost.
    */
   p = my_malloc( len, &rc );
   if (rc == ERROR)
      error( WARNING, window->bottom_line, main4 );

   else {
      net_change = len - ll->len;

      my_free( ll->line );
      my_memcpy( p, g_status.line_buff, len );
      ll->line = p;
      ll->len  = len;

      if (net_change != 0) {
         for (wp=g_status.window_list; wp != NULL; wp=wp->next) {
            if (wp->file_info == file && wp != window)
               if (wp->rline > window->rline)
                  wp->bin_offset += net_change;
         }
      }

      file->modified = TRUE;
   }
   return( rc );
}
#endif


/*
 * Name:    set_prompt
 * Purpose: To display a prompt, highlighted, at the bottom of the screen.
 * Date:    October 1, 1989
 * Passed:  prompt: prompt to be displayed
 *          line:   line to display prompt
 * jmh 980726: changed eol_clear to text color rather than message.
 */
void set_prompt( const char *prompt, int line )
{
register int prompt_col;

   /*
    * work out where the answer should go
    */
   prompt_col = strlen( prompt );

   /* jmh 981002: truncate the message if it's too long, instead of failing an
    *             assertion.
    * jmh 030325: since it's now constant, and s_output will truncate anyway,
    *              simply change the col.
    */
   if (prompt_col > g_display.ncols - 1)
      prompt_col = g_display.ncols - 1;

   /*
    * output the prompt
    */
   s_output( prompt, line, 0, Color( Message ) );
   eol_clear( prompt_col, line, Color( Text ) );

   /*
    * put cursor at end of prompt
    */
   xygoto( prompt_col, line );
   refresh( );
}


/*
 * Name:    show_eof
 * Purpose: display eof message
 * Date:    September 16, 1991
 * Notes:   line:  ususally, line to display is "<=== eof ===>"
 *
 * jmh 980702: also display the top of file message
 * jmh 980703: cover the entire window
 */
void show_eof( TDE_WIN *window )
{
register int col;
text_t temp[MAX_COLS+2];
unsigned char attr[MAX_COLS+2];
const char *msg;
int  wid, len;
int  j;
int  spaces;
int  start_col;

   start_col = window->left;
   wid = window->end_col + 1 - start_col;
   memset( temp, EOF_CHAR, wid );
   memset( attr, Color( EOF ), wid );
   spaces = (wid > 30) ? 3 : (wid > 20) ? 2 : 1;
   msg = eof_text[(window->rline != 0)];
   len = strlen( msg );
   col = (wid - len) / 2;
   memcpy( temp + col, msg, len );
   for (j = 0; j < spaces; ++j)
      temp[col-1-j] = temp[col+len+j] = ' ';

   if (ruler_win.rline != -1) {
      if (window->cline == ruler1 || window->cline == ruler2 ||
          window->rline == ruler_win.rline)
         make_popup_ruler( window, temp, attr, wid, ruler1, ruler2 );
   }

   display_line( temp, attr, wid, window->cline, start_col );
}


/*
 * Name:    show_line_numbers
 * Class:   helper function
 * Purpose: display just the line numbers for the popup ruler
 * Author:  Jason Hood
 * Date:    July 20, 2005
 * Passed:  window:  window being displayed
 * Notes:   rline is assumed to be at the top line of the window.
 */
static void show_line_numbers( TDE_WIN *window )
{
long n;
int  line;
char buf[16];

   line = window->top_line;
   n = window->rline;
   if (n == 0) {
      ++n;
      ++line;
   }
   for (; line <= window->bottom_line; ++line) {
      sprintf( buf, "%*ld", window->file_info->len_len - 1,
                            labs( n - ruler_win.rline ) + 1 );
      s_output( buf, line, window->left, Color( Ruler ) );
      if (++n > window->file_info->length)
         break;
   }
}


/*
 * Name:    display_current_window
 * Purpose: display text in current window
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   use a temporary window structure, "w", to do the dirty work.
 *
 * jmh 991126: use normal syntax coloring to blank window, if appropriate.
 * jmh 050720: special processing for the popup ruler.
 */
void display_current_window( TDE_WIN *window )
{
register int count;     /* number of lines in window */
register int number;    /* number of text lines to display */
TDE_WIN w;              /* scratch window structure */
int  curl;              /* current line on screen, window->cline */
int  col;               /* color of blank lines */
int  below;             /* is the ruler below the line being measured? */
int  hyt;

   /*
    * jmh 980801: reset the current line count, since it's not required.
    */
   window->cur_count = 0;

   /*
    * initialize the scratch variables
    */
   dup_window_info( &w, window );
   count = w.cline - w.top_line;
   if (count > 0) {
      w.cline -= count;
      w.rline -= count;
      do
         w.ll = w.ll->prev;
      while (--count != 0);
   }
   /*
    * start at the top of the window and display a window full of text
    */
   curl  = (swap_er == -1) ? window->cline : -1;
   count = number = hyt = w.bottom_line - w.top_line;
   if (w.rline + number > w.file_info->length) {
      number = (int)(w.file_info->length - w.rline);
      hyt = number + 1;
   }
   count -= number;

   /*
    * determine what the popup ruler wants displayed
    */
   if (ruler_win.rline != -1) {
      ruler1 = ruler2 = curl = -1;
      if (ruler_win.rline >= w.rline && ruler_win.rline <= w.rline + hyt) {
         ruler1 = w.top_line + (int)(ruler_win.rline - w.rline);
         if (ruler1 == w.top_line || (ruler1 == w.top_line+1 && hyt != 1)) {
            below = TRUE;
            ++ruler1;
            ruler2 = ruler1 + (ruler1 < w.top_line + hyt);
         } else {
            below = FALSE;
            --ruler1;
            ruler2 = ruler1 - (ruler1 > w.top_line);
         }
         if (w.file_info->dirty & RULER) {
            if ((w.file_info->dirty & GLOBAL)  &&  mode.line_numbers)
               show_line_numbers( &w );
            while ((!below && w.cline != ruler2)  ||
                    (below && w.rline != ruler_win.rline)) {
               ++w.cline;
               ++w.rline;
               w.ll = w.ll->next;
            }
            if (w.file_info->dirty & GLOBAL) {
               if (w.file_info->dirty & LOCAL) {
                  if (below  &&  (ruler1 == w.top_line + 2  ||
                      (ruler_win.rline == 2 && ruler1 == w.top_line + 3))  &&
                      w.rline > 0) {
                     --w.cline;
                     --w.rline;
                     w.ll = w.ll->prev;
                  }
               } else /* (w.file_info->dirty & NOT_LOCAL) */ {
                  if (w.cline != w.top_line  &&  w.rline > 0) {
                     --w.cline;
                     --w.rline;
                     w.ll = w.ll->prev;
                  }
               }
               number = 3;
            } else
               number = 2;
            if (number > w.bottom_line - w.cline)
               number = w.bottom_line - w.cline;
            count = 0;
            if (w.rline + number > w.file_info->length) {
               number = (int)(w.file_info->length - w.rline);
               count = 1;
            }
         }
      }
   }

   if (w.rline == 0) {
      show_eof( &w );
      ++w.cline;
      ++w.rline;
      w.ll = w.ll->next;
      --number;
   }
   for (; number >= 0; number--) {
      if (w.cline == curl)
         show_curl_line( window );
      else
         update_line( &w );
      ++w.cline;
      ++w.rline;
      w.ll = w.ll->next;
   }
   if (count > 0) {
      show_eof( &w );
      col = (window->syntax) ? syntax_color[0] : Color( Text );
      while (--count != 0) {
         ++w.cline;
         window_eol_clear( &w, col );
      }
   }
   show_asterisk( window );
}


/*
 * Name:    redraw_screen
 * Purpose: display all visible windows, modes, and headers
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 *
 * jmh 991126: redraw all windows in one loop.
 */
int  redraw_screen( TDE_WIN *window )
{
register TDE_WIN *wp;           /* window to redraw */

#if defined( __UNIX__ )
   cls( );
#endif

   for (wp = g_status.window_list; wp; wp = wp->next)
      redraw_current_window( wp );

   window->file_info->dirty = FALSE;
   show_modes( );

   refresh( );

#if defined( __WIN32__ )
   show_window_fname( window );         /* console title */
#endif

   return( OK );
}


/*
 * Name:    redraw_current_window
 * Purpose: redraw all info in window
 * Date:    July 13, 1991
 * Passed:  window:  pointer to current window
 */
void redraw_current_window( TDE_WIN *window )
{
   if (window->visible) {
      show_window_header( window );
      show_ruler( window );
      show_ruler_pointer( window );
      display_current_window( window );
      if (window->vertical)
         show_vertical_separator( window );
      if (g_status.command != SizeWindow)
         show_tab_modes( );
   }
}


/*
 * Name:    show_changed_line
 * Purpose: Only one line was changed in file, just show it
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 *
 * jmh 980728: Even though only one line has been changed, that change may
 *              affect other lines' colors in the syntax highlighting.
 * jmh 981125: Corrected bugs with the above.
 */
void show_changed_line( TDE_WIN *window )
{
TDE_WIN *above;                 /* window above current */
TDE_WIN *below;                 /* window below current */
TDE_WIN w;                      /* scratch window structure */
long changed_line;              /* line number in file that was changed */
long top_line, bottom_line;     /* top and bottom line in file on screen */
int  line_on_screen;            /* is changed line on screen? */
file_infos *file;               /* file pointer */
long count;                     /* number of additional lines to display */
long rline;                     /* original rline in other window */
int  i;

   file  = window->file_info;
   count = syntax_check_lines( window->ll, file->syntax );

   if (window->visible) {
      window->cur_count = window->bottom_line - window->cline;
      if (window->cur_count > count)
         window->cur_count = (int)count;
      if (file->dirty & LOCAL)
         show_curl_line( window );
   }

   /*
    * now update the line in all other windows
    */
   if (file->dirty & NOT_LOCAL) {
      changed_line = window->rline;
      above = window->prev;
      below = window->next;
      while (above || below) {
         if (above) {
            dup_window_info( &w, above );
            above = above->prev;
         } else if (below) {
            dup_window_info( &w, below );
            below = below->next;
         }

         /*
          * is this window the changed file and is it visible?
          */
         if (w.file_info == file && w.visible) {

            /*
             * calculate file lines at top and bottom of screen.
             * the changed line may not be the curl in other windows.
             */
            line_on_screen = FALSE;
            rline = 0;
            top_line    = w.rline - (w.cline - w.top_line);
            bottom_line = w.rline + (w.bottom_line - w.cline);
            if (changed_line == w.rline)
               line_on_screen = CURLINE;
            else if (changed_line < w.rline &&
                     changed_line + count >= top_line) {
               line_on_screen = NOTCURLINE;
               rline = w.rline;
               while (w.rline > changed_line && w.rline > top_line) {
                  w.ll = w.ll->prev;
                  --w.rline;
                  --w.cline;
               }
               count -= w.rline - changed_line;
            } else if (changed_line > w.rline && changed_line <= bottom_line) {
               line_on_screen = NOTCURLINE;
               while (w.rline < changed_line) {
                  w.ll = w.ll->next;
                  ++w.rline;
                  ++w.cline;
               }
            }

            /*
             * display the changed line if on screen
             */
            if (line_on_screen) {
               if (line_on_screen == NOTCURLINE)
                  update_line( &w );
               else
                  show_curl_line( &w );

               i = w.bottom_line - w.cline;
               if (i > count)
                  i = (int)count;
               while (i != 0) {
                  w.ll = w.ll->next;
                  ++w.rline;
                  ++w.cline;
                  if (w.rline == rline)
                     show_curl_line( &w );
                  else
                     update_line( &w );
                  --i;
               }
            }
         }
      }
   }
   file->dirty = FALSE;
}


/*
 * Name:    show_curl_line
 * Purpose: show current line in curl color
 * Date:    January 16, 1992
 * Passed:  window:  pointer to current window
 * jmh 980724: use a global variable to indicate the current line
 * jmh 980801: display the lines changed by syntax highlighting (caused by
 *              the NOT_LOCAL update in show_changed_line).
 * jmh 991031: don't use current line when swapping.
 * jmh 050720: moved above test into display_current_window.
 */
void show_curl_line( TDE_WIN *window )
{
TDE_WIN w;
register int cur_count;

   if (window->visible  &&  g_status.screen_display) {
      g_status.cur_line = TRUE;
      update_line( window );
      g_status.cur_line = FALSE;
      cur_count = window->cur_count;
      if (cur_count > 0) {
         dup_window_info( &w, window );
         do {
            w.ll = w.ll->next;
            ++w.rline;
            ++w.cline;
            update_line( &w );
         } while (--cur_count != 0);
         window->cur_count = 0;
      }
   }
}


/*
 * Name:    update_line
 * Purpose: Display the current line in window
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   Show string starting at column zero and if needed blank rest
 *           of line.
 * jmh:     September 8, 1997 - added syntax highlighting.
 * jmh 980725: added not-eol display (ie. line extends beyond window edge)
 * jmh 980816: display break point in the hilited file color.
 * jmh 991027: combined all display routines into one, moved to utils.c.
 * jmh 991028: display the swap region.
 * jmh 991108: display line number (using ruler coloring).
 * jmh 030330: fix bug with line number.
 * jmh 031027: break point and swap have their own colors.
 */
void update_line( TDE_WIN *window )
{
file_infos *file;
line_list_ptr ll;   /* current line being displayed */
text_ptr text;      /* position to begin display    */
int  len;
long rline;
long br;
text_t line_char[MAX_COLS];                 /* characters to display */
unsigned char line_attr[MAX_LINE_LENGTH+8]; /* color of every char. in line */
text_ptr lchar;
unsigned char *lattr;                   /* color of every char. on-screen */
int  blank = 0;                         /* color to blank out the screen line */
int  block;
int  max_col;
int  block_line;
int  line_block;
int  break_point;
int  show_eol;
int  show_neol;
int  bc;
int  ec;
int  swap_line;
int  line_swap;
int  nlen;

   ll = window->ll;
   if (ll->len == EOF || !g_status.screen_display)
      return;

   file    = window->file_info;
   rline   = window->rline;
   max_col = window->end_col + 1 - window->start_col;
   block   = file->block_type;
   br      = file->block_br;
   block_line  = (block > NOTMARKED && rline >= br && rline <= file->block_er);
   line_block  = ((block == LINE && block_line) ||
                  (block == STREAM &&
                   ((rline > br && rline < file->block_er) ||
                    (rline == file->block_er && file->block_ec == -1 &&
                     rline != file->block_br))));
   break_point = (rline == file->break_point);
   swap_line   = (rline <= swap_er && rline >= swap_br);
   line_swap   = (swap_line && swap_bc == -1);

   /*
    * figure which line to display.
    * actually, we could be displaying any line in any file.  we display
    *  the line_buffer only if window->ll == g_status.buff_node
    *  and window->ll->line has been copied to g_status.line_buff.
    */
   if (g_status.copied && ll == g_status.buff_node) {
      text = g_status.line_buff;
      len  = g_status.line_buff_len;
   } else {
      text = ll->line;
      len  = ll->len;
   }
   if (file->inflate_tabs)
      text = tabout( text, &len, file->ptab_size, mode.show_eol );

   nlen  = (mode.line_numbers) ? file->len_len : 0;
   lchar = line_char + nlen;
   lattr = line_attr + nlen;

   if (window->syntax && !line_block && !break_point && !line_swap)
      blank = syntax_attr( text, len, ll->type, lattr, file->syntax );

   /*
    * lets look at the base column.  if the line to display is shorter
    *  than the base column, then set text to eol and we can't see the
    *  eol either.
    */
   show_eol = (mode.show_eol == 1);
   bc = window->bcol;
   if (bc > 0) {
      text  += bc;
      lattr += bc;
      len   -= bc;
      if (len < 0) {
         len = 0;
         show_eol = FALSE;
      }
   }
   /*
    * for display purposes, set the line length to screen width if line
    *  is longer than screen.
    */
   show_neol = (mode.show_eol == 2 && len > max_col);
   if (len > max_col)
      len = max_col;

   memset( line_char, ' ', nlen + max_col );
   my_memcpy( lchar, text, len );

   if (show_eol) {
      if (len < max_col)
         lchar[len] = EOL_CHAR;
   } else if (show_neol)
      lchar[max_col - 1] = NEOL_CHAR;

   if (nlen) {
      long n = rline;
      if (ruler_win.rline != -1)
         n = labs( n - ruler_win.rline ) + 1;
      my_ltoa( n, (char *)lchar - numlen( n ) - 1, 10 );
      lchar[-1] = HALF_SPACE;

      block = (g_status.cur_line) ? Color( Pointer ) : Color( Ruler );
      memset( lattr - nlen, block, nlen - 1 );
      bc = (window->syntax) ? syntax_color[0]   /* COL_NORMAL */
                            : Color( Text );
      lattr[-1] = (block >> 4) | (bc & 0xf0);
   }

   block = Color( Block );

   if (line_block)
      memset( lattr, block, max_col );

   else if (line_swap)
      memset( lattr, Color( Swap ), max_col );

   else {
      if (window->syntax && !break_point)
         memset( lattr + len, blank, max_col - len );
      else {
         blank = (break_point) ?        Color( BP )    :
                 (g_status.cur_line &&
                  !mode.cursor_cross) ? Color( Curl )  :
                 (ll->type & DIRTY)   ? Color( Dirty ) :
                                        Color( Text );
         memset( lattr, blank, max_col );
      }
      if (block_line) {
         bc = file->block_bc;
         ec = file->block_ec;
         if (file->block_type == STREAM) {
            if (rline == br && (br != file->block_er || ec == -1))
               ec = bc + max_col;
            else if (rline == file->block_er && br != file->block_er)
               bc = 0;
         }
         memset( line_attr+nlen + bc, block, ec - bc + 1 );
      }
      if (swap_line)
         memset( line_attr+nlen + swap_bc, Color( Swap ),
                 swap_ec - swap_bc + 1 );
   }

   if (found_rline == rline)
      memset( line_attr+nlen + found_rcol, Color( Help ), found_vlen );

   /*
    * Popup ruler
    */
   if (ruler_win.rline != -1) {
      if (window->cline == ruler1 || window->cline == ruler2 ||
          window->rline == ruler_win.rline)
         make_popup_ruler( window, lchar, lattr, max_col, ruler1, ruler2 );
   }

   /*
    * Set the cursor cross.
    */
   else if (mode.cursor_cross) {
      blank = Color( Cross );
      line_attr[nlen + window->rcol] ^= blank;
      if (g_status.cur_line) {
         bc = max_col;
         do
            lattr[--bc] ^= blank;
         while (bc != 0);
      }
   }

   display_line( line_char, lattr - nlen, nlen + max_col,
                 window->cline, window->left );
}


/*
 * Name:    dup_window_info
 * Purpose: Copy window info from one window pointer to another
 * Date:    June 5, 1991
 * Passed:  dw: destination window
 *          sw: source window
 */
void dup_window_info( TDE_WIN *dw, TDE_WIN *sw )
{
   memcpy( dw, sw, sizeof( TDE_WIN ) );
}


/*
 * Name:    adjust_windows_cursor
 * Purpose: A change has been made, make sure pointers are not ahead of
 *           or behind file.
 * Date:    June 5, 1991
 * Passed:  window:       pointer to current window
 *          line_change:  number of lines add to or subtracted from file
 * Notes:   If a file has been truncated in one window and there is another
 *           window open to the same file and its current line is near the
 *           end, the current line is reset to the last line of the file.
 * jmh 991126: adjust other windows' rline (keep the same line, not the same
 *              line number);
 *             corrected other windows' bin_offset value (have to recalculate
 *              from the beginning).
 */
void adjust_windows_cursor( TDE_WIN *window, long line_change )
{
register TDE_WIN *next;
file_infos *file;
long rline;
long length;
long i;
MARKER *marker;
int  ndiff;

   if (line_change == 0)
      return;

   file   = window->file_info;
   rline  = window->rline;
   length = file->length;

   for (next = g_status.window_list; next != NULL; next = next->next) {
      if (next->file_info == file  &&  next != window  &&
          next->rline > rline) {
         i = next->rline + line_change;
         if (i > length + 1)
            i = length + 1;
         first_line( next );
         move_to_line( next, i, TRUE );
         check_cline( next, next->cline );
         file->dirty = NOT_LOCAL;
      }
   }

   /*
    * jmh 050715: doing a block delete or move requires using the block line.
    */
   if (g_status.command == DeleteBlock || g_status.command == MoveBlock)
      rline = file->block_er;

   /*
    * now adjust any markers.
    */
   for (i = 0; i < NO_MARKERS; i++) {
      marker = &file->marker[ (int) i ];
      if (marker->rline > rline) {
         marker->rline += line_change;
         if (marker->rline < 1L)
            marker->rline = 1L;
         else if (marker->rline > length)
            marker->rline = length;
      }
   }

   /*
    * adjust the break point.
    */
   if (file->break_point > rline) {
      file->break_point += line_change;
      if (file->break_point < 1L || file->break_point > length)
         file->break_point = 0;
   }

   /*
    * check if the width of the line numbers have changed.
    */
   if (mode.line_numbers) {
      ndiff = numlen( length ) + 1 - file->len_len;
      if (ndiff != 0) {
         file->len_len += ndiff;
         for (next = g_status.window_list; next != NULL; next = next->next) {
            if (next->file_info == file) {
               next->start_col += ndiff;
               next->ccol      += ndiff;
               if (next->ccol > next->end_col)
                  next->ccol = next->end_col;
            }
         }
         file->dirty = GLOBAL;
      }
   }
}


/*
 * Name:    first_non_blank
 * Purpose: To find the column of the first non-blank character
 * Date:    June 5, 1991
 * Passed:  s:    the string to search
 *          len:  length of string
 *          tabs: do tabs count as blanks?
 *          tab_size: size of tabs
 * Returns: the first non-blank column
 */
int  first_non_blank( text_ptr s, int len, int tabs, int tab_size )
{
register int count = 0;

   if (len > 0) {
      if (tabs) {
         do {
            if (*s == ' ')
               ++count;
            else if (*s == '\t')
               count += tab_size - (count % tab_size);
            else
               break;
            s++;
         } while (--len != 0);
      } else {
         while (*s++ == ' '  &&  --len != 0)
            ++count;
      }
   }
   return( count );
}


/*
 * Name:    find_end
 * Purpose: To find the last character in a line
 * Date:    October 31, 1992
 * Passed:  s:    the string to search
 *          len:  length of string
 *          tabs: do tabs count as blanks?
 *          tab_size: size of tabs
 * Returns: the column of the last character
 */
int  find_end( text_ptr s, int len, int tabs, int tab_size )
{
register int count = 0;

   if (len > 0) {
      if (tabs) {
         do {
            if (*s++ == '\t')
               count += tab_size - (count % tab_size);
            else
               ++count;
         } while (--len != 0);
      } else
         count = len;
   }
   return( count );
}


/*
 * Name:    is_line_blank
 * Purpose: is line empty or does it only contain spaces?
 * Date:    November 28, 1991
 * Passed:  s:    the string to search
 *          len:  length of string
 *          tabs: do tabs count as spaces?
 * Returns: TRUE if line is blank or FALSE if something is in line
 */
int is_line_blank( text_ptr s, int len, int tabs )
{
   if (len > 0) {
      if (tabs)
         while ((*s == ' ' || *s == '\t')  &&  --len != 0)
            ++s;
      else
         while (*s == ' '  &&  --len != 0)
            ++s;
   } else
      len = 0;
   return( len == 0 );
}


/*
 * Name:    show_window_header
 * Purpose: show file stuff in window header
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   Clear line and display header in a lite bar
 */
void show_window_header( TDE_WIN *window )
{
register TDE_WIN *win;          /* put window pointer in a register */
int  line;

   win = window;
   line = win->cline;
   win->cline = win->top;
   window_eol_clear( win, Color( Head ) );
   win->cline = line;
   show_window_number_letter( win );
   show_window_fname( win );
   show_crlf_mode( win );
   show_size( win );
   show_line_col( win );
}


/*
 * Name:    show_window_number_letter
 * Purpose: show file number and letter of window in lite bar
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 */
void show_window_number_letter( TDE_WIN *window )
{
int  num;
char temp[4];
register TDE_WIN *win;   /* put window pointer in a register */

   win = window;
   num = win->file_info->file_no;
   if (num < 10) {
      temp[0] = ' ';
      temp[1] = '0' + num;
   } else {
      temp[0] = '0' + num / 10;
      temp[1] = '0' + num % 10;
   }
   temp[2] = win->letter;
   temp[3] = '\0';
   s_output( temp, win->top, win->left, Color( Head ) );
}


/*
 * Name:     show_window_fname
 * Purpose:  show file name in window header.
 * Date:     June 5, 1991
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen
 * Passed:   window:  pointer to current window
 * Notes:    Clear name field and display name in a lite bar
 *
 * jmh 980503: assume if the name is "" then it's a pipe and display "(pipe)"
 *     021105: made the length relative to the window size;
 *             display the attribute relative to the right edge.
 */
void show_window_fname( TDE_WIN *window )
{
char *p;
register TDE_WIN *win;          /* put window pointer in a register */
int  col;
int  wid;
char temp[PATH_MAX+2];

   win = window;
   col = win->left + 5;
   wid = win->end_col - col - 12;       /* line:col */
   if (!win->vertical)
      wid -= 17;                        /* attr crlf length */
   else if (wid < 1)
      return;

   assert( wid <= g_display.ncols );

   c_repeat( ' ', wid, col, win->top, Color( Head ) );

   if (win->title)
      strcpy( temp, win->title );
   else if (*(p = win->file_info->file_name) == '\0')
      strcpy( temp, p + 1 );
   else
      relative_path( temp, win, TRUE );

#if defined( __WIN32__ )
   {
   char title[8+PATH_MAX+2];
      join_strings( title, "TDE - ", temp );
      SetConsoleTitle( title );
   }
#endif

   p = temp;
   if (strlen( p ) > (size_t)wid) {
      if (win->vertical  &&  !win->title) {
         p = win->file_info->fname;
         if (strlen( p ) > (size_t)wid) {
            if (wid < 9)
               p = "";
            else
               p = reduce_string( temp, p, wid, MIDDLE );
         }
      } else
         reduce_string( p, p, wid, MIDDLE );
   }
   s_output( p, win->top, col, Color( Head ) );

   if (!win->vertical) {
      s_output( str_fattr( temp, win->file_info->file_attrib ),
                win->top, win->end_col-28, Color( Head ) );
   }
}


/*
 * Name:     show_crlf_mode
 * Purpose:  display state of crlf flag
 * Date:     June 5, 1991
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen
 * jmh 991111: used an array.
 * jmh 021105: position relative to right edge.
 */
void show_crlf_mode( TDE_WIN *window )
{
   if (!window->vertical)
      s_output( crlf_mode[window->file_info->crlf],
                window->top, window->end_col-23, Color( Head ) );
}


/*
 * Name:    show_size
 * Purpose: show number of lines in file
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 *
 * jmh 021105: position relative to right edge.
 */
void show_size( TDE_WIN *window )
{
   if (!window->vertical  &&  window->file_info->crlf != BINARY)
      n_output( window->file_info->length, 7,
                window->end_col-18, window->top, Color( Head ) );
}


/*
 * Name:    quit
 * Purpose: To close the current window without saving the current file.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   If the file has been modified but not saved, then the user is
 *           given a second chance before the changes are discarded.
 *          Note that this is only necessary if this is the last window
 *           that refers to the file.  If another window still refers to
 *           the file, then the check can be left until later.
 *          jmh - if output has been redirected then quitting will send
 *                 nothing - file (function file_file) should be used instead.
 *                 This is so windows can be closed without redirection taking
 *                 place multiple times.
 *
 * jmh 991121: Frank only counted visible windows, which means if a window is
 *              split and then zoomed, closing one would close both. I don't
 *              like this, so just use the reference count.
 */
int  quit( TDE_WIN *window )
{
int  rc = OK;

   if (window->file_info->modified && window->file_info->ref_count == 1  &&
       !window->file_info->scratch) {
      /*
       * abandon changes?
       */
      if (get_yn( utils12, window->bottom_line, R_PROMPT | R_ABORT ) != A_YES)
         rc = ERROR;
   }

   /*
    * remove window, allocate screen lines to other windows etc
    */
   if (rc == OK)
      finish( window );
   return( OK );
}


/*
 * Name:     quit_all
 * Purpose:  To abandon all windows and leave the editor
 * Author:   Jason Hood
 * Date:     29 December, 1996
 * Modified: 27 August, 1997
 * Passed:   window:  pointer to current window
 * Notes:    if any window has been modified, prompt before leaving
 *
 * Change:   added disabled message during output redirection
 * 020911:   possibly save the workspace.
 * 040715:   no longer save the workspace.
 */
int  quit_all( TDE_WIN *window )
{
int  prompt_line;
register TDE_WIN *wp;
int  mod = FALSE;
int  rc  = A_YES;

   prompt_line = g_display.end_line;
   if (g_status.output_redir) {
      error( WARNING, prompt_line, main22 );
      return( ERROR );
   }

   for (wp=g_status.window_list; wp != NULL; wp=wp->next) {
      if (wp->file_info->modified && !wp->file_info->scratch) {
         mod = TRUE;
         break;
      }
   }
   if (mod) {
      /*
       * abandon all files?
       */
      rc = get_yn( utils12a, prompt_line, R_PROMPT | R_ABORT );
   }
   if (rc == A_YES)
      g_status.stop = TRUE;

   return( OK );
}


/*
 * Name:     date_time_stamp
 * Purpose:  put system date and time into file at cursor position
 * Date:     June 5, 1992
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen
 * Passed:   window:  pointer to current window
 *
 * jmh 980702: test for EOF
 * jmh 991126: if EOF or read-only, display anyway, but return ERROR.
 */
int  date_time_stamp( TDE_WIN *window )
{
char answer[MAX_COLS+2];

   format_time( mode.stamp, answer, NULL );
   if (window->ll->len == EOF  ||  window->file_info->read_only) {
      error( INFO, window->bottom_line, answer );
      return( ERROR );
   }
   return( add_chars( (text_ptr)answer, window ) );
}


/*
 * Name:     stamp_format
 * Purpose:  to set a format for the date and time stamp
 * Date:     May 21, 1998
 * Author:   Jason Hood
 * Passed:   window:  pointer to current window
 */
int  stamp_format( TDE_WIN *window )
{
register TDE_WIN *win;   /* put window pointer in a register */
char answer[MAX_COLS+2];

   win = window;
   if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
      return( ERROR );

   strcpy( answer, mode.stamp );
   /*
    * stamp format (F1 = help):
    */
   if (get_name( utils17, g_display.end_line, answer, &h_stamp ) <= 0)
      return( ERROR );

   strcpy( mode.stamp, answer );
   return( OK );
}


/*
 * Name:    add_chars
 * Purpose: insert string into file
 * Date:    June 5, 1992
 * Passed:  string:  string to add to file
 *          window:  pointer to current window
 *
 * jmh 980524: treat the tab character as though the key was pressed.
 * jmh 980526: treat the newline character as the Rturn function.
 * jmh 980728: test rc in the while.
 * jmh 020913: if called from PasteFromClipboard use tab chars, not the func.
 * jmh 050816: PasteFromClipboard will wrap at max (well, usually).
 */
int  add_chars( text_ptr string, TDE_WIN *window )
{
int  rc = OK;

   while (*string && rc == OK) {
      if (*string == '\t' && g_status.command != PasteFromClipboard)
         rc = tab_key( window );
      else if (*string == '\n')
         rc = insert_newline( window );
      else {
         g_status.key_pressed = *string;
         if (g_status.command == PasteFromClipboard  &&
             window->rcol >= MAX_LINE_LENGTH - 2) {
            rc = mode.right_justify;
            mode.right_justify = FALSE;
            g_status.command = FormatText;
            word_wrap( window );
            mode.right_justify = rc;
            g_status.command = PasteFromClipboard;
         }
         rc = insert_overwrite( window );
      }
      ++string;
   }
   return( rc );
}


/*
 * Name:    shell
 * Purpose: shell to DOS (or whatever)
 * Author:  Jason Hood
 * Date:    November 27, 1996
 * Modified: August 28, 1997 - tested for redirection
 * Passed:  window:  pointer to current window
 * Notes:   the video mode should not be changed on return to TDE
 *
 * jmh 990410: UNIX: use SHELL environment (or "/bin/sh") instead of "".
 * jmh 021023: restore overscan color.
 * jmh 030320: test if files have been modified.
 * jmh 031026: Execute function will prompt for command.
 * jmh 031029: test for different timestamps, not just newer.
 */
int  shell( TDE_WIN *window )
{
char *cmd, *pos, *name;
char answer[PATH_MAX+2];
char fname[PATH_MAX];
int  quote;
int  capture, pause, quiet, save;
int  fh, fh1, fh2;
int  output_reloaded, old_reload;
int  rc;
TDE_WIN *win;
TDE_WIN *out_win;
file_infos *fp;
ftime_t ftime;

   if (g_status.command == Execute) {
      if (do_dialog( exec_dialog, exec_proc ) == ERROR)
         return( ERROR );
      cmd = get_dlg_text( EF_Command );
      pos = answer;
      while (*cmd) {
         if (*cmd != '%')
            *pos++ = *cmd;
         else {
            if (cmd[1] == '=') {
               ++cmd;
               quote = FALSE;
            } else
               quote = TRUE;
            switch (*++cmd) {
               case '%' :
                  *pos++ = '%';
                  break;

               case 'f' :
               case 'F' :
                  name = window->file_info->file_name;
                  if (*name == '\0') {
                     /*
                      * command requires a file
                      */
                     error( WARNING, g_display.end_line, utils19 );
                     return( ERROR );
                  }
                  if (*cmd == 'f')
                     name = relative_path( fname, window, FALSE );
                  if (quote) {
                     if (pos > answer && (pos[-1] == '\"' || pos[-1] == '\''))
                        quote = FALSE;
                     else
                        quote = (strpbrk( name, " +;,=%" ) != NULL);
                  }
                  if (quote)
                     *pos++ = '\"';
                  do {
#if !defined( __UNIX__ )
                     if (*name == '/')
                        *pos++ = '\\';
                     else
#endif
                     *pos++ = *name;
                  } while (*++name);
                  if (quote)
                     *pos++ = '\"';
                  break;

               case 'w' :
               case 'W' :
                  *pos = '\0';
                  rc = copy_word( window, pos,
                                  sizeof(answer) - (int)(pos - answer),
                                  (*cmd == 'W') );
                  if (rc == 0)
               case 'p' :
                  {
                     /*
                      * Enter parameter for command :
                      */
                     strcpy( pos, h_parm.prev->str );
                     rc = get_name( utils20, g_display.end_line, pos, &h_parm );
                     if (rc <= 0)
                        return( ERROR );
                  }
                  pos += rc;
                  break;
            }
         }
         ++cmd;
      }
      *pos = '\0';
      cmd  = answer;
      capture = CB_Capture;
      save = !CB_Original;
      auto_reload = CB_Reload;

   } else {
      capture = save = FALSE;
      cmd = "";
   }

   if (*cmd == '\0') {
      quiet = TRUE;
      pause = FALSE;
#if defined( __UNIX__ )
      if ((cmd = getenv( "SHELL" )) == NULL)
         cmd = "/bin/sh";
#elif defined( __MSC__ )
      cmd = getenv( "COMSPEC" );
#elif defined( __WIN32__ )
      cmd = NULL;
#endif
   } else {
      quiet =  CB_NEcho;
      pause = !CB_NPause;
   }

   if (un_copy_line( window->ll, window, TRUE, TRUE ) == ERROR)
      return( ERROR );
   if (save) {
      if (save_all( window ) == ERROR)
         return( ERROR );
   }

   console_suspend( );
   if (!quiet) {
      putchar( '[' );
      if (capture)
         putchar( OUTPUT );
      fputs( "tde] ", stdout );
      puts( cmd );
   }
   if (capture) {
      fh1  = dup( 1 );                  /* standard output */
      fh2  = dup( 2 );                  /* standard error */
      name = tmpnam( NULL );            /* unique temporary filename */
      fh   = creat( name, 0200 );       /* S_IWUSR - don't make read-only */
      dup2( fh, 1 );
      dup2( fh, 2 );
   }
#if defined( __GNUC__ )
   /* prevent warnings */
   else {
      fh = fh1 = fh2 = 0;
      name = NULL;
   }
   out_win = NULL;
#endif

#if defined( __WIN32__ )
   if (cmd == NULL) {
      /*
       * running "cmd /c cmd" disables history for some reason, so use spawn,
       * but spawn won't work with redirected input, so use CreateProcess,
       * which disables history if it's not redirected, so now the input handle
       * is always created.  Weird.
       */
      STARTUPINFO si;
      PROCESS_INFORMATION pi;
         memset( &si, 0, sizeof(si) );
         si.cb = sizeof(si);
         if (CreateProcess( NULL, getenv( "COMSPEC" ), NULL, NULL, TRUE, 0,
                            NULL, NULL, &si, &pi))
            WaitForSingleObject( pi.hProcess, INFINITE );
   }
   else
#endif
   system( cmd );

   if (capture) {
      close( fh );
      dup2( fh1, 1 );
      dup2( fh2, 2 );
      close( fh1 );
      close( fh2 );
   }
   console_resume( pause );

   if (g_display.adapter != MDA)
      set_overscan_color( Color( Overscan ) );
   redraw_screen( window );

#if defined( __MSDOS__ )
   /*
    * Restore the correct cwd, since DOS maintains it globally (UNIX and
    *  Win32 maintain it per-process, so it's not needed there).
    */
   set_path( window );
#endif

   output_reloaded = FALSE;
   old_reload = auto_reload;
   for (fp = g_status.file_list; fp != NULL; fp = fp->next) {
      if (*fp->file_name == '\0') {
         rc = FALSE;
         if (capture && !output_reloaded) {
            if (strcmp( fp->file_name + 2, cmd ) == 0) {
               output_reloaded = rc = TRUE;
               strcpy( fp->file_name, name );
               old_reload  = auto_reload;
               auto_reload = TRUE;
            }
         }
      } else if (get_ftime( fp->file_name, &ftime ) == OK) {
#if defined( __WIN32__ )
         rc = (CompareFileTime( &ftime, &fp->ftime ) != 0);
#else
         rc = (ftime != fp->ftime);
#endif
      } else
         rc = FALSE;
      if (rc) {
         win = find_file_window( fp );
         rc = reload_file( win );

         if (output_reloaded == TRUE) {
            ++output_reloaded;
            auto_reload = old_reload;
            fp->file_attrib = 0;
            fp->file_name[0] = '\0';
            fp->file_name[1] = OUTPUT;
            strcpy( fp->file_name + 2, cmd );
            out_win = win;
            if (rc == OK)
               unlink( name );
         }
      }
   }
   auto_reload = FALSE;
   if (capture) {
      if (output_reloaded)
         change_window( window, out_win );
      else if (attempt_edit_display( name, LOCAL ) == OK) {
         unlink( name );
         g_status.current_file->scratch = -1;
         g_status.current_file->file_attrib = 0;
         g_status.current_file->file_name[0] = '\0';
         g_status.current_file->file_name[1] = OUTPUT;
         strcpy( g_status.current_file->file_name + 2, cmd );
         show_window_fname( g_status.current_window );
      }
   }

   return( OK );
}


/*
 * Name:    exec_proc
 * Purpose: dialog callback for Execute
 * Author:  Jason Hood
 * Date:    November 30, 2003
 * Notes:   turning on Capture will turn on and disable No pause;
 *          a blank command will turn on and disable No echo and No pause.
 */
int  exec_proc( int id, char *text )
{
static int  necho  = ERROR;     /* remember the old echo state */
static int  npause = ERROR;     /* remember the old pause state */

   if (id == -IDE_COMMAND) {
      if (*text == '\0') {
         if (necho == ERROR) {
            necho = CB_NEcho;
            if (!necho)
               check_box( IDC_NECHO );
            check_box_enabled( IDC_NECHO, FALSE );
         }
         if (npause == ERROR) {
            npause = CB_NPause;
            if (!npause)
               check_box( IDC_NPAUSE );
            check_box_enabled( IDC_NPAUSE, FALSE );
         }
      } else {
         check_box_enabled( IDC_NECHO, TRUE );
         if (necho == FALSE)
            check_box( IDC_NECHO );
         necho = ERROR;
         if (!CB_Capture) {
            check_box_enabled( IDC_NPAUSE, TRUE );
            if (npause == FALSE)
               check_box( IDC_NPAUSE );
            npause = ERROR;
         }
      }

   } else if (id == IDC_CAPTURE) {
      if (necho == ERROR) { /* empty command will have already done pause */
         if (CB_Capture) {
            npause = CB_NPause;
            if (!npause)
               check_box( IDC_NPAUSE );
            check_box_enabled( IDC_NPAUSE, FALSE );
         } else if (npause != ERROR) {
            check_box_enabled( IDC_NPAUSE, TRUE );
            if (!npause)
               check_box( IDC_NPAUSE );
            npause = ERROR;
         }
      }
   }

   return( OK );
}


/* Name:    user_screen
 * Purpose: to display the screen prior to running TDE, or from the shell
 * Author:  Jason Hood
 * Date:    December 28, 1996
 * Passed:  window:  pointer to current window
 * Notes:   DOS only
 * jmh 990418: added Linux version
 * jmh 031125: restore overscan color
 */
int  user_screen( TDE_WIN *window )
{
   page( 0 );
   if (g_display.adapter != MDA)
      set_overscan_color( g_display.old_overscan );
   getkey( );
#if defined( __UNIX__ )
   redraw_screen( window );
#else
   page( 1 );
# if defined( __WIN32__ )
   show_window_fname( window );
# endif
#endif
   if (g_display.adapter != MDA)
      set_overscan_color( Color( Overscan ) );
   return( OK );
}


static void create_stat( const char *, char *,
                         char [][STAT_WIDTH+1], char [], int );

/*
 * Name:    show_status
 * Purpose: display markers, block and break point settings
 * Author:  Jason Hood
 * Date:    April 10, 1999
 * Passed:  window:  pointer to current window
 * Notes:   write to a temporary buffer first to find the largest string,
 *           then write them centered in the status_screen.
 *
 * 990425:  display filename.
 * 990503:  display file size and size in memory (not including window or
 *            syntax info, but including file info).
 * 991021:  create the screen in memory, using frame style.
 *          Assumes no line will be greater than STAT_WIDTH (other than the
 *            filename).
 * 991028:  Corrected errors I introduced above.
 * 991111:  Display line ending in brackets after file size.
 * 991120:  Display "(pipe)" when appropriate.
 * 991126:  Include window info.
 * 010605:  Moved sizes to show_statistics();
 *          added syntax highlighting info.
 * 030226:  Display "(scratch)" when appropriate.
 * 030320:  Added file time and current time, using a different format.
 */
int  show_status( TDE_WIN *window )
{
char buf[STAT_COUNT+1][STAT_WIDTH+1];
char blen[STAT_COUNT+1];
int  len;
file_infos *file = window->file_info;
int  j;
int  o = 0;
long lines;

   memset( blen, 0, sizeof(blen) );

   blen[STAT_PREV] = sprintf( buf[STAT_PREV], status1b[file->marker[0].marked],
                              file->marker[0].rline, file->marker[0].rcol+1 );

   for (j = 1; j < NO_MARKERS; ++j) {
      blen[STAT_MARK+j] =
         sprintf( buf[STAT_MARK+j], status1a[file->marker[j].marked],
                  j, file->marker[j].rline, file->marker[j].rcol+1 );
   }

   if (!g_status.marked)
      blen[STAT_BLOCK] = sprintf( buf[STAT_BLOCK], status2[0] );
   else if (g_status.marked_file != file)
      blen[STAT_BLOCK] = sprintf( buf[STAT_BLOCK], status2[5],
                              status_block[g_status.marked_file->block_type] );
   else {
      if (file->block_type == LINE)
         len = sprintf( buf[STAT_BLOCK], status2[LINE],
                        file->block_br, file->block_er );
      else /* file->block_type == BOX or STREAM */
         len = sprintf( buf[STAT_BLOCK],
                        status2[file->block_type + (file->block_ec == -1)],
                        status_block[file->block_type],
                        file->block_br, file->block_bc+1,
                        file->block_er, file->block_ec+1 );
      blen[STAT_BLOCK] = len;

      len = 0;
      lines = file->block_er - file->block_br + (file->block_type != STREAM);
      if (file->block_type == BOX) {
         j = file->block_ec - file->block_bc + 1;
         if (j != 1 || lines != 1)
            len = sprintf( buf[STAT_BLOCK+1], status2b[0], lines,
                           status2a[lines == 1], j, status2a[2 + (j == 1)] );
      }
      else if (lines > 1) /* && file->block_type == LINE or STREAM */
         len = sprintf( buf[STAT_BLOCK+1], status2b[1], lines );
      if (len) {
         blen[STAT_BLOCK+1] = len;
         o = 1;
      } else
         buf[STAT_BLOCK][(int)blen[STAT_BLOCK]++] = '.';
   }

   blen[STAT_BREAK+o] = sprintf( buf[STAT_BREAK+o],
                                 status3[file->break_point != 0],
                                 file->break_point );

   blen[STAT_LANG+o] = sprintf( buf[STAT_LANG+o], status4[file->syntax != NULL],
                                file->syntax->name );

   blen[STAT_REF+o] = sprintf( buf[STAT_REF+o], status5[file->ref_count != 1],
                               file->ref_count );

   if (*file->file_name == '\0')
      *buf[0] = '\0';
   else
      format_time( stat_time, buf[0], ftime_to_tm( &file->ftime ) );
   blen[STAT_FTIME+o] = sprintf( buf[STAT_FTIME+o], status6, buf[0] );
   format_time( stat_time, buf[0], NULL );
   blen[STAT_TIME+o] = sprintf( buf[STAT_TIME+o], status7, buf[0] );

   create_stat( status0, file->file_name, buf, blen, STAT_COUNT+o );

   return( OK );
}


/*
 * Name:    show_statistics
 * Purpose: display certain statistics about the current file
 * Author:  Jason Hood
 * Date:    June 5, 2001
 * Passed:  window:  pointer to current window
 * Notes:   write to a temporary buffer first to find the largest string,
 *           then write them centered in the stat_screen.
 *          Include the EOF char in file size, if relevant.
 *
 * jmh 011128: modified memory calculation due to new management.
 * jmh 021102: fix display bug when max line length exceeds number of lines.
 * jmh 030329: fixed bug using wrong strings/counts.
 */
int  show_statistics( TDE_WIN *window )
{
char buf[STATS_COUNT][STAT_WIDTH+1];
char blen[STATS_COUNT];
int  len;
file_infos *file = window->file_info;
line_list_ptr ll;
text_ptr line;
long llen, flen, mlen;          /* line, file and memory lengths */
long lines, blank;              /* non-blank & blank lines */
long chars, spaces, strings;
long alnum, symbols, words;
int  line_len[3], word_len[3], string_len[3];
int  in_word, in_string;
int  sa[3], na[3];
int  pct[2];
int  j, k;

   if (un_copy_line( window->ll, window, TRUE, TRUE ) == ERROR)
      return( ERROR );

   show_search_message( TALLYING );

   #define STAT_LEN( stat ) \
      if (stat##_len[0] < stat##_len[1])\
         stat##_len[1] = stat##_len[0];\
      if (stat##_len[0] > stat##_len[2])\
         stat##_len[2] = stat##_len[0];\

   /*
    * For memory size, add all the line_list_struc's and the file info. The +2
    * is for the top and end of file markers; the 2+ is for length markers.
    */
   mlen = 2+sizeof(file_infos) + 2+sizeof(TDE_WIN)
          + (file->length+2) * (2+sizeof(line_list_struc));

   /*
    * Calculate the overall length of lines.
    */
   llen = flen = 0;
   blank = 0;
   chars = spaces  = strings = 0;
   alnum = symbols = words   = 0;
   line_len[0] = word_len[0] = string_len[0] = 0;                 /* current */
   line_len[1] = word_len[1] = string_len[1] = MAX_LINE_LENGTH;   /* shortest */
   line_len[2] = word_len[2] = string_len[2] = 0;                 /* longest */
   in_word = in_string = FALSE;
   for (ll = file->line_list->next; ll->len != EOF; ll = ll->next) {
      len = ll->len;
      if (len == 0)
         ++blank;
      else {
         mlen += (len <= 4) ? 6 : (len + 2);
         flen += len;
         line = ll->line;
         line_len[0] = find_end(line, len, file->inflate_tabs, file->ptab_size);
         llen += line_len[0];
         STAT_LEN( line );
         do {
            if (bj_isspace( *line )) {
               ++spaces;
               if (in_string) {
                  STAT_LEN( string );
                  string_len[0] = 0;
                  in_string = FALSE;
               }
               if (in_word) {
                  STAT_LEN( word );
                  word_len[0] = 0;
                  in_word = FALSE;
               }
            } else {
               ++chars;
               ++string_len[0];
               if (!in_string) {
                  ++strings;
                  in_string = TRUE;
               }
               if (myiswhitespc( *line )) {
                  ++symbols;
                  if (in_word) {
                     STAT_LEN( word );
                     word_len[0] = 0;
                     in_word = FALSE;
                  }
               } else {
                  ++alnum;
                  ++word_len[0];
                  if (!in_word) {
                     ++words;
                     in_word = TRUE;
                  }
               }
            }
            ++line;
         } while (--len != 0);
         if (in_string) {
            STAT_LEN( string );
            string_len[0] = 0;
            in_string = FALSE;
         }
         if (in_word) {
            STAT_LEN( word );
            word_len[0] = 0;
            in_word = FALSE;
         }
      }
   }
   if (strings == 0)
      line_len[1] = string_len[1] = 0;
   if (words == 0)
      word_len[1] = 0;

   lines = file->length - blank;

   /*
    * For file size, add the line ending.
    */
   switch (file->crlf) {
      case CRLF: flen += file->length; /* fall through */
      case LF:   flen += file->length;
   }
   /*
    * Adjust for EOF marker.
    */
   if (file->crlf != BINARY) {
      ll = ll->prev;
      if (ll->len > 0 && ll->line[ll->len-1] == '\x1a')
         /*
          * The EOF marker already exists - remove the line ending.
          */
         switch (file->crlf) {
            case CRLF: --flen;  /* fall through */
            case LF:   --flen;
         }
      else if (mode.control_z)
         ++flen;
   }

   memset( blen, 0, sizeof(blen) );

   /*
    * determine the width of each column (title)
    */
   sa[0] = strlen( stats[0] );
   for (j = 1; j < 4; ++j) {
      len = strlen( stats[j] );
      if (len > sa[0])
         sa[0] = len;
   }
   sa[1] = sa[2] = sa[0];
   for (k = 0; k < 3; ++k) {
      for (j = 0; j < 3; ++j) {
         len = strlen( stats1[j][k] );
         if (len > sa[k])
            sa[k] = len;
      }
   }

   /*
    * determine the width of each column (value)
    */
   na[0] = numlen( (file->length > line_len[2]) ? file->length : line_len[2] );
   na[1] = numlen( (alnum > symbols) ? alnum : symbols );
   na[2] = numlen( (chars > spaces) ? chars : spaces );

   #define AVG( sum, num )\
      (((num) == 0) ? 0 : (int)(((sum) + (num) / 2) / (num)))
   #define PCT( num, den )\
      (((den) == 0) ? 0 : (int)(100 * ((num) + (den) / 200) / (den)))

   pct[0] = PCT( symbols, alnum );
   pct[1] = PCT( spaces, chars );
   if (numlen( pct[0] ) > na[1])
      na[1] = numlen( pct[0] );
   if (numlen( pct[1] ) > na[2])
      na[2] = numlen( pct[1] );

   blen[STATS_LINE] =
   blen[STATS_LINE+1] =
   blen[STATS_LINE+2] =
   blen[STATS_LINE+3] =
      sprintf( buf[STATS_LINE], "%*s: %*ld   %*s: %*ld   %*s: %*ld",
               sa[0], stats1[0][0], na[0], lines,
               sa[1], stats1[0][1], na[1], words,
               sa[2], stats1[0][2], na[2], strings );
   sprintf( buf[STATS_WORD+1], "%*s: %*d   %*s: %*d   %*s: %*d",
            sa[0], stats[0], na[0], line_len[1],
            sa[1], stats[0], na[1], word_len[1],
            sa[2], stats[0], na[2], string_len[1] );
   sprintf( buf[STATS_WORD+2], "%*s: %*d   %*s: %*d   %*s: %*d",
            sa[0], stats[1], na[0], line_len[2],
            sa[1], stats[1], na[1], word_len[2],
            sa[2], stats[1], na[2], string_len[2] );
   sprintf( buf[STATS_WORD+3], "%*s: %*d   %*s: %*d   %*s: %*d",
            sa[0], stats[2], na[0], AVG( llen, lines ),
            sa[1], stats[2], na[1], AVG( alnum, words ),
            sa[2], stats[2], na[2], AVG( chars, strings ) );

   blen[STATS_WORD+5] =
   blen[STATS_WORD+6] =
   blen[STATS_WORD+7] =
      sprintf( buf[STATS_WORD+5], "%*s: %*ld   %*s: %*ld   %*s: %*ld",
               sa[0], stats1[1][0], na[0], blank,
               sa[1], stats1[1][1], na[1], alnum,
               sa[2], stats1[1][2], na[2], chars );
   sprintf( buf[STATS_WORD+6], "%*s: %*ld   %*s: %*ld   %*s: %*ld",
            sa[0], stats1[2][0], na[0], file->length,
            sa[1], stats1[2][1], na[1], symbols,
            sa[2], stats1[2][2], na[2], spaces );
   sprintf( buf[STATS_WORD+7], "%*s: %*d   %*s: %*d   %*s: %*d",
            sa[0], stats[2], na[0], AVG( llen, file->length ),
            sa[1], stats[3], na[1], pct[0],
            sa[2], stats[3], na[2], pct[1] );

   blen[STATS_SIZE] = sprintf( buf[STATS_SIZE], stats4, numlen( mlen ), flen,
                               eol_mode[file->crlf] );

   blen[STATS_MEM] = sprintf( buf[STATS_MEM], stats5, mlen );

   show_search_message( CLR_SEARCH );

   create_stat( stats0, file->file_name, buf, blen, STATS_COUNT );

   return( OK );
}


/*
 * Name:    create_stat
 * Purpose: create and display the status or statistics screens
 * Author:  Jason Hood
 * Date:    June 7, 2001
 * Passed:  title:  title of the screen
 *          name:   name of the file
 *          buf:    lines to display
 *          blen:   length of each line
 *          cnt:    number of lines in buf
 *
 * 030320:  remove the time.
 */
static void create_stat( const char *title, char *name,
                         char buf[][STAT_WIDTH+1], char blen[], int cnt )
{
char *frame;
char *p;
int  wid, len;
int  max;
int  j;

   reduce_string( buf[3], name + (*name == '\0'), STAT_WIDTH - 6, MIDDLE );
   blen[3] = strlen( buf[3] );

   max = 0;
   for (j = cnt - 2; j >= 3; --j)
      if (blen[j] > max)
         max = blen[j];

   wid = max + 6;
   assert( wid <= STAT_WIDTH );

   j = 2;
   frame = create_frame( wid, cnt, 1, &j, -1, -1 );
   if (frame == NULL) {
      /*
       * No memory for the frame, so use the raw strings.
       */
      for (j = 0; j < cnt; ++j) {
         memset( buf[j] + blen[j], ' ', wid - blen[j] );
         buf[j][wid] = '\0';
         stat_screen[j] = buf[j];
      }
   } else {
      p = frame;
      for (j = 0; j < cnt; ++j) {
         stat_screen[j] = p;
         p += wid + 1;
      }
      len = strlen( title );
      memcpy( stat_screen[1] + (wid - len) / 2, title, len );

      len = blen[3];
      memcpy( stat_screen[3] + (wid - len) / 2, buf[3], len );

      len = (wid - max) / 2;
      for (j = cnt - 2; j >= 5; --j) {
         if (blen[j] != 0)
            memcpy( stat_screen[j] + len, buf[j], blen[j] );
      }
   }

   stat_screen[cnt] = NULL;
   show_help( );

   if (frame != NULL)
      free( frame );
}


/*
 * Name:    create_frame
 * Purpose: create or display a frame
 * Author:  Jason Hood
 * Date:    November 19, 2003
 * Passed:  wid:    width of the frame
 *          hyt:    height of the frame
 *          cnt:    number of "inner" lines
 *          inner:  positions of the inner lines
 *          x:      column to display or -1 to create
 *          y:      row to display
 * Returns: NULL if displayed or no memory; otherwise pointer to buffer
 * Notes:   display will use the dialog color.
 *          the frame is filled with spaces.
 *          the buffer is a 2d array of char, not a 1d array of char pointers.
 */
char *create_frame( int wid, int hyt, int cnt, int *inner, int x, int y )
{
char line[MAX_COLS];
char sep[MAX_COLS];
char *p;
char *buf, *frame;
int  row;
const char *fc, *fc2;
int  j;

   assert( wid < MAX_COLS );

   row = y;
   if (x >= 0) {
      g_display.output_space = g_display.frame_space;
      frame = buf = NULL;
   } else {
      frame = buf = malloc( (wid + 1) * hyt );
      if (buf == NULL)
         return( NULL );
   }

   fc = graphic_char[g_display.frame_style];
   fc2 = (g_display.frame_style < 3) ? fc : graphic_char[2];

   line[wid] = '\0';

   line[0] = fc2[CORNER_LEFT_UP];
   memset( line + 1, fc2[HORIZONTAL_LINE], wid - 2 );
   line[wid-1] = fc2[CORNER_RIGHT_UP];
   if (x >= 0)
      s_output( line, y++, x, Color( Dialog ) );
   else {
      memcpy( buf, line, wid + 1 );
      buf += wid + 1;
   }

   line[0] = line[wid-1] = fc[VERTICAL_LINE];
   memset( line + 1, ' ', wid - 2 );
   if (cnt) {
      sep[0] = fc[LEFT_T];
      memset( sep + 1, fc[HORIZONTAL_LINE], wid - 2 );
      sep[wid-1] = fc[RIGHT_T];
      sep[wid] = '\0';
   }

   for (j = 1; j < hyt - 1; ++j) {
      if (cnt && *inner == j) {
         ++inner;
         --cnt;
         p = sep;
      } else
         p = line;
      if (x >= 0)
         s_output( p, y++, x, Color( Dialog ) );
      else {
         memcpy( buf, p, wid + 1 );
         buf += wid + 1;
      }
   }

   line[0] = fc2[CORNER_LEFT_DOWN];
   memset( line + 1, fc2[HORIZONTAL_LINE], wid - 2 );
   line[wid-1] = fc2[CORNER_RIGHT_DOWN];
   if (x >= 0)
      s_output( line, y, x, Color( Dialog ) );
   else
      memcpy( buf, line, wid + 1 );

   if (x >= 0) {
      shadow_area( wid, hyt, row, x );
      g_display.output_space = FALSE;
   }

   return( frame );
}


/*
 * Name:    numlen
 * Purpose: to find the character length of a number
 * Author:  Jason Hood
 * Date:    November 8, 1999
 * Passed:  num:  number whose character length is desired
 * Returns: number of characters that make up num
 * Notes:   Assumes num is a positive 32-bit number.
 */
int  numlen( long num )
{
int  len;

   if (num < 10000) {
           if (num <   10) len = 1;
      else if (num <  100) len = 2;
      else if (num < 1000) len = 3;
      else                 len = 4;
   } else {
           if (num < (long)1e5) len = 5;
      else if (num < (long)1e6) len = 6;
      else if (num < (long)1e7) len = 7;
      else if (num < (long)1e8) len = 8;
      else if (num < (long)1e9) len = 9;
      else                      len = 10;
   }

   return( len );
}


/*
 * Name:    relative_path
 * Purpose: convert an absolute filename into a relative filename
 * Author:  Jason Hood
 * Date:    October 28, 2003 (originally 980504 as part of show_window_fname)
 * Passed:  rel:     buffer for converted name (at least PATH_MAX chars)
 *          window:  window containing file to convert
 *          split:   set TRUE to convert to "name - path" format
 * Returns: rel
 * Notes:   if the current directory is within one subdirectory of the file
 *           don't split. Eg:
 *
 *           cwd: d:/dir1/dir2a/
 *                d:/dir1/dir2a/file1      --> file1
 *                d:/dir1/dir2a/dir3/file2 --> dir3/file2
 *                d:/dir1/file3            --> ../file3
 *                d:/dir1/dir2b/file4      --> ../dir2b/file4
 *
 *          if the file is on a different drive, test with that drive's cwd
 *           and use the drive letter (eg: "d:utils.c").
 */
char *relative_path( char *rel, TDE_WIN *window, int split )
{
char *path, *name;
char cwd[PATH_MAX];
char drv[4];
int  len;
int  rc;

   path = window->file_info->file_name;
   if (*path == '\0') {
      *rel = '\0';
      return( rel );
   }
   *drv = '\0';
   len  = 0;
   rc   = get_current_directory( cwd );
#if !defined( __UNIX__ )
   if (rc == OK  &&  *cwd != *path) {
      drv[0] = *path;
      drv[1] = ':';
      drv[2] = '\0';
      get_full_path( drv, cwd );
      name = strchr( cwd, '\0' );
      if (name[-1] != '/') {
         name[0] = '/';
         name[1] = '\0';
      }
   }
#endif
   if (rc == OK) {
      len = strlen( cwd );
      if (strncmp( cwd, path, len ) != 0) {
         /*
          * Go back a directory; if that matches, use the parent shortcut.
          */
         for (len -= 2; len != 0 && cwd[len] != '/'; --len) ;
         if (len
#if !defined( __UNIX__ )
             && cwd[len-1] != ':'
#endif
             && strncmp( cwd, path, len + 1 ) == 0) {
            rc = 1;
            ++len;
         } else {
#if !defined( __UNIX__ )
            if (path[1] == ':')
               len = 2;
            else
#endif
            len = 0;
         }
      }
      if (split) {
         /*
          * Don't split if the file is:
          *  only one directory below the current or its parent;
          *  in the root directory;
          *  in the first subdirectory.
          */
         name = strchr( path + len + (path[len] == '/'), '/' );
         if (name)
            name = strchr( name + 1, '/' );
         if (!name)
            split = FALSE;
      }
   }
   if (split  &&  (name = window->file_info->fname) != path) {
      combine_strings( rel, name, " - ", drv );
      *cwd  = *name;
      *name = '\0';
      if (rc == 1)
         strcat( rel, "../" );
      strcat( rel, path + len );
      *name = *cwd;
   } else {
      strcpy( rel, drv );
      if (rc == 1)
         strcat( rel, "../" );
      strcat( rel, path + len );
   }
   return( rel );
}
