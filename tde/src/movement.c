/*
 * jmh 991021: This file was originally part of utils.c, but since it was
 *             growing so big, I separated these movement functions.
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


TDE_WIN *browser_window;        /* current window being used for browsing */

static int  browser_parse( char *, long *, int * );


/*
 * Stuff for draw mode.
 */
#define G_UP 1          /*     G_UP     */
#define G_DN 2          /* G_LF +  G_RT */
#define G_LF 4          /*     G_DN     */
#define G_RT 8

static char get_g_flag( char, const char * );
static char get_g_char( char, const char * );
static void set_vert_g_char( TDE_WIN *, char );
static void set_horz_g_char( TDE_WIN *, char );


/*
 * Name:    first_line
 * Purpose: move to the first line
 * Author:  Jason Hood
 * Date:    March 27, 2003
 * Passed:  window:  pointer to window
 * Notes:   if the file is empty, will move to EOF.
 */
void first_line( TDE_WIN *window )
{
   window->ll = window->file_info->line_list->next;
   window->rline = 1;
   window->bin_offset = 0;
}


/*
 * Name:    inc_line
 * Purpose: move down one line
 * Author:  Jason Hood
 * Date:    November 20, 1999
 * Passed:  window:  pointer to window to move
 *          eof:     TRUE if EOF is allowed, FALSE otherwise.
 * Returns: TRUE if the move occurred
 *          FALSE if already at EOF/last line
 * Notes:   move rline, ll and bin_offset to the next line.
 */
int  inc_line( TDE_WIN *window, int eof )
{
int  rc;

   rc = (window->rline < window->file_info->length + (eof == TRUE));
   if (rc) {
      if (window->rline != 0)
         window->bin_offset += window->ll->len;
      ++window->rline;
      window->ll = window->ll->next;
   }
   return( rc );
}


/*
 * Name:    dec_line
 * Purpose: move up one line
 * Author:  Jason Hood
 * Date:    November 20, 1999
 * Passed:  window:  pointer to window to move
 *          tof:     TRUE if TOF is allowed, FALSE otherwise.
 * Returns: TRUE if the move occurred
 *          FALSE if already at TOF/first line
 * Notes:   move rline, ll and bin_offset to the previous line.
 */
int  dec_line( TDE_WIN *window, int tof )
{
int  rc;

   rc = (window->rline > 0 + (tof == FALSE));
   if (rc) {
      --window->rline;
      window->ll = window->ll->prev;
      if (window->rline != 0)
         window->bin_offset -= window->ll->len;
   }
   return( rc );
}


/*
 * Name:    move_to_line
 * Purpose: move the window to a new line
 * Author:  Jason Hood
 * Date:    November 20, 1999
 * Passed:  window:  pointer to window to move
 *          line:    line to move to
 *          eof:     TRUE if TOF and EOF are valid lines, FALSE otherwise.
 * Returns: TRUE if the move occurred
 *          FALSE if the line is out of range, position unchanged
 * Notes:   move rline, ll and bin_offset to the specified line.
 */
int  move_to_line( TDE_WIN *window, long line, int eof )
{
int  rc;
register TDE_WIN *win;
long n;
line_list_ptr ll;
long o;

   win = window;
   rc  = (line >= 0 + (eof == FALSE) &&
          line <= win->file_info->length + (eof == TRUE));
   if (rc) {
      n = win->rline;
      if (n != line) {
         if (line <= 1) {
            o  = 0;
            ll = win->file_info->line_list;
            if (line == 1)
               ll = ll->next;

         } else {
            ll = win->ll;
            o  = win->bin_offset;
            if (line < n) {
               if (n - line < line - 1) {
                  for (; n > line; n--) {
                     ll = ll->prev;
                     o -= ll->len;
                  }
                  if (line == 0)
                     o += EOF;

               } else {
                  ll = win->file_info->line_list->next;
                  o  = 0;
                  for (n = 1; n < line; n++) {
                     o += ll->len;
                     ll = ll->next;
                  }
               }
            } else {
               if (n == 0)
                  o = -EOF;
               for (; n < line; n++) {
                  o += ll->len;
                  ll = ll->next;
               }
            }
         }
         win->rline = line;
         win->ll    = ll;
         win->bin_offset = o;
      }
   }
   return( rc );
}


/*
 * Name:    move_display
 * Purpose: move to a line and display it
 * Author:  Jason Hood
 * Date:    November 24, 1999
 * Passed:  window:  pointer to current window
 *          pos:     pointer to window containing new position
 * Notes:   set window's rline, ll, rcol and bin_offset to those in pos.
 *          If the new line is on-screen, move to it, otherwise move the line
 *           to the current cursor position.
 *
 * jmh 020815: Billy Chen noted the ruler wasn't being properly updated.
 *              (This is also fixed in some other functions.)
 */
void move_display( TDE_WIN *window, TDE_WIN *pos )
{
register TDE_WIN *win;
TDE_WIN old;
long diff;
int  cline;

   win = window;
   dup_window_info( &old, win );

   win->rline = pos->rline;
   win->rcol  = pos->rcol;
   win->ll    = pos->ll;
   win->bin_offset = pos->bin_offset;

   diff = win->rline - old.rline;
   if (diff) {
      cline = win->cline;
      if (labs( diff ) >= g_display.end_line  ||
          check_cline( win, win->cline + (int)diff )) {
         check_cline( win, cline );
         win->file_info->dirty = LOCAL | RULER;
      }
   }
   check_virtual_col( win, win->rcol, win->ccol );

   if (!win->file_info->dirty) {
      if (old.rcol != win->rcol)
         show_ruler( win );

      if (old.rline != win->rline) {
         update_line( &old );
         show_curl_line( win );
      }
   }
}


/*
 * Name:    prepare_move_up
 * Purpose: Do the stuff needed to move the cursor up one line.
 * Date:    November 23, 1999
 * Passed:  window:  pointer to current window
 * Notes:   Put all the stuff needed to move the cursor up one line in
 *           one function, so several functions can use the guts of the
 *           algorithm.
 */
int  prepare_move_up( TDE_WIN *window )
{
int  rc = OK;
register TDE_WIN *win;  /* put window pointer in a register */

   win = window;
   if (win->rline > 0) {
      if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
         return( ERROR );

      undo_move( win, LineDown );

      if (win->cline == win->top_line) {
         dec_line( win, TRUE );
         win->file_info->dirty = LOCAL;
      } else {
         update_line( win );
         dec_line( win, TRUE );
         --win->cline;          /* we aren't at top of screen - so move up */
         show_curl_line( win );
      }
   } else
      rc = ERROR;
   return( rc );
}


/*
 * Name:    prepare_move_down
 * Purpose: Do the stuff needed to move the cursor down one line.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   Put all the stuff needed to move the cursor down one line in
 *           one function, so several functions can use the guts of the
 *           algorithm.
 *
 * jmh 991123: don't scroll when on EOF (use ScrollDown instead).
 */
int  prepare_move_down( TDE_WIN *window )
{
int  rc = OK;
register TDE_WIN *win;  /* put window pointer in a register */

   win = window;
   if (win->ll->next != NULL) {
      if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
         return( ERROR );

      if (g_status.command == BegNextLine)
         undo_move( win, (win->rcol == 0) ? LineUp : 0 );
      else if (g_status.command == NextLine || g_status.command == EndNextLine)
         undo_move( win, 0 );
      else
         undo_move( win, LineUp );

      if (win->cline == win->bottom_line) {
         inc_line( win, TRUE );
         win->file_info->dirty = LOCAL;
      } else {
         update_line( win );
         inc_line( win, TRUE );
         ++win->cline;          /* if not at bottom of screen move down */
         show_curl_line( win );
      }
   } else
      rc = ERROR;
   return( rc );
}


/*
 * Name:    home
 * Purpose: To move the cursor to the left of the current line.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   this routine is made a little more complicated with cursor sync.
 *            if the g_status.copied flag is set we need to see from what file
 *            the line_buff was copied.
 */
int  home( TDE_WIN *window )
{
register int rcol;
register TDE_WIN *win;   /* put window pointer in a register */
text_ptr p;
int  len;
int  tabs;
int  tab_size;

   win = window;

   if (g_status.command == StartOfLine)
      rcol = 0;
   else {
      tabs = win->file_info->inflate_tabs;
      tab_size = win->file_info->ptab_size;

      if (g_status.copied && win->file_info == g_status.current_file) {
         p   = g_status.line_buff;
         len = g_status.line_buff_len;
      } else {
         p   = win->ll->line;
         len = win->ll->len;
      }
      rcol =  (is_line_blank( p, len, tabs )) ? 0 :
             first_non_blank( p, len, tabs, tab_size );
      if (win->rcol == rcol)
         rcol = 0;
   }

   undo_move( win, 0 );

   show_ruler_char( win );
   check_virtual_col( win, rcol, win->ccol );
   cursor_sync( win );
   return( OK );
}


/*
 * Name:    goto_eol
 * Purpose: To move the cursor to the eol character of the current line.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   this routine is made a little more complicated with cursor sync.
 *            if the g_status.copied flag is set we need to see from what file
 *            the line_buff was copied.
 *
 * jmh 030302: make allowances for being called from StreamCharLeft.
 * jmh 030303: make allowances for being called from EndNextLine.
 */
int  goto_eol( TDE_WIN *window )
{
register int rcol;
register TDE_WIN *win;   /* put window pointer in a register */
text_ptr p;
int  len;

   if (g_status.command_count == 0  ||  g_status.command == StreamCharLeft  ||
       g_status.command == EndNextLine) {
      win = window;
      if (g_status.command == EndOfLine)
         undo_move( win, 0 );
      else if (g_status.command == EndNextLine)
         prepare_move_down( win );

      if (g_status.copied && win->file_info == g_status.current_file) {
         p   = g_status.line_buff;
         len = g_status.line_buff_len;
      } else {
         p   = win->ll->line;
         len = win->ll->len;
      }
      rcol = find_end( p, len, win->file_info->inflate_tabs,
                               win->file_info->ptab_size );
      show_ruler_char( win );
      win->ccol = win->start_col + rcol - win->bcol;
      check_virtual_col( win, rcol, win->ccol );
      if (g_status.command != StreamCharLeft)
         cursor_sync( win );
   }
   return( OK );
}


/*
 * Name:    goto_top
 * Purpose: To move the cursor to the top of the current window.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   If the start of the file occurs before the top of the window,
 *           then the start of the file is moved to the top of the window.
 */
int  goto_top( TDE_WIN *window )
{
register TDE_WIN *win;   /* put window pointer in a register */
long line;
int  diff;
int  empty;

   if (g_status.command_count == 0) {
      win   = window;
      diff  = win->cline - win->top_line;
      line  = win->rline;
      empty = (win->file_info->length == 0);
      if ((diff && (line != 1 || empty))  ||  (line == 0 && !empty)) {
         if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
            return( ERROR );

         undo_move( win, 0 );

         update_line( win );
         line -= diff;
         win->cline = win->top_line;
         if (line == 0 && !empty) {
            ++line;
            if (win->cline < win->bottom_line)
               ++win->cline;
         }
         move_to_line( win, line, TRUE );
         show_curl_line( win );
      }
      cursor_sync( win );
   }
   return( OK );
}


/*
 * Name:    goto_bottom
 * Purpose: To move the cursor to the bottom of the current window.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 */
int  goto_bottom( TDE_WIN *window )
{
register TDE_WIN *win;   /* put window pointer in a register */
int  diff;
long line;
int  empty;
long length;

   if (g_status.command_count == 0) {
      win   = window;
      diff  = win->bottom_line - win->cline;
      line  = win->rline;
      empty = ((length = win->file_info->length) == 0);
      if ((diff && (line != length || empty)) || (line == length+1 && !empty)) {
         if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
            return( ERROR );

         undo_move( win, 0 );

         if (line == length+1) {
            dec_line( win, TRUE );
            if (win->cline == win->top_line)
               win->file_info->dirty = LOCAL;
            else {
               --win->cline;    /* we aren't at top of screen - so move up */
               show_curl_line( win );
            }
         } else {
            update_line( win );
            if (line + diff > length)
               diff = (int)(length - line);
            line += diff;
            win->cline += diff;
            move_to_line( win, line, TRUE );
            show_curl_line( win );
         }
      }
      cursor_sync( win );
   }
   return( OK );
}


/*
 * Name:    page_up
 * Purpose: To move the cursor one (half) page up the window
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   The cursor line is moved back the required number of lines
 *           towards the start of the file.
 *          If the start of the file is reached, then movement stops.
 *
 * jmh 020906: added the half-page function.
 */
int  page_up( TDE_WIN *window )
{
int  i;                 /* count of lines scanned */
int  rc = OK;           /* default return code */
register TDE_WIN *win;  /* put window pointer in a register */
long number;

   win = window;
   i   = win->cline - win->top_line;
   if (win->rline != i) {
      if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
         return( ERROR );

      number = win->rline - ((g_status.command == ScreenUp) ? win->page
                             : ((win->page + g_status.overlap) / 2));
      if (number < i) {
         number = i;
         undo_move( win, 0 );
      } else
         undo_move( win, (g_status.command == ScreenUp) ? ScreenDown
                                                        : HalfScreenDown );

      move_to_line( win, number, TRUE );
      win->file_info->dirty = LOCAL;
   } else
      rc = ERROR;
   cursor_sync( win );
   return( rc );
}


/*
 * Name:    page_down
 * Purpose: To move the cursor one (half) page down the window
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   The cursor line is moved forwards the required number of lines
 *           towards the end of the file.
 *          If the end of the file is reached, then movement stops.
 *
 * jmh 020906: added the half-page function.
 */
int  page_down( TDE_WIN *window )
{
int  rc = OK;
long line;
long last;
register TDE_WIN *win;  /* put window pointer in a register */

   win  = window;
   line = win->rline;
   last = win->file_info->length + 1;
   if (line - (win->cline - win->top_line) <
       last - (win->bottom_line - win->top_line)) {
      if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
         return( ERROR );

      line += (g_status.command == ScreenDown) ? win->page
              : ((win->page + g_status.overlap) / 2);
      if (line > last) {
         win->cline -= (int)(line - last);
         line = last;
         undo_move( win, 0 );
      } else
         undo_move( win, (g_status.command == ScreenDown) ? ScreenUp
                                                          : HalfScreenUp );

      move_to_line( win, line, TRUE );
      win->file_info->dirty = LOCAL;
   } else
      rc = ERROR;
   cursor_sync( win );
   return( rc );
}


/*
 * Name:    scroll_up
 * Purpose: To scroll the window up one line
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   If this is the first page, then update screen here.  Else, make
 *           the window LOCAL dirty because we have to redraw screen.
 */
int  scroll_up( TDE_WIN *window )
{
int  rc = OK;
register TDE_WIN *win;   /* put window pointer in a register */

   win = window;
   if (win->rline > 0) {
      if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
         return( ERROR );

      if (win->rline == (win->cline - win->top_line)) {
         undo_move( win, LineDown );
         update_line( win );
         dec_line( win, TRUE );
         --win->cline;
         show_curl_line( win );
      } else {
         if (win->cline == win->bottom_line) {
            undo_move( win, LineDown );
            dec_line( win, TRUE );
         } else
            ++win->cline;
         win->file_info->dirty = LOCAL;
      }
   } else
     rc = ERROR;
   cursor_sync( win );
   return( rc );
}


/*
 * Name:    scroll_down
 * Purpose: scroll window down one line
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   If there is a line to scroll_down, make the window LOCAL dirty.
 *          We have to redraw the screen anyway, so don't update here.
 */
int  scroll_down( TDE_WIN *window )
{
int  rc = OK;
register TDE_WIN *win;   /* put window pointer in a register */

   win = window;
   if (win->cline == win->top_line) {
      if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
         return( ERROR );
      if (inc_line( win, TRUE )) {
         undo_move( win, LineUp );
         win->file_info->dirty = LOCAL;
      } else
         rc = ERROR;
   } else {
      --win->cline;
      win->file_info->dirty = LOCAL;
   }
   cursor_sync( win );
   return( rc );
}


/*
 * Name:    pan_up
 * Purpose: To leave cursor on same logical line and scroll text up
 * Date:    September 1, 1991
 * Passed:  window:  pointer to current window
 * Notes:   If cursor is on first page then do not scroll.
 */
int  pan_up( TDE_WIN *window )
{
int  rc = OK;
register TDE_WIN *win;   /* put window pointer in a register */

   win = window;

   /*
    * see if cursor is on the first page. if it's not then pan_up.
    */
   if (win->rline != (win->cline - win->top_line)) {
      if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
         return( ERROR );
      undo_move( win, PanDn );
      dec_line( win, TRUE );
      win->file_info->dirty = LOCAL;
   } else
      rc = ERROR;
   cursor_sync( win );
   return( rc );
}


/*
 * Name:    pan_down
 * Purpose: To leave cursor on same logical line and scroll text down
 * Date:    September 1, 1991
 * Passed:  window:  pointer to current window
 * Notes:   If cursor is on last line in file then do not scroll.
 */
int  pan_down( TDE_WIN *window )
{
int  rc = OK;
register TDE_WIN *win;   /* put window pointer in a register */

   win = window;
   if (win->ll->next != NULL) {
      if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
         return( ERROR );
      undo_move( win, PanUp );
      inc_line( win, TRUE );
      win->file_info->dirty = LOCAL;
   } else
      rc = ERROR;
   cursor_sync( win );
   return( rc );
}


/*
 * Name:    move_up
 * Purpose: To move the cursor up one line
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   If the cursor is at the top of the window, then the file must
 *           be scrolled down.
 *
 * jmh 991018: added drawing mode.
 */
int  move_up( TDE_WIN *window )
{
int  rc = OK;
register TDE_WIN *win;  /* put window pointer in a register */

   win = window;

   if (mode.draw && win->ll->len != EOF)
      set_vert_g_char( win, G_UP );

   rc = prepare_move_up( win );

   if (mode.draw && rc == OK) {
      if (win->rline == 0) {
         g_status.command = AddLine;
         rc = insert_newline( win );
         prepare_move_down( win );
      } else
         set_vert_g_char( win, ~G_DN );
   }

   cursor_sync( win );
   return( rc );
}


/*
 * Name:    move_down
 * Purpose: To move the cursor down one line
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   If the cursor is at the bottom of the window, then the file must
 *           be scrolled up.   If the cursor is at the bottom of the file,
 *           then scroll line up until it is at top of screen.
 *
 * jmh 991018: added drawing mode.
 */
int  move_down( TDE_WIN *window )
{
int  rc;
register TDE_WIN *win;   /* put window pointer in a register */

   win = window;

   if (mode.draw) {
      if (win->ll->len != EOF)
         set_vert_g_char( win, G_DN );
      if (win->rline == win->file_info->length) {
         g_status.command = AddLine;
         insert_newline( win );
      }
   }

   rc = prepare_move_down( win );

   if (mode.draw && win->ll->len != EOF)
      set_vert_g_char( win, ~G_UP );

   cursor_sync( win );
   return( rc );
}


/*
 * Name:    beg_next_line
 * Purpose: To move the cursor to the beginning of the next line.
 * Date:    October 4, 1991
 * Passed:  window:  pointer to current window
 */
int  beg_next_line( TDE_WIN *window )
{
int  rc;

   rc = prepare_move_down( window );
   show_ruler_char( window );
   check_virtual_col( window, 0, window->start_col );
   cursor_sync( window );
   return( rc );
}


/*
 * Name:    next_line
 * Purpose: To move the cursor to the first character of the next line.
 * Date:    October 4, 1991
 * Passed:  window:  pointer to current window
 *
 * jmh 990504: match indentation/margin if the next line is blank.
 * jmh 031114: indent mode overrides fixed wrap mode.
 */
int  next_line( TDE_WIN *window )
{
register int rcol;
register TDE_WIN *win;   /* put window pointer in a register */
line_list_ptr ll;
int  tabs;
int  tab_size;
int  rc;

   win = window;
   tabs = win->file_info->inflate_tabs;
   tab_size = win->file_info->ptab_size;

   rc = prepare_move_down( win );
   ll = win->ll;
   rcol =  (mode.indent || mode.word_wrap == DYNAMIC_WRAP) ?
          find_left_margin( ll, DYNAMIC_WRAP, tabs, tab_size ) :
          first_non_blank( ll->line, ll->len, tabs, tab_size );
   show_ruler_char( win );
   check_virtual_col( win, rcol, win->ccol );
   cursor_sync( win );
   return( rc );
}


/*
 * Name:    move_left
 * Purpose: To move the cursor left one character
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   If the cursor is already at the left of the screen, then
 *           scroll horizontally if we're not at beginning of line.
 * jmh 980724: update cursor cross
 * jmh 991018: added drawing mode.
 * jmh 991123: return ERROR if already at BOL;
 *             skip ccol == start_col test;
 *             don't bother showing the ruler after the sync.
 * jmh 030302: recognise the stream version.
 * jmh 031114: for the stream version treat beyond EOL as EOL.
 */
int  move_left( TDE_WIN *window )
{
register TDE_WIN *win;
text_ptr p;
int  len;
int  rc = OK;

   win = window;
   if (win->rcol > 0) {
      if (mode.cursor_cross)
         win->file_info->dirty = LOCAL;

      if (mode.draw && win->ll->len != EOF)
         set_horz_g_char( win, G_LF );

      undo_move( win, CharRight );

      if (g_status.command == StreamCharLeft  &&  !mode.draw) {
         if (g_status.copied && win->file_info == g_status.current_file) {
            p   = g_status.line_buff;
            len = g_status.line_buff_len;
         } else {
            p   = win->ll->line;
            len = win->ll->len;
         }
         len = find_end( p, len, win->file_info->inflate_tabs,
                                 win->file_info->ptab_size );
         if (len == 0)
            goto prev_line;
         if (win->rcol >= len)
            rc = goto_eol( win );
      }

      if (win->ccol > win->start_col) {
         show_ruler_char( win );
         --win->ccol;
      } else { /* (win->ccol == win->start_col) */
         --win->bcol;
         win->file_info->dirty = LOCAL | RULER;
      }
      --win->rcol;
   } else if (g_status.command == StreamCharLeft  &&  !mode.draw) {
prev_line:
      rc = prepare_move_up( win );
      if (rc == OK)
         rc = goto_eol( win );
   } else
      rc = ERROR;
   cursor_sync( win );
   return( rc );
}


/*
 * Name:    move_right
 * Purpose: To move the cursor right one character
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   If the cursor is already at the right of the screen (logical
 *          column 80) then scroll horizontally right.
 * jmh 980724: update cursor cross
 * jmh 991018: added drawing mode.
 * jmh 991123: return ERROR if can't go any further;
 *             skip ccol == end_col test;
 *             don't bother showing the ruler after the sync.
 * jmh 030302: recognise the stream version.
 */
int  move_right( TDE_WIN *window )
{
register TDE_WIN *win;
text_ptr p;
int  len;
int  rc = OK;
int  moved = FALSE;

   win = window;
   if (g_status.command == StreamCharRight  &&  !mode.draw) {
      if (g_status.copied && win->file_info == g_status.current_file) {
         p   = g_status.line_buff;
         len = g_status.line_buff_len;
      } else {
         p   = win->ll->line;
         len = win->ll->len;
      }
      len = find_end( p, len, win->file_info->inflate_tabs,
                              win->file_info->ptab_size );
      if (win->rcol >= len) {
         undo_move( win, 0 );
         rc = prepare_move_down( win );
         show_ruler_char( win );
         check_virtual_col( win, 0, win->start_col );
         moved = TRUE;
      }
   }
   if (!moved) {
      if (win->rcol < MAX_LINE_LENGTH - 1) {
         if (mode.cursor_cross)
            win->file_info->dirty = LOCAL;

         if (mode.draw && win->ll->len != EOF)
            set_horz_g_char( win, G_RT );

         undo_move( win, CharLeft );

         if (win->ccol < win->end_col) {
            show_ruler_char( win );
            ++win->ccol;
         } else { /* (win->ccol == win->end_col) */
            ++win->bcol;
            win->file_info->dirty = LOCAL | RULER;
         }
         ++win->rcol;
      } else
         rc = ERROR;
   }
   cursor_sync( win );
   return( rc );
}


/*
 * Name:    pan_left
 * Purpose: To pan the screen left one character
 * Date:    January 5, 1992
 * Passed:  window:  pointer to current window
 *
 * jmh 991123: return ERROR if can't pan any further.
 */
int  pan_left( TDE_WIN *window )
{
int  rc = OK;

   if (window->bcol > 0 ) {
      undo_move( window, PanRight );
      --window->bcol;
      --window->rcol;
      window->file_info->dirty = LOCAL | RULER;
   } else
      rc = ERROR;
   cursor_sync( window );
   return( rc );
}


/*
 * Name:    pan_right
 * Purpose: To pan the screen right one character
 * Date:    January 5, 1992
 * Passed:  window:  pointer to current window
 *
 * jmh 991123: return ERROR if can't pan any further.
 */
int  pan_right( TDE_WIN *window )
{
int  rc = OK;

   if (window->bcol < MAX_LINE_LENGTH -
                        (window->end_col - window->start_col + 1)) {
      undo_move( window, PanLeft );
      ++window->rcol;
      ++window->bcol;
      window->file_info->dirty = LOCAL | RULER;
   } else
      rc = ERROR;
   cursor_sync( window );
   return( rc );
}


/*
 * Name:    word_left
 * Purpose: To move the cursor left one word
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   Words are considered strings of letters, numbers and underscores,
 *          which must be separated by other characters.
 *
 * jmh 991123: allow movement off EOF.
 * jmh 021012: recognise WordWrap as a special case (for paragraph formatting).
 */
int  word_left( TDE_WIN *window )
{
text_ptr p;             /* text pointer */
int  len;               /* length of current line */
int  rc;
int  tabs;
int  rcol;
register int r1;
TDE_WIN w;
int  (*space)( int );   /* Function to determine what a word is */
int  (*spc)( int );     /* Function to determine what is a space */
int  end;               /* End of word? */
int  eol;               /* On or beyond EOL? */

   if (window->rline > 0) {
      if (un_copy_line( window->ll, window, TRUE, TRUE ) == ERROR)
         return( ERROR );

      switch (g_status.command) {
         default:
         case WordLeft:      space = myiswhitespc;    break;
         case WordEndLeft:   space = myisnotwhitespc; break;
         case WordWrap:
         case StringLeft:    space = bj_isspc;        break;
         case StringEndLeft: space = bj_isnotspc;
      }
      end = (g_status.command == WordEndLeft  ||
             g_status.command == StringEndLeft);
      spc = (g_status.command == WordLeft || g_status.command == WordEndLeft)
             ? myiswhitespc : bj_isspc;

      rc = OK;
      dup_window_info( &w, window );
      rcol = w.rcol;
      p    = w.ll->line;
      len  = w.ll->len;

      tabs = (window->file_info->inflate_tabs)
             ? window->file_info->ptab_size : 0;
      r1 =  tabs ? entab_adjust_rcol( p, len, rcol, tabs ) : rcol;
      if (r1 >= len) {
         eol = (r1 > len  &&  end) ? ERROR : TRUE;
         r1  = len - 1;
      } else
         eol = FALSE;

      if (r1 > 0)
         p += r1;
      if (!end && r1 >= 0 && !space(*p) && (eol || (r1 > 0 && !space(*(p-1))))){
         /* do nothing */
      } else if (eol == ERROR && r1 >= 0 && space( *p )) {
         /* do nothing */
      } else {

         /*
          * if we are on the first letter of a word, get off.
          */
         if (end && r1 > 0 && !eol && spc( *p ) && !spc( *(p-1) )) {
            --r1;
            --p;
         }
         for (; r1 >= 0 && !spc( *p ); r1--, p--);

         /*
          * go to the previous line if word begins at 1st col in line.
          */
         if (r1 < 0) {
            if (dec_line( &w, FALSE )) {
               p  = w.ll->line;
               r1 = w.ll->len - 1;
               if (r1 > 0)
                  p += r1;
            } else
               rc = ERROR;
         }

         /*
          * skip all blanks until we get to a previous word
          */
         while (rc == OK  &&  (p == NULL  ||  spc( *p ))) {
            for (; r1 >= 0 && spc( *p ); r1--, p--);
            if (r1 < 0) {
               if (dec_line( &w, FALSE )) {
                  p  = w.ll->line;
                  r1 = w.ll->len - 1;
                  if (r1 > 0)
                     p += r1;
               } else
                  rc = ERROR;
            } else
               break;
         }
      }
      if (rc == OK) {
         if (g_status.command != WordWrap)
            undo_move( window, (g_status.command_count == 0) ? 0 : WordRight );

         /*
          * now, find the beginning of the word.
          */
         for (; r1 >= 0 && !space( *p ); r1--, p--);
         ++r1;
         w.rcol =  tabs ? detab_adjust_rcol( w.ll->line, r1, tabs ) : r1;
         move_display( window, &w );
      }
   } else
      rc = ERROR;

   if (g_status.command != WordWrap)
      cursor_sync( window );
   return( rc );
}


/*
 * Name:    word_right
 * Purpose: To move the cursor right one word
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   Words are considered strings of letters, numbers and underscores,
 *           which must be separated by other characters.
 *
 * jmh 991123: allow movement off TOF.
 * jmh 021012: recognise WordWrap as a special case (for paragraph formatting).
 */
int  word_right( TDE_WIN *window )
{
int  len;               /* length of current line */
text_ptr p;             /* text pointer */
int  rc;
int  tabs;
int  rcol;
register int r1;
TDE_WIN w;
int  (*space)( int );   /* Function to determine what a word is */
int  end;               /* Searching for end of word? */
int  tab_size;

   if (window->ll->next != NULL) {
      if (un_copy_line( window->ll, window, TRUE, TRUE ) == ERROR)
         return( ERROR );

      rc = OK;
      dup_window_info( &w, window );
      r1  = rcol = window->rcol;
      p   = w.ll->line;
      len = w.ll->len;

      tabs = window->file_info->inflate_tabs;
      tab_size = window->file_info->ptab_size;
      r1   =  tabs ? entab_adjust_rcol( p, len, rcol, tab_size ) : rcol;

      end   = (g_status.command == WordEndRight ||
               g_status.command == StringEndRight);
      space = (g_status.command == WordRight ||
               g_status.command == WordEndRight) ? myiswhitespc : bj_isspc;

      /*
       * if rcol is past EOL, move it to EOL
       */
      r1 =  r1 > len ? len : r1;
      if (r1 > 0)
         p += r1;

      /*
       * if cursor is on a word, find end of word.
       */
      if (end  &&  r1 < len  &&  !space( *p )) {
         /* do nothing */
      } else {
         for (; r1 < len && !space( *p ); r1++, p++);

         /*
          * go to the next line if word ends at eol.
          */
         if (r1 == len) {
            if (inc_line( &w, FALSE )) {
               p   = w.ll->line;
               len = w.ll->len;
               r1  = 0;
            } else
               rc = ERROR;
         }

         /*
          * now, go forward thru the file looking for the first letter of word.
          */
         while (rc == OK  &&  (p == NULL  ||  space( *p ))) {
            for (; r1 < len && space( *p ); r1++, p++);
            if (r1 == len) {
               if (inc_line( &w, FALSE )) {
                  p   = w.ll->line;
                  len = w.ll->len;
                  r1  = 0;
               } else
                  rc = ERROR;
            } else
               break;
         }
      }
      if (rc == OK) {
         if (g_status.command != WordWrap)
            undo_move( window, (g_status.command_count == 0) ? 0 : WordLeft );

         /*
          * now find the end of the word.
          */
         if (end)
            for (; r1 < len && !space( *p ); r1++, p++);

         w.rcol =  tabs ? detab_adjust_rcol( w.ll->line, r1, tab_size ) : r1;
         move_display( window, &w );
      }
   } else
      rc = ERROR;

   if (g_status.command != WordWrap)
      cursor_sync( window );
   return( rc );
}


/*
 * Name:     find_dirty_line
 * Purpose:  To move the cursor to the next/previous dirty line, if it exists
 * Date:     April 1, 1993
 * Modified: August 12, 1997, Jason Hood - reposition for real tabs
 * Passed:   window:  pointer to current window
 *
 * jmh 980824: skip groups of dirty lines.
 * jmh 991124: combined next and previous dirty lines into one routine.
 */
int  find_dirty_line( TDE_WIN *window )
{
int  rc;
TDE_WIN w;
int  (*new_line)( TDE_WIN *, int );

   if (un_copy_line( window->ll, window, TRUE, TRUE ) == ERROR)
      return( ERROR );
   dup_window_info( &w, window );
   rc = OK;
   new_line = (g_status.command == PrevDirtyLine) ? dec_line : inc_line;
   while (w.ll->type & DIRTY)
      new_line( &w, TRUE );
   while (!(w.ll->type & DIRTY)) {
      if (!new_line( &w, FALSE )) {
         rc = ERROR;
         break;
      }
   }
   if (rc == OK) {
      undo_move( window, 0 );
      set_marker( window );             /* remember previous position */
      move_display( window, &w );
   } else
      error( WARNING, window->bottom_line, utils16 );

   cursor_sync( window );
   return( rc );
}


/*
 * Name:    find_browse_line
 * Purpose: move the cursor to the file/line given in another window
 * Author:  Jason Hood
 * Date:    November 16, 2003
 * Passed:  window:  pointer to current window
 * Notes:   if the current window is output or results, either function will
 *           make it the current browse window, using the current line.
 *          if there is no browse window, use the current file if it's scratch.
 *          if the end/top of file is reached, wrap.
 */
int  find_browse_line( TDE_WIN *window )
{
int  current;
char name[PATH_MAX];
char cwd[PATH_MAX];
long rline = 0;
int  rcol = -1;
long stop;
TDE_WIN w;
TDE_WIN *win;
file_infos *fp;
int  rc;

   if (un_copy_line( window->ll, window, TRUE, TRUE ) == ERROR)
      return( ERROR );

   if (window->file_info->file_name[0] == '\0' &&
       (window->file_info->file_name[1] == OUTPUT ||
        window->file_info->file_name[1] == RESULTS)) {
      browser_window = window;
      fp = window->file_info;
      current = TRUE;
   } else
      current = FALSE;

   if (browser_window == NULL) {
      /*
       * try and find a browser file after the current
       */
      fp = window->file_info->next;
      while (fp != NULL  &&  !(fp->file_name[0] == '\0' &&
             (fp->file_name[1] == OUTPUT || fp->file_name[1] == RESULTS)))
         fp = fp->next;

      if (fp == NULL) {
         /*
          * try and find a browser file before the current
          */
         fp = window->file_info->prev;
         while (fp != NULL  &&  !(fp->file_name[0] == '\0' &&
                (fp->file_name[1] == OUTPUT || fp->file_name[1] == RESULTS)))
            fp = fp->prev;
      }

      if (fp == NULL) {
         if (window->file_info->scratch) {
            browser_window = window;
            fp = window->file_info;
         } else {
            /*
             * no browser window
             */
            error( WARNING, g_display.end_line, utils18a );
            return( ERROR );
         }
      } else
         browser_window = find_file_window( fp );
      current = (g_status.command == NextBrowse);
   } else
      fp = browser_window->file_info;

   /*
    * Set the current directory to that of the browser, since it uses
    * relative paths.
    */
   get_current_directory( cwd );
   set_current_directory( fp->backup_fname );

   show_search_message( SEARCHING );
   if (fp->length == 1)
      current = TRUE;
   stop = browser_window->rline;
   rc = ERROR;
   while (rc == ERROR) {
      if (current)
         current = FALSE;
      else {
         /*
          * update_line does not recognise hidden windows
          */
         if (!browser_window->visible)
            g_status.screen_display = FALSE;
         if (g_status.command == NextBrowse)
            prepare_move_down( browser_window );
         else
            prepare_move_up( browser_window );
         if (!browser_window->visible)
            g_status.screen_display = TRUE;
         if (browser_window->ll->len == EOF) {
            fp->dirty = LOCAL;
            if (g_status.command == NextBrowse) {
               first_line( browser_window );
               check_cline( browser_window, browser_window->top_line + 1 );
            } else {
               move_to_line( browser_window, fp->length, FALSE );
               check_cline( browser_window, browser_window->bottom_line - 1 );
            }
         }
         if (browser_window->rline == stop) {
            rc = ERROR;
            break;
         }
      }
      rc = browser_parse( name, &rline, &rcol );
   }
   show_search_message( CLR_SEARCH );

   if (fp->dirty) {
      if (browser_window->visible)
         display_current_window( browser_window );
      fp->dirty = FALSE;
   }

   if (rc == OK  &&  *name == '\0') {
      /* a results window was generated from a pipe or scratch window;
       * if the window before the browser is non-file, assume that one,
       * otherwise fail.
       */
      if (fp->prev == NULL || fp->prev->file_name[0] != '\0')
         rc = ERROR;
   }

   if (rc == OK) {
      if (*name == '\0')
         win = find_file_window( fp->prev );
      else {
         get_full_path( name, name );
         win = NULL;
         for (fp = g_status.file_list; fp != NULL; fp = fp->next) {
            if (strcmp( name, fp->file_name ) == 0) {
               win = find_file_window( fp );
               break;
            }
         }
      }
      if (win != NULL) {
         change_window( window, win );
         if (rline != 0) {
            set_marker( win );
            if (rline > win->file_info->length)
               rline = win->file_info->length + 1;
            if (win != window) {
               g_status.wrapped = TRUE;
               show_search_message( CHANGED );
            } else if ((rline < win->rline && g_status.command == NextBrowse) ||
                       (rline > win->rline && g_status.command == PrevBrowse)) {
               g_status.wrapped = TRUE;
               show_search_message( WRAPPED );
            }
            dup_window_info( &w, win );
            move_to_line( &w, rline, TRUE );
            if (rcol != -1)
               w.rcol = rcol;
            move_display( win, &w );
            if (win->cline == w.cline  &&  win->file_info->dirty)
               center_window( win );
         }
      } else {
         g_status.jump_to = rline;
         if (rcol != -1)
            g_status.jump_col = rcol + 1;
         rc = attempt_edit_display( name, LOCAL );
         if (rc == OK) {
            g_status.wrapped = TRUE;
            show_search_message( CHANGED );
         }
      }
   } else {
      /*
       * no browse lines found
       */
      error( INFO, g_display.end_line, utils18b );
   }

   if (!mode.track_path) {
      /*
       * Restore the original directory and update all window headers.
       */
      set_current_directory( cwd );
      set_path( NULL );
   }

   return( rc );
}


/*
 * Name:    browser_parse
 * Purpose: parse the current browser line for filename and line number
 * Author:  Jason Hood
 * Date:    November 17, 2003
 * Passed:  name:  buffer to store filename
 *          rline: pointer to store line number
 *          rcol:  pointer to store column
 * Returns: OK if found, ERROR if not
 * Notes:   Search the first four tokens for a filename; if found, test the
 *           two tokens before or after it for line number and column.
 *          A token is delimited by " \t:;,", which cannot be in the filename.
 *          If the line contains only the filename itself, set rline to zero
 *           (browse files from a list of names, but ignore compiler messages
 *           without line numbers).
 *          Set rcol to -1 if no column was found.
 *
 * 040714:  parse a results window separately (to recognise spaces in names
 *           and pipes [returns name = ""]).
 * 070501:  recognise "[]()" as delimiters (for "filename(linenum)" and
 *           "filename [linenum]" formats).
 */
static int  browser_parse( char *name, long *rline, int *rcol )
{
char *line;
int  len;
char *token[4];
int  t_len[4];
int  tokens;
int  results;
char num[8];
long number;
int  rc;
int  j, k;

   line = (char *)browser_window->ll->line;
   len  = browser_window->ll->len;
   if (is_line_blank( (text_ptr)line, len, TRUE ))
      return( ERROR );

   results = (browser_window->file_info->file_name[0] == '\0' &&
              browser_window->file_info->file_name[1] == RESULTS);
   tokens = 0;
   j = 0;
   while (tokens < 4) {
      while (j < len && bj_isspace( line[j] ))
         ++j;
      if (j < len) {
         token[tokens] = line + j;
         t_len[tokens] = 0;
         while (j < len  &&  line[j] != ':'  &&
                (results  ||  (strchr( " \t;,[]()", line[j] ) == NULL))) {
            ++t_len[tokens];
            ++j;
         }
         ++tokens;
         ++j;
      } else
         break;
   }
   rc = ERROR;
#if !defined( __UNIX__ )
   /*
    * A colon could be a drive specifier - if we have a one letter token,
    * see if it can be combined with the next.  (This needs to be done first
    * on the odd chance the same file is on two drives.)
    */
   for (j = 0; j < tokens - 1; ++j) {
      if (t_len[j] == 1 && token[j][1] == ':') {
         strncpy( name, token[j], 2 + t_len[j+1] );
         name[2 + t_len[j+1]] = '\0';
         rc = file_exists( name );
         if (rc == OK || rc == READ_ONLY) {
            t_len[j] = 2 + t_len[j+1];
            --tokens;
            for (k = j + 1; k < tokens; ++k) {
               token[k] = token[k+1];
               t_len[k] = t_len[k+1];
            }
            rc = OK;
            break;
         } else
            rc = ERROR;
      }
   }
   if (rc == ERROR) {
#endif
   if (results  &&  t_len[0] == 0) {
      *name = '\0';
      j = 0;
      rc = OK;
   } else for (j = 0; j < tokens; ++j) {
      strncpy( name, token[j], t_len[j] );
      name[t_len[j]] = '\0';
      rc = file_exists( name );
      if (rc == OK || rc == READ_ONLY) {
         rc = OK;
         break;
      } else
         rc = ERROR;
   }
#if !defined( __UNIX__ )
   }
#endif

   if (rc == OK) {
      /*
       * If there's only the one token, which takes the whole line, use
       * 0 as the line number; otherwise fail.
       */
      *rline = 0;
      *rcol  = -1;
      if (tokens == 1) {
         if (t_len[0] != len)
            rc = ERROR;
      } else {
         rc = ERROR;
         for (k = 0; k < tokens; ++k) {
            if (k != j && t_len[k] < 8) {
               memcpy( num, token[k], t_len[k] );
               num[t_len[k]] = '\0';
               number = strtol( num, &line, 10 );
               if ((int)(line - num) == t_len[k] && number != 0) {
                  rc = OK;
                  if (*rline == 0)
                     *rline = number;
                  else {
                     *rcol = (int)number - 1;
                     break;
                  }
               }
            }
         }
      }
   }
   return( rc );
}


/*
 * Name:    center_window
 * Purpose: To place the current line or cursor in the center of a window.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 */
int  center_window( TDE_WIN *window )
{
int  center;
int  center_line;
int  diff;
file_infos *file;
register TDE_WIN *win;           /* put window pointer in a register */

   win = window;
   file = win->file_info;
   center = (win->bottom_line + 1 - win->top_line) / 2;
   center_line = win->top_line + center;
   diff = center_line - win->cline;
   if (diff != 0) {
      if (g_status.command == CenterWindow) {
         if (diff < 0 || win->rline + diff <= file->length + 1) {
            if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
               return( ERROR );
            undo_move( win, 0 );
            update_line( win );
            win->cline += diff;
            move_to_line( win, win->rline + diff, TRUE );
            show_curl_line( win );
         }
      } else {
         if (diff < 0 || center_line <= win->top_line + win->rline) {
            win->cline  = center_line;
            file->dirty = LOCAL;
         }
      }
   }
   if (g_status.command == CenterWindow  ||  g_status.command == CenterLine)
      cursor_sync( win );
   return( OK );
}


/*
 * Name:    topbot_line
 * Purpose: move the line to the top or bottom of the window
 * Author:  Jason Hood
 * Date:    October 25, 1999
 * Passed:  window:  pointer to current window
 */
int  topbot_line( TDE_WIN *window )
{
int  new_line;
int  top = window->top_line;

   if (g_status.command == TopLine)
      new_line = top;
   else {
      new_line = window->bottom_line;
      if (new_line > top + window->rline)
         new_line = top + (int)window->rline;
   }
   if (window->cline != new_line) {
      window->cline = new_line;
      window->file_info->dirty = LOCAL;
   }
   cursor_sync( window );
   return( OK );
}


/*
 * Name:    screen_right
 * Purpose: To move the cursor one (half) screen to the right
 * Date:    September 13, 1991
 * Passed:  window:  pointer to current window
 * Notes:   Add 80 columns to the real cursor.  If the cursor is past the
 *          maximum line length then move it back.
 * jmh 980910: if not-eol display, add 79 columns.
 * jmh 991110: made more like _left.
 * jmh 991124: return ERROR if can't go any further.
 * jmh 020906: removed the "horizontal_" prefix;
 *             added the half-screen function.
 */
int  screen_right( TDE_WIN *window )
{
int  screen_width;
int  end;
int  rc = OK;

   screen_width = window->end_col - window->start_col + 1;
   end = MAX_LINE_LENGTH - screen_width;
   if (g_status.command == HalfScreenRight)
      screen_width /= 2;
   else if (mode.show_eol == 2)
      --screen_width;

   if (window->rcol + screen_width < MAX_LINE_LENGTH) {
      undo_move( window, (g_status.command == ScreenRight) ? ScreenLeft
                                                           : HalfScreenLeft );
      window->rcol += screen_width;
      window->bcol += screen_width;
      if (window->bcol > end)
         window->bcol = end;
      window->file_info->dirty = LOCAL | RULER;
   } else {
      if (window->bcol != end) {
         window->bcol = end;
         window->file_info->dirty = LOCAL | RULER;
      } else
         rc = ERROR;
   }
   if (rc == OK)
      check_virtual_col( window, window->rcol, window->ccol );
   cursor_sync( window );
   return( rc );
}


/*
 * Name:    screen_left
 * Purpose: To move the cursor one (half) screen to the left
 * Date:    September 13, 1991
 * Passed:  window:  pointer to current window
 * Notes:   Subtract screen width from the real cursor.  If the cursor is less
 *           than zero then see if bcol is zero.  If bcol is not zero then make
 *           bcol zero.
 * jmh 980910: if not-eol display, include one-column "overlap".
 * jmh 991124: return ERROR if can't go any further.
 * jmh 020906: removed the "horizontal_" prefix;
 *             added half-screen function.
 */
int  screen_left( TDE_WIN *window )
{
int  screen_width;
int  rc = OK;

   screen_width = window->end_col - window->start_col + 1;
   if (g_status.command == HalfScreenLeft)
      screen_width /= 2;
   else if (mode.show_eol == 2)
      --screen_width;

   if (window->rcol - screen_width < 0) {
      if (window->bcol != 0) {
         window->bcol = 0;
         window->file_info->dirty = LOCAL | RULER;
      } else
         rc = ERROR;
   } else {
      undo_move( window, (g_status.command == ScreenLeft) ? ScreenRight
                                                          : HalfScreenRight );
      window->rcol -= screen_width;
      window->bcol -= screen_width;
      if (window->bcol < 0)
         window->bcol = 0;
      window->file_info->dirty = LOCAL | RULER;
   }
   if (rc == OK)
      check_virtual_col( window, window->rcol, window->ccol );
   cursor_sync( window );
   return( rc );
}


/*
 * Name:    goto_top_file
 * Purpose: To move the cursor to the top of the file.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 */
int  goto_top_file( TDE_WIN *window )
{
register TDE_WIN *win;   /* put window pointer in a register */

   win = window;
   if (win->rline != win->cline - win->top_line) {
      if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
         return( ERROR );
      undo_move( win, 0 );
      set_marker( win );                /* remember previous position */
      move_to_line( win, win->cline - win->top_line, TRUE );
      display_current_window( win );
   }
   cursor_sync( win );
   return( OK );
}


/*
 * Name:    goto_end_file
 * Purpose: To move the cursor to the end of the file.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 */
int  goto_end_file( TDE_WIN *window )
{
register TDE_WIN *win;  /* put window pointer in a register */
long new_line;

   win = window;
   new_line = win->file_info->length + 1 - (win->bottom_line - win->cline);
   if (win->rline < new_line) {
      if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
         return( ERROR );
      undo_move( win, 0 );
      set_marker( win );                /* remember previous position */
      move_to_line( win, new_line, TRUE );
      display_current_window( win );
   }
   cursor_sync( win );
   return( OK );
}


/*
 * Name:     goto_position
 * Purpose:  To move the cursor to a particular line in the file
 * Date:     June 5, 1991
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen
 *           August 11, 1997, Jason Hood - can use -ve numbers to go from eof
 *                                         -1 is last line, -2 second last...
 *           August 12, 1997, Jason Hood - reposition for real tabs
 * Passed:   window:  pointer to current window
 *
 * jmh 990921: allow an optional ":col" to move to a column position (0 is at
 *              eol, -1 is last character, etc). The line can be omitted, in
 *              which case the current is used. If the first character is "+"
 *              treat it as a binary offset. Uses strtol() instead of atol() so
 *              hex values can be entered.
 * jmh 990923: corrected binary offset values.
 * jmh 991124: allow column values on TOF/EOF lines.
 */
int  goto_position( TDE_WIN *window )
{
long number;             /* line number selected */
file_infos *file;
TDE_WIN w;
int  rcol;
int  rc;
char answer[MAX_COLS+2];
char temp[MAX_COLS+2];
char *colon;

   /*
    * find out where we are going
    */
   *answer = '\0';
   /*
    * Position (line:col or +offset) :
    */
   if (get_name( find11, window->bottom_line, answer, &h_line ) <= 0)
      return( ERROR );

   if (un_copy_line( window->ll, window, TRUE, TRUE ) == ERROR)
      return( ERROR );

   rc = OK;
   dup_window_info( &w, window );
   file = w.file_info;
   rcol = w.rcol;
   number = strtol( answer, &colon, 0 );

   if (*answer == '+') {
      if (number > w.bin_offset) {
         if (w.rline > file->length)
            number = w.bin_offset;
         else
            while (w.bin_offset + w.ll->len < number  &&
                   inc_line( &w, FALSE )) ;
      } else if (number < w.bin_offset) {
         if (w.bin_offset - number < number) {
            while (w.bin_offset - w.ll->prev->len > number  &&
                   dec_line( &w, FALSE )) ;
         } else {
            first_line( &w );
            while (w.bin_offset + w.ll->len < number)
               inc_line( &w, FALSE );
         }
      }
      rcol = (int)(number - w.bin_offset);
      if (w.rline == file->length  &&  rcol > w.ll->len)
         rcol = w.ll->len;
      if (file->inflate_tabs)
         rcol = detab_adjust_rcol( w.ll->line, rcol, file->ptab_size );

   } else {
      if (number < 0)
         number += file->length + 1;
      if ((number == 0 && *colon == ':')  ||
          move_to_line( &w, number, FALSE )) {
         if (*colon == ':') {
            rcol = atoi( colon + 1 );
            if (rcol > MAX_LINE_LENGTH)
               rcol = w.rcol;
            else if (rcol <= 0)
               rcol += find_end( w.ll->line, w.ll->len,
                                 file->inflate_tabs, file->ptab_size );
            else
               rcol--;
         }
      } else
         rc = ERROR;
   }
   if (rc == OK) {
      undo_move( window, 0 );
      set_marker( window );             /* remember previous position */
      w.rcol = rcol;
      move_display( window, &w );
   } else {
      /*
       * out of range.  must be in the range 1 -
       */
      sprintf( temp, "%s%ld", find12, file->length );
      error( WARNING, window->bottom_line, temp );
   }
   return( rc );
}


/*
 * Name:     set_marker
 * Purpose:  To set file marker
 * Date:     December 28, 1991
 * Passed:   window:  pointer to current window
 * Modified: August 9, 1997, Jason Hood - added block markers
 *           September 13, 1997, Jason Hood - added previous position
 *           July 18, 1998, Jason Hood - added macro marker
 *           July 28, 1998, Jason Hood - removed block markers (see goto_marker)
 *
 * jmh 021028: store cline.
 */
int  set_marker( TDE_WIN *window )
{
register MARKER *marker;        /* put the marker in a register */

   if (g_status.command >= SetMark1 && g_status.command <= SetMark3)
      marker = &window->file_info->marker[g_status.command - SetMark1 + 1];
   else if (g_status.command == MacroMark)
      marker = &g_status.macro_mark;
   else
      marker = &window->file_info->marker[0];

   marker->rline  = window->rline;
   marker->cline  = window->cline;
   marker->rcol   = window->rcol;
   marker->ccol   = window->ccol;
   marker->bcol   = window->bcol;
   marker->marked = TRUE;
   return( OK );
}


/*
 * Name:     goto_marker
 * Purpose:  To goto a file marker
 * Date:     December 28, 1991
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen
 * Modified: August 9, 1997, Jason Hood - added block markers
 *           August 11, 1997, Jason Hood - move cursor to line if on screen
 * Passed:   window:  pointer to current window
 *
 * jmh 980728: Create block markers on the fly.
 * jmh 980801: If already at the block marker, move to the other one (ie. if
 *              at top-left corner, move to top-right; if at bottom-right,
 *              move to bottom-left).
 * jmh 021028: restore cline;
 *             default to previous position (due to isearch).
 * jmh 021210: set cline correctly for move to first page block.
 * jmh 030224: fixed cline again.
 * jmh 030327: only use the marker's cline if the rline is off-screen.
 */
int  goto_marker( TDE_WIN *window )
{
int  m;
file_infos *file;
MARKER *marker, prev;
register TDE_WIN *win;   /* put window pointer in a register */
TDE_WIN w;
int  rc;

   win  = window;
   file = win->file_info;
   m = -1;
   if (g_status.command >= GotoMark1 && g_status.command <= GotoMark3) {
      m = g_status.command - GotoMark1 + 1;
      marker = &file->marker[m];
   } else if (g_status.command == MacroMark)
      marker = &g_status.macro_mark;
   else if (g_status.command == BlockBegin || g_status.command == BlockEnd) {
      marker = &prev;
      if (g_status.marked && g_status.marked_file == file) {
         prev.marked = TRUE;
         prev.ccol   = win->ccol;
         prev.bcol   = win->bcol;
         if (g_status.command == BlockBegin) {
            prev.rline = file->block_br;
            prev.rcol  = (win->rline == file->block_br &&
                          win->rcol  == file->block_bc) ? file->block_ec
                                                        : file->block_bc;
         } else {
            prev.rline = file->block_er;
            prev.rcol  = (win->rline == file->block_er &&
                          win->rcol  == file->block_ec) ? file->block_bc
                                                        : file->block_ec;
         }
         prev.cline = win->cline;
      } else
         prev.marked = FALSE;
   } else {
      prev   = file->marker[0];
      marker = &prev;
   }

   if (marker->marked) {
      if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
         return( ERROR );

      if (marker->rline != win->rline && marker->rcol != win->rcol)
         undo_move( win, 0 );

      /*
       * Remember the previous position, but not if it's a macro mark
       */
      if (g_status.command != MacroMark)
         set_marker( win );

      if (marker->rline > file->length)
         marker->rline = file->length;
      if (marker->rline < 1)
         marker->rline = 1;

      dup_window_info( &w, win );
      move_to_line( &w, marker->rline, FALSE );
      w.rcol    = marker->rcol;
      win->ccol = marker->ccol;
      if (win->bcol != marker->bcol) {
         win->bcol = marker->bcol;
         file->dirty = LOCAL | RULER;
      }
      move_display( win, &w );
      if (win->cline == w.cline  &&  file->dirty)
         check_cline( win, marker->cline );
      rc = OK;
   } else {
      /*
       * Don't display a warning for the block markers and previous position
       */
      if (m > 0) {
         if (m == 10)
            m = 0;
         utils13[UTILS13_NO_SLOT] = (char)('0' + m);
         /*
          * marker not set
          */
         error( WARNING, win->bottom_line, utils13 );
      }
      rc = ERROR;
   }
   return( rc );
}



/***************************  Drawing mode  **************************/


#define G_V (G_UP|G_DN)
#define G_H (G_LF|G_RT)

/*
 * Given a character in graphic_char[], return its component lines.
 */
static const char g_flag[] = { G_V, G_UP|G_RT, G_H|G_UP, G_UP|G_LF,
                                     G_V|G_RT, G_V|G_H,   G_V|G_LF,
                                    G_DN|G_RT, G_H|G_DN, G_DN|G_LF, G_H };

/*
 * Given component lines, return a character in graphic_char[].
 */
static const char g_char[] = { 0, 0, 0, VERTICAL_LINE,
                               0, CORNER_RIGHT_DOWN, CORNER_RIGHT_UP, RIGHT_T,
                               0, CORNER_LEFT_DOWN, CORNER_LEFT_UP, LEFT_T,
                               HORIZONTAL_LINE, BOTTOM_T, TOP_T, INTERSECTION };


/*
 * Name:    get_g_flag
 * Purpose: determine the line drawing components of a character
 * Author:  Jason Hood
 * Date:    October 19, 1999
 * Passed:  c:  character being tested
 *          gc: graphic set being used
 * Returns: components of c
 * Notes:   ASCII and block sets need to be treated separately.
 */
static char get_g_flag( char c, const char* gc )
{
char *pos;
char f = 0;

   if (gc == graphic_char[0]) {                 /* ASCII */
      if (c == gc[INTERSECTION])
         f = G_H | G_V;
      else if (c == gc[VERTICAL_LINE])
         f = G_V;
      else if (c == gc[HORIZONTAL_LINE])
         f = G_H;

   } else if (gc == graphic_char[5]) {          /* Block */
      if (c == gc[5])
         f = G_H | G_V;
      else if (c == gc[8])
         f = G_H;

   } else if ((pos = memchr( gc, c, 11 )) != NULL)
      f = g_flag[(int)(pos - (char *)gc)];

   return( f );
}


/*
 * Name:    get_g_char
 * Purpose: to return a character based on line components
 * Author:  Jason Hood
 * Date:    October 21, 1999
 * Passed:  c:  components being requested
 *          gc: graphic set being used
 * Returns: graphic character
 * Notes:   Block characters are still a nuisance.
 */
static char get_g_char( char c, const char *gc )
{
static const char block_char[] = { 0, 0, 0, 5, 0, 8, 5, 5,
                                   0, 8, 5, 5, 8, 8, 5, 5 };

   if (gc == graphic_char[5])
      return( gc[(int)block_char[(int)c]] );

   return( gc[(int)g_char[(int)c]] );
}


/*
 * Name:    set_vert_g_char
 * Purpose: draw a vertical line
 * Author:  Jason Hood
 * Date:    October 21, 1999
 * Passed:  win:  pointer to current window
 *          d:    direction to draw
 * Notes:   call before (G_UP or G_DN) and after (~G_DN or ~G_UP) the movement.
 */
static void set_vert_g_char( TDE_WIN *win, char d )
{
int  rcol = win->rcol;
const char *gc = graphic_char[abs( mode.graphics ) - 1];
int  c0, c1, c2;
text_ptr b = g_status.line_buff;
int  len;

   copy_line( win->ll, win, TRUE );
   len = g_status.line_buff_len;
   c0 = c1 = c2 = 0;
   if (rcol < len)
      c1 = get_g_flag( b[rcol], gc );
   if (rcol > 0 && rcol-1 < len)
      c0 = get_g_flag( b[rcol-1], gc );
   if (rcol+1 < len)
      c2 = get_g_flag( b[rcol+1], gc );

   if (d == G_UP || d == G_DN) {
      c1 |= d;
      if (c0 & G_RT) c1 |= G_LF;
      if (c2 & G_LF) c1 |= G_RT;
      if (c1 == d)   c1  = G_V;

   } else {
      if (c1 & G_H) {
         c1 |= ~d;
         if (!(c0 & G_RT)) c1 &= ~G_LF;
         if (!(c2 & G_LF)) c1 &= ~G_RT;
      }
   }

   if (c1 != 0) {
      if (rcol >= len) {
         memset( b + len, ' ', rcol - len );
         g_status.line_buff_len = rcol + 1;
      }
      b[rcol] = get_g_char( (char)c1, gc );
      win->ll->type |= DIRTY;
      win->file_info->dirty = GLOBAL;
      show_changed_line( win );
   }
}


/*
 * Name:    set_horz_g_char
 * Purpose: draw a horizontal line
 * Author:  Jason Hood
 * Date:    October 21, 1999
 * Passed:  win:  pointer to current window
 *          d:    direction to draw
 * Notes:   call before the movement. Have to treat block graphics separately.
 */
static void set_horz_g_char( TDE_WIN *win, char d )
{
int  rcol = win->rcol;
const char *gc = graphic_char[abs( mode.graphics ) - 1];
text_ptr b = g_status.line_buff;
char f0, f1, f2, t0, t1, t2;
text_ptr s;
int len;
int o;

   copy_line( win->ll, win, TRUE );
   len = g_status.line_buff_len;
   f0 = f1 = f2 = 0;
   t0 = t1 = t2 = 0;

   o = (d == G_LF) ? -1 : 1;

   if (rcol < len)
      f1 = get_g_flag( b[rcol], gc );
   if (rcol+o < len)
      t1 = get_g_flag( b[rcol+o], gc );
   if (win->rline > 1) {
      len = win->ll->prev->len;
      s = detab_a_line( win->ll->prev->line, &len,
                        win->file_info->inflate_tabs,
                        win->file_info->ptab_size );
      if (rcol < len)
         f0 = get_g_flag( s[rcol], gc );
      if (rcol+o < len)
         t0 = get_g_flag( s[rcol+o], gc );
   }
   len = win->ll->next->len;
   if (len != EOF) {
      s = detab_a_line( win->ll->next->line, &len,
                        win->file_info->inflate_tabs,
                        win->file_info->ptab_size );
      if (rcol < len)
         f2 = get_g_flag( s[rcol], gc );
      if (rcol+o < len) {
         t2 = get_g_flag( s[rcol+o], gc );
         if (gc == graphic_char[5] && t2 == G_H)
            t2 |= G_UP;
      }
   }

   f1 |= d;
   if (f0 & G_DN) f1 |= G_UP;
   if (f2 & G_UP) f1 |= G_DN;
   if (f1 == d)   f1  = G_H;

   if (rcol >= g_status.line_buff_len) {
      memset( b + g_status.line_buff_len, ' ',
              rcol - g_status.line_buff_len );
      g_status.line_buff_len = rcol + 1;
   }
   b[rcol] = get_g_char( f1, gc );

   if (t1 & G_V) {
      t1 |= (d == G_LF) ? G_RT : G_LF;
      if (!(t0 & G_DN)) t1 &= ~G_UP;
      if (!(t2 & G_UP)) t1 &= ~G_DN;
      b[rcol+o] = get_g_char( t1, gc );
   }
   win->ll->type |= DIRTY;
   win->file_info->modified = TRUE;
   win->file_info->dirty = GLOBAL;
   show_changed_line( win );
}
