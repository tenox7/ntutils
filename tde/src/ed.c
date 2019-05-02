/*******************  start of original comments  ********************/
/*
 * Written by Douglas Thomson (1989/1990)
 *
 * This source code is released into the public domain.
 */

/*
 * Name:    dte - Doug's Text Editor program - main editor module
 * Purpose: This file contains the main editor module, and a number of the
 *           smaller miscellaneous editing commands.
 *          It also contains the code for dispatching commands.
 * File:    ed.c
 * Author:  Douglas Thomson
 * System:  this file is intended to be system-independent
 * Date:    October 1, 1989
 * I/O:     file being edited
 *          files read or written
 *          user commands and prompts
 * Notes:   see the file "dte.doc" for general program documentation
 */
/*********************  end of original comments   ********************/

/*
 * The basic editor routines have been EXTENSIVELY rewritten.  I have added
 * support for lines longer than 80 columns and I have added line number
 * support.  I like to know the real line number that editor functions are
 * working on and I like to know the total number of lines in a file.
 *
 * I rewrote the big series of ifs in the dispatch subroutine.  It is now
 * an array of pointers to functions.  We know what function to call as soon
 * as a key is pressed.  It is also makes it easier to implement a configuration
 * utility and macros.
 *
 * I added a few functions that I use quite often and I deleted a few that I
 * rarely use.  Added are Split Line, Join Line, and Duplicate Line.  Deleted
 * are Goto Marker 0-9 (others?).
 *
 * ************ In TDE 1.3, I put Goto Marker 0-9 back in.  ***************
 *
 * I felt that the insert routine should be separated into two routines.  One
 * for inserting the various combinations of newlines and one for inserting
 * ASCII and extended ASCII characters.
 *
 * One of Doug's design considerations was keeping screen updates to a minimum.
 * I have expanded upon that idea and added support for updating windows
 * LOCALly, GLOBALly, or NOT_LOCALly.  For example, scrolling in one window
 * does not affect the text in another window - LOCAL update.  Adding, deleting,
 * or modifying text in one window may affect text in other windows - GLOBAL
 * update.  Sometimes, updates to the current window are handled in the task
 * routines so updates to other windows are done NOT_LOCALly.
 *
 * In version 2.2, the big text buffer scheme was replaced by a double
 *  linked list.
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

#include "tdestr.h"     /* typedefs for global variables */
#include "define.h"     /* editor function defs */
#include "tdefunc.h"    /* prototypes for all functions in tde */
#include "common.h"


#if !defined( __DOS16__ )
#include <setjmp.h>
jmp_buf editor_jmp;
#endif

#if defined( __WIN32__ )
extern int    handle_mouse;     /* defined in win32/console.c */
extern HANDLE conin;
#endif

extern long found_rline;        /* defined in findrep.c */


/*
 * Name:    insert_newline
 * Purpose: insert a newline
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   There a several ways to insert a line into a file:  1) pressing
 *          a key, 2) word wrap, 3) any others?
 *          When doing word wrap or format paragraph, don't show any changes.
 *            Wait until the function finishes then show all changes at once.
 */
int  insert_newline( TDE_WIN *window )
{
text_ptr source;        /* source for block move to make room for c */
text_ptr dest;          /* destination for block move */
int  len;               /* length of current line */
int  split_len;
int  add;               /* characters to be added (usually 1 in insert mode) */
int  rcol;
int  rc;
long length;
int  carriage_return;
int  split_line;
int  wordwrap;
int  dirty;
int  old_bcol;
register TDE_WIN *win;  /* put window pointer in a register */
file_infos *file;       /* pointer to file structure in current window */
line_list_ptr new_node;

   rc = OK;
   win = window;
   file = win->file_info;
   length = file->length;
   wordwrap = mode.word_wrap;
   switch (g_status.command) {
      case WordWrap :
         carriage_return = TRUE;
         split_line = FALSE;
         break;
      case AddLine  :
         split_line = carriage_return = FALSE;
         break;
      case SplitLine :
         split_line = carriage_return = TRUE;
         break;
      case Rturn :
      default    :

         /*
          * if file is opened in BINARY mode, lets keep the user from
          *   unintentially inserting a line feed into the text.
          */
         if (file->crlf == BINARY)
            return( next_line( win ) );

         show_ruler_char( win );
         carriage_return = TRUE;
         split_line = FALSE;
         break;
   }

   /*
    * make window temporarily invisible to the un_copy_line function
    */
   win->visible = FALSE;
   old_bcol = win->bcol;
   new_node = new_line( DIRTY, &rc );
   if (rc == OK) {
      if (win->ll->len != EOF) {
         file->modified = TRUE;
         if (mode.do_backups == TRUE)
            rc = backup_file( win );
         copy_line( win->ll, win, TRUE );
         len = g_status.line_buff_len;
         split_len = 0;
         /* jmh - added the AddLine test, since the current line is unchanged */
         if (win->rcol < len  &&  g_status.command != AddLine)
            win->ll->type |= DIRTY;

         if (carriage_return || split_line) {
            if (win->rcol < len) {
               split_len = len - win->rcol;
               len = win->rcol;
            }
         }
         source = g_status.line_buff + len;
         g_status.line_buff_len = len;
         if (un_copy_line( win->ll, win, TRUE, TRUE ) == OK) {

            assert( split_len >= 0 );
            assert( split_len < MAX_LINE_LENGTH );

            memmove( g_status.line_buff, source, split_len );
            g_status.line_buff_len = len = split_len;
            g_status.copied = TRUE;
         } else
            rc = ERROR;
      } else {
         g_status.line_buff_len = len = 0;
         g_status.copied = TRUE;
      }

      if (rc == OK) {
         /*
          * we are somewhere in the list and we need to insert the new node.
          *  if we are anywhere except the EOF node, insert the new node
          *  after the current node.  if the current node is the EOF node,
          *  insert the new node before the EOF node.  this keeps the
          *  EOF node at the end of the list.
          */
         if (win->ll->next == NULL) {
            insert_node( file, win->ll, new_node );
            win->ll = new_node;
         } else
            insert_node( file, win->ll, new_node );

         rc = un_copy_line( new_node, win, FALSE, TRUE );
         adjust_windows_cursor( win, 1 );
         restore_marked_block( win, 1 );

         if (win->ll->len != EOF && file->syntax != NULL)
            syntax_check( win->ll, file->syntax->info );
         syntax_check_lines( new_node, file->syntax );

         if (file->dirty != GLOBAL)
            file->dirty = NOT_LOCAL;
         if (length == 0l || wordwrap || win->cline == win->bottom_line)
            file->dirty = GLOBAL;
         else if (!split_line)
            update_line( win );

         /*
          * If the cursor is to move down to the next line, then update
          *  the line and column appropriately.
          */
         if (rc == OK  &&  (carriage_return || split_line)) {
            dirty = file->dirty;
            if (win->cline < win->bottom_line)
               win->cline++;
            inc_line( win, TRUE );
            rcol = win->rcol;
            old_bcol = win->bcol;

            if (win->ll->next != NULL) {
               if (mode.indent || wordwrap) {
                  /*
                   * autoindentation is required. Match the indentation of
                   *  the first line above that is not blank.
                   */
                  add = find_left_margin( wordwrap == FIXED_WRAP ?
                                            win->ll : win->ll->prev, wordwrap,
                                          file->inflate_tabs, file->ptab_size );

                  assert( add >= 0 );
                  assert( add < MAX_LINE_LENGTH );

                  copy_line( win->ll, win, TRUE );
                  len = g_status.line_buff_len;
                  source = g_status.line_buff;
                  if (len + add > MAX_LINE_LENGTH)
                     add = MAX_LINE_LENGTH - len;
                  dest = source + add;

                  assert( len >= 0);
                  assert( len < MAX_LINE_LENGTH );

                  memmove( dest, source, len );

                  /*
                   * now put in the autoindent characters
                   */

                  assert( add >= 0 );
                  assert( add < MAX_LINE_LENGTH );

                  memset( source, ' ', add );
                  win->rcol = add;
                  g_status.line_buff_len += add;
                  shift_block( file, win->rline, 0, add );
                  rc = un_copy_line( win->ll, win, TRUE, TRUE );
               } else
                  win->rcol = 0;
            }
            if (rc == OK  &&  split_line) {
               win->rline--;
               win->ll = win->ll->prev;
               if (win->cline > win->top_line)
                  win->cline--;
               win->rcol = rcol;
            }
            check_virtual_col( win, win->rcol, win->ccol );
            if (dirty == GLOBAL || file->dirty == LOCAL || wordwrap)
               file->dirty = GLOBAL;
            else
               file->dirty = dirty;
         }
      } else
         my_free( new_node );
   } else
      error( WARNING, window->bottom_line, main4 );

   /*
    * record that file has been modified
    */
   win->visible = TRUE;
   if (rc == OK) {
      if (file->dirty != GLOBAL)
         my_scroll_down( win );
      show_size( win );
      show_avail_mem( );
      if (old_bcol != win->bcol)
         show_ruler( win );
   }
   return( rc );
}


/*
 * Name:    adjust_for_tabs
 * Purpose: handle tabs in the line buffer in an appropriate manner
 * Date:    May 24, 1998 (jmh)
 * Passed:  rcol:  pointer to current (real) column
 *          inflate_tabs: are tabs being expanded?
 *          tab_size: size of tab expansion
 * Returns: rcol adjusted if needed
 * Notes:   If tabs are deflated or inflated, call detab_linebuff(), but if
 *          real tabs are in use and it's insert mode, then leave the tabs
 *          alone and adjust rcol.  However, if we're in a tab, insert spaces
 *          before it.
 * jmh 981129: must be updating the cursor in the normal direction.
 * jmh 991010: if it's not normal direction, it's overwrite mode.
 */
static void adjust_for_tabs( int *rcol, int inflate_tabs, int tab_size )
{
int   col;
int   len;
int   pad;
text_ptr source;
text_ptr dest;

   if (inflate_tabs == 2 && mode.insert) {
      len   = g_status.line_buff_len;
      col   = *rcol;
      *rcol = entab_adjust_rcol( g_status.line_buff, len, col, tab_size );
      pad   = col - detab_adjust_rcol( g_status.line_buff, *rcol, tab_size );
      if (*rcol < len && g_status.line_buff[*rcol] == '\t') {
         if (pad > 0 && len + pad < MAX_LINE_LENGTH) {
            source = g_status.line_buff + *rcol;
            dest   = source + pad;
            memmove( dest, source, len - *rcol );
            memset( source, ' ', pad );
            g_status.line_buff_len += pad;
            *rcol += pad;
         }
      } else if (*rcol == len)
         *rcol += pad;
   } else
      detab_linebuff( inflate_tabs, tab_size );
}


/*
 * Name:    insert_overwrite
 * Purpose: To make the necessary changes after the user has typed a normal
 *           printable character
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 *
 * jmh 980524: use a "real tabs" mode. If insert is on, don't detab the line.
 * jmh 981129: added the ability to update the cursor in different directions,
 *              however, it always functions in overwrite mode (which is
 *              assumed to be on).
 * jmh 020913: tab character in tab mode will use tab_key() with fixed tabs;
 *              will probably cause problems in overwrite mode, but overwriting
 *              tab characters should be done in deflate mode, anyway.
 */
int  insert_overwrite( TDE_WIN *window )
{
text_ptr source;        /* source for block move to make room for c */
text_ptr dest;          /* destination for block move */
int  len;               /* length of current line */
int  pad;               /* padding to add if cursor beyond end of line */
int  add;               /* characters to be added (usually 1 in insert mode) */
int  rcol;
register TDE_WIN *win;  /* put window pointer in a register */
int  rc = OK;
int  dir = mode.cur_dir; /* direction to update cursor */

   win = window;
   if (win->ll->len != EOF && g_status.key_pressed < 256) {
      rcol = win->rcol;
      /*
       * first check we have room - the editor can not
       *  cope with lines wider than MAX_LINE_LENGTH
       */
      if (rcol >= MAX_LINE_LENGTH) {
         /*
          * cannot insert more characters
          */
         error( WARNING, win->bottom_line, ed2 );
         rc = ERROR;
      } else if (g_status.key_pressed == '\t' && win->file_info->inflate_tabs) {
         add = mode.smart_tab;
         mode.smart_tab = FALSE;
         rc = tab_key( win );
         mode.smart_tab = add;
      } else {
         copy_line( win->ll, window, FALSE );
         adjust_for_tabs( &rcol, win->file_info->inflate_tabs,
                                 win->file_info->ptab_size );

         /*
          * work out how many characters need to be inserted
          */
         len = g_status.line_buff_len;
         pad = rcol > len ? rcol - len : 0;

         if (mode.insert || rcol >= len || (dir == CUR_LEFT && rcol == 0)) {
            /*
             * inserted characters, or overwritten characters at the end of
             *  the line, are inserted.
             */
            add = 1;
            if (dir == CUR_LEFT && len == 0)
               ++add;
         } else
            /*
             *  and no extra space is required to overwrite existing characters
             */
            add = 0;

         /*
          * check that current line would not get too long.
          */
         if (len + pad + add >= MAX_LINE_LENGTH) {
            /*
             * no more room to add
             */
            error( WARNING, win->bottom_line, ed3 );
            rc = ERROR;
         } else {

            /*
             * make room for whatever needs to be inserted
             */
            if (pad > 0  ||  add > 0) {

               shift_tabbed_block( win->file_info );

               source = g_status.line_buff + rcol - pad;
               dest   = source + pad + add;

               assert( len + pad - rcol >= 0 );
               assert( len + pad - rcol < MAX_LINE_LENGTH );

               memmove( dest, source, len + pad - rcol );

               /*
                * put in the required padding
                */

               assert( pad >= 0 );
               assert( pad < MAX_LINE_LENGTH );

               memset( source, ' ', pad + add );

               shift_block( win->file_info, win->rline, rcol, add );
            }
            if (dir == CUR_LEFT && rcol == 0) {
               *g_status.line_buff = ' ';
               ++rcol;
            }
            g_status.line_buff[rcol] = (char)g_status.key_pressed;
            g_status.line_buff_len += pad + add;
            entab_linebuff( win->file_info->inflate_tabs,
                            win->file_info->ptab_size );

            /*
             * always increment the real column (rcol) then adjust the
             * logical and base column as needed.   show the changed line
             * in all but the LOCAL window.  In the LOCAL window, there are
             * two cases:  1) update the line, or 2) redraw the window if
             * cursor goes too far right.
             *
             * jmh 981129: update cursor according to direction.
             */
            win->file_info->modified = TRUE;
            win->file_info->dirty = NOT_LOCAL;
            win->ll->type |= DIRTY;
            show_changed_line( win );
            switch (dir) {
               case CUR_RIGHT:
                  if (win->ccol < win->end_col) {
                     show_curl_line( win );
                     show_ruler_char( win );
                     win->ccol++;
                  } else {
                     win->bcol++;
                     win->file_info->dirty = LOCAL;
                     show_ruler( win );
                  }
                  rcol = win->rcol + 1;    /* real tabs may make rcol wrong */
                  check_virtual_col( win, rcol, win->ccol );

                  if (mode.word_wrap) {
                     add = mode.right_justify;
                     mode.right_justify = FALSE;
                     g_status.command = FormatText;
                     word_wrap( win );
                     mode.right_justify = add;
                  }
                  break;

               case CUR_LEFT :
                  --rcol;
                  --win->ccol;
                  check_virtual_col( win, rcol, win->ccol );
                  if (win->file_info->dirty == FALSE)
                     show_curl_line( win );
                  break;

               case CUR_DOWN :
                  if (win->ll->next->len == EOF) {
                     g_status.command = AddLine;
                     rc = insert_newline( win );
                  }
                  if (rc == OK)
                     prepare_move_down( win );
                  break;

               case CUR_UP :
                  prepare_move_up( win );
                  if (win->rline == 0) {
                     g_status.command = AddLine;
                     rc = insert_newline( win );
                     prepare_move_down( win );
                  }
                  break;
            }
         }
      }
   }
   return( rc );
}


/*
 * Name:    join_line
 * Purpose: To join current line and line below at cursor
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   trunc the line.  then, join with line below, if it exists.
 *
 * jmh 991113: if this is called from WordDelete, remove leading space.
 * jmh 000604: corrected bug when WordDelete joined with a blank line.
 */
int  join_line( TDE_WIN *window )
{
int  len;               /* length of current line */
int  new_len;           /* length of the joined lines */
int  next_len;          /* length of the line below current line */
text_ptr q;             /* next line in file */
text_ptr tab_free;      /* next line in file -- with the tabs removed */
int  pad;               /* padding spaces required */
register TDE_WIN *win;  /* put window pointer in a register */
TDE_WIN *wp;
line_list_ptr next_node;
int  rc;

   win = window;
   if (win->ll->len == EOF  ||  win->ll->next->len == EOF)
      return( ERROR );

   rc = OK;

   assert( win->ll->next != NULL );

   next_node = win->ll->next;
   load_undo_buffer( win->file_info, win->ll->line, win->ll->len );
   copy_line( win->ll, win, TRUE );

   /*
    * if cursor is in line before eol, reset len to rcol
    */
   if (win->rcol < (len = g_status.line_buff_len))
      len = win->rcol;

   /*
    * calculate needed padding
    */
   pad = win->rcol > len ? win->rcol - len : 0;

   assert( pad >= 0 );
   assert( pad < MAX_LINE_LENGTH );

   /*
    * if there any tabs in the next line, expand them because we
    *   probably have to redo them anyway.
    * jmh 991113: keep the tabs in real tab mode.
    */
   next_len = next_node->len;
   if (win->file_info->inflate_tabs == 1)
      tab_free = detab_a_line( next_node->line, &next_len,
                               win->file_info->inflate_tabs,
                               win->file_info->ptab_size );
   else
      tab_free = next_node->line;
   if (g_status.command == WordDelete) {
      while (next_len > 0  &&  bj_isspace( *tab_free )) {
         ++tab_free;
         --next_len;
      }
   }

   assert( next_len >= 0 );
   assert( next_len < MAX_LINE_LENGTH );
   assert( len >= 0 );
   assert( len < MAX_LINE_LENGTH );

   /*
    * check room to combine lines
    */
   new_len = len + pad + next_len;
   if (new_len >= MAX_LINE_LENGTH) {
      /*
       * cannot combine lines.
       */
      error( WARNING, win->bottom_line, ed4 );
      rc = ERROR;
   } else {
      win->file_info->modified = TRUE;
      if (mode.do_backups == TRUE)
         rc = backup_file( win );

      q = g_status.line_buff + len;
      /*
       * insert padding
       */
      if (pad > 0) {
         while (pad--)
            *q++ = ' ';
      }
      my_memcpy( q, tab_free, next_len );
      g_status.line_buff_len = new_len;
      if ((rc = un_copy_line( win->ll, win, FALSE, TRUE )) == OK) {

         if (next_node->next != NULL)
            next_node->next->prev = win->ll;
         win->ll->next = next_node->next;
         win->ll->type |= DIRTY;

         --win->file_info->length;
         ++win->rline;
         adjust_windows_cursor( win, -1 );
         restore_marked_block( win, -1 );
         --win->rline;
         syntax_check_lines( win->ll, win->file_info->syntax );

         wp = g_status.window_list;
         while (wp != NULL) {
            if (wp->file_info == win->file_info) {
               /*
                * make sure none of the window pointers point to the
                *   node we are about to delete.
                */
               if (wp != win) {
                  if (wp->ll == next_node)
                     wp->ll = win->ll->next;
               }
            }
            wp = wp->next;
         }

         /*
          * now, it's safe to delete the next_node line as well as
          *   the next node.
          */
         my_free( next_node->line );
         my_free( next_node );

         show_size( win );
         show_avail_mem( );
         win->file_info->dirty = GLOBAL;
      }
   }
   return( rc );
}


/*
 * Name:     word_delete
 * Purpose:  To delete from the cursor to the start of the next word.
 * Date:     September 1, 1991
 * Modified: 27 November, 1996, Jason Hood - treat words similar to word_right
 * Passed:   window:  pointer to current window
 * Notes:    If the cursor is at the right of the line, then combine the
 *            current line with the next one, leaving the cursor where it is.
 *           If the cursor is on an alphanumeric character, then all
 *            subsequent alphanumeric characters are deleted.
 *           If the cursor is on a space, then all subsequent spaces
 *            are deleted.
 *           If the cursor is on a punctuation character, then all
 *            subsequent punctuation characters are deleted.
 */
int  word_delete( TDE_WIN *window )
{
int  len;               /* length of current line */
int  count;             /* number of characters deleted from line */
register int start;     /* column that next word starts in */
text_ptr source;        /* source for block move to delete word */
text_ptr dest;          /* destination for block move */
text_ptr p;
register TDE_WIN *win;  /* put window pointer in a register */
int  rcol;
int  rc;

   win = window;
   if (win->ll->len == EOF)
      return( ERROR );

   rc = OK;
   rcol = win->rcol;
   copy_line( win->ll, win, FALSE );
   adjust_for_tabs( &rcol, win->file_info->inflate_tabs,
                           win->file_info->ptab_size );
   if (rcol >= (len = g_status.line_buff_len)) {
      rc = join_line( win );
      if (rc == OK) {
         p = win->ll->line;
         if (p != NULL) {
            p += win->rcol;
            if (win->rcol < win->ll->len) {
               len = win->ll->len - win->rcol;
               load_undo_buffer( win->file_info, p, len );
            }
         }
      }
   } else {

      assert( len >= 0);
      assert( len < MAX_LINE_LENGTH );

      /*
       * normal word delete
       *
       * find the start of the next word
       */
      start = rcol;
      if (bj_isspace( g_status.line_buff[start] )) {
         /*
          * the cursor was on a space, so eat all consecutive spaces
          *  from the cursor onwards.
          */
         while (start < len  &&  bj_isspace( g_status.line_buff[start] ))
            ++start;
      } else if (myiswhitespc( g_status.line_buff[start] )) {
         /*
          * the cursor was on a white space, so eat all consecutive
          *  white space from the cursor onwards (jmh)
          */
         while (start < len  &&  myiswhitespc( g_status.line_buff[start] ))
            ++start;
      } else {
         /*
          * eat all consecutive characters in the same class (spaces
          *  are considered to be in the same class as the cursor
          *  character)
          */
         while (start < len  &&  !myiswhitespc( g_status.line_buff[start] ))
            ++start;
         while (start < len  &&  bj_isspace( g_status.line_buff[start] ))
            ++start;
      }

      /*
       * move text to delete word
       */
      count = start - rcol;
      source = g_status.line_buff + start;
      dest = g_status.line_buff + rcol;

      shift_tabbed_block( win->file_info );

      assert( len - start >= 0 );

      memmove( dest, source, len - start );
      g_status.line_buff_len = len - count;

      shift_block( win->file_info, win->rline, start, -count );

      entab_linebuff( win->file_info->inflate_tabs, win->file_info->ptab_size );
      win->file_info->modified = TRUE;
      win->file_info->dirty = GLOBAL;
      win->ll->type |= DIRTY;

      /*
       * word_delete is also called by the word processing functions to get
       *   rid of spaces.
       */
      if (g_status.command == WordDelete)
         show_changed_line( win );
   }
   return( rc );
}


/*
 * Name:    dup_line
 * Purpose: Duplicate current line
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   cursor stays on current line
 */
int  dup_line( TDE_WIN *window )
{
register int len;       /* length of current line */
register TDE_WIN *win;  /* put window pointer in a register */
line_list_ptr next_node;
int  rc;

   win = window;

   /*
    * don't dup a NULL line
    */
   if (win->ll->len == EOF || un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
      return( ERROR );

   rc = OK;
   len = win->ll->len;

   assert( len >= 0);
   assert( len < MAX_LINE_LENGTH );

   next_node = new_line_text( win->ll->line, len, DIRTY, &rc );
   if (rc == OK) {
      win->file_info->modified = TRUE;
      if (mode.do_backups == TRUE)
         rc = backup_file( win );

      insert_node( win->file_info, win->ll, next_node );
      syntax_check_lines( next_node, win->file_info->syntax );
      adjust_windows_cursor( win, 1 );
      restore_marked_block( win, 1 );

      /*
       * if current line is the bottom line, we can't see the dup line because
       * cursor doesn't move and dup line is added after current line.
       */
      if (win->cline != win->bottom_line)
         my_scroll_down( win );
      if (win->file_info->dirty != GLOBAL)
         win->file_info->dirty = NOT_LOCAL;

      /*
       * record that file has been modified
       */
      show_size( win );
      show_avail_mem( );
   } else {
      /*
       * cannot duplicate line
       */
      error( WARNING, win->bottom_line, ed5 );
   }
   return( rc );
}


/*
 * Name:     back_space
 * Purpose:  To delete the character to the left of the cursor.
 * Date:     June 5, 1991
 * Modified: August 10, 1997, Jason Hood - added WordDeleteBack
 * Passed:   window:  pointer to current window
 * Notes:    If the cursor is at the left of the line, then combine the
 *            current line with the previous one.
 *           If in indent mode, and the cursor is on the first non-blank
 *            character of the line, then match the indentation of an
 *            earlier line.
 *           In WordDeleteBack, delete all whitespace, if any, then all
 *            non-whitespace up to the first whitespace (similar to WordLeft).
 *            If there is only whitespace, join with the previous line.
 *
 * jmh 980605: Corrected WordDeleteBack error when there was only one space
 *             at the beginning of the line.
 *     980614: Corrected WordDeleteBack error when only leading spaces were
 *             deleted.
 *     980701: Allow blank lines to be deleted if at eof.
 * jmh 981129: Correct sync bug of above.
 *             Recognize cursor update direction. Simply move the other way and
 *              use the insert_overwrite function to overwrite a space. However,
 *              CUR_LEFT in insert mode just calls char_del_under.
 * jmh 020911: Changed WordDeleteBack to join with previous line,
 *              since I added the DelBegOfLine function.
 * jmh 021210: Corrected WordDeleteBack error on blank lines.
 * jmh 050707: Corrected up/down cursor direction beyond EOF.
 */
int  back_space( TDE_WIN *window )
{
int  rc;                /* return code */
int  len;               /* length of the current line */
text_ptr source;        /* source of block move to delete character */
text_ptr dest;          /* destination of block move */
text_ptr p;             /* previous line in file */
int  plen;              /* length of previous line */
int  del_count;         /* number of characters to delete */
int  pos = 0;           /* the position of the first non-blank char */
int  rcol;
int  ccol;
int  old_bcol;
register TDE_WIN *win;  /* put window pointer in a register */
TDE_WIN *wp;
line_list_ptr temp_ll;
int  tabs;
int  tab_size;

   win = window;
   if (win->ll->len == EOF) {
      if (win->ll->prev == NULL || win->ll->prev->len != 0)
         rc = ERROR;
      else {
         prepare_move_up( win );
         rc = line_kill( win );
      }
      return( rc );
   }
   rc   = OK;
   tabs = win->file_info->inflate_tabs;
   tab_size = win->file_info->ptab_size;
   rcol = win->rcol;
   if (mode.cur_dir != CUR_RIGHT) {
      if (mode.cur_dir == CUR_LEFT)
         ++rcol;
      else if (mode.cur_dir == CUR_DOWN) {
         if (win->ll->prev->len == EOF)
            rc = ERROR;
         else
            prepare_move_up( win );
      } else /* mode.cur_dir == CUR_UP */ {
         if (win->ll->next->len == EOF)
            rc = ERROR;
         else
            prepare_move_down( win );
      }
      if (rc == OK) {
         if (g_status.copied)
            g_status.line_buff[rcol] = ' ';
         else {
            rcol = entab_adjust_rcol( win->ll->line, win->ll->len, rcol,
                                      tab_size );
            if (rcol < win->ll->len)
               win->ll->line[rcol] = ' ';
         }
         show_curl_line( win );
         if (mode.cur_dir == CUR_LEFT)
            check_virtual_col( win, rcol, ++win->ccol );
      }
      return( rc );
   }

   copy_line( win->ll, win, FALSE );
   adjust_for_tabs( &rcol, tabs, tab_size );
   len = g_status.line_buff_len;
   ccol = win->ccol;
   old_bcol = win->bcol;

   if (g_status.command == WordDeleteBack) {
      pos = (rcol < len) ? rcol-1 : len-1;
      while (pos >= 0 && myiswhitespc( g_status.line_buff[pos] ))
         --pos;
      if (pos == -1 && win->rline > 1 && rcol <= len) {
         /*
          * remove the leading whitespace prior to joining
          */
         g_status.line_buff_len -= rcol;
         memcpy( g_status.line_buff, g_status.line_buff + rcol,
                 g_status.line_buff_len );
         rcol = 0;
      }
   }

   if (rcol == 0) {
      if (win->rline > 1) {
         /*
          * combine this line with the previous, if any
          */

         assert( win->ll->prev != NULL );

         p = win->ll->prev->line;
         plen = win->ll->prev->len;
         if (len + 2 + plen >= MAX_LINE_LENGTH) {
            /*
             * cannot combine lines
             */
            error( WARNING, win->bottom_line, ed4 );
            return( ERROR );
         }

         win->file_info->modified = TRUE;
         if ((rc = un_copy_line( win->ll, win, TRUE, FALSE )) == OK) {
            dec_line( win, FALSE );
            win->ll->type |= DIRTY;
            copy_line( win->ll, win, TRUE );
            len = g_status.line_buff_len;
            win->rcol = rcol = len;

            p = win->ll->next->line;
            plen = win->ll->next->len;

            /*
             * copy previous line into new previous line.
             */
            assert( plen >= 0 );
            assert( len  >= 0 );

            my_memcpy( g_status.line_buff+len, p, plen );
            g_status.line_buff_len = len + plen;

            load_undo_buffer( win->file_info, p, plen );
            my_free( p );

            temp_ll = win->ll->next;

            if (temp_ll->prev != NULL)
               temp_ll->prev->next = temp_ll->next;
            temp_ll->next->prev = temp_ll->prev;

            syntax_check_lines( win->ll, win->file_info->syntax );

            --win->file_info->length;
            ++win->rline;
            adjust_windows_cursor( win, -1 );
            restore_marked_block( win, -1 );
            --win->rline;

            wp = g_status.window_list;
            while (wp != NULL) {
               if (wp->file_info == win->file_info) {
                  if (wp != win) {
                     if (wp->ll == temp_ll)
                        wp->ll = win->ll->next;
                  }
               }
               wp = wp->next;
            }

            my_free( temp_ll );

            if (win->cline > win->top_line)
               --win->cline;

            /*
             * make sure cursor stays on the screen, at the end of the
             *  previous line
             */
            ccol = rcol - win->bcol;
            show_size( win );
            show_avail_mem( );
            check_virtual_col( win, rcol, ccol );
            win->file_info->dirty = GLOBAL;
            show_ruler( win );
         }
      } else
         return( ERROR );
   } else {
      if (g_status.command == WordDeleteBack) {
         while (pos >= 0 && !myiswhitespc( g_status.line_buff[pos] ))
            --pos;
         del_count = rcol - pos - 1;
      } else {
         /*
          * normal delete
          *
          * find out how much to delete (depends on indent mode)
          */
         del_count = 1;   /* the default */
         if (mode.indent) {
            /*
             * indent only happens if the cursor is on the first
             *  non-blank character of the line
             */
            pos = first_non_blank( g_status.line_buff, len, tabs, tab_size );
            if (pos == win->rcol  ||
                is_line_blank( g_status.line_buff, len, tabs )) {
               /*
                * now work out how much to indent
                */
               temp_ll = win->ll->prev;
               for (; temp_ll != NULL; temp_ll=temp_ll->prev) {
                  p = temp_ll->line;
                  if (!is_line_blank( p, temp_ll->len, tabs )) {
                     plen = first_non_blank( p, temp_ll->len, tabs, tab_size );
                     if (plen < win->rcol) {
                        /*
                         * found the line to match
                         *
                         * jmh 980524: remove the tabs if using real tabs
                         * jmh 990502: that was dumb - just adjust plen
                         * jmh 991004: add spaces, too
                         */
                        if (tabs == 2 && mode.insert) {
                           adjust_for_tabs( &plen, tabs, tab_size );
                           len = g_status.line_buff_len;
                           rcol = entab_adjust_rcol( g_status.line_buff, len,
                                                     win->rcol, tab_size );
                        }
                        del_count = rcol - plen;
                        break;
                     }
                  }
               }
            }
         }
      }

      /*
       * move text to delete char(s), unless no chars actually there
       */
      if (rcol - del_count < len) {
         dest = g_status.line_buff + rcol - del_count;
         if (rcol > len) {
            source = g_status.line_buff + len;
            pos    = 0;
            len    = rcol - del_count;
         } else {
            source = g_status.line_buff + rcol;
            pos    = len - rcol;
            len   -= del_count;
         }
         shift_tabbed_block( win->file_info );

         assert( pos >= 0 );
         assert( len >= 0 );
         assert( len <= MAX_LINE_LENGTH );

         memmove( dest, source, pos );
         g_status.line_buff_len = len;
         shift_block( win->file_info, win->rline, rcol, -del_count );
         if (tabs == 2 && mode.insert) {
            pos = detab_adjust_rcol( g_status.line_buff, rcol - del_count,
                                     tab_size );
            del_count = win->rcol - pos;
         }
         entab_linebuff( tabs, tab_size );
      }
      rcol  = win->rcol - del_count;
      ccol -= del_count;
      win->file_info->dirty = NOT_LOCAL;
      win->ll->type |= DIRTY;
      show_ruler_char( win );
      show_changed_line( win );
      check_virtual_col( win, rcol, ccol );
      if (!win->file_info->dirty)
         show_curl_line( win );
      if (old_bcol != win->bcol)
         show_ruler( win );
   }
   win->file_info->modified = TRUE;
   return( rc );
}


/*
 * Name:    transpose
 * Purpose: swap two characters
 * Date:    September 11, 1997
 * Author:  Jason Hood
 * Passed:  window:  pointer to current window
 * Notes:   swaps the current character with the previous character;
 *           if at bol (ie. column 1) swap with the next character;
 *           if at eol (ie. one more than length) swap the two previous chars.
 *
 * 980502:  Test line first; detabbed the buffer before testing length.
 */
int  transpose( TDE_WIN *window )
{
register TDE_WIN *win;
char swap;              /* the swapped character */
int  which;             /* whether it's previous or next */
int  rcol;              /* the current character */
int  len;               /* length of the line */

   win = window;
   if (win->ll->len == EOF)
      return( OK );
   rcol = win->rcol;
   copy_line( win->ll, win, TRUE );
   len = g_status.line_buff_len;
   if (len < 2 || rcol > len)
      return( OK );
   if (rcol == len)
      --rcol;
   which = (rcol > 0) ? -1 : 1;
   swap = g_status.line_buff[rcol+which];
   g_status.line_buff[rcol+which] = g_status.line_buff[rcol];
   g_status.line_buff[rcol] = swap;
   entab_linebuff( win->file_info->inflate_tabs, win->file_info->ptab_size );
   win->file_info->dirty    = GLOBAL;
   win->file_info->modified = TRUE;
   window->ll->type |= DIRTY;
   show_changed_line( win );
   return( OK );
}


/*
 * Name:    line_kill
 * Purpose: To delete the line the cursor is on.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   win->ll->s == NULL then do not do a
 *          line kill (can't kill a NULL line).
 */
int  line_kill( TDE_WIN *window )
{
register TDE_WIN *win;   /* put window pointer in a register */
register TDE_WIN *wp;
line_list_ptr killed_node;
int  rc;

   win = window;
   killed_node = win->ll;
   rc = OK;
   if (killed_node->len != EOF) {
      win->file_info->modified = TRUE;
      if (mode.do_backups == TRUE)
         rc = backup_file( win );

      if (rc == OK) {
         load_undo_buffer( win->file_info,
            g_status.copied ? g_status.line_buff     : killed_node->line,
            g_status.copied ? g_status.line_buff_len : killed_node->len );

         --win->file_info->length;

         win->ll = win->ll->next;

         killed_node->prev->next = killed_node->next;
         killed_node->next->prev = killed_node->prev;

         wp = g_status.window_list;
         while (wp != NULL) {
            if (wp->file_info == win->file_info) {
               if (wp != win) {
                  if (wp->ll == killed_node)
                     wp->ll = win->ll;
               }
            }
            wp = wp->next;
         }

         /*
          * free the line and the node
          */
         my_free( killed_node->line );
         my_free( killed_node );

         win->file_info->dirty = NOT_LOCAL;

         g_status.copied = FALSE;
         /*
          * move all cursors one according to i, restore begin and end block
          */
         adjust_windows_cursor( win, -1 );
         restore_marked_block( win, -1 );
         syntax_check_lines( win->ll, win->file_info->syntax );

         /*
          * we are not doing a GLOBAL update, so update current window here
          */
         if (win->file_info->dirty == NOT_LOCAL)
            my_scroll_down( win );
         show_size( win );
         show_avail_mem( );
      }
   } else
      rc = ERROR;
   return( rc );
}


/*
 * Name:    char_del_under
 * Purpose: To delete the character under the cursor.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   If the cursor is beyond the end of the line, then this
 *           command is ignored.
 *          DeleteChar and StreamDeleteChar use this function.
 *
 * jmh 980524: added test for real tabs.
 */
int  char_del_under( TDE_WIN *window )
{
text_ptr source;        /* source of block move to delete character */
int  len;
register TDE_WIN *win;  /* put window pointer in a register */
int  rcol;

   win = window;
   if (win->ll->len == EOF)
      return( OK );
   rcol = win->rcol;
   copy_line( win->ll, win, FALSE );
   adjust_for_tabs( &rcol, win->file_info->inflate_tabs,
                           win->file_info->ptab_size );

   if (rcol < (len = g_status.line_buff_len)) {
      /*
       * move text to delete using buffer
       */
      source = g_status.line_buff + rcol + 1;

      assert( len - rcol >= 0 );
      shift_tabbed_block( win->file_info );
      memmove( source-1, source, len - rcol );
      --g_status.line_buff_len;
      shift_block( win->file_info, win->rline, rcol+1, -1 );
      entab_linebuff( win->file_info->inflate_tabs, win->file_info->ptab_size );
      win->file_info->dirty    = GLOBAL;
      win->file_info->modified = TRUE;
      win->ll->type |= DIRTY;
      show_changed_line( win );
   } else if (g_status.command == StreamDeleteChar)
      join_line( win );
   else if (win->file_info->modified == FALSE)
      /*
       * prevent un_copy_line from indicating the file is modified
       */
      g_status.copied = FALSE;

   return( OK );
}


/*
 * Name:    eol_kill
 * Purpose: To delete everything from the cursor to the end of the line.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   If the cursor is beyond the end of the line, then this
 *           command is ignored.
 */
int  eol_kill( TDE_WIN *window )
{
register TDE_WIN *win;   /* put window pointer in a register */

   win = window;
   if (win->ll->len == EOF)
      return( OK );
   copy_line( win->ll, win, TRUE );
   if (win->rcol < g_status.line_buff_len) {
      load_undo_buffer( win->file_info, g_status.line_buff,
                                        g_status.line_buff_len );
      /*
       * truncate to delete rest of line
       */
      shift_block( win->file_info, win->rline, g_status.line_buff_len,
                   win->rcol - g_status.line_buff_len );
      g_status.line_buff_len = win->rcol;
      entab_linebuff( win->file_info->inflate_tabs, win->file_info->ptab_size );
      win->file_info->dirty = GLOBAL;
      win->file_info->modified = TRUE;
      win->ll->type |= DIRTY;
      show_changed_line( win );
   }
   return( OK );
}


/*
 * Name:    bol_kill
 * Purpose: to delete or erase everything from beginning of line till the cursor
 * Author:  Jason Hood
 * Date:    September 11, 2002
 * Passed:  window:  pointer to current window
 * Notes:   delete works similar to home: it will take the first non-blank as
 *           the beginning of the line, then the first column.
 *          erase will overwrite with blanks, rather than actually delete.
 *          ignored if cursor is already at the first column.
 *
 * 021011:  fixed delete bugs beyond EOL, on non-empty blank lines and tabs.
 * 060914:  update syntax highlighting on GLOBAL change.
 */
int  bol_kill( TDE_WIN *window )
{
register TDE_WIN *win;   /* put window pointer in a register */
int  rcol, bol;
int  del_count;
int  tabs, tab_size;
int  old_bcol;

   win = window;
   if (win->ll->len == EOF)
      return( OK );

   rcol = win->rcol;
   tabs = win->file_info->inflate_tabs;
   tab_size = win->file_info->ptab_size;

   if (rcol != 0) {
      copy_line( win->ll, win, (g_status.command == EraseBegOfLine) );
      load_undo_buffer( win->file_info, g_status.line_buff,
                                        g_status.line_buff_len );
      if (g_status.command == EraseBegOfLine)
         memset( g_status.line_buff, ' ', rcol );
      else {
         adjust_for_tabs( &rcol, tabs, tab_size );
         old_bcol =
              bol = first_non_blank( g_status.line_buff, g_status.line_buff_len,
                                     tabs, tab_size );
         if (tabs)
            bol = entab_adjust_rcol( g_status.line_buff, g_status.line_buff_len,
                                     bol, tab_size );
         if (rcol <= bol  ||  is_line_blank( g_status.line_buff,
                                             g_status.line_buff_len, tabs ))
            bol = 0;
         shift_tabbed_block( win->file_info );
         if (rcol >= g_status.line_buff_len) {
            del_count = win->rcol - old_bcol;
            g_status.line_buff_len = bol;
         } else {
            memcpy( g_status.line_buff + bol, g_status.line_buff + rcol,
                    g_status.line_buff_len - rcol );
            del_count = rcol - bol;
            g_status.line_buff_len -= del_count;
            if (tabs == 2  &&  mode.insert) {
               old_bcol = detab_adjust_rcol(g_status.line_buff, bol, tab_size);
               del_count = win->rcol - old_bcol;
            }
         }
         shift_block( win->file_info, win->rline, rcol, bol - rcol );
         show_ruler_char( win );
         old_bcol = win->bcol;
         check_virtual_col( win, win->rcol - del_count, win->ccol - del_count );
         if (old_bcol != win->bcol)
            show_ruler( win );
      }
      entab_linebuff( tabs, tab_size );
      win->file_info->modified = TRUE;
      win->ll->type |= DIRTY;
      if (!win->file_info->dirty) {
         win->file_info->dirty = GLOBAL;
         show_changed_line( win );
      } else {
         win->file_info->dirty = GLOBAL;
         syntax_check_lines( win->ll, win->file_info->syntax );
      }
   }
   return( OK );
}


/*
 * Name:    set_tabstop
 * Purpose: To set the current interval between tab stops
 * Date:    October 1, 1989
 * Notes:   Tab interval must be reasonable, and this function will
 *           not allow tabs more than g_display.ncols / 2.
 * jmh 991126: ensure tabs are greater than 0.
 * jmh 021028: allow tabs to be up to g_display.ncols.
 * jmh 021105: use MAX_TAB_SIZE define.
 */
int  set_tabstop( TDE_WIN *window )
{
register int rc;
file_infos *file;

   file = window->file_info;
   
   set_dlg_num( EF_Logical,  file->ltab_size );
   set_dlg_num( EF_Physical, file->ptab_size );

   rc = do_dialog( tabs_dialog, tabs_proc );

   if (rc == OK) {
      file->ltab_size = (int)get_dlg_num( EF_Logical );
      file->ptab_size = (int)get_dlg_num( EF_Physical );
      show_tab_modes( );
      if (file->inflate_tabs)
         file->dirty = GLOBAL;
   }
   return( rc );
}


/*
 * Name:    tabs_proc
 * Purpose: dialog callback for SetTabs
 * Author:  Jason Hood
 * Date:    December 1, 2003
 * Notes:   ensure tabs don't go beyond limits.
 */
int  tabs_proc( int id, char *text )
{
int  tab;
int  rc = OK;

   if (id == IDE_LOGICAL || id == IDE_PHYSICAL) {
      tab = atoi( text );
      if (tab < 1 || tab > MAX_TAB_SIZE) {
         /*
          * tab size too long
          */
         error( WARNING, g_display.end_line, (id==IDE_LOGICAL) ? ed8a : ed8b );
         rc = ERROR;
      }
   }
   return( rc );
}


/*
 * Name:    dynamic_tab_size
 * Purpose: interactively change the physical tab size
 * Author:  Jason Hood
 * Date:    March 4, 2003
 * Notes:   use left/right to change by one, up/down by four.
 */
int  dynamic_tab_size( TDE_WIN *window )
{
TDE_WIN *win[20];
file_infos *file;
int  tab, old_tab;
int  prompt_line;
long key;
int  func;
DISPLAY_BUFF;
char str[6];
int  cnt, j;

   file = window->file_info;
   if (file->ref_count > 1) {
      cnt = -1;
      for (window = g_status.window_list; window; window = window->next)
         if (window->file_info == file  &&  window->visible  &&  cnt < 19)
            win[++cnt] = window;
   } else {
      win[0] = window;
      cnt = 0;
   }
   tab = old_tab = file->ptab_size;
   prompt_line = g_display.mode_line;
   SAVE_LINE( prompt_line );
   /*
    * Tab size:
    */
   eol_clear( 0, prompt_line, Color( Help ) );
   s_output( ed7c, prompt_line, 0, Color( Help ) );
   xygoto( -1, -1 );

   do {
      sprintf( str, "%d ", file->ptab_size );
      s_output( str, prompt_line, ED7C_SLOT, Color( Help ) );
      key = getkey( );
      func = (key == _ENTER) ? Rturn :
             (key == _ESC)   ? AbortCommand:
             getfunc( key );
      switch (func) {
         case CharLeft:
         case StreamCharLeft:
            if (file->ptab_size > 1)
               --file->ptab_size;
            break;
         case CharRight:
         case StreamCharRight:
            if (file->ptab_size < MAX_TAB_SIZE)
               ++file->ptab_size;
            break;
         case LineUp:
            if (file->ptab_size > 4)
               file->ptab_size -= 4;
            else
               file->ptab_size = 1;
            break;
         case LineDown:
            if (file->ptab_size < MAX_TAB_SIZE - 3)
               file->ptab_size += 4;
            else
               file->ptab_size = MAX_TAB_SIZE;
            break;
      }
      if (tab != file->ptab_size) {
         tab = file->ptab_size;
         for (j = cnt; j >= 0; --j)
            display_current_window( win[j] );
      }
   } while (func != Rturn && func != AbortCommand);

   RESTORE_LINE( prompt_line );

   if (old_tab != file->ptab_size) {
      if (func == Rturn) {
         if (file->ref_count > 20)
            file->dirty = GLOBAL;
         show_tab_modes( );
      } else {
         file->ptab_size = old_tab;
         file->dirty = GLOBAL;
      }
   }

   return( OK );
}


/*
 * Name:    show_line_col
 * Purpose: show current real line and column of current cursor position
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   Blank old position and display new position.  current line and
 *          column may take up to 12 columns, which allows the display of
 *          9,999 columns and 9,999,999 lines.
 *
 * jmh 980804: prevent display of current character when resizing windows.
 *             actually made the line:col 12 columns, not the 13 it was (it
 *              was either that, or make min window width 16; since 10 million
 *              lines is at least 20 million bytes (crlf) I don't think it's
 *              a problem).
 * jmh 980822: display marker indicators in the mode line.
 * jmh 990408: modified current character display code;
 *             made hex_digit static;
 *             added base offset (line:col+ofs) to indicate horizontal scroll.
 * jmh 990915: added " =EOL" when cursor is at end-of-line.
 * jmh 990923: corrected binary offset when used with inflated tabs.
 * jmh 990924: only blank the markers when necessary, since it appears to
 *              flicker on fast computers.
 * jmh 991027: don't display markers when sizing windows.
 * jmh 991029: don't display with SwapBlock as well.
 * jmh 021028: don't display with ISearch*.
 * jmh 030330: corrected position of binary offset (relative to right edge).
 * jmh 050708: don't display base offset on large files.
 */
void show_line_col( TDE_WIN *window )
{
int  i, j;
register int k;
char line_col[20];
static const char hex_digit[] = "0123456789abcdef";
file_infos *file;
static long old_rline;
int  do_mode;

   line_col[19] = '\0';
   k = 19;

   /*
    * convert base offset to ascii and store in display buffer.
    */
   if (window->bcol != 0) {
      i = numlen( window->bcol );
      if (i + numlen( window->rcol+1 ) + numlen( window->rline ) + 2 <= 12) {
         k -= i;
         my_ltoa( window->bcol, line_col + k, 10 );
         line_col[--k] = '+';
      }
   }

   /*
    * convert column to ascii and store in display buffer.
    */
   i = numlen( window->rcol+1 );
   j = line_col[k];
   k -= i;
   my_ltoa( window->rcol+1, line_col + k, 10 );
   line_col[k+i] = j;

   /*
    * convert line to ascii and store in display buffer.
    */
   i = numlen( window->rline );
   k -= i + 1;
   my_ltoa( window->rline, line_col + k, 10 );

   /*
    * put in colon to separate line and column
    */
   line_col[k+i] = ':';

   /*
    * blank out the remainder of the display.
    */
   while (--k >= 7)
      line_col[k] = ' ';

   /*
    * find line to start line:column display then output
    */
   s_output( line_col+7, window->top, window->end_col-11, Color( Head ) );

   file = window->file_info;

   do_mode = (g_status.command != SizeWindow  &&
              g_status.command != SwapBlock   &&
              g_status.command != PopupRuler  &&
              !(g_status.search & SEARCH_I));
   if (do_mode  &&  !mode.draw) {
      strcpy( line_col, " =   " );
      i = window->rcol;
      k = -2;
      if (g_status.copied) {
         if (file->inflate_tabs)
            i = entab_adjust_rcol( g_status.line_buff, g_status.line_buff_len,
                                   i, file->ptab_size );
         if (i < g_status.line_buff_len)
            k = g_status.line_buff[i];
         else if (i == g_status.line_buff_len)
            k = -1;
      } else {
         if (file->inflate_tabs  &&  window->ll->len != EOF)
            i = entab_adjust_rcol( window->ll->line, window->ll->len, i,
                                   file->ptab_size );
         if (i < window->ll->len)
            k = window->ll->line[i];
         else if (i == window->ll->len)
            k = -1;
      }
      if (k >= 0) {
         line_col[0] = k;
         line_col[2] = hex_digit[(k >> 4)];
         line_col[3] = hex_digit[(k & 0x0f)];
         line_col[4] = 'x';
      } else if (k == -1)
         strcpy( line_col+2, EOL_TEXT );

      c_output( *line_col,  58, g_display.mode_line, Color( Mode ) );
      s_output( line_col+1, g_display.mode_line, 59, Color( Mode ) );
   }

   /*
    * if file was opened in binary mode, show offset from beginning of file.
    */
   if (file->crlf == BINARY && !window->vertical) {
      k =  window->ll->line == NULL  ?  0  :  window->rcol;
      if (file->inflate_tabs  &&  k != 0) {
         if (g_status.copied)
            k = entab_adjust_rcol( g_status.line_buff, g_status.line_buff_len,
                                   k, file->ptab_size );
         else
            k = entab_adjust_rcol( window->ll->line, window->ll->len, k,
                                   file->ptab_size );
      }
      n_output( window->bin_offset + k, 7,
                window->end_col - 18, window->top, Color( Head ) );
   }
   show_asterisk( window );

   /*
    * Indicate if the line has a marker by displaying its number in the
    * mode line, using the diagnostic color. Number 0 represents previous
    * position.
    */
   if (!g_status.wrapped && do_mode) {
      for (i = 0, k = 0; k < NO_MARKERS; ++k) {
         if (file->marker[k].marked &&
             file->marker[k].rline == window->rline)
            line_col[i++] = k + '0';
      }
      if (old_rline != window->rline) {
         show_search_message( CLR_SEARCH );
         old_rline = window->rline;
      }
      if (i) {
         line_col[i] = '\0';
         s_output( line_col, g_display.mode_line, 75 - i, Color( Diag ) );
      }
   }
}


/*
 * Name:    show_asterisk
 * Purpose: give user an indication if file is dirty
 * Date:    September 16, 1991
 * Passed:  window:  pointer to current window
 *
 * jmh 990428: if in read-only mode, display an exclamation mark.
 * jmh 991108: take into account line number display.
 * jmh 030323: display '!' beside '*'; use '#' for scratch windows.
 */
void show_asterisk( TDE_WIN *window )
{
char buf[3];

   buf[0] = ' ';
   buf[1] = (window->file_info->scratch)  ? '#' :
            (window->file_info->modified) ? '*' :
            ' ';
   buf[2] = '\0';
   if (window->file_info->read_only)
      buf[(buf[1] == ' ')] = '!';
   s_output( buf, window->top, window->left+3, Color( Head ) );
}


/*
 * Name:    toggle_overwrite
 * Purpose: toggle overwrite-insert mode
 * Date:    September 16, 1991
 * Passed:  arg_filler:  argument to satisfy function prototype
 * jmh 981130: disable toggling when cursor update direction is not normal.
 * jmh 990404: move set_cursor_size() to show_insert_mode().
 */
int  toggle_overwrite( TDE_WIN *arg_filler )
{
   if (mode.cur_dir == CUR_RIGHT) {
      mode.insert = !mode.insert;
      show_insert_mode( );
   }
   return( OK );
}


/*
 * Name:    toggle_smart_tabs
 * Purpose: toggle smart tab mode
 * Date:    June 5, 1992
 * Passed:  arg_filler:  argument to satisfy function prototype
 */
int  toggle_smart_tabs( TDE_WIN *arg_filler )
{
   mode.smart_tab = !mode.smart_tab;
   show_tab_modes( );
   return( OK );
}


/*
 * Name:    toggle_indent
 * Purpose: toggle indent mode
 * Date:    September 16, 1991
 * Passed:  arg_filler:  argument to satisfy function prototype
 */
int  toggle_indent( TDE_WIN *arg_filler )
{
   mode.indent = !mode.indent;
   show_indent_mode( );
   return( OK );
}


/*
 * Name:    set_margins
 * Purpose: set left, paragraph and right margin for word wrap
 * Date:    November 27, 1991
 * Passed:  window
 *
 * jmh 031115: combined the three individual margins into one function.
 */
int  set_margins( TDE_WIN *window )
{
int  rc;

   set_dlg_num( EF_Left,  mode.left_margin  + 1 );
   set_dlg_num( EF_Right, mode.right_margin + 1 );
   set_dlg_num( EF_Para,  mode.parg_margin  + 1 );
   CB_Justify = mode.right_justify;

   rc = do_dialog( margins_dialog, margins_proc );

   if (rc == OK) {
      mode.left_margin   = (int)get_dlg_num( EF_Left  ) - 1;
      mode.right_margin  = (int)get_dlg_num( EF_Right ) - 1;
      mode.parg_margin   = (int)get_dlg_num( EF_Para  ) - 1;
      mode.right_justify = CB_Justify;
      show_all_rulers( );
   }
   return( rc );
}


/*
 * Name:    margins_proc
 * Purpose: dialog callback for SetMargins
 * Author:  Jason Hood
 * Date:    November 30, 2003
 * Notes:   ensure left and paragraph margins are less than right margin;
 *          ensure right margin is less than maximum line length.
 */
int  margins_proc( int id, char *text )
{
int  left, right, para;
int  rc = OK;

   if (id == 0) {
      left  = (int)get_dlg_num( EF_Left  );
      right = (int)get_dlg_num( EF_Right );
      para  = (int)get_dlg_num( EF_Para  );

      if (right < 1 || right > MAX_LINE_LENGTH ||
          (right <= left && right <= para)) {
         /*
          * right margin out of range
          */
         error( WARNING, g_display.end_line, ed12 );
         rc = IDE_RIGHT;

      } else if (left < 1 || left >= right) {
         /*
          * left margin out of range
          */
         error( WARNING, g_display.end_line, ed10 );
         rc = IDE_LEFT;

      } else if (para < 1 || para >= right) {
         /*
          * paragraph margin out of range
          */
         error( WARNING, g_display.end_line, ed14 );
         rc = IDE_PARA;
      }
   }
   return( rc );
}


/*
 * Name:    toggle_crlf
 * Purpose: toggle crlf mode
 * Date:    November 27, 1991
 * Passed:  arg_filler:  argument to satisfy function prototype
 *
 * jmh 980702: display size to remove offset when changing from binary
 */
int  toggle_crlf( TDE_WIN *window )
{
register TDE_WIN *w;

   ++window->file_info->crlf;
   if (window->file_info->crlf > BINARY)
      window->file_info->crlf = CRLF;
   w = g_status.window_list;
   while (w != NULL) {
      if (w->file_info == window->file_info  &&  w->visible) {
         show_crlf_mode( w );
         show_size( w );
      }
      w = w->next;
   }
   return( OK );
}


/*
 * Name:    toggle_ww
 * Purpose: toggle word wrap mode
 * Date:    November 27, 1991
 * Passed:  arg_filler:  argument to satisfy function prototype
 */
int  toggle_ww( TDE_WIN *arg_filler )
{
   ++mode.word_wrap;
   if (mode.word_wrap > DYNAMIC_WRAP)
      mode.word_wrap = NO_WRAP;
   show_wordwrap_mode( );
   return( OK );
}


/*
 * Name:    toggle_trailing
 * Purpose: toggle eleminating trainling space at eol
 * Date:    November 25, 1991
 * Passed:  arg_filler:  argument to satisfy function prototype
 */
int  toggle_trailing( TDE_WIN *arg_filler )
{
   mode.trailing = !mode.trailing;
   show_trailing( );
   return( OK );
}


/*
 * Name:    toggle_z
 * Purpose: toggle writing control z at eof
 * Date:    November 25, 1991
 * Passed:  arg_filler:  argument to satisfy function prototype
 */
int  toggle_z( TDE_WIN *arg_filler )
{
   mode.control_z = !mode.control_z;
   show_control_z( );
   return( OK );
}


/*
 * Name:    toggle_eol
 * Purpose: toggle writing eol character at eol
 * Date:    November 25, 1991
 * Passed:  arg_filler:  argument to satisfy function prototype
 * jmh 980725: added a third state - display a character if the line
 *              extends beyond the window edge.
 */
int  toggle_eol( TDE_WIN *arg_filler )
{
register file_infos *file;

   if (++mode.show_eol == 3)
      mode.show_eol = 0;
   for (file=g_status.file_list; file != NULL; file=file->next)
      file->dirty = GLOBAL;
   return( OK );
}


/*
 * Name:    toggle_search_case
 * Purpose: toggle search case
 * Date:    September 16, 1991
 * Passed:  arg_filler:  argument to satisfy function prototype
 *
 * jmh 010704: work with regx.
 */
int  toggle_search_case( TDE_WIN *arg_filler )
{
   mode.search_case = (mode.search_case == IGNORE) ? MATCH : IGNORE;
   show_search_case( );
   build_boyer_array( );
   if (regx.search_defined == OK)
      build_nfa( );
   return( OK );
}


/*
 * Name:    toggle_sync
 * Purpose: toggle sync mode
 * Date:    January 15, 1992
 * Passed:  arg_filler:  argument to satisfy function prototype
 */
int  toggle_sync( TDE_WIN *arg_filler )
{
   mode.sync = !mode.sync;
   show_sync_mode( );
   return( OK );
}


/*
 * Name:    toggle_ruler
 * Purpose: toggle ruler
 * Date:    March 5, 1992
 * Passed:  arg_filler:  argument to satisfy function prototype
 */
int  toggle_ruler( TDE_WIN *arg_filler )
{
register TDE_WIN *wp;

   mode.ruler = !mode.ruler;
   wp = g_status.window_list;
   while (wp != NULL) {
      /*
       * there has to be more than one line in a window to display a ruler.
       *   even if the ruler mode is on, we need to check the num of lines.
       */
      wp->ruler = (mode.ruler  &&  wp->bottom_line != wp->top_line);
      setup_window( wp );
      check_cline( wp, wp->cline );
      if (wp->visible)
         redraw_current_window( wp );
      wp = wp->next;
   }
   return( OK );
}


/*
 * Name:    toggle_tabinflate
 * Purpose: toggle inflating tabs
 * Date:    October 31, 1992
 * Passed:  window:  pointer to current window
 */
int  toggle_tabinflate( TDE_WIN *window )
{
register file_infos *file;

   file = window->file_info;
   ++file->inflate_tabs;
   if (file->inflate_tabs > 2)
      file->inflate_tabs = 0;
   file->dirty = GLOBAL;
   show_tab_modes( );
   return( OK );
}


/*
 * Name:    toggle_cursor_cross
 * Purpose: toggle cursor cross
 * Author:  Jason Hood
 * Date:    July 24, 1998
 * Passed:  arg_filler:  argument to satisfy function prototype
 */
int  toggle_cursor_cross( TDE_WIN *arg_filler )
{
register file_infos *file;

   mode.cursor_cross = !mode.cursor_cross;
   for (file=g_status.file_list; file != NULL; file=file->next)
      file->dirty = GLOBAL;
   return( OK );
}


/*
 * Name:    toggle_graphic_chars
 * Purpose: toggle graphic characters (on/off - not different sets)
 * Author:  Jason Hood
 * Date:    July 24, 1998
 * Passed:  arg_filler:  argument to satisfy function prototype
 */
int  toggle_graphic_chars( TDE_WIN *arg_filler )
{
   mode.graphics = -mode.graphics;
   show_graphic_chars( );
   return( OK );
}


/*
 * Name:    change_cur_dir
 * Purpose: to change the cursor update direction
 * Author:  Jason Hood
 * Date:    November 29, 1998
 * Passed:  arg_filler:  argument to satisfy function prototype
 * Notes:   use the arrow keys to set the new direction.
 *          set overwrite mode, if necessary.
 */
int  change_cur_dir( TDE_WIN *arg_filler )
{
int  func;
int  rc = OK;
static int io_mode_changed = FALSE;     /* remember that it was insert mode */

   /*
    * Press the key for cursor update direction.
    */
   func = getfunc( prompt_key( ed21, g_display.end_line ) );
   switch (func) {
      case Rturn           :
      case StreamCharRight :
      case CharRight       : func = CUR_RIGHT; break;
      case StreamCharLeft  :
      case CharLeft        : func = CUR_LEFT;  break;
      case LineDown        : func = CUR_DOWN;  break;
      case LineUp          : func = CUR_UP;    break;
      default              : rc   = ERROR;     break;
   }
   if (rc == OK) {
      if (func != CUR_RIGHT && mode.insert) {
         toggle_overwrite( NULL );      /* NOTE: dummy argument */
         io_mode_changed = TRUE;
      }
      mode.cur_dir = func;
      if (func == CUR_RIGHT && io_mode_changed) {
         toggle_overwrite( NULL );      /* NOTE: dummy argument */
         io_mode_changed = FALSE;
      }
      show_cur_dir( );
   }

   return( rc );
}


/*
 * Name:    toggle_read_only
 * Purpose: toggle the read-only (viewer) flag
 * Author:  Jason Hood
 * Date:    April 28, 1999
 * Passed:  window:  pointer to current window
 * Notes:   in read-only mode, the file cannot be changed. It also allows
 *           normal characters to be used for functions.
 */
int  toggle_read_only( TDE_WIN *window )
{
register TDE_WIN *wp;
file_infos *fp = window->file_info;

   fp->read_only = !fp->read_only;
   for (wp = g_status.window_list; wp != NULL; wp = wp->next) {
      if (wp->file_info == fp  &&  wp->visible)
         show_asterisk( wp );
   }
   return( OK );
}


/*
 * Name:    toggle_draw
 * Purpose: turn drawing mode on or off
 * Author:  Jason Hood
 * Date:    October 18, 1999
 * Passed:  arg_filler:  argument to satisfy function prototype
 * Notes:   turn on the graphic characters
 */
int  toggle_draw( TDE_WIN *arg_filler )
{
static int graphics = FALSE;

   mode.draw = !mode.draw;
   show_draw_mode( );

   if (mode.draw) {
      if (mode.graphics < 0) {
         toggle_graphic_chars( NULL );
         graphics = TRUE;
      }
   } else {
      if (graphics) {
         graphics = FALSE;
         if (mode.graphics > 0)
            toggle_graphic_chars( NULL );
      }
   }
   return( OK );
}


/*
 * Name:    toggle_line_numbers
 * Purpose: turn constant line number display on or off
 * Author:  Jason Hood
 * Date:    November 8, 1999
 * Passed:  arg_filler:  argument to satisfy function prototype
 * Notes:   modify start_col to skip the line number digits. This is known
 *           to be safe, since the window must be at least 15 characters
 *           and the line number display can be at most 10.
 */
int  toggle_line_numbers( TDE_WIN *arg_filler )
{
register TDE_WIN *wp;
file_infos *fp;

   mode.line_numbers = !mode.line_numbers;

   for (fp = g_status.file_list; fp != NULL; fp = fp->next) {
      if (mode.line_numbers)
         fp->len_len = numlen( fp->length ) + 1;
      else
         fp->len_len = -fp->len_len;
   }

   for (wp = g_status.window_list; wp != NULL; wp = wp->next) {
      wp->start_col += wp->file_info->len_len;
      wp->ccol      += wp->file_info->len_len;
      if (wp->ccol > wp->end_col)
         wp->ccol = wp->end_col;
      if (wp->visible) {
         show_ruler( wp );
         display_current_window( wp );
      }
   }

   return( OK );
}


/*
 * Name:    toggle_cwd
 * Purpose: turn the current working directory display on or off
 * Author:  Jason Hood
 * Date:    February 26, 2003
 * Passed:  arg_filler:  argument to satisfy function prototype
 * Notes:   use the top line of the display to show the cwd.
 *          will fail if any top window is only one line (let the user decide
 *           how to rearrange windows).
 */
int  toggle_cwd( TDE_WIN *arg_filler )
{
register TDE_WIN *wp;

   if (!mode.display_cwd) {
      wp = g_status.window_list;
      while (wp != NULL) {
         if (wp->visible  &&  wp->top == 0  &&
             wp->top_line == wp->bottom_line  &&  !wp->ruler)
            break;
         wp = wp->next;
      }
      if (wp != NULL) {
         error( WARNING, g_display.end_line, ed26 );
         return( OK );
      }
      show_cwd( );
   }
   mode.display_cwd = !mode.display_cwd;
   wp = g_status.window_list;
   while (wp != NULL) {
      if (wp->visible) {
         if (mode.display_cwd) {
            if (wp->top == 0) {
               wp->top = 1;
               if (wp->ruler  &&  wp->top_line == wp->bottom_line)
                  wp->ruler = FALSE;
               else if (wp->cline == wp->top_line)
                  ++wp->cline;
            }
         } else {
            if (wp->top == 1) {
               wp->top = 0;
               if (mode.ruler  &&  wp->top_line == wp->bottom_line)
                  wp->ruler = TRUE;
               else if (wp->rline == wp->cline - wp->top_line)
                  --wp->cline;
            }
         }
         setup_window( wp );
         redraw_current_window( wp );
      }
      wp = wp->next;
   }
   return( OK );
}


/*
 * Name:    toggle_quickedit
 * Purpose: toggle the console's QuickEdit mode
 * Author:  Jason Hood
 * Date:    February 19, 2006
 * Passed:  arg_filler:  argument to satisfy function prototype
 * Notes:   QuickEdit controls how the mouse will be used in Win32.
 */
int  toggle_quickedit( TDE_WIN *arg_filler )
{
#if defined( __WIN32__ )
DWORD mode;

   GetConsoleMode( conin, &mode );
   mode |= ENABLE_EXTENDED_FLAGS;
   mode ^= ENABLE_QUICK_EDIT;
   SetConsoleMode( conin, mode );
#endif

   return( OK );
}


/*
 * Name:    cursor_sync
 * Purpose: carry out cursor movements in all visible windows
 * Date:    January 15, 1992
 * Passed:  window
 * Notes:   switch sync semaphore when we do this so we don't get into a
 *          recursive loop.  all cursor movement commands un_copy_line before
 *          moving the cursor off the current line.   you MUST make certain
 *          that the current line is uncopied in the task routines that
 *          move the cursor off the current line before calling sync.
 */
void cursor_sync( TDE_WIN *window )
{
register TDE_WIN *wp;
register file_infos *fp;

   if (mode.sync && mode.sync_sem) {

   /*
    * these functions must un_copy a line before sync'ing
    */
#if defined( __MSC__ )
      switch (g_status.command) {
         case  NextLine        :
         case  BegNextLine     :
         case  LineDown        :
         case  LineUp          :
         case  WordRight       :
         case  WordLeft        :
         case  ScreenDown      :
         case  ScreenUp        :
         case  EndOfFile       :
         case  TopOfFile       :
         case  BotOfScreen     :
         case  TopOfScreen     :
         case  JumpToPosition  :
         case  CenterWindow    :
         case  CenterLine      :
         case  ScrollDnLine    :
         case  ScrollUpLine    :
         case  PanUp           :
         case  PanDn           :
         case  NextDirtyLine   :
         case  PrevDirtyLine   :
         case  ParenBalance    :
            assert( g_status.copied == FALSE );
            break;
         default  :
            break;
      }
#endif

      mode.sync_sem = FALSE;
      for (wp = g_status.window_list;  wp != NULL;  wp = wp->next) {
         if (wp->visible  &&  wp != window) {
            (*do_it[g_status.command])( wp );
            show_line_col( wp );
            show_ruler_pointer( wp );
         }
      }
      mode.sync_sem = TRUE;
      for (fp = g_status.file_list; fp != NULL; fp = fp->next)
         if (fp->dirty != FALSE)
            fp->dirty = GLOBAL;
   }
}


/*
 * Name:    editor
 * Purpose: Set up the editor structures and display changes as needed.
 * Date:    June 5, 1991
 * Modified: December 29, 1996, jmh - moved the line option to edit_next_file
 * Notes:   Master editor routine.
 *
 * jmh 981127: move -b to edit_next_file.
 * jmh 990404: moved initial set_cursor_size to main.c.
 * jmh 990414: no longer cls (DOS);
 *             moved reset cursor size to video_config (UNIX).
 * jmh 990429: add -v to load everything read-only (viewer mode).
 * jmh 010528: set g_status.errmsg to explain why the editor didn't start.
 * jmh 010605: moved -v to main().
 * jmh 021023: modify sas_defined to indicate a command line grep.
 * jmh 021024: moved the grep options into edit_next_file(), to allow for
 *              options before them (in particular, -e);
 *             removed the directory prompt, since patterns are handled anyway
 *              and file_exists() can test for a directory; pressing enter will
 *              bring up the directory list immediately.
 */
void editor( void )
{
char *name;                     /* name of file to start editing */
register TDE_WIN *window;       /* current active window */
int  c;

   /*
    * initialize search and seize
    */
   g_status.sas_defined = FALSE;
   for (c = 0; c < SAS_P; c++)
      g_status.sas_arg_pointers[c] = NULL;

   c = edit_next_file( g_status.current_window );
   /*
    * Check that user specified file to edit, if not offer help
    */
   if (c >= 1) {
      /*
       * Are we reading from a pipe?
       */
      if (g_status.input_redir)
         c = attempt_edit_display( "", CMDLINE | GLOBAL | g_option.read_only );

      else if (c == 2  ||  (c = load_workspace( )) == FALSE) {

         show_help( );

         name = g_status.rw_name;
         *name = '\0';
         /*
          * file name, path or pattern
          */
         c = get_name( ed15b, g_display.end_line, name, &h_file );

         assert( c < PATH_MAX );

         if (c != ERROR) {
            if (c == 0  ||  is_pattern( name )  ||
                file_exists( name ) == SUBDIRECTORY)
               c = dir_help_name( (TDE_WIN *)NULL, name );
            else {
               c = attempt_edit_display( name, CMDLINE | GLOBAL |
                                               g_option.read_only );
               if (g_status.stop)
                  c = ERROR;
            }
         }
      }
   }

   g_status.stop =   c == ERROR  ?  TRUE  :  FALSE;

#if !defined( __DOS16__ )
   setjmp( editor_jmp );
#endif

   /*
    * main loop - keep updating the display and processing any commands
    *  while user has not pressed the stop key
    */
   for (; g_status.stop != TRUE;) {
      window = g_status.current_window;
      display_dirty_windows( window );

      /*
       * set the critical error handler flag to a known state before we
       *   do each editor command.
       */
      CEH_OK;

      /*
       * Get a key from the user.  Look up the function assigned to that key.
       * All regular text keys are assigned to function 0.  Text characters
       * are less than 0x100, decimal 256, which includes the ASCII and
       * extended ASCII character set.
       */
      g_status.viewer_key = TRUE;
#if defined( __WIN32__ )
      handle_mouse = TRUE;
      g_status.key_pressed = getkey( );
      handle_mouse = FALSE;
#else
      g_status.key_pressed = getkey( );
#endif
      g_status.command = getfunc( g_status.key_pressed );
      if (g_status.wrapped) {
         g_status.wrapped = FALSE;
         show_search_message( CLR_SEARCH );
      }
      if (found_rline) {
         found_rline = 0;
         show_curl_line( g_status.current_window );
      }
      g_status.control_break = FALSE;
      record_key( g_status.key_pressed, g_status.command );
      execute( window );
   }
}


/*
 * Name:    repeat
 * Purpose: Repeat a key a specified number of times
 * Author:  Jason Hood
 * Date:    July 26, 1998
 * Notes:   If the repeat count is negative, treat it as a line number and
 *           subtract the current line from it to get the count.
 *          If the repeat count is zero, treat it as an infinite count.
 *          Needs to be re-entrant (could call a macro which could call it).
 *
 * 990214:  recognize the break-point for the infinite count (why wasn't it
 *           already done??)
 * 030403:  prevent repeating the currently recording macro key.
 * 060828:  return OK if parameters are valid;
 *          make recursive macros temporarily non-recursive;
 *          handle repeating a macro from a macro.
 */
int  repeat( TDE_WIN *window )
{
long count;
long key;
int  func;
MACRO *mac;
MACRO_STACK *stack;
int  rc;
int  prompt_line;
int  recursive;
long *keys = NULL;              /* remove GCC warning */

   prompt_line = window->bottom_line;
   count = -1;
   /*
    * Repeat count:
    */
   if (get_number( ed19, prompt_line, &count, NULL ) != OK)
      return( ERROR );

   if (count < 0)
      count = labs( count + window->rline );
   else if (count == 0)
      count = -1;

   /*
    * Press the key to be repeated.
    */
   g_status.viewer_key = TRUE;
   key  = prompt_key( ed20, prompt_line );
   func = getfunc( key );
   mac  = g_status.key_macro;

   if (func < 0 || func >= NUM_FUNCS)
      return( ERROR );

   /*
    * if repeating a macro from within a macro, pretend we're not in a macro,
    * otherwise the playback code will continue the macro before returning here
    */
   if (g_status.macro_executing && (func == PlayBack || func == PseudoMacro)) {
      if (push_macro_stack( ) != OK) {
         error( WARNING, prompt_line, ed16 );
         return( ERROR );
      }
      stack = g_status.mstack;
      g_status.mstack = NULL;
      g_status.macro_executing = FALSE;
   } else
      stack = NULL;

   /*
    * if repeating a recursive macro, remove the last key from the macro
    * to make it non-recursive, allowing the count to be used
    */
   if (func == PlayBack && mac->len > 1 && mac->key.keys[mac->len-1] == key) {
      --mac->len;
      if (mac->len == 1) {
         keys = mac->key.keys;
         mac->key.key = *keys;
      }
      recursive = TRUE;
   } else
      recursive = FALSE;

   for (rc = OK; count != 0 && rc == OK; --count) {
      window = g_status.current_window;
      display_dirty_windows( window );

      CEH_OK;

      if (g_status.wrapped) {
         g_status.wrapped = FALSE;
         show_search_message( CLR_SEARCH );
      }
      if (found_rline) {
         found_rline = 0;
         show_curl_line( window );
      }

      if (count < 0 && window->file_info->break_point != 0 &&
          window->file_info->break_point == window->rline)
         rc = ERROR;
      else {
         g_status.key_pressed = key;
         g_status.command     = func;
         g_status.key_macro   = mac;
         rc = execute( window );
      }

      if (g_status.control_break == TRUE || g_status.stop == TRUE)
         rc = ERROR;
   }

   if (recursive) {
      if (mac->len == 1)
         mac->key.keys = keys;
      ++mac->len;
   }

   if (stack != NULL) {
      g_status.macro_executing = TRUE;
      g_status.mstack = stack;
      pop_macro_stack( );
      --g_status.macro_next;    /* push saves the next key */
   }

   return( OK );
}


/*
 * Name:    display_dirty_windows
 * Purpose: Set up the editor structures and display changes as needed.
 * Date:    June 5, 1991
 * Notes:   Display all windows with dirty files.
 *
 * jmh 980728: Do the current window first.
 */
void display_dirty_windows( TDE_WIN *window )
{
register TDE_WIN *below;        /* window below current */
register TDE_WIN *above;        /* window above current */
file_infos *file;               /* temporary file structure */

   file = window->file_info;
   if (file->dirty & LOCAL) {
      display_current_window( window );
      if (file->dirty & RULER)
         show_ruler( window );
   }

   /*
    * update all windows that point to any file that has been changed
    */
   above = below = window;
   while (above->prev || below->next) {
      if (above->prev) {
         above = above->prev;
         show_dirty_window( above );
      }
      if (below->next) {
         below = below->next;
         show_dirty_window( below );
      }
   }
   for (file=g_status.file_list; file != NULL; file=file->next)
      file->dirty = FALSE;

   /*
    * Set the cursor position at window->ccol, window->cline.  Show the
    * user where in the file the cursor is positioned.
    */
   show_line_col( window );
   show_ruler_pointer( window );
   xygoto( window->ccol, window->cline );
   refresh( );
}



/*
 * Name:    show_dirty_window
 * Purpose: show changes in non-current window
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 */
void show_dirty_window( TDE_WIN *window )
{
register TDE_WIN *win;   /* register window pointer */
int  dirty;

  win = window;
  if (win->visible) {
     dirty = win->file_info->dirty;
     if (dirty & NOT_LOCAL) {
        display_current_window( win );
        show_size( win );
     }
     show_asterisk( win );
  }
}


/*
 * Name:    paste
 * Purpose: copy text from the Windows clipboard into TDE
 * Author:  Jason Hood
 * Date:    August 26, 2002
 * Passed:  window:  pointer to current window
 * Notes:   not available in 16-bit DOS or UNIX.
 *          the clipboard is CRLF and NUL-terminated.
 *          turn off indent mode and (dynamic) word wrap, as the clipboard
 *           text will have its own indentation and line length.
 * jmh 050815: recognise embedded CRs and LF lines.
 */
int  paste( TDE_WIN *window )
{
int  rc = OK;
#if defined( __DJGPP__ ) || defined( __WIN32__ )
char *text;
char *p, *q;
int  indent;
int  wordwrap;
line_list_ptr node;
line_list_ptr shl = NULL;
int  len, size;
long lines = 0;
int  newline = FALSE;

   text = get_clipboard( );
   if (text == NULL)
      rc = ERROR;
   else {
      indent = mode.indent;
      wordwrap = mode.word_wrap;
      mode.indent = FALSE;
      mode.word_wrap = NO_WRAP;
      window->visible = FALSE;
      size = strlen( text );
      q = memchr( text, '\n', size );
      if (q != NULL) {
         if (q != text  &&  q[-1] == '\r')
            q[-1] = '\0';
         else
            *q = '\0';
         p = q + 1;
         g_status.command = SplitLine;
         if (insert_newline( window ) == ERROR)
            return( ERROR );
         newline = TRUE;
      } else
         p = text + size;
      size -= p - text;
      rc = add_chars( (text_ptr)text, window );
      if (rc == OK)
         rc = un_copy_line( window->ll, window, TRUE, TRUE );
      while (rc == OK  &&  (q = memchr( p, '\n', size )) != NULL) {
         len = q - p;
         if (q != p  &&  q[-1] == '\r')
            --len;
         if (len >= MAX_LINE_LENGTH) {
            q = p + (len = MAX_LINE_LENGTH - 1) - 1;
            while (q > p  &&  *q != ' ') {
               --len;
               --q;
            }
            if (q == p)
               q = p + (len = MAX_LINE_LENGTH - 1) - 1;
         }
         node = new_line_text( (text_ptr)p, len, DIRTY, &rc );
         if (rc == OK) {
            insert_node( window->file_info, window->ll, node );
            inc_line( window, FALSE );
            if (window->cline != window->bottom_line)
               ++window->cline;
            if (shl == NULL)
               shl = node;
            ++lines;
         }
         size -= q + 1 - p;
         p = q + 1;
      }
      if (lines) {
         syntax_check_block( 1, lines, shl, window->file_info->syntax );
         adjust_windows_cursor( window, lines );
         restore_marked_block( window, lines );
      }
      if (newline)
         beg_next_line( window );
      if (rc == OK  &&  *p)
         rc = add_chars( (text_ptr)p, window );

      window->visible = TRUE;
      redraw_current_window( window );
      mode.word_wrap = wordwrap;
      mode.indent = indent;
      free( text );
   }
#endif
   return( rc );
}


/*
 * Name:    context_help
 * Purpose: load a help file and search for a word
 * Author:  Jason Hood
 * Date:    July 10, 2005
 * Passed:  window:  pointer to current window
 * Notes:   if the cursor is on a word, search for that word
 */
int  context_help( TDE_WIN *window )
{
int  rc;
file_infos *fp;
char word[MAX_COLS];
int  len;
char *p, *q, *w;

   if (un_copy_line( window->ll, window, TRUE, TRUE ) == ERROR)
      return( ERROR );

   /*
    * see if it's already loaded
    */
   rc = ERROR;
   for (fp = g_status.file_list; fp != NULL; fp = fp->next) {
      if (strcmp( fp->file_name, mode.helpfile ) == 0) {
         change_window( window, find_file_window( fp ) );
         rc = OK;
         break;
      }
   }
   if (rc == ERROR) {
      if (file_exists( mode.helpfile ) == ERROR) {
         combine_strings( line_out, ed27a, mode.helpfile, ed27b );
         /*
          * context help file not available
          */
         error( WARNING, window->bottom_line, line_out );
      } else
         rc = attempt_edit_display( mode.helpfile, LOCAL );
      if (rc == OK) {
         g_status.current_file->read_only = TRUE;
         g_status.current_file->scratch = -1;
         show_asterisk( g_status.current_window );
      }
   }

   if (rc == OK) {
      *word = '\0';
      if ((len = copy_word( window, word, sizeof(word), 0 )) != 0) {
         p = (char *)regx.pattern;
         q = mode.helptopic;
         while (*q) {
            if (*q == '~') {
               w = word;
               do {
                  if (strchr( "\\.,<>^$[]*+?|()", *w ))
                     *p++ = '\\';
                  *p++ = *w++;
               } while (--len != 0);
            } else {
               if (*q == '\\' && q[1] == '~')
                  ++q;
               *p++ = *q;
            }
            ++q;
         }
         *p = '\0';

         assert( p - (char *)regx.pattern <= sizeof(regx.pattern) );

         regx.search_defined = build_nfa( );
         if (regx.search_defined != ERROR) {
            g_status.search = SEARCH_REGX;
            perform_search( g_status.current_window );
         }
      }
   }
   return( rc );
}
