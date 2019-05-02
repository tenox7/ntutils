/*******************  start of original comments  ********************/
/*
 * Written by Douglas Thomson (1989/1990)
 *
 * This source code is released into the public domain.
 */

/*
 * Name:    dte - Doug's Text Editor program - block commands module
 * Purpose: This file contains all the commands than manipulate blocks.
 * File:    block.c
 * Author:  Douglas Thomson
 * System:  this file is intended to be system-independent
 * Date:    October 1, 1989
 */
/*********************  end of original comments   ********************/

/*
 * In the DTE editor, Doug only supported functions for STREAM blocks.
 *
 * The block routines have been EXTENSIVELY rewritten.  This editor uses LINE,
 * STREAM, and BOX blocks.  That is, one may mark entire lines, streams of
 * characters, or column blocks.  Block operations are done in place.  There
 * are no paste and cut buffers.  In limited memory situations, larger block
 * operations can be carried out.  Block operations can be done within or
 * across files.
 *
 * In TDE, version 1.1, I separated the BOX and LINE actions.
 *
 * In TDE, version 1.3, I put STREAM blocks back in.  Added block upper case,
 *  block lower case, and block strip high bit.
 *
 * In TDE, version 1.4, I added a block number function.  Here at our lab,
 *  I often need to number samples, lines, etc..., comes in fairly useful.
 *
 * In TDE, version 2.0, I added a box block sort function.
 *
 * In TDE, version 2.0e, I added BlockRot13, BlockFixUUE, and BlockEmailReply.
 *
 * In TDE, version 2.2, the big text buffer was changed to a double linked list.
 *  all characters in the line of each node must be accurately counted.
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
 *  public domain, Frank Davis.  You may distribute it freely.
 */

#include "tdestr.h"
#include "common.h"
#include "tdefunc.h"
#include "define.h"


static void do_multi_box_mark( TDE_WIN * );
static void do_multi_line_mark( TDE_WIN * );
static void do_multi_stream_mark( TDE_WIN * );
static void detab_begin( line_list_ptr, int, int, int );
static int  find_comment( text_ptr, text_ptr, int );
static int  find_comment_close( text_ptr, text_ptr, int );
static void border_block_buff( text_ptr, int, long,long,long, text_ptr, int * );

static int  tabbed_bc = -1, tabbed_ec;
static int  tabbed_bc_ofs, tabbed_ec_ofs;

long swap_br, swap_er = -1;
int  swap_bc, swap_ec;

static int find_swap_lines( TDE_WIN *, line_list_ptr *, line_list_ptr *,
                            long, long, int );
static int find_swap_box( TDE_WIN *, long, long, int, int, int );


/*
 * Name:     mark_block
 * Class:    primary editor function
 * Purpose:  To record the position of the start of the block in the file.
 * Date:     June 5, 1991
 * Modified: August 9, 1997, Jason Hood - removed redundant logic, added markers
 * Passed:   window:  pointer to current window
 * Notes:    Assume the user will mark begin and end of a block in either
 *            line, stream, or box mode.  If the user mixes types, then block
 *            type defaults to current block type.
 *
 * jmh 980728: Allow explicit setting of the block beginning/end with the
 *              MarkBegin and MarkEnd functions. If no block is set, default
 *              to box.
 *             Removed the markers - now inherent in goto_marker.
 *
 * jmh 990404: Mark a paragraph when the beginning/ending marks and the cursor
 *              are all the same position.
 * jmh 990507: Modified MarkBox and MarkLine in the following manner:
 *               MarkBox:     mark character;
 *               & again:     mark word;
 *               & again:     mark string;
 *               & again:     mark paragraph.
 *
 *               MarkLine:    mark line;
 *               & again:     mark group of indented or blank lines;
 *               & again:     mark an additional line above and below;
 *               & again:     mark paragraph.
 *
 *            Moved all that code into three separate functions.
 *
 * jmh 050714: marking a LINE after a STREAM will cause the first or last line
 *               to be completely marked, keeping a STREAM block.
 */
int  mark_block( TDE_WIN *window )
{
int  type;
register file_infos *file;      /* temporary file variable */
register TDE_WIN *win;          /* put window pointer in a register */
int  rc;

   win  = window;
   file = win->file_info;
   if (win->rline > file->length || win->ll->len == EOF)
      return( ERROR );

   switch (g_status.command) {
      case MarkBox:    type = BOX;    break;
      case MarkLine:   type = LINE;   break;
      case MarkStream: type = STREAM; break;
      case MarkBegin:
      case MarkEnd:    type = (g_status.marked) ? file->block_type : BOX;
                       break;
      default:
         return( ERROR );
   }

   if (g_status.marked == FALSE) {
      g_status.marked = TRUE;
      g_status.marked_file = file;
   }

   rc = OK;
   /*
    * define blocks for only one file.  it is ok to modify blocks in any window
    * pointing to original marked file.
    */
   if (file == g_status.marked_file) {
      /*
       * mark beginning and ending column regardless of block mode.
       */
      if (file->block_type <= NOTMARKED) {
         file->block_bc = file->block_ec = win->rcol;
         file->block_br = file->block_er = win->rline;

      } else if (g_status.command == MarkBegin) {
         file->block_bc = win->rcol;
         file->block_br = win->rline;

      } else if (g_status.command == MarkEnd) {
         if (type != STREAM || file->block_ec != -1)
            file->block_ec = win->rcol;
         file->block_er = win->rline;

      } else if (g_status.command_count == 0) {
         if (file->block_br <= win->rline) {
            if (type == LINE && file->block_type == STREAM) {
               if (file->block_br == win->rline &&
                   (win->rcol < file->block_bc || file->block_ec == -1))
                  file->block_ec = win->rcol;
               else {
                  type = STREAM;
                  file->block_ec = -1;
                  --g_status.command_count;
               }
            } else
               file->block_ec = win->rcol;
            file->block_er = win->rline;
         } else {
            file->block_br = win->rline;
            if (file->block_bc < win->rcol && type != STREAM)
               file->block_ec = win->rcol;
            else {
               if (type == LINE && file->block_type == STREAM) {
                  if (file->block_ec != -1) {
                     file->block_bc = 0;
                     type = STREAM;
                  } else {
                     file->block_bc =
                     file->block_ec = win->rcol;
                  }
               } else
                  file->block_bc = win->rcol;
            }
         }

      } else {
         un_copy_line( win->ll, win, TRUE, TRUE );
         if (is_line_blank( win->ll->line, win->ll->len, file->inflate_tabs )
             &&  type != LINE)
            return( OK );

         if (type == BOX)
            do_multi_box_mark( win );
         else if (type == LINE)
            do_multi_line_mark( win );
         else /* type == STREAM */
            do_multi_stream_mark( win );
      }
      /*
       * if user marks ending line less than beginning line then switch
       */
      if (file->block_er < file->block_br) {
         long lnum = file->block_er;
         file->block_er = file->block_br;
         file->block_br = lnum;
      }
      /*
       * if user marks ending column less than beginning column then switch
       */
      if (file->block_ec < file->block_bc  &&  file->block_ec != -1  &&
          (type != STREAM  ||  file->block_br == file->block_er)) {
         int num = file->block_ec;
         file->block_ec = file->block_bc;
         file->block_bc = num;
      }
      file->block_type = type;
      file->dirty = GLOBAL;
   } else {
      /*
       * block already defined
       */
      error( WARNING, win->bottom_line, block1 );
      rc = ERROR;
   }
   return( rc );
}


/*
 * Name:    do_multi_box_mark
 * Purpose: to mark a word, string or paragraph
 * Author:  Jason Hood
 * Date:    May 7, 1999
 * Passed:  window:  pointer to current window
 *
 * jmh 991014: if on a non-letter when marking a word, mark a string.
 */
static void do_multi_box_mark( TDE_WIN *window )
{
int  num;
long lnum;
register file_infos *file;      /* temporary file variable */
line_list_ptr ll;
text_ptr p;
int  (*space)( int );           /* Function to determine what a word is */
int  pb, pe;                    /* paragraph beginning/end columns */
int  tabs;
int  tab_size;

   if (g_status.command_count > 3)
      return;

   file = window->file_info;
   tabs = file->inflate_tabs;
   tab_size = file->ptab_size;
   ll = window->ll;
   if (g_status.command_count == 3) {
      lnum = window->rline;
      pb   = MAX_LINE_LENGTH;
      pe   = 0;
      do {
         num = first_non_blank( ll->line, ll->len, tabs, tab_size );
         if (num < pb)
            pb = num;
         num = find_end( ll->line, ll->len, tabs, tab_size ) - 1;
         if (num > pe)
            pe = num;
         lnum--;
         ll = ll->prev;
      } while (!is_line_blank( ll->line, ll->len, tabs ));
      file->block_br = lnum + 1;

      lnum = window->rline + 1;
      ll   = window->ll->next;
      while (!is_line_blank( ll->line, ll->len, tabs )) {
         num = first_non_blank( ll->line, ll->len, tabs, tab_size );
         if (num < pb)
            pb = num;
         num = find_end( ll->line, ll->len, tabs, tab_size ) - 1;
         if (num > pe)
            pe = num;
         lnum++;
         ll = ll->next;
      }
      file->block_er = lnum - 1;

   } else {
      copy_line( ll, window, TRUE );
      /*
       * Prevent uncopy calling the file modified.
       */
      g_status.copied = FALSE;
      if (window->rcol >= g_status.line_buff_len)
         return;

      if (g_status.command_count == 1) {
         file->block_br = file->block_er = window->rline;
         pb = window->rcol - 1;
         pe = pb + 1;
         space = (myiswhitespc( g_status.line_buff[pe] )) ? bj_isspc :
                                                            myiswhitespc;
      } else {
         pb = file->block_bc - 1;
         pe = file->block_ec + 1;
         space = bj_isspc;
      }
      p = g_status.line_buff + pb;
      for (; pb >= 0 && !space( *p-- ); --pb) ;
      ++pb;
      p = g_status.line_buff + pe;
      for (; pe < g_status.line_buff_len && !space( *p++ ); ++pe) ;
      --pe;
   }
   file->block_bc = pb;
   file->block_ec = pe;
}


/*
 * Name:    do_multi_line_mark
 * Purpose: to mark like indentation or paragraph
 * Author:  Jason Hood
 * Date:    May 7, 1999
 * Passed:  window:  pointer to current window
 *
 * jmh 991012: include "nested" indents.
 * jmh 050709: mark a group of blank lines.
 */
static void do_multi_line_mark( TDE_WIN *window )
{
long lnum;
register file_infos *file;      /* temporary file variable */
line_list_ptr ll;
int  i;                         /* indentation level */
int  indent;                    /* test for indentation? */
int  tabs;
int  tab_size;

   if (g_status.command_count > 3)
      return;

   file = window->file_info;
   tabs = file->inflate_tabs;
   tab_size = file->ptab_size;

   if (g_status.command_count == 2) {
      if (file->block_br > 1)
         --file->block_br;
      if (file->block_er < file->length)
         ++file->block_er;

   } else {
      indent = (g_status.command_count == 1);
      lnum = window->rline;
      ll = window->ll;
      if (is_line_blank( ll->line, ll->len, tabs)) {
         if (!indent)
            return;
         do {
            lnum--;
            ll = ll->prev;
         } while (ll->len != EOF  &&  is_line_blank( ll->line, ll->len, tabs ));
         file->block_br = lnum + 1;

         lnum = window->rline + 1;
         ll   = window->ll->next;
         while (ll->len != EOF  &&  is_line_blank( ll->line, ll->len, tabs )) {
            lnum++;
            ll = ll->next;
         }
         file->block_er = lnum - 1;

      } else {
         i = first_non_blank( ll->line, ll->len, tabs, tab_size );
         do {
            lnum--;
            ll = ll->prev;
         } while (!is_line_blank( ll->line, ll->len, tabs ) &&
           (!indent || i <= first_non_blank( ll->line,ll->len,tabs,tab_size )));
         file->block_br = lnum + 1;

         lnum = window->rline + 1;
         ll   = window->ll->next;
         while (!is_line_blank( ll->line, ll->len, tabs ) &&
         (!indent || i <= first_non_blank( ll->line,ll->len, tabs,tab_size ))) {
            lnum++;
            ll = ll->next;
         }
         file->block_er = lnum - 1;
      }
   }
}


/*
 * Name:    do_multi_stream_mark
 * Purpose: to mark a paragraph
 * Author:  Jason Hood
 * Date:    May 7, 1999
 * Passed:  window:  pointer to current window
 *
 * jmh 020913: second will mark a line (without leading space); third will
 *              now mark a paragraph.
 */
static void do_multi_stream_mark( TDE_WIN *window )
{
long lnum;
register file_infos *file;      /* temporary file variable */
line_list_ptr ll;
int  tabs;
int  tab_size;

   if (g_status.command_count > 2)
      return;

   file = window->file_info;
   tabs = file->inflate_tabs;
   tab_size = file->ptab_size;

   lnum = window->rline;
   ll = window->ll;
   if (g_status.command_count == 1)
      file->block_br = file->block_er = lnum;
   else {
      do {
         lnum--;
         ll = ll->prev;
      } while (!is_line_blank( ll->line, ll->len, tabs ));
      ll = ll->next;
      file->block_br = lnum + 1;
   }
   file->block_bc = first_non_blank( ll->line, ll->len, tabs, tab_size );

   if (g_status.command_count == 2) {
      ll   = window->ll->next;
      lnum = window->rline + 1;
      while (!is_line_blank( ll->line, ll->len, tabs )) {
         lnum++;
         ll = ll->next;
      }
      ll = ll->prev;
      file->block_er = lnum - 1;
   }
   file->block_ec = find_end( ll->line, ll->len, tabs, tab_size ) - 1;
}


/*
 * Name:    move_mark
 * Purpose: move the block mark to the cursor
 * Author:  Jason Hood
 * Date:    July 10, 2005
 * Passed:  window:  pointer to current window
 * Notes:   BOX and LINE blocks are made relative the cursor position;
 *           STREAM block begins at the cursor, but ends the same.
 */
int  move_mark( TDE_WIN *window )
{
register file_infos *file;      /* temporary file variable */
register TDE_WIN *win;          /* put window pointer in a register */
int  bc, ec;
long br, er;

   if (!g_status.marked  ||  window->ll->len == EOF)
      return( ERROR );

   win  = window;
   file = win->file_info;
   bc   = g_status.marked_file->block_bc;
   br   = g_status.marked_file->block_br;
   ec   = g_status.marked_file->block_ec;
   er   = g_status.marked_file->block_er;
   file->block_type = g_status.marked_file->block_type;
   file->block_bc = win->rcol;
   file->block_br = win->rline;
   file->block_er = win->rline + er - br;
   if (file->block_er > file->length)
      file->block_er = file->length;
   if (file->block_type == STREAM && (er != br || ec == -1)) {
      if (win->rline + er - br > file->length)
         file->block_ec = -1;
      else
         file->block_ec = ec;
   } else
      file->block_ec = win->rcol + ec - bc;

   file->dirty = GLOBAL;
   if (file != g_status.marked_file) {
      g_status.marked_file->dirty = GLOBAL;
      g_status.marked_file->block_type = -g_status.marked_file->block_type;
      g_status.marked_file = file;
   }

   return( OK );
}


/*
 * Name:     unmark_block
 * Class:    primary editor function
 * Purpose:  To set all block information to NULL or 0
 * Date:     June 5, 1991
 * Modified: August 9, 1997, Jason Hood - remove block beginning/end markers
 * Passed:   window:  pointer to current window
 * Notes:    Reset all block variables if marked, otherwise return.
 *            If a marked block is unmarked then redraw the screen(s).
 * jmh:      October 2, 1997 - I'm sometimes too hasty in unmarking blocks, so
 *            I'd like to be able to re-mark it. Don't reset the markers, and
 *            remember the globals (in static variables, 'cos I don't want to
 *            add even more global status variables). Don't re-mark in a macro
 *            though, because it might require a block to be set (this is how
 *            I did DeleteWordBack before I wrote the function).
 *
 * jmh:      November 26 - made old_file global so it won't re-mark a closed
 *            file. (I'll add it to g_status when I feel like recompiling
 *            everything.) (January 24, 1998 - done.)
 *
 * jmh 980728: re-mark the block in the current file, using a negative value
 *              when unmarking to remember the type. This removes the need
 *              for old_file and allows "multiple" blocks.
 *             Remove markers.
 */
int  unmark_block( TDE_WIN *window )
{
register file_infos *marked_file = NULL;

   if (g_status.marked == TRUE) {
      marked_file          = g_status.marked_file;
      g_status.marked_file = NULL;

   } else if (!mode.record && !g_status.macro_executing) {
      marked_file          = window->file_info;
      if (marked_file->block_type < NOTMARKED)
         g_status.marked_file = marked_file;
      else
         marked_file = NULL;
   }
   if (marked_file != NULL) {
      g_status.marked         = !g_status.marked;
      marked_file->dirty      = GLOBAL;
      marked_file->block_type = -marked_file->block_type;
   }
   return( OK );
}


/*
 * Name:    restore_marked_block
 * Class:   helper function
 * Purpose: To restore block beginning and ending row after an editing function
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 *          net_change: number of lines added or subtracted
 * Notes:   If a change has been made before the marked block then the
 *           beginning and ending row need to be adjusted by the number of
 *           lines added or subtracted from file.
 * jmh:     November 26, 1997 - adjust the beginning/end markers (how did I
 *           miss this the first time?).
 * jmh 980728: Remove markers.
 * jmh 980821: update unmarked ("hidden") blocks, too.
 * jmh 990421: corrected marked beyond end of file bug.
 */
void restore_marked_block( TDE_WIN *window, int net_change )
{
long length;
register file_infos *marked_file;
int  update;
int  updated;
int  type;
long rline;

   if (net_change != 0) {
      marked_file = window->file_info;
      type = abs( marked_file->block_type );
      rline = window->rline;
      if (type != NOTMARKED  &&  rline <= marked_file->block_er) {
         update = (g_status.marked && g_status.marked_file == marked_file);
         updated = FALSE;

         /*
          * if cursor is before marked block then adjust block by net change.
          */
         if (rline < marked_file->block_br) {
            marked_file->block_br += net_change;
            marked_file->block_er += net_change;
            updated = TRUE;

         /*
          * if cursor is somewhere in marked block don't restore, do redisplay
          */
         } else if (rline > marked_file->block_br  &&
                    rline < marked_file->block_er) {
            marked_file->block_er += net_change;
            updated = TRUE;

         /*
          * if the cursor is on the beginning and/or end of the block,
          * does the block change?
          */
         } else {
            if (rline == marked_file->block_br  &&
                rline == marked_file->block_er) {
               if (net_change < 0) {
                  if (g_status.command == DeleteLine) {
                     if (update)
                        unmark_block( window );
                  } else {
                     marked_file->block_br += net_change;
                     marked_file->block_er += net_change;
                     marked_file->block_bc += window->rcol;
                     marked_file->block_ec += window->rcol;
                     updated = TRUE;
                  }
               } else if (g_status.command != AddLine  &&
                          g_status.command != DuplicateLine) {
                  if (type == LINE) {
                     marked_file->block_er += net_change;
                     updated = TRUE;
                  } else {
                     if (window->rcol <= marked_file->block_bc) {
                        marked_file->block_br += net_change;
                        marked_file->block_er += net_change;
                        marked_file->block_bc -= window->rcol;
                        marked_file->block_ec -= window->rcol;
                        updated = TRUE;
                     } else if (window->rcol <= marked_file->block_ec) {
                        marked_file->block_ec = window->rcol;
                        updated = TRUE;
                     }
                  }
               }

            } else if (rline == marked_file->block_br) {
               marked_file->block_er += net_change;
               updated = TRUE;
               if (net_change > 0) {
                  if (type != LINE && window->rcol <= marked_file->block_bc) {
                     marked_file->block_br += net_change;
                     if (type == STREAM)
                        marked_file->block_bc -= window->rcol;
                  }
               } else if (g_status.command == DeleteLine) {
                  if (type == STREAM) {
                     marked_file->block_bc = 0;
                     if (marked_file->block_ec == -1  &&
                         marked_file->block_br == marked_file->block_er) {
                        marked_file->block_ec = 0;
                        marked_file->block_type = (marked_file->block_type < 0)
                                                  ? -LINE : LINE;
                     }
                  }
               } else if (type != BOX) {
                  marked_file->block_br += net_change;
                  marked_file->block_bc += window->rcol;
               }

            } else /* rline == marked_file->block_er */ {
               if (net_change < 0) {
                  marked_file->block_er += net_change;
                  updated = TRUE;
                  if (type == STREAM) {
                     if (g_status.command == DeleteLine)
                        marked_file->block_ec = -1;
                     else if (marked_file->block_ec != -1)
                        marked_file->block_ec += window->rcol;
                  }
               } else if (g_status.command != AddLine  &&
                          g_status.command != DuplicateLine) {
                  if (type == LINE) {
                     marked_file->block_er += net_change;
                     updated = TRUE;
                  } else if (window->rcol <= marked_file->block_ec) {
                     updated = TRUE;
                     if (type == STREAM) {
                       marked_file->block_er += net_change;
                       marked_file->block_ec -= window->rcol;
                     } else if (window->rcol <= marked_file->block_bc)
                        marked_file->block_er += net_change;
                  }
               }
            }
         }

         /*
          * check for lines of marked block beyond end of file
          */
         length = marked_file->length;
         if (marked_file->block_br > length) {
            if (update)
               unmark_block( window );
         } else if (marked_file->block_er > length) {
            marked_file->block_er = length;
            if (type == STREAM)
               marked_file->block_ec = -1;
            updated = TRUE;
         }
         if (updated)
            marked_file->dirty = GLOBAL;
      }
   }
}


/*
 * Name:    shift_tabbed_block
 * Class:   helper function
 * Purpose: to prepare shift_block() for a line containing real tabs
 * Author:  Jason Hood
 * Date:    October 10, 1999
 * Passed:  file:  pointer to file containing block
 * Returns: sets local globals
 * Notes:   the line being modified should be in the buffer.
 *          Needs to be called before the modification.
 *          Is overly complicated to handle blocks defined inside a tab, or
 *           beyond the end of the line.
 */
void shift_tabbed_block( file_infos *file )
{
   tabbed_bc = tabbed_ec = -1;

   if (file->inflate_tabs == 2 && mode.insert &&
       memchr( g_status.line_buff, '\t', g_status.line_buff_len ) != NULL) {
      tabbed_bc = entab_adjust_rcol( g_status.line_buff, g_status.line_buff_len,
                                     file->block_bc, file->ptab_size );
      if (file->block_ec != -1)
      tabbed_ec = entab_adjust_rcol( g_status.line_buff, g_status.line_buff_len,
                                     file->block_ec, file->ptab_size );

      tabbed_bc_ofs = tabbed_ec_ofs = 0;
      if (tabbed_bc >= g_status.line_buff_len) {
         tabbed_bc_ofs = tabbed_bc - g_status.line_buff_len;
         tabbed_bc = g_status.line_buff_len;
      } else if (g_status.line_buff[tabbed_bc] == '\t') {
         if (file->block_bc != detab_adjust_rcol( g_status.line_buff,
                                                  tabbed_bc, file->ptab_size ))
            tabbed_bc_ofs = file->block_bc -
                              detab_adjust_rcol( g_status.line_buff,
                                                 ++tabbed_bc, file->ptab_size );
      }
      if (tabbed_ec != -1) {
         if (tabbed_ec >= g_status.line_buff_len) {
            tabbed_ec_ofs = tabbed_ec - g_status.line_buff_len;
            tabbed_ec = g_status.line_buff_len;
         } else if (g_status.line_buff[tabbed_ec] == '\t') {
            tabbed_ec_ofs = file->block_ec -
                              detab_adjust_rcol( g_status.line_buff, tabbed_ec,
                                                 file->ptab_size );
         }
      }
   }
}


/*
 * Name:    shift_block
 * Class:   helper function
 * Purpose: To shift block beginning/ending column after an editing function
 * Author:  Jason Hood
 * Date:    October 9, 1999
 * Passed:  file:  pointer to file containing block
 *          rline: line number where changed occurred
 *          rcol:  position within rline where changed occurred
 *          net_change: number of characters added or removed
 * Notes:   Only applies to the beginning or ending line of a STREAM block,
 *           or a single-line BOX block.
 *          Characters are added at rcol, removed before rcol.
 *          shift_tabbed_block() should be called first (only for real tabs),
 *           the line should be modified, and then this function called.
 */
void shift_block( file_infos *file, long rline, int rcol, int net_change )
{
int  type;
int  bc, ec;
long br, er;

   type = abs( file->block_type );
   br = file->block_br;
   er = file->block_er;
   if (net_change != 0  &&
       ((type == BOX && rline == br && rline == er) ||
        (type == STREAM && (rline == br ||
                           (rline == er && file->block_ec != -1))))) {
      if (tabbed_bc >= 0) {
         bc = tabbed_bc;
         ec = tabbed_ec;
      } else {
         bc = file->block_bc;
         ec = file->block_ec;
      }
      if (net_change > 0) {
         if (type == STREAM && br != er) {
            if (rline == br) {
               if (rcol <= bc)
                  bc += net_change;
            } else {
               if (rcol <= ec)
                  ec += net_change;
            }
         } else if (rcol <= ec) {
            ec += net_change;
            if (rcol <= bc)
               bc += net_change;
         }
      } else {
         if (type == STREAM && br != er) {
            if (rline == br) {
               if (rcol <= bc)
                  bc += net_change;
               else if (rcol + net_change < bc)
                  bc = rcol + net_change;
            } else {
               if (rcol <= ec)
                  ec += net_change;
               else if (rcol + net_change < ec) {
                  ec = rcol + net_change - 1;
                  if (ec < 0)
                     ec = 0;
               }
            }
         } else {
            if (rcol <= bc) {
               bc += net_change;
               if (ec != -1)
                  ec += net_change;
            } else if (rcol <= ec) {
               ec += net_change;
               if (rcol + net_change < bc)
                  bc = rcol + net_change;
            } else if (rcol + net_change <= ec) {
               ec = rcol + net_change - 1;
               if (ec < bc) {
                  ec = file->block_bc;
                  file->block_type = -type;
                  if (g_status.marked && g_status.marked_file == file) {
                     g_status.marked = FALSE;
                     g_status.marked_file = NULL;
                  }
               }
            }
         }
      }
      if (tabbed_bc >= 0) {
         bc = detab_adjust_rcol( g_status.line_buff, bc, file->ptab_size )
              + tabbed_bc_ofs;
         if (ec != -1)
            ec = detab_adjust_rcol( g_status.line_buff, ec, file->ptab_size )
                 + tabbed_ec_ofs;
      }
      file->block_bc = bc;
      file->block_ec = ec;
   }
   tabbed_bc = tabbed_ec = -1;
}


/*
 * Name:    prepare_block
 * Class:   helper function
 * Purpose: To prepare a window/file for a block read, move or copy.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 *          file: pointer to file information.
 *          text_line: pointer to line in file to prepare.
 *          lend: line length.
 *          bc: beginning column of BOX.
 * Notes:   The main complication is that the cursor may be beyond the end
 *           of the current line, in which case extra padding spaces have
 *           to be added before the block operation can take place.
 *           this only occurs in BOX and STREAM operations.
 *          since we are padding a line, do not trim trailing space.
 */
int  prepare_block( TDE_WIN *window, line_list_ptr ll, int bc )
{
register int pad = 0;   /* amount of padding to be added */
register int len;

   assert( bc >= 0 );
   assert( bc < MAX_LINE_LENGTH );
   assert( ll->len != EOF );
   assert( g_status.copied == FALSE );

   copy_line( ll, window, TRUE );

   len = g_status.line_buff_len;
   pad = bc - len;
   if (pad > 0) {
      /*
       * insert the padding spaces
       */

      assert( pad >= 0 );
      assert( pad < MAX_LINE_LENGTH );

      memset( g_status.line_buff+len, ' ', pad );
      g_status.line_buff_len += pad;
   }
   /*
    * if inflate_tabs, let's don't entab the line until we get
    *   thru processing this line, e.g. copying, numbering....
    */
   return( un_copy_line( ll, window, FALSE, FALSE ) );
}


/*
 * Name:    pad_dest_line
 * Class:   helper function
 * Purpose: To prepare a window/file for a block move or copy.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 *          dest_file: pointer to file information.
 *          dest_line: pointer to line in file to prepare.
 *          ll:        pointer to linked list node to insert new node
 * Notes:   We are doing a BOX action (except DELETE).   We have come
 *          to the end of the file and have no more lines.  All this
 *          routine does is add a blank line to file.
 */
int  pad_dest_line( TDE_WIN *window, file_infos *dest_file, line_list_ptr ll )
{
int  rc;
line_list_ptr new_node;

   rc = OK;
   new_node = new_line( 0, &rc );
   if (rc == OK)
      insert_node( dest_file, ll, new_node );
   else {
      /*
       * file too big
       */
      error( WARNING, window->bottom_line, block4 );
   }
   return( rc );
}


/*
 * Name:    block_operation
 * Class:   primary editor function
 * Purpose: Master BOX, STREAM, or LINE routine.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   Operations on BOXs, STREAMs, or LINEs require several common
 *           operations.  All require finding the beginning and ending marks.
 *          This routine will handle block operations across files.  Since one
 *           must determine the relationship of source and destination blocks
 *           within a file, it is relatively easy to expand this relationship
 *           across files.
 *          This is probably the most complicated routine in the editor.  It
 *           is not easy to understand.
 *
 * jmh 980728: update syntax highlight flags.
 * jmh 980801: Add BORDER action.
 * jmh 980810: renamed from move_copy_delete_overlay_block;
 *             added left/right/center justify line and box blocks.
 * jmh 991026: allow LINE overlays.
 * jmh 991112: Add SUM action.
 */
int  block_operation( TDE_WIN *window )
{
int  action;
TDE_WIN *source_window = NULL;  /* source window for block moves */
line_list_ptr source;           /* source for block moves */
line_list_ptr dest;             /* destination for block moves */
int  lens;                      /* length of source line */
int  lend;                      /* length of destination line */
int  add;                       /* characters being added from another line */
int  block_len;                 /* length of the block */
line_list_ptr block_start;      /* start of block in file */
line_list_ptr block_end;  /* end of block in file - not same for LINE or BOX */
int  prompt_line;
int  same;                      /* are dest and source files the same file? */
int  source_only;               /* does operation affect only the block? */
int  source_changed;            /* does the operation affect only the dest? */
file_infos *source_file;
file_infos *dest_file;
int  rcol, bc, ec;              /* temporary column variables */
long rline;                     /* temporary real line variable */
long br, er;                    /* temporary line variables */
long block_num;                 /* starting number for block number */
long block_inc;                 /* increment to use for block number */
int  block_base;                /* radix of the block number */
int  block_type;
int  fill_char = 0;             /* fill character or length of pattern */
text_t style[MAX_COLS+2];       /* border characters or fill pattern */
int  style_len[8];              /* border lengths */
int  rc;
LANGUAGE *syntax;
const char *errmsg = NULL;
long number;

   /*
    * initialize block variables
    */
   rc = un_copy_line( window->ll, window, TRUE, TRUE );
   check_block( &source_window );
   if (g_status.marked == FALSE || rc == ERROR)
      return( ERROR );

   source_only = TRUE;
   source_changed = TRUE;
   switch (g_status.command) {
      case MoveBlock :
         action = MOVE;
         source_only = FALSE;
         break;
      case DeleteBlock :
         action = DELETE;
         break;
      case CopyBlock :
         action = COPY;
         source_only = FALSE;
         source_changed = FALSE;
         break;
      case KopyBlock :
         action = KOPY;
         source_only = FALSE;
         source_changed = FALSE;
         break;
      case FillBlock :
      case FillBlockDown :
      case FillBlockPattern :
         action = FILL;
         break;
      case OverlayBlock :
         action = OVERLAY;
         source_only = FALSE;
         source_changed = FALSE;
         break;
      case NumberBlock :
         action = NUMBER;
         break;
      case SwapBlock :
         action = SWAP;
         source_only = FALSE;
         break;
      case BorderBlock :
      case BorderBlockEx :
         action = BORDER;
         break;
      case BlockLeftJustify :
      case BlockRightJustify :
      case BlockCenterJustify :
         action = JUSTIFY;
         break;
      case SumBlock:
         action = SUM;
         source_changed = FALSE;
         break;
      default :
         return( ERROR );
   }
   source_file = g_status.marked_file;
   dest_file = window->file_info;
   if ((source_file->read_only && source_changed) ||
       (dest_file->read_only   && !source_only))
      return( ERROR );

   prompt_line = window->bottom_line;
   block_start = source_file->block_start;
   block_end = source_file->block_end;
   if (block_start == NULL  ||  block_end == NULL)
      return( ERROR );

   block_type = source_file->block_type;
   if (block_type != LINE  &&  block_type != STREAM  &&  block_type != BOX)
      return( ERROR );

   dest = window->ll;
   rline = window->rline;
   if (!source_only)
      if (dest->next == NULL ||
          (dest->prev == NULL && (block_type != LINE || action == SWAP)))
         return( ERROR );
   rc = OK;

   /*
    * jmh 980810: let's verify the block type at the beginning.
    */
   if (block_type != BOX) {
      switch (action) {
         case NUMBER:  errmsg = block3a; break;
         case FILL:    errmsg = block2;  break;
         case BORDER:  errmsg = block26; break;
         case SUM:     errmsg = block34; break;
      }
   }
   if (block_type == STREAM) {
      switch (action) {
         case OVERLAY: errmsg = block5;  break;
         case SWAP:    errmsg = block3b; break;
         case JUSTIFY: errmsg = block29; break;
      }
   }
   if (errmsg != NULL) {
      error( WARNING, prompt_line, errmsg );
      return( ERROR );
   }

   /*
    * set up Beginning Column, Ending Column, Beginning Row, Ending Row
    */
   bc = source_file->block_bc;
   ec = source_file->block_ec;
   br = source_file->block_br;
   er = source_file->block_er;

   /*
    * if we are BOX FILLing or BOX NUMBERing, beginning column is bc,
    *   not the column of cursor
    * jmh 980801: as with BORDERing.
    * jmh 980810: and JUSTIFYing.
    * jmh 991112: and SUMming.
    */
   rcol = (source_only) ? bc : window->rcol;

   /*
    * must find out if source and destination file are the same.
    */
   same = FALSE;
   if (source_only) {
      rc = (action == FILL)   ? get_block_fill_char(window, &fill_char, style) :
           (action == NUMBER) ? get_block_numbers( &block_num, &block_inc,
                                                   &block_base )               :
           (action == BORDER) ? get_block_border_style(window,style,style_len) :
           OK;
      if (rc == ERROR)
         return( ERROR );
      dest = block_start;
      same = TRUE;
   }

   if (source_file == dest_file && !source_only) {
      same = TRUE;
      if (block_type == BOX) {
         if (action == MOVE) {
            if (rline == br  &&  (rcol >= bc && rcol <= ec+1))
               return( ERROR );
         } else if (action == SWAP) {
            /*
             * jmh 980811: can't swap a block within itself
             */
            if (rline >= br && rline <= er &&
                rcol  >= bc && rcol  <= ec)
               return( ERROR );
         }
      } else if (block_type == LINE) {
         if (rline >= br && rline <= er) {
             /*
              * if COPYing or KOPYing within the block itself, reposition the
              * destination to the next line after the block (if it exists)
              */
            if (action == COPY || action == KOPY)
               dest = block_end;
             /*
              * a block moved to within the block itself has no effect
              * jmh 980810: a block can't swap with itself, either
              */
            else if (action == MOVE || action == SWAP)
               return( ERROR );
         }
      } else if (rline >= br && rline <= er) {

         /*
          * to find out if cursor is in a STREAM block we have to do
          * a few more tests.  if cursor is on the beginning row or
          * ending row, then check the beginning and ending column.
          */
         if ((rline > br && rline < er) ||
             (br == er && rcol >= bc && (rcol <= ec || ec == -1)) ||
             (br != er && ((rline == br && rcol >= bc) ||
                           (rline == er && (rcol <= ec || ec == -1))))) {

            /*
             * if the cursor is in middle of STREAM, make destination
             * the last character following the STREAM block.
             */
            if (action == COPY || action == KOPY) {
               if (ec == -1) {
                  dest = block_end->next;
                  rcol = 0;
                  rline = er + 1;
               } else {
                  dest = block_end;
                  rcol = ec + 1;
                  rline = er;
               }
            } else if (action == MOVE)
               return( ERROR );
         }
      }
   }

   /*
    * 1. can't create lines greater than MAX_LINE_LENGTH
    * 2. if we are FILLing a BOX - fill block buff once right here
    */
   block_len = (ec+1) - bc;
   if (block_type == BOX) {
      if (!source_only) {
         if (rcol + block_len > MAX_LINE_LENGTH) {
            /*
             * Error: line too long
             */
            error( INFO, prompt_line, ltol );
            return( ERROR );
         }
      }
      if (action == BORDER) {
         /*
          * The box must be at least big enough for the corners
          */
         if (g_status.command == BorderBlock) {
            if (block_len < 2 || br == er)
               rc = ERROR;
         } else {
            if (style_len[0] + style_len[2] > block_len  ||
                style_len[5] + style_len[7] > block_len  ||
                (((style_len[0] || style_len[2]) &&
                  (style_len[5] || style_len[7])) && br == er))
               rc = ERROR;
         }
         if (rc == ERROR) {
            /*
             * box not big enough for border
             */
            error( WARNING, prompt_line, block28a );
            return( ERROR );
         }
      }
   } else if (block_type == LINE) {
      block_len = 0;
   } else if (block_type == STREAM) {
      lend = block_end->len;
      if ((action == DELETE || action == MOVE) && ec != -1) {

         /*
          * Is what's left on start of STREAM block line plus what's left at
          * end of STREAM block line too long?
          */
         if (lend > ec)
            lend -= ec;
         else
            lend = 0;
         if (bc + lend > MAX_LINE_LENGTH) {
            error( INFO, prompt_line, ltol );
            return( ERROR );
         }
      }

      if (action != DELETE) {

         /*
          * We are doing a MOVE, COPY, or KOPY.  Find out if what's on the
          * current line plus the start of the STREAM line are too long.
          * Then find out if end of the STREAM line plus what's left of
          * the current line are too long.
          */
         lens = block_start->len;

         /*
          * if we had to move the destination of the STREAM COPY or KOPY
          * to the end of the STREAM block, then dest and window->ll->line
          * will not be the same.  In this case, set length to length of
          * first line in STREAM block.  Then we can add the residue of
          * the first line in block plus residue of the last line of block.
          */
         if (dest->line == window->ll->line)
            add = dest->len;
         else
            add = lens;

         /*
          * Is current line plus start of STREAM block line too long?
          */
         if (lens > bc)
            lens -= bc;
         else
            lens = 0;
         if (rcol + lens > MAX_LINE_LENGTH) {
            error( INFO, prompt_line, ltol );
            return( ERROR );
         }

         /*
          * Is residue of current line plus residue of STREAM block line
          * too long?
          */
         if (add > bc)
            add -= bc;
         else
            add = 0;
         if (lend > ec  &&  ec != -1)
            lend -= ec;
         else
            lend = 0;
         if (add + lend > MAX_LINE_LENGTH) {
            error( INFO, prompt_line, ltol );
            return( ERROR );
         }
      }
      if (block_start == block_end && ec != -1) {
         block_type = BOX;
         block_len = (ec+1) - bc;
      }
   }

   if (mode.do_backups == TRUE) {
      if (!source_only) {
         window->file_info->modified = TRUE;
         rc = backup_file( window );
      }
      if (source_changed) {
         source_window->file_info->modified = TRUE;
         if (rc != ERROR)
            rc = backup_file( source_window );
      }
      if (rc == ERROR)
         return( ERROR );
   }
   source = block_start;

   assert( block_start != NULL );
   assert( block_start->len != EOF );
   assert( block_end != NULL );
   assert( block_end->len != EOF );

   if (block_type == BOX)
      do_box_block( window, source_window, action,
                    source_file, dest_file, source, dest, br, er, rline,
                    block_num, block_inc, block_base, fill_char, style,
                    style_len, same, block_len, bc, ec, rcol, &rc );

   else if (block_type == LINE) {
      if (action != JUSTIFY)
         do_line_block( window, source_window, action,
                        source_file, dest_file, same, block_start, block_end,
                        source, dest, br, er, rline, &rc );
      else
         justify_line_block( source_window, block_start, br, er, &rc );

   } else
      do_stream_block( window, source_window, action,
                       source_file, dest_file, block_start, block_end,
                       source, dest, rline, br, er, bc, ec, rcol, &rc );

   if (!((action == SWAP && rc == ERROR) || action == SUM)) {
      if (!source_only) {
         dest_file->modified = TRUE;
         dest_file->dirty = GLOBAL;
      }
      if (source_only || source_changed) {
         source_file->modified = TRUE;
         source_file->dirty = GLOBAL;
      }
   }

   /*
    * Update the syntax highlight flags of affected lines.
    * Start with the source lines.
    */
   syntax = source_file->syntax;
   if (source_changed) {
      if (block_type == BOX)
         syntax_check_block( br, er, block_start, syntax );
      else if (block_type == STREAM)
         syntax_check_lines( block_start, syntax );
      /*
       * else block_type == LINE which is done in do_line_block
       */
   }

   /*
    * Now check the destination lines.
    */
   if (!source_only && block_type != LINE)
      syntax_check_block( br, er, dest, dest_file->syntax );

   /*
    * unless we are doing a KOPY, FILL, NUMBER, or OVERLAY we need to unmark
    * the block.  if we just did a KOPY, the beginning and ending may have
    * changed.  so, we must readjust beginning and ending rows.
    * jmh - don't unmark SWAP, as well.
    * jmh 980801: turned the unmark conditional around.
    * jmh 980814: move the mark along with the block.
    *             Go back to unmarking SWAP.
    */
   number = (er+1) - br;
   if ((action == KOPY || action == COPY) && block_type != BOX &&
       same && br >= rline  &&  rc != ERROR) {
      source_file->block_br += number - (block_type == STREAM);
      source_file->block_er += number - (block_type == STREAM && ec != -1);
   }
   if (action == SWAP) {
      swap_er = -1;
      if (rc != ERROR)
         unmark_block( window );
   } else if (action == COPY || action == MOVE || action == DELETE) {
      unmark_block( window );

      if (action == MOVE) {
         dest_file->block_type = -block_type;
         if (same && block_type == BOX && rline == br && rcol > bc) {
            dest_file->block_bc = rcol - block_len;
            dest_file->block_ec = rcol - 1;
         } else {
            dest_file->block_bc = rcol;
            if (block_type != STREAM)
               dest_file->block_ec = rcol + ec - bc;
         }
         --number;
         if (same && rline > br && block_type != BOX) {
            dest_file->block_br = rline - number;
            dest_file->block_er = rline;
         } else {
            dest_file->block_br = rline;
            dest_file->block_er = rline + number;
            if (block_type == LINE) {
               ++dest_file->block_br;
               ++dest_file->block_er;
            }
         }
      }
   }
   show_avail_mem( );
   return( rc );
}


/*
 * Name:    do_line_block
 * Purpose: delete, move, copy, or kopy a LINE block
 * Date:    April 1, 1993
 * Passed:  window:         pointer to destination window (current window)
 *          source_window:  pointer to source window
 *          action:         block action  --  KOPY, MOVE, etc...
 *          source_file:    pointer to source file structure
 *          dest_file:      pointer to destination file
 *          same:           are source and destination files same? T or F
 *          block_start:    pointer to first node in block
 *          block_end:      pointer to last node in block
 *          source:         pointer to source node
 *          dest:           pointer to destination node
 *          br:             beginning line number in marked block
 *          er:             ending line number in marked block
 *          rline:          current line number in destination file
 *          rc:             return code
 *
 * jmh:     September 11, 1997 - added SWAP
 * jmh 980817: if a break point is defined, SWAP will swap the block with the
 *              block delineated by the cursor and break point. If there is no
 *              break point, it swaps the same number of lines, or till eof.
 * jmh 981125: corrected a MOVE syntax check (set dest_end).
 * jmh 991026: added OVERLAY operation.
 * jmh 991027: SWAP asks for the region to be swapped.
 * jmh 991029: shift the mark to follow the SWAP.
 * jmh 991108: reduce the mark in an overlapped OVERLAY.
 */
void do_line_block( TDE_WIN *window,  TDE_WIN *source_window,  int action,
                    file_infos *source_file,  file_infos *dest_file,  int same,
                    line_list_ptr block_start,  line_list_ptr block_end,
                    line_list_ptr source,  line_list_ptr dest,
                    long br,  long er,  long rline,  int *rc )
{
line_list_ptr temp_ll;          /* temporary list pointer */
line_list_ptr temp2;            /* temporary list pointer */
line_list_ptr dest_end;
int  lens;                      /* length of source line */
long li;                        /* temporary line variables */
long diff;
long diff_dest = 1;
TDE_WIN d_w;                    /* a couple of temporary TDE_WINs */

   dest_end = dest;
   if (action == COPY || action == KOPY) {

      assert( br >= 1 );
      assert( br <= source_file->length );
      assert( er >= br );
      assert( er <= source_file->length );

      for (li=br; li <= er  &&  *rc == OK; li++) {
         lens = source->len;

         assert( lens * sizeof(char) < MAX_LINE_LENGTH );

         temp_ll = new_line_text( source->line, lens, source->type|DIRTY, rc );
         if (*rc == OK) {
            insert_node( dest_file, dest_end, temp_ll );
            dest_end = temp_ll;
            source = source->next;
         } else {
            /*
             * file too big
             */
            error( WARNING, window->bottom_line, main4 );
            er = li - 1;
         }
      }
   } else if (action == MOVE) {
      temp_ll = block_start;
      for (li=br; li <= er; li++) {
         temp_ll->type |= DIRTY;
         temp_ll = temp_ll->next;
      }
      block_start->prev->next = block_end->next;
      block_end->next->prev   = block_start->prev;
      /*
       * jmh 980806: The block has effectively been deleted; syntax check
       *             the lines after it while the block line pointers
       *             are still valid.
       */
      syntax_check_lines( block_end->next, source_file->syntax );
      dest->next->prev  = block_end;
      block_start->prev = dest;
      block_end->next   = dest->next;
      dest->next        = block_start;
      dest_end          = block_end;

   } else if (action == DELETE) {
      block_end->next->prev   = block_start->prev;
      block_start->prev->next = block_end->next;
      /*
       * jmh 980806: as with MOVE
       */
      syntax_check_lines( block_end->next, source_file->syntax );
      block_end->next = NULL;
      my_free_group( TRUE );
      while (block_start != NULL) {
         temp_ll = block_start;
         block_start = block_start->next;
         my_free( temp_ll->line );
         my_free( temp_ll );
      }
      my_free_group( FALSE );

   } else if (action == SWAP) {
      *rc = find_swap_lines( window, &dest, &dest_end, br, er, same );
      if (*rc == ERROR)
         return;
      temp_ll = dest;
      for (li = swap_br; li <= swap_er; ++li) {
         temp_ll->type |= DIRTY;
         temp_ll = temp_ll->next;
      }
      temp_ll = block_start;
      for (li = br; li <= er; ++li) {
         temp_ll->type |= DIRTY;
         temp_ll = temp_ll->next;
      }
      temp_ll           = block_start->prev;
      temp2             = dest->prev;
      temp_ll->next     = dest;
      temp2->next       = block_start;
      block_start->prev = temp2;
      dest->prev        = temp_ll;

      temp_ll           = block_end->next;
      temp2             = dest_end->next;
      temp_ll->prev     = dest_end;
      temp2->prev       = block_end;
      block_end->next   = temp2;
      dest_end->next    = temp_ll;

   } else if (action == OVERLAY) {
      if (same && rline == br)
         return;
      li = rline + er - br - dest_file->length;
      if (li > 0) {
         temp_ll = dest_file->line_list_end->prev;
         for (; li > 0  &&  *rc == OK; li--)
            *rc = pad_dest_line( window, dest_file, temp_ll );
         if (*rc == ERROR)
            return;
      }
      dup_window_info( &d_w, window );
      d_w.rline   = rline;
      d_w.ll      = dest;
      d_w.visible = FALSE;
      if (!same || rline < br || rline > er) {
         for (li = br; li <= er; ++li) {
            copy_line( source, source_window, TRUE );
            un_copy_line( d_w.ll, &d_w, TRUE, TRUE );
            d_w.ll->type |= DIRTY;
            source = source->next;
            d_w.ll = d_w.ll->next;
            ++d_w.rline;
         }
         --d_w.rline;
         if (same && d_w.rline >= br && d_w.rline < er)
            source_file->block_br = d_w.rline + 1;
      } else {
         for (li = er - br; li > 0; li--) {
            source = source->next;
            d_w.ll = d_w.ll->next;
            ++d_w.rline;
         }
         for (li = er; li >= br; --li) {
            copy_line( source, source_window, TRUE );
            un_copy_line( d_w.ll, &d_w, TRUE, TRUE );
            d_w.ll->type |= DIRTY;
            source = source->prev;
            d_w.ll = d_w.ll->prev;
            --d_w.rline;
         }
         source_file->block_er = rline - 1;
      }
      syntax_check_block( br, er, dest, dest_file->syntax );
   }

   diff =  er + 1L - br;
   if (action == COPY || action == KOPY || action == MOVE) {
      if (action == MOVE)
         dest_file->length += diff;
      adjust_windows_cursor( window, diff );
   }
   if (action == DELETE || action == MOVE) {
      source_file->length -= diff;
      adjust_windows_cursor( source_window, -diff );
   }
   if (action == SWAP) {
      diff_dest = swap_er + 1L - swap_br;
      source_file->length += diff_dest - diff;
      dest_file->length   += diff - diff_dest;
      adjust_windows_cursor( source_window, diff_dest - diff );
      adjust_windows_cursor( window,        diff - diff_dest );
      if (!same) {
         source_file->block_er = br + diff_dest - 1;
         dest_file->block_type = -LINE;
         if (source_window->rline > er)
            source_window->rline += diff_dest - diff;
         else if (source_window->rline > br + diff_dest - 1)
            source_window->rline = br + diff_dest - 1;
      } else if (swap_br > er)
         swap_br += diff_dest - diff;
      dest_file->block_br = swap_br;
      dest_file->block_er = swap_br + diff - 1;
   }
   /*
    * do the syntax highlighting check
    */
   if (action == DELETE && source_window->rline >= br) {
      source_window->rline -= diff;
      if (source_window->rline < br)
         source_window->rline = br;
   }
   if (action != DELETE && action != OVERLAY) {
      syntax_check_lines( (action == SWAP) ? dest : dest->next,
                          dest_file->syntax );
      syntax_check_lines( dest_end->next, dest_file->syntax );
      if (action == SWAP) {
         syntax_check_lines( block_start, source_file->syntax );
         syntax_check_lines( block_end->next, source_file->syntax );
      }
   }
   /*
    * restore all cursors in all windows
    */
   restore_cursors( dest_file );
   if (dest_file != source_file)
      restore_cursors( source_file );
   show_avail_mem( );
}


/*
 * Name:    do_stream_block
 * Purpose: delete, move, copy, or kopy a STREAM block
 * Date:    June 5, 1991
 * Passed:  window:         pointer to destination window (current window)
 *          source_window:  pointer to source window
 *          action:         block action  --  KOPY, MOVE, etc...
 *          source_file:    pointer to source file structure
 *          dest_file:      pointer to destination file
 *          block_start:    pointer to first node in block
 *          block_end:      pointer to last node in block
 *          source:         pointer to source node
 *          dest:           pointer to destination node
 *          rline:          current line number in destination file
 *          br:             beginning line number in marked block
 *          er:             ending line number in marked block
 *          bc:             beginning column of block
 *          ec:             ending column of block
 *          rcol:           current column of cursor
 *          rc:             return code
 *
 * jmh 020815: David Hughes pointed out a bug with the first and last lines
 *              of a COPY/KOPY possibly being modified. Use load_box_buff()
 *              instead of prepare_block() to overcome it;
 *             made appropriate lines dirty;
 *             fixed a bug padding the destination line beyond eol.
 * jmh 050714: if ec == -1 the last line itself is marked.
 */
void do_stream_block( TDE_WIN *window,  TDE_WIN *source_window,  int action,
                      file_infos *source_file,  file_infos *dest_file,
                      line_list_ptr block_start,  line_list_ptr block_end,
                      line_list_ptr source,  line_list_ptr dest, long rline,
                      long br, long er, int bc, int ec, int rcol, int *rc )
{
line_list_ptr temp_ll;          /* temporary list pointer */
int  lens;                      /* length of source line */
int  lend;                      /* length of destination line */
long li;                        /* temporary line variables */
long diff;
TDE_WIN s_w, d_w;               /* a couple of temporary TDE_WINs */

   dup_window_info( &s_w, source_window );
   dup_window_info( &d_w, window );
   s_w.rline   = br;
   s_w.ll      = block_start;
   s_w.visible = FALSE;
   d_w.rline   = rline;
   d_w.ll      = dest;
   d_w.visible = FALSE;

   if (!(action==COPY || action==KOPY)) {
      /*
       * pad the start of the STREAM block if needed.
       */
      lens = block_start->len;
      if (lens < bc || source_file->inflate_tabs)
         *rc = prepare_block( &s_w, block_start, bc );

      /*
       * pad the end of the STREAM block if needed.
       */
      if (ec != -1) {
         lens = block_end->len;
         if (*rc == OK  &&  (lens < ec+1  ||  source_file->inflate_tabs))
            *rc = prepare_block( &s_w, block_end, ec+1 );
      }
   }

   /*
    * pad the destination line if necessary
    */
   copy_line( dest, &d_w, TRUE );
   *rc = un_copy_line( dest, &d_w, FALSE, FALSE );
   lend = dest->len;
   if (*rc == OK && (action==MOVE || action==COPY || action==KOPY)) {
      if (lend < rcol+1 || dest_file->inflate_tabs)
         *rc = prepare_block( &d_w, dest, rcol+1 );
   }

   if ((action == COPY || action == KOPY) && *rc == OK) {

      /*
       * concatenate the end of the STREAM block with the end of the
       *   destination line.
       */
      lens = dest->len - rcol;

      assert( lens >= 0 );
      assert( lens <= MAX_LINE_LENGTH );
      assert( ec + 1 >= 0 );
      assert( ec + 1 <= MAX_LINE_LENGTH );
      assert( rcol >= 0 );

      if (ec != -1)
         load_box_buff( g_status.line_buff, block_end, 0, ec, ' ',
                        source_file->inflate_tabs, source_file->ptab_size );
      my_memcpy( g_status.line_buff+ec+1, dest->line+rcol, lens );
      lens += ec + 1;
      g_status.line_buff_len = lens;

      temp_ll = new_line( DIRTY, rc );
      if (*rc == OK) {
         g_status.copied = TRUE;
         *rc = un_copy_line( temp_ll, &d_w, TRUE, FALSE );

         if (*rc == OK) {
            dest->next->prev = temp_ll;
            temp_ll->next = dest->next;
            dest->next = temp_ll;
            temp_ll->prev = dest;
         } else
            my_free( temp_ll );
      }

      /*
       * file too big
       */
      if (*rc != OK)
         error( WARNING, window->bottom_line, main4 );

      if (*rc == OK) {
         g_status.copied = FALSE;
         copy_line( dest, &d_w, FALSE );
         lens = find_end( block_start->line, block_start->len,
                          source_file->inflate_tabs, source_file->ptab_size );

         assert( lens >= 0 );
         assert( lens <= MAX_LINE_LENGTH );
         assert( bc >= 0 );
         assert( bc <= MAX_LINE_LENGTH );
         assert( rcol >= 0 );

         load_box_buff( g_status.line_buff+rcol, block_start, bc, lens-1, ' ',
                        source_file->inflate_tabs, source_file->ptab_size );
         g_status.line_buff_len = rcol + lens - bc;
         *rc = un_copy_line( dest, &d_w, TRUE, FALSE );
         dest->type |= DIRTY;
      }

      source = block_start->next;
      if (dest == block_start)
         source = source->next;
      for (li = br + (ec != -1); li < er  &&  *rc == OK; li++) {
         lens = source->len;

         assert( lens >= 0 );
         assert( lens <= MAX_LINE_LENGTH );

         temp_ll = new_line_text( source->line, lens, DIRTY, rc );
         if (*rc == OK) {
            insert_node( window->file_info, dest, temp_ll );
            dest   = temp_ll;
            source = source->next;
         } else {
            /*
             * file too big
             */
            error( WARNING, window->bottom_line, main4 );
            *rc = WARNING;
         }
      }
   } else if (action == MOVE) {

      /*
       * is the dest on the same line as the block_start?
       */
      if (dest == block_start) {

         /*
          * move the text between rcol and bc in block_start->line
          *   to block_end->line + ec.
          */
         lens = bc - rcol;
         if (ec == -1) {
            temp_ll = new_line_text( block_start->line+rcol, lens, DIRTY, rc );
            if (*rc == OK)
               insert_node( window->file_info, block_end, temp_ll );
            else {
               /*
                * file too big
                */
               error( WARNING, window->bottom_line, main4 );
               *rc = WARNING;
            }
         } else {
            lend = block_end->len - (ec + 1);
            g_status.copied = FALSE;
            copy_line( block_end, &s_w, FALSE );


            assert( lens >= 0 );
            assert( lens <= MAX_LINE_LENGTH );
            assert( lend >= 0 );
            assert( lend <= MAX_LINE_LENGTH );
            assert( ec + lens + 1 <= MAX_LINE_LENGTH );
            assert( rcol >= 0 );


            my_memmove( g_status.line_buff + ec + lens + 1,
                        g_status.line_buff + ec + 1,  lend );
            my_memcpy( g_status.line_buff+ec+1, block_start->line+rcol, lens );
            g_status.line_buff_len = block_end->len + lens;
            *rc = un_copy_line( block_end, &d_w, TRUE, FALSE );
         }

         /*
          * now, remove the text between rcol and bc on block_start->line
          */
         if (*rc == OK) {
            lend = block_start->len - bc;
            copy_line( block_start, &s_w, FALSE );

            assert( lend >= 0 );
            assert( lend < MAX_LINE_LENGTH );

            my_memmove( g_status.line_buff + rcol,
                        g_status.line_buff + bc, lend );

            assert( block_start->len - (bc - rcol) >= 0 );
            assert( block_start->len - (bc - rcol) <= MAX_LINE_LENGTH );

            g_status.line_buff_len = block_start->len - (bc - rcol);
            *rc = un_copy_line( block_start, &d_w, TRUE, FALSE );
            block_start->type |= DIRTY;
            block_end->type   |= DIRTY;
         }

      /*
       * is the dest on the same line as the block_end?
       */
      } else if (dest == block_end) {

         /*
          * move the text between rcol and ec on block_end->line to
          *   block_start->line + bc.
          */
         lens = rcol - ec;
         lend = block_start->len - bc;
         g_status.copied = FALSE;
         copy_line( block_start, &s_w, FALSE );

         assert( lens >= 0 );
         assert( lens <= MAX_LINE_LENGTH );
         assert( lend >= 0 );
         assert( lend <= MAX_LINE_LENGTH );
         assert( bc + lens <= MAX_LINE_LENGTH );
         assert( ec + 1 >= 0 );

         my_memmove( g_status.line_buff + bc + lens,
                     g_status.line_buff + bc,  lend );
         my_memcpy( g_status.line_buff+bc, block_end->line+ec+1, lens );

         assert( block_start->len + lens >= 0 );
         assert( block_start->len + lens <= MAX_LINE_LENGTH );

         g_status.line_buff_len = block_start->len + lens;
         *rc = un_copy_line( block_start, &d_w, TRUE, FALSE );

         /*
          * now, remove the text on block_end->line between rcol and ec
          */
         if (*rc == OK) {
            lend = block_end->len - (rcol + 1);
            copy_line( block_end, &s_w, FALSE );

            assert( lend >= 0 );
            assert( lend <= MAX_LINE_LENGTH );
            assert( ec + 1 >= 0 );
            assert( rcol + 1 >= 0 );
            assert( ec + 1 <= MAX_LINE_LENGTH );
            assert( rcol + 1 <= MAX_LINE_LENGTH );
            assert( block_end->len - (rcol - ec) >= 0 );
            assert( block_end->len - (rcol - ec) <= MAX_LINE_LENGTH );


            my_memmove( g_status.line_buff + ec + 1,
                        g_status.line_buff + rcol + 1, lend );
            g_status.line_buff_len = block_end->len - (rcol - ec);
            *rc = un_copy_line( block_end, &d_w, TRUE, FALSE );
            block_start->type |= DIRTY;
            block_end->type   |= DIRTY;
         }
      } else {

         lens = dest->len - rcol;

         assert( ec + 1 >= 0 );
         assert( ec + 1 <= MAX_LINE_LENGTH );
         assert( lens >= 0 );
         assert( lens <= MAX_LINE_LENGTH );
         assert( rcol >= 0 );
         assert( rcol <= MAX_LINE_LENGTH );

         my_memcpy( g_status.line_buff, block_end->line, ec+1 );
         my_memcpy( g_status.line_buff+ec+1, dest->line+rcol, lens );
         lens += ec + 1;
         g_status.line_buff_len = lens;

         temp_ll = new_line( DIRTY, rc );
         if (*rc == OK) {
            g_status.copied = TRUE;
            *rc = un_copy_line( temp_ll, &d_w, TRUE, FALSE );

            if (*rc != ERROR) {
               dest->next->prev = temp_ll;
               temp_ll->next = dest->next;
               dest->next = temp_ll;
               temp_ll->prev = dest;
               if (ec == -1)
                  ++dest_file->length;
            } else
               my_free( temp_ll );
         }

         /*
          * file too big
          */
         if (*rc != OK)
            error( WARNING, window->bottom_line, main4 );

         if (*rc == OK) {
            copy_line( dest, &d_w, FALSE );
            lens = block_start->len - bc;

            assert( bc >= 0 );
            assert( bc <= MAX_LINE_LENGTH );
            assert( lens >= 0 );
            assert( lens <= MAX_LINE_LENGTH );
            assert( rcol >= 0 );
            assert( rcol <= MAX_LINE_LENGTH );

            my_memcpy( g_status.line_buff+rcol, block_start->line+bc, lens );
            g_status.line_buff_len = lens + rcol;
            *rc = un_copy_line( dest, &d_w, TRUE, FALSE );
            dest->type |= DIRTY;
         }

         if (*rc == OK  &&  (block_start->next != block_end || ec == -1)  &&
             block_start != block_end) {
            block_start->next->prev = dest;
            dest->next = block_start->next;
            if (ec == -1) {
               block_start->next = block_end->next;
               block_end->next->prev = block_start;
               temp_ll->prev = block_end;
               block_end->next = temp_ll;
            } else {
               temp_ll->prev = block_end->prev;
               block_end->prev->next = temp_ll;
            }
            temp_ll = block_start;
            for (li = br; li < er; ++li) {
               temp_ll->type |= DIRTY;
               temp_ll = temp_ll->next;
            }
         }

         if (*rc == OK) {
            copy_line( block_start, &s_w, TRUE );
            lend = bc;
            lens = (ec == -1) ? 0 : block_end->len - (ec + 1);

            assert( bc >= 0 );
            assert( bc <= MAX_LINE_LENGTH );
            assert( lens >= 0 );
            assert( lens <= MAX_LINE_LENGTH );
            assert( lend >= 0 );
            assert( lend <= MAX_LINE_LENGTH );
            assert( ec + 1 >= 0 );
            assert( ec + 1 <= MAX_LINE_LENGTH );
            assert( lens + lend >= 0 );
            assert( lens + lend <= MAX_LINE_LENGTH );

            my_memcpy( g_status.line_buff+bc, block_end->line+ec+1, lens );
            g_status.line_buff_len = lend + lens;
            *rc = un_copy_line( block_start, &s_w, TRUE, FALSE );
            if (ec != -1) {
               block_start->next = block_end->next;
               block_end->next->prev = block_start;
               my_free( block_end->line );
               my_free( block_end );
            }
         }
      }
   } else if (action == DELETE) {
      copy_line( block_start, &s_w, FALSE );
      lens = (ec == -1) ? 0 : block_end->len - (ec + 1);

      assert( bc >= 0 );
      assert( bc <= MAX_LINE_LENGTH );
      assert( lens >= 0 );
      assert( lens <= MAX_LINE_LENGTH );
      assert( ec + 1 >= 0 );
      assert( ec + 1 <= MAX_LINE_LENGTH );
      assert( bc + lens >= 0 );
      assert( bc + lens <= MAX_LINE_LENGTH );

      my_memcpy( g_status.line_buff+bc, block_end->line + ec+1, lens );
      g_status.line_buff_len = bc + lens;
      *rc = un_copy_line( block_start, &s_w, TRUE, FALSE );
      block_start->type |= DIRTY;
      if (br != er) {
         source = block_start->next;
         block_start->next = block_end->next;
         block_end->next->prev = block_start;
         block_end->next = NULL;
         my_free_group( TRUE );
         while (source != NULL) {
            temp_ll = source;
            source = source->next;
            my_free( temp_ll->line );
            my_free( temp_ll );
         }
         my_free_group( FALSE );
      }
   }

   if (*rc == OK) {
      diff = er - br;
      if (action == COPY || action == KOPY || action == MOVE) {
         if (action == MOVE)
            dest_file->length += diff;
         else if (diff || ec == -1)
            ++dest_file->length;
         adjust_windows_cursor( window, diff + (ec == -1) );
      }
      if (action == DELETE || action == MOVE) {
         source_file->length -= diff;
         adjust_windows_cursor( source_window, -diff );
      }
      if (action == DELETE && source_window->rline >= br) {
         source_window->rline -= diff;
         if (source_window->rline < br)
            source_window->rline = br;
      }
   }

   /*
    * restore all cursors in all windows
    */
   restore_cursors( dest_file );
   if (dest_file != source_file)
      restore_cursors( source_file );
   show_avail_mem( );
}


/*
 * Name:    do_box_block
 * Purpose: delete, move, copy, or kopy a BOX block
 * Date:    June 5, 1991
 * Passed:  window:         pointer to destination window (current window)
 *          source_window:  pointer to source window
 *          action:         block action  --  OVERLAY, FILL, etc...
 *          source_file:    pointer to source file structure
 *          dest_file:      pointer to destination file
 *          source:         pointer to source node
 *          dest:           pointer to destination node
 *          br:             beginning line number in marked block
 *          er:             ending line number in marked block
 *          rline:          current line number in destination file
 *          block_num:      starting number when numbering a block
 *          block_inc:      increment used to number a block
 *          block_base:     radix to use for the number
 *          fill_char:      character to fill a block or length of pattern
 *          style:          border characters or fill pattern
 *          style_len:      border lengths
 *          same:           are source and destination files same? T or F
 *          block_len:      width of box block
 *          bc:             beginning column of block
 *          ec:             ending column of block
 *          rcol:           current column of cursor
 *          rc:             return code
 *
 * jmh 980801: added BORDER action / style parameter.
 * jmh 980811: added JUSTIFY action.
 * jmh 991030: added variable width SWAP, shift the mark.
 * jmh 991112: added SUM action. If the file is read-only or the cursor is on
 *              the top/end of file line, display the total, otherwise insert
 *              (or overwrite) it at the current position.
 * jmh 010520: corrected SUM bug (needed an extra NUL).
 */
void do_box_block( TDE_WIN *window,  TDE_WIN *source_window,  int action,
                   file_infos *source_file,  file_infos *dest_file,
                   line_list_ptr source,  line_list_ptr dest, long br, long er,
                   long rline, long block_num, long block_inc, int block_base,
                   int fill_char, text_ptr style, int *style_len,
                   int same, int block_len, int bc, int ec, int rcol, int *rc )
{
line_list_ptr p;                /* temporary list pointer */
int  lens;                      /* length of source line */
int  lend;                      /* length of destination line */
int  add;                       /* characters being added from another line */
text_ptr block_buff;
text_ptr swap_buff = NULL;
int  sbc, sec;                  /* temporary column variables */
long li;                        /* temporary line variables */
long dest_add;                  /* number of bytes added to destination file */
TDE_WIN s_w, d_w;       /* a couple of temporary TDE_WINs for BOX stuff */
TDE_WIN *w;
int  padded_file = FALSE;
int  (*justify)( TDE_WIN * ) = 0;
int  lm = 0, rm = 0;
int  source_tabs, dest_tabs;
int  source_tab_size, dest_tab_size;
int  swap_len;
int  dbc, dec;
int  sover, dover, ssame;
int  diff;

   if (action == SWAP)
      swap_buff = malloc( BUFF_SIZE + 2 );
   block_buff   = malloc( BUFF_SIZE + 2 );
   if (block_buff == NULL) {
      if (swap_buff != NULL)
         free( swap_buff );
      error( WARNING, window->bottom_line, block4 );
      *rc = ERROR;
      return;
   }
   swap_len = diff = 0;
   sover = dover = ssame = FALSE;

   /*
    * see if we need to add pad lines at eof.
    */
   if (action == MOVE || action == COPY || action == KOPY ||
       action == SWAP || action == OVERLAY) {
      dest_add = rline - br;
      if (dest_add + er > dest_file->length) {
         dest_add -= (dest_file->length - er);
         p = dest_file->line_list_end->prev;
         for (; dest_add > 0  &&  *rc == OK; dest_add--)
            *rc = pad_dest_line( window, dest_file, p );
         padded_file = TRUE;
      }
   }
   dest_add = 0;
   if (action == SWAP) {
      *rc = find_swap_box( window, br, er, bc, ec, same );
      if (*rc == OK) {
         swap_len = swap_ec - swap_bc + 1;
         diff = block_len - swap_len;
         while (rline > swap_br) {
            dest = dest->prev;
            --rline;
         }
         while (rline < swap_br) {
            dest = dest->next;
            ++rline;
         }
         rcol = swap_bc;
      }
   }

   if (*rc == OK) {

      dup_window_info( &s_w, source_window );
      dup_window_info( &d_w, window );
      s_w.rline   = br;
      s_w.ll      = source;
      s_w.visible = FALSE;
      d_w.rline   = rline;
      d_w.ll      = dest;
      d_w.visible = FALSE;
      source_tabs = source_file->inflate_tabs;
      dest_tabs   = dest_file->inflate_tabs;
      source_tab_size = source_file->ptab_size;
      dest_tab_size   = dest_file->ptab_size;

      if (action == JUSTIFY) {
         justify = (g_status.command == BlockLeftJustify)  ? flush_left  :
                   (g_status.command == BlockRightJustify) ? flush_right :
                                                             flush_center;
         lm = mode.left_margin;
         rm = mode.right_margin;
         mode.left_margin  = 0;
         mode.right_margin = block_len - 1;
      }

      /*
       * jmh 991103: if we're swapping a different number of lines, then the
       *             swap is directly above the block.
       */
      if (action == SWAP  &&  er - br != swap_er - swap_br) {
         for (li = br; li <= er  &&  *rc == OK && !g_status.control_break;
              li++, s_w.rline++, d_w.rline++) {
            lens = find_end( source->line, source->len,
                             source_tabs, source_tab_size );
            lend = find_end( dest->line, dest->len, dest_tabs, dest_tab_size );
            if (lens != 0 || lend != 0) {
               load_box_buff( block_buff, source, bc, ec, ' ',
                              dest_tabs, dest_tab_size );
               *rc = copy_buff_2file( &d_w, block_buff, dest, bc,
                                      block_len, COPY );
               if (*rc == OK) {
                  add = block_len;
                  if (lens <= ec)
                     add = lens - bc;
                  *rc = delete_box_block( &s_w, source, bc, add );
                  dest->type |= DIRTY;
               }
            }
            source = source->next;
            dest   = dest->next;
         }
         source = d_w.ll;
         s_w.rline = swap_br;
         sbc = bc + block_len;
         sec = ec + block_len;
         for (li = swap_br; li <= swap_er && *rc == OK &&
                            !g_status.control_break;
              li++, s_w.rline++, d_w.rline++) {
            lens = find_end( source->line, source->len,
                             source_tabs, source_tab_size );
            lend = find_end( dest->line, dest->len, dest_tabs, dest_tab_size );
            if (lens != 0 || lend != 0) {
               load_box_buff( block_buff, source, sbc, sec, ' ',
                              dest_tabs, dest_tab_size );
               *rc = copy_buff_2file( &d_w, block_buff, dest, bc,
                                      block_len, COPY );
               if (*rc == OK) {
                  add = block_len;
                  if (lens <= sec)
                     add = lens - sbc;
                  *rc = delete_box_block( &s_w, source, sbc, add );
                  dest->type |= DIRTY;
               }
            }
            source = source->next;
            dest   = dest->next;
         }
         source_file->block_br = swap_br;
         source_file->block_er = swap_br + er - br;


      /*
       * If the block overlaps the destination, work backwards so the block
       * doesn't use previous block text to fill the block.
       */
      } else if (same  &&  rline > br && rline <= er  &&
          (action == MOVE || action == COPY || action == KOPY ||
           action == OVERLAY)) {

         /*
          * move source and dest pointers to the end of the block
          */
         for (li = er - br; li > 0; li--) {
            load_undo_buffer( dest_file, dest->line, dest->len );
            source = source->next;
            dest   = dest->next;
            ++s_w.rline;
            ++d_w.rline;
         }

         for (li = er; *rc == OK  &&  li >= br  &&  !g_status.control_break;
              li--, s_w.rline--, d_w.rline--) {
            lens = find_end( source->line, source->len,
                             source_tabs, source_tab_size );
            lend = find_end( dest->line, dest->len, dest_tabs, dest_tab_size );
            if (lens != 0 || lend != 0) {
               load_box_buff( block_buff, source, bc, ec, ' ',
                              source_tabs, source_tab_size );
               *rc = copy_buff_2file( &d_w, block_buff, dest, rcol,
                                      block_len, action );
               dest->type |= DIRTY;

               if (action == MOVE) {
                  if (lens > bc) {
                     source->type |= DIRTY;
                     add = block_len;
                     if (lens <= ec)
                        add = lens - bc;
                     *rc = delete_box_block( &s_w, source, bc, add );
                  }
               }
            }
            source = source->prev;
            dest   = dest->prev;
         }
         if (action == OVERLAY && rcol == bc)
            source_file->block_er = rline - 1;

      } else {
         if (action == FILL) {
            if (g_status.command == FillBlock)
               block_fill( block_buff, fill_char, block_len );
            else if (g_status.command == FillBlockDown)
               load_box_buff( block_buff, source, bc, ec, ' ',
                              source_tabs, source_tab_size );

         } else if (action == SUM)
            block_buff[block_len] = '\0';

         else if (action == SWAP && same) {
            sover = (rline > br && rline <= er && rcol < bc);
            dover = (br > swap_br && br <= swap_er && bc < swap_bc);
            ssame = (br == swap_br && bc > swap_bc);
         }

         for (li = br; *rc == OK  &&  li <= er  &&  !g_status.control_break;
              li++, s_w.rline++, d_w.rline++) {
            switch (action) {
               case BORDER  :
               case FILL    :
               case NUMBER  :
               case DELETE  :
               case MOVE    :
               case SWAP    :
                  load_undo_buffer( source_file, source->line, source->len );
                  if (action != SWAP)
                     break;
                  /* else fall through */
               case COPY    :
               case KOPY    :
               case OVERLAY :
                  load_undo_buffer( dest_file, dest->line, dest->len );
                  break;
            }
            lens = find_end( source->line, source->len,
                             source_tabs, source_tab_size );
            lend = find_end( dest->line, dest->len, dest_tabs, dest_tab_size );

            if (action == SUM) {
               load_box_buff( block_buff, source, bc, ec, '\0',
                              source_tabs, source_tab_size );
               dest_add += strtol( (char *)block_buff, NULL, 10 );

            /*
             * with FILL and NUMBER operations, we're just adding chars
             *   to the file at the source location.  we don't have to
             *   worry about bookkeeping.
             * jmh 980801: as with BORDER.
             * jmh 980811: as with JUSTIFY.
             */
            } else if (action == FILL   || action == NUMBER ||
                       action == BORDER || action == JUSTIFY) {
               if (action == NUMBER) {
                  number_block_buff( block_buff, block_len,
                                     block_num, block_base );
                  block_num += block_inc;

               } else if (action == BORDER) {
                  load_box_buff( block_buff, source, bc, ec, ' ',
                                 source_tabs, source_tab_size );
                  border_block_buff( block_buff, block_len, br, er, li,
                                     style, style_len );

               } else if (action == JUSTIFY) {
                  load_box_buff( g_status.line_buff, source, bc, ec, ' ',
                                 source_tabs, source_tab_size );
                  g_status.line_buff_len = block_len;
                  g_status.copied = TRUE;
                  justify( &s_w );
                  memset( block_buff, ' ', block_len );
                  memcpy( block_buff, g_status.line_buff,
                          g_status.line_buff_len );
                  g_status.copied = FALSE;

               } else if (g_status.command == FillBlockPattern)
                  diff = block_pattern( block_buff, block_len, fill_char,
                                        style, diff );

               *rc = copy_buff_2file( &s_w, block_buff, source, rcol,
                                      block_len, action );
               source->type |= DIRTY;

            /*
             * if we are doing a BOX action and both the source and
             * destination are 0 then we have nothing to do.
             */
            } else if (lens != 0 || lend != 0) {

               /*
                * do actions that may require adding to file
                */
               /* MOVE, COPY, KOPY, OVERLAY */
               if (action != DELETE && action != SWAP) {
                  load_box_buff( block_buff, source, bc, ec, ' ',
                                 source_tabs, source_tab_size );
                  *rc = copy_buff_2file( &d_w, block_buff, dest, rcol,
                                         block_len, action );
                  dest->type |= DIRTY;

               } else if (action == SWAP) {
                  sbc = bc;
                  sec = ec;
                  if (sover && li >= rline) {
                     sbc += diff;
                     sec += diff;
                  }
                  load_box_buff( block_buff, source, sbc, sec, ' ',
                                 source_tabs, source_tab_size );
                  dbc = swap_bc;
                  dec = swap_ec;
                  if (dover && d_w.rline >= br) {
                     dbc -= diff;
                     dec -= diff;
                  }
                  load_box_buff( swap_buff, dest, dbc, dec, ' ',
                                 dest_tabs, dest_tab_size );

                  *rc = copy_buff_2file( &d_w, block_buff, dest, dbc,
                                         block_len, -swap_len );
                  if (ssame)
                     sbc += diff;
                  *rc = copy_buff_2file( &s_w, swap_buff, source, sbc,
                                         swap_len, -block_len );
                  source->type |= DIRTY;
                  dest->type   |= DIRTY;
               }

               /*
                * do actions that may require deleting from file
                */
               if (action == MOVE || action == DELETE) {
                  if (lens > bc) {
                     source->type |= DIRTY;
                     add = block_len;
                     if (lens <= ec)
                        add = lens - bc;
                     sbc = bc;
                     if (action == MOVE && rline == br && rcol < bc)
                        sbc += block_len;
                     *rc = delete_box_block( &s_w, source, sbc, add );
                  }
               }
            }
            source = source->next;
            dest   = dest->next;
         }
         if (action == SWAP) {
            dest_file->block_br = swap_br;
            dest_file->block_er = swap_er;
            dest_file->block_bc = swap_bc;
            if (same && rline == br && bc < swap_bc)
               dest_file->block_bc -= diff;
            dest_file->block_ec = dest_file->block_bc + block_len - 1;
            if (!same) {
               dest_file->block_type = -BOX;
               source_file->block_ec -= diff;
            }
         } else if (action == OVERLAY) {
            --d_w.rline;
            if (same && rcol == bc && d_w.rline >= br && d_w.rline < er)
               source_file->block_br = d_w.rline + 1;
         }
      }
      if (action == JUSTIFY) {
         mode.left_margin  = lm;
         mode.right_margin = rm;
      }
      else if (action == SUM) {
         if (dest_file->read_only || window->ll->len == EOF) {
            sprintf( (char *)block_buff, block35, dest_add );
            error( INFO, window->bottom_line, (char *)block_buff );
         } else {
            my_ltoa( dest_add, (char *)block_buff, 10 );
            add_chars( block_buff, window );
         }
      }
   }
   if (padded_file) {
      w = g_status.window_list;
      while (w != NULL) {
         if (w->file_info == dest_file  &&  w->visible)
            show_size( w );
         w = w->next;
      }
   }
   free( block_buff );
   if (action == SWAP)
      free( swap_buff );
   show_avail_mem( );
}


/*
 * Name:    load_box_buff
 * Class:   helper function
 * Purpose: copy the contents of a BOX to a block buffer.
 * Date:    June 5, 1991
 * Passed:  block_buff: local buffer for block moves
 *          ll:         node to source line in file to load
 *          bc:     beginning column of BOX. used only in BOX operations.
 *          ec:     ending column of BOX. used only in BOX operations.
 *          filler: character to fill boxes that end past eol
 * Notes:   For BOX blocks, there are several things to take care of:
 *            1) The BOX begins and ends within a line - just copy the blocked
 *            characters to the block buff.  2) the BOX begins within a line
 *            but ends past the eol - copy all the characters within the line
 *            to the block buff then fill with padding.  3) the BOX begins and
 *            ends past eol - fill entire block buff with padding (filler).
 *          the fill character varies with the block operation.  for sorting
 *            a box block, the fill character is '\0'.  for adding text to
 *            the file, the fill character is a space.
 */
void load_box_buff( text_ptr block_buff, line_list_ptr ll, int bc, int ec,
                    char filler, int tabs, int tab_size )
{
int  len;
int  avlen;
register int i;
register text_ptr bb;
text_ptr s;

   assert( bc >= 0 );
   assert( ec >= bc );
   assert( ec < MAX_LINE_LENGTH );

   bb = block_buff;
   len = ll->len;
   s = detab_a_line( ll->line, &len, tabs, tab_size );
   /*
    * block start may be past eol
    */
   if (len < ec + 1) {
      /*
       * does block start past eol? - fill with pad
       */
      assert( ec + 1 - bc >= 0 );

      memset( block_buff, filler, (ec + 1) - bc );
      if (len >= bc) {
         /*
          * block ends past eol - fill with pad
          */
         avlen = len - bc;
         s += bc;
         for (i=avlen; i>0; i--)
            *bb++ = *s++;
      }
   } else {
      /*
       * block is within line - copy block to buffer
       */
      avlen = (ec + 1) - bc;
      s += bc;
      for (i=avlen; i>0; i--)
         *bb++ = *s++;
   }
}


/*
 * Name:    copy_buff_2file
 * Class:   helper function
 * Purpose: copy the contents of block buffer to destination file
 * Date:    June 5, 1991
 * Passed:  window:     pointer to current window
 *          block_buff: local buffer for moves
 *          dest:       pointer to destination line in destination file
 *          rcol:       if in BOX mode, destination column in destination file
 *          block_len:  if in BOX mode, width of block to copy
 *          action:     type of block action
 * Notes:   In BOX mode, the destination line has already been prepared.
 *          Just copy the BOX buffer to the destination line.
 *
 * jmh 991030: an action < 1 indicates a SWAP, where abs( action ) is the
 *              length of the original block.
 */
int  copy_buff_2file( TDE_WIN *window, text_ptr block_buff, line_list_ptr dest,
                      int rcol, int block_len, int action )
{
text_ptr s;
int  len;
int  pad;
int  add;

   copy_line( dest, window, TRUE );
   len = g_status.line_buff_len;

   assert( len >= 0 );
   assert( len < MAX_LINE_LENGTH );
   assert( rcol >= 0 );
   assert( rcol < MAX_LINE_LENGTH );
   assert( block_len >= 0 );
   assert( block_len < BUFF_SIZE );

   if (rcol > len) {
      pad = rcol - len;
      memset( g_status.line_buff + len, ' ', pad );
      len += pad;
   }

   s = g_status.line_buff + rcol;

   /*
    * s is pointing to location to perform BOX operation.  If we do a
    * FILL or OVERLAY, we do not necessarily add any extra space.  If the
    * line does not extend all the thru the BOX then we add.
    * we always add space when we COPY, KOPY, or MOVE
    * jmh 991030: If we do a SWAP, may need to add or remove space.
    */
   add = len - rcol;
   if (action < 1) {            /* action == SWAP */
      if (add < -action)
         len += -action - add;
      pad = block_len + action; /* block_len - swap_len */
      if (pad > 0)
         memmove( s + pad, s, add );
      else if (pad < 0) {
         add += pad;
         if (add > 0)
            memmove( s, s - pad, add );
      }
      len += pad;
   } else if (action == COPY || action == KOPY || action == MOVE) {
      memmove( s + block_len, s, add );
      len += block_len;
   } else if (add < block_len)
      len += block_len - add;

   assert( rcol + block_len <= len );
   assert( len >= 0 );
   assert( len < MAX_LINE_LENGTH );

   memmove( s, block_buff, block_len );
   g_status.line_buff_len = len;
   return( un_copy_line( dest, window, TRUE, TRUE ) );
}


/*
 * Name:    block_fill
 * Class:   helper function
 * Purpose: fill the block buffer with character
 * Date:    June 5, 1991
 * Passed:  block_buff: local buffer for moves
 *          fill_char:  fill character
 *          block_len:  number of columns in block
 * Notes:   Fill block_buffer for block_len characters using fill_char.  This
 *          function is used only for BOX blocks.
 */
void block_fill( text_ptr block_buff, int fill_char, int block_len )
{
   assert( block_len >= 0 );
   assert( block_len < BUFF_SIZE );
   assert( block_buff != NULL );

   memset( block_buff, fill_char, block_len );
}


/*
 * Name:    block_pattern
 * Class:   helper function
 * Purpose: fill the block buffer with a pattern
 * Author:  Jason Hood
 * Date:    March 5, 2003
 * Passed:  block_buff: local buffer for moves
 *          block_len:  number of columns in block
 *          pat_len:    length of the pattern
 *          pattern:    the pattern
 *          start:      the position to start the pattern
 * Returns: the new start position
 * Notes:   fills as much of the block as possible with the pattern, with
 *           the remainder continuing onto the next line.
 *          assumes block_len and pat_len are both greater than zero.
 *          assumes start is smaller than pat_len.
 */
int  block_pattern( text_ptr block_buff, int block_len,
                    int pat_len, text_ptr pattern, int start )
{
text_ptr pat;

   pat = pattern + start;
   start = pat_len - start;
   do {
      do {
         *block_buff++ = *pat++;
         --block_len; --start;
      } while (start != 0 && block_len != 0);
      if (start == 0) {
         start = pat_len;
         pat = pattern;
      }
   } while (block_len != 0);

   return( pat_len - start );
}


/*
 * Name:    number_block_buff
 * Class:   helper function
 * Purpose: put a number into the block buffer
 * Date:    June 5, 1991
 * Passed:  block_buff: local buffer for moves
 *          block_len:  number of columns in block
 *          block_num:  long number to fill block
 *          block_base: the radix of the number
 * Notes:   Fill block_buffer for block_len characters with number.
 *          This function is used only for BOX blocks.
 *
 * jmh 031116: added zero fill option.
 * jmh 031119: added base option.
 */
void number_block_buff( text_ptr block_buff, int block_len,
                        long block_num, int block_base )
{
int  len;               /* length of number buffer */
text_t temp[32+2];
text_t *p;

   assert( block_len >= 0 );
   assert( block_len < BUFF_SIZE );

   block_fill( block_buff, (CB_Zero) ? '0' : ' ', block_len );
   if (CB_Zero && !CB_Left && block_num < 0 && block_base == 10) {
      block_num = -block_num;
      *block_buff = '-';
      ++block_buff;
      --block_len;
   }
   len = strlen( my_ltoa( block_num, (char *)temp, block_base ) );
   p = temp;
   if (CB_Left) {
      if (len > block_len)
         len = block_len;
   } else {
      if (len > block_len) {
         p  += len - block_len;
         len = block_len;
      }
      block_buff += block_len - len;
   }
   do
      *block_buff++ = *p++;
   while (--len != 0);
}


/*
 * Name:    border_block_buff
 * Class:   helper function
 * Purpose: put the appropriate border characters into the buffer
 * Author:  Jason Hood
 * Date:    November 16, 2003
 * Passed:  block_buff: local buffer for moves
 *          block_len:  number of columns in block
 *          br:         beginning line number
 *          er:         ending line number
 *          li:         current line number
 *          style:      border characters (BorderBlock)
 *          style_len:  border lengths (BorderBlockEx)
 * Notes:   The buffer should have been copied from the block prior to entry.
 */
static void border_block_buff( text_ptr block_buff, int block_len,
                               long br, long er, long li,
                               text_ptr style, int *style_len )
{
int  lc, ln, rc;
int  lm, rm;

   if (g_status.command == BorderBlock) {
      if (li == br) {
         block_buff[0] = style[0];
         block_buff[block_len-1] = style[1];
         memset( block_buff+1, style[6], block_len-2 );
      } else if (li == er) {
         block_buff[0] = style[2];
         block_buff[block_len-1] = style[3];
         memset( block_buff+1, style[7], block_len-2 );
      } else {
         block_buff[0] = style[4];
         block_buff[block_len-1] = style[5];
      }
   } else /* g_status.command == BorderBlockEx */ {
      if ((li == br  &&
           (style_len[0] || style_len[1] || style_len[2]))  ||
          (li == er  &&
           (style_len[5] || style_len[6] || style_len[7]))) {
         if (li == br) {
            lc = 0;
            ln = 1;
            rc = 2;
         } else {
            lc = 5;
            ln = 6;
            rc = 7;
         }
         lm = style_len[lc];
         rm = style_len[rc];
         if (lm)
            memcpy( block_buff, get_dlg_text( EF_Border( lc ) ), lm );
         if (rm)
            memcpy( block_buff + block_len - rm,
                    get_dlg_text( EF_Border( rc ) ), rm );
         if (style_len[ln]) {
            if (lm == 0)
               lm = style_len[3];
            if (rm == 0)
               rm = style_len[4];
            if (block_len - lm - rm > 0)
               block_pattern( block_buff + lm, block_len - lm - rm,
                              style_len[ln],
                              (text_ptr)get_dlg_text( EF_Border( ln ) ), 0 );
         }
      } else {
         if (style_len[3] >= block_len)
            memcpy( block_buff, get_dlg_text( EF_Border( 3 )), block_len );
         else {
            if (style_len[4] >= block_len)
               memcpy( block_buff, get_dlg_text( EF_Border( 4 )) +
                          style_len[4] - block_len, block_len );
            else {
               memcpy( block_buff, get_dlg_text( EF_Border(3) ), style_len[3] );
               memcpy( block_buff + block_len - style_len[4],
                       get_dlg_text( EF_Border( 4 )), style_len[4] );
            }
         }
      }
   }
}


/*
 * Name:    restore_cursors
 * Class:   helper function
 * Purpose: a file has been modified - must restore all cursor pointers
 * Date:    June 5, 1991
 * Passed:  file:  pointer to file with changes
 * Notes:   Go through the window list and adjust the cursor pointers
 *          as needed.
 *
 * jmh 010521: fix bug with TOF's EOF length for bin_offset.
 */
void restore_cursors( file_infos *file )
{
register TDE_WIN *window;
long n;

   assert( file != NULL );

   window = g_status.window_list;
   while (window != NULL) {
      if (window->file_info == file) {
         n = window->rline;
         if (n < 0L)
            n = 0L;
         else if (n > file->length)
            n = file->length;
         first_line( window );
         move_to_line( window, n, TRUE );
         check_cline( window, window->cline );
         if (window->visible)
            show_size( window );
      }
      window = window->next;
   }
}


/*
 * Name:    delete_box_block
 * Class:   helper function
 * Purpose: delete the marked text
 * Date:    June 5, 1991
 * Passed:  s_w:    source window
 *          source: pointer to line with block to delete
 *          bc:     beginning column of block - BOX mode only
 *          add:    number of characters in block to delete
 * Notes:   Used only for BOX blocks.  Delete the block.
 */
int  delete_box_block( TDE_WIN *s_w, line_list_ptr source, int bc, int add )
{
text_ptr s;
int  number;

   assert( s_w != NULL );
   assert( source != NULL );
   assert( bc >= 0 );
   assert( bc < MAX_LINE_LENGTH );
   assert( add >= 0 );
   assert( add < MAX_LINE_LENGTH );

   copy_line( source, s_w, TRUE );
   number = g_status.line_buff_len - bc;
   s = g_status.line_buff + bc + add;

   assert( number >= 0 );
   assert( number < MAX_LINE_LENGTH );
   assert( bc + add >= 0 );
   assert( bc + add < MAX_LINE_LENGTH );
   assert( add <= g_status.line_buff_len );

   memmove( s - add, s, number );
   g_status.line_buff_len -= add;
   return( un_copy_line( source, s_w, TRUE, TRUE ) );
}


/*
 * Name:    check_block
 * Class:   helper function
 * Purpose: To check that the block is still valid.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to pointer to window containing block
 * Notes:   After some editing, the marked block may not be valid. For example,
 *          deleting all the lines in a block in another window.  We don't
 *          need to keep up with the block text pointers while doing normal
 *          editing; however, we need to refresh them before doing block stuff.
 *          If window is not NULL, set it to a window containing the block.
 *
 * jmh 980525: only unmark if a block is marked.
 * jmh 010520: added window parameter
 */
void check_block( TDE_WIN **window )
{
register file_infos *file;
TDE_WIN filler;

   file = g_status.marked_file;
   if (file == NULL || file->block_br > file->length) {
      if (g_status.marked == TRUE)
         unmark_block( &filler );
   } else {
      if (file->block_er > file->length) {
         file->block_er = file->length;
         if (file->block_type == STREAM && file->block_er == file->block_br &&
             file->block_ec == -1) {
            file->block_type = LINE;
            file->block_ec = 0;
         }
      }
      find_begblock( file );
      find_endblock( file );
      if (window != NULL) {
         *window = g_status.window_list;
         while ((*window)->file_info != file)
            *window = (*window)->next;
      }
   }
}


/*
 * Name:    find_begblock
 * Class:   helper editor function
 * Purpose: find the beginning line in file with marked block
 * Date:    June 5, 1991
 * Passed:  file: file containing marked block
 * Notes:   file->block_start contains starting line of marked block.
 */
void find_begblock( file_infos *file )
{
line_list_ptr ll;
long li;           /* line counter (long i) */

   assert( file != NULL );
   assert( file->length > 0 );

   ll = file->line_list->next;
   for (li=1; li<file->block_br && ll->next != NULL; li++)
      ll = ll->next;

   file->block_start = ll;
}


/*
 * Name:    find_endblock
 * Class:   helper function
 * Purpose: find the ending line in file with marked block
 * Date:    June 5, 1991
 * Passed:  file: file containing marked block
 * Notes:   If in LINE mode, file->block_end is set to end of line of last
 *          line in block.  If in BOX mode, file->block_end is set to
 *          beginning of last line in marked block.  If the search for the
 *          ending line of the marked block goes past the eof, set the
 *          ending line of the block to the last line in the file.
 */
void find_endblock( file_infos *file )
{
line_list_ptr ll; /* start from beginning of file and go to end */
long i;           /* line counter */
register file_infos *fp;

   assert( file != NULL );
   assert( file->block_start != NULL );

   fp = file;
   ll = fp->block_start;
   if (ll != NULL) {
      for (i=fp->block_br;  i < fp->block_er && ll->next != NULL; i++)
         ll = ll->next;
      if (ll != NULL)
         fp->block_end = ll;
      else {

         /*
          * last line in marked block is NULL.  if LINE block, set end to
          * last character in the file.  if STREAM or BOX block, set end to
          * start of last line in file.  ending row, or er, is then set to
          * file length.
          */
         fp->block_end = fp->line_list_end->prev;
         fp->block_er = fp->length;
      }
   }
}


/*
 * Name:    block_write
 * Class:   primary editor function
 * Purpose: To write the currently marked block to a disk file.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   If the file already exists, the user gets to choose whether
 *           to overwrite or append.
 *
 * jmh 020630: allow escape to abort the overwrite/append prompt;
 *             return the correct code (didn't change rc after get_fattr()).
 * jmh 021020: allow appending to read-only files.
 * jmh 040716: handle SaveTo here, to allow appending.
 */
int  block_write( TDE_WIN *window )
{
int  prompt_line;
int  rc;
file_infos *file;
int  block_type;
long br, er;
int  type;
int  append;
DISPLAY_BUFF;

   /*
    * make sure block is marked OK
    */
   if (un_copy_line( window->ll, window, TRUE, TRUE ) == ERROR)
      return( ERROR );

   if (g_status.command == SaveTo) {
      file = window->file_info;
      block_type = NOTMARKED;
      br   = 1;
      er   = file->length;
      type = 1;
   } else {
      check_block( NULL );
      if (!g_status.marked)
         return( ERROR );
      file = g_status.marked_file;
      block_type = file->block_type;
      br   = file->block_br;
      er   = file->block_er;
      type = 0;
   }

   prompt_line = window->bottom_line;

   /*
    * find out which file to write to
    */

   *g_status.rw_name = '\0';
   if (get_name(block6[0][type], prompt_line, g_status.rw_name, &h_file) > 0) {
      /*
       * if the file exists, find out whether to overwrite or append
       */
      rc = file_exists( g_status.rw_name );
      if (rc == SUBDIRECTORY) {
         rc = ERROR;
         /*
          * invalid path or filename
          */
         error( WARNING, prompt_line, win8 );
      } else if (rc == OK || rc == READ_ONLY) {
         /*
          * file exists. overwrite or append?
          */
         append = get_response( block7, prompt_line, R_PROMPT | R_ABORT,
                                2, L_OVERWRITE, 0, L_APPEND, 1 );
         if (append == ERROR  ||
             change_mode( g_status.rw_name, prompt_line ) == ERROR)
            rc = ERROR;
      } else {
         append = 0;
         rc = OK;
      }

      if (rc != ERROR) {
         SAVE_LINE( prompt_line );
         /*
          * writing/appending block/file to
          */
         combine_strings(line_out,block6[1+append][type],g_status.rw_name,"'");
         s_output( line_out, prompt_line, 0, Color( Message ) );
         refresh( );

         if (append)
            rc = hw_append( g_status.rw_name, file, br, er, block_type );
         else
            rc = hw_save(   g_status.rw_name, file, br, er, block_type );

         if (rc == ERROR) {
            /*
             * could not write/append block/file
             */
            error( WARNING, prompt_line, block6[3+append][type] );
         }
         RESTORE_LINE( prompt_line );
      }
   } else
      rc = ERROR;
   return( rc );
}


/*
 * Name:     block_print
 * Class:    primary editor function
 * Purpose:  Print an entire file or the currently marked block.
 * Date:     June 5, 1991
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen
 * Passed:   window:  pointer to current window
 * Notes:    With the added Critical Error Handler routine, let's fflush
 *           the print buffer first.
 *
 * jmh 980528: modified the block line testing code.
 */
int  block_print( TDE_WIN *window )
{
int  col;
int  prompt_line;
line_list_ptr block_start;   /* start of block in file */
file_infos *file;
int  block_type;
text_ptr p;
int  len;
int  bc;
int  ec;
int  last_c;
long lbegin;
long lend;
long l;
int  color;
int  rc;
DISPLAY_BUFF;

   color = Color( Message );
   if (un_copy_line( window->ll, window, TRUE, TRUE ) == ERROR)
      return( ERROR );
   rc = OK;

   prompt_line = window->bottom_line;
   SAVE_LINE( prompt_line );
   /*
    * print entire file or just marked block?
    */
   col = get_response( block13, prompt_line, R_ALL, 2, L_FILE,  L_FILE,
                                                       L_BLOCK, L_BLOCK );
   if (col == ERROR)
      rc = ERROR;

   if (rc == OK) {
      /*
       * if everything is everything, flush the printer before we start
       *   printing.  then, check the critical error flag after the flush.
       */
      fflush( my_stdprn );
      if (CEH_ERROR)
         rc = ERROR;
   }

   if (rc != ERROR) {
      file = window->file_info;
      block_type  = NOTMARKED;
      lend =  l   = file->length;
      block_start = file->line_list->next;

      if (col == L_BLOCK) {
         check_block( &window );
         if (g_status.marked == TRUE) {
            file        = g_status.marked_file;
            block_start = file->block_start;
            block_type  = file->block_type;
            lend =   l  = file->block_er + 1l - file->block_br;
         } else
            rc = ERROR;
      }

      if (rc != ERROR) {
         eol_clear( 0, prompt_line, color );
         /*
          * printing line   of    press control-break to cancel.
          */
         s_output( block14, prompt_line, 0, color );
         n_output( l, 0, BLOCK14_TOTAL_SLOT, prompt_line, color );
         xygoto( BLOCK14_LINE_SLOT, prompt_line );
         refresh( );

         /*
          * bc and ec are used only in BOX and STREAM blocks.
          * last_c is last column in a BOX block
          */
         bc = file->block_bc;
         ec = file->block_ec;
         last_c = ec + 1 - bc;

         p = g_status.line_buff;
         lbegin = 1;
         for (col=OK; l>0 && col == OK && !g_status.control_break; l--) {
            n_output( lbegin, 0, BLOCK14_LINE_SLOT, prompt_line, color );
            g_status.copied = FALSE;
            if (block_type == BOX  ||
                (block_type == STREAM && lend == 1 && ec != -1)) {
               load_box_buff( p, block_start, bc, ec, ' ',
                              file->inflate_tabs, file->ptab_size );
               len = last_c;
            } else {
               copy_line( block_start, window, TRUE );
               len = g_status.line_buff_len;
               if (block_type == STREAM) {
                  if (lbegin == 1) {
                     if (bc > len)
                        len = 0;
                     else {
                        len -= bc;

                        assert( len >= 0 );
                        assert( len < MAX_LINE_LENGTH );

                        my_memcpy( p, p + bc, len );
                     }
                  } else if (l == 1L && ec != -1) {
                     if (len > ec + 1)
                        len = ec + 1;
                  }
               }
            }

            assert( len >= 0 );
            assert( len < MAX_LINE_LENGTH );

            *(p+len) = '\r';
            ++len;
            *(p+len) = '\n';
            ++len;
            if (fwrite( p, sizeof( char ), len, my_stdprn ) < (unsigned)len ||
                CEH_ERROR)
               col = ERROR;
            block_start = block_start->next;
            ++lbegin;
         }
         g_status.copied = FALSE;
         if (!CEH_ERROR)
            fflush( my_stdprn );
         else
            rc = ERROR;
      }
   }
   g_status.copied = FALSE;
   RESTORE_LINE( prompt_line );
   return( rc );
}


/*
 * Name:    get_block_fill_char
 * Class:   helper function
 * Purpose: get the character to fill marked block.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 *          c: address of character to fill block or length of pattern
 *          pat:  pattern
 * jmh 980723: recognize macros.
 * jmh 030305: recognize FillBlockPattern and FillBlockDown.
 */
int  get_block_fill_char( TDE_WIN *window, int *c, text_ptr pat )
{
long key;
int  prompt_line;
int  rc;

   rc = OK;
   if (g_status.command != FillBlockDown) {
      prompt_line = window->bottom_line;
      if (g_status.command == FillBlock) {
         /*
          * enter character to fill block (esc to exit)
          */
         key = prompt_key( block15, prompt_line );
         if (key < 256)
            *c = (int)key;
         else
            rc = ERROR;
      } else { /* g_status.command == FillBlockPattern */
         /*
          * enter pattern to fill block (esc to exit)
          */
         strcpy( (char *)pat, h_fill.prev->str );
         *c = get_name( block15a, prompt_line, (char *)pat, &h_fill );
         if (*c <= 0)
            rc = ERROR;
      }
   }
   return( rc );
}


/*
 * Name:    get_block_numbers
 * Class:   helper function
 * Purpose: get the starting number and increment
 * Date:    June 5, 1991
 * Passed:  block_num:  address of number to start numbering
 *          block_inc:  address of number to add to block_num
 *          block_base: address of radix of number
 */
int  get_block_numbers( long *block_num, long *block_inc, int *block_base )
{
int  rc;
char *num;

   if (*get_dlg_text( EF_Step ) == '\0')
      set_dlg_num( EF_Step, 1 );
   if (*get_dlg_text( EF_Base ) == '\0')
      set_dlg_num( EF_Base, 10 );

   rc = do_dialog( number_dialog, NULL );
   if (rc == OK) {
      *block_base = (int)get_dlg_num( EF_Base );
      if (*block_base < 2 || *block_base > 36)
         *block_base = 10;
      num = get_dlg_text( EF_Start );
      *block_num = strtol( num, NULL, *block_base );
      if (*block_num == 0  &&  *num == '\0')
         rc = ERROR;
      else
         *block_inc = strtol( get_dlg_text( EF_Step ), NULL, *block_base );
   }
   return( rc );
}


/*
 * Name:    block_trim_trailing
 * Class:   primary editor function
 * Purpose: Trim trailing space in a LINE block.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   Use copy_line and un_copy_line to do the work.
 *
 * jmh 060827: only copy lines that have a trailing space, per David Hughes.
 */
int  block_trim_trailing( TDE_WIN *window )
{
int  prompt_line;
int  rc;
line_list_ptr p;             /* pointer to block line */
file_infos *file;
TDE_WIN *sw = NULL, s_w;
long er;
int  trailing;               /* save trailing setting */

   /*
    * make sure block is marked OK and that this is a LINE block
    */
   prompt_line = window->bottom_line;
   rc = un_copy_line( window->ll, window, TRUE, TRUE );
   check_block( &sw );
   if (g_status.marked != TRUE || g_status.marked_file->read_only)
     rc = ERROR;
   if (rc != ERROR) {

      file = g_status.marked_file;
      if (file->block_type != LINE) {
         /*
          * can only trim trailing space in line blocks
          */
         error( WARNING, prompt_line, block21 );
         return( ERROR );
      }
      trailing = mode.trailing;
      mode.trailing = TRUE;

      /*
       * initialize everything
       */
      if (mode.do_backups == TRUE) {
         file->modified = TRUE;
         rc = backup_file( sw );
      }
      dup_window_info( &s_w, sw );

      /*
       * set window to invisible so the un_copy_line function will
       * not display the lines while trimming.
       */
      s_w.visible = FALSE;

      p  = file->block_start;
      er = file->block_er;
      s_w.rline = file->block_br;
      for (; rc == OK && s_w.rline <= er  &&  !g_status.control_break;
           s_w.rline++) {
         /*
          * use the line buffer to trim space.
          */
         if (p->len) {
            text_t c = p->line[p->len-1];
            if (c == ' ' || (c == '\t' && file->inflate_tabs)) {
               copy_line( p, &s_w, FALSE );
               rc = un_copy_line( p, &s_w, TRUE, FALSE );
            }
         }
         p = p->next;
      }

      /*
       * IMPORTANT:  we need to reset the copied flag because the cursor may
       * not necessarily be on the last line of the block.
       */
      g_status.copied = FALSE;
      file->dirty = GLOBAL;
      mode.trailing = trailing;
      show_avail_mem( );
   }
   return( rc );
}


/*
 * Name:    block_email_reply
 * Class:   primary editor function
 * Purpose: insert the standard replay character '>' at beginning of line
 * Date:    June 5, 1992
 * Passed:  window:  pointer to current window
 * Notes:   it is customary to prepend "> " to the initial text and
 *             ">" to replies to replies to etc...
 */
int  block_email_reply( TDE_WIN *window )
{
int  prompt_line;
int  add;
int  len;
int  rc;
text_ptr source;        /* source for block move to make room for c */
text_ptr dest;          /* destination for block move */
line_list_ptr p;        /* pointer to block line */
file_infos *file;
TDE_WIN *sw = NULL, s_w;
long er;

   /*
    * make sure block is marked OK and that this is a LINE block
    */
   prompt_line = window->bottom_line;
   rc = un_copy_line( window->ll, window, TRUE, TRUE );
   check_block( &sw );
   if (rc != ERROR  &&  g_status.marked == TRUE) {
      file = g_status.marked_file;
      if (file->read_only)
         return( ERROR );
      if (file->block_type != LINE) {
         /*
          * can only reply line blocks
          */
         error( WARNING, prompt_line, block25 );
         return( ERROR );
      }

      if (mode.do_backups == TRUE) {
         file->modified = TRUE;
         rc = backup_file( sw );
      }

      /*
       * use a local window structure to do the dirty work.  initialize
       *   the local window structure to the beginning of the marked
       *   block.
       */
      dup_window_info( &s_w, sw );

      /*
       * set s_w to invisible so the un_copy_line function will
       * not display the lines while doing block stuff.
       */
      s_w.visible = FALSE;
      s_w.rline = file->block_br;
      p  = file->block_start;
      er = file->block_er;

      /*
       * for each line in the marked block, prepend the reply character(s)
       */
      for (; rc == OK  &&  s_w.rline <= er  &&  !g_status.control_break;
                                                             s_w.rline++) {

         /*
          * put the line in the g_status.line_buff.  use add to count the
          *   number of characters to insert at the beginning of a line.
          *   the original reply uses "> ", while replies to replies use ">".
          */
         copy_line( p, &s_w, FALSE );
         source = g_status.line_buff;
         len = g_status.line_buff_len;
         if (len > 0  &&  *source == '>')
            add = 1;
         else
            add = 2;

         /*
          * see if the line has room to add the ">" character.  if there is
          *   room, move everything down to make room for the
          *   reply character(s).
          */
         if (len + add < MAX_LINE_LENGTH) {
            dest = source + add;

            assert( len >= 0 );
            assert( len < MAX_LINE_LENGTH );
            assert( len + add < MAX_LINE_LENGTH );

            memmove( dest, source, len );
            *source = '>';
            if (add > 1)
              *(source+1) = ' ';
            g_status.line_buff_len = len + add;
            rc = un_copy_line( p, &s_w, TRUE, FALSE );
         }
         p = p->next;
         g_status.copied = FALSE;
      }

      /*
       * IMPORTANT:  we need to reset the copied flag because the cursor may
       * not necessarily be on the last line of the block.
       */
      g_status.copied = FALSE;
      syntax_check_block( file->block_br, er, file->block_start, file->syntax );
      file->dirty = GLOBAL;
      show_avail_mem( );
   }
   return( OK );
}


/*
 * Name:     block_convert_case
 * Class:    primary editor function
 * Purpose:  convert characters to lower case, upper case, strip hi bits,
 *           or e-mail functions
 * Date:     June 5, 1991
 * Modified: August 10, 1997, Jason Hood - correctly handles real tabs.
 *                             corrected STREAM beginning/ending lines.
 *           November 14, Jason Hood - corrected one-line STREAM.
 * Passed:   window:  pointer to current window
 *
 * jmh 990915: added BlockInvertCase conversion.
 * jmh 010624: added BlockCapitalise;
 *             modify bc and ec rather than detabbing;
 *             tweaked all the functions.
 * jmh 011121: made count signed (corrects block starting beyond EOL bug).
 * jmh 020826: fixed box bug when ec was at EOL.
 */
int  block_convert_case( TDE_WIN *window )
{
int  len;
int  block_type;
line_list_ptr begin;
register file_infos *file;
TDE_WIN *sw = NULL;
long number;
long er;
int  count;
int  bc, ec;
int  block_len;
int  tabs, ptab;
int  rc;
void (*char_func)( text_ptr, size_t );

   /*
    * make sure block is marked OK
    */
   if (un_copy_line( window->ll, window, TRUE, TRUE ) == ERROR)
      return( ERROR );
   rc = OK;
   check_block( &sw );
   if (g_status.marked == TRUE && !g_status.marked_file->read_only) {

      /*
       * set char_func() to the required block function
       */
      switch (g_status.command) {
         case BlockUpperCase  :
            char_func = upper_case;
            break;
         case BlockLowerCase  :
            char_func = lower_case;
            break;
         case BlockInvertCase :
            char_func = invert_case;
            break;
         case BlockCapitalise :
            char_func = capitalise;
            break;
         case BlockRot13      :
            char_func = rot13;
            break;
         case BlockFixUUE     :
            char_func = fix_uue;
            break;
         case BlockStripHiBit :
            char_func = strip_hi;
            break;
         default :
            return( ERROR );
      }

      file  = g_status.marked_file;
      file->modified = TRUE;
      if (mode.do_backups == TRUE)
         rc = backup_file( sw );

      if (rc == OK) {
         tabs = file->inflate_tabs;
         ptab = file->ptab_size;

         block_type = file->block_type;
         ec = file->block_ec;
         if (block_type == STREAM && file->block_br == file->block_er && ec!=-1)
            block_type = BOX;
         block_len = ec + 1 - file->block_bc;

         begin = file->block_start;
         er = file->block_er;
         for (number = file->block_br; number <= er; number++) {
            begin->type |= DIRTY;
            len = begin->len;
            bc = 0;
            if (block_type == LINE ||
                (block_type == STREAM && number > file->block_br &&
                 (number < er || (number == er && ec == -1)))) {
               count = len;
            } else {
               if (block_type == BOX) {
                  bc = file->block_bc;
                  if (tabs) {
                     bc = entab_adjust_rcol( begin->line, len, bc, ptab );
                     ec = entab_adjust_rcol( begin->line, len, file->block_ec,
                                             ptab );
                     block_len = ec - bc + 1;
                  }
                  count =  len > ec ? block_len : len - bc;
               } else /* (block_type == STREAM) */ {
                  if (number == file->block_br) {
                     bc = file->block_bc;
                     if (tabs)
                        bc = entab_adjust_rcol( begin->line, len, bc, ptab );
                     count = len - bc;
                  } else /* (number == er) */ {
                     if (tabs)
                        ec = entab_adjust_rcol( begin->line, len, ec, ptab );
                     count = (ec < len) ? ec+1 : len;
                  }
               }
            }
            if (count > 0) {

               assert( count < MAX_LINE_LENGTH );
               assert( bc >= 0 );
               assert( bc < MAX_LINE_LENGTH );

               (*char_func)( begin->line+bc, count );
            }
            begin = begin->next;
         }

         /*
          * IMPORTANT:  we need to reset the copied flag because the cursor may
          * not necessarily be on the last line of the block.
          */
         g_status.copied = FALSE;
         file->dirty = GLOBAL;
      }
   } else
      rc = ERROR;
   return( rc );
}


/*
 * Name:    upper_case
 * Purpose: To convert all lower case characters to upper characters
 * Date:    June 5, 1991
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen
 * Passed:  s:     the starting point
 *          count: number of characters to convert
 * Returns: none
 * Notes:   upper_lower[] is defined in bottom of prompts.c.  modify as
 *           needed for your favorite language.
 */
void upper_case( text_ptr s, size_t count )
{
   do {
      if (bj_islower( *s ))             /* if lowercase letter */
         *s = upper_lower[*s];          /*    then upcase it   */
      s++;
   } while (--count != 0);
}


/*
 * Name:    lower_case
 * Purpose: To convert all upper case characters to lower characters
 * Date:    June 5, 1991
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen
 * Passed:  s:     the starting point
 *          count: number of characters to convert
 * Returns: none
 * Notes:   upper_lower[] is defined in bottom of prompts.c.  modify as
 *           needed for your favorite language.
 */
void lower_case( text_ptr s, size_t count )
{
   do {
      if (bj_isupper( *s ))             /* if uppercase letter */
         *s = upper_lower[*s];          /*    then lowcase it  */
      s++;
   } while (--count != 0);
}


/*
 * Name:    invert_case
 * Purpose: To convert all lower case characters to upper characters and
 *           all upper case characters to lower
 * Author:  Jason Hood
 * Date:    September 15, 1999
 * Passed:  s:     the starting point
 *          count: number of characters to convert
 * Returns: none
 * Notes:   upper_lower[] is defined in bottom of prompts.c.  modify as
 *           needed for your favorite language.
 */
void invert_case( text_ptr s, size_t count )
{
   do {
      if (bj_isalpha( *s ))
         *s = upper_lower[*s];
      s++;
   } while (--count != 0);
}


/*
 * Name:    capitalise
 * Purpose: To convert first letter of each word to upper case and all other
 *           letters to lower case
 * Author:  Jason Hood
 * Date:    June 24, 2001
 * Passed:  s:     the starting point
 *          count: number of characters to convert
 * Returns: none
 * Notes:   upper_lower[] is defined in bottom of prompts.c.  modify as
 *           needed for your favorite language.
 */
void capitalise( text_ptr s, size_t count )
{
   do {
      while (count != 0  &&  !bj_isalpha( *s )) {
         s++;
         count--;
      }
      if (count == 0)
         break;
      if (bj_islower( *s ))
         *s = upper_lower[*s];
      while (--count != 0  &&  bj_isalpha( *++s )) {
         if (bj_isupper( *s ))
            *s = upper_lower[*s];
      }
   } while (count != 0);
}


/*
 * Name:    rot13
 * Purpose: To rotate all alphabet characters by 13
 * Date:    June 5, 1991
 * Passed:  s:     the starting point
 *          count: number of characters to convert
 * Returns: none
 * Notes:   simple rot13
 *          i really don't know how to handle rot13 for alphabets
 *           other than English.
 */
void rot13( text_ptr s, size_t count )
{
register text_t c;

   do {
      c = *s;
      if (c >= 'a'  &&  c <= 'm')
         c += 13;
      else if (c >= 'n'  &&  c <= 'z')
         c -= 13;
      else if (c >= 'A'  &&  c <= 'M')
         c += 13;
      else if (c >= 'N'  &&  c <= 'Z')
         c -= 13;
      *s++ = c;
   } while (--count != 0);
}


/*
 * Name:    fix_uue
 * Purpose: To fix EBCDIC ==> ASCII translation problem
 * Date:    June 5, 1991
 * Passed:  s:     the starting point
 *          count: number of characters to convert
 * Returns: none
 * Notes:   to fix the EBCDIC to ASCII translation problem, three characters
 *           need to be changed,  0x5d -> 0x7c, 0xd5 -> 0x5b, 0xe5 -> 0x5d
 */
void fix_uue( text_ptr s, size_t count )
{
   do {
      switch (*s) {
         case 0x5d :
            *s = 0x7c;
            break;
         case 0xd5 :
            *s = 0x5b;
            break;
         case 0xe5 :
            *s = 0x5d;
            break;
         default :
            break;
      }
      s++;
   } while (--count != 0);
}


/*
 * Name:    strip_hi
 * Purpose: To strip bit 7 from characters
 * Date:    June 5, 1991
 * Passed:  s:     the starting point, which should be normalized to a segment
 *          count: number of characters to strip (size_t)
 *                 count should not be greater than MAX_LINE_LENGTH
 * Returns: none
 * Notes:   this function is useful on WordStar files.  to make a WordStar
 *           file readable, the hi bits need to be anded off.
 *          incidentally, soft CRs and LFs may be converted to hard CRs
 *           and LFs.  this function makes no attempt to split lines
 *           when soft returns are converted to hard returns.
 */
void strip_hi( text_ptr s, size_t count )
{
   do {
      *s++ &= 0x7f;
   } while (--count != 0);
}


/*
 * Name:    get_block_border_style
 * Class:   helper function
 * Purpose: ask for the characters to draw the border
 * Author:  Jason Hood
 * Date:    July 31, 1998
 * Passed:  window:    pointer to current window
 *          style:     pointer to buffer (BorderBlock)
 *          style_len: pointer to lengths buffer (BorderBlockEx)
 * Notes:   The style depends on length:
 *            no characters - use current graphic set (not Linux)
 *            one           - entire border is one character
 *            two           - first char is left/right edge, second top/bottom
 *            three         - corner, left/right, top/bottom
 *            four          - left, right, top, bottom (corners are left/right)
 *            five          - top-left/right, bottom-left/right and the edges
 *            six           - corners, left/right and top/bottom edges
 *            eight         - corners, left, right, top and bottom edges
 *          Anything else is an error.
 *          If graphic characters are off, turn them on.
 */
int  get_block_border_style( TDE_WIN *window, text_ptr style, int *style_len )
{
char answer[MAX_COLS+2];
int  len;
char *gc;
int  old_gc;
int  prompt_line;
int  rc;
int  j;

   rc = OK;
   old_gc = mode.graphics;
   if (old_gc < 0)
      toggle_graphic_chars( NULL );

   if (g_status.command == BorderBlock) {
      prompt_line = window->bottom_line;
      strcpy( answer, h_border.prev->str );
      /*
       * Box style:
       */
      rc = get_name( block27, prompt_line, answer, &h_border );
      if (rc != ERROR) {
         len = rc;
         if (len == 7 || len > 8) {
            error( WARNING, prompt_line, block28 ); /* invalid style */
            rc = ERROR;
         } else {
            switch( len ) {
               case 0:
                  len = abs( mode.graphics ) - 1;
                  gc  = (char*)graphic_char[len];
                  if (len == 5) {
                     memset( style, gc[INTERSECTION], 6 );
                     style[6] = gc[TOP_T];
                     style[7] = gc[BOTTOM_T];
                  } else {
                     style[0] = gc[CORNER_LEFT_UP];
                     style[1] = gc[CORNER_RIGHT_UP];
                     style[2] = gc[CORNER_LEFT_DOWN];
                     style[3] = gc[CORNER_RIGHT_DOWN];
                     style[4] =
                     style[5] = gc[VERTICAL_LINE];
                     style[6] =
                     style[7] = gc[HORIZONTAL_LINE];
                  }
                  break;
               case 1:
                  memset( style, *answer, 8 );
                  break;
               case 2:
                  memset( style, *answer, 6 );
                  style[6] = style[7] = answer[1];
                  break;
               case 3:
                  memset( style, *answer, 4 );
                  style[4] = style[5] = answer[1];
                  style[6] = style[7] = answer[2];
                  break;
               case 4:
                  style[0] = style[2] = style[4] = answer[0];
                  style[1] = style[3] = style[5] = answer[1];
                  style[6] = answer[2];
                  style[7] = answer[3];
                  break;
               case 5:
                  memcpy( style, answer, 4 );
                  memset( style+4, answer[4], 4 );
                  break;
               case 6:
                  memcpy( style, answer, 4 );
                  style[4] = style[5] = answer[4];
                  style[6] = style[7] = answer[5];
                  break;
               case 8:
                  memcpy( style, answer, 8 );
                  break;
            }
         }
      }

   } else /* (g_status.command == BorderBlockEx) */ {
      rc = do_dialog( border_dialog, NULL );
      if (rc == OK) {
         for (j = 0; j < 8; ++j)
            style_len[j] = strlen( get_dlg_text( EF_Border( j ) ) );
      }
   }

   if (old_gc < 0 && mode.graphics > 0)
      toggle_graphic_chars( NULL );

   return( rc == ERROR ? ERROR : OK );
}


/*
 * Name:    justify_line_block
 * Purpose: To left/right/center justify each line in the block
 * Author:  Jason Hood
 * Date:    August 11, 1998
 * Passed:  window:       pointer to source window
 *          block_start:  pointer to first node in block
 *          br:           beginning line number in marked block
 *          er:           ending line number in marked block
 *          rc:           return code
 */
void justify_line_block( TDE_WIN *window, line_list_ptr block_start,
                         long br, long er, int *rc )
{
TDE_WIN w;
int  (*justify)( TDE_WIN * );

   dup_window_info( &w, window );
   w.visible = FALSE;
   w.ll = block_start;
   justify = (g_status.command == BlockLeftJustify)  ? flush_left  :
             (g_status.command == BlockRightJustify) ? flush_right :
                                                       flush_center;

   for (w.rline = br; w.rline <= er && *rc == OK && !g_status.control_break;
        ++w.rline) {
      *rc = justify( &w );
      if (*rc == OK)
         *rc = un_copy_line( w.ll, &w, TRUE, TRUE );
      w.ll = w.ll->next;
   }
}


/*
 * Name:    block_indent
 * Purpose: to indent and undent line and box blocks
 * Author:  Jason Hood
 * Date:    August 11, 1998
 * Passed:  window:  pointer to current window
 * Notes:   Block[IU]ndentN asks for a number and adds/removes that many spaces
 *           from the beginning of the block; Block[IU]ndent performs a tab /
 *           backspace on the first line of the block and carries that change
 *           through the remaining lines.
 *          If subsequent lines start before the first line, those characters
 *           will be removed. eg:
 *
 *                 statement1;
 *                 statement2;
 *              }
 *
 *           Undenting these three lines will end up deleting the brace.
 *          If there's no room for the indentation, simply ignore that line.
 *          Place lines that have something deleted in the undo buffer.
 *
 * jmh 981205: Corrected BlockUndent and tab bugs.
 * jmh 991026: Indent 0 spaces to line up all lines below the first line.
 */
int  block_indent( TDE_WIN *window )
{
TDE_WIN *sw = NULL, s_w;
file_infos *file;
int  type;
int  indent;
line_list_ptr p;
line_list_ptr temp_ll;
text_ptr t;
text_ptr s;
int  col;
static long level = 2;          /* remember previous indentation level */
int  add = 0;
int  rem;
int  bc;
long er;
long li;
int  rc;
int  tabs;
int  ltab_size;
int  ptab_size;

   rc = un_copy_line( window->ll, window, TRUE, TRUE );
   check_block( &sw );
   if (g_status.marked == FALSE || rc == ERROR ||
       g_status.marked_file->read_only)
      return( ERROR );

   file = g_status.marked_file;
   type = file->block_type;
   if (type == STREAM) {
      /*
       * cannot indent stream blocks
       */
      error( WARNING, window->bottom_line, block30 );
      return( ERROR );
   }
   tabs = file->inflate_tabs;
   ltab_size = file->ltab_size;
   ptab_size = file->ptab_size;

   p  = file->block_start;
   li = file->block_br;
   er = file->block_er;
   bc = (type == BOX) ? file->block_bc : 0;
   indent = (g_status.command == BlockIndentN ||
             g_status.command == BlockIndent);

   /*
    * Skip any initial blank lines.
    */
   if (type == LINE)
      for (; li <= er && is_line_blank( p->line, p->len, tabs );
           p = p->next, ++li) ;
   else {
      col = bc;
      for (; li <= er; p = p->next, ++li) {
         if (tabs)
            col = entab_adjust_rcol( p->line, p->len, bc, ptab_size );
         if (col < p->len && !is_line_blank(p->line + col, p->len - col, tabs))
            break;
      }
   }
   if (li > er)
      return( ERROR );

   copy_line( p, sw, TRUE );
   g_status.copied = FALSE;     /* in case of error */

   /*
    * Verify something to undent.
    */
   if (!indent && g_status.line_buff[bc] != ' ')
      return( ERROR );

   if (mode.do_backups == TRUE) {
      file->modified = TRUE;
      rc = backup_file( sw );
      if (rc == ERROR)
         return( ERROR );
   }

   /*
    * use a local window structure to do the dirty work.
    * set s_w to invisible so the un_copy_line function will
    *   not display the lines while doing block stuff.
    */
   dup_window_info( &s_w, sw );
   s_w.visible = FALSE;

   /*
    * Determine the level of indentation.
    */
   for (col = bc; g_status.line_buff[col] == ' '; ++col) ;

   if (g_status.command == BlockIndent || g_status.command == BlockUndent) {
      s_w.rcol = col;
      s_w.ll   = p;
      if (g_status.command == BlockIndent)
         add = (mode.smart_tab) ? next_smart_tab( &s_w ) :
                                  ltab_size - (bc % ltab_size);
      else {
         if (mode.indent) {
            for (temp_ll = file->block_start->prev; temp_ll != NULL;
                 temp_ll = temp_ll->prev) {
               t = temp_ll->line;
               rem = temp_ll->len;
               t = detab_a_line( t, &rem, tabs, ptab_size );
               if (bc < rem) {
                  add = first_non_blank( t + bc, rem - bc, tabs, ptab_size );
                  if (add < col - bc  &&  add != rem - bc) {
                     add = col - bc - add;
                     break;
                  }
               }
            }
            if (add == 0)
               add = col - bc;
         } else {
            add = (mode.smart_tab) ? prev_smart_tab( &s_w ) :
                                     bc % ltab_size;
            if (add == 0)
               add = ltab_size;
         }
      }
   } else {
      /*
       * indentation level:
       */
      rc = get_number( block31, window->bottom_line, &level, NULL );
      if (rc == ERROR)
         return( ERROR );
      add = abs( (int)level );
   }
   if (!indent) {
      if (add > col - bc)
         add = col - bc;
   }

   /*
    * for each line in the marked block, do the necessary indentation.
    */
   tabs = (tabs == 2 && type == LINE);
   if (add == 0) {
      p = p->next;
      ++li;
   } else
      g_status.copied = TRUE;   /* already know first line is copied */
   for (; rc == OK  &&  li <= er  &&  !g_status.control_break; li++) {
      if (tabs)
         detab_begin( p, ptab_size, indent, add );
      else
         copy_line( p, &s_w, TRUE );

      if (bc < g_status.line_buff_len) {
         s   = g_status.line_buff + bc;
         rem = g_status.line_buff_len - bc;
         if (add == 0) {
            for (rc = 0; s[rc] == ' ' && rc < rem; ++rc) ;
            rc += bc;
            if (rc < col) {
               rc = col - rc;
               if (g_status.line_buff_len + rc < MAX_LINE_LENGTH) {
                  memmove( s + rc, s, rem );
                  memset( s, ' ', rc );
                  g_status.line_buff_len += rc;
               }
            } else if (rc > col) {
               rc -= col;
               memmove( s, s + rc, rem - rc );
               g_status.line_buff_len -= rc;
            }
         } else if (indent) {
            if (g_status.line_buff_len + add < MAX_LINE_LENGTH) {
               memmove( s + add, s, rem );
               memset( s, ' ', add );
               g_status.line_buff_len += add;
            }
         } else {
            for (col = bc-1 + ((add > rem) ? rem : add); col >= bc; --col)
               if (g_status.line_buff[col] != ' ')
                  break;
            if (col >= bc)
               load_undo_buffer( file, p->line, p->len );

            rem -= add;
            if (rem > 0) {
               memmove( s, s + add, rem );
               g_status.line_buff_len -= add;
            } else
               g_status.line_buff_len = bc;
         }
         s_w.rline = li;
         rc = un_copy_line( p, &s_w, TRUE, TRUE );
      } else
         g_status.copied = FALSE;

      p = p->next;
   }

   /*
    * IMPORTANT:  we need to reset the copied flag because the cursor may
    * not necessarily be on the last line of the block.
    */
   g_status.copied = FALSE;
   syntax_check_block( file->block_br, er, file->block_start, file->syntax );
   file->dirty = GLOBAL;
   show_avail_mem( );

   return( rc );
}


/*
 * Name:    detab_begin
 * Purpose: replace leading tabs with spaces (helper for block_indent())
 * Author:  Jason Hood
 * Date:    October 4, 1999
 * Passed:  ll:     line to be detabbed
 *          tab:    size of tabs
 *          indent: TRUE for indenting, FALSE for undenting
 *          add:    number of spaces to be added/removed
 * Notes:   the detabbed line is placed in the line buffer
 */
static void detab_begin( line_list_ptr ll, int tab, int indent, int add )
{
text_ptr p = ll->line;
text_ptr to = g_status.line_buff;
int  len = ll->len;
int  col;
int  spaces;

   if (len == 0)
      g_status.line_buff_len = 0;

   else {
      col = 0;
      while (len > 0 && ((*p == '\t' || *p == ' ')  ||
                         (!indent && col < add))) {
         if (*p == '\t') {
            spaces = tab - (col % tab);
            memset( to, ' ', spaces );
            to  += spaces;
            col += spaces;
         } else {
            *to++ = *p;
            ++col;
         }
         ++p;
         --len;
      }
      my_memcpy( to, p, len );
      g_status.line_buff_len = col + len;
   }
   g_status.copied = TRUE;
}


/*
 * Name:    find_swap_lines
 * Purpose: mark the region to be swapped with the block
 * Author:  Jason Hood
 * Date:    October 28, 1999
 * Passed:  window:   pointer to destination window
 *          dest:     pointer to current line
 *          dest_end: pointer to last line
 *          br:       beginning line number in marked block
 *          er:       ending line number in marked block
 *          same:     are source and destination files same? T or F
 * Returns: OK if swap is to occur:
 *            dest contains beginning line, dest_end ending line,
 *            swap_br beginning line number, swap_er ending line number,
 *            swap_bc = -1, swap_er is undefined.
 *          ERROR if cancelled, swap_er = -1, other swap_* undefined.
 * Notes:   display the region in the hilited file color.
 *          Use Ctrl+Up/Down to select the same number of lines as the block;
 *           Up/Down to modify the size; Home/End to select top or bottom.
 */
static int find_swap_lines( TDE_WIN *window,
                            line_list_ptr *dest,
                            line_list_ptr *dest_end,
                            long br, long er, int same )
{
long key;
int  func;
int  col;
int  wid;
TDE_WIN win;
int  redraw;
long last;
long diff;
int  rc;
DISPLAY_BUFF;

   dup_window_info( &win, window );
   last = win.file_info->length;
   col  = win.start_col;
   wid  = win.end_col - col + 1;

   swap_bc = -1;
   swap_br = swap_er = win.rline;
   *dest = *dest_end = win.ll;

   SAVE_LINE( g_display.mode_line );
   eol_clear( 0, g_display.mode_line, Color( Help ) );
   rc = (g_display.ncols - (int)strlen( block32 )) / 2;
   if (rc < 0)
      rc = 0;
   /*
    * Select swap lines...
    */
   s_output( block32, g_display.mode_line, rc, Color( Help ) );

   diff = 0;
   redraw = 2;

   for (;;) {
      if (redraw) {
         win.rline += diff;
         if (check_cline( &win, win.cline + (int)diff ))
            redraw = 1;
         if (redraw == 1)
            display_current_window( &win );
         else if (redraw == 2)
            hlight_line( col, win.cline, wid, Color( Swap ) );
         show_line_col( &win );
         xygoto( win.ccol, win.cline );
         refresh( );
         redraw = FALSE;
      }
      key  = getkey_macro( );
      func = (key == RTURN) ? Rturn :
             (key == ESC)   ? AbortCommand :
             getfunc( key );
      if (func == Rturn || func == AbortCommand)
         break;
      else switch (func) {
         case BegOfLine:
            win.ll = *dest;
            diff   = swap_br;
            goto Home_or_End;
         case EndOfLine:
            win.ll = *dest_end;
            diff   = swap_er;
         Home_or_End:
            diff  -= win.rline;
            redraw = 3;
         break;

         case LineDown:
            if (win.rline == swap_er) {
               ++swap_er;
               if (swap_er > last || (same && swap_er == br))
                  --swap_er;
               else {
                  win.ll = *dest_end = (*dest_end)->next;
                  diff   = 1;
                  redraw = 2;
               }
            } else {
               ++swap_br;
               update_line( &win );
               win.ll = *dest = (*dest)->next;
               diff   = 1;
               redraw = 3;
            }
         break;

         case LineUp:
            if (win.rline == swap_br) {
               --swap_br;
               if (swap_br == 0 || (same && swap_br == er))
                  ++swap_br;
               else {
                  win.ll = *dest = (*dest)->prev;
                  diff   = -1;
                  redraw = 2;
               }
            } else {
               --swap_er;
               update_line( &win );
               win.ll = *dest_end = (*dest_end)->prev;
               diff   = -1;
               redraw = 3;
            }
         break;

         case ScrollUpLine:
            if (swap_br == swap_er) {
               swap_br -= er - br;
               if (same && swap_br <= er && swap_er > er)
                  swap_br = er + 1;
               if (swap_br < 1)
                  swap_br = 1;
               for (diff = swap_er - swap_br; diff > 0; --diff)
                  win.ll = win.ll->prev;
               *dest  = win.ll;
               diff   = swap_br - swap_er;
               redraw = 1;
            }
         break;

         case ScrollDnLine:
            if (swap_br == swap_er) {
               swap_er += er - br;
               if (same && swap_er >= br && swap_br < br)
                  swap_er = br - 1;
               if (swap_er > last)
                  swap_er = last;
               for (diff = swap_er - swap_br; diff > 0; --diff)
                  win.ll = win.ll->next;
               *dest_end = win.ll;
               diff   = swap_er - swap_br;
               redraw = 1;
            }
         break;
      }
   }
   RESTORE_LINE( g_display.mode_line );
   xygoto( window->ccol, window->cline );
   if (func == AbortCommand) {
      swap_er = -1;
      display_current_window( window );
      refresh( );
      rc = ERROR;
   } else
      rc = OK;
   return( rc );
}


/*
 * Name:    find_swap_box
 * Purpose: mark the region to be swapped with the block
 * Author:  Jason Hood
 * Date:    October 29, 1999
 * Passed:  window:   pointer to destination window
 *          dest:     pointer to current line
 *          br:       beginning line number in marked block
 *          er:       ending line number in marked block
 *          bc:       beginning column in marked block
 *          ec:       ending column in marked block
 *          same:     are source and destination files same? T or F
 * Returns: OK if swap is to occur:
 *            swap_br beginning line number, swap_er ending line number,
 *            swap_bc beginning column, swap_er ending column.
 *          ERROR if cancelled, swap_er = -1, other swap_* undefined.
 * Notes:   display the region in the hilited file color.
 *          If the cursor is above the block and an overlap would occur then
 *           keep the width of the box and swap the lines. Eg:
 *
 *              1       a
 *              2       b
 *              a  ==>  c       Mark the "abc" and swap with the "12".
 *              b       1
 *              c       2
 *
 *          Use Ctrl+Up to select the same number of lines as the block and
 *           Up/Down to modify the size. Enter to accept, Esc to cancel.
 *
 *          Otherwise swap the same number of lines, but a different number
 *           of columns. Eg:
 *
 *              swap_br = block_br;  ==>  block_br = swap_br;
 *              swap_er = block_er;       block_er = swap_er;
 *
 *           Mark "swap" and swap with "block" (or vice versa).
 *
 *          Use Ctrl+Left/Right to select the same number of columns as the
 *           block; Left/Right to modify size; Home/End to select left or
 *           right edge.
 */
static int find_swap_box( TDE_WIN *window, long br, long er, int bc, int ec,
                          int same )
{
int  block_wid;
long block_len;
int  win_wid;
long key;
int  func;
TDE_WIN win;
TDE_WIN u_w;
int  line_only;
int  len;
int  col = 0;
int  diff;
long ldiff;
int  redraw;
int  i;
int  rc;
DISPLAY_BUFF;
const char *help;

   dup_window_info( &win, window );
   block_wid = ec - bc;
   block_len = er - br;
   win_wid = win.end_col - win.start_col;

   swap_bc = swap_ec = win.rcol;
   swap_br = win.rline;
   swap_er = swap_br + block_len;
   if (same && swap_bc >= bc && swap_bc <= ec && swap_br < br &&
       (swap_er >= br || (swap_er == br - 1 && swap_bc == bc))) {
      line_only = TRUE;
      swap_bc = bc;
      swap_ec = ec;
      swap_er = br - 1;
      len = block_wid;
      col = win.ccol - (win.rcol - bc);
      if (col < win.start_col) {
         col = win.start_col;
         len -= win.bcol - bc;
      }
      if (len > win.end_col - col)
         len = win.end_col - col;
      ++len;
      help = block33b;

   } else {
      line_only = FALSE;
      same = (same && br <= swap_er && swap_br <= er);
      len = win.bottom_line - win.cline;
      if (len > block_len)
         len = (int)block_len;
      help = block33a;
   }

   SAVE_LINE( g_display.mode_line );
   eol_clear( 0, g_display.mode_line, Color( Help ) );
   i = (g_display.ncols - (int)strlen( help )) / 2;
   if (i < 0)
      i = 0;
   /*
    * Select swap box...
    */
   s_output( help, g_display.mode_line, i, Color( Help ) );

   diff   = 0;
   ldiff  = 0;
   redraw = 1;
   for (;;) {
      if (redraw) {
         if (diff) {
            win.rcol += diff;
            win.ccol += diff;
            if (win.ccol < win.start_col) {
               win.ccol = win.start_col;
               win.bcol = win.rcol;
               redraw = 1;
            } else if (win.ccol > win.end_col) {
               win.ccol = win.end_col;
               win.bcol = win.rcol - win_wid;
               redraw = 1;
            }
         }
         if (ldiff) {
            if (redraw == 3)
               update_line( &win );
            win.rline += ldiff;
            if (check_cline( &win, win.cline + (int)ldiff ))
               redraw = 1;
            if (ldiff > 0)
               for (; ldiff > 0; --ldiff)
                  win.ll = win.ll->next;
            else
               for (; ldiff < 0; ++ldiff)
                  win.ll = win.ll->prev;
         }
         if (redraw == 1)
            display_current_window( &win );
         else if (redraw == 2) {
            if (diff) {
               for (i = len; i >= 0; --i)
                  hlight_line( win.ccol, win.cline + i, 1, Color( Swap ) );
            } else
               hlight_line( col, win.cline, len, Color( Swap ) );

         } else if (redraw == 3 && diff) {
            dup_window_info( &u_w, &win );
            for (i = len; i >= 0; --i) {
               update_line( &u_w );
               ++u_w.rline;
               ++u_w.cline;
               u_w.ll = u_w.ll->next;
            }
         }
         show_line_col( &win );
         xygoto( win.ccol, win.cline );
         refresh( );
         redraw = 0;
         diff   = 0;
         ldiff  = 0;
      }
      key  = getkey_macro( );
      func = (key == RTURN) ? Rturn :
             (key == ESC)   ? AbortCommand :
             getfunc( key );
      if (func == Rturn || func == AbortCommand)
         break;

      else if (!line_only) switch (func) {
         case BegOfLine: diff = swap_bc - win.rcol; redraw = 4; break;
         case EndOfLine: diff = swap_ec - win.rcol; redraw = 4; break;

         case CharRight:
         case StreamCharRight:
            if (win.rcol == swap_ec) {
               ++swap_ec;
               if (swap_ec >= MAX_LINE_LENGTH || (same && swap_ec == bc))
                  --swap_ec;
               else {
                  diff   = 1;
                  redraw = 2;
               }
            } else {
               ++swap_bc;
               diff   = 1;
               redraw = 3;
            }
         break;

         case CharLeft:
         case StreamCharLeft:
            if (win.rcol == swap_bc) {
               --swap_bc;
               if (swap_bc < 0 || (same && swap_bc == ec))
                  ++swap_bc;
               else {
                  diff   = -1;
                  redraw = 2;
               }
            } else {
               --swap_ec;
               diff   = -1;
               redraw = 3;
            }
         break;

         case WordLeft:
            if (swap_bc > swap_ec - block_wid) {
               swap_bc = swap_ec - block_wid;
               if (same && swap_bc <= ec && swap_ec > ec)
                  swap_bc = ec + 1;
               if (swap_bc < 0)
                  swap_bc = 0;
               diff   = swap_bc - win.rcol;
               redraw = 1;
            }
         break;

         case WordRight:
            if (swap_ec < swap_bc + block_wid) {
               swap_ec = swap_bc + block_wid;
               if (same && swap_ec >= bc && swap_bc < bc)
                  swap_ec = bc - 1;
               if (swap_ec > MAX_LINE_LENGTH - 1)
                  swap_ec = MAX_LINE_LENGTH - 1;
               diff   = swap_ec - win.rcol;
               redraw = 1;
            }
         break;

      } else switch (func) {
         case LineUp:
            if (swap_br > 1) {
               --swap_br;
               ldiff  = -1;
               redraw = 2;
            }
         break;

         case LineDown:
            if (swap_br < br - 1) {
               ++swap_br;
               ldiff  = 1;
               redraw = 3;
            }
         break;

         case ScrollUpLine:
            if (swap_br > swap_er - block_len) {
               swap_br = swap_er - block_len;
               if (swap_br < 1)
                  swap_br = 1;
               ldiff  = swap_br - win.rline;
               redraw = 1;
            }
         break;
      }
   }
   RESTORE_LINE( g_display.mode_line );
   xygoto( window->ccol, window->cline );
   if (func == AbortCommand) {
      swap_er = -1;
      display_current_window( window );
      refresh( );
      rc = ERROR;
   } else
      rc = OK;
   return( rc );
}


/*
 * Name:    block_to_clipboard
 * Purpose: place the block on the Windows clipboard
 * Author:  Jason Hood
 * Date:    August 26, 2002
 * Passed:  window:  pointer to current window
 * Notes:   copy the selection to memory, using CRLF as the line ending and
 *           appending a NUL.
 *          currently ignores any trailing space (doesn't go beyond eol).
 */
int  block_to_clipboard( TDE_WIN *window )
{
#if defined( __DOS16__ ) || defined( __UNIX__ )
   return( OK );
#else
int  cut;
char *buf;
int  size, length;
int  len;
int  block_type;
line_list_ptr begin;
register file_infos *file;
long number;
long er;
int  count;
int  bc, ec;
int  block_len;
int  tabs, ptab;
int  nomem;
int  rc;

   /*
    * make sure block is marked OK
    */
   if (un_copy_line( window->ll, window, TRUE, TRUE ) == ERROR)
      return( ERROR );
   rc = ERROR;
   check_block( NULL );
   cut = (g_status.command == CutToClipboard);
   if (g_status.marked == TRUE && !(g_status.marked_file->read_only && cut)) {

      buf = malloc( BUFF_SIZE + 3 );
      nomem = (buf == NULL);
      if (!nomem) {
         length = BUFF_SIZE + 3;
         size = 0;

         file = g_status.marked_file;
         tabs = file->inflate_tabs;
         ptab = file->ptab_size;

         block_type = file->block_type;
         ec = file->block_ec;
         if (block_type == STREAM && file->block_br == file->block_er && ec!=-1)
            block_type = BOX;
         block_len = ec + 1 - file->block_bc;

         begin = file->block_start;
         er = file->block_er;
         for (number = file->block_br; number <= er; number++) {
            len = begin->len;
            bc = 0;
            if (block_type == LINE ||
                (block_type == STREAM && number > file->block_br &&
                 (number < er || (number == er && ec == -1)))) {
               count = len;
            } else {
               if (block_type == BOX) {
                  bc = file->block_bc;
                  if (tabs) {
                     bc = entab_adjust_rcol( begin->line, len, bc, ptab );
                     ec = entab_adjust_rcol( begin->line, len, file->block_ec,
                                             ptab );
                     block_len = ec - bc + 1;
                  }
                  count =  len > ec ? block_len : len - bc;
               } else /* (block_type == STREAM) */ {
                  if (number == file->block_br) {
                     bc = file->block_bc;
                     if (tabs)
                        bc = entab_adjust_rcol( begin->line, len, bc, ptab );
                     count = len - bc;
                     if (ec == -1)
                        block_type = LINE;
                  } else /* (number == er) */ {
                     if (tabs)
                        ec = entab_adjust_rcol( begin->line, len, ec, ptab );
                     count = (ec < len) ? ec+1 : len;
                  }
               }
            }
            if (count < 0)
               count = 0;
            if (size + count + 3 > length) {
               char *tmp = realloc( buf, length + BUFF_SIZE + 3 );
               if (tmp != NULL) {
                  buf = tmp;
                  length += BUFF_SIZE + 3;
               } else {
                  nomem = TRUE;
                  break;
               }
            }
            if (count > 0) {

               assert( count < MAX_LINE_LENGTH );
               assert( bc >= 0 );
               assert( bc < MAX_LINE_LENGTH );

               memcpy( buf + size, begin->line + bc, count );
               size += count;
            }
            if (block_type == LINE  ||  number < er) {
               buf[size++] = '\r';
               buf[size++] = '\n';
            }
            begin = begin->next;
         }
         buf[size++] = '\0';
         rc = set_clipboard( buf, size );

         if (rc == OK  &&  !nomem) {
            if (cut) {
               g_status.command = DeleteBlock;
               rc = block_operation( window );
            } else if (g_status.command == CopyToClipboard)
               rc = unmark_block( window );
         }

         /*
          * IMPORTANT:  we need to reset the copied flag because the cursor may
          * not necessarily be on the last line of the block.
          */
         g_status.copied = FALSE;
         free( buf );

      }
      if (nomem)
         error( WARNING, window->bottom_line, block4 );
   }
   return( rc );
#endif
}


/*
 * Name:    comment_block
 * Purpose: to comment a block according to the current language
 * Author:  Jason Hood
 * Date:    March 2, 2003
 * Passed:  window:  pointer to current window
 * Notes:   uses the syntax highlighting info to determine the comment
 *           characters.  Block commenting a line block will add extra lines
 *           above and below the block; a stream block will have the comments
 *           added at the beginning and end of the stream; a box block will add
 *           the comments at the beginning and end for each line.  Line
 *           commenting a box block will add the comment at the beginning for
 *           each line; line blocks will match indentation.  In all cases the
 *           block will be extended to include the comments.
 */
int  comment_block( TDE_WIN *window )
{
TDE_WIN *win = NULL, s_w;
file_infos *file;
int  type;
line_list_ptr p;
line_list_ptr temp_ll;
text_ptr s, s1;
int  col;
int  bc;
int  ec;
long er;
long li;
int  rc;
int  tabs;
int  ptab_size;
syntax_info *syn;
char com1[6], com2[6];
int  len1, len2;
const char *errmsg = NULL;

   rc = un_copy_line( window->ll, window, TRUE, TRUE );
   check_block( &win );
   if (g_status.marked == FALSE  ||  g_status.marked_file->read_only  ||
       g_status.marked_file->syntax == NULL  ||  rc == ERROR)
      return( ERROR );

   file = g_status.marked_file;
   type = file->block_type;
   syn  = file->syntax->info;
   if (g_status.command == BlockBlockComment) {
      if (!syn->comstart[0][0]) {
         /*
          * No block comment defined.
          */
         errmsg = block36;
      }
   } else { /* (g_status.command == BlockLineComment) */
      if (!syn->comment[0][0]) {
         /*
          * No line comment defined.
          */
         errmsg = block37;

      } else if (type == STREAM) {
         /*
          * cannot line comment stream blocks
          */
         errmsg = block38;
      }
   }
   if (errmsg) {
      error( WARNING, window->bottom_line, errmsg );
      return( ERROR );
   }

   tabs = file->inflate_tabs;
   ptab_size = file->ptab_size;

   p  = file->block_start;
   li = file->block_br;
   er = file->block_er;
   if (type == STREAM  &&  li == er  &&  file->block_ec != -1)
      type = BOX;

   if (type == LINE) {
      /*
       * determine the indentation.
       */
      bc = MAX_LINE_LENGTH;
      for (; li <= er; ++li) {
         col = first_non_blank( p->line, p->len, tabs, ptab_size );
         if (col < bc) {
            bc = col;
            if (bc == 0)
               break;
         }
         p = p->next;
      }
      li = file->block_br;
      p  = file->block_start;
   } else
      bc = file->block_bc;

   /*
    * use a local window structure to do the dirty work.
    * set s_w to invisible so the un_copy_line function will
    *   not display the lines while doing block stuff.
    */
   dup_window_info( &s_w, win );
   s_w.visible = FALSE;

   s = g_status.line_buff + bc;
   if (g_status.command == BlockBlockComment) {
      len1 = strlen( (char*)syn->comstart[0] );
      len2 = strlen( (char*)syn->comend[0] );
      if (type == LINE) {
         /*
          * Block commenting a line block - add lines above and below
          * the block.
          */
         if (bc + len1 > MAX_LINE_LENGTH)
            bc = MAX_LINE_LENGTH - len1;
         if (bc + len2 > MAX_LINE_LENGTH)
            bc = MAX_LINE_LENGTH - len2;
         s = g_status.line_buff + bc;
         memset( g_status.line_buff, ' ', bc );
         memcpy( s, syn->comstart[0], len1 );
         temp_ll = new_line_text( g_status.line_buff, bc + len1, DIRTY, &rc );
         if (rc == OK) {
            insert_node( file, p->prev, temp_ll );
            ++file->block_er;
            s_w.rline = li - 1;
            adjust_windows_cursor( &s_w, 1 );
            memcpy( s, syn->comend[0], len2 );
            p = new_line_text( g_status.line_buff, bc + len2, DIRTY, &rc );
            if (rc == OK) {
               insert_node( file, file->block_end, p );
               s_w.rline = file->block_er++;
               adjust_windows_cursor( &s_w, 1 );
            }
            file->modified = TRUE;
            er += 2;
            file->block_start = temp_ll;
         }
      } else {
         memcpy( com1, (char*)syn->comstart[0], len1 );
         memcpy( com2 + 1, (char*)syn->comend[0], len2++ );
         com1[len1++] = com2[0] = ' ';
         if (type == STREAM) {
            /*
             * Block commenting a stream block - add the opening comment at
             * the start of the stream and the closing at the end.
             */
            if (tabs == 2)
               detab_begin( p, ptab_size, FALSE, bc );
            else
               copy_line( p, &s_w, TRUE );
            if (bc > g_status.line_buff_len) {
               int add = bc - g_status.line_buff_len;
               memset( g_status.line_buff + g_status.line_buff_len, ' ', add );
               g_status.line_buff_len += add;
            }
            if (g_status.line_buff_len + len1 < MAX_LINE_LENGTH) {
               memmove( s + len1, s, g_status.line_buff_len - bc );
               memcpy( s, com1, len1 );
               g_status.line_buff_len += len1;
            }
            s_w.rline = file->block_br;
            rc = un_copy_line( p, &s_w, TRUE, TRUE );
            p->type |= DIRTY;

            p = file->block_end;
            ec = (file->block_ec == -1) ? find_end( p->line, p->len,
                                                    tabs, ptab_size )
                                        : file->block_ec + 1;
            s = g_status.line_buff + ec;
            if (tabs == 2)
               detab_begin( p, ptab_size, FALSE, ec - 1 );
            else
               copy_line( p, &s_w, TRUE );
            if (ec > g_status.line_buff_len) {
               int add = ec - g_status.line_buff_len;
               memset( g_status.line_buff + g_status.line_buff_len, ' ', add );
               g_status.line_buff_len += add;
            }
            if (g_status.line_buff_len + len2 < MAX_LINE_LENGTH) {
               memmove( s + len2, s, g_status.line_buff_len - ec );
               memcpy( s, com2, len2 );
               g_status.line_buff_len += len2;
            }
            s_w.rline = file->block_er;
            rc = un_copy_line( p, &s_w, TRUE, TRUE );
            p->type |= DIRTY;
            if (file->block_ec != -1)
               file->block_ec += len2;

         } else /* (type == BOX) */ {
            /*
             * Block commenting a box block - for each line in the block, add
             * the opening comment at the start of the block and the closing
             * at the end.
             */
            ec = file->block_ec + 1;
            s1 = g_status.line_buff + ec + len1;
            for (; rc == OK  &&  li <= er  &&  !g_status.control_break; li++) {
               if (tabs == 2)
                  detab_begin( p, ptab_size, FALSE, ec - 1 );
               else
                  copy_line( p, &s_w, TRUE );
               if (ec > g_status.line_buff_len) {
                  int add = ec - g_status.line_buff_len;
                  memset( g_status.line_buff+g_status.line_buff_len, ' ', add );
                  g_status.line_buff_len += add;
               }
               if (g_status.line_buff_len + len1 + len2 < MAX_LINE_LENGTH) {
                  memmove( s + len1, s, g_status.line_buff_len - bc );
                  memmove( s1 + len2, s1, g_status.line_buff_len - ec );
                  memcpy( s, com1, len1 );
                  memcpy( s1, com2, len2 );
                  g_status.line_buff_len += len1 + len2;
               }
               s_w.rline = li;
               rc = un_copy_line( p, &s_w, TRUE, TRUE );
               p->type |= DIRTY;
               p = p->next;
            }
            file->block_ec += len1 + len2;
         }
      }
   } else if (g_status.command == BlockLineComment) {
      /*
       * Line commenting a line or box block - for each line in the block,
       * add the comment at the start. Line blocks will have the comment
       * aligned to the line closest to the left edge, not placed at the
       * beginning of each line.
       */
      len1 = strlen( (char*)syn->comment[0] );
      memcpy( com1, (char*)syn->comment[0], len1 );
      com1[len1++] = ' ';
      for (; rc == OK  &&  li <= er  &&  !g_status.control_break; li++) {
         if (tabs == 2)
            detab_begin( p, ptab_size, FALSE, bc );
         else
            copy_line( p, &s_w, TRUE );
         if (bc > g_status.line_buff_len) {
            int add = bc - g_status.line_buff_len;
            memset( g_status.line_buff + g_status.line_buff_len, ' ', add );
            g_status.line_buff_len += add;
         }
         if (g_status.line_buff_len + len1 < MAX_LINE_LENGTH) {
            memmove( s + len1, s, g_status.line_buff_len - bc );
            memcpy( s, com1, len1 );
            g_status.line_buff_len += len1;
         }
         s_w.rline = li;
         rc = un_copy_line( p, &s_w, TRUE, TRUE );
         p->type |= DIRTY;
         p = p->next;
      }
      if (type == BOX)
         file->block_ec += len1;
   }
   g_status.copied = FALSE;
   syntax_check_block( file->block_br, er, file->block_start, file->syntax );
   file->dirty = GLOBAL;
   show_avail_mem( );

   if (rc == ERROR) {
      /*
       * file too big
       */
      error( WARNING, window->bottom_line, main4 );
   }

   return( rc );
}


/*
 * Name:    uncomment_block
 * Purpose: remove the comments around a block
 * Author:  Jason Hood
 * Date:    March 4, 2003
 * Passed:  window:  pointer to current window
 * Notes:   the comments should be part of the block.
 *          the first line should have a comment, but it is not necessary
 *           for subsequent lines to be commented.
 *          it will only uncomment one style (ie. block or line), but it can
 *           uncomment both types of that style.
 */
int  uncomment_block( TDE_WIN *window )
{
TDE_WIN *win = NULL, s_w;
file_infos *file;
int  type;
line_list_ptr p;
line_list_ptr q;
text_ptr s, s1;
int  len;
int  col;
int  bc;
int  ec;
long er;
long li;
int  rc;
int  tabs;
int  ptab_size;
syntax_info *syn;
int  com, len1;
int  sub, tot;

   rc = un_copy_line( window->ll, window, TRUE, TRUE );
   check_block( &win );
   if (g_status.marked == FALSE  ||  g_status.marked_file->read_only  ||
       g_status.marked_file->syntax == NULL  ||  rc == ERROR)
      return( ERROR );

   file = g_status.marked_file;
   type = file->block_type;
   syn  = file->syntax->info;

   tabs = file->inflate_tabs;
   ptab_size = file->ptab_size;

   p  = file->block_start;
   q  = file->block_end;
   li = file->block_br;
   er = file->block_er;
   if (type == STREAM  &&  li == er  &&  file->block_ec != -1)
      type = BOX;

   dup_window_info( &s_w, win );
   s_w.visible = FALSE;

   com = -1;
   if (syn->comment[0][0]) {
      bc = find_comment( syn->comment[0], p->line, p->len );
      if (bc != -1)
         com = 0;
      else if (syn->comment[1][0]) {
         bc = find_comment( syn->comment[1], p->line, p->len );
         if (bc != -1)
            com = 1;
      }
      if (com != -1) {
         /*
          * Removing a line comment. The comment (and the space after it, if
          * one) will be removed from each line in the block, except for a
          * stream block, where only the first line applies.
          */
         sub = MAX_LINE_LENGTH;
         for (; rc == OK  &&  li <= er  &&  !g_status.control_break; li++) {
            if (bc != -1) {
               if (tabs == 2)
                  detab_begin( p, ptab_size, FALSE, bc );
               else
                  copy_line( p, &s_w, TRUE );
               s = g_status.line_buff + bc;
               len = len1 = strlen( (char*)syn->comment[com] );
               if (bc + len1 < g_status.line_buff_len && s[len1] == ' ')
                  ++len;
               if (len < sub)
                  sub = len;
               memcpy( s, s + len, g_status.line_buff_len - len );
               g_status.line_buff_len -= len;
               s_w.rline = li;
               rc = un_copy_line( p, &s_w, TRUE, TRUE );
               p->type |= DIRTY;
            }
            if (type == STREAM)
               break;
            p = p->next;
            bc = find_comment( syn->comment[0], p->line, p->len );
            if (bc != -1)
               com = 0;
            else if (syn->comment[1][0]) {
               bc = find_comment( syn->comment[1], p->line, p->len );
               if (bc != -1)
                  com = 1;
            }
         }
         if (type == BOX  &&  file->block_ec - sub >= file->block_bc)
            file->block_ec -= sub;
      }
   }

   if (com == -1  &&  syn->comstart[0][0]) {
      bc = find_comment( syn->comstart[0], p->line, p->len );
      if (bc != -1)
         com = 0;
      else if (syn->comstart[1][0]) {
         bc = find_comment( syn->comstart[1], p->line, p->len );
         if (bc != -1)
            com = 1;
      }
      if (com != -1) {
         if (type == STREAM) {
            ec = find_comment_close( syn->comend[com], q->line, q->len );
            if (q == p && ec < bc + (int)strlen( (char*)syn->comend[com] ))
               ec = -1;
            if (ec != -1) {
               /*
                * Remove the block comment at the beginning and end of a
                * stream block.
                */
            stream:
               if (tabs)
                  detab_begin( q, ptab_size, FALSE, ec );
               else
                  copy_line( q, &s_w, TRUE );
               s = g_status.line_buff + ec;
               len = strlen( (char*)syn->comend[com] );
               if (ec > 0 && s[-1] == ' ') {
                  ++len;
                  --s;
               }
               memcpy( s, s + len, g_status.line_buff_len - len );
               g_status.line_buff_len -= len;
               s_w.rline = file->block_er;
               rc = un_copy_line( q, &s_w, TRUE, TRUE );
               q->type |= DIRTY;
               if (file->block_ec != -1) {
                  file->block_ec -= len;
                  if (file->block_ec < 0)
                     file->block_ec = 0;
               }

               if (tabs)
                  detab_begin( p, ptab_size, FALSE, bc );
               else
                  copy_line( p, &s_w, TRUE );
               s = g_status.line_buff + bc;
               len = len1 = strlen( (char*)syn->comstart[com] );
               if (bc + len1 < g_status.line_buff_len && s[len1] == ' ')
                  ++len;
               memcpy( s, s + len, g_status.line_buff_len - len );
               g_status.line_buff_len -= len;
               s_w.rline = file->block_br;
               rc = un_copy_line( p, &s_w, TRUE, TRUE );
               p->type |= DIRTY;
            }
         } else {
            ec = find_comment_close( syn->comend[com], p->line, p->len );
            if (ec < bc + (int)strlen( (char*)syn->comend[com] ))
               ec = -1;
            if (ec != -1) {
               /*
                * Remove the block comment from the beginning and end of each
                * line in a box or line block.
                */
               sub = MAX_LINE_LENGTH;
               for (; rc == OK && li <= er && !g_status.control_break; li++) {
                  if (ec != -1) {
                     if (tabs == 2)
                        detab_begin( p, ptab_size, FALSE, ec );
                     else
                        copy_line( p, &s_w, TRUE );
                     s = g_status.line_buff + bc;
                     s1 = g_status.line_buff + ec;
                     len = strlen( (char*)syn->comend[com] );
                     if (ec > 0 && s1[-1] == ' ') {
                        ++len;
                        --s1;
                     }
                     tot = len;
                     memcpy( s1, s1 + len, g_status.line_buff_len - len );
                     g_status.line_buff_len -= len;
                     len = len1 = strlen( (char*)syn->comstart[com] );
                     if (bc + len1 < g_status.line_buff_len && s[len1] == ' ')
                        ++len;
                     tot += len;
                     if (tot < sub)
                        sub = tot;
                     memcpy( s, s + len, g_status.line_buff_len - len );
                     g_status.line_buff_len -= len;
                     s_w.rline = li;
                     rc = un_copy_line( p, &s_w, TRUE, TRUE );
                     p->type |= DIRTY;
                  }
                  p = p->next;
                  bc = find_comment( syn->comstart[0], p->line, p->len );
                  if (bc != -1)
                     com = 0;
                  else if (syn->comment[1][0]) {
                     bc = find_comment( syn->comstart[1], p->line, p->len );
                     if (bc != -1)
                        com = 1;
                  }
                  if (bc == -1)
                     break;
                  ec = find_comment_close( syn->comend[com], p->line, p->len );
               }
               if (type == BOX  &&  file->block_ec - sub >= file->block_bc)
                  file->block_ec -= sub;

            } else if (type == LINE) {
               ec = find_comment_close( syn->comend[com], q->line, q->len );
               if (q == p && ec < bc + (int)strlen( (char*)syn->comend[com] ))
                  ec = -1;
               if (ec != -1) {
                  col = first_non_blank( q->line, q->len, tabs, ptab_size );
                  if (col == ec) {
                     if (tabs)
                        col = entab_adjust_rcol(p->line, p->len, bc, ptab_size);
                     len = strlen( (char*)syn->comstart[com] );
                     if (is_line_blank( p->line + col + len,
                                        p->len - len - col, tabs )) {
                        /*
                         * The opening and closing comments are on lines by
                         * themselves, so delete the lines.
                         */
                        move_to_line( &s_w, li, FALSE );
                        line_kill( &s_w );
                        move_to_line( &s_w, er - 1, FALSE );
                        line_kill( &s_w );
                        /*
                         * readjust the block beginning, due to the dup'ed win
                         */
                        ++file->block_br;
                        com = -1;       /* line_kill() checks syntax */
                     }
                  }
                  if (com != -1) {
                     /*
                      * The comment apparently begins on the beginning of the
                      * block and ends on the end, so do a stream remove.
                      */
                     goto stream;
                  }
               }
            }
         }
      }
   }
   g_status.copied = FALSE;
   if (com != -1)
      syntax_check_block( file->block_br, er, file->block_start, file->syntax );
   file->dirty = GLOBAL;
   show_avail_mem( );

   return( rc );
}


/*
 * Name:    find_comment
 * Class:   helper function
 * Purpose: determine if the line has a comment
 * Author:  Jason Hood
 * Date:    March 4, 2003
 * Passed:  comment:  the comment to find
 *          line:     the line in which to find it
 *          len:      length of above
 * Returns: the column of the comment or -1 if not found
 * Notes:   uses marked_file for block and tab values.
 *          for stream blocks, should only be used for the first line.
 *          will only match a comment completely inside the block.
 */
static int  find_comment( text_ptr comment, text_ptr line, int len )
{
int  bc, ec;
int  pos;
int  type;
int  tabs;
int  tab_size;

   tabs = g_status.marked_file->inflate_tabs;
   tab_size = g_status.marked_file->ptab_size;
   type = g_status.marked_file->block_type;

   if (type == LINE) {
      bc = 0;
      ec = len - 1;
   } else {
      bc = g_status.marked_file->block_bc;
      if (tabs)
         bc = entab_adjust_rcol( line, len, bc, tab_size );
      if (type == STREAM)
         ec = len - 1;
      else {
         ec = g_status.marked_file->block_ec;
         if (tabs)
            ec = entab_adjust_rcol( line, len, ec, tab_size );
         if (ec >= len)
            ec = len - 1;
      }
   }

   while (bc <= ec  &&  bj_isspace( line[bc] ))
      ++bc;
   pos = bc;
   while (bc <= ec  &&  line[bc++] == *comment) {
      if (*++comment == '\0')
         break;
   }

   if (*comment == '\0') {
      if (tabs)
         pos = detab_adjust_rcol( line, pos, tab_size );
   } else
      pos = -1;

   return( pos );
}


/*
 * Name:    find_comment_close
 * Class:   helper function
 * Purpose: determine if the line ends with a comment
 * Author:  Jason Hood
 * Date:    March 4, 2003
 * Passed:  comment:  the comment to find
 *          line:     the line in which to find it
 *          len:      length of above
 * Returns: the virtual column or -1 if not found
 * Notes:   uses marked_file for block and tab values.
 *          for stream blocks, should only be used for the last line.
 *          will only match a comment completely inside the block.
 */
static int  find_comment_close( text_ptr comment, text_ptr line, int len )
{
int  bc, ec;
int  type;
int  tabs;
int  tab_size;
text_ptr com = (text_ptr)strchr( (char*)comment, '\0' ) - 1;

   tabs = g_status.marked_file->inflate_tabs;
   tab_size = g_status.marked_file->ptab_size;
   type = g_status.marked_file->block_type;

   if (type == LINE) {
      bc = 0;
      ec = len - 1;
   } else {
      if (type == STREAM)
         bc = 0;
      else {
         bc = g_status.marked_file->block_bc;
         if (tabs)
            bc = entab_adjust_rcol( line, len, bc, tab_size );
      }
      ec = g_status.marked_file->block_ec;
      if (ec == -1)
         ec = len - 1;
      else {
         if (tabs)
            ec = entab_adjust_rcol( line, len, ec, tab_size );
         if (ec >= len)
            ec = len - 1;
      }
   }

   while (ec >= bc  &&  bj_isspace( line[ec] ))
      --ec;
   while (ec >= bc  &&  line[ec] == *com) {
      if (--com == comment - 1)
         break;
      --ec;
   }

   if (com == comment - 1) {
      if (tabs)
         ec = detab_adjust_rcol( line, ec, tab_size );
   } else
      ec = -1;

   return( ec );
}
