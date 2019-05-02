/*
 * Being that the windows in TDE are numbered and lettered, we can easily
 * prompt for windows to diff.  Might as well do a few standard diff
 * options:  ignore leading space, ignore all space, ignore blank lines,
 * ignore end-of-line, and Ignore/Match case.  Once the diff is defined,
 * just press one key to find the next diff.  Any two visible windows may
 * be diffed, which is really nice for comparing similar functions or
 * data in separate areas of a file.
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
 * This code is released into the public domain, Frank Davis.
 * You may distribute it freely.
 */

#include "tdestr.h"
#include "common.h"
#include "define.h"
#include "tdefunc.h"


/*
 * Name:    define_diff
 * Purpose: get info needed to initialize diff
 * Date:    October 31, 1992
 * Passed:  window:  pointer to current window
 * Notes:   allow the user to start the diff at the beginning of the
 *            file or at the current cursor location.  once the diff
 *            has been defined, the user may press one key to diff again.
 *          user may diff any two visible windows on the screen.
 *
 * jmh 980726: provide some default window numbers and letters.
 *             As a result can also test for two visible windows.
 */
int  define_diff( TDE_WIN *window )
{
int  rc;
char answer[MAX_COLS+2];
TDE_WIN *win;

   rc = OK;

   /*
    * Find a second window number and letter. If it could not be
    * found, then there is only one visible window.
    */
   win = window->next;
   while (win != NULL && !win->visible)
      win = win->next;
   if (win == NULL) {
      win = g_status.window_list;
      while (win != window && !win->visible)
         win = win->next;
      if (win == window) {
         error( WARNING, window->bottom_line, diff_prompt0 );
         return( ERROR );
      }
   }

   sprintf( answer, "%d%c", window->file_info->file_no, window->letter );
   set_dlg_text( EF_First, answer );
   sprintf( answer, "%d%c", win->file_info->file_no, win->letter );
   set_dlg_text( EF_Second, answer );

   rc = do_dialog( diff_dialog, diff_proc );
   if (rc == OK) {
      if (CB_All)
         diff.all_space = diff.leading = TRUE;
      else {
         diff.all_space = FALSE;
         diff.leading   = CB_Lead;
      }
      diff.blank_lines = CB_Blank;
      diff.ignore_eol  = CB_EOL;

      if (un_copy_line( window->ll, window, TRUE, TRUE ) == ERROR)
         return( ERROR );

      /*
       * if everything is everything, initialize the diff pointers.
       */
      diff.defined = TRUE;
      if (!CB_Here) {
         diff.d1 = diff.w1->file_info->line_list->next;
         diff.d2 = diff.w2->file_info->line_list->next;
         diff.rline1 = 1L;
         diff.rline2 = 1L;
         diff.bin_offset1 = 0;
         diff.bin_offset2 = 0;
         rc = differ( 0, 0, window->bottom_line );
      } else {
         diff.d1 = diff.w1->ll;
         diff.d2 = diff.w2->ll;
         diff.rline1 = diff.w1->rline;
         diff.rline2 = diff.w2->rline;
         diff.bin_offset1 = diff.w1->bin_offset;
         diff.bin_offset2 = diff.w2->bin_offset;
         rc = differ( diff.w1->rcol, diff.w2->rcol, window->bottom_line );
      }
   }
   return( rc );
}

/*
 * Name:    diff_proc
 * Purpose: dialog callback for DefineDiff
 * Author:  Jason Hood
 * Date:    November 30, 2003
 * Notes:   verify the windows exist, are visible and different;
 *          disable leading space if all space is checked.
 */
int  diff_proc( int id, char *text )
{
TDE_WIN *win;
int  rc = OK;

   if (id == IDE_FIRST || id == IDE_SECOND) {
      if (find_window( &win, text, g_display.end_line ) == ERROR)
         rc = ERROR;

   } else if (id == IDC_ALL) {
      check_box_enabled( IDC_LEAD, !CB_All );

   } else if (id == 0) {
      find_window( &diff.w1, get_dlg_text( EF_First ),  g_display.end_line );
      find_window( &diff.w2, get_dlg_text( EF_Second ), g_display.end_line );
      if (diff.w1 == diff.w2) {
         /*
          * DIFF windows are the same
          */
         error( WARNING, g_display.end_line, diff_prompt1 );
         rc = ERROR;
      }
   }

   return( rc );
}


/*
 * Name:    repeat_diff
 * Purpose: compare two cursor positions
 * Date:    October 31, 1992
 * Passed:  window:  pointer to current window
 * Notes:   user may press this key at any time once the diff has been
 *            defined.
 */
int  repeat_diff( TDE_WIN *window )
{
register int rc = ERROR;

   if (diff.defined) {
      if (un_copy_line( window->ll, window, TRUE, TRUE ) == ERROR)
         return( ERROR );

      /*
       * initialize the diff pointers.
       */
      diff.d1 = diff.w1->ll;
      diff.d2 = diff.w2->ll;
      diff.rline1 = diff.w1->rline;
      diff.rline2 = diff.w2->rline;
      diff.bin_offset1 = diff.w1->bin_offset;
      diff.bin_offset2 = diff.w2->bin_offset;
      rc = differ( diff.w1->rcol, diff.w2->rcol, window->bottom_line );
   } else
      error( WARNING, window->bottom_line, diff_prompt5 );
   return( rc );
}


/*
 * Name:    differ
 * Purpose: diff text pointers
 * Date:    October 31, 1992
 * Passed:  initial_rcol1:  beginning column to begin diff in window1
 *          initial_rcol2:  beginning column to begin diff in window2
 *          bottom:         line to display diagnostics
 * Notes:   a straight diff on text pointers is simple; however, diffing
 *            with leading spaces and tabs is kinda messy.  let's do the
 *            messy diff.
 *
 * jmh 980702: use of bj_tolower instead of tolower.
 * jmh 991124: made it a bit easier to read by using macros;
 *             remember previous position.
 * jmh 010629: set file_info in the temp. windows so inc_line() works.
 * jmh 020812: David Hughes pointed out the macros should have no semi-colons
 *              and found a bug with (not) ignoring blank lines;
 *             rewrote it a bit (restructured the char. loop to remove some
 *              duplicated code, removed the tabs from the diff structure).
 * jmh 030311: treat extra lines as a difference.
 */
int  differ( int initial_rcol1, int initial_rcol2, int bottom )
{
int  rcol1;             /* virtual real column on diff window 1 */
int  rcol2;             /* virtual real column on diff window 2 */
int  r1;                /* real real column rcol1 - needed for tabs */
int  r2;                /* real real column rcol2 - needed for tabs */
char c1;                /* character under r1 */
char c2;                /* character under r2 */
int  leading1;          /* adjustment for leading space in window 1 */
int  leading2;          /* adjustment for leading space in window 2 */
int  len1;              /* length of diff1 line */
int  len2;              /* length of diff2 line */
line_list_ptr node1;    /* scratch node in window 1 */
line_list_ptr node2;    /* scratch node in window 2 */
text_ptr diff1;         /* scratch text ptr in window 1 */
text_ptr diff2;         /* scratch text ptr in window 2 */
TDE_WIN win1;           /* rline, ll and bin_offset of window 1 */
TDE_WIN win2;           /* rline, ll and bin_offset of window 2 */
int  len;               /* line length variable */
int  tabs1;             /* local variable for inflate_tabs, T or F */
int  tabs2;
int  tab_size1;         /* size of tabs in window 1 */
int  tab_size2;         /* size of tabs in window 2 */

#define ADJUST_TAB1 \
   r1 =  tabs1 ? entab_adjust_rcol( diff1, len1, rcol1, tab_size1 ) : rcol1

#define ADJUST_TAB2 \
   r2 =  tabs2 ? entab_adjust_rcol( diff2, len2, rcol2, tab_size2 ) : rcol2

#define GET_C1 \
   c1 =  (char)(r1 < len1  ?  *(diff1 + r1) :  0);\
   if (c1 == '\t'  &&  tabs1)\
      c1 = ' '

#define GET_C2 \
   c2 =  (char)(r2 < len2  ?  *(diff2 + r2) :  0);\
   if (c2 == '\t'  &&  tabs2)\
      c2 = ' '


   /*
    * initialize the text pointers and the initial column.  skip any
    *  initial blank lines.
    */
   win1.file_info = diff.w1->file_info;
   win2.file_info = diff.w2->file_info;
   win1.rline = diff.rline1;
   win2.rline = diff.rline2;
   win1.ll = node1 = diff.d1;
   win2.ll = node2 = diff.d2;
   win1.bin_offset = diff.bin_offset1;
   win2.bin_offset = diff.bin_offset2;
   tabs1 = diff.w1->file_info->inflate_tabs;
   tabs2 = diff.w2->file_info->inflate_tabs;
   tab_size1 = diff.w1->file_info->ptab_size;
   tab_size2 = diff.w2->file_info->ptab_size;

   if (diff.blank_lines) {
      while (is_line_blank( node1->line, node1->len, tabs1 )  &&
             inc_line( &win1, TRUE )) {
         node1 = win1.ll;
         initial_rcol1 = 0;
      }
      while (is_line_blank( node2->line, node2->len, tabs2 )  &&
             inc_line( &win2, TRUE )) {
         node2 = win2.ll;
         initial_rcol2 = 0;
      }
   }

   /*
    * if everything is everything, initialize the diff variables and diff.
    */
   if (node1->len != EOF  &&  node2->len != EOF) {
      diff1 = node1->line;
      diff2 = node2->line;
      rcol1 = initial_rcol1;
      rcol2 = initial_rcol2;
      len1  = node1->len;
      len2  = node2->len;

      assert( rcol1 >= 0 );
      assert( rcol1 < MAX_LINE_LENGTH );
      assert( rcol2 >= 0 );
      assert( rcol2 < MAX_LINE_LENGTH );
      assert( len1 >= 0 );
      assert( len1 < MAX_LINE_LENGTH );
      assert( len2 >= 0 );
      assert( len2 < MAX_LINE_LENGTH );

      /*
       * if cursors are past EOL, move them back to EOL.
       */
      len = find_end( diff1, len1, tabs1, tab_size1 );
      if (rcol1 > len)
         rcol1 = len;
      len = find_end( diff2, len2, tabs2, tab_size2 );
      if (rcol2 > len)
         rcol2 = len;

      /*
       * if skip leading space, make sure our cursors start on first non-space.
       */
      if (diff.leading) {
         leading1 = skip_leading_space( diff1, len1, tabs1, tab_size1 );
         leading2 = skip_leading_space( diff2, len2, tabs2, tab_size2 );
         if (rcol1 < leading1)
            rcol1 = leading1;
         if (rcol2 < leading2)
            rcol2 = leading2;
      }

      /*
       * we now have a valid rcol for the diff start, we may need to adjust
       *   for tabs, though.
       */
      assert( rcol1 >= 0 );
      assert( rcol1 < MAX_LINE_LENGTH );
      assert( rcol2 >= 0 );
      assert( rcol2 < MAX_LINE_LENGTH );

      show_search_message( DIFFING );
      while ((node1->len != EOF  ||  node2->len != EOF)  &&
                         !g_status.control_break) {

         /*
          * diff each character in the diff lines until we reach EOL
          */
         while (TRUE) {
            ADJUST_TAB1;
            ADJUST_TAB2;
            /*
             * if one of the node pointers has come to EOL, move to next
             *   diff line.
             */
            if (diff.ignore_eol) {
               if (r1 >= len1) {
                  node1 = skip_eol( &win1, &r1, &rcol1, tabs1, tab_size1 );
                  len1  = node1->len;
                  diff1 = node1->line;
               }
               if (r2 >= len2) {
                  node2 = skip_eol( &win2, &r2, &rcol2, tabs2, tab_size2 );
                  len2  = node2->len;
                  diff2 = node2->line;
               }
            }
            /*
             * look at each character in each diff window
             */
            GET_C1;
            GET_C2;
            /*
             * skip spaces, if needed
             */
            if (diff.all_space) {
               while (c1 == ' '  &&  r1 < len1) {
                  ++rcol1;
                  ADJUST_TAB1;
                  GET_C1;
               }
               while (c2 == ' '  &&  r2 < len2) {
                  ++rcol2;
                  ADJUST_TAB2;
                  GET_C2;
               }
               if (diff.ignore_eol) {
                  if (r1 >= len1) {
                     node1 = skip_eol( &win1, &r1, &rcol1, tabs1, tab_size1 );
                     len1  = node1->len;
                     diff1 = node1->line;
                     GET_C1;
                  }
                  if (r2 >= len2) {
                     node2 = skip_eol( &win2, &r2, &rcol2, tabs2, tab_size2 );
                     len2  = node2->len;
                     diff2 = node2->line;
                     GET_C2;
                  }
               }
            }
            if (r1 >= len1  ||  r2 >= len2)
               break;

            /*
             * convert the characters to lower case, if needed.
             */
            if (mode.search_case == IGNORE) {
               c1 = (char)bj_tolower( c1 );
               c2 = (char)bj_tolower( c2 );
            }

            if (c1 != c2)
               break;

            ++rcol1;
            ++rcol2;
         }

         /*
          * if we haven't come to the end of a file buffer, check the last
          *   characters.  see if pointers are at EOL.
          */
         if (node1->len != EOF  ||  node2->len != EOF) {
            if (r1 < len1  ||  node1->len == EOF  ||
                r2 < len2  ||  node2->len == EOF) {
               undo_move( diff.w1, 0 );
               undo_move( diff.w2, 0 );

               set_marker( diff.w1 );
               set_marker( diff.w2 );

               win1.rcol = rcol1;
               move_display( diff.w1, &win1 );
               show_diff_window( diff.w1 );
               win2.rcol = rcol2;
               move_display( diff.w2, &win2 );
               show_diff_window( diff.w2 );
               show_search_message( CLR_SEARCH );
               return( OK );
            } else {
               node1 = skip_eol( &win1, &r1, &rcol1, tabs1, tab_size1 );
               len1  = node1->len;
               diff1 = node1->line;
               node2 = skip_eol( &win2, &r2, &rcol2, tabs2, tab_size2 );
               len2  = node2->len;
               diff2 = node2->line;
            }
         }

         assert( rcol1 >= 0 );
         assert( rcol1 < MAX_LINE_LENGTH );
         assert( rcol2 >= 0 );
         assert( rcol2 < MAX_LINE_LENGTH );
         assert( r1 >= 0 );
         assert( r1 < MAX_LINE_LENGTH );
         assert( r2 >= 0 );
         assert( r2 < MAX_LINE_LENGTH );
         assert( r1 <= rcol1 );
         assert( r2 <= rcol2 );
         if (node1->len == EOF)
            assert( len1 == EOF );
         else {
            assert( len1 >= 0 );
            assert( len1 < MAX_LINE_LENGTH );
         }
         if (node2->len == EOF)
            assert( len2 == EOF );
         else {
            assert( len2 >= 0 );
            assert( len2 < MAX_LINE_LENGTH );
         }
      }
      error( INFO, bottom, diff_prompt4 );
      show_search_message( CLR_SEARCH );
   }
   return( ERROR );
}


/*
 * Name:    skip_leading_space
 * Purpose: put the diff on the first non-blank character
 * Date:    October 31, 1992
 * Passed:  s:  the string to search
 *          len: length of string
 *          tabs: should tabs be skipped?
 *          tab_size: size of tabs
 * Returns: the first non-blank column
 */
int  skip_leading_space( text_ptr s, int len, int tabs, int tab_size )
{
register int count = 0;
text_ptr ll = s;

   assert( len >= 0 );
   assert( len < MAX_LINE_LENGTH );

   if (s != NULL) {
      if (tabs) {
         while (len > 0  &&  (*s == ' ' || *s == '\t')) {
            ++count;
            ++s;
            --len;
         }
         count = detab_adjust_rcol( ll, count, tab_size );
      } else {
         while (len > 0  &&  *s == ' ') {
           ++count;
           ++s;
           --len;
         }
      }
   }
   if (len == 0)
      count = 0;

   return( count );
}


/*
 * Name:    skip_eol
 * Purpose: move the diff to the next line
 * Date:    October 31, 1992
 * Passed:  win:         pointer to current window (node, line number, offset)
 *          r:           tab adjusted real col
 *          rcol:        real real col
 *          tabs:        do tabs count?
 *          tab_size:    size of tabs
 * Returns: next non-blank node
 */
line_list_ptr skip_eol( TDE_WIN *win, int *r, int *rcol,
                        int tabs, int tab_size )
{
   *r = *rcol = 0;
   if (inc_line( win, TRUE )) {
      if (diff.blank_lines)
         while (is_line_blank( win->ll->line, win->ll->len, tabs )  &&
                inc_line( win, TRUE )) ;

      if (win->ll->len != EOF) {
         if (diff.leading)
            *rcol = skip_leading_space( win->ll->line, win->ll->len,
                                        tabs, tab_size );
         else
            *rcol = 0;
         *r = *rcol;
         if (tabs)
            *r = entab_adjust_rcol( win->ll->line, win->ll->len, *rcol,
                                    tab_size );
      }
   }
   return( win->ll );
}


/*
 * Name:    show_diff_window
 * Purpose: update the contents of a diff window
 * Date:    October 31, 1992
 * Passed:  win:  pointer to window
 */
void show_diff_window( TDE_WIN *win )
{
   if (win->file_info->dirty & LOCAL)
      display_current_window( win );
   show_line_col( win );
   if (win->file_info->dirty & RULER)
      show_ruler( win );
   show_ruler_pointer( win );
   win->file_info->dirty = FALSE;
}
