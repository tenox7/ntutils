/*
 * Most of the tab routines were gathered into one file.  There is an
 * assembly routine tdeasm.c that expands tabs.  That routine is in
 * assembly to keep screen updates fairly fast. (jmh 991202: moved it here.)
 *
 * The basic detab and entab algorithms were supplied by Dave Regan,
 *   regan@jacobs.cs.orst.edu
 *
 * For more info on tabs see:
 *
 *     Brian W. Kernighan and P. J. Plauger, _Software Tools_, Addison-
 *     Wesley Publishing Company, Reading, Mass, 1976, pp 18-27 and 35-39,
 *     ISBN 0-20103669-X.
 *
 * The above reference gives info on fixed and variable tabs.  But when
 *  it comes to non-fixed tabs, I prefer "smart" tabs.  Being lazy, I find
 *  it more convenient to let the editor figure variable tabs for me.
 *
 * jmh 990501: lots of changes due to tabs becoming file dependent. This also
 *             causes lots of changes in other files that are not documented.
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
 * This program is released into the public domain, Frank Davis.
 *   You may distribute it freely.
 */

#include "tdestr.h"
#include "common.h"
#include "tdefunc.h"
#include "define.h"


/*
 * Name:    detab_to_col
 * Purpose: copy the line to the line buffer, detabbing up to rcol
 * Author:  Jason Hood
 * Date:    October 4, 1999
 * Passed:  rcol: column to stop detabbing
 *          tabs: tab mode
 *          size: size of tabs
 * Notes:   helper function for tab_key() and backtab() using real tabs.
 *
 * jmh 030305: due to the extra restoring functionality of copy_line(), made
 *              this routine more similar to adjust_for_tabs().
 */
static void detab_to_col( int rcol, int tabs, int size )
{
text_ptr p;
int  len;

   if (tabs == 2) {
      len = g_status.line_buff_len;
      if (len > 0) {
         rcol = entab_adjust_rcol( g_status.line_buff, len, rcol, size );
         if (rcol < len)
            len = ++rcol;
         else
            rcol = len;
         p = tabout( g_status.line_buff, &len, size, 0 );
         memmove( g_status.line_buff + len, g_status.line_buff + rcol,
                  g_status.line_buff_len - rcol );
         memcpy( g_status.line_buff, p, len );
         g_status.line_buff_len += len - rcol;
      }
   } else
      detab_linebuff( tabs, size );
}


/*
 * Name:    tab_key
 * Purpose: To make the necessary changes after the user types the tab key.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   If in insert mode, then this function adds the required
 *           number of spaces in the file.
 *          If not in insert mode, then tab simply moves the cursor right
 *           the required distance.
 */
int  tab_key( TDE_WIN *window )
{
int  spaces;            /* the spaces to move to the next tab stop */
text_ptr source;        /* source for block move to make room for c */
text_ptr dest;          /* destination for block move */
int  pad;
int  len;
register int rcol;
register TDE_WIN *win;   /* put window pointer in a register */
file_infos *file;
int  rc;

   win = window;
   if (win->ll->len == EOF)
      return( OK );
   show_ruler_char( win );

   rcol = win->rcol;
   file = win->file_info;
   /*
    * work out the number of spaces to the next tab stop
    */
   if (mode.smart_tab)
      spaces = next_smart_tab( win );
   else
      spaces = file->ltab_size - (rcol % file->ltab_size);

   assert( spaces >= 0 );
   assert( spaces < MAX_LINE_LENGTH );

   rc = OK;
   if (rcol + spaces < MAX_LINE_LENGTH) {
      if (mode.insert && !file->read_only) {
         copy_line( win->ll, win, FALSE );
         detab_to_col( rcol, file->inflate_tabs, file->ptab_size );

         /*
          * work out how many characters need to be inserted
          */
         len = g_status.line_buff_len;
         pad = rcol > len ? rcol - len : 0;
         if (len + pad + spaces >= MAX_LINE_LENGTH) {
            /*
             *  line too long to add
             */
            error( WARNING, win->bottom_line, ed1 );
            rc = ERROR;
            g_status.copied = FALSE;
         } else {
            if (pad > 0  || spaces > 0) {
               source = g_status.line_buff + rcol - pad;
               dest = source + pad + spaces;

               shift_tabbed_block( file );

               assert( len + pad - rcol >= 0 );
               assert( len + pad - rcol < MAX_LINE_LENGTH );

               memmove( dest, source, len + pad - rcol );

               /*
                * if padding was required, then put in the required spaces
                */

               assert( pad + spaces >= 0 );
               assert( pad + spaces < MAX_LINE_LENGTH );

               memset( source, ' ', pad + spaces );
               g_status.line_buff_len += pad + spaces;
               shift_block( file, win->rline, rcol, spaces );
               entab_linebuff( file->inflate_tabs, file->ptab_size );
            }

            win->ll->type |= DIRTY;
            file->modified = TRUE;
            file->dirty = GLOBAL;
            show_changed_line( win );
         }
      }
      if (rc == OK) {
         rcol += spaces;
         win->ccol += spaces;
         check_virtual_col( win, rcol, win->ccol );
      }
   }
   return( rc );
}


/*
 * Name:    backtab
 * Purpose: To make the necessary changes after the user presses the backtab.
 * Date:    November 1, 1991
 * Passed:  window:  pointer to current window
 * Notes:   If in insert mode, then this function subs the required
 *           number of spaces in the file.
 *          If not in insert mode, then tab simply moves the cursor left
 *           the required distance.
 */
int  backtab( TDE_WIN *window )
{
int  spaces;            /* the spaces to move to the next tab stop */
text_ptr source;        /* source for block move to make room for c */
text_ptr dest;          /* destination for block move */
int  pad;
int  len;
register int rcol;
register TDE_WIN *win;   /* put window pointer in a register */
file_infos *file;

   win  = window;
   rcol = win->rcol;
   if (win->ll->len == EOF || rcol == 0)
      return( OK );
   show_ruler_char( win );

   file = win->file_info;
   /*
    * work out the number of spaces to the previous tab stop
    */
   if (mode.smart_tab)
      spaces = prev_smart_tab( win );
   else
      spaces = rcol % file->ltab_size;

   if (spaces == 0)
      spaces = file->ltab_size;
   copy_line( win->ll, win, FALSE );
   detab_to_col( rcol, file->inflate_tabs, file->ptab_size );
   len = g_status.line_buff_len;
   if (mode.insert && rcol - spaces < len && !file->read_only) {
      pad = rcol > len ? rcol - len : 0;
      if (pad > 0  || spaces > 0) {
         /*
          * if padding was required, then put in the required spaces
          */
         if (pad > 0) {

            assert( rcol - pad >= 0 );
            assert( pad < MAX_LINE_LENGTH );

            source = g_status.line_buff + rcol - pad;
            dest = source + pad;

            assert( pad >= 0 );
            assert( pad < MAX_LINE_LENGTH );

            memmove( dest, source, pad );
            memset( source, ' ', pad );
            g_status.line_buff_len += pad;
         }
         source = g_status.line_buff + rcol;
         dest = source - spaces;

         shift_tabbed_block( file );

         assert( len + pad - rcol >= 0 );
         assert( len + pad - rcol < MAX_LINE_LENGTH );

         memmove( dest, source, len + pad - rcol );
         g_status.line_buff_len -= spaces;
         shift_block( file, win->rline, rcol, -spaces );
         entab_linebuff( file->inflate_tabs, file->ptab_size);
      }

      win->ll->type |= DIRTY;
      file->modified = TRUE;
      file->dirty = GLOBAL;
      show_changed_line( win );
      rcol -= spaces;
      win->ccol -= spaces;
   } else {
      /*
       * move the cursor without changing the text underneath
       */
      rcol -= spaces;
      if (rcol < 0)
         rcol = 0;
      win->ccol -= spaces;
   }
   check_virtual_col( win, rcol, win->ccol );
   return( OK );
}


/*
 * Name:    next_smart_tab
 * Purpose: To find next smart tab
 * Date:    June 5, 1992
 * Passed:  window: pointer to the current window
 * Notes:   To find a smart tab 1) find the first non-blank line above the
 *            current line, 2) find the first non-blank character after
 *            column of the cursor.
 */
int  next_smart_tab( TDE_WIN *window )
{
register int spaces;    /* the spaces to move to the next tab stop */
text_ptr s;             /* pointer to text */
line_list_ptr ll;
register TDE_WIN *win;   /* put window pointer in a register */
int  rcol;
int  len;
int  tabs;
int  ltab_size;
int  ptab_size;

   win = window;
   tabs = win->file_info->inflate_tabs;
   ltab_size = win->file_info->ltab_size;
   ptab_size = win->file_info->ptab_size;
   rcol = win->rcol;

   /*
    * find first previous non-blank line above the cursor.
    */
   ll = win->ll->prev;
   while (ll->prev != NULL  &&  is_line_blank( ll->line, ll->len, tabs ))
      ll = ll->prev;

   if (ll->prev != NULL) {
      s = ll->line;
      /*
       * if cursor is past the eol of the smart line, lets find the
       *   next fixed tab.
       * jmh 991004: find next fixed tab if on the last character, as well.
       */
      if (rcol >= find_end( s, ll->len, tabs, ptab_size ) - 1)
         spaces = ltab_size - (rcol % ltab_size);
      else {

         len = ll->len;
         s = detab_a_line( s, &len, tabs, ptab_size );

         spaces = 0;
         s   += rcol;
         len -= rcol;

         /*
          * if we are on a word, find the end of it.
          */
         while (len > 0  &&  *s != ' ') {
            ++s;
            ++spaces;
            --len;
         }
         /*
          * jmh 991004: if we found eol, move to the next fixed tab.
          */
         if (len == 0)
            spaces += ltab_size - ((rcol + spaces) % ltab_size);

         /*
          * now find the start of the next word.
          */
         else while (len > 0  &&  *s == ' ') {
            ++s;
            ++spaces;
            --len;
         }
      }
   } else
      spaces = ltab_size - (rcol % ltab_size);

   return( spaces );
}


/*
 * Name:    prev_smart_tab
 * Purpose: To find previous smart tab
 * Date:    June 5, 1992
 * Passed:  window: pointer to the current window
 * Notes:   To find a smart tab 1) find the first non-blank line above the
 *            current line, 2) find the first non-blank character before
 *            column of the cursor.
 *          there are several cases to consider:  1) the cursor is past the
 *            the end of the smart line, 2) the smart pointer is in the
 *            middle of a word, 3) there are no more words between the
 *            smart pointer and the beginning of the line.
 */
int  prev_smart_tab( TDE_WIN *window )
{
register int spaces;    /* the spaces to move to the next tab stop */
text_ptr s;             /* pointer to text */
int  len;
line_list_ptr ll;
register TDE_WIN *win;  /* put window pointer in a register */
int  rcol;
int  tabs;
int  tab_size;
int  ptab_size;

   win = window;
   tabs = win->file_info->inflate_tabs;
   tab_size = win->file_info->ltab_size;
   ptab_size = win->file_info->ptab_size;
   rcol = win->rcol;

   /*
    * find first previous non-blank line above the cursor, if it exists.
    */
   ll = win->ll->prev;
   while (ll->prev != NULL  && is_line_blank( ll->line, ll->len, tabs ))
      ll = ll->prev;

   if (ll->prev != NULL) {
      s = ll->line;

      /*
       * if there are no words between the cursor and column 1 of the
       *   smart tab line, find previous fixed tab.
       */
      if (rcol <= first_non_blank( s, ll->len, tabs, ptab_size ))
         spaces = rcol % tab_size;
      else {

         len = ll->len;
         s = detab_a_line( s, &len, tabs, ptab_size );

         /*
          * now, we need to figure the initial pointer and space.
          *   if the cursor is past the eol of the smart line, then
          *   set the smart pointer "s" to the end of line and "spaces" to
          *   the number of characters between the cursor and the eol
          *   of the smart line.  otherwise, set the smart pointer "s" to
          *   the column of the cursor and "spaces" to 0.
          */
         if (len < rcol) {
            spaces = rcol - len;
         } else {
            len = rcol;
            spaces = 0;
         }
         s += len;

         while (len > 0  &&  *(s-1) == ' ' ) {
            --s;
            ++spaces;
            --len;
         }

         /*
          * now find the beginning of the previous word.
          */
         while (len > 0  &&  *(s-1) != ' ') {
            --s;
            ++spaces;
            --len;
         }
      }
   } else
      spaces = rcol % tab_size;

   return( spaces );
}


/*
 * Name:    entab
 * Purpose: To compress spaces to tabs
 * Date:    October 31, 1992
 * Passed:  s:   pointer to line
 *          len: length of line
 *          tab_size: size of tabs
 *          ind: TRUE to indent only (stop at first non-blank)
 * Returns: TRUE if the line changed, FALSE otherwise
 * Notes:   the text_ptr can point to either the g_status.line_buff or
 *            a line in the main text buffer.
 *
 * jmh 991202: modified to work with the block compress.
 *             Don't bother using the tab buffer, since the only times it is
 *              called the line buffer is wanted.
 *             Treat tabs as spaces (since " \t" can usually just become "\t").
 */
int  entab( text_ptr s, int len, int tab_size, int ind )
{
int  last_col;
int  space;
register int col;
text_ptr to;
int  rc;

   assert( s != NULL );
   assert( len >= 0 );
   assert( len < MAX_LINE_LENGTH );

   rc = FALSE;
   to = g_status.line_buff;
   if (len > 0) {
      last_col = col = 0;
      do {
         if (*s == ' ')
            col++;
         else if (*s == '\t')
            col += tab_size - (col % tab_size);
         else {
            if (col != last_col) {
               do {
                  space = tab_size - (last_col % tab_size);
                  /*
                   * when space == 1, forget about emmitting a tab
                   *   for 1 space.
                   * jmh 991202: in real tabs, let's keep the tab.
                   * jmh 010630: perhaps not.
                   */
                  if (last_col + space <= col) {
                     if (space == 1) {
                        *to++ = ' ';
                        last_col++;
                     } else {
                        *to++ = '\t';
                        last_col += space;
                     }
                     rc = TRUE;
                  } else
                     do {
                        *to++ = ' ';
                        last_col++;
                     } while (last_col < col);
               } while (last_col < col);
            }
            /*
             * when we see a quote character, stop entabbing.
             */
            if (*s == '\"' || *s == '\'' || ind) {
               my_memcpy( to, s, len );
               to += len;
               last_col = col;
               break;
            }
            *to++ = *s;
            ++col;
            last_col = col;
         }
         s++;
      } while (--len != 0);

      if (col != last_col) {
         do {
            space = tab_size - (last_col % tab_size);
            if (last_col + space <= col) {
               if (space == 1) {
                  *to++ = ' ';
                  last_col++;
               } else {
                  *to++ = '\t';
                  last_col += space;
               }
               rc = TRUE;
            } else
               do {
                  *to++ = ' ';
                  last_col++;
               } while (last_col < col);
         } while (last_col < col);
      }
      g_status.line_buff_len = (int)(to - g_status.line_buff);
   } else
      g_status.line_buff_len = 0;
   return( rc );
}


/*
 * Name:    detab_linebuff
 * Purpose: To expand tabs in line buffer so we can handle editing functions
 * Date:    October 31, 1992
 * Notes:   before we do any editing function that modifies text, let's
 *            expand any tabs to space.
 *          if inflate_tabs is FALSE, then nothing is done.
 */
void detab_linebuff( int inflate_tabs, int tab_size )
{
int  len;

   if (inflate_tabs  &&  g_status.copied) {
      len = g_status.line_buff_len;
      tabout( g_status.line_buff, &len, tab_size, 0 );

      assert( len >= 0 );
      assert( len < MAX_LINE_LENGTH );

      memmove( g_status.line_buff, g_status.tabout_buff, len );
      g_status.line_buff_len = len;
   }
}


/*
 * Name:    entab_linebuff
 * Purpose: To compress space in line buffer
 * Date:    October 31, 1992
 * Notes:   compress spaces back to tabs in the line buffer, if
 *             inflate_tabs == TRUE
 *
 * jmh 991203: modified to suit the changed entab() function.
 */
void entab_linebuff( int inflate_tabs, int tab_size )
{
   if (inflate_tabs  &&  g_status.copied)
      entab( g_status.line_buff, g_status.line_buff_len, tab_size, FALSE );
}


/*
 * Name:     detab_a_line
 * Purpose:  To inflate tabs in any line in the file
 * Date:     October 31, 1992
 * Modified: August 20, 1997, Jason Hood - test for NULL explicitly
 * Returns:  pointer to text or tabout_buff
 * Notes:    expand an arbitrary line in the file.
 */
text_ptr detab_a_line( text_ptr s, int *len, int inflate_tabs, int tab_size )
{
   if (inflate_tabs && s != NULL) {

      assert( *len >= 0 );
      assert( *len < MAX_LINE_LENGTH );

      s = tabout( s, len, tab_size, 0 );
   }
   return( s );
}


/*
 * Name:    tabout
 * Purpose: Expand tabs in display line
 * Date:    October 31, 1992
 * Passed:  s:  string pointer
 *          len:  pointer to current length of string
 * Notes:   Expand tabs in the display line according to current tab.
 *          If eol display is on, let's display tabs, too.
 *
 * jmh 990501: added tab_size as a parameter.
 * jmh 991126: added tab display (mode.show_eol) as a parameter.
 */
text_ptr tabout( text_ptr s, int *len, int tab_size, int display )
{
text_ptr to;
int  i;
int  tab_len;
char show_tab;
#if !defined( __DOS16__ )
int  col;
int  space;
#endif

   if (*len > 0) {
      i = tab_len = *len;
      show_tab = (display == 1) ? '\t' : ' ';
      to = g_status.tabout_buff;

#if !defined( __DOS16__ )

      /*
       * for right now, no assembly in unix or djgpp or win32
       */
      col = 0;
      do {
         if (*s != '\t') {
            *to++ = *s;
            ++col;
         } else {
            space = tab_size - (col % tab_size);
            col += space;
            if (col > MAX_LINE_LENGTH) {
               space = col - MAX_LINE_LENGTH;
               col = MAX_LINE_LENGTH;
            }
            memset( to, ' ', space );
            *to = show_tab;
            to += space;
            tab_len += space - 1;
         }
         ++s;
      } while (--i != 0  &&  col < MAX_LINE_LENGTH);

#else

   ASSEMBLE {
        push    si
        push    di
        push    ds

        mov     bx, WORD PTR tab_size   /* keep tab_size in bx */
        xor     cx, cx                  /* keep col in cx */
        les     di, to                  /* es:di = to or the destination */
        lds     si, s                   /* ds:si = s  or the source */

        push    di                      /* preserve to */
        mov     ax, 0x2020              /* two spaces */
        mov     cx, MAX_LINE_LENGTH / 2 /* assume even */
        rep     stosw                   /* memset( to, ' ', MAX_LINE_LENGTH ) */
        pop     di                      /* restore to */
   }

top:
   ASSEMBLE {
        lodsb                           /* al = BYTE PTR ds:si */
        cmp     al, 0x09                /* is this a tab character? */
        jne     not_tab

        mov     al, BYTE PTR show_tab   /* tab or space */
        stosb                           /* store in es:di and incr di */
        mov     ax, cx
        xor     dx, dx                  /* set up dx:ax for IDIV */
        div     bx                      /* dx = col % tab_size */
        mov     ax, bx                  /* put tab_size in ax */
        sub     ax, dx                  /* ax = tab_size - (col % tab_size) */
        add     cx, ax                  /* col += space */
        dec     ax                      /* --space */
        add     di, ax                  /* to += space */
        add     WORD PTR tab_len, ax    /* add spaces to string length */
        jmp     SHORT next_char
   }

not_tab:
   ASSEMBLE {
        stosb                           /* store character in es:di inc di */
        inc     cx                      /* increment col counter */
  }

next_char:
   ASSEMBLE {
        dec     WORD PTR i              /* at end of string? */
        jz      get_out

        cmp     cx, MAX_LINE_LENGTH     /* at end of tabout buffer? */
        jl      top
}
get_out:
   ASSEMBLE {
        pop     ds
        pop     di
        pop     si
   }

#endif

      if (tab_len > MAX_LINE_LENGTH)
         tab_len = MAX_LINE_LENGTH;
      *len = g_status.tabout_buff_len = tab_len;
   }
   return( g_status.tabout_buff );
}


/*
 * Name:    detab_adjust_rcol
 * Purpose: given rcol in a line, find virtual column
 * Date:    October 31, 1992
 * Modified: August 20, 1997, Jason Hood - test for NULL like entab_adjust_rcol
 * Passed:  s:  string pointer
 * Notes:   without expanding tabs, calculate the display column according
 *            to current tab stop.
 */
int  detab_adjust_rcol( text_ptr s, int rcol, int tab_size )
{
register int col;

   assert( rcol >= EOF );
   assert( rcol < MAX_LINE_LENGTH );
   assert( tab_size != 0 );

   if (s != NULL && rcol > 0) {
      col = 0;
      do {
         if (*s++ == '\t')
            col += tab_size - (col % tab_size);
         else
            col++;
      } while (--rcol != 0);
   } else
      col = rcol;

   assert( col >= 0 );
   assert( col < MAX_LINE_LENGTH );

   return( col );
}


/*
 * Name:    entab_adjust_rcol
 * Purpose: given virtual rcol in a line, find real column
 * Date:    October 31, 1992
 * Passed:  s:     string pointer
 *          len:   length of string
 *          rcol:  virtual real column
 *          tab_size: size of tabs
 * Notes:   without expanding tabs, calculate which col the real cursor
 *            is referencing.
 *
 * jmh:     October 31, 1997 - added "|| len == EOF" to the first assertion, to
 *           prevent it activating when the cursor is at EOF.
 * jmh 990915: keep relative position beyond EOL.
 */
int  entab_adjust_rcol( text_ptr s, int len, int rcol, int tab_size )
{
register int col;
register int last_col;

   assert( len >= 0 || len == EOF );
   assert( len < MAX_LINE_LENGTH );
   assert( rcol >= EOF );
   assert( rcol < MAX_LINE_LENGTH );
   assert( tab_size != 0 );

   if (len > 0 && rcol > 0) {
      last_col = col = 0;
      do {
         if (*s++ != '\t')
            ++col;
         else {
            col += tab_size - (col % tab_size);
            if (col > rcol)
               break;
         }
         ++last_col;
      } while (col < rcol  &&  --len != 0);
      if (col < rcol)
         last_col += rcol - col;
   } else
      last_col = rcol;

   assert( last_col >= 0 );
   assert( last_col < MAX_LINE_LENGTH );

   return( last_col );
}


/*
 * Name:    block_expand_tabs
 * Purpose: Expand tabs in a marked block.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   Tabs are expanded using the current tab interval.
 *          Lines are checked to make sure they are not too long.
 *
 * jmh 991202: return ERROR if no block.
 */
int  block_expand_tabs( TDE_WIN *window )
{
int  len;
int  tab_size;
int  dirty;
register int spaces;
line_list_ptr p;                /* pointer to block line */
file_infos *file;
TDE_WIN *sw, s_w;
long er;
int  i;
int  rc;
text_ptr b, t;

   /*
    * make sure block is marked OK and that this is a LINE block
    */
   rc = un_copy_line( window->ll, window, TRUE, TRUE );
   check_block( &sw );
   file = g_status.marked_file;
   if (rc == ERROR  ||  g_status.marked == FALSE  ||  file->read_only)
      return( ERROR );

   if (file->block_type != LINE) {
      /*
       * can only expand tabs in line blocks
       */
      error( WARNING, window->bottom_line, block20 );
      return( ERROR );
   }

   /*
    * initialize everything
    */
   dirty = FALSE;
   tab_size = file->ptab_size;
   dup_window_info( &s_w, sw );
   p  = file->block_start;
   er = file->block_er;
   s_w.rline = file->block_br;
   s_w.visible = FALSE;
   for (; s_w.rline <= er  &&  !g_status.control_break  &&  rc == OK;
        s_w.rline++) {

      /*
       * use the line buffer to expand LINE blocks.
       */
      t = memchr( p->line, '\t', p->len );
      if (t != NULL) {
         g_status.line_buff_len = (int)(t - p->line);
         memcpy( g_status.line_buff, p->line, g_status.line_buff_len );
         b = g_status.line_buff + g_status.line_buff_len;
         len = p->len - g_status.line_buff_len;
         for (i = g_status.line_buff_len + 1; len > 0  &&  rc == OK;
              t++, b++, len--) {

            /*
             * each line in the LINE block is copied to the g_status.line_buff.
             *  look at the text in the buffer and expand tabs.
             */
            if (*t == '\t') {
               spaces = i % tab_size;
               if (spaces)
                  spaces = tab_size - spaces;
               memset( b, ' ', spaces+1 );
               b += spaces;
               i += spaces;
               g_status.line_buff_len += spaces;

               assert( g_status.line_buff_len < MAX_LINE_LENGTH );

            } else {
               *b = *t;
            }
            ++i;
            ++g_status.line_buff_len;
         }
         g_status.copied = TRUE;
         rc = un_copy_line( p, &s_w, TRUE, FALSE );
         dirty = TRUE;
      }
      p = p->next;
   }

   /*
    * IMPORTANT:  we need to reset the copied flag because the cursor may
    * not necessarily be on the last line of the block.
    */
   g_status.copied = FALSE;
   if (dirty)
      file->dirty = GLOBAL;

   return( rc );
}


/*
 * Name:    block_compress_tabs
 * Purpose: Compress tabs in a marked block.
 * Date:    October 31, 1992
 * Passed:  window:  pointer to current window
 * Notes:   Tabs are compressed using the current tab setting.
 */
int  block_compress_tabs( TDE_WIN *window )
{
int  rc;
int  tab_size;
int  dirty;
line_list_ptr p;                /* pointer to block line */
file_infos *file;
TDE_WIN *sw, s_w;
long er;
int  indent_only;

   /*
    * make sure block is marked OK and that this is a LINE block
    */
   rc = un_copy_line( window->ll, window, TRUE, TRUE );
   check_block( &sw );
   file = g_status.marked_file;
   if (rc == ERROR  ||  g_status.marked == FALSE  ||  file->read_only)
      return( ERROR );

   if (file->block_type != LINE) {
      /*
       * can only compress tabs in line blocks
       */
      error( WARNING, window->bottom_line, block20a );
      return( ERROR );
   }

   /*
    * initialize everything
    */
   indent_only = (g_status.command == BlockIndentTabs);
   dirty = FALSE;
   tab_size = file->ptab_size;
   dup_window_info( &s_w, sw );
   s_w.visible = FALSE;
   p  = file->block_start;
   er = file->block_er;
   s_w.rline = file->block_br;
   for (; s_w.rline <= er  &&  !g_status.control_break  &&  rc == OK;
        s_w.rline++) {
      /*
       * if any tabs were found, write g_status.line_buff to file.
       */
      if (entab( p->line, p->len, tab_size, indent_only )) {
         g_status.copied = TRUE;
         rc = un_copy_line( p, &s_w, TRUE, FALSE );
         dirty = TRUE;
      }
      p = p->next;
   }

   /*
    * IMPORTANT:  we need to reset the copied flag because the cursor may
    * not necessarily be on the last line of the block.
    */
   g_status.copied = FALSE;
   if (dirty)
      file->dirty = GLOBAL;

   return( rc );
}
