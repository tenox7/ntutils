/*******************  start of original comments  ********************/
/*
 * Written by Douglas Thomson (1989/1990)
 *
 * This source code is released into the public domain.
 */

/*
 * Name:    dte - Doug's Text Editor program - window module
 * Purpose: This file contains the code associated with opening and sizing
 *           windows, and also displaying the help window.
 * File:    window.c
 * Author:  Douglas Thomson
 * System:  this file is intended to be system-independent
 * Date:    October 12, 1989
 */
/*********************  end of original comments   ********************/


/*
 * The window routines have been EXTENSIVELY rewritten.  Some routines were
 * changed so only one logical function is carried out, eg. 'initialize_window'.
 * I like the Microsoft way of resizing windows - just press the up and down
 * arrows to adjust the window to desired size.  I also like pressing one key
 * to change windows.  All of which are implemented in TDE.
 *
 * In TDE, version 1.4, I added support for vertical windows.
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


int  make_window;       /* tell other functions not to redraw */

extern TDE_WIN    *results_window; /* defined in findrep.c */
extern file_infos *results_file;

extern TDE_WIN    *browser_window; /* defined in movement.c */

static TDE_WIN *find_make_window( TDE_WIN * );


/*
 * Name:    next_window
 * Purpose: To move to the next visible/hidden window.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   Start with current window.  If next window exists then go to it
 *           else go to the first (top) window on screen.
 *          When I added vertical windows, finding the "correct" next
 *           window became extremely, unnecessarily, unmanageably complicated.
 *           let's just use a simple procedure to find the first available,
 *           visible, next window.
 *
 * jmh 990502: incorporated next_hidden_window().
 */
int  next_window( TDE_WIN *window )
{
register TDE_WIN *wp;
int  change;
int  visibility;

   change = FALSE;
   visibility = (g_status.command == NextWindow);

   /*
    * start with current window and look for the next appropriate window
    */
   wp = window->next;
   while (wp != NULL) {
      if (wp->visible == visibility) {
         change = TRUE;
         break;
      }
      wp = wp->next;
   }

   /*
    * if we haven't found a window yet, go to the beginning of
    *  the list until we find a visible/hidden window.
    */
   if (!change) {
      wp = g_status.window_list;
      while (wp != window) {
         if (wp->visible == visibility) {
            change = TRUE;
            break;
         }
         wp = wp->next;
      }
   }
   if (change)
      change_window( window, wp );

   return( OK );
}


/*
 * Name:    prev_window
 * Purpose: To move to the previous visible/hidden window.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   Start with current window.  If previous window exists then go to
 *           it else go to the last (bottom) window on screen.  Opposite of
 *           next_window.
 *          when I added vertical windows, finding the "correct" previous
 *           window became extremely, unnecessarily, unmanageably complicated.
 *           let's just use a simple procedure to find the first available,
 *           visible, previous window.
 *
 * jmh 990502: incorporated prev_hidden_window().
 */
int  prev_window( TDE_WIN *window )
{
register TDE_WIN *wp;
register TDE_WIN *win;
int  visibility;

   visibility = (g_status.command == PreviousWindow);

   /*
    * start with current window and look for the previous appropriate window
    */
   win = window->prev;
   while (win != NULL) {
      if (win->visible == visibility)
         break;
      win = win->prev;
   }

   /*
    * if we haven't found an appropriate window yet, keep searching right
    *  to the end of the list, remembering the last one found
    */
   if (win == NULL) {
      wp = window->next;
      while (wp != NULL) {
         if (wp->visible == visibility)
            win = wp;
         wp = wp->next;
      }
   }
   if (win)
      change_window( window, win );

   return( OK );
}


/*
 * Name:    goto_window
 * Purpose: To select a new window by number or file name
 * Author:  Jason Hood
 * Date:    May 2, 1999
 * Passed:  window:  pointer to current window
 *
 * jmh 060829: blank answer will bring up a window list.
 */
int  goto_window( TDE_WIN *window )
{
char answer[MAX_COLS+2];
TDE_WIN *win;
int  rc;
LIST *wlist;
DIRECTORY dir;
char path[PATH_MAX];
char *name;
int  len;

   *answer = '\0';
   rc = get_name( win20, window->bottom_line, answer, &h_win );
   if (rc > 0) {
      rc = find_window( &win, answer, window->bottom_line );
      if (rc == OK)
         change_window( window, win );

   } else if (rc == 0) {
      wlist = malloc( g_status.window_count * sizeof(*wlist) );
      if (wlist == NULL)
         return( ERROR );

      len = numlen( g_status.window_count );
      for (win = g_status.window_list; win != NULL; win = win->next) {
         if (win->title)
            name = win->title;
         else if (*win->file_info->file_name == '\0')
            name = win->file_info->file_name + 1;
         else
            name = relative_path( path, win, FALSE );
         reduce_string( path, name, g_display.ncols-4-4-8-len, MIDDLE );
         sprintf( answer, "%c %*d%c %c%c%c %s",
                  win == window ? '!' : (win->visible ? '+' : '-'),
                  len, win->file_info->file_no, win->letter,
                  win->file_info->read_only ? '!' : ' ',
                  win->file_info->scratch   ? '#' : ' ',
                  win->file_info->modified  ? '*' : ' ',
                  path );
         wlist[rc].name = my_strdup( answer );
         if (wlist[rc].name == NULL)
            break;
         wlist[rc].color = Color( Dialog );
         wlist[rc].data  = win;
         ++rc;
      }
      shell_sort( wlist, rc );
      setup_directory_window( &dir, wlist, rc );
      write_directory_list( wlist, &dir );
      xygoto( -1, -1 );
      rc = select_file( wlist, "", &dir );
      redraw_screen( window );
      if (rc == OK)
         change_window( window, wlist[dir.select].data );
      for (len = g_status.window_count; --len >= 0;)
         my_free( wlist[len].name );
      free( wlist );
   }

   return( rc );
}


/*
 * Name:    change_window
 * Purpose: helper function to change to a new window
 * Date:    May 2, 1999
 * Passed:  window:  current window
 *          wp:      new window
 *
 * jmh 020730: call setup_window().
 * jmh 021021: track the path.
 */
void change_window( TDE_WIN *window, TDE_WIN *wp )
{
int  redraw = FALSE;

   if (window == wp)
      return;

   un_copy_line( window->ll, window, TRUE, TRUE );
   g_status.current_window = wp;
   g_status.current_file   = wp->file_info;

   if (wp->visible)
      show_tab_modes( );

   else {
      wp->cline      += window->top_line - wp->top_line;
      wp->top         = window->top;
      wp->bottom_line = window->bottom_line;
      wp->left        = window->left;
      wp->end_col     = window->end_col;
      wp->vertical    = window->vertical;
      wp->ruler       = window->ruler;
      setup_window( wp );
      check_cline( wp, wp->cline );
      check_virtual_col( wp, wp->rcol, wp->ccol );
      wp->visible = TRUE;
      if (!make_window) {
         window->visible = FALSE;
         if (diff.defined  &&  (diff.w1 == window  ||  diff.w2 == window))
            diff.defined = FALSE;
         redraw = TRUE;
      }
   }

   if (mode.track_path)
      set_path( wp );

   if (redraw)
      redraw_current_window( wp );
#if defined( __WIN32__ )
   else if (!make_window)
      show_window_fname( wp );          /* update console title */
#endif
}


/*
 * Name:    split_horizontal
 * Purpose: To split screen horizontally at the cursor.
 * Date:    June 5, 1991
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen
 * Passed:  window:  pointer to current window
 * Notes:   split the screen horizontally at the cursor position.
 *
 * jmh 991109: the cursor position is the new header.
 * jmh 020823: move the old window up a line instead of scrolling.
 * jmh 030227: added NewHorizontal function to load the next file instead of
 *              splitting the current.
 * jmh 030323: removed NewHorizontal;
 *             added Half version to split the window in half, leaving the
 *              cursor where it is.
 * jmh 030324: do an "actual" split - the top window has everything before
 *              the cursor, the bottom everything after (ie. the top and
 *              bottom lines do not change).
 */
int  split_horizontal( TDE_WIN *window )
{
register TDE_WIN *wp;
register TDE_WIN *win;  /* register pointer for window */
TDE_WIN *temp;
file_infos *file;       /* file structure for file belonging to new window */
int  half;
int  top_line;
int  change;
const char *errmsg;
int  rc;

   rc = ERROR;
   win = window;
   if (win != NULL) {

      /*
       * check that there is room for the window
       */
      errmsg = NULL;
      half = (g_status.command == SplitHalfHorizontal);
      top_line = (half) ? ((win->top_line + win->bottom_line + 1) / 2)
                        : win->cline;

      if (top_line == win->top_line)
         /*
          * move cursor down first
          */
         errmsg = win1a;

      else if (top_line == win->bottom_line)
         /*
          * move cursor up first
          */
         errmsg = win1;

      if (errmsg) {
         if (half)
            /*
             * window too small
             */
            errmsg = win1b;
         error( WARNING, win->bottom_line, errmsg );

      } else {
         file = win->file_info;

         assert( file != NULL );

         rc = create_window( &temp, top_line, win->bottom_line,
                             win->left, win->end_col, file );

         if (rc == OK) {
            wp = temp;
            wp->visible = FALSE;
            un_copy_line( win->ll, win, TRUE, TRUE );
            wp->visible = TRUE;
            /*
             * set up the new cursor position as appropriate
             */
            wp->rcol       = win->rcol;
            wp->ccol       = win->ccol;
            wp->bcol       = win->bcol;
            wp->rline      = win->rline;
            wp->bin_offset = win->bin_offset;
            wp->ll         = win->ll;
            wp->syntax     = win->syntax;
            wp->vertical   = win->vertical;

            if (win->title)
               wp->title = my_strdup( win->title );

            change = win->cline - top_line;
            if (half  &&  change) {
               if (change < 0) {
                  if (file->length < top_line - win->top_line - 1) {
                     check_cline( wp, wp->cline + win->cline - win->top_line );
                     file->dirty = LOCAL;
                  } else {
                     move_to_line( wp, wp->rline - change + wp->ruler, TRUE );
                     inc_line( wp, TRUE );
                  }
               } else {
                  move_to_line( win, win->rline - change - 1, TRUE );
                  wp->cline  = win->cline;
                  win->cline = top_line - 1;
               }
            } else {
               --win->cline;
               dec_line( win, TRUE );
               inc_line( wp, TRUE );
               if (wp->ruler)
                  inc_line( wp, TRUE );
            }

            /*
             * record that the current window has lost some lines from
             *  the bottom for the new window, and adjust its page size
             *  etc accordingly.
             */
            win->bottom_line = top_line - 1;
            setup_window( win );
            show_line_col( win );
            show_curl_line( win );

            if (file->dirty) {
               redraw_current_window( wp );
               file->dirty = FALSE;
            } else {
               show_window_header( wp );
               show_ruler( wp );
               if (wp->ll->next == NULL)
                  show_eof( wp );
               else
                  show_curl_line( wp );
            }

            /*
             * the new window becomes the current window.
             */
            g_status.current_window = wp;
            show_window_count( );
         }
      }
   }
   return( rc );
}


/*
 * Name:    make_horizontal
 * Purpose: To split screen horizontally at the cursor.
 * Date:    June 5, 1991
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen
 * Passed:  window:  pointer to current window
 * Notes:   split the screen horizontally at the cursor position.
 *          the next hidden window becomes visible, or the next
 *           file is loaded.
 *
 * jmh 030323: replaced NewHorizontal with MakeHorizontal to resize the
 *              current window.
 */
int  make_horizontal( TDE_WIN *window )
{
register TDE_WIN *wp;
register TDE_WIN *win;  /* register pointer for window */
int  half;
int  top_line;
int  change;
const char *errmsg;
int  rc;

   rc = ERROR;
   win = window;
   if (win != NULL) {

      /*
       * check that there is room for the window
       */
      errmsg = NULL;
      half = (g_status.command == MakeHalfHorizontal);
      top_line = (half) ? ((win->top_line + win->bottom_line + 1) / 2)
                        : win->cline;

      if (top_line == win->top_line)
         /*
          * move cursor down first
          */
         errmsg = win1a;

      else if (top_line == win->bottom_line)
         /*
          * move cursor up first
          */
         errmsg = win1;

      if (errmsg) {
         if (half)
            /*
             * window too small
             */
            errmsg = win1b;
         error( WARNING, win->bottom_line, errmsg );

      } else {
         wp = find_make_window( win );
         if (wp != NULL) {
            un_copy_line( win->ll, win, TRUE, TRUE );

            /*
             * record that the current window has lost some lines from
             *  the bottom for the new window, and adjust its page size
             *  etc accordingly.
             */
            change = -1;
            if (half) {
               if (win->cline >= top_line)
                  change = win->bottom_line - win->cline;
            } else {
               dec_line( win, TRUE );
               --win->cline;
            }
            win->bottom_line = top_line - 1;
            setup_window( win );
            if (change >= 0) {
               check_cline( win, win->bottom_line - change );
               display_current_window( win );
            } else {
               show_line_col( win );
               show_curl_line( win );
            }

            change  = wp->cline - wp->top_line;
            wp->top = top_line;
            if (wp->bottom_line == top_line + 1)
               wp->ruler = FALSE;
            setup_window( wp );
            if (wp->cline < wp->top_line)
               check_cline( wp, wp->top_line + change );
            wp->visible = TRUE;
            redraw_current_window( wp );
         }
      }
   }
   return( rc );
}


/*
 * Name:    split_vertical
 * Purpose: To split screen vertically at the cursor.
 * Date:    June 5, 1991
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen
 * Passed:  window:  pointer to current window
 * Notes:   split the screen vertically at the cursor position.
 *
 * Change:  Use of new function: get_next_letter( )
 *
 * jmh 030227: added NewVertical function to load the next file instead of
 *              splitting the current.
 * jmh 030324: removed NewVertical;
 *             added Half version to split the window in half, leaving the
 *              cursor where it is.
 * jmh 030325: do an "actual" split - the left window has everything before
 *              the cursor, the right everything after (ie. the left and
 *              right columns do not change).
 */
int  split_vertical( TDE_WIN *window )
{
register TDE_WIN *wp;
register TDE_WIN *win;  /* register pointer for window */
TDE_WIN *temp;
file_infos *file;       /* file structure for file belonging to new window */
int  half;
int  left;
int  change;
const char *errmsg;
int  rc;

   rc = ERROR;
   win = window;
   if (win != NULL) {

      /*
       * check that there is room for the window
       */
      errmsg = NULL;
      half = (g_status.command == SplitHalfVertical);
      left = (half) ? ((win->left + win->end_col + 1) / 2)
                    : win->ccol;

      if (win->left + 15 > left)
         /*
          * move cursor right first
          */
         errmsg = win2;

      else if (win->end_col - 15 < left)
         /*
          * move cursor left first
          */
         errmsg = win3;

      if (errmsg) {
         if (half)
            /*
             * window too small
             */
            errmsg = win1b;
         error( WARNING, win->bottom_line, errmsg );

      } else {
         file = win->file_info;

         assert( file != NULL );

         rc = create_window( &temp, win->top, win->bottom_line,
                             left+1, win->end_col, file );

         if (rc == OK) {
            wp = temp;
            wp->visible = FALSE;
            un_copy_line( win->ll, win, TRUE, TRUE );
            wp->visible = TRUE;
            /*
             * set up the new cursor position as appropriate
             */
            wp->rline      = win->rline;
            wp->bin_offset = win->bin_offset;
            wp->ll         = win->ll;
            wp->cline      = win->cline;
            wp->rcol       = win->rcol;
            wp->bcol       = win->bcol + left - win->left + 1;
            wp->syntax     = win->syntax;

            if (win->title)
               wp->title = my_strdup( win->title );

            change = win->ccol - left;
            if (half  &&  change) {
               if (change > 0) {
                  wp->ccol = win->ccol;
                  win->rcol -= change + 1;
                  win->ccol = left - 1;
               } else {
                  wp->rcol -= change - 1;
                  if (mode.line_numbers)
                     wp->rcol += file->len_len;
               }
            } else {
               --win->ccol;
               --win->rcol;
               ++wp->rcol;
               if (mode.line_numbers)
                  wp->rcol += file->len_len;
            }

            /*
             * record that the current window has lost some columns from
             *  the right for the new window
             */
            win->end_col  = left - 1;
            win->vertical = TRUE;
            wp->vertical  = TRUE;
            redraw_current_window( win );
            redraw_current_window( wp );

            /*
             * the new window becomes the current window.
             */
            g_status.current_window = wp;
            show_window_count( );
         }
      }
   }
   return( rc );
}


/*
 * Name:    make_vertical
 * Purpose: To split screen vertically at the cursor.
 * Date:    June 5, 1991
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen
 * Passed:  window:  pointer to current window
 * Notes:   split the screen vertically at the cursor position.
 *
 * jmh 030324: replaced NewVertical with MakeVertical to resize the
 *              current window;
 */
int  make_vertical( TDE_WIN *window )
{
register TDE_WIN *wp;
register TDE_WIN *win;  /* register pointer for window */
int  half;
int  left;
int  change;
const char *errmsg;
int  rc;

   rc = ERROR;
   win = window;
   if (win != NULL) {

      /*
       * check that there is room for the window
       */
      errmsg = NULL;
      half = (g_status.command == MakeHalfVertical);
      left = (half) ? ((win->left + win->end_col + 1) / 2)
                    : win->ccol;

      if (win->left + 15 > left)
         /*
          * move cursor right first
          */
         errmsg = win2;

      else if (win->end_col - 15 < left)
         /*
          * move cursor left first
          */
         errmsg = win3;

      if (errmsg) {
         if (half)
            /*
             * window too small
             */
            errmsg = win1b;
         error( WARNING, win->bottom_line, errmsg );

      } else {
         wp = find_make_window( win );
         if (wp != NULL) {
            un_copy_line( win->ll, win, TRUE, TRUE );

            /*
             * record that the current window has lost some columns from
             *  the right for the new window
             */
            change = -1;
            if (half) {
               if (win->ccol >= left)
                  change = win->end_col - win->ccol;
            } else {
              --win->ccol;
              --win->rcol;
            }
            win->end_col = left - 1;
            if (change >= 0) {
               win->ccol = win->end_col - change;
               if (win->ccol < win->start_col)
                  win->ccol = win->start_col;
               win->bcol = win->rcol - (win->ccol - win->start_col);
            }
            win->vertical = TRUE;
            redraw_current_window( win );

            change = wp->ccol - wp->start_col;
            wp->left = left + 1;
            setup_window( wp );
            if (wp->ccol < wp->start_col)
               check_virtual_col( wp, wp->rcol, wp->start_col + change );
            else
               wp->bcol = wp->rcol - (wp->ccol - wp->start_col);
            wp->vertical = TRUE;
            wp->visible  = TRUE;
            redraw_current_window( wp );
            wp->file_info->dirty = FALSE;
         }
      }
   }
   return( rc );
}


/*
 * Name:    show_vertical_separator
 * Purpose: To separate vertical screens
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 */
void show_vertical_separator( TDE_WIN *window )
{
int  col;

   col = window->left - 1;
   if (col >= 0)
      c_repeat( VERTICAL_CHAR, window->top - window->bottom_line - 1,
                col, window->top, Color( Head ) );
}


/*
 * Name:    balance_horizontal
 * Purpose: make all windows the same height
 * Author:  Jason Hood
 * Date:    July 24, 2005
 * Passed:  window:  pointer to current window
 * Notes:   all windows that have the same left and right edge as the current
 *           window will be made the same height.
 */
int  balance_horizontal( TDE_WIN *window )
{
TDE_WIN *vis_win[20];
int  visible = 0;
TDE_WIN *win;
int  top, bot, hyt;
int  i, j, rem;

   /*
    * find all the visible windows that share edges with this window
    */
   top = g_display.mode_line;
   bot = 0;
   for (win = g_status.window_list; win != NULL; win = win->next) {
      if (win->visible  &&  win->left    == window->left  &&
                            win->end_col == window->end_col) {
         if (visible == 20) {
            /*
             * too many windows
             */
            error( WARNING, window->bottom_line, win5 );
            return( ERROR );
         }
         /*
          * order windows top to bottom
          */
         for (i = 0; i < visible; ++i) {
            if (win->top < vis_win[i]->top) {
               memmove( vis_win+i+1, vis_win+i, (visible-i)*sizeof(TDE_WIN*) );
               break;
            }
         }
         vis_win[i] = win;
         visible++;
      }
   }
   /*
    *   ÚÄÂÄ¿   remove windows that don't actually join with the current
    *   ³1³2³   window.  If the current window is 4 or 6 the above loop will
    *   ÃÄÁÄ´   give us 1, 4 & 6, but only 4 & 6 should be balanced.
    *   ³ 3 ³
    *   ÃÄÂÄ´
    *   ³4³5³
    *   ÃÄÅÄ´
    *   ³6³7³
    *   ÀÄÁÄÙ
    */
   for (i = 0; vis_win[i] != window;)
      ++i;
   for (j = i+1; j < visible; ++j) {
      if (vis_win[j]->top - 1 != vis_win[j-1]->bottom_line) {
         visible = j;
         break;
      }
   }
   for (j = i; j > 0; --j) {
      if (vis_win[j-1]->bottom_line + 1 != vis_win[j]->top) {
         memcpy( vis_win, vis_win + j, (visible - j) * sizeof(TDE_WIN*) );
         visible -= j;
         break;
      }
   }

   if (visible > 1) {
      top = vis_win[0]->top;
      bot = vis_win[visible-1]->bottom_line;
      hyt = (bot - top + visible) / visible;
      rem = (bot - top + 1) % visible;
      for (i = 1; i < visible; ++i) {
         top += hyt;
         vis_win[i-1]->bottom_line = top - 1;
         vis_win[i]->top = top;
         if (--rem == 0)
            --hyt;
      }
      for (i = 0; i < visible; ++i) {
         if (mode.ruler)
            vis_win[i]->ruler = (vis_win[i]->top+1 != vis_win[i]->bottom_line);
         setup_window( vis_win[i] );
         check_cline( vis_win[i], vis_win[i]->cline );
         redraw_current_window( vis_win[i] );
      }
   }

   return( OK );
}


/*
 * Name:    balance_vertical
 * Purpose: make all windows the same width
 * Author:  Jason Hood
 * Date:    July 24, 2005
 * Passed:  window:  pointer to current window
 * Notes:   all windows that have the same top and bottom edge as the current
 *           window will be made the same width.
 */
int  balance_vertical( TDE_WIN *window )
{
TDE_WIN *vis_win[20];
int  visible = 0;
TDE_WIN *win;
int  lft, ryt, wid;
int  i, j, rem;

   /*
    * find all the visible windows that share edges with this window
    */
   lft = g_display.ncols;
   ryt = 0;
   for (win = g_status.window_list; win != NULL; win = win->next) {
      if (win->visible  &&  win->top == window->top  &&
                            win->bottom_line == window->bottom_line) {
         if (visible == 20) {
            /*
             * too many windows
             */
            error( WARNING, window->bottom_line, win5 );
            return( ERROR );
         }
         /*
          * order windows left to right
          */
         for (i = 0; i < visible; ++i) {
            if (win->left < vis_win[i]->left) {
               memmove( vis_win+i+1, vis_win+i, (visible-i)*sizeof(TDE_WIN*) );
               break;
            }
         }
         vis_win[i] = win;
         visible++;
      }
   }
   /*
    * ÚÄÂÄÂÄÂÄ¿ remove windows that don't actually join with the current
    * ³1³ ³4³6³ window.  If the current window is 4 or 6 the above loop
    * ÃÄ´3ÃÄÅÄ´ will give us 1, 4 & 6, but only 4 & 6 should be balanced.
    * ³2³ ³5³7³
    * ÀÄÁÄÁÄÁÄÙ
    */
   for (i = 0; vis_win[i] != window;)
      ++i;
   for (j = i+1; j < visible; ++j) {
      if (vis_win[j]->left - 2 != vis_win[j-1]->end_col) {
         visible = j;
         break;
      }
   }
   for (j = i; j > 0; --j) {
      if (vis_win[j-1]->end_col + 2 != vis_win[j]->left) {
         memcpy( vis_win, vis_win + j, (visible - j) * sizeof(TDE_WIN*) );
         visible -= j;
         break;
      }
   }

   if (visible > 1) {
      lft = vis_win[0]->left;
      ryt = vis_win[visible-1]->end_col;
      wid = (ryt - lft + 1 + visible) / visible;
      rem = (ryt - lft + 1 - (visible - 1)) % visible;
      for (i = 1; i < visible; ++i) {
         lft += wid;
         vis_win[i-1]->end_col = lft - 2;
         vis_win[i]->left = lft;
         if (--rem == 0)
            --wid;
      }
      for (i = 0; i < visible; ++i) {
         setup_window( vis_win[i] );
         check_virtual_col( vis_win[i], vis_win[i]->rcol, vis_win[i]->ccol );
         redraw_current_window( vis_win[i] );
      }
   }

   return( OK );
}


/*
 * Name:    size_window
 * Purpose: To change the size of the current and one other window.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   Use the Up and Down arrow keys to make the current window
 *           bigger or smaller.  The window above will either grow
 *           or contract accordingly.
 *
 * jmh 980803: the few times I wanted to use this function it wouldn't let me.
 *              So now it resizes everything, except the top window, which is
 *              now defined as top == 0 && left == 0.
 *             Borrowing the close window example:
 *
 *                    ÚÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄ¿
 *                    ³      ³    2     ³
 *                    ³      ÃÄÄÄÄÄÂÄÄÄÄ´
 *                    ³      ³  3  ³ 4  ³
 *                    ³  1   ÃÄÄÄÄÄÁÄÄÄÄ´
 *                    ³      ³    5     ³
 *                    ³      ÃÄÄÄÄÄÄÄÄÄÄ´
 *                    ³      ³    6     ³
 *                    ÀÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÙ
 *
 *             window 1 is the top window and cannot be resized. Window 2
 *             cannot be sized horizontally, since it still works on top line.
 *             Sizing any of windows 2, 3, 5 & 6 vertically will size the other
 *             windows, plus window 1. Sizing window 3 or 4 horizontally will
 *             affect the other, plus 2. Similarly, 5 will affect 3 and 4.
 *             As you've probably guessed, use Left and Right (ie. CharLeft
 *             and CharRight) to move the left edge.
 *             I've also used the mode line to display the message (in the
 *              help color) and removed the cursor.
 *
 * jmh 020730: don't resize hidden windows (let change_window() do it).
 * jmh 050801: resize the top window, so now 1 & 2 can be resized.
 */
int  size_window( TDE_WIN *window )
{
int  func;
long c;
int  resize;
int  new_top_line;
int  new_left_col;
int  lowest_top_line  = MAX_LINES;
int  highest_top_line = 0;
int  lowest_left_col  = MAX_COLS;
int  highest_left_col = 0;
register TDE_WIN *above;
register TDE_WIN *win;
TDE_WIN *vis_win[20];   /* assume only 20 visible windows */
int  vis_flag[20];      /* display flags for those windows */
int  visible = 0;       /* number of visible windows */
int  flag;
int  i;
DISPLAY_BUFF;
const char *errmsg = NULL;

/*
 * These are the display flags.
 */
#define TOP_EDGE    1
#define BOTTOM_EDGE 2
#define LEFT_EDGE   4
#define RIGHT_EDGE  8

   win = window;
   if (win->top == mode.display_cwd && win->bottom_line == g_display.end_line &&
       !win->vertical)
      /*
       * cannot resize top window
       */
      errmsg = win6;
   else {
      /*
       * resizing affects current window and all (visible) windows that
       * share its top and/or left edge.
       */
      new_top_line = (win->top == mode.display_cwd)
                     ? win->bottom_line + 1 : win->top;
      new_left_col = (win->left == 0) ? win->end_col + 2 : win->left;
      for (above = g_status.window_list; above != NULL; above = above->next) {
         if (above->visible) {
            flag = 0;
            if (above->top == new_top_line) {
               flag |= TOP_EDGE;
               if (above->bottom_line - 1 < lowest_top_line)
                  lowest_top_line = above->bottom_line - 1;

            } else if (above->bottom_line + 1 == new_top_line) {
               flag |= BOTTOM_EDGE;
               if (above->top_line + 1 > highest_top_line)
                  highest_top_line = above->top_line + 1;
            }
            if (above->left == new_left_col) {
               flag |= LEFT_EDGE;
               if (above->end_col - 14 < lowest_left_col)
                  lowest_left_col = above->end_col - 14;

            } else if (above->end_col + 2 == new_left_col) {
               flag |= RIGHT_EDGE;
               if (above->left + 16 > highest_left_col)
                  highest_left_col = above->left + 16;
            }
            if (flag != 0) {
               if (visible == 20) {
                  /*
                   * too many windows
                   */
                  errmsg = win5;
                  break;
               }
               vis_flag[visible]  = flag;
               vis_win[visible++] = above;
            }
         }
      }
   }
   if (errmsg != NULL) {
      error( WARNING, win->bottom_line, errmsg );
      return( ERROR );
   }

   un_copy_line( win->ll, win, TRUE, TRUE );
   SAVE_LINE( g_display.mode_line );
   eol_clear( 0, g_display.mode_line, Color( Help ) );
   i = (g_display.ncols - (int)strlen( win4 )) / 2;
   if (i < 0)
      i = 0;
   /*
    * use cursor keys to change window size
    */
   s_output( win4, g_display.mode_line, i, Color( Help ) );
   xygoto( -1, -1 );

   for (;;) {
      /*
       * If user has redefined the ESC and Return keys, make them Rturn
       *  and AbortCommand in this function.
       */
      do {
         c    = getkey( );
         func = getfunc( c );
         if (c == RTURN || func == NextLine || func == BegNextLine)
            func = Rturn;
         else if (c == ESC)
            func = AbortCommand;
      } while (func != Rturn          && func != AbortCommand &&
               func != LineUp         && func != LineDown     &&
               func != CharLeft       && func != CharRight    &&
               func != StreamCharLeft && func != StreamCharRight);

      if (func == Rturn || func == AbortCommand)
         break;

      resize = FALSE;
      new_top_line = (win->top == mode.display_cwd)
                     ? win->bottom_line + 1 : win->top;
      new_left_col = (win->left == 0) ? win->end_col + 2 : win->left;
      for (i = 0; i < visible; ++i) {
         above = vis_win[i];
         flag  = vis_flag[i];
         switch (func) {
            /*
             * if LineUp, make current window top line grow and bottom line
             *  of above window shrink.  if window movement covers up current
             *  line of window then we must adjust logical line and real line.
             */
            case LineUp:
               if (new_top_line > highest_top_line) {
                  if (flag & TOP_EDGE) {
                     if (above->rline == (above->cline - above->top_line))
                        --above->cline;
                     --above->top_line;
                     --above->top;
                     resize = TRUE;
                     if (mode.ruler) {
                        if (above->ruler == FALSE) {
                           if (above->cline == above->top_line)
                              ++above->cline;
                           above->ruler = TRUE;
                           --lowest_top_line;
                        }
                     }
                  } else if (flag & BOTTOM_EDGE) {
                     if (above->cline == above->bottom_line)
                        --above->cline;
                     --above->bottom_line;
                     resize = TRUE;
                  }
               }
               break;

            /*
             * if LineDown, make current window top line shrink and bottom line
             *  of above window grow.  if window movement covers up current
             *  line of window then we must adjust logical line and real line.
             */
            case LineDown:
               if (new_top_line < lowest_top_line) {
                  if (flag & TOP_EDGE) {
                     if (above->cline == above->top_line)
                        ++above->cline;
                     ++above->top_line;
                     ++above->top;
                     resize = TRUE;
                  } else if (flag & BOTTOM_EDGE) {
                     ++above->bottom_line;
                     resize = TRUE;
                     if (mode.ruler) {
                        if (above->ruler == FALSE) {
                           ++above->cline;
                           above->ruler = TRUE;
                           ++highest_top_line;
                        }
                     }
                  }
               }
               break;

            /*
             * if CharLeft, make current window left edge grow and right edge
             * of adjacent window shrink.
             */
            case CharLeft:
            case StreamCharLeft:
               if (new_left_col > highest_left_col) {
                  if (flag & LEFT_EDGE) {
                     --above->left;
                     --above->start_col;
                     --above->ccol;
                     resize = TRUE;
                  } else if (flag & RIGHT_EDGE) {
                     --above->end_col;
                     check_virtual_col( above, above->rcol, above->ccol );
                     resize = TRUE;
                  }
               }
               break;

            /*
             * if CharRight, make current window left edge shrink and right
             * edge of adjacent window grow.
             */
            case CharRight:
            case StreamCharRight:
               if (new_left_col < lowest_left_col) {
                  if (flag & LEFT_EDGE) {
                     ++above->left;
                     ++above->start_col;
                     ++above->ccol;
                     check_virtual_col( above, above->rcol, above->ccol );
                     resize = TRUE;
                  } else if (flag & RIGHT_EDGE) {
                     ++above->end_col;
                     resize = TRUE;
                  }
               }
               break;
         }

         /*
          * if we resize a window, then update window size and current and
          *  real lines if needed.
          */
         if (resize == TRUE) {
            setup_window( above );
            redraw_current_window( above );
         }
      }
   }
   RESTORE_LINE( g_display.mode_line );
   return( OK );
}


/*
 * Name:    zoom_window
 * Purpose: To blow-up current window.
 * Date:    September 1, 1991
 * Modified: August 11, 1997, Jason Hood - added call to setup_window
 * Passed:  window:  pointer to current window
 * Notes:   Make all windows, visible and hidden, full size.
 *
 * jmh 010625: fixed bug with top not being set;
 *             removed start_col (setup_window() does it).
 * jmh 030324: preserve screen position of the window.
 * jmh 030325: set the ruler.
 */
int  zoom_window( TDE_WIN *window )
{
register TDE_WIN *wp;

   if (window != NULL) {
      un_copy_line( window->ll, window, TRUE, TRUE );
      for (wp = g_status.window_list; wp != NULL; wp = wp->next) {
         wp->visible     = FALSE;
         wp->top         = mode.display_cwd;
         wp->left        = 0;
         wp->bottom_line = g_display.end_line;
         wp->end_col     = g_display.ncols - 1;
         wp->vertical    = FALSE;
         wp->ruler       = mode.ruler;
         setup_window( wp );
         check_cline( wp, wp->cline );
         wp->bcol = wp->rcol - wp->ccol;
         if (wp->bcol < 0)
            wp->bcol = 0;
         check_virtual_col( wp, wp->rcol, wp->ccol );
      }
      window->visible = TRUE;
      redraw_current_window( window );

      /*
       * can't diff one window, reset the diff
       */
      diff.defined = FALSE;
   }
   return( OK );
}


/*
 * Name:    initialize_window
 * Purpose: To open a new window
 * Date:    June 5, 1991
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen
 * Returns: OK if window opened successfully
 *          ERROR if anything went wrong
 * Notes:   If this is first window, then set up as normal displayed window;
 *          otherwise, make the present window invisible and open a new
 *          window in the same screen location as the old one.
 *
 * Change:  Use of new function get_next_letter( )
 *
 * jmh 990921: process command line column number or binary offset.
 * jmh 031202: place jump line in the center of the window.
 */
int  initialize_window( void )
{
int  top;
int  bottom;
int  start_col;
int  end_col;
TDE_WIN *wp;        /* used for scanning windows */
TDE_WIN *window;
register file_infos *fp;     /* used for scanning files */
register int rc;
line_list_ptr ll;
line_list_ptr temp_ll;
int  rcol;

   rc = OK;
   window = g_status.current_window;
   fp = g_status.current_file;
   if (window == NULL) {
      /*
       * special case if this is the first window on screen.
       */
      top       = mode.display_cwd;
      bottom    = g_display.end_line;
      start_col = 0;
      end_col   = g_display.ncols - 1;
   } else {
      /*
       * else put the new window in same place as current window.
       *  make current window invisible.  new window becomes current window.
       */
      top       = window->top;
      bottom    = window->bottom_line;
      start_col = window->left;
      end_col   = window->end_col;
   }

   assert( top < bottom );
   assert( start_col < end_col );
   assert( fp != NULL );

   if (create_window( &wp, top, bottom, start_col, end_col, fp ) == ERROR) {
      /*
       * out of memory
       */
      error( WARNING, bottom, main4 );

      /*
       * This is a real nuisance. We had room for the file and the
       *  file structure, but not enough for the window as well.
       * Now we must free all the memory that has already been
       *  allocated.
       */
      if (fp->ref_count == 0) {

         /*
          * remove fp from file pointer list.
          */
         if (fp->prev != NULL)
            fp->prev->next = fp->next;
         else
            g_status.file_list = fp->next;

         if (fp->next != NULL)
            fp->next->prev = fp->prev;

         /*
          * free the undo stack, line pointers, and linked list.
          */

         my_free_group( TRUE );

         my_free( fp->undo_top );

         ll = fp->line_list;
         while (ll != NULL) {
            temp_ll = ll->next;
            my_free( ll->line );
            my_free( ll );
            ll = temp_ll;
         }

         my_free_group( FALSE );

#if defined( __MSC__ )
         _fheapmin( );
#endif

         free( fp );
         wp = g_status.current_window;
         if (wp != NULL && wp->visible)
            g_status.current_file = wp->file_info;
         else
            g_status.stop = TRUE;
      }
      rc = ERROR;
   }

   if (rc != ERROR) {
      /*
       * set up the new cursor position as appropriate
       */
      wp->rcol = wp->bcol = 0;

      /*
       * if user entered something like +10 file.doc, make line 10 the
       *  current line. jmh - If the line number is negative, then take
       *  it from the end of the file (so -1 is the last line).
       */
      wp->ll    = fp->line_list->next;
      wp->rline = 1L;
      if (g_status.jump_to != 0) {
         if (g_status.jump_to < 0)
            g_status.jump_to += fp->length + 1;
         move_to_line( wp, g_status.jump_to, TRUE );
      } else if (g_status.jump_off != 0) {
         while (wp->bin_offset + wp->ll->len < g_status.jump_off  &&
                inc_line( wp, FALSE )) ;
         rcol = (int)(g_status.jump_off - wp->bin_offset);
         if (wp->rline == fp->length  &&  rcol > wp->ll->len)
            rcol = wp->ll->len;
         if (fp->inflate_tabs)
            rcol = detab_adjust_rcol( wp->ll->line, rcol, fp->ptab_size );
         check_virtual_col( wp, rcol, wp->ccol );
      }
      if (wp->rline > 1)
         check_cline( wp, (wp->top_line + wp->bottom_line + 1) / 2 );
      else if (wp->bottom_line > wp->top_line)
         ++wp->cline;
      if (g_status.jump_col != 1) {
         rcol = g_status.jump_col;
         if (g_status.jump_col <= 0)
            rcol += find_end( wp->ll->line, wp->ll->len,
                              fp->inflate_tabs, fp->ptab_size );
         else
            rcol--;
         check_virtual_col( wp, rcol, wp->ccol );
      }
      if (window != NULL  &&  !make_window)
         window->visible = FALSE;

      /*
       * the new window becomes the current window.
       */
      g_status.current_window = wp;
      g_status.jump_to  = 0;
      g_status.jump_col = 1;
      g_status.jump_off = 0;
   }
   return( rc );
}


/*
 * Name:    get_next_letter
 * Purpose: To find next letter to use as a window name
 * Date:    August 29, 1993
 * Author:  Byrial Jensen
 * Passed:  next_letter:  The number window to create starting from 0
 *          (Yes, the name is not very good, but I want to keep the
 *          changes as small as possible)
 */
int  get_next_letter( int next_letter )
{
   if (next_letter + 1 > (int)strlen( windowletters ) )
      return( LAST_WINDOWLETTER );
   else
      return( (int) (windowletters[next_letter]) );
}


/*
 * Name:    setup_window
 * Purpose: To set the page length and the center line of a window, based
 *           on the top and bottom lines.
 * Date:    June 5, 1991
 * Passed:  window: window to be set up
 *
 * jmh 991110: set the top line based on top and ruler, and start column based
 *              on left and line numbers.
 */
void setup_window( TDE_WIN *window )
{
   window->top_line = window->top + 1 + window->ruler;
   window->page = window->bottom_line - window->top_line + 1 - g_status.overlap;
   if (window->page < 1)
      window->page = 1;

   window->start_col = window->left;
   if (mode.line_numbers)
      window->start_col += window->file_info->len_len;
}


/*
 * Name:    create_window
 * Purpose: To allocate space for a new window structure and set up some
 *           of the relevant fields.
 * Date:    June 5, 1991
 * Passed:  window: pointer to window pointer
 *          top:    the top line of the new window
 *          bottom: the bottom line of the new window
 *          start_col:  starting column of window on screen
 *          end_col:    ending column of window on screen
 *          file:   the file structure to be associated with the new window
 * Returns: OK if window could be created
 *          ERROR if out of memory
 *
 * jmh 030325: make it visible and assign the letter.
 */
int  create_window( TDE_WIN **window, int top, int bottom, int start_col,
                    int end_col, file_infos *file )
{
TDE_WIN *wp;            /* temporary variable - use it instead of **window */
register TDE_WIN *prev;
int  rc;                /* return code */

   rc = OK;
   /*
    * allocate space for new window structure
    */
   if ((*window = (TDE_WIN *)calloc( 1, sizeof(TDE_WIN) )) == NULL) {
      /*
       * out of memory
       */
      error( WARNING, g_display.end_line, main4 );
      rc = ERROR;
   } else {

     /*
      * set up appropriate fields
      */
      wp              = *window;
      wp->file_info   = file;
      wp->top         = top;
      wp->bottom_line = bottom;
      wp->left        = start_col;
      wp->end_col     = end_col;
      wp->vertical    = !(start_col == 0 && end_col == g_display.ncols-1);
      wp->prev        = NULL;
      wp->next        = NULL;
      wp->ruler       = (mode.ruler && bottom - top > 1);
      setup_window( wp );
      wp->cline       = wp->top_line;
      wp->ccol        = wp->start_col;
      wp->visible     = TRUE;
      wp->letter      = get_next_letter( file->next_letter++ );

      /*
       * add window into window list
       */
      prev = g_status.current_window;
      if (prev) {
         wp->prev = prev;
         if (prev->next)
            prev->next->prev = wp;
         wp->next = prev->next;
         prev->next = wp;
      }
      if (g_status.window_list == NULL)
         g_status.window_list = wp;

      /*
       * record that another window is referencing this file
       */
      ++file->ref_count;
      ++g_status.window_count;
   }
   return( rc );
}


/*
 * Name:    finish
 * Purpose: To remove the current window and terminate the program if no
 *           more windows are left.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   Order of deciding which window becomes current window:
 *          1) If any invisible window with same top and bottom line,
 *          and start_col and end_col, then first invisible one becomes
 *          current window.
 *          2) window above if it exists becomes current window
 *          3) window below if it exists becomes current window
 *          4) window right if it exists becomes current window
 *          5) window left  if it exists becomes current window
 *          6) first available invisible window becomes current window.
 *              (Modified January 23, 1998 by jmh.)
 *          When I added vertical windows, this routine became a LOT
 *           more complicated.  To keep things reasonably sane, let's
 *           only close windows that have three common edges, eg.
 *
 *                    ÚÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄ¿
 *                    ³      ³    no    ³
 *                    ³      ÃÄÄÄÄÄÂÄÄÄÄ´
 *                    ³      ³yes1 ³yes1³
 *                    ³  no  ÃÄÄÄÄÄÁÄÄÄÄ´
 *                    ³      ³   yes2   ³
 *                    ³      ÃÄÄÄÄÄÄÄÄÄÄ´
 *                    ³      ³   yes2   ³
 *                    ÀÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÙ
 *
 *          Windows with 'no' cannot be closed.  Windows with 'yes' can
 *          be combined with windows that have the same yes number.
 *
 * jmh 980809: removed menu to choose RepeatGrep; just press Return instead.
 * jmh 021023: handle g_status.stop differently when loading a new file to
 *              allow startup macros to work.
 * jmh 021024: remove the grep prompt, to allow -e to work automatically
 *              (Quit/FileAll can be used to stop).
 * jmh 030324: preserve screen position of the newly visible window.
 */
int  finish( TDE_WIN *window )
{
register TDE_WIN *wp, *wn;      /* for scanning other windows */
register TDE_WIN *win;          /* register pointer for window */
file_infos *file, *fp;          /* for scanning other files */
int  poof;
int  top;
int  bottom;
int  start_col;
int  end_col;
int  max_letter;
int  letter_index;
int  file_change = FALSE;
int  zoomed;
line_list_ptr ll;
line_list_ptr temp_ll;
UNDO *undo;
UNDO *temp_u;

   win = window;
   if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
      return( ERROR );

   zoomed = (!win->vertical  &&  win->top == mode.display_cwd  &&
             win->bottom_line == g_display.end_line);

   file = win->file_info;
   if (g_status.command != CloseWindow) {
      /*
       * remove all hidden windows that point to same file
       */
      for (wp = g_status.window_list; wp != NULL; wp = wn) {
         wn = wp->next;
         if (wp->file_info == file) {
            if (!wp->visible) {
               if (wp->prev == NULL) {
                  if (wn == NULL)
                     g_status.stop = TRUE;
                  else
                     g_status.window_list = wn;
               } else
                  wp->prev->next = wn;
               if (wn)
                  wn->prev = wp->prev;
               my_free( wp->title );
               free( wp );
               --file->ref_count;
               --g_status.window_count;
            }
         }
      }
   }

   if (win->prev == NULL && win->next == NULL)
      g_status.stop = TRUE;

   poof = FALSE;

   if (g_status.stop != TRUE) {
      top       = win->top;
      bottom    = win->bottom_line;
      start_col = win->left;
      end_col   = win->end_col;
      if (g_status.command != CloseWindow) {
         /*
          * see if there are any invisible windows with same top and bottom
          *  lines, and start_col and end_col as this window.
          */
         wp = win->prev;
         while (wp != NULL && poof == FALSE) {
            if (wp->top == top         &&  wp->bottom_line == bottom  &&
                wp->left == start_col  &&  wp->end_col == end_col     &&
                !wp->visible)
               poof = TRUE;
            else
               wp = wp->prev;
         }
         if (poof == FALSE) {
            wp = win->next;
            while (wp != NULL && poof == FALSE) {
               if (wp->top == top         &&  wp->bottom_line == bottom  &&
                   wp->left == start_col  &&  wp->end_col == end_col     &&
                   !wp->visible)
                  poof = TRUE;
               else
                  wp = wp->next;
            }
         }
      }

      if (poof == FALSE) {
         if (!zoomed) {
            /*
             * see if there are any windows above
             */
            wp = g_status.window_list;
            while (wp != NULL && poof == FALSE) {
               if (wp->bottom_line+1 == win->top  &&
                   wp->left    == win->left       &&
                   wp->end_col == win->end_col    && wp->visible) {
                  poof = TRUE;
                  top  = wp->top;
               } else
                  wp = wp->next;
            }
            if (poof == FALSE) {
               /*
                * see if there are any windows below
                */
               wp = g_status.window_list;
               while (wp != NULL && poof == FALSE) {
                  if (wp->top-1   == win->bottom_line  &&
                      wp->left    == win->left         &&
                      wp->end_col == win->end_col      && wp->visible) {
                     poof = TRUE;
                     bottom = wp->bottom_line;
                  } else
                     wp = wp->next;
               }
            }
            if (poof == FALSE) {
               /*
                * see if there are any windows right
                */
               wp = g_status.window_list;
               while (wp != NULL && poof == FALSE) {
                  if (wp->top         == win->top          &&
                      wp->bottom_line == win->bottom_line  &&
                      wp->left-2      == win->end_col      && wp->visible) {
                     poof = TRUE;
                     end_col = wp->end_col;
                  } else
                     wp = wp->next;
               }
            }
            if (poof == FALSE) {
               /*
                * see if there are any windows left
                */
               wp = g_status.window_list;
               while (wp != NULL && poof == FALSE) {
                  if (wp->top         == win->top          &&
                      wp->bottom_line == win->bottom_line  &&
                      wp->end_col+2   == win->left         && wp->visible) {
                     poof = TRUE;
                     start_col = wp->left;
                  } else
                     wp = wp->next;
               }
            }
         }
         if (poof == FALSE) {
            /*
             * see if there are any other invisible windows.
             */
            wp = win->prev;
            while (wp != NULL && poof == FALSE) {
               if (!wp->visible)
                  poof = TRUE;
               else
                  wp = wp->prev;
            }
            if (poof == FALSE) {
               wp = win->next;
               while (wp != NULL && poof == FALSE) {
                  if (!wp->visible)
                     poof = TRUE;
                  else
                     wp = wp->next;
               }
            }
         }
      }
      if (poof) {
         wp->visible  = TRUE;
         wp->top      = top;
         wp->bottom_line = bottom;
         wp->left     = start_col;
         wp->end_col  = end_col;
         wp->vertical = !(start_col == 0 && end_col == g_display.ncols - 1);
         wp->ruler    = (mode.ruler && bottom - top > 1);
         setup_window( wp );
         check_cline( wp, wp->cline );
         wp->bcol = wp->rcol - (wp->ccol - wp->start_col);
         if (wp->bcol < 0)
            wp->bcol = 0;
         check_virtual_col( wp, wp->rcol, wp->ccol );
         set_path( wp );
         show_window_header( wp );
         if (wp->vertical)
            show_vertical_separator( wp );

         /*
          * The window above, below, or previously invisible becomes the new
          *  current window.
          */
         g_status.current_window = wp;
         g_status.current_file = wp->file_info;
         wp->file_info->dirty = LOCAL | RULER;
      }
   }

   if (!poof && g_status.stop != TRUE) {
      /*
       * cannot close current window
       */
      error( WARNING, win->bottom_line, win7 );
      return( ERROR );
   }

   if (g_status.command == CloseWindow && !zoomed)
      win->visible = FALSE;
   else {
      /*
       * free unused file memory if necessary
       */
      if (--file->ref_count == 0) {

         /*
          * if a block is marked, unmark it
          */
         if (file == g_status.marked_file) {
            g_status.marked      = FALSE;
            g_status.marked_file = NULL;
         }

         /*
          * if it's the current search results or browser window, reset it
          */
         if (file == results_file)
            results_window = NULL;
         if (browser_window && file == browser_window->file_info)
            browser_window = NULL;

         for (fp = g_status.file_list; fp != NULL; fp = fp->next) {
            if (fp->file_no > file->file_no)
               fp->file_no--;
         }
         file_change = file->file_no;

         if (file->scratch  &&  file->scratch == g_status.scratch_count) {
            /*
             * find the new highest scratch number
             */
            file->scratch = g_status.scratch_count = 0;
            for (fp = g_status.file_list; fp != NULL; fp = fp->next) {
               if (fp->scratch > g_status.scratch_count)
                  g_status.scratch_count = fp->scratch;
            }
         }

         /*
          * no window now refers to this file, so remove file from the list
          */
         if (file->prev == NULL)
            g_status.file_list = file->next;
         else
            file->prev->next = file->next;
         if (file->next)
            file->next->prev = file->prev;

         /*
          * free the line pointers, linked list of line pointers, and
          *  file struc.
          */

         my_free_group( TRUE );

         ll = file->undo_top;
         while (ll != NULL) {
            temp_ll = ll->next;
            my_free( ll->line );
            my_free( ll );
            ll = temp_ll;
         }

         ll = file->line_list;
         while (ll != NULL) {
            temp_ll = ll->next;
            my_free( ll->line );
            my_free( ll );
            ll = temp_ll;
         }

         undo = file->undo;
         while (undo != NULL) {
            temp_u = undo->prev;
            if ((unsigned long)undo->text > 255)
               my_free( undo->text );
            my_free( undo );
            undo = temp_u;
         }

         my_free_group( FALSE );

#if defined( __MSC__ )
         _fheapmin( );
#endif

         free( file );
         if (--g_status.file_count != 0) {
            show_file_count( );
            show_avail_mem( );
         }
      }

      /*
       * remove the current window from the window list
       */
      if (win->prev == NULL)
         g_status.window_list = win->next;
      else
         win->prev->next = win->next;

      if (win->next)
         win->next->prev = win->prev;

      if (diff.defined  &&  (diff.w1 == win  ||  diff.w2 == win))
         diff.defined = FALSE;

      if (!file_change  &&  win->letter != LAST_WINDOWLETTER) {
         int let = (int)(strchr( windowletters, win->letter )
                         - (char *)windowletters);
         for (wp = g_status.window_list; wp != NULL; wp = wp->next) {
            if (wp->file_info == file  &&  wp->letter != LAST_WINDOWLETTER) {
               letter_index = (int)(strchr( windowletters, wp->letter )
                                    - (char *)windowletters);
               if (letter_index > let) {
                  wp->letter = windowletters[letter_index - 1];
                  if (wp->visible)
                     show_window_number_letter( wp );
               }
            }
         }
      }

      /*
       * free the memory taken by the window structure
       */
      my_free( win->title );
      free( win );
      --g_status.window_count;
   }

   if (g_status.stop == FALSE) {
      show_window_count( );
      show_tab_modes( );
      if (file_change) {
         for (wp = g_status.window_list; wp != NULL; wp = wp->next)
            if (wp->visible)
               show_window_number_letter( wp );
      } else {
         max_letter = 0;
         for (wp = g_status.window_list; wp != NULL; wp = wp->next) {
            if (wp->file_info == file) {
               if (wp->letter == LAST_WINDOWLETTER) {
                  max_letter = strlen( windowletters );
                  break;
               } else {
                  letter_index = (int)(strchr( windowletters, wp->letter )
                                       - (char *)windowletters);
                  if (letter_index > max_letter)
                     max_letter = letter_index;
               }
            }
         }
         if (max_letter < file->next_letter - 1)
            file->next_letter = max_letter + 1;
      }
      /*
       * was the original search results or browser window closed after
       * being split?  if so, find the other one
       */
      if (win == results_window)
         results_window = find_file_window( results_file );
      if (win == browser_window)
         browser_window = find_file_window( file );

   } else {
      if (g_status.sas_defined) {
         if (g_status.sas_arg < g_status.sas_argc) {
            g_status.command = RepeatGrep;
            g_status.current_window = NULL;
            g_status.current_file   = NULL;
            g_status.stop = FALSE;
            if (search_and_seize( g_status.window_list ) == ERROR)
               g_status.stop = TRUE;
         }
      } else {
         if (g_status.arg < g_status.argc) {
            g_status.current_window = NULL;
            g_status.current_file   = NULL;
            g_status.stop = FALSE;
            if (edit_next_file( g_status.window_list ) != OK)
               g_status.stop = TRUE;
         }
      }
   }

   return( OK );
}


/*
 * Name:    find_window
 * Purpose: to find a window given a string
 * Author:  Jason Hood
 * Date:    May 2, 1999
 * Passed:  window:       place to store new window
 *          win:          string containing new window
 *          prompt_line:  line to display error message
 * Returns: OK if new window exists
 *          ERROR if new window doesn't exist or is not visible for diff,
 *                or is visible for make window.
 * Notes:   Assume that if it doesn't start with a number, it's a filename
 *           (unlike other file routines, this will always match case);
 *          if it has a number but no letter, assume *windowletters ('a').
 *          replaces verify_letter() and verify_number() from diff.c.
 *
 * 031117:  if no letter is given, find the first (visible) window.
 * 060829:  make filename searching ignore case (except for Unix);
 *          let it match the current window's filename.
 */
int  find_window( TDE_WIN **window, char *win, int prompt_line )
{
char *name = win;
register file_infos *fp;
register TDE_WIN *wp;
int  len;
int  num;
char let;
int  rc;

   if (*win == '\0')
      return( ERROR );

   rc = ERROR;

   if (!bj_isdigit( *win )) {
      /*
       * See if we can find a filename that starts with the string.
       */
      len = strlen( win );
      fp = g_status.current_file;
      do {
         fp = fp->next;
         if (fp == NULL)
            fp = g_status.file_list;
#if defined( __UNIX__ )
         if (strncmp( fp->fname, win, len ) == 0)
#else
         if (strnicmp( fp->fname, win, len ) == 0)
#endif
         {
            rc = OK;
            break;
         }
      } while (fp != g_status.current_file);
      if (rc == ERROR)
         fp = NULL;
      else
         win += len;    /* point to the NUL for the letter */

   } else {
      /*
       * string has a number, convert it.
       */
      num = 0;
      do
         num = num * 10 + *win - '0';
      while (bj_isdigit( *++win ));
      /*
       * now that we have a window number, see if any files have that number
       */
      for (fp = g_status.file_list; fp != NULL; fp = fp->next) {
         if (fp->file_no == num)
            break;
      }
   }
   wp = g_status.window_list;
   if (fp != NULL) {
      if (*win == '\0') {
         *window = find_file_window( fp );
         rc = OK;
      } else {
         let = *win;
         for (; wp != NULL; wp = wp->next) {
            if (wp->file_info == fp  &&  wp->letter == let) {
               *window = wp;
               rc = OK;
               break;
            }
         }
      }
   }
   if (rc == ERROR  ||
       (!wp->visible && g_status.command == DefineDiff)  ||
       (wp->visible && make_window)) {
      const char *prefix = win21a, *suffix;
      if (rc == ERROR) {
         if (bj_isdigit( *name ))
            suffix = win21b;
         else {
            prefix = win22a;
            suffix = win22b;
         }
      } else {
         suffix = (wp->visible) ? win21c : diff_prompt6;
         rc = ERROR;
      }
      combine_strings( line_out, prefix, name, suffix );
      error( WARNING, prompt_line, line_out );
   }
   return( rc );
}


/*
 * Name:    find_file_window
 * Purpose: find a window containing a specific file
 * Author:  Jason Hood
 * Date:    November 17, 2003
 * Passed:  file:  the file to find
 * Returns: the window
 * Notes:   give preference to a visible window
 */
TDE_WIN *find_file_window( file_infos *file )
{
TDE_WIN *wp;
TDE_WIN *win = NULL;

   for (wp = g_status.window_list; wp != NULL; wp = wp->next) {
      if (wp->file_info == file) {
         if (win == NULL)
            win = wp;
         if (file->ref_count == 1  ||  wp->visible)
            break;
      }
   }
   return( win );
}


/*
 * Name:    title_window
 * Purpose: give the window a different name
 * Author:  Jason Hood
 * Date:    March 31, 2003
 * Notes:   display this title instead of the file name.
 *          use an empty string to reset it.
 */
int  title_window( TDE_WIN *window )
{
char answer[MAX_COLS+2];
int  len;
char *title;

   /*
    * default to the current title, or the file name.
    */
   if (window->title)
      strcpy( answer, window->title );
   else
      strcpy( answer, window->file_info->file_name
                      + (*window->file_info->file_name == '\0') );
   len = get_name( win23, window->bottom_line, answer, &h_win );
   if (len == 0) {
      /*
       * remove the title
       */
      my_free( window->title );
      window->title = NULL;

   } else if (len > 0) {
      title = my_strdup( answer );
      if (title != NULL) {
         my_free( window->title );
         window->title = title;
      } else
         error( WARNING, window->bottom_line, main4 );
   }
   if (len != ERROR)
      show_window_fname( window );

   return( OK );
}


/*
 * Name:    find_make_window
 * Purpose: select the new window for the make window functions
 * Author:  Jason Hood
 * Date:    March 31, 2003
 * Passed:  window:  pointer to current window
 * Returns: the window, or NULL if cancelled
 */
static TDE_WIN *find_make_window( TDE_WIN *window )
{
int  ch;
int  func = ERROR;
int  rc;
int  row, col;
TDE_WIN *win = NULL;

   make_window = TRUE;

   if (g_status.macro_executing &&
       g_status.macro_next < g_status.current_macro->len) {
      long key = g_status.current_macro->key.keys[g_status.macro_next++];
      func = getfunc( key );
      if (func == Pause)
         func = ERROR;
   }

   if (func == ERROR) {
      xygoto( -1, -1 );
      g_display.output_space = g_display.frame_space;
      row = (g_display.nlines - make_window_menu.minor_cnt - 2) / 2;
      col = (g_display.ncols - make_window_menu.width) / 2;
      ch  = -1;
      make_window_menu.checked = FALSE;
      func = (int)pull_me( &make_window_menu, row, col, &ch );
      g_display.output_space = FALSE;
   }

   if (func != ERROR) {
      g_status.command = func & ~_FUNCTION;
      g_status.control_break = FALSE;
      record_key( 0, g_status.command );
      rc = execute( window );
      if (rc == OK  &&  window != g_status.current_window)
         win = g_status.current_window;
   }

   make_window = FALSE;

   return( win );
}
