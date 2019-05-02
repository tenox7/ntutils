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

#ifndef VK_A                            /* MinGW32 doesn't define these */
# define VK_A 65
# define VK_Z 90
#endif


       HANDLE conin;                    /* console input */
static HANDLE conout;                   /* console output (TDE's screen) */
static HANDLE StdOut;                   /* standard output (user screen) */
static HANDLE StdOutPut;                /* for redirected shelling */
static char   title[256];               /* console window title */
static DWORD  conmode;                  /* original console input mode */
static DWORD  susmode;                  /* suspended console input mode */

static int  capslock_state;             /* the current state of capslock */
static int  numlock_state;              /* the current state of numlock */
static int  process_keypad( int );      /* read Alt+Keypad character entry */
       int  handle_mouse;               /* recognise mouse input */


/*
 * the USA character set, to determine if AltGr should be a character,
 *  rather than a function key (Left\ is handled separately).
 */
#define FIRST_CHAR _1
#define LAST_CHAR  _SLASH
static const char usa_char[] = {
        '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',    8,
      9, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',  13,
      0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
     '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/'
};


/*
 * Name:    console_init
 * Purpose: initialise the console (video and keyboard)
 * Author:  Jason Hood
 * Date:    October 23, 2002
 * Passed:  cfg:  pointer to hold video config data
 * Notes:   initialises both video and keyboard hardware.
 *          moved some stuff from main.c into here.
 *
 * 050729:  always create output, since there's no distinction between
 *           console and other character devices.
 */
void console_init( struct vcfg *cfg )
{
SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
OSVERSIONINFO osvi;
DWORD mode;

   conout = CreateConsoleScreenBuffer( GENERIC_READ | GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       NULL, CONSOLE_TEXTMODE_BUFFER, NULL );

   StdOut = CreateFile( "CONOUT$", GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   &sa, OPEN_EXISTING, 0, 0 );

   conin = CreateFile( "CONIN$", GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 &sa, OPEN_EXISTING, 0, 0 );

   GetConsoleMode( conin, &conmode );
   /*
    * 070422: Win9X doesn't like the extended flag (using Ctrl+C will
    *         SIGSEGV), so test the version and adjust the flags accordingly.
    */
   osvi.dwOSVersionInfoSize = sizeof(osvi);
   GetVersionEx( &osvi );
   mode = osvi.dwPlatformId == VER_PLATFORM_WIN32_NT
          ? ENABLE_EXTENDED_FLAGS | (conmode & ENABLE_QUICK_EDIT) : 0;
   SetConsoleMode( conin, mode | ENABLE_MOUSE_INPUT );

   page( 1 );
   video_config( cfg );
   GetConsoleTitle( title, sizeof(title) );

   SetConsoleCtrlHandler( (PHANDLER_ROUTINE)ctrl_break_handler, TRUE );
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
   page( 0 );

   StdOutPut = GetStdHandle( STD_OUTPUT_HANDLE );
   SetStdHandle( STD_OUTPUT_HANDLE, StdOut );

   GetConsoleMode( conin, &susmode );
   SetConsoleMode( conin, conmode );
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
   SetStdHandle( STD_OUTPUT_HANDLE, StdOutPut );

   SetConsoleMode( conin, susmode );
   if (pause)
      getkey( );

   page( 1 );
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
   page( 0 );                   /* back to the original screen */
   SetConsoleMode( conin, conmode );
   CloseHandle( conin );
   CloseHandle( StdOut );
   CloseHandle( conout );
}


/*
 * Name:    video_config
 * Purpose: initialise console input and output
 * Date:    August 13, 2002
 * Passed:  cfg:  pointer to video parameters
 */
void video_config( struct vcfg *cfg )
{
CONSOLE_SCREEN_BUFFER_INFO csbi;

   GetConsoleScreenBufferInfo( conout, &csbi );
   cfg->cols = csbi.dwSize.X;
   cfg->rows = csbi.dwSize.Y;

   g_display.adapter = VGA;
   cfg->color = TRUE;

   g_display.cursor[SMALL_CURSOR]  = 15;
   g_display.cursor[MEDIUM_CURSOR] = 50;
   g_display.cursor[LARGE_CURSOR]  = 100;

   mode.enh_kbd = TRUE;

   g_display.shadow_width = 1;
}


/*
 * Name:    getkey
 * Purpose: get keyboard input
 * Date:    August 13, 2002
 * Passed:  None
 * Notes:   map Windows keys to TDE keys.
 *          handle Alt+keypad since Windows low-level console input doesn't.
 *
 * 020923:  recognise the Menu key as the PullDown function.
 * 021012:  better support for non-US keyboards, hopefully (AltGr characters).
 */
long getkey( void )
{
static INPUT_RECORD key;
static long kee;
WORD vk;
DWORD read;
int  scan, mod, ch;
int  state;
TDE_WIN *wp;
#define KEYEV key.Event.KeyEvent
#define MOUSEEV key.Event.MouseEvent
#define ALT_PRESSED (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED)
#define CTRL_PRESSED (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)

   if (handle_mouse && key.EventType == MOUSE_EVENT) {
do_move:
      if (MOUSEEV.dwMousePosition.X > 0) {
         --MOUSEEV.dwMousePosition.X;
         return( CharRight | _FUNCTION );
      }
      if (MOUSEEV.dwMousePosition.X < 0) {
         ++MOUSEEV.dwMousePosition.X;
         return( CharLeft | _FUNCTION );
      }
      if (MOUSEEV.dwMousePosition.Y > 0) {
         --MOUSEEV.dwMousePosition.Y;
         return( LineDown | _FUNCTION );
      }
      if (MOUSEEV.dwMousePosition.Y < 0) {
         ++MOUSEEV.dwMousePosition.Y;
         return( LineUp | _FUNCTION );
      }
      key.EventType = 0;
      if (MOUSEEV.dwControlKeyState & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED))
         return( MarkBox | _FUNCTION );
      if (MOUSEEV.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))
         return( MarkLine | _FUNCTION );
      if (MOUSEEV.dwControlKeyState & SHIFT_PRESSED)
         return( MarkStream | _FUNCTION );
   }

   if (key.EventType == KEY_EVENT  &&  KEYEV.wRepeatCount > 1) {
      --KEYEV.wRepeatCount;
      return( kee );
   }

   g_status.control_break = FALSE;

go_again:
   ReadConsoleInput( conin, &key, 1, &read );

   if (g_status.control_break)
      return( _CONTROL_BREAK );

   if (handle_mouse  &&  key.EventType == MOUSE_EVENT  &&
       MOUSEEV.dwButtonState  &&  MOUSEEV.dwEventFlags == 0) {
      if (MOUSEEV.dwButtonState & RIGHTMOST_BUTTON_PRESSED) {
         key.EventType = 0;
         return( PasteFromClipboard | _FUNCTION );
      }
      for (wp = g_status.window_list; wp != NULL; wp = wp->next) {
         if (wp->visible &&
             MOUSEEV.dwMousePosition.X >= wp->start_col  &&
             MOUSEEV.dwMousePosition.X <= wp->end_col    &&
             MOUSEEV.dwMousePosition.Y >= wp->top_line   &&
             MOUSEEV.dwMousePosition.Y <= wp->bottom_line) {
            MOUSEEV.dwMousePosition.X -= wp->ccol;
            MOUSEEV.dwMousePosition.Y -= wp->cline;
            if (g_status.current_window != wp) {
               change_window( g_status.current_window, wp );
               return( _FUNCTION );
            }
            if (g_status.current_window->rline + MOUSEEV.dwMousePosition.Y
                > g_status.current_file->length + 1)
               goto go_again;
            if (MOUSEEV.dwControlKeyState & ALT_PRESSED)
               return( MarkBox | _FUNCTION );
            if (MOUSEEV.dwControlKeyState & CTRL_PRESSED)
               return( MarkLine | _FUNCTION );
            if (MOUSEEV.dwControlKeyState & SHIFT_PRESSED)
               return( MarkStream | _FUNCTION );
            goto do_move;
         }
      }
   }
   if (key.EventType != KEY_EVENT  ||  KEYEV.bKeyDown == FALSE)
      goto go_again;

   vk = KEYEV.wVirtualKeyCode;
   if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU ||
       vk == VK_CAPITAL || vk == VK_NUMLOCK || vk == VK_SCROLL)
      goto go_again;

   if (vk == VK_APPS)
      return( PullDown | _FUNCTION );

   state = KEYEV.dwControlKeyState;
   /*
    * Remember the lock states for their respective functions.
    */
   capslock_state = state & CAPSLOCK_ON;
   numlock_state  = state & NUMLOCK_ON;

   mod = 0;
   if (state & SHIFT_PRESSED)
      mod |= _SHIFT;
   if (state & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
      mod |= _CTRL;
   if (state & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
      mod |= _ALT;
   if (state & ENHANCED_KEY)
      mod |= _FUNCTION;

   scan = KEYEV.wVirtualScanCode + 255;

   if ((mod & _ALT) && !(mod & _FUNCTION) && scan >= _HOME && scan <= _INS
       && scan != _GREY_MINUS && scan != _GREY_PLUS)
      return( process_keypad( scan ) );

   ch = KEYEV.uChar.AsciiChar;

   /*
    * If only AltGr has been pressed, see if the character is different to
    *  my mapping of it - if so, assume an AltGr character.
    * -16 (240) seems to be generated for some non-alphanum keys.
    * jmh 021019: special case for Left\.
    * jmh 021104: it seems NT/2K/XP (or the keyboard itself) also generates
    *              a left control press.
    */
   if (ch != 0 && ch != -16  &&
       (state & RIGHT_ALT_PRESSED)  &&  !(mod & _SHIFT)  &&
        (!(mod & _CTRL) || !(state & RIGHT_CTRL_PRESSED))) {
      if (scan >= FIRST_CHAR && scan <= LAST_CHAR) {
         if (bj_tolower( ch ) != usa_char[scan - FIRST_CHAR])
            return( ch );
      } else if (scan == _LEFT_BACKSLASH  &&  ch != '\\')
         return( ch );
   }

   if (mod & _FUNCTION) {
      if (scan == _SLASH)
         scan = _GREY_SLASH;
      else {
         if (vk == VK_RETURN)
            scan = _GREY_ENTER;
         else if (vk == VK_MULTIPLY)
            scan = _PRTSC;
         ch = 0;
      }
   }

   kee = translate_key( scan, mod, (text_t)ch );
   if (kee == ERROR)
      goto go_again;

   return( kee );
}


/*
 * Name:    process_keypad
 * Purpose: allow Alt+Keypad character entry
 * Author:  Jason Hood
 * Date:    August 22, 2002
 * Passed:  first:  the first key pressed
 * Returns: the character entered
 * Notes:   assumes first is an actual keypad digit.
 *          keeps processing (ignoring) keys until Alt is released, with
 *           the exception of Alt+KP0 (same as DOS).
 */
static int  process_keypad( int first )
{
INPUT_RECORD key;
int  scan;
int  num;
DWORD read;

/* Translate keypad scancodes to digits. */
static int key_digit[] = { 7, 8, 9, -1, 4, 5, 6, -1, 1, 2, 3, 0 };

   num = key_digit[first - _HOME];
   if (num == 0)
      return( 0 );

   while (TRUE) {
      ReadConsoleInput( conin, &key, 1, &read );
      if (key.EventType == KEY_EVENT) {
         scan = KEYEV.wVirtualKeyCode;
         if (KEYEV.bKeyDown == FALSE) {
            if (scan == VK_MENU)
               break;
         } else {
            scan = KEYEV.wVirtualScanCode + 255;
            if (!(KEYEV.dwControlKeyState & ENHANCED_KEY) &&
                scan >= _HOME && scan <= _INS &&
                scan != _GREY_MINUS && scan != _GREY_PLUS)
               num = num * 10 + key_digit[scan - _HOME];
         }
      }
   }
   return( num & 255 );
}


/*
 * Name:    waitkey
 * Purpose: call the BIOS keyboard status subfunction
 * Date:    August 13, 2002
 * Passed:  enh_keyboard:  boolean - TRUE if 101 keyboard, FALSE otherwise
 * Returns: 1 if no key ready, 0 if key is waiting
 * Notes:   not needed in Win32.
 */
int  waitkey( int enh_keyboard )
{
   return( 0 );
}


/*
 * Name:    capslock_active
 * Purpose: To find out if Caps Lock is active
 * Date:    August 13, 2002
 * Returns: Non zero if Caps Lock is active,
 *          Zero if Caps Lock is not active.
 * Note:    set by the last key read in getkey().
 */
int  capslock_active( void )
{
   return( capslock_state );
}


/*
 * Name:    numlock_active
 * Purpose: To find out if Num Lock is active
 * Date:    September 3, 2002
 * Returns: Non zero if Num Lock is active,
 *          Zero if Num Lock is not active.
 * Note:    set by the last key read in getkey().
 */
int  numlock_active( void )
{
   return( numlock_state );
}


/*
 * Name:    flush_keyboard
 * Purpose: flush keys from the keyboard buffer
 * Date:    August 13, 2002
 */
void flush_keyboard( void )
{
   FlushConsoleInputBuffer( conin );
}


/* Name:    page
 * Purpose: Change the text screen page
 * Author:  Jason Hood
 * Date:    August 13, 2002
 * Passed:  payg:  desired page number
 * Notes:   if payg is 0 use the standard buffer;
 *          anything else uses the TDE buffer.
 */
void page( unsigned char payg )
{
   SetConsoleActiveScreenBuffer( payg ? conout : StdOut );
   if (!payg)
      SetConsoleTitle( title );
}


/*
 * Name:    xygoto
 * Purpose: To move the cursor to the required column and line.
 * Date:    August 13, 2002
 * Passed:  col:    desired column (0 up to max)
 *          line:   desired line (0 up to max)
 */
void xygoto( int col, int line )
{
static int hidden;
COORD pos;
CONSOLE_CURSOR_INFO cci;

   if (col < 0 || line < 0) {
      if (!hidden) {
         hidden = TRUE;
         cci.bVisible = FALSE;
         cci.dwSize =  mode.insert ? g_display.insert_cursor :
                                     g_display.overw_cursor;
         SetConsoleCursorInfo( conout, &cci );
      }
   } else {
      pos.X = col;
      pos.Y = line;
      SetConsoleCursorPosition( conout, pos );
      if (hidden) {
         hidden = FALSE;
         cci.bVisible = TRUE;
         cci.dwSize =  mode.insert ? g_display.insert_cursor :
                                     g_display.overw_cursor;
         SetConsoleCursorInfo( conout, &cci );
      }
   }
}


/*
 * Name:    display_line
 * Purpose: display a line in the file
 * Author:  Jason Hood
 * Date:    August 13, 2002
 * Passed:  text:  characters to display
 *          attr:  attributes to use
 *          len:   length to display
 *          line:  row of display
 *          col:   column of display
 * Notes:   all parameters are assumed valid.
 */
void display_line( unsigned char *text, unsigned char *attr, int len,
                   int line, int col )
{
COORD pos;
WORD  a[MAX_COLS];
DWORD length;

   pos.X = col;
   pos.Y = line;
   for (length = len; (int)--length >= 0;)
      a[length] = attr[length];
   WriteConsoleOutputCharacter( conout, text, len, pos, &length );
   WriteConsoleOutputAttribute( conout, a,    len, pos, &length );
}


/*
 * Name:    c_output
 * Purpose: Output one character on prompt lines
 * Date:    August 13, 2002
 * Passed:  c:     character to output to screen
 *          col:   col to display character
 *          line:  line number to display character
 *          attr:  attribute of character
 * Returns: none
 */
void c_output( int c, int col, int line, int attr )
{
COORD pos;
DWORD length;

   pos.X = col;
   pos.Y = line;
   FillConsoleOutputCharacter( conout, (char)c,    1, pos, &length );
   FillConsoleOutputAttribute( conout, (WORD)attr, 1, pos, &length );
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
COORD pos;
DWORD length;
int  max;

   if (cnt < 0) {
      cnt = -cnt;
      max = g_display.nlines - line;
      if (cnt > max)
         cnt = max;
      if (cnt > 0) {
         pos.X = col;
         pos.Y = line;
         do {
            FillConsoleOutputCharacter( conout, (char)c,    1, pos, &length );
            FillConsoleOutputAttribute( conout, (WORD)attr, 1, pos, &length );
            ++pos.Y;
         } while (--cnt != 0);
      }

   } else {
      max = g_display.ncols - col;
      if (cnt > max)
         cnt = max;
      if (cnt > 0) {
         pos.X = col;
         pos.Y = line;
         FillConsoleOutputCharacter( conout, (char)c,    cnt, pos, &length );
         FillConsoleOutputAttribute( conout, (WORD)attr, cnt, pos, &length );
      }
   }
}


/*
 * Name:    s_output
 * Purpose: To output a string
 * Date:    August 13, 2002
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
COORD pos;
int   len;
DWORD length;

   if (g_display.output_space && col != 0)
      c_output( ' ', col - 1, line, attr );

   pos.X = col;
   pos.Y = line;
   len = strlen( s );
   WriteConsoleOutputCharacter( conout, s,    len, pos, &length );
   FillConsoleOutputAttribute(  conout, (WORD)attr, len, pos, &length );

   if (g_display.output_space && col + len < g_display.ncols)
      c_output( ' ', col + len, line, attr );
}


/*
 * Name:    hlight_line
 * Date:    August 13, 2002
 * Passed:  x:     column to begin hi lite
 *          y:     line to begin hi lite
 *          lgth:  number of characters to hi lite
 *          attr:  attribute color
 */
void hlight_line( int x, int y, int lgth, int attr )
{
COORD pos;
DWORD length;

   pos.X = x;
   pos.Y = y;
   FillConsoleOutputAttribute( conout, (WORD)attr, lgth, pos, &length );
}


/*
 * Name:    cls
 * Purpose: clear screen
 * Date:    August 13, 2002
 */
void cls( void )
{
COORD pos = { 0, 0 };
int   len;
DWORD length;

   len = g_display.nlines * g_display.ncols;
   FillConsoleOutputCharacter( conout, ' ', len, pos, &length );
   FillConsoleOutputAttribute( conout, (WORD)Color( Text ), len, pos, &length );
}


/*
 * Name:    set_cursor_size
 * Purpose: To set cursor size according to insert mode.
 * Date:    August 13, 2002
 * Passed:  csize:  desired cursor size
 */
void set_cursor_size( int csize )
{
CONSOLE_CURSOR_INFO cci;

   cci.bVisible = TRUE;
   cci.dwSize = csize;
   SetConsoleCursorInfo( conout, &cci );
}


/*
 * Name:    set_overscan_color
 * Purpose: To set overscan color
 * Date:    August 13, 2002
 * Passed:  color:  overscan color
 * Notes:   not present in a window.
 */
void set_overscan_color( int color )
{
}


/*
 * Name:    save_screen_line
 * Purpose: To save the characters and attributes of a line on screen.
 * Date:    August 13, 2002
 * Passed:  col:    desired column, usually always zero
 *          line:   line on screen to save (0 up to max)
 *          screen_buffer:  buffer for screen contents, must be >= 160 chars
 * Notes:   Save the contents of the line on screen where prompt is
 *           to be displayed
 */
void save_screen_line( int col, int line, Char *screen_buffer )
{
COORD size, pos = { 0, 0 };
SMALL_RECT region;

   size.X = g_display.ncols;
   size.Y = 1;
   region.Left = col;
   region.Right = g_display.ncols - 1;
   region.Top =
   region.Bottom = line;
   ReadConsoleOutput( conout, screen_buffer, size, pos, &region );
}


/*
 * Name:    restore_screen_line
 * Purpose: To restore the characters and attributes of a line on screen.
 * Date:    August 13, 2002
 * Passed:  col:    usually always zero
 *          line:   line to restore (0 up to max)
 *          screen_buffer:  buffer for screen contents, must be >= 160 chars
 * Notes:   Restore the contents of the line on screen where the prompt
 *           was displayed
 */
void restore_screen_line( int col, int line, Char *screen_buffer )
{
COORD size, pos = { 0, 0 };
SMALL_RECT region;

   size.X = g_display.ncols;
   size.Y = 1;
   region.Left = col;
   region.Right = g_display.ncols - 1;
   region.Top =
   region.Bottom = line;
   WriteConsoleOutput( conout, screen_buffer, size, pos, &region );
}


/*
 * Name:    save_area
 * Purpose: save text and attribute
 * Date:    July 25, 1997
 * Passed:  buffer: storage for text and attribute
 *          wid: width to save
 *          len: length to save
 *          row: starting row of save
 *          col: starting column of save
 *
 * 991023:  I should point out that the buffer should be (at least)
 *            (wid + 4) * (len + 1) * sizeof(Char) bytes.
 */
void save_area( Char *buffer, int wid, int len, int row, int col )
{
COORD size, pos = { 0, 0 };
SMALL_RECT region;

   adjust_area( &wid, &len, &row, &col, NULL );
   size.X = wid;
   size.Y = len;
   region.Left = col;
   region.Top  = row;
   region.Right  = col + wid - 1;
   region.Bottom = row + len - 1;
   ReadConsoleOutput( conout, buffer, size, pos, &region );
}


/*
 * Name:    restore_area
 * Purpose: restore text and attribute
 * Date:    July 25, 1997
 * Passed:  buffer: storage for text and attribute
 *          wid: width to restore
 *          len: length to restore
 *          row: starting row for restore
 *          col: starting column for restore
 *
 * 991023:  I should point out that these parameters should be identical
 *            to those used in the corresponding save_area().
 */
void restore_area( Char *buffer, int wid, int len, int row, int col )
{
COORD size, pos = { 0, 0 };
SMALL_RECT region;

   adjust_area( &wid, &len, &row, &col, NULL );
   size.X = wid;
   size.Y = len;
   region.Left = col;
   region.Top  = row;
   region.Right  = col + wid - 1;
   region.Bottom = row + len - 1;
   WriteConsoleOutput( conout, buffer, size, pos, &region );
}


/*
 * Name:    shadow_area
 * Purpose: draw a shadow around an area
 * Author:  Jason Hood
 * Date:    August 13, 2002
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
int  alen;
int  i;
COORD pos;
DWORD length;

   if (g_display.shadow) {
      alen = len;
      adjust_area( &wid, &len, &row, &col, &w );
      if (w > 0) {
         pos.X = col + wid - 1;
         pos.Y = row;
         for (i = len; i > 1; i--) {
            ++pos.Y;
            FillConsoleOutputAttribute( conout, 8, w, pos, &length );
         }
      }
      if (alen < len) {
         pos.X = col + g_display.shadow_width;
         pos.Y = row + len - 1;
         wid -= g_display.shadow_width + w;
         FillConsoleOutputAttribute( conout, 8, wid, pos, &length );
      }
   }
}
