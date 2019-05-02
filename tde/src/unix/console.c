/*
 * All video and keyboard functions were gathered into one file.
 *
 * In version 3.2, this file was split into 3 sections:  1) unix curses
 * made to look and feel like a PC, 2) our PC hardware specific stuff,
 * and 3) hardware independent stuff.
 *
 * This file contains the keyboard and video i/o stuff.  Most of this stuff
 * is specific to the PC hardware, but it should be easily modified when
 * porting to other platforms.  With a few key functions written in
 * assembly, we have fairly fast video performance and good keyboard control.
 * Incidentally, the commented C code in the functions perform the same
 * function as the assembly code.  In earlier versions of TDE, I didn't
 * update the commented C code when I changed the assembly code.  The
 * C and assembly should be equivalent in version 2.2.
 *
 * Although using a curses type package would be more portable, curses
 * can be slow on PCs.  Let's keep our video and keyboard routines in
 * assembly.  I feel the need for speed.
 *
 * Being that TDE 2.2 keeps an accurate count of characters in each line, we
 * can allow the user to enter any ASCII or Extended ASCII key.
 *
 * Determining the video adapter type on the PC requires a function.  In
 *  TDE, that function is:
 *
 *                void video_config( struct vcfg *cfg )
 *
 * video_config( ) is based on Appendix C in _Programmer's Guide to
 *  PC & PS/2 Video Systems_ by Richard Wilton.
 *
 * See:
 *
 *   Richard Wilton, _Programmer's Guide to PC & PS/2 Video Systems_,
 *    Microsoft Press, Redmond, Washington, 1987, Appendix C, pp 511-521.
 *    ISBN 1-55615-103-9.
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
 * Date:             August 29, 1993 version 3.1
 * Date:             November 13, version 3.2
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
#include "keys.h"
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <sys/io.h>


#if defined( PC_CHARS )
# define pc_chars A_ALTCHARSET
#else
# define pc_chars 0
#endif


int  stdoutput;                 /* the redirected stdout */
int  xterm;                     /* running in an xterm? */

static int  port_access;        /* is VGA IO allowed? */


static void set_colors( int );
static chtype c_xlat( int, int );


/*
 * Name:    console_init
 * Purpose: initialise the console (video and keyboard)
 * Author:  Jason Hood
 * Date:    October 23, 2002
 * Passed:  cfg:  pointer to hold video config data
 * Notes:   initialises both video and keyboard hardware.
 *          moved some stuff from main.c into here.
 *
 * jmh 031125: turn on bright background (VGA, console, root)
 */
void console_init( struct vcfg *cfg )
{
char *term;

   /*
    * if output has been redirected, remember where and restore it to screen.
    */
   if (g_status.output_redir) {
      stdoutput = dup( 1 );
      freopen( STDFILE, "w", stdout );
   }

   term = getenv( "TERM" );
   xterm = (term != NULL  &&  strncmp( term, "xterm", 5 ) == 0);

   page( 1 );
   video_config( cfg );

   if (!xterm && cfg->color) {
      if (ioperm( 0x3c0, 0x1b, TRUE ) == 0) {
         port_access = TRUE;
         inb( 0x3da );          /* switch flip-flop to index mode */
         outb( 0x30, 0x3c0 );   /* display enable, register 0x10 */
         outb( 0x04, 0x3c0 );   /* 9-bit chars, blink mode off */
      }
   }
}


/*
 * Name:    console_suspend
 * Purpose: temporarily shutdown the console
 * Author:  Jason Hood
 * Date:    October 23, 2002
 * Notes:   readies the console for a shell
 */
void console_suspend( void )
{
   bkgdset( COLOR_PAIR( 0 ) );
   cls( );
   endwin( );
   page( 0 );
   set_overscan_color( 0 );
}


/*
 * Name:    console_resume
 * Purpose: restore the console
 * Author:  Jason Hood
 * Date:    October 23, 2002
 * Notes:   restores the console for TDE
 */
void console_resume( int pause )
{
   if (pause)
      getkey( );

   page( 1 );
   refresh( );
   set_overscan_color( Color( Overscan ) );
}


/*
 * Name:    console_exit
 * Purpose: resets the console
 * Author:  Jason Hood
 * Date:    October 23, 2002
 * Notes:   resets the console after TDE has finished
 */
void console_exit( void )
{
   /*
    * shutdown curses.
    */
   curs_set( CURSES_SMALL );
   bkgdset( COLOR_PAIR( 0 ) );
   nl( );
   noraw( );
   endwin( );

   page( 0 );

   /*
    * restore stdout to the redirection.
    */
   if (g_status.output_redir)
      dup2( stdoutput, 1 );
}


/*
 * Name:    video_config
 * Purpose: initialise the curses package
 * Date:    November 13, 1993
 * Passed:  cfg:  flag to initialise or shut down
 * Notes:   curses has three (?) cursor shapes:  invisible, normal, large.
 *
 * jmh 990213: moved some stuff here from hw_initialise().
 * jmh 990408: moved some stuff here from terminate() - if cfg is NULL, shut
 *              down curses, otherwise initialise curses.
 * jmh 990414: reset cursor size, background color.
 * jmh 010808: set scrollok to FALSE.
 * jmh 030403: moved curses shutdown to console_exit().
 */
void video_config( struct vcfg *cfg )
{
   initscr( );
   start_color( );
   noecho( );
   raw( );
   nonl( );
   scrollok( stdscr, FALSE );
   nodelay( stdscr, TRUE );

   g_display.cursor[SMALL_CURSOR]  = CURSES_SMALL;
   g_display.cursor[MEDIUM_CURSOR] =
   g_display.cursor[LARGE_CURSOR]  = CURSES_LARGE;
   g_display.curses_cursor = CURSES_INVISBL;

   cfg->color = has_colors( );
   g_display.adapter = (cfg->color) ? CGA : MDA;
   set_colors( cfg->color );
   getmaxyx( stdscr, cfg->rows, cfg->cols );
#if defined( __linux__ )
   g_display.shadow_width = (xterm || cfg->rows > 30) ? 1 : 2;
#else
   g_display.shadow_width = 1;
#endif
}


/*
 * Name:    set_colors
 * Purpose: translate curses colors to DOS colors.
 * Date:    October 24, 1999
 * Passed:  color:  TRUE for color, FALSE for monochrome.
 * Notes:   moved from hw_initialize().
 */
static void set_colors( int color )
{
int  i;
int  j;
/*
 * Ensure the colors are in the same order as DOS.
 */
static const int col[8] = {
   COLOR_BLACK, COLOR_BLUE,    COLOR_GREEN,  COLOR_CYAN,
   COLOR_RED,   COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE
};

   if (color) {
      if (COLOR_PAIRS >= 64) {
         for (i = 0; i < 64; ++i)
            init_pair( i, col[i%8], col[i/8] );
         for (j = 0; j < 8; ++j) {
            for (i = 0; i < 8; ++i) {
               tde_color_table[i+j*16]   = COLOR_PAIR( (i+j*8) );
               tde_color_table[i+j*16+8] = COLOR_PAIR( (i+j*8) ) | A_BOLD;
            }
         }
         /*
          * jmh 010629: pair 0 can't be initialised (remains white on black),
          *             which means color 8 ("bright black" on black) is not
          *             set correctly, causing the shadow to look wrong.
          *             Let's use blue on blue for "bright black" on black.
          */
         init_pair( 9, COLOR_BLACK, COLOR_BLACK );
         tde_color_table[8] = COLOR_PAIR( 9 ) | A_BOLD;
      } else {
         init_pair( 0, COLOR_WHITE, COLOR_BLACK );
         init_pair( 1, COLOR_WHITE, COLOR_BLUE );
         init_pair( 2, COLOR_BLUE, COLOR_BLACK );
         init_pair( 3, COLOR_YELLOW, COLOR_CYAN );
         init_pair( 4, COLOR_WHITE, COLOR_RED );
         init_pair( 5, COLOR_GREEN, COLOR_MAGENTA );
         init_pair( 6, COLOR_GREEN, COLOR_BLACK );
         init_pair( 7, COLOR_BLUE, COLOR_WHITE );

         /*
          * lets try to emulate the PC color table.  in normal color mode,
          *  there are 8 background colors and 16 foreground colors.
          */
         for (i=0; i<8; i++) {
            tde_color_table[i]     = COLOR_PAIR( 0 );
            tde_color_table[i+16]  = COLOR_PAIR( 1 );
            tde_color_table[i+32]  = COLOR_PAIR( 2 );
            tde_color_table[i+48]  = COLOR_PAIR( 3 );
            tde_color_table[i+64]  = COLOR_PAIR( 4 );
            tde_color_table[i+80]  = COLOR_PAIR( 5 );
            tde_color_table[i+96]  = COLOR_PAIR( 6 );
            tde_color_table[i+112] = COLOR_PAIR( 7 );
         }

         for (i=0; i<8; i++) {
            tde_color_table[i+8]   = COLOR_PAIR( 0 ) | A_BOLD;
            tde_color_table[i+24]  = COLOR_PAIR( 1 ) | A_BOLD;
            tde_color_table[i+40]  = COLOR_PAIR( 2 ) | A_BOLD;
            tde_color_table[i+56]  = COLOR_PAIR( 3 ) | A_BOLD;
            tde_color_table[i+72]  = COLOR_PAIR( 4 ) | A_BOLD;
            tde_color_table[i+88]  = COLOR_PAIR( 5 ) | A_BOLD;
            tde_color_table[i+104] = COLOR_PAIR( 6 ) | A_BOLD;
            tde_color_table[i+120] = COLOR_PAIR( 7 ) | A_BOLD;
         }
      }
   } else {
      for (i = 0; i < 128; ++i)
         tde_color_table[i] = A_NORMAL;
      tde_color_table[1]    = A_UNDERLINE;
      tde_color_table[15]   = A_BOLD;
      tde_color_table[112]  = A_REVERSE;
      tde_color_table[127]  = A_STANDOUT;
   }
   for (i = 0; i < 128; ++i)
      tde_color_table[i+128] = tde_color_table[i] | A_BLINK;
}


/*
 * Name:    kbReadShiftState
 * Purpose: to determine the state of shift, control and alt keys
 * Author:  Jason Hood
 * Date:    April 17, 1999
 * Passed:  Nothing
 * Returns: Shift state
 * Notes:   based on the same function in Rhide 1.4 by Robert Hoehne.
 */
static int  kbReadShiftState( void )
{
int  arg = 6;   /* TIOCLINUX function #6 */
int  shift;

  if (ioctl( 0, TIOCLINUX, &arg ) != -1) {
     shift = 0;
     if (arg & 1)
        shift |= _SHIFT;
     if (arg & 4)
        shift |= _CTRL;
     if (arg & (2 | 8))
        shift |= _ALT;
  } else
     shift = ERROR;
  return( shift );
}


/*
 * Name:    getkey
 * Purpose: use unix curses to get keyboard input
 * Date:    November 13, 1993
 * Passed:  None
 * Notes:   the getch( ) function is not part of the ANSI C standard.
 *            most (if not all) PC C compilers include getch( ) with the
 *            conio stuff.  in unix, getch( ) is in the curses package.
 *          in TDE, lets try to map the curses function keys to their
 *            "natural" positions on PC hardware.  most of the key mapping
 *            is at the bottom of default.c.
 *          this function is based on the ncurses package in Linux.  It
 *            probably needs to be modified for curses in other unices.
 *
 * jmh 980725: translate keys.
 * jmh 990417: now that the shift state is known, map keys to DOS equivalents.
 *              I don't think all the keys I've defined here are actually
 *              available (such as those used for terminal switching).
 * jmh 990419: eat escape prefix characters (type esc-esc, 'cos it's quicker;
 *              is there any way to remove that delay?).
 * jmh 990426: use KDGKBLED ioctl to test for NumLock.
 * jmh 020908: updated to new keyboard mapping.
 * jmh 031123: translate escape sequences myself (remove curses_to_tde[]).
 */
long getkey( void )
{
int  key, ch;
long new_key;
int  shift_state;
int  alt_esc;
char escape[12];
int  esclen;
int  j, k;

go_again:
   g_status.control_break = FALSE;
   alt_esc = FALSE;
   key = esclen = 0;

   while ((ch = getch( )) == ERR) ;

#if 0 /* intended for testing Unix escape sequences */
   if (ch == 13)  /* ^M / enter */
     return( _FUNCTION|Rturn );
   if (ch == 17)  /* ^Q */
     return( _FUNCTION|Quit );
   if (ch == 28)  /* ^\ */
     return( _FUNCTION|PullDown );
   return( ch );
#endif

   if (g_status.control_break)
      return( _CONTROL_BREAK );

   if (ch >= 128)
      return( ch );

   if (ch == 27) {
      while ((ch = getch()) != ERR) {
         if (esclen == sizeof(escape) - 1) {
            while (getch() != ERR) ;
            goto go_again;
         }
         escape[esclen++] = ch;
      }
      if (esclen == 0)
         ch = 27;
      else if (esclen == 1) {
         alt_esc = TRUE;
         ch = escape[0];
      } else {
         if (escape[0] == 27) {
            alt_esc = TRUE;
            key = 1;
            --esclen;
         }
         if (esclen > 6)
            goto go_again;
         k = esc_idx[esclen];
         for (j = esc_idx[esclen - 1]; j < k; ++j) {
            if (memcmp( esc_seq[j].key, escape + key, esclen ) == 0) {
               key = esc_seq[j].key_index;
               ch  = 0;
               break;
            }
         }
         if (j >= k)
            goto go_again;
      }
   }
   if (key == 0 && ch < 128)
      key = char_to_scan[ch];

   if (xterm  ||  (shift_state = kbReadShiftState( )) == ERROR) {
      shift_state = (alt_esc) ? _ALT : 0;
      shift_state |= key & ~511;
   }
   key &= 511;

   /*
    * separate control keys from their counterparts; unfortunately, this
    * doesn't allow the counterparts to be CTRLed (eg. ^I is indisting-
    * uishable from ^Tab).
    * In an xterm, use the normal key instead of the control key.
    */
   if (ch == 8 || ch == 9 || ch == 13 || ch == 27) {
      if (xterm)
         shift_state &= ~_CTRL;
      if (!(shift_state & _CTRL)) {
         if (ch == 8)
            key = _BACKSPACE;
         else if (ch == 9)
            key = _TAB;
         else if (ch == 13) {
            if ((shift_state & _ALT) && !alt_esc)
               key = _GREY_ENTER;
            else
               key = _ENTER;
         } else /* if (ch == 27) */
            key = _ESC;
      }

   } else if (xterm && ch == 127) {
      key = _DEL, ch = 0;

   /*
    * try and separate the keypad characters
    */
   } else if (ch == '/' || ch == '*' || ch == '-' || ch == '+') {
      if (((shift_state & _ALT) && !alt_esc) || (shift_state & _CTRL)) {
         if (ch == '/')
            key = _GREY_SLASH;
         else if (ch == '*')
            key = _GREY_STAR;
         else if (ch == '-')
            key = _GREY_MINUS;
         else if (ch == '+')
            key = _GREY_PLUS;
      } else if (shift_state & _SHIFT) {
         if (ch == '/')
            key = _GREY_SLASH;
         else if (ch == '-')
            key = _GREY_MINUS;
      }
   }

   if (key >= _HOME && key <= _DEL)
      shift_state |= _FUNCTION;

   new_key = translate_key( key, shift_state, ch );
   if (new_key == ERROR)
      goto go_again;

   return( new_key );
}


/*
 * Name:    waitkey
 * Purpose: nothing right now
 * Date:    November 13, 1993
 * Passed:  enh_keyboard:  boolean - TRUE if 101 keyboard, FALSE otherwise
 * Returns: 1 if no key ready, 0 if key is waiting
 * Notes:   we might do something with this function later...
 */
int  waitkey( int enh_keyboard )
{
   return( 0 );
}


/*
 * Name:    capslock_active
 * Purpose: To find out if Caps Lock is active
 * Date:    September 1, 1993
 * Author:  Byrial Jensen
 * Returns: Non zero if Caps Lock is active,
 *          Zero if Caps Lock is not active.
 * Note:    How is capslock tested with Linux? - jmh
 * jmh 990426: Discovered KDGKBLED ioctl.
 */
int  capslock_active( void )
{
int  kbled;

   if (ioctl( 0, KDGKBLED, &kbled ) == -1)
      kbled = 0;
   return( kbled & LED_CAP );
}


/*
 * Name:    numlock_active
 * Purpose: To find out if Num Lock is active
 * Date:    September 8, 2002
 * Returns: Non zero if Num Lock is active,
 *          Zero if Num Lock is not active.
 *
 * jmh 031125: this function is only used to let Shift switch between keypad
 *              function and number, but since there is no current separation
 *              between the keypad and editing keys, ignore it.
 */
int  numlock_active( void )
{
#if 0
int  kbled;

   if (ioctl( 0, KDGKBLED, &kbled ) == -1)
      kbled = 0;
   return( kbled & LED_NUM );
#else
   return( 1 );
#endif
}


/*
 * Name:    flush_keyboard
 * Purpose: flush keys from the keyboard buffer.  flushinp is a curses func.
 * Date:    November 13, 1993
 * Passed:  none
 */
void flush_keyboard( void )
{
   flushinp( );
}


/* Name:    page
 * Purpose: Change the text screen page
 * Author:  Jason Hood
 * Date:    April 18, 1999
 * Passed:  payg:   desired page number
 * Notes:   if payg is 1, read from /dev/vcsa0
 *          if payg is 0, write to /dev/vcsa0
 *          Assumes screen size does not change between calls.
 *          Assumes stdout is not redirected (to relocate the cursor).
 *          If unable to open /dev/vcsa0 this function has no effect.
 *
 * jmh 010808: try and open /dev/vcsa if /dev/vcsa0 fails.
 * jmh 030403: ignored in an xterm.
 * jmh 031126: open the current vt (doesn't require root privileges)
 */
void page( unsigned char payg )
{
int  fd;
static struct {
   char lines, cols,
        x, y;
} dim;
static char *scrn = NULL;
static int  size;
static char vt[] = "/dev/vcsa??";
struct stat stb;

   if (xterm)
      return;

   if (vt[9] == '?') {
      if (fstat( 0, &stb ) == 0) {
         fd = stb.st_rdev & 0xff;
         if (fd < 10) {
            vt[9]  = fd + '0';
            vt[10] = '\0';
         } else {
            vt[9]  = fd / 10 + '0';
            vt[10] = fd % 10 + '0';
         }
      }
   }

   if ((fd = open( vt, O_RDWR )) == -1)
      return;

   if (payg == 1) {
      read( fd, &dim, 4 );
      size = dim.lines * dim.cols * 2;
      if (scrn != NULL)
         free( scrn );
      scrn = malloc( size );
      read( fd, scrn, size );

   } else if (scrn != NULL) {
      lseek( fd, 4, SEEK_SET );
      write( fd, scrn, size );
      /*
       * Unfortunately, writing these doesn't update them.
       */
      printf( "\033[%d;%dH", (int)dim.y+1, (int)dim.x+1 );
      fflush( stdout );
   }
   close( fd );
}


/*
 * Name:    xygoto
 * Purpose: To move the cursor to the required column and line.
 * Date:    November 13, 1993
 * Passed:  col:    desired column (0 up to max)
 *          line:   desired line (0 up to max)
 * Notes;   on the PC, we use col = -1 and line = -1 to turn the cursor off.
 *            in unix, we try to trap the -1's.
 *          curs_set is a curses function.
 */
void xygoto( int col, int line )
{
   if (col < 0 || line < 0) {
      curs_set( CURSES_INVISBL );
      g_display.curses_cursor = CURSES_INVISBL;
   } else {

      /*
       * if the unix cursor is invisible, lets turn it back on.
       */
      if (g_display.curses_cursor == CURSES_INVISBL)
         set_cursor_size( mode.insert ? g_display.insert_cursor :
                                        g_display.overw_cursor );
      move( line, col );
   }
}


/*
 * Name:    c_xlat
 * Purpose: Translate character to chtype, recognising undisplayable chars.
 * Author:  Jason Hood
 * Date:    November 26, 2003
 * Passed:  c:     character
 *          attr:  attribute
 * Returns: chtype of character and attribute
 * Note:    use the diagnostic color for undisplayable characters (which were
 *           found by experimentation).
 */
static chtype c_xlat( int c, int attr )
{
text_t uc;

   uc = (text_t)c;

   if (uc < 32) {
#if defined( PC_CHARS )
      if (uc == 0 || uc == 8 || uc == 10 || (uc >= 12 && uc <= 15) || uc == 27)
#endif
         return( (uc+64) | tde_color_table[Color( Diag )] );
   }
   if (uc == 127 || uc == 0x9b)
      return( '.' | tde_color_table[Color( Diag )] );

   return( uc | pc_chars | tde_color_table[attr] );
}


/*
 * Name:    display_line
 * Purpose: display a line in the file
 * Author:  Jason Hood
 * Date:    October 28, 1999
 * Passed:  text:  characters to display
 *          attr:  attributes to use
 *          len:   length to display
 *          line:  row of display
 *          col:   column of display
 * Notes:   all parameters are assumed valid.
 */
void display_line( text_ptr text, unsigned char *attr, int len,
                   int line, int col )
{
   move( line, col );
   do
      addch( c_xlat( *text++, *attr++ ) );
   while (--len != 0);
}


/*
 * Name:    c_output
 * Purpose: Output one character on prompt lines
 * Date:    November 13, 1993
 * Passed:  c:     character to output to screen
 *          col:   col to display character
 *          line:  line number to display character
 *          attr:  attribute of character
 * Returns: none
 */
void c_output( int c, int col, int line, int attr )
{
   mvaddch( line, col, c_xlat( c, attr ) );
}


/*
 * Name:    c_repeat
 * Purpose: Output one character many times
 * Author:  Jason Hood
 * Date:    November 18, 2003
 * Passed:  c:     character to output to screen
 *          cnt:   number of times to output it
 *          col:   column to display character
 *          line:  line number to display character
 *          attr:  attribute of character
 * Notes:   if the count is negative, repeat the row, not the column.
 */
void c_repeat( int c, int cnt, int col, int line, int attr )
{
chtype ch;

   ch = c_xlat( c, attr );
   move( line, col );
   if (cnt < 0)
      vline( ch, -cnt );
   else
      hline( ch, cnt );
}


/*
 * Name:    s_output
 * Purpose: To output a string
 * Date:    November 13, 1993
 * Passed:  s:     string to output
 *          line:  line to display
 *          col:   column to begin display
 *          attr:  color to display string
 * Notes:   This function is used to output most strings not part of file text.
 *
 * 991023:  Possibly output an extra space before and after.
 */
void s_output( const char *s, int line, int col, int attr )
{
int  max = g_display.ncols;

   if (g_display.output_space && col != 0)
      mvaddch( line, col - 1, ' ' | tde_color_table[attr] );
   else
      move( line, col );

   while (*s  &&  col++ < max)
      addch( c_xlat( *s++, attr ) );

   if (g_display.output_space && col != max)
      addch( ' ' | tde_color_table[attr] );
}


/*
 * Name:    hlight_line
 * Date:    November 13, 1993
 * Passed:  x:     column to begin hi lite
 *          y:     line to begin hi lite
 *          lgth:  number of characters to hi lite
 *          attr:  attribute color
 */
void hlight_line( int x, int y, int lgth, int attr )
{
   if (x + lgth > g_display.ncols)
      lgth = g_display.ncols - x;

   if (lgth > 0) {
      move( y, x );
      do
         addch( (inch( ) & A_CHARTEXT) | pc_chars | tde_color_table[attr] );
      while (--lgth != 0);
   }

   refresh( );
}


/*
 * Name:    cls
 * Purpose: clear screen
 * Date:    November 13, 1993
 * Notes:   Call the curses clear screen function.
 */
void cls( void )
{
   touchwin( curscr );
   clear( );
   refresh( );
}


/*
 * Name:    set_cursor_size
 * Purpose: To set cursor size according to insert mode.
 * Date:    November 13, 1993
 * Passed:  csize:  not used in unix curses
 * Notes:   use the global display structures to set the cursor size
 *          curs_set( ) is a curses function.
 */
void set_cursor_size( int csize )
{
   curs_set( csize );
   g_display.curses_cursor = csize;
}


/*
 * Name:    set_overscan_color
 * Purpose: To set overscan color
 * Date:    November 13, 1993
 * Passed:  color:  overscan color
 * Notes:   i'm not sure how to set the overscan in Linux (yet?).
 *
 * jmh 031125: access the VGA hardware directly (console, root).
 */
void set_overscan_color( int color )
{
   if (port_access) {
      inb( 0x3da );             /* switch flip-flop to index mode */
      outb( 0x31, 0x3c0 );      /* display enable, register 0x11 */
      outb( color, 0x3c0 );     /* overscan color */
   }
}


/*
 * Name:    save_screen_line
 * Purpose: To save the characters and attributes of a line on screen.
 * Date:    November 13, 1993
 * Passed:  col:    desired column, usually always zero
 *          line:   line on screen to save (0 up to max)
 *          screen_buffer:  buffer for screen contents, must be >= 80 chtype
 * Notes:   Save the contents of the line on screen where prompt is
 *           to be displayed.
 */
void save_screen_line( int col, int line, chtype *screen_buffer )
{
   mvinchstr( line, col, screen_buffer );
}


/*
 * Name:    restore_screen_line
 * Purpose: To restore the characters and attributes of a line on screen.
 * Date:    November 13, 1993
 * Passed:  col:    usually always zero
 *          line:   line to restore (0 up to max)
 *          screen_buffer:  buffer for screen contents, must be >= 80 chtype
 * Notes:   Restore the contents of the line on screen where the prompt
 *           was displayed
 */
void restore_screen_line( int col, int line, chtype *screen_buffer )
{
   mvaddchstr( line, col, screen_buffer );
   refresh( );
}


/*
 * Name:    save_area
 * Purpose: save text and attribute
 * Date:    November 13, 1993
 * Passed:  buffer: storage for text and attribute
 *          wid: width to save
 *          len: length to save
 *          row: starting row of save
 *          col: starting column of save
 * Notes:   use curses to get char and attr
 *
 * jmh 991022:  I should point out that the buffer should be (at least)
 *                (wid + 4) * (len + 1) * sizeof(chtype) bytes.
 */
void save_area( chtype *buffer, int wid, int len, int row, int col )
{
int  i;

   adjust_area( &wid, &len, &row, &col, NULL );

   len--;
   for (i = 0; len >= 0; len--) {
      mvinchnstr( row+len, col, buffer, wid );
      buffer += wid;
   }
}


/*
 * Name:    restore_area
 * Purpose: restore text and attribute
 * Date:    November 13, 1993
 * Passed:  buffer: storage for text and attribute
 *          wid: width to restore
 *          len: length to restore
 *          row: starting row for restore
 *          col: starting column for restore
 * Notes:   use curses to set char and attr
 *
 * jmh 991022:  I should point out that these parameters should be identical
 *                to those used in the corresponding save_area().
 */
void restore_area( chtype *buffer, int wid, int len, int row, int col )
{
int  i;

   adjust_area( &wid, &len, &row, &col, NULL );

   len--;
   for (i = 0; len >= 0; len--) {
      mvaddchnstr( row+len, col, buffer, wid );
      buffer += wid;
   }
   refresh( );
}


/*
 * Name:    shadow_area
 * Purpose: draw a shadow around an area
 * Author:  Jason Hood
 * Date:    October 20, 1999
 * Passed:  wid: width of area
 *          len: length of area
 *          row: starting row of area
 *          col: starting column of area
 * Notes:   the characters being shadowed are not saved.
 *          Use attribute 8 (dark grey on black) as the shadow.
 */
void shadow_area( int wid, int len, int row, int col )
{
int  w;
int  i;
int  j;
int  alen;

   if (g_display.shadow) {
      alen = len;
      adjust_area( &wid, &len, &row, &col, &w );

      if (w > 0) {
         for (i = len - 1; i > 0; i--) {
            move( row+i, col+wid-w );
            for (j = w; j >= 1; j--)
               addch( (inch( ) & A_CHARTEXT) | pc_chars | tde_color_table[8] );
         }
      }
      if (alen < len) {
         col += g_display.shadow_width;
         wid -= g_display.shadow_width + w;
         row += len - 1;
         /* hlight_line( col, row, wid, 8 ); */
         move( row, col );
         while (--wid >= 0)
            addch( (inch( ) & A_CHARTEXT) | pc_chars | tde_color_table[8] );
      }
      refresh( );
   }
}
