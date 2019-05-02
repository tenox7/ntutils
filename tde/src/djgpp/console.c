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
#define __dj_include_pc_h_      /* don't include pc.h and its getkey */
#include <bios.h>               /* for direct BIOS keyboard input */
#include <sys/movedata.h>       /* for reading the BIOS memory */
#include <inlines/pc.h>         /* for inportb to access the keyboard */
#include <sys/exceptn.h>


static unsigned long display_address;   /* address of display memory */

static int  s_cbrk;             /* original control-break checking flag */
static int  stdoutput;          /* stdout for redirected shelling */

static int  kbd_active = TRUE;          /* should my INT09 be used? */
volatile
static unsigned int  kbd_shift = 0;     /* the actual shift states */
static unsigned char kbd_bios_shift,    /* the fake BIOS shift states */
                     kbd_scancode,      /* the hardware scan code */
                     kbd_prefix = 0,    /* detect prefixed codes */
                     kbd_menu;          /* the Menu key */
static _go32_dpmi_seginfo kbd_old_int;  /* the original INT09 */
static _go32_dpmi_seginfo kbd_new_int;  /* my new INT09 */

#define KBD_INT (_go32_info_block.master_interrupt_controller_base + 1)

/*
 * Convert a row and column co-ordinate into a screen memory address.
 */
#define SCREEN_PTR( row, col ) \
   (display_address + ((row) * g_display.ncols + (col)) * 2)


static void bright_background( void );
static int  shadow_width( int, int, int );
static void kbd_int( void );


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

   __djgpp_set_ctrl_c( 0 );             /* turn off Ctrl-C breaking */
   s_cbrk = getcbrk( );                 /* disable DOS Ctrl-Break checking */
   setcbrk( 0 );

   /*
    * Setup and install the keyboard hardware interrupt.
    *
    * The method of finding the length of a function by defining another
    * function after it doesn't work in newer GCCs (functions get reordered),
    * so just use GDB to disassemble it and find its length that way.
    */
   _go32_dpmi_lock_code( kbd_int, 428 );
   _go32_dpmi_lock_data( (void *)&kbd_shift, sizeof(kbd_shift) );
   _go32_dpmi_lock_data( (void *)&kbd_bios_shift, sizeof(kbd_bios_shift) );
   _go32_dpmi_lock_data( (void *)&kbd_scancode, sizeof(kbd_scancode) );
   _go32_dpmi_lock_data( (void *)&kbd_prefix, sizeof(kbd_prefix) );
   _go32_dpmi_get_protected_mode_interrupt_vector( KBD_INT, &kbd_old_int );
   kbd_new_int.pm_selector = _go32_my_cs();
   kbd_new_int.pm_offset = (unsigned long)kbd_int;
   _go32_dpmi_chain_protected_mode_interrupt_vector( KBD_INT, &kbd_new_int );
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
   kbd_active = FALSE;
   setcbrk( s_cbrk );

   /*
    * If output has been redirected then put it back to the screen for the
    * shell and restore it afterwards.
    */
   if (g_status.output_redir) {
      stdoutput = dup( fileno( stdout ) );
      freopen( STDFILE, "w", stdout );
   }

   __djgpp_exception_toggle( );
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
   __djgpp_exception_toggle( );

   if (g_status.output_redir)
      dup2( stdoutput, fileno( stdout ) );

   kbd_active = TRUE;
   kbd_shift = 0;
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
   setcbrk( s_cbrk );

   /*
    * reset the cursor size and unload the keyboard utility
    */
   set_cursor_size( g_display.curses_cursor );
   _go32_dpmi_set_protected_mode_interrupt_vector( KBD_INT, &kbd_old_int );

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

struct LOWMEMVID
{
   unsigned char  vidmode;              /* 0x449 */
   unsigned short scrwid;               /* 0x44A */
   unsigned short scrlen;               /* 0x44C */
   unsigned short scroff;               /* 0x44E */
   struct   LOCATE
   {
      unsigned char col;
      unsigned char row;
   } __attribute__ ((packed)) csrpos[8];/* 0x450 */
   unsigned short csrsize;              /* 0x460 */
   unsigned char  page;                 /* 0x462 */
   unsigned short addr_6845;            /* 0x463 */
   unsigned char  crt_mode_set;         /* 0x465 */
   unsigned char  crt_palette;          /* 0x466 */
   unsigned char  system_stuff[29];     /* 0x467 */
   unsigned char  rows;                 /* 0x484 */
   unsigned short points;               /* 0x485 */
   unsigned char  ega_info;             /* 0x487 */
   unsigned char  info_3;               /* 0x488 */
   unsigned char  skip[13];             /* 0x489 */
   unsigned char  kb_flag_3;            /* 0x496 */
} __attribute__ ((packed)) vid;

union REGS in, out;
unsigned char temp, active_display;

   /*
    * Move system information into our video structure.
    */
   dosmemget( 0x449, sizeof( vid ), &vid );

   /*
    * this next code was contributed by Jim Derr <jim.derr@spacebbs.com>
    * jmh 991024: slightly modified, since I moved it from hw_initialize().
    */
   cfg->cols = vid.scrwid;
   cfg->rows = vid.rows + 1;
   if (cfg->rows == 1)
      cfg->rows = 25;

   in.w.ax =  0x1a00;
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
   display_address = 0xb8000ul + vid.scroff;
   cfg->color = (vid.addr_6845 == 0x3D4);
   if (!cfg->color)
      display_address -= 0x8000;        /* monochrome is at 0xb0000 */

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

   r.w.ax = 0x1003;
   r.w.bx = 0;
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
 * Name:    kbd_int
 * Purpose: keyboard hardware interrupt handler
 * Author:  Jason Hood
 * Date:    August 17, 2002
 * Notes:   the normal BIOS function only allows the standard control
 *           characters (Ctrl+@,A-Z,[\]^_). It also does not allow
 *           Alt+Keypad0 as NUL (0 is used to indicate no input). Finally,
 *           it has its own idea about what a scan code is. This function
 *           overcomes all those problems, except for the alternate scan
 *           codes for F11/12 & Ctrl+PrtSc. It fools the BIOS into thinking
 *           the shift keys have been released. So now instead of Ctrl+1 being
 *           ignored, it generates '1', but with my Ctrl flag set.
 *          I'm not sure about other keyboards, but mine generates a prefixed
 *           shift code for the grey keys. It also generates the prefixed
 *           shift break code for Shift+Grey/ before the Grey/ code, then the
 *           prefixed shift make code after (ie: instead of 2A E035 E0B5 AA,
 *           it is 2A E0AA E035 E0B5 E02A AA). Let's ignore the prefixed
 *           shifts altogether.
 *          Unlike BIOS, I treat the shifts as one key. This will lead to
 *           Shift being unrecognised if you press both keys, then release
 *           one. It turns out BIOS has this trouble, anyway, with Ctrl & Alt.
 * 020903:  After playing around with KEYB.COM, may as well only disable
 *           control (both of them), to let the AltGr characters go through.
 * 020923:  Test for the Menu key specially.
 * 021023:  Use kbd_active to avoid potential shell problems.
 */
static void kbd_int( void )
{
   if (!kbd_active)
      return;

   /*
    * Ask the keyboard for the scancode.
    */
   kbd_scancode = inportb( 0x60 );

   /*
    * Test for the prefix code.
    */
   if (kbd_scancode == 0xE0) {
      kbd_prefix = 1;
      return;
   }

   if ((kbd_scancode == 0x2a || kbd_scancode == 0x36) && !kbd_prefix)
      kbd_shift |= _SHIFT;         /* Unprefixed Left- or RightShift pressed */
   else if (kbd_scancode == 0x1d)  /* either Ctrl */
      kbd_shift |= _CTRL;
   else if (kbd_scancode == 0x38)  /* either Alt */
      kbd_shift |= _ALT;
   else if ((kbd_scancode == 0xaa || kbd_scancode == 0xb6) && !kbd_prefix)
      kbd_shift &= ~_SHIFT;        /* Unprefixed Left- or RightShift released */
   else if (kbd_scancode == 0x9d)  /* either Ctrl */
      kbd_shift &= ~_CTRL;
   else if (kbd_scancode == 0xb8)  /* either Alt */
      kbd_shift &= ~_ALT;
   else if (kbd_scancode == 0x5d) {/* Menu */
      kbd_menu = TRUE;
      kbd_prefix = 0;
      return;
   }

   /*
    * Read the BIOS shift states and turn off the controls.
    */
   kbd_bios_shift = _farpeekw( _dos_ds, 0x417 ) & ~0x0104;

   /*
    * Leave BIOS Ctrl on to allow for Shift+ and/or Ctrl+PrtSc.
    * (Ctrl+Break is unaffected.)
    */
   if (kbd_scancode == 0x37 && kbd_shift && kbd_prefix)
      kbd_bios_shift |= 4;

   /*
    * Turn BIOS Alt off to allow Alt+KP0 as NUL, but only if no other
    * number has been entered yet.
    */
   else if ((kbd_shift & _ALT) && kbd_scancode == 0x52 && !kbd_prefix
            && _farnspeekb( 0x419 ) == 0)
      kbd_bios_shift &= ~0x0208;

   _farnspokew( 0x417, kbd_bios_shift );
   kbd_prefix = 0;
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


/*
 * Name:    getkey
 * Purpose: get keyboard input
 * Author:  Jason Hood
 * Date:    August 17, 2002
 * Passed:  None
 * Notes:   Uses the BIOS to read the next keyboard character.  Service 0x00
 *           is the XT keyboard read.  Service 0x10 is the extended keyboard
 *           read.  Test for a bunch of special cases.  Allow the user to enter
 *           any ASCII or Extended ASCII code as a normal text character.
 *
 * 020903:  updated to reflect the new translate_key().
 * 020923:  recognise the Menu key as the PullDown function.
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
   while (waitkey( mode.enh_kbd )) {
      if (kbd_menu) {
         kbd_menu = FALSE;
         return( PullDown | _FUNCTION );
      }
      __dpmi_yield( );
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

   /*
    * With the new interrupt routine, Ctrl-Break returns 0 in both
    * character and scan code.
    */
   if (key == 0  &&  (kbd_shift & _CTRL))
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

   kee = translate_key( scan, kbd_shift, ch );
   if (kee == ERROR)
      goto go_again;

   return( kee );
}


/*
 * Name:    waitkey
 * Purpose: call the BIOS keyboard status subfunction
 * Date:    October 31, 1992
 * Passed:  enh_keyboard:  boolean - TRUE if 101 keyboard, FALSE otherwise
 * Returns: 1 if no key ready, 0 if key is waiting
 * Notes:   lets get the keyboard status so we can spin on this function until
 *           the user presses a key.  some OS replacements for DOS want
 *           application programs to poll the keyboard instead of rudely
 *           sitting on the BIOS read key function.
 */
int  waitkey( int enh_keyboard )
{
   return( _bios_keybrd( enh_keyboard ? 0x11 : 0x01 ) == 0 ? 1 : 0 );
}


/*
 * Name:    capslock_active
 * Purpose: To find out if Caps Lock is active
 * Date:    September 1, 1993
 * Returns: Non zero if Caps Lock is active,
 *          Zero if Caps Lock is not active.
 * Note:    The function reads directly the BIOS variable
 *          at address 0040h:0017h bit 6
 * jmh 020903: Make use of the kbd_bios_shift variable.
 */
int  capslock_active( void )
{
   return( kbd_bios_shift & 0x40 );
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
   return( kbd_bios_shift & 0x20 );
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
         (void)_bios_keybrd( 0x10 );
   } else {
      while (!waitkey( mode.enh_kbd ))
         (void)_bios_keybrd( 0 );
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
 * Notes:   all parameters are assumed valid.
 */
void display_line( text_ptr text, unsigned char *attr, int len,
                   int line, int col )
{
unsigned long screen_ptr = SCREEN_PTR( line, col );

   _farsetsel( _dos_ds );
   do {
      _farnspokew( screen_ptr, *text++ | (*attr++ << 8) );
      screen_ptr += 2;
   } while (--len != 0);
}


/*
 * Name:    c_output
 * Purpose: Output one character
 * Date:    June 5, 1991
 * Passed:  c:     character to output to screen
 *          col:   col to display character
 *          line:  line number to display character
 *          attr:  attribute of character
 * Returns: none
 */
void c_output( int c, int col, int line, int attr )
{
   _farpokew( _dos_ds, SCREEN_PTR( line, col ),
              (unsigned char)c | (attr << 8) );
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
unsigned long screen_ptr;
int  max;
int  ofs;

   if (cnt < 0) {
      cnt = -cnt;
      max = g_display.nlines - line;
      ofs = g_display.ncols * 2;
   } else {
      max = g_display.ncols - col;
      ofs = 2;
   }

   if (cnt > max)
      cnt = max;

   if (cnt > 0) {
      _farsetsel( _dos_ds );
      screen_ptr = SCREEN_PTR( line, col );
      c = (unsigned char)c | (attr << 8);
      do {
         _farnspokew( screen_ptr, c );
         screen_ptr += ofs;
      } while (--cnt != 0);
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
 * 991023:  Possibly output an extra space before and after.
 */
void s_output( const char *s, int line, int col, int attr )
{
unsigned long screen_ptr = SCREEN_PTR( line, col );
int  max_col = g_display.ncols;

   attr <<= 8;
   _farsetsel( _dos_ds );

   if (g_display.output_space && col != 0)
      _farnspokew( screen_ptr - 2, ' ' | attr );

   while (*s && col++ < max_col) {
      _farnspokew( screen_ptr, (unsigned char)*s++ | attr );
      screen_ptr += 2;
   }
   if (g_display.output_space && col != max_col)
      _farnspokew( screen_ptr, ' ' | attr );
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
unsigned long screen_ptr = SCREEN_PTR( y, x ) + 1;

   _farsetsel( _dos_ds );
   do {
      _farnspokeb( screen_ptr, attr );
      screen_ptr += 2;
   } while (--lgth != 0);
}


/*
 * Name:    cls
 * Purpose: clear screen
 * Date:    June 5, 1991
 * Notes:   Call the video BIOS routine to clear the screen.
 */
void cls( void )
{
union REGS regs;

   regs.w.cx = 0;                       /* starting row/column = 0 */
   regs.h.dh = g_display.nlines - 1;    /* ending row */
   regs.h.dl = g_display.ncols - 1;     /* ending column */
   regs.h.bh = Color( Text );           /* attribute (jmh 990414) */
   regs.h.al = 0;                       /* get number of lines */
   regs.h.ah = 6;                       /* get function number */
   int86( VIDEO_INT, &regs, &regs );
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
union REGS regs;

   regs.h.ah = 1;                       /* function 1 - set cursor size */
   regs.w.cx = csize;                   /* get cursor size ch:cl == top:bot */
   int86( VIDEO_INT, &regs, &regs );
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
union REGS regs;

   regs.h.ah = 0x0b;
   regs.h.bl = color;
   regs.h.bh = 0;
   int86( VIDEO_INT, &regs, &regs );
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
 */
void save_screen_line( int col, int line, Char *screen_buffer )
{
   dosmemget( SCREEN_PTR( line, col ), g_display.ncols * 2, screen_buffer );
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
 */
void restore_screen_line( int col, int line, Char *screen_buffer )
{
   dosmemput( screen_buffer, g_display.ncols * 2, SCREEN_PTR( line, col ) );
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
unsigned long pointer;
int  w;
int  cols = g_display.ncols * 2;

   adjust_area( &wid, &len, &row, &col, NULL );

   w = wid * 2;
   pointer = display_address + row * cols + col * 2;
   do {
      dosmemget( pointer, w, buffer );
      buffer  += wid;
      pointer += cols;
   } while (--len != 0);
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
unsigned long pointer;
int  w;
int  cols = g_display.ncols * 2;

   adjust_area( &wid, &len, &row, &col, NULL );

   w = wid * 2;
   pointer = display_address + row * cols + col * 2;
   do {
      dosmemput( buffer, w, pointer );
      buffer  += wid;
      pointer += cols;
   } while (--len != 0);
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
unsigned long pointer;
int  w;
int  cols = g_display.ncols * 2;
int  alen;
int  i, j;

   if (g_display.shadow) {
      alen = len;
      adjust_area( &wid, &len, &row, &col, &w );

      _farsetsel( _dos_ds );
      if (w > 0) {
         pointer = display_address + (row + 1) * cols + (col + wid) * 2 - 1;
         for (i = len; i > 1; i--) {
            for (j = (w - 1) * 2; j >= 0; j -= 2)
               _farnspokeb( pointer - j, 8 );
            pointer += cols;
         }
      }
      if (alen < len) {
         col += g_display.shadow_width;
         wid -= g_display.shadow_width + w;
         /* hlight_line( col, row + len - 1, wid, 8 ); */
         pointer = display_address + (row + len - 1) * cols + col * 2 + 1;
         do {
            _farnspokeb( pointer, 8 );
            pointer += 2;
         } while (--wid != 0);
      }
   }
}
