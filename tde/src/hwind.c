/*******************  start of original comments  ********************/
/*
 * Written by Douglas Thomson (1989/1990)
 *
 * This source code is released into the public domain.
 */

/*
 * Name:    hardware independent screen IO module
 * Purpose: This file contains the code to interface the rest of the
 *           editor to the display and input hardware.
 * File:    hwind.c
 * Author:  Douglas Thomson
 * System:  this file is intended to be system-independent
 * Date:    October 2, 1989
 * Notes:   This is the only module that is allowed to call the hardware
 *           dependent display IO library.
 *          Typically, functions here check whether any action is
 *           necessary (for example, the cursor may already happen to be
 *           in the required position), call hardware dependent functions
 *           to achieve the required effect, and finally update status
 *           information about the current state of the terminal display.
 *          The idea behind this approach is to keep the hardware
 *           dependent code as small and simple as possible, thus making
 *           porting the code easier.
 */
/*********************  end of original comments   ********************/


/*
 * Some routines were added to display current editor modes in the lite bar
 * at the bottom of the screen. Other routines were rewritten in assembly.
 * I feel the need for speed.
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

#include <time.h>

#include "tdestr.h"
#include "common.h"
#include "tdefunc.h"
#include "define.h"


/*
 * Name:    format_time
 * Purpose: format the time according to a format string
 * Author:  Jason Hood
 * Date:    May 21, 1998
 * Passed:  format: format string
 *          buffer: output string
 *          t:      time to convert, or NULL for current
 * Notes:   buffer is assumed to be big enough to hold the string.
 *
 * 010617:  fixed a bug with "%%" (didn't increment past the 2nd one).
 * 010624:  added month and day word alignment.
 * 030320:  added t parameter (for file times).
 */
void format_time( const char *format, char *buffer, struct tm *t )
{
time_t epoch;
int  num;
const char * const *ptr;
static const char * const formats[] = { "%d", "%02d", "%2d" };
static const char * const aligns[]  = { "%-*s", "%*s" };
int  f, a;
int  wid;
int  j, len;
char *b = buffer;

   if (t || (epoch = time( NULL )) != -1) {
      if (t || (t = localtime( &epoch )) != NULL) {
         len = strlen( format );
         for (j = 0; j < len; ++j) {
            if (format[j] != '%')
               *b++ = format[j];
            else {
               if (format[++j] == '%')
                  *b++ = '%';
               else {
                  num = -1;
                  ptr = NULL;
                  f = a = 0;
                  wid = 0;
                  if (format[j] == '0') {
                     f = 1;
                     ++j;
                  } else if (format[j] == '2') {
                     f = 2;
                     ++j;
                  } else if (format[j] == '+') {
                     a = 1;
                     ++j;
                  } else if (format[j] == '-') {
                     a = 2;
                     ++j;
                  }
                  switch (format[j]) {
                     case TS_DAYMONTH:  num = t->tm_mday;
                                        break;

                     case TS_DAYWEEK:   ptr = days[(f != 0)];
                                        num = t->tm_wday;
                                        wid = longest_day;
                                        break;

                     case TS_ENTER:     *b++ = '\n';
                                        break;

                     case TS_12HOUR:    num = t->tm_hour;
                                             if (num > 12) num -= 12;
                                        else if (num == 0) num  = 12;
                                        break;

                     case TS_24HOUR:    num = t->tm_hour;
                                        break;

                     case TS_MONTHNUM:  num = t->tm_mon + 1;
                                        break;

                     case TS_MONTHWORD: ptr = months[(f != 0)];
                                        num = t->tm_mon;
                                        wid = longest_month;
                                        break;

                     case TS_MINUTES:   num = t->tm_min;
                                        break;

                     case TS_MERIDIAN:  ptr = time_ampm;
                                        num = (t->tm_hour >= 12);
                                        break;

                     case TS_SECONDS:   num = t->tm_sec;
                                        break;

                     case TS_TAB:       *b++ = '\t';
                                        break;

                     case TS_YEAR2:     num = t->tm_year % 100;
                                        f   = 1;
                                        break;

                     case TS_YEAR:      num = t->tm_year + 1900;
                                        break;

                     case TS_ZONE:
                                     #if !defined( __DJGPP__ )
                                     # if defined( __WIN32__ )
                                        ptr = (const char**)_tzname;
                                     # else
                                        ptr = (const char**)tzname;
                                     # endif
                                        num = t->tm_isdst;
                                     #else
                                        ptr = (const char **)&t->tm_zone;
                                        num = 0;
                                     #endif
                                        break;

                     default:           *b++ = '%';
                                        *b++ = format[j];
                  }
                  if (ptr != NULL) {
                     if (!a)
                        b += sprintf( b, "%s", ptr[num] );
                     else
                        b += sprintf( b, aligns[a-1], wid, ptr[num] );
                  } else if (num != -1)
                     b += sprintf( b, formats[f], num );
               }
            }
         }
      }
   }
   *b = '\0';
}


/*
 * Name:    show_modes
 * Purpose: show current editor modes in lite bar at bottom of screen
 * Date:    June 5, 1991
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen (file_win_mem)
 * jmh 981129: update the "temporary" modes.
 * jmh 981130: split file_win_mem into file_win and mem_eq.
 */
void show_modes( void )
{
   eol_clear( 0, g_display.mode_line, Color( Mode ) );

   show_graphic_chars( ); /* file & window count */
   show_cur_dir( );       /* memory */
   if (mode.record)
      show_recording( );
   else {
      show_tab_modes( );
      show_indent_mode( );
      show_search_case( );
      show_sync_mode( );
   }
   show_wordwrap_mode( );
   if (mode.draw)
      show_draw_mode( );
   show_trailing( );
   show_control_z( );
   show_insert_mode( );

   if (mode.display_cwd)
      show_cwd( );
}


/*
 * Name:    show_file_count
 * Purpose: show number of open files in lite bar at bottom of screen
 * Date:    June 5, 1991
 * jmh 981129: do nothing if graphic characters are on
 */
void show_file_count( void )
{
char temp[4];

   if (mode.graphics <= 0) {
      sprintf( temp, "%-3d", g_status.file_count );
      s_output( temp, g_display.mode_line, 3, Color( Mode ) );
   }
}


/*
 * Name:    show_window_count
 * Purpose: show total number of windows in lite bar at bottom of screen
 * Passed:  wc:  window count - visible and hidden.
 * Date:    September 13, 1991
 * jmh 981129: do nothing if graphic characters are on
 */
void show_window_count( void )
{
char temp[4];

   if (mode.graphics <= 0) {
      sprintf( temp, "%-3d", g_status.window_count );
      s_output( temp, g_display.mode_line, 8, Color( Mode ) );
   }
}


/*
 * Name:    show_avail_mem
 * Purpose: show available free memory in lite bar at bottom of screen
 * Date:    June 5, 1991
 * jmh 981129: do nothing if cursor direction is not normal
 */
void show_avail_mem( void )
{
long avail_mem;
char temp[12];
char suffix;
int  i;

   if (mode.cur_dir == CUR_RIGHT) {
      /*
       * reverse the sign if avail_mem is larger than a long.
       */
      avail_mem = my_heapavail( );
      if (avail_mem < 0)
         avail_mem = -avail_mem;

      if (avail_mem < 1048577L)
         suffix = ' ';
      else if (avail_mem < 67108865L) {
         avail_mem >>= 10; /* / 1024L */
         suffix = 'k';
      } else {
         avail_mem >>= 20; /* / 1048576L */
         suffix = 'M';
      }
      my_ltoa( avail_mem, temp, 10 );
      i = numlen( avail_mem );
      temp[i] = suffix;
      while (++i < 8)
         temp[i] = ' ';
      temp[8] = '\0';
      s_output( temp, g_display.mode_line, 14, Color( Mode ) );
   }
}


/*
 * Name:    show_tab_modes
 * Purpose: show smart tab mode in lite bar at bottom of screen
 * Date:    October 31, 1992
 *
 * jmh 990501: in smart tab mode, display physical tab size;
 *             in fixed tab mode, display logical tab size.
 * jmh 030329: use sprintf, instead of individual strings.
 */
void show_tab_modes( void )
{
char tab[12];

   sprintf( tab, "%s%c%c%-3d",
            tabs,
            smart[mode.smart_tab],
            tab_mode[g_status.current_file->inflate_tabs],
            mode.smart_tab ? g_status.current_file->ptab_size
                           : g_status.current_file->ltab_size );
   s_output( tab, g_display.mode_line, 22, Color( Mode ) );
}


/*
 * Name:    show_indent_mode
 * Purpose: show indent mode in lite bar at bottom of screen
 * Date:    June 5, 1991
 */
void show_indent_mode( void )
{
   s_output( indent_mode[mode.indent], g_display.mode_line, 32, Color( Mode ) );
}


/*
 * Name:    show_search_case
 * Purpose: show search mode in lite bar
 * Date:    June 5, 1991
 */
void show_search_case( void )
{
   s_output( case_mode[mode.search_case - 1], g_display.mode_line, 40,
             Color( Mode ) );
}


/*
 * Name:    show_sync_mode
 * Purpose: show sync mode in lite bar
 * Date:    January 15, 1992
 */
void show_sync_mode( void )
{
   s_output( sync_mode[mode.sync], g_display.mode_line, 48, Color( Mode ) );
}


/*
 * Name:    show_wordwrap_mode
 * Purpose: display state of word wrap mode
 * Date:    June 5, 1991
 */
void show_wordwrap_mode( void )
{
   s_output( ww_mode[mode.word_wrap], g_display.mode_line, 54, Color( Mode ) );
}


/*
 * Name:     show_trailing
 * Purpose:  show state of trailing flag
 * Date:     June 5, 1991
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen
 */
void show_trailing( void )
{
   c_output( mode.trailing ? MODE_TRAILING : ' ', 65, g_display.mode_line,
             Color( Mode ) );
}


/*
 * Name:     show_control_z
 * Purpose:  show state of control z flag
 * Date:     June 5, 1991
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen
 */
void show_control_z( void )
{
   c_output( mode.control_z ? MODE_CONTROL_Z : ' ', 77, g_display.mode_line,
             Color( Mode ) );
}


/*
 * Name:     show_insert_mode
 * Purpose:  show insert mode in lite bar
 * Date:     June 5, 1991
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen
 * jmh 990404: added set_cursor_size() from toggle_overwrite().
 * jmh 010808: removed the UNIX define.
 */
void show_insert_mode( void )
{
   c_output( mode.insert ? MODE_INSERT : MODE_OVERWRITE, 79,
             g_display.mode_line, Color( Mode ) );
   set_cursor_size( mode.insert ? g_display.insert_cursor :
                                  g_display.overw_cursor );
}


/*
 * Name:     show_graphic_chars
 * Purpose:  show state of graphic characters
 * Author:   Jason Hood
 * Date:     July 24, 1998
 * jmh 981129: overwrite the file/window count with a flashing message.
 * jmh 031123: restore curses cursor position.
 */
void show_graphic_chars( void )
{
#if defined( __UNIX__ )
int  x, y;

   getyx( stdscr, y, x );
#endif

   if (mode.graphics <= 0) {
      s_output( file_win, g_display.mode_line, 1, Color( Mode ) );
      show_file_count( );
      show_window_count( );
   } else {
      graphic_mode[GRAPHIC_SLOT] = graphic_char[mode.graphics-1][5];
      s_output( graphic_mode, g_display.mode_line, 1, Color( Special ) );
   }

#if defined( __UNIX__ )
   move( y, x );
#endif
}


/*
 * Name:    show_cur_dir
 * Purpose: show the cursor update direction
 * Author:  Jason Hood
 * Date:    November 29, 1998
 * Notes:   Overwrite the memory display with a flashing direction string.
 */
void show_cur_dir( void )
{
   if (mode.cur_dir == CUR_RIGHT) {
      s_output( mem_eq, g_display.mode_line, 12, Color( Mode ) );
      show_avail_mem( );
   } else {
      c_repeat( ' ', 10, 12, g_display.mode_line, Color( Mode ) );
      s_output( cur_dir_mode[mode.cur_dir], g_display.mode_line, 12,
                Color( Special ) );
   }
}


/*
 * Name:    show_draw_mode
 * Purpose: indicate draw mode is active
 * Author:  Jason Hood
 * Date:    October 18, 1999
 * Notes:   overwrite the current character display
 */
void show_draw_mode( void )
{
   if (mode.draw)
      s_output( draw_mode, g_display.mode_line, 58, Color( Special ) );
   else
      show_line_col( g_status.current_window );
}


/*
 * Name:    show_undo_mode
 * Purpose: indicate whether undo is group or individual
 * Author:  Jason Hood
 * Date:    November 20, 1999
 * Notes:   use the diagnostic space
 */
void show_undo_mode( void )
{
   show_search_message( UNDO_GROUP + mode.undo_group );
   g_status.wrapped = TRUE;
}


/*
 * Name:    show_undo_move
 * Purpose: indicate whether movement is undone
 * Author:  Jason Hood
 * Date:    May 20, 2001
 * Notes:   use the diagnostic space
 */
void show_undo_move( void )
{
   show_search_message( UNDO_MOVE + mode.undo_move );
   g_status.wrapped = TRUE;
}


/*
 * Name:    show_recording
 * Purpose: indicate a macro is being recorded
 * Author:  Jason Hood
 * Date:    November 29, 1998
 * Notes:   Overwrites the tabs, indent, ingore and sync displays.
 */
void show_recording( void )
{
   s_output( main15, g_display.mode_line, 22, Color( Special ) );
   show_avail_strokes( 0 );
}


/*
 * Name:    show_cwd
 * Purpose: display the cwd at the top of the screen
 * Author:  Jason Hood
 * Date:    February 26, 2003
 */
void show_cwd( void )
{
char cwd[PATH_MAX];

   if (get_current_directory( cwd ) == ERROR)
      strcpy( cwd, "." );
   s_output( reduce_string( cwd, cwd, g_display.ncols, MIDDLE ),
             0, 0, Color( CWD ) );
   eol_clear( strlen( cwd ), 0, Color( CWD ) );
}


/*
 * Name:    my_scroll_down
 * Purpose: display a portion of a window
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   Using the bios scroll functions causes a slightly noticable
 *            "flicker", even on fast VGA monitors.  This is caused by changing
 *            the high-lited cursor line to text color then calling the bios
 *            function to scroll.  Then, the current line must then be
 *            hilited.
 *          This function assumes that win->cline is the current line.
 *
 * jmh 030329: don't blank out lines beyond EOF, since they're already blank;
 *             use the normal syntax color, when appropriate.
 */
void my_scroll_down( TDE_WIN *window )
{
int  i;
TDE_WIN w;              /* scratch window struct for dirty work */

   if (!window->visible  ||  !g_status.screen_display)
      return;

   dup_window_info( &w, window );
   if (w.ll->next == NULL)
      show_eof( &w );
   else {
      show_curl_line( window );
      for (i = w.bottom_line - w.cline; i > 0; i--) {
         ++w.cline;
         ++w.rline;
         w.ll = w.ll->next;
         if (w.ll->len != EOF)
            update_line( &w );
         else {
            show_eof( &w );
            break;
         }
      }
   }
   if (w.ll->len == EOF  &&  w.cline != w.bottom_line) {
      ++w.cline;
      window_eol_clear( &w, (w.syntax) ? syntax_color[0] : Color( Text ) );
   }
}


/*
 * Name:    eol_clear
 * Purpose: To clear the line from col to max columns
 * Date:    June 5, 1991
 * Passed:  col:   column to begin clear
 *          line:  line to clear
 *          attr:  color to clear
 */
void eol_clear( int col, int line, int attr )
{
   c_repeat( ' ', g_display.ncols, col, line, attr );
}


/*
 * Name:    window_eol_clear
 * Purpose: To clear the line from start_col to end_col
 * Date:    June 5, 1991
 * Passed:  window:  pointer to window
 *          attr:  color to clear
 */
void window_eol_clear( TDE_WIN *window, int attr )
{
int  col;

   if (!g_status.screen_display)
      return;

   col = window->left;
   c_repeat( ' ', window->end_col - col + 1, col, window->cline, attr );
}


/*
 * Name:    n_output
 * Purpose: display a number
 * Author:  Jason Hood
 * Date:    November 19, 2003
 * Passed:  num:  number to display
 *          wid:  width to display it
 *          col:  column
 *          line: line number
 *          attr: color
 * Notes:   if wid is 0, the number will simply be displayed at col; if wid
 *           is positive, it will be displayed at col and padded with spaces
 *           to fit wid; if negative, it will be right aligned.  If the number
 *           is too big to fit, the most significant digits are dropped.
 *          assumes num is positive.
 */
void n_output( long num, int wid, int col, int line, int attr )
{
char buf[16];
int  len;
int  spc;

   my_ltoa( num, buf, 10 );
   if (wid) {
      len = numlen( num );
      spc = col;
      if (wid < 0) {
         wid = -wid;
         if (wid - len > 0)
            col += wid - len;
      } else
         spc += len;
      if (len >= wid)
         buf[wid] = '\0';
      else
         c_repeat( ' ', wid - len, spc, line, attr );
   }
   s_output( buf, line, col, attr );
}


/*
 * Name:    combine_strings
 * Purpose: stick 3 strings together
 * Date:    June 5, 1991
 * Passed:  buff:    buffer to hold concatenation of 3 strings
 *          s1:  pointer to string 1
 *          s2:  pointer to string 2
 *          s3:  pointer to string 3
 *
 * jmh 021031: test for s3 being NULL (instead of requiring "").
 * jmh 021106: if buff is line_out, truncate s2 such that s1+s2+s3 will fit
 *              on the screen.
 */
void combine_strings( char *buff, const char *s1, const char *s2,
                                  const char *s3 )
{
char temp[MAX_COLS];
int  len;

   if (buff == line_out) {
      len = g_display.ncols - 1 - strlen( s1 );
      if (s3)
         len -= strlen( s3 );
      s2 = reduce_string( temp, s2, len, MIDDLE );
   }

#if defined( __DJGPP__ ) || defined( __TURBOC__ )
   if (s3)
      strcpy( stpcpy( stpcpy( buff, s1 ), s2 ), s3 );
   else
      strcpy( stpcpy( buff, s1 ), s2 );
#else
   strcpy( buff, s1 );
   strcat( buff, s2 );
   if (s3)
      strcat( buff, s3 );
#endif
}


/*
 * Name:    reduce_string
 * Purpose: reduce the length of a string
 * Author:  Jason Hood
 * Date:    November 5, 2002
 * Passed:  buf:  place to store the reduced string
 *          str:  the string to reduce
 *          wid:  the maximum length of the string
 *          flag: how the string should be reduced
 * Returns: buf
 * Notes:   flag can be BEGIN, MIDDLE or END, where that portion of the string
 *           is replaced with "..." (ie. flag indicates the bit removed, not
 *           the bit remaining). eg: "a long string" reduced to eight chars:
 *           BEGIN:  "...tring"
 *           MIDDLE: "a ...ing"
 *           END:    "a lon..."
 *          buf is assumed to be at least wid+1 characters.
 */
char *reduce_string( char *buf, const char *str, int wid, int flag )
{
int  len;
int  wid2;

   len = strlen( str );
   if (len <= wid)
      memcpy( buf, str, len + 1 );
   else {
      wid -= 3;
      if (flag == BEGIN)
         join_strings( buf, "...", str + len - wid );
      else if (flag == END) {
         memcpy( buf, str, wid );
         strcpy( buf + wid, "..." );
      } else /* flag == MIDDLE */ {
         wid2 = wid / 2;
         memcpy( buf, str, wid2 );
         join_strings( buf + wid2, "...", str + len - (wid2 + (wid & 1)) );
      }
   }
   return( buf );
}


/*
 * Name:    make_ruler
 * Purpose: make ruler with tabs, tens, margins, etc...
 * Date:    June 5, 1991
 *
 * jmh 991110: made the ruler global;
 *             use the actual tens digit, not div 10.
 */
void make_ruler( void )
{
register char *p;
char *end;
char tens;

   memset( ruler_line, RULER_FILL, MAX_LINE_LENGTH+MAX_COLS );
   end = ruler_line + MAX_LINE_LENGTH+MAX_COLS;
   *end = '\0';

   /*
    * indicate the "fives" column
    */
   for (p = ruler_line + 4; p < end; p += 10)
      *p = RULER_TICK;

   /*
    * put a tens digit in the tens column
    */
   tens = '1';
   for (p = ruler_line + 9; p < end; p += 10) {
      *p = tens;
      if (++tens > '9')
         tens = '0';
   }

   ruler_line[mode.parg_margin]  = PGR_CHAR;
   ruler_line[mode.left_margin]  = (char)(unsigned char)LM_CHAR;
   ruler_line[mode.right_margin] = (mode.right_justify) ? RM_CHAR_JUS :
                                                          RM_CHAR_RAG;
}


/*
 * Name:    show_ruler
 * Purpose: show ruler with tens, margins, etc...
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 *
 * jmh 991110: display as an offset into the global ruler.
 */
void show_ruler( TDE_WIN *window )
{
register TDE_WIN *win;
char *ruler;
char save;
int  len;

   win = window;
   if (win->ruler && win->visible) {
      ruler = ruler_line + win->bcol;
      len = win->end_col - win->start_col + 1;
      save = ruler[len];
      ruler[len] = '\0';
      s_output( ruler, win->top+1, win->start_col, Color( Ruler ) );
      ruler[len] = save;

      if (mode.line_numbers)
         c_repeat( ' ', win->file_info->len_len, win->left, win->top+1,
                   Color( Ruler ) );

      refresh( );
   }
}


/*
 * Name:    show_ruler_char
 * Purpose: show ruler character under ruler pointer
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 */
void show_ruler_char( TDE_WIN *window )
{
register TDE_WIN *win;

   win = window;
   if (win->ruler && win->visible) {
      c_output( ruler_line[win->rcol], win->ccol, win->top+1, Color( Ruler ) );
      refresh( );
   }
}


/*
 * Name:    show_ruler_pointer
 * Purpose: show ruler pointer
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 *
 * jmh 050721: cursor cross assumes show_ruler_char is called first to remove
 *              the old one before calling this to show the new.
 */
void show_ruler_pointer( TDE_WIN *window )
{
   if (window->ruler && window->visible) {
      c_output( RULER_PTR, window->ccol, window->top+1, Color( Pointer ) );
      refresh( );
   }
}


/*
 * Name:    show_all_rulers
 * Purpose: make and show all rulers in all visible windows
 * Date:    June 5, 1991
 */
void show_all_rulers( void )
{
register TDE_WIN *wp;

   make_ruler( );
   wp = g_status.window_list;
   while (wp != NULL) {
      if (wp->visible) {
         show_ruler( wp );
         show_ruler_pointer( wp );
      }
      wp = wp->next;
   }
}


/*
 * Name:    make_popup_ruler
 * Purpose: create the popup ruler
 * Author:  Jason Hood
 * Date:    July 18, 2005
 * Passed:  window:  window to display ruler
 *          line:    buffer for ruler characters
 *          attr:    buffer for ruler color
 *          len:     width of the window
 *          line1:   the units screen line
 *          line2:   the tens screen line
 * Notes:   the buffers are relative to the window's base column
 */
void make_popup_ruler( TDE_WIN *window, text_ptr line, unsigned char *attr,
                       int len, int line1, int line2 )
{
int  col;
int  num, digit;

   /*
    * determine the starting column and number
    */
   col = ruler_win.rcol;
   if (col >= window->bcol + len)
      return;

   if (col < window->bcol) {
      num = window->bcol - ruler_win.rcol + 1;
      col = 0;
   } else {
      num = 1;
      col -= window->bcol;
      len -= col;
   }

   if (num == 1  &&  col-1 >= 0) {
      attr[col-1] = Color( Ruler );
      line[col-1] = (window->rline == ruler_win.rline || window->cline == line1)
                    ? RULER_EDGE : (line2 < line1) ? RULER_CRNRA : RULER_CRNRB;
   }
   if (window->rline == ruler_win.rline)
      return;

   memset( attr + col, Color( Ruler ), len );

   digit = (num % 10) + '0';
   num /= 10;
   do {
      if (window->cline == line1) {
         line[col] = digit;
         if (digit == '0'  &&  line1 == line2)
            line[col] = windowletters[(num - 1) % strlen( windowletters )];
      } else {
         if (digit == '0')
            my_ltoa( num, (char*)line + col - numlen( num ) + 1, 10 );
         else
            line[col] = RULER_LINE;
      }
      if (++digit > '9') {
         digit = '0';
         ++num;
      }
      ++col;
   } while (--len);
}


/*
 * Name:    popup_ruler
 * Purpose: display and manipulate a ruler
 * Author:  Jason Hood
 * Date:    July 18, 2005
 * Passed:  window:  pointer to current window
 * Notes:   pops up a ruler to count columns; also uses the line number
 *           display to count lines.
 */
int  popup_ruler( TDE_WIN *window )
{
TDE_WIN dw;                             /* display window */
DISPLAY_BUFF;
int  i;
long key;
int  func;
line_list_ptr ll;
int  max, len;
int  wid, hyt;
int  bcol;
long rline, last;
int  lnum = FALSE;

   dup_window_info( &dw, window );
   hyt = dw.bottom_line - dw.top_line;
   if (hyt < 1) {
      /*
       * need at least two lines
       */
      error( WARNING, dw.bottom_line, ruler_bad );
      return( ERROR );
   }

   xygoto( -1, -1 );
   SAVE_LINE( g_display.mode_line );
   eol_clear( 0, g_display.mode_line, Color( Help ) );
   i = (g_display.ncols - (int)strlen( ruler_help )) / 2;
   if (i < 0)
      i = 0;
   s_output( ruler_help, g_display.mode_line, i, Color( Help ) );

   dup_window_info( &ruler_win, window );
   ruler_win.bcol = 0;

   /*
    * move dw to the top line
    */
   while (dw.cline != dw.top_line) {
      dw.ll = dw.ll->prev;
      --dw.rline;
      --dw.cline;
   }

   /*
    * if EOF occurs in the top three lines, scroll back
    */
   i = 0;
   if (ruler_win.cline == dw.top_line  &&
       (dw.rline == dw.file_info->length ||
        dw.rline == dw.file_info->length+1))
      i = 2;
   else if (ruler_win.cline == dw.top_line+1  &&
            (dw.rline == dw.file_info->length ||
             dw.rline == dw.file_info->length-1))
      i = 1;
   if (i) {
      dw.file_info->dirty = TRUE;
      do {
         if (dw.rline == 0  ||
             dw.top_line + (int)(ruler_win.rline - dw.rline) == dw.bottom_line)
            break;
         --dw.rline;
         dw.ll = dw.ll->prev;
      } while (--i != 0);
   }

   /*
    * find the top line of the last page
    */
   last = dw.file_info->length + 1 - hyt;
   if (last < 0)
      last = 0;

   wid = dw.end_col - dw.start_col + 1;
   rline = dw.rline;
   if (mode.cursor_cross)
      bcol = -1;
   else {
      bcol = dw.bcol;
      if (!dw.file_info->dirty)
         dw.file_info->dirty = (mode.line_numbers) ? LOCAL | RULER : RULER;
   }
   for (;;) {
      if (dw.bcol < 0)
         dw.bcol = 0;
      else if (dw.bcol > MAX_LINE_LENGTH - wid)
         dw.bcol = MAX_LINE_LENGTH - wid;
      if (bcol != dw.bcol  ||  rline != dw.rline  ||  dw.file_info->dirty) {
         display_current_window( &dw );
         show_line_col( &ruler_win );
         bcol  = dw.bcol;
         rline = dw.rline;
         dw.file_info->dirty = FALSE;
      }
      key = getkey();
      func = (key == RTURN) ? Rturn :
             (key == ESC)   ? AbortCommand :
             getfunc( key );
      if (func == Rturn || func == AbortCommand)
         break;
      switch (func) {
         /* Left */
         case CharLeft:
         case StreamCharLeft:
            if (ruler_win.rcol > 0) {
               --ruler_win.rcol;
               if (ruler_win.rcol < dw.bcol)
                  --dw.bcol;
               else
                  dw.file_info->dirty = RULER;
            }
            break;

         /* Right */
         case CharRight:
         case StreamCharRight:
            if (ruler_win.rcol < MAX_LINE_LENGTH) {
               ++ruler_win.rcol;
               /* keep half the ruler visible */
               if (ruler_win.rcol >= dw.bcol + wid/2)
                  ++dw.bcol;
               else
                  dw.file_info->dirty = RULER;
            }
            break;

         /* Up */
         case LineUp:
            if (ruler_win.rline < dw.rline || ruler_win.rline > dw.rline + hyt){
               move_to_line( &ruler_win, (ruler_win.rline < dw.rline)
                                         ? dw.rline : dw.rline + hyt, FALSE );
               dw.file_info->dirty = LOCAL | RULER;
            } else if (dec_line( &ruler_win, TRUE )) {
               if (ruler_win.rline == dw.rline - 1) {
                  --dw.rline;
                  dw.ll = dw.ll->prev;
               } else
                  dw.file_info->dirty = LOCAL | RULER;
            }
            break;

         /* Down */
         case LineDown:
            if (ruler_win.rline < dw.rline || ruler_win.rline > dw.rline + hyt){
               move_to_line( &ruler_win, (ruler_win.rline < dw.rline)
                                         ? dw.rline : dw.rline + hyt, FALSE );
               dw.file_info->dirty = LOCAL | RULER;
            } else if (inc_line( &ruler_win, TRUE )) {
               if (ruler_win.rline == dw.rline + hyt + 1) {
                  ++dw.rline;
                  dw.ll = dw.ll->next;
               } else
                  dw.file_info->dirty = NOT_LOCAL | RULER;
            }
            break;

         case StartOfLine:
            ruler_win.rcol = 0;
            dw.file_info->dirty = RULER;
            break;

         /* Ctrl+Left */
         case WordLeft:
            --dw.bcol;
            break;

         /* Ctrl+Right */
         case WordRight:
            ++dw.bcol;
            break;

         /* Ctrl+Up */
         case ScrollUpLine:
            if (dw.rline > 0) {
               --dw.rline;
               dw.ll = dw.ll->prev;
            }
            break;

         /* Ctrl+Down */
         case ScrollDnLine:
            if (dw.rline < last) {
               ++dw.rline;
               dw.ll = dw.ll->next;
            }
            break;

         /* Home */
         case BegOfLine:
            dw.bcol = 0;
            break;

         /* End */
         case EndOfLine:
            /*
             * find the longest line on the screen
             */
            max = 0;
            ll = dw.ll;
            for (i = hyt; i >= 0; --i) {
               len = find_end( ll->line, ll->len, dw.file_info->inflate_tabs,
                                                  dw.file_info->ptab_size );
               if (len > max)
                  max = len;
               ll = ll->next;
               if (ll->len == EOF)
                  break;
            }
            dw.bcol = max - wid;
            break;

         case HalfScreenLeft:
            dw.bcol -= wid / 2;
            break;

         case ScreenLeft:
            dw.bcol -= wid;
            break;

         case HalfScreenRight:
            dw.bcol += wid / 2;
            /*if (dw.bcol < 0)
               dw.bcol = INT_MAX;*/
            break;

         case ScreenRight:
            dw.bcol += wid;
            /*if (dw.bcol < 0)
               dw.bcol = INT_MAX;*/
            break;

         /* PgUp */
         case ScreenUp:
         case HalfScreenUp:
            if (dw.rline > 0) {
               i = hyt;
               if (func == HalfScreenUp)
                  i /= 2;
               for (; i >= 0; --i) {
                  dw.ll = dw.ll->prev;
                  if (--dw.rline == 0)
                     break;
               }
            }
            break;

         /* PgDn */
         case ScreenDown:
         case HalfScreenDown:
            if (dw.rline < last) {
               i = hyt;
               if (func == HalfScreenDown)
                  i /= 2;
               for (; i >= 0; --i) {
                  dw.ll = dw.ll->next;
                  if (++dw.rline == last)
                     break;
               }
            }
            break;

         case TopOfFile:
            dw.rline = 0;
            dw.ll = dw.file_info->line_list;
            break;

         case EndOfFile:
            move_to_line( &dw, last, TRUE );
            break;

         case ToggleLineNumbers:
            if (!mode.line_numbers) {
               lnum = mode.line_numbers = TRUE;
               dw.file_info->len_len = numlen( dw.file_info->length ) + 1;
               dw.start_col += dw.file_info->len_len;
               wid -= dw.file_info->len_len;
               bcol = -1;
            }
            break;
      }
   }
   ruler_win.rline = -1;

   RESTORE_LINE( g_display.mode_line );
   xygoto( window->ccol, window->cline );
   if (lnum) {
      mode.line_numbers = FALSE;
      dw.start_col -= dw.file_info->len_len;
   }
   display_current_window( window );

   return( OK );
}


/*
 * Name:    show_help
 * Purpose: display help screen for certain functions
 * Author:  Jason Hood
 * Date:    August 9, 1998
 * Passed:  nothing (uses g_status.command)
 * Returns: OK if screen was restored;
 *          ERROR if screen was redrawn;
 *          1 if key pressed was Help.
 * Notes:   center the help display horizontally (assuming all strings are
 *           equal length), but above center vertically.
 *          if there's enough memory, save and restore screen contents,
 *           otherwise just redraw the screen.
 *
 * jmh 990410: added Status screen.
 * jmh 991022: added Help and Credit screens.
 * jmh 991025: display the credit screen a bit higher.
 * jmh 991110: added CharacterSet.
 * jmh 010605: added Statistics screen.
 * jmh 010607: removed extra keypress at viewer help.
 * jmh 050710: moved the normal help after viewer help to get_help.
 */
int  show_help( void )
{
const char * const *pp;
int  row;
int  col;
Char *buffer;
int  wid;
int  len;
int  rc = OK;

   switch (g_status.command) {
      case RegXForward:
      case RegXBackward:
      case RepeatRegXForward:
      case RepeatRegXBackward:
      case DefineSearch:
      case RepeatSearch:
         pp = regx_help;
         break;

      case DefineGrep:
      case RepeatGrep:
         if (grep_dialog->n == IDE_G_FILES)
            pp = wildcard_help;
         else if (CB_G_RegX)
            pp = regx_help;
         else
            return( OK );
         break;

      case ReplaceString:
         if (!CB_R_RegX)
            return( OK );
         pp = (replace_dialog->n == IDE_R_PATTERN) ? regx_help : replace_help;
         break;

      case StampFormat:
         pp = stamp_help;
         break;

      case BorderBlock:
         pp = border_help;
         break;

      case Execute:
         pp = exec_help;
         break;

      case Status:
      case Statistics:
         pp = (const char**)stat_screen;
         break;

      case CharacterSet:
         pp = char_help;
         break;

      case 0:                   /* Don't call help when in insert_overwrite() */
      case About:
         pp = credit_screen;
         break;

      default:
         pp = NULL;
         break;
   }
   if (pp == NULL) {
      if (g_status.command != Help)
         return( OK );
      len = help_dim[g_status.current_file->read_only][0];
      wid = help_dim[g_status.current_file->read_only][1];
   } else {
      for (len = 0; pp[len] != NULL; ++len) ;
      wid = strlen( pp[0] );
   }
   row = (g_display.mode_line - len) / (2 + (g_status.command == 0));
   col = (g_display.ncols - wid) / 2;
   if (row < 0)
      row = 0;
   if (col < 0)
      col = 0;

   g_display.output_space = g_display.frame_space;
   buffer =  (g_status.command == 0) ? NULL :
            (Char *)malloc( (wid + 4) * (len + 1) * sizeof(Char) );
   if (buffer != NULL)
      save_area( buffer, wid, len, row, col );

   xygoto( -1, -1 );
   show_strings( pp, len, row, col );
   if (g_status.command == CharacterSet)
      c_output( '\0', col+ZERO_COL, row+ZERO_ROW, Color( Help ) );
   shadow_area( wid, len, row, col );
   if (g_status.command == Help && g_status.current_file->read_only) {
      g_status.viewer_key = TRUE;
      if (getfunc( getkey( ) ) == Help)
         rc = 1;
   } else
      if (g_status.command != 0)        /* No pause required for credits */
         getkey( );

   if (buffer != NULL) {
      restore_area( buffer, wid, len, row, col );
      g_display.output_space = FALSE;
      free( buffer );
   } else {
      g_display.output_space = FALSE;
      if (g_status.current_window != NULL) {
         redraw_screen( g_status.current_window );
         if (rc != 1)
            rc = ERROR;
      }
   }

   return( rc );
}


/*
 * Name:    show_strings
 * Purpose: display strings from an array
 * Author:  Jason Hood
 * Date:    August 9, 1998
 * Passed:  strings: the array to display
 *          num:     number of strings to display
 *          row:     starting line position
 *          col:     starting column position
 * Returns: nothing
 * Notes:   Assumes there are at least num strings in the array.
 *          Will stop should the bottom line be reached.
 *          Uses the help color.
 * jmh 991022: special case for Help.
 */
void show_strings( const char * const *strings, int num, int row, int col )
{
int  max_row = g_display.mode_line + 1;
int  line;

   if (max_row > row + num)
      max_row = row + num;

   if (strings == NULL)
      for (line = 0; row < max_row; ++line, ++row)
         s_output( help_screen[g_status.current_file->read_only][line],
                   row, col, Color( Help ) );
   else
      for (; row < max_row; strings++, row++)
         s_output( *strings, row, col, Color( Help ) );
}


/*
 * Name:    adjust_area
 * Purpose: modify the area to take into account spacing and shadow
 * Author:  Jason Hood
 * Date:    October 23, 1999
 * Passed:  wid: pointer to width
 *          len: pointer to length
 *          row: pointer to starting row
 *          col: pointer to starting column
 *          swd: pointer to shadow width
 * Returns: none, but may modify above
 * Notes:   Assumes the values are valid on entry, but adjusts them to take
 *           into account spaces and shadow and ensure they remain valid.
 *          If swd is not NULL, set it to the width of the shadow being used.
 *           To determine if the bottom shadow is to be drawn, compare the old
 *           length with the new.
 */
void adjust_area( int *wid, int *len, int *row, int *col, int *swd )
{
int  sw;

   if (g_display.output_space) {
      if (*col + *wid < g_display.ncols)
         ++*wid;
      if (*col != 0) {
         --*col;
         ++*wid;
      }
   }
   if (g_display.shadow) {
      sw = g_display.ncols - (*col + *wid);
      if (sw > g_display.shadow_width)
         sw = g_display.shadow_width;
      *wid += sw;
      if (swd != NULL)
         *swd = sw;
      if (*row + *len < g_display.nlines)
         ++*len;
   }
}
