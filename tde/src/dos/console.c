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
 * jmh - In version 5.0, I removed the commented C code from update_line();
 *       look at djgpp/console.c instead.
 * jmh 991027: removed all commented C code and chen's screen changes comments.
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

#include <dos.h>
#include <bios.h>               /* for direct BIOS keyboard input */


static char *display_address;   /* address of display memory */

static int  s_cbrk;             /* original control-break checking flag */
static int  stdoutput;          /* stdout for redirected shelling */

#if defined( __MSC__ )
static void (interrupt *old_control_c)( ); /* variable for old CNTL-C */
#endif
static void (interrupt *old_int1b)( );     /* variable for old int 1b */

/*
 * Convert a row and column co-ordinate into a screen memory address.
 */
#define SCREEN_PTR( row, col ) \
   (display_address + ((row) * g_display.ncols + (col)) * 2)


static void bright_background( void );
static int  shadow_width( int, int, int );

# if defined( __MSC__ )
static int  getcbrk( void );
static void setcbrk( int );
static void interrupt harmless( void );
#else
static int  harmless( void );
#endif
static void interrupt ctrl_break( void );


/*
 * Name:    console_init
 * Purpose: initialise the console (video and keyboard)
 * Author:  Jason Hood
 * Date:    October 23, 2002
 * Passed:  cfg:  pointer to hold video config data
 * Notes:   initialises both video and keyboard hardware.
 *          moved some stuff from main.c into here.
 */
void console_init( struct vcfg *cfg )
{
   /*
    * May as well make use of additional video memory.  There may be a
    * problem with hi-res text modes (greater than 100x80, 132x61), but
    * since I've been using page 1 since 5.0 without complaints, I guess
    * it's not a problem, after all. (Whilst I can set 132x66, I never do,
    * at least not for any period of time. :) )
    */
   page( 1 );
   video_config( cfg );

   /*
    * trap control-break to make it harmless, and turn checking off.
    *   trap control-C to make it harmless.
    */
   old_int1b = _dos_getvect( (unsigned)0x1b );
   _dos_setvect( 0x1b, ctrl_break );
   s_cbrk = getcbrk( );
#if defined( __MSC__ )
   old_control_c = _dos_getvect( (unsigned)0x23 );
   _dos_setvect( 0x23, harmless );
#else
   ctrlbrk( harmless );
#endif
   setcbrk( 0 );

   kbd_install( TRUE );
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
   set_cursor_size( g_display.curses_cursor );
   if (g_display.adapter != MDA)
      set_overscan_color( g_display.old_overscan );
   kbd_install( FALSE );
   setcbrk( s_cbrk );

   /*
    * If output has been redirected then put it back to the screen for the
    * shell and restore it afterwards.
    */
   if (g_status.output_redir) {
      stdoutput = dup( fileno( stdout ) );
      freopen( STDFILE, "w", stdout );
   }

   _dos_setvect( 0x1b, old_int1b );
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
   _dos_setvect( 0x1b, ctrl_break );

   if (g_status.output_redir)
      dup2( stdoutput, fileno( stdout ) );

   kbd_install( TRUE );
   setcbrk( 0 );

   if (pause)
      getkey( );

   page( 1 );
   bright_background( );
   if (g_display.adapter != MDA)
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
   _dos_setvect( 0x1b, old_int1b );
   /*
    * restore control-break checking
    */
#if defined( __MSC__ )
   _dos_setvect( 0x23, old_control_c );
#endif
   setcbrk( s_cbrk );

   /*
    * reset the cursor size and unload the keyboard utility
    */
   set_cursor_size( g_display.curses_cursor );
   kbd_install( FALSE );

   page( 0 );                   /* back to page 0 for DOS */
}


/*
 *   BIOS Data Areas
 *
 *   See:
 *
 *      IBM Corp., _Technical Reference:  Personal Computer AT_,
 *      Boca Raton, Florida, 1984, IBM part no. 1502494,
 *      pp 5-29 thru 5-32.
 *
 *  These addresses, variable names, types, and descriptions are directly
 *   from the IBM AT Technical Reference manual, BIOS listing, copyright IBM.
 *
 *   Address  Name           Type   Description
 *   0x0449   CRT_MODE       Byte   Current CRT mode
 *   0x044a   CRT_COLS       Word   Number columns on screen
 *   0x044c   CRT_LEN        Word   length of regen buffer, video buffer, bytes
 *   0x044e   CRT_START      Word   Starting address of regen buffer
 *   0x0450   CURSOR_POSN    Word   cursor for each of up to 8 pages.
 *   0x0460   CURSOR_MODE    Word   current cursor mode setting.
 *   0x0462   ACTIVE_PAGE    Byte   Current page
 *   0x0463   ADDR_6845      Word   base address for active display card
 *   0x0465   CRT_MODE_SET   Byte   current mode of display card
 *   0x0466   CRT_PALETTE    Byte   overscan color for CGA, EGA, and VGA.
 *   0x0467   io_rom_init    Word   Pointer to optional i/o rom init routine
 *   0x0469   io_rom_seg     Word   Pointer to io rom segment
 *   0x046b   intr_flag      Byte   Flag to indicate an interrupt happened
 *   0x046c   timer_low      Word   Low word of timer count
 *   0x046e   timer_high     Word   High word of timer count
 *   0x0470   timer_ofl      Byte   Timer has rolled over since last count
 *   0x0471   bios_break     Byte   Bit 7 = 1 if Break Key has been hit
 *   0x0472   reset_flag     Word   Word = 1234h if keyboard reset underway
 *   0x0484   ROWS           Byte   Number of displayed character rows - 1
 *   0x0485   POINTS         Word   Height of character matrix
 *   0x0487   INFO           Byte   EGA and VGA display data
 *   0x0488   INFO_3         Byte   Configuration switches for EGA and VGA
 *   0x0489   flags          Byte   Miscellaneous flags
 *   0x0496   kb_flag_3      Byte   Additional keyboard flag
 *   0x048A   DCC            Byte   Display Combination Code
 *   0x04A8   SAVE_PTR       Dword  Pointer to BIOS save area
 */
void video_config( struct vcfg *cfg )
{
#pragma pack( 1 )    /* Use pragma to force packing on byte boundaries. */

struct LOWMEMVID
{
   unsigned char vidmode;           /* 0x449 */
   unsigned int  scrwid;            /* 0x44A */
   unsigned int  scrlen;            /* 0x44C */
   unsigned int  scroff;            /* 0x44E */
   struct   LOCATE
   {
      unsigned char col;
      unsigned char row;
   } csrpos[8];                     /* 0x450 */
   unsigned int  csrsize;           /* 0x460 */
   unsigned char page;              /* 0x462 */
   unsigned int  addr_6845;         /* 0x463 */
   unsigned char crt_mode_set;      /* 0x465 */
   unsigned char crt_palette;       /* 0x466 */
   unsigned char system_stuff[29];  /* 0x467 */
   unsigned char rows;              /* 0x484 */
   unsigned int  points;            /* 0x485 */
   unsigned char ega_info;          /* 0x487 */
   unsigned char info_3;            /* 0x488 */
   unsigned char skip[13];          /* 0x489 */
   unsigned char kb_flag_3;         /* 0x496 */
} vid;
struct LOWMEMVID _far *pvid = &vid;
#pragma pack( )    /* revert to previously defined pack pragma. */

union REGS in, out;
unsigned char temp, active_display;

   /*
    * Move system information into our video structure.
    */
   my_memmove( pvid, (void *)0x00000449l, sizeof( vid ) );

   /*
    * this next code was contributed by Jim Derr <jim.derr@spacebbs.com>
    * jmh 991024: slightly modified, since I moved it from hw_initialize().
    */
   cfg->cols = vid.scrwid;
   cfg->rows = vid.rows + 1;
   if (cfg->rows == 1)
      cfg->rows = 25;


   in.x.ax =  0x1a00;
   int86( VIDEO_INT, &in, &out );
   temp = out.h.al;
   active_display = out.h.bl;
   if (temp == 0x1a && (active_display == 7 || active_display == 8))
      g_display.adapter = VGA;
   else {
      in.h.ah =  0x12;
      in.h.bl =  0x10;
      int86( VIDEO_INT, &in, &out );
      if (out.h.bl != 0x10) {         /* EGA */
         if (vid.ega_info & 0x08)
            g_display.adapter = vid.addr_6845 == 0x3d4 ? CGA : MDA;
         else
            g_display.adapter = EGA;
      } else if (vid.addr_6845 == 0x3d4)
         g_display.adapter = CGA;
      else
         g_display.adapter = MDA;
   }

   /*
    * jmh 990213: Due to the config file being read after the cursor style
    *             variable, just set cursor sizes.
    */
   if (g_display.adapter == CGA || g_display.adapter == EGA) {
      g_display.cursor[SMALL_CURSOR]  = 0x0607;
      g_display.cursor[MEDIUM_CURSOR] = 0x0407;
      g_display.cursor[LARGE_CURSOR]  = 0x0007;
   } else {
      g_display.cursor[SMALL_CURSOR]  = 0x0b0c;
      g_display.cursor[MEDIUM_CURSOR] = 0x070b;
      g_display.cursor[LARGE_CURSOR]  = 0x000b;
   }
   /*
    * jmh 990213: remember original cursor size.
    */
   g_display.curses_cursor = vid.csrsize;

   /* jmh - added the vid.scroff for proper page offset */
   FP_OFF( display_address ) = vid.scroff;
   if (vid.addr_6845 == 0x3D4) {
      cfg->color = TRUE;
      FP_SEG( display_address ) = 0xb800;
   } else {
      cfg->color = FALSE;
      FP_SEG( display_address ) = 0xb000;
   }

   /*
    * let's save the overscan color
    */
   g_display.old_overscan = vid.crt_palette;

   /*
    * Get keyboard type.  Since there is no interrupt that determines
    * keyboard type, use this method.  Look at bit 4 in keyboard flag3.
    * This method is not always foolproof on clones.
    */
   if ((vid.kb_flag_3 & 0x10) != 0)
      mode.enh_kbd = TRUE;
   else
      mode.enh_kbd = FALSE;

   /*
    * jmh 991023: determine the width of the right edge of the shadow.
    */
   g_display.shadow_width = shadow_width( cfg->cols, vid.points, cfg->rows );

   bright_background( );
}


/*
 * Name:    bright_background
 * Purpose: use bright background colors instead of blinking foreground
 * Author:  Jason Hood
 * Date:    September 23, 2002
 * Notes:   no other program I know turns it off, so I won't bother, either.
 */
static void bright_background( void )
{
union REGS r;

   r.x.ax = 0x1003;
   r.x.bx = 0;
   int86( VIDEO_INT, &r, &r );
}


/*
 * Name:    shadow_width
 * Purpose: determine the dimension of the shadow
 * Author:  Jason Hood
 * Date:    October 23, 1999
 * Passed:  width:   width of screen
 *          height:  character height
 *          lines:   last line on screen
 * Returns: number of characters to use for right edge of shadow
 * Notes:   compensate for the difference between character height and width,
 *           but use no more than two characters.
 *          Is there a better way to do this?
 */
static int shadow_width( int width, int height, int lines )
{
   if (width < 80 || height == 0)
      width = 1;
   else if (width > 90)
      width = 2;
   else {
      lines = height * lines;
      if (lines <= 200)
         lines = 6;
      else if (lines <= 350)
         lines = 10;
      else if (lines <= 400)
         lines = 12;
      else  /* lines <= 480 */
         lines = 14;
      width = (height > lines) + 1;
   }
   return( width );
}


/*
 * Translate the BIOS scancode back to the hardware scancode.
 * These are from 0x54 to 0xA6.
 */
static int bios_to_scan[] = {
   _F1, _F2, _F3, _F4, _F5, _F6, _F7, _F8, _F9, _F10, /* Shift */
   _F1, _F2, _F3, _F4, _F5, _F6, _F7, _F8, _F9, _F10, /* Ctrl (unused) */
   _F1, _F2, _F3, _F4, _F5, _F6, _F7, _F8, _F9, _F10, /* Alt */
   _PRTSC,                                            /* Ctrl */
   _LEFT, _RIGHT, _END, _PGDN, _HOME,                 /* Ctrl (unused) */
   _1, _2, _3, _4, _5, _6, _7, _8, _9, _0, _MINUS, _EQUALS, /* Alt */
   _PGUP,                                             /* Ctrl */
   _F11, _F12, _F11, _F12, _F11, _F12, _F11, _F12,    /* plain Shift Ctrl Alt */
   _UP, _GREY_MINUS, _CENTER, _GREY_PLUS, _DOWN,      /* Ctrl (unused) */
   _INS, _DEL, _TAB, _GREY_SLASH, _GREY_STAR,         /* Ctrl (unused) */
   _HOME, _UP, _PGUP, 0, _LEFT, 0, _RIGHT, 0, _END,   /* Alt */
   _DOWN, _PGDN, _INS, _DEL, _GREY_SLASH, _TAB, _GREY_ENTER  /* Alt */
};

extern volatile unsigned int  far kbd_shift;    /* defined in dos/kbdint.asm */
extern volatile unsigned char far kbd_menu;     /* ditto */


/*
 * Name:    getkey
 * Purpose: get keyboard input
 * Author:  Jason Hood
 * Date:    August 17, 2002
 * Passed:  None
 * Notes:   Uses the BIOS to read the next keyboard character.  Service 0x00
 *           is the XT keyboard read.  Service 0x10 is the extended keyboard
 *           read.  Test for a bunch of special cases.  Allow the user to enter
 *           any ASCII or Extended ASCII code as a normal text characters.
 *
 *          This function works in conjunction with a keyboard interrupt
 *           handler installed in dos/kbdint.asm.
 *
 * 020903:  updated to reflect the new translate_key().
 */
long getkey( void )
{
unsigned key;
register int scan, ch;
long     kee;

go_again:
   kbd_shift &= ~_FUNCTION;     /* turn off any previous extended key */

   /*
    * lets spin on waitkey until the user presses a key.  waitkey polls
    *  the keyboard and returns 0 when a key is waiting.
    */
   while (waitkey( mode.enh_kbd ))
      if (kbd_menu) {
         kbd_menu = FALSE;
         return( PullDown | _FUNCTION );
      }

   /*
    *  _bios_keybrd == int 16.  It returns the scan code in ah, hi part of key,
    *   and the ascii key code in al, lo part of key.  If the character was
    *   entered via ALT-xxx, the scan code, hi part of key, is 0.
    */
   if (mode.enh_kbd) {
      key = _bios_keybrd( 0x10 );

      /*
       * couple of special cases:
       *   on the numeric keypad, some keys return 0xe0 in the lo byte.
       *   1) if user enters Alt-224, then the hi byte == 0 and lo byte == 0xe0.
       *   we need to let this get thru as an Extended ASCII char.  2) although
       *   we could make the cursor pad keys do different functions than the
       *   numeric pad cursor keys, let's set the 0xe0 in the lo byte to zero
       *   and forget about support for those keys.
       */
      if ((key & 0x00ff) == 0x00e0 && (key & 0xff00) != 0) {
         key &= 0xff00;
         kbd_shift |= _FUNCTION;
      }
   } else
      key = _bios_keybrd( 0 );

   if (key == 0xffffU)
      return( _CONTROL_BREAK );

   scan = (key & 0xff00) >> 8;
   ch   =  key & 0x00ff;

   /*
    * At the bottom of page 195 in MASM 6.0 ref manual, "..when the keypad
    *  ENTER and / keys are read through the BIOS interrupt 16h, only E0h
    *  is seen since the interrupt only gives one-byte scan codes."
    */
   if (scan == 0xe0) {
      if (ch == 13) {
         scan = _GREY_ENTER;
         ch   = 0;
      } else if (ch == '/')
         scan = _GREY_SLASH;

   /*
    * Remap the BIOS scan code to the hardware.
    */
   } else if (scan >= 0x54 && scan <= 0xA6)
      scan = bios_to_scan[scan - 0x54];

   /*
    * My machine at home is sorta weird.  On every machine that I've
    *  tested at the awffice, the ALT-xxx combination returns 0 for the
    *  scan byte and xxx for the ASCII code. My machine returns 184 (decimal)
    *  as the scan code?!?!?  I added the next two lines for my machine at
    *  home.  I wonder if someone forgot to zero out ah for Alt keypad entry
    *  when they wrote my bios?
    */
   else if (scan == 0 || scan >= 0x80)
      return( ch );

   /*
    * Convert the scan code to the function key.
    */
   else {
      scan += 255;

      /*
       * Special case for Alt+KP0 to enter the NUL character, and to
       * test for AltGr characters.
       */
      if (kbd_shift & _ALT) {
         if (scan == _INS && !(kbd_shift & _FUNCTION))
            return( 0 );
         if (ch != 0)
            return( ch );
      }
   }

   kee = translate_key( scan, kbd_shift, (text_t)ch );
   if (kee == ERROR)
      goto go_again;

   return( kee );
}


/*
 * Name:    waitkey
 * Purpose: call the BIOS keyboard status subfunction
 * Date:    October 31, 1992
 * Modified: August 11, 1997, Jason Hood - Borland (BC3.1) falsely returns 0
 *                                         for Ctrl-Break; it should be -1
 * Passed:  enh_keyboard:  boolean - TRUE if 101 keyboard, FALSE otherwise
 * Returns: 1 if no key ready, 0 if key is waiting
 * Notes:   lets get the keyboard status so we can spin on this function until
 *           the user presses a key.  some OS replacements for DOS want
 *           application programs to poll the keyboard instead of rudely
 *           sitting on the BIOS read key function.
 */
int  waitkey( int enh_keyboard )
{
#if defined( __TURBOC__ )
   return( (_bios_keybrd( enh_keyboard ? 0x11 : 0x01 ) == 0 &&
            !g_status.control_break) ? 1 : 0 );
#else
   return( _bios_keybrd( enh_keyboard ? 0x11 : 0x01 ) == 0 ? 1 : 0 );
#endif
}


#if defined( __MSC__ )
/*
 * Name:    getcbrk
 * Purpose: get the control-break checking flag
 * Author:  Jason Hood
 * Date:    October 23, 2002
 * Returns: the state of the flag
 */
static int  getcbrk( void )
{
union REGS regs;

   regs.h.ah = 0x33;
   regs.h.al = 0;
   intdos( &regs, &regs );
   return( regs.h.dl );
}

/*
 * Name:    setcbrk
 * Purpose: set the control-break checking flag
 * Author:  Jason Hood
 * Date:    October 23, 2002
 * Passed:  flag:  the new state
 */
static void setcbrk( int flag )
{
union REGS regs;

   regs.h.ah = 0x33;
   regs.h.al = 1;
   regs.h.dl = (char)flag;
   intdos( &regs, &regs );
}
#endif


/*
 * Name:    harmless
 * Purpose: Do nothing when control-C is pressed
 * Date:    June 5, 1991
 * Notes:   Interrupt 23, the Control-C handler, is a MS DOS system function.
 *            Since we want to use Control-C as a regular function key,
 *            let's do absolutely nothing when Control-C is pressed.
 */
#if defined( __MSC__ )
static void interrupt harmless( void )
{
}
#else
static int  harmless( void )
{
   return 1;
}
#endif


/*
 * Name:    ctrl_break
 * Purpose: Set our control-break flag when control-break is pressed.
 * Date:    June 5, 1992
 * Notes:   Control-break is a little different from Control-C.  When
 *           Control-C is pressed, MS DOS processes it as soon as possible,
 *           which may be quite a while.  On the other hand, when
 *           Control-break is pressed on IBM and compatibles, interrupt 0x1b
 *           is generated immediately.  Since an interrupt is generated
 *           immediately, we can gain control of run-away functions, like
 *           recursive macros, by checking our Control-break flag.
 */
void interrupt ctrl_break( void )
{
   g_status.control_break = TRUE;
}


/*
 * Name:    capslock_active
 * Purpose: To find out if Caps Lock is active
 * Date:    September 1, 1993
 * Author:  Byrial Jensen
 * Returns: Non zero if Caps Lock is active,
 *          Zero if Caps Lock is not active.
 * Note:    The function reads directly the BIOS variable
 *          at address 0040h:0017h bit 6
 */
int  capslock_active( void )
{
   return( (int)( *((unsigned char far *)0x00400017L) & 0x40 ) );
}


/*
 * Name:    numlock_active
 * Purpose: To find out if Num Lock is active
 * Date:    September 3, 2002
 * Returns: Non zero if Num Lock is active,
 *          Zero if Num Lock is not active.
 * Note:    Same as Caps Lock, but using bit 5.
 */
int  numlock_active( void )
{
   return( (int)( *((unsigned char far *)0x00400017L) & 0x20 ) );
}


/*
 * Name:    flush_keyboard
 * Purpose: flush keys from the keyboard buffer
 * Date:    June 5, 1993
 * Passed:  enh_keyboard:  boolean - TRUE if 101 keyboard, FALSE otherwise
 * Returns: 1 if no key ready, 0 if key is waiting
 * Notes:   lets get the keyboard status so we can spin on this function until
 *           the user presses a key.  some OS replacements for DOS want
 *           application programs to poll the keyboard instead of rudely
 *           sitting on the BIOS read key function.
 */
void flush_keyboard( void )
{
   if (mode.enh_kbd) {
      while (!waitkey( mode.enh_kbd ))
         _bios_keybrd( 0x10 );
   } else {
      while (!waitkey( mode.enh_kbd ))
         _bios_keybrd( 0 );
   }
}


/* Name:    page
 * Purpose: Change the text screen page
 * Author:  Jason Hood
 * Date:    December 28, 1996
 * Passed:  payg:   desired page number
 * Notes:   use standard BIOS video interrupt 0x10 to set the set.
 */
void page( unsigned char payg )
{
union REGS inregs, outregs;

   inregs.h.ah = 5;
   inregs.h.al = payg;
   int86( VIDEO_INT, &inregs, &outregs );
}


/*
 * Name:    xygoto
 * Purpose: To move the cursor to the required column and line.
 * Date:    September 28, 1991
 * Passed:  col:    desired column (0 up to max)
 *          line:   desired line (0 up to max)
 * Notes:   use standard BIOS video interrupt 0x10 to set the set.
 *
 * jmh 991026: (-1, -1) doesn't work for (my) 132-column mode. Use one
 *             more than the screen height, instead.
 */
void xygoto( int col, int line )
{
union REGS inregs, outregs;

   if (col < 0 || line < 0) {
      col  = 0;
      line = g_display.nlines;
   }
   inregs.h.ah = 2;
   inregs.h.bh = 1;                     /* jmh - the editor is on page 1 */
   inregs.h.dh = (unsigned char)line;
   inregs.h.dl = (unsigned char)col;
   int86( VIDEO_INT, &inregs, &outregs );
}


/*
 * Name:    display_line
 * Purpose: display a line in the file
 * Author:  Jason Hood
 * Date:    October 27, 1999
 * Passed:  text:  characters to display
 *          attr:  attributes to use
 *          len:   length to display
 *          line:  row of display
 *          col:   column of display
 * Notes:   all parameters are assumed valid (and len > 0).
 *          Assumes text and attr to be in the same segment.
 */
void display_line( text_ptr text, unsigned char *attr, int len,
                   int line, int col )
{
char *screen_ptr = SCREEN_PTR( line, col );

   ASSEMBLE {
        push    ds
        push    si
        push    di
        les     di, screen_ptr
        lds     si, text
        mov     bx, WORD PTR attr
        mov     cx, len
   }
   char_out:

   ASSEMBLE {
        lodsb
        mov     ah, [bx]
        inc     bx
        stosw
        dec     cx
        jnz     char_out
        pop     di
        pop     si
        pop     ds
   }
}


/*
 * Name:    c_output
 * Purpose: Output one character on prompt lines
 * Date:    June 5, 1991
 * Passed:  c:     character to output to screen
 *          col:   col to display character
 *          line:  line number to display character
 *          attr:  attribute of character
 * Returns: none
 */
void c_output( int c, int col, int line, int attr )
{
char *screen_ptr = SCREEN_PTR( line, col );

   ASSEMBLE {
        les     bx, screen_ptr          /* load SEGMENT+OFFSET of screen ptr */
        mov     al, BYTE PTR c          /* get character */
        mov     ah, BYTE PTR attr       /* get attribute */
        mov     es:[bx], ax             /* show char on screen */
   }
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
char *screen_ptr = SCREEN_PTR( line, col );
int  max_col = g_display.ncols;
int  max_row = g_display.nlines;

   ASSEMBLE {
        push    di

        mov     cx, cnt
        test    cx, cx
        jg      fill_row
        neg     cx
        mov     ax, max_row
        sub     ax, line
        mov     bx, max_col
        shl     bx, 1
        jmp     test_count
   }
fill_row:
   ASSEMBLE {
        mov     ax, max_col
        sub     ax, col
        mov     bx, 2
   }
test_count:
   ASSEMBLE {
        cmp     cx, ax                  /* is count within the maximum? */
        jle     display
        mov     cx, ax
   }
display:
   ASSEMBLE {
        les     di, screen_ptr
        mov     ah, BYTE PTR attr
        mov     al, BYTE PTR c
        jmp     short done
   }
repeater:
   ASSEMBLE {
        mov     es:[di], ax
        add     di, bx
   }
done:
   ASSEMBLE {
        dec     cx                      /* is count more than zero? */
        jns     repeater

        pop     di
   }
}


/*
 * Name:    s_output
 * Purpose: To output a string
 * Date:    June 5, 1991
 * Passed:  s:     string to output
 *          line:  line to display
 *          col:   column to begin display
 *          attr:  color to display string
 * Notes:   This function is used to output most strings not part of file text.
 *
 * jmh 991023:  Possibly output an extra space before and after.
 * jmh 991027:  don't bother testing (s == NULL) or (*s == '\n').
 */
void s_output( const char *s, int line, int col, int attr )
{
char *screen_ptr = SCREEN_PTR( line, col );
int  max_col = g_display.ncols;
int  space = g_display.output_space;

   ASSEMBLE {
        push    ds                      /* save ds on stack */
        push    di                      /* save di on stack */
        push    si                      /* save si on stack */

        les     di, screen_ptr          /* load SEGMENT+OFFSET of screen ptr */
        lds     si, s                   /* load segment+offset of string ptr */
        mov     dx, WORD PTR col        /* put cols in dx */
        mov     cx, WORD PTR max_col    /* keep max_col in cx */
        mov     ah, BYTE PTR attr       /* keep attribute in ah */
        cmp     BYTE PTR space, 0       /* outputting a space? */
        je      begin                   /* no, skip it */
        test    dx, dx                  /* already on left edge? */
        je      begin                   /* yep, skip it */
        mov     al, ' '
        mov     es:[di-2], ax
   }
begin:
   ASSEMBLE {
        sub     cx, dx          /* max_col -= col */
        jle     getout
   }
top:

   ASSEMBLE {
        lodsb                   /* get next char in string - put in al */
        test    al, al          /* is it '\0' */
        jz      end             /* yes, end of string */
        stosw                   /* else show attr + char on screen (ah + al) */
        loop    SHORT top       /* get another character */
   }
end:

   ASSEMBLE {
        cmp     BYTE PTR space, 0
        je      getout
        jcxz    getout
        mov     al, ' '
        stosw
   }
getout:

   ASSEMBLE {
        pop     si              /* get back si */
        pop     di              /* get back di */
        pop     ds              /* get back ds */
   }
}


/*
 * Name:    hlight_line
 * Date:    July 21, 1991
 * Passed:  x:     column to begin hi lite
 *          y:     line to begin hi lite
 *          lgth:  number of characters to hi lite
 *          attr:  attribute color
 * Notes:   The attribute byte is the hi byte.
 */
void hlight_line( int x, int y, int lgth, int attr )
{
char *screen_ptr = SCREEN_PTR( y, x );

   ASSEMBLE {
        push    di              /* save di */

        mov     cx, lgth        /* number of characters to change color */
        les     di, screen_ptr  /* get destination - video memory */
        mov     al, BYTE PTR attr       /* attribute */
   }
lite_len:

   ASSEMBLE {
        inc     di              /* skip over character to attribute */
        stosb                   /* store a BYTE */
        loop    lite_len        /* change next attribute */

        pop     di              /* restore di */
   }
}


/*
 * Name:    cls
 * Purpose: clear screen
 * Date:    June 5, 1991
 * Notes:   Call the video BIOS routine to clear the screen.
 */
void cls( void )
{
int  line;
unsigned char column;
unsigned char attr;

   line = g_display.nlines - 1;
   column = (unsigned char)(g_display.ncols - 1);
   attr   = (unsigned char)Color( Text );

   ASSEMBLE {
        xor     cx, cx                  /* starting row in ch = 0 */
                                        /* starting column in cl = 0 */
        mov     ax, WORD PTR line       /* get ending row */
        mov     dh, al                  /* put it in dh */
        mov     dl, Byte Ptr column     /* ending column in dl = 79 */
        mov     bh, Byte Ptr attr       /* attribute in bh (jmh 990414) */
        mov     al, cl                  /* get number of lines */
        mov     ah, 6                   /* get function number */
        push    bp                      /* some BIOS versions wipe out bp */
        int     VIDEO_INT
        pop     bp
   }
}


/*
 * Name:    set_cursor_size
 * Purpose: To set cursor size according to insert mode.
 * Date:    June 5, 1991
 * Passed:  csize:  desired cursor size
 * Notes:   use the global display structures to set the cursor size
 */
void set_cursor_size( int csize )
{
   ASSEMBLE {
        mov     ah, 1                   /* function 1 - set cursor size */
        mov     cx, WORD PTR csize      /* get cursor size ch:cl == top:bot */
        push    bp
        int     VIDEO_INT               /* video interrupt = 10h */
        pop     bp
   }
}


/*
 * Name:    set_overscan_color
 * Purpose: To set overscan color
 * Date:    April 1, 1993
 * Passed:  color:  overscan color
 * Notes:   before setting the overscan color, the old overscan color
 *           needs to be saved so it can be restored.
 */
void set_overscan_color( int color )
{
   ASSEMBLE {
        mov     ah, 0x0b                /* function 0x0b */
        mov     bl, BYTE PTR color      /* get new overscan color */
        xor     bh, bh
        push    bp
        int     VIDEO_INT               /* video interrupt = 10h */
        pop     bp
   }
}


/*
 * Name:    save_screen_line
 * Purpose: To save the characters and attributes of a line on screen.
 * Date:    June 5, 1991
 * Passed:  col:    desired column, usually always zero
 *          line:   line on screen to save (0 up to max)
 *          screen_buffer:  buffer for screen contents, must be >= 160 chars
 * Notes:   Save the contents of the line on screen where prompt is
 *           to be displayed
 * jmh 991028: may as well write it in assembly.
 */
void save_screen_line( int col, int line, Char *screen_buffer )
{
char *p = SCREEN_PTR( line, col );
int  max_cols = g_display.ncols;

   ASSEMBLE {
        push    ds
        push    si
        push    di

        les     di, screen_buffer
        lds     si, p
        mov     cx, max_cols
        rep     movsw

        pop     di
        pop     si
        pop     ds
   }
}


/*
 * Name:    restore_screen_line
 * Purpose: To restore the characters and attributes of a line on screen.
 * Date:    June 5, 1991
 * Passed:  col:    usually always zero
 *          line:   line to restore (0 up to max)
 *          screen_buffer:  buffer for screen contents, must be >= 160 chars
 * Notes:   Restore the contents of the line on screen where the prompt
 *           was displayed
 * jmh 991028: may as well write it in assembly.
 */
void restore_screen_line( int col, int line, Char *screen_buffer )
{
char *p = SCREEN_PTR( line, col );
int  max_cols = g_display.ncols;

   ASSEMBLE {
        push    ds
        push    si
        push    di

        les     di, p
        lds     si, screen_buffer
        mov     cx, max_cols
        rep     movsw

        pop     di
        pop     si
        pop     ds
   }
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
 *
 * jmh 991022:  Rewrote in assembly.
 *              I should point out that the buffer should be (at least)
 *                (wid + 4) * (len + 1) * sizeof(Char) bytes.
 */
void save_area( Char *buffer, int wid, int len, int row, int col )
{
char *pointer;
int  cols = g_display.ncols;

   adjust_area( &wid, &len, &row, &col, NULL );

   pointer = SCREEN_PTR( row, col );

   ASSEMBLE {
        push    ds
        push    si
        push    di

        lds     si, pointer
        les     di, buffer
        mov     bx, cols                /* BX = (cols - wid) * 2 */
        sub     bx, wid                 /* It is used to point to */
        shl     bx, 1                   /*  the next screen row */
        mov     ax, len
   }
   line_loop:

   ASSEMBLE {
        mov     cx, wid
        rep     movsw                   /* buffer = screen */
        add     si, bx                  /* next row */
        dec     ax                      /* any more? */
        jnz     line_loop               /* yep (assumes len > 0) */

        pop     di
        pop     si
        pop     ds
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
 *
 * jmh 991022:  Rewrote in assembly.
 *              I should point out that these parameters should be identical
 *                to those used in the corresponding save_area().
 */
void restore_area( Char *buffer, int wid, int len, int row, int col )
{
char *pointer;
int  cols = g_display.ncols;

   adjust_area( &wid, &len, &row, &col, NULL );

   pointer = SCREEN_PTR( row, col );

   ASSEMBLE {
        push    ds
        push    si
        push    di

        les     di, pointer
        lds     si, buffer
        mov     bx, cols                /* BX = (cols - wid) * 2 */
        sub     bx, wid                 /* It is used to point to */
        shl     bx, 1                   /*  the next screen row */
        mov     ax, len
   }
   line_loop:

   ASSEMBLE {
        mov     cx, wid
        rep     movsw                   /* screen = buffer */
        add     di, bx                  /* next row */
        dec     ax                      /* any more? */
        jnz     line_loop               /* yep (assumes len > 0) */

        pop     di
        pop     si
        pop     ds
   }
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
char *pointer;
int w;
int cols = g_display.ncols * 2;
int alen;

   if (g_display.shadow) {
      alen = len;
      adjust_area( &wid, &len, &row, &col, &w );

      if (w > 0) {
         pointer = display_address + (row + 1) * cols + (col + wid) * 2;
      ASSEMBLE {
        push    di

        les     di, pointer
        mov     dx, cols
        mov     ax, w
        shl     ax, 1
        add     dx, ax          /* dx is offset to next line */
        mov     al, 8           /* attribute */
        mov     cx, len
        dec     cx              /* skip the first line */
        std                     /* going backwards */
      }
   shadow_line:

      ASSEMBLE {
        mov     bx, w           /* number of characters to shadow */
      }
   shadow_char:

      ASSEMBLE {
        dec     di              /* point to attribute */
        stosb                   /* store a BYTE */
        dec     bx
        jnz     shadow_char
        add     di, dx          /* next line */
        loop    shadow_line

        cld
        pop     di
       }
      }
      if (alen < len) {
         col += g_display.shadow_width;
         wid -= g_display.shadow_width + w;
         /* hlight_line( col, row + len - 1, wid, 8 ); */
         pointer = display_address + (row + len - 1) * cols + col * 2;
      ASSEMBLE {
        push    di

        mov     cx, wid
        les     di, pointer
        mov     al, 8
      }
   shadow_len:

      ASSEMBLE {
        inc     di              /* skip over character to attribute */
        stosb                   /* store a BYTE */
        loop    shadow_len      /* change next attribute */

        pop     di
       }
      }
   }
}
