/*
 * This file was originally console.c, but I split that into three separate
 * files for each of the systems. These query functions were what was left
 * over - jmh.
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
#include "config.h"
#include "syntax.h"
#include <stdarg.h>


#define CopyWord    NextLine
#define CopyString  BegNextLine

static int  prompt_line;
static const char *prompt_text;
static int  prompt_len = ERROR;
static int  prompt_col;

static int  dlg_active = FALSE;
static int  dlg_tab  = FALSE;
static int  dlg_move = 0;
static int  first_edit, last_edit;
static int  first_cbox, num_cbox;
static DIALOG* current_dlg;
static int  dlg_col, dlg_row;
static int  dlg_id;
static DLG_PROC dlg_proc;
static int  dlg_drawn;

static HISTORY *search_history( char *, int, HISTORY *, HISTORY *, int );

static void display_prompt( void );

static void init_dialog( DIALOG * );
static void display_dialog( void );
static void hlight_label( int, int );


/*
 * Name:    getfunc
 * Purpose: get the function assigned to key c
 * Date:    July 11, 1991
 * Passed:  c:  key just pressed
 * Notes:   key codes less than 256 or 0x100 are not assigned a function.
 *           The codes in the range 0-255 are ASCII and extended ASCII chars.
 * jmh 980727: 0 is used to represent the insert_overwrite function for
 *              normal characters, and an unassigned function key or two-key.
 *             If the function is PlayBack, point g_status.key_macro to
 *              the macro structure; otherwise it's undefined.
 * jmh 020824: use a bitmask to indicate the key is actually a function.
 */
int  getfunc( long c )
{
int  func = 0;
TREE *two_key = NULL;
TDE_WIN  *win;
LANGUAGE *language;
int  scan, shift;

   if (c >= 256) {
      if (PARENT_KEY( c )) {
         win = g_status.current_window;
         if (win->syntax) {
            language = win->file_info->syntax;
            do {
               two_key = search_tree( c, language->key_tree.right );
               language = language->parent;
            } while ((two_key == NULL || two_key->func == 0) &&
                     language != NULL);
         }
         if (two_key == NULL || two_key->func == 0)
            two_key = search_tree( c, key_tree.right );
         if (two_key != NULL) {
            func = two_key->func;
            g_status.key_macro = two_key->macro;
         }
      } else if ((int)c & _FUNCTION)
         func = (int)c & ~_FUNCTION;
      else {
         scan = KEY( c );
         shift = SHIFT( c );
         func = key_func[shift][scan];
         if (func == PlayBack)
            g_status.key_macro = macro[shift][scan];
      }
   }
   return( func );
}


/*
 * Name:    translate_key
 * Purpose: to transform the key to something else
 * Author:  Jason Hood
 * Date:    July 25, 1998
 * Passed:  key:  the TDE code of the key to be translated
 *          mod:  the modifiers (shift, ctrl, alt, extended)
 *          ch:   the associated (OS) character
 * Returns: the new key;
 *          ERROR if the key should be ignored.
 * Notes:   basically the same as Int15 service 4Fh keyboard intercept.
 *          _FUNCTION modifier is used to indicate an extended key.
 *
 *          Treat Grey/-*+ the same as the keypad numbers: if NumLock is on
 *           they are their characters; if NumLock is off they are function
 *           keys. With Shift this is inverted. GreyEnter is always treated
 *           as a function key.
 *
 * jmh 980726: Translate two-keys directly. Keys are now a long, where normal
 *             ASCII and Extended-ASCII are 0-255, function keys are from 256,
 *             pseudo-macros are 8481-65535 (as hi- and lo-byte), and two-keys
 *             are 16842753 onwards (as hi- and lo-word for the two keys,
 *             where the hi-key parent must be a function key).
 * jmh 990428: recognize viewer keys.
 * jmh 020903: with the modified keyboard handling, this function can accept
 *              more responsibilty, leaving getkey() to read the scancode and
 *              shift state, and retrieve the system character. The viewer
 *              keys are also proper keys now, not transformed characters.
 *              Unfortunately, this means the keypad won't work. I'm not too
 *              bothered, at the moment.
 * jmh 020924: make two-keys also use keys, not characters.
 */
long translate_key( int key, int mod, text_t ch )
{
long new_key;
int  func = 0;
static int two_key = 0;
int  viewer = FALSE;

   /*
    * I should probably have unnamed keys (key89, key90, ...) so
    * those people with fancy keyboards can use them.
    */
   if (KEY( key ) > MAX_KEYS)
      return( ERROR );

   if (g_status.viewer_key) {
      if (g_status.current_file->read_only)
         viewer = TRUE;
      g_status.viewer_key = FALSE;
   }
   if (two_key)
      viewer = TRUE;

   key = scancode_map[KEY( key )];

   /*
    * Prevent enter, escape, backspace and tab mapping to characters.
    */
   if (key == _ENTER || key == _ESC || key == _BACKSPACE || key == _TAB)
      ch = 0;

   if (mod == 0 || mod == _FUNCTION) {
      /*
       * No shifted keys, so test the characters.
       */
      if (!viewer && !(ch == 0 || (!numlock_active()  &&
                                   (key == _GREY_STAR || key == _GREY_SLASH ||
                                    key == _GREY_PLUS || key == _GREY_MINUS))))
         key = ch;

   } else if ((mod & ~_FUNCTION) == _SHIFT) {
      /*
       * Shift key only, again test characters, as well as extended "keypad".
       */
      if (ch == 0 || viewer) {
         if ((mod & _FUNCTION) || (key < _HOME || key > _DEL))
            key |= _SHIFT;
      } else if (!(numlock_active() &&
                   (key == _GREY_STAR || key == _GREY_SLASH ||
                    key == _GREY_PLUS || key == _GREY_MINUS)))
         key = ch;

   /*
    * Other combinations of shift keys.
    */
   } else
      key |= mod & ~_FUNCTION;

   if (two_key) {
      show_search_message( CLR_SEARCH );
      new_key = CREATE_TWOKEY( two_key, key );
      two_key = 0;
      func    = getfunc( new_key );

   } else {
      func = getfunc( key );
      if (func == TwoCharKey) {
         /*
          * Next key..
          */
         show_search_message( NEXTKEY );
         two_key = key;
         return( ERROR );
      } else
         new_key = key;
   }

   /*
    * Test for a single-key macro.
    */
   if (func == PlayBack && g_status.key_macro->len == 1  &&
       (!mode.record || new_key != g_status.recording_key)) {
      new_key = g_status.key_macro->key.key;
      if ((unsigned long)new_key < 256  &&  capslock_active( ))
         new_key = (bj_islower( (int)new_key )) ? bj_toupper( (int)new_key ) :
                                                  bj_tolower( (int)new_key );

   /*
    * Toggle the graphic chararacters. This allows them to be used anywhere
    * (as I feel they should), but at the expense of flexible macro
    * definitions (it wouldn't use the graphic character of the current set,
    * but the recorded character).
    */
   } else if (func == ToggleGraphicChars) {
      toggle_graphic_chars( NULL );     /* Note: dummy argument */
      new_key = ERROR;

   /*
    * Translate numbers and minus to an appropriate graphic character.
    * Use F1 - F6 to select the set:
    *  F1 - single line;
    *  F2 - double line;
    *  F3 - single horizontal, double vertical;
    *  F4 - double horizontal, single vertical.
    *  F5 - solid (24568), "arrows" (79), stipples (013), and small block (-).
    *  F6 - ASCII (+-|) (991019)
    */
   } else if (mode.graphics > 0) {
      if (key >= _F1 && key <= _F6) {
         mode.graphics = (key == _F6) ? 1 : key - _F1 + 2;
         show_graphic_chars( );
         new_key = ERROR;
      } else if ((key >= '0' && key <= '9') || key == '-') {
         key     = (key == '-') ? 10 : key - '0';
         new_key = graphic_char[mode.graphics-1][key];
      }
   }

   return( new_key );
}


/*
 * Name:    get_name
 * Purpose: To prompt the user and read the string entered in response.
 * Date:    June 5, 1992
 * Passed:  prompt: prompt to offer the user
 *          line:   line to display prompt
 *          name:   default answer
 *          hist:   history to search/add answer to
 * Returns: name:   user's answer
 *          length of the answer
 *          ERROR if user aborted the command
 *
 * jmh 031115: turned into a wrapper for get_string().
 * jmh 050918: if using file history, turn leading "~/" into TDE's home dir.
 */
int  get_name( const char *prompt, int line, char *name, HISTORY *hist )
{
DISPLAY_BUFF;
int  rc;

   /*
    * set up prompt
    */
   prompt_len  = strlen( prompt );
   prompt_line = line;
   prompt_text = prompt;
   prompt_col  = (g_status.current_window != NULL &&
                  g_status.current_window->syntax) ? syntax_color[0]
                                                   : Color( Text );

   assert( prompt_len < g_display.ncols );

   SAVE_LINE( line );

   rc = get_string( prompt_len, line, g_display.ncols - prompt_len,
                    prompt_col, ' ', name, hist );

   if (rc > 1  &&  hist == &h_file  &&  *name == '~' && (name[1] == '/'
#if !defined( __UNIX__ )
                                                     ||  name[1] == '\\'
#endif
                                                                        )) {
      char buf[PATH_MAX+2];
      join_strings( buf, g_status.argv[0], name + 2 );
      rc = strlen( buf );
      memcpy( name, buf, rc + 1 );
   }

   RESTORE_LINE( line );

   prompt_len = ERROR;

   return( rc );
}


/*
 * Name:    display_prompt
 * Class:   helper function
 * Purpose: display the prompt from get_string()
 * Author:  Jason Hood
 * Date:    November 15, 2003
 * Notes:   get_name() needs to be called first
 */
static void display_prompt( void )
{
   s_output( prompt_text, prompt_line, 0, Color( Message ) );
   eol_clear( prompt_len, prompt_line, prompt_col );
}


/*
 * Name:    get_string
 * Purpose: Read a string from the user (or macro)
 * Date:    June 5, 1992
 * Passed:  col:    column position for input
 *          line:   line position for input
 *          width:  width of input display
 *          normal: color to blank unused portion
 *          blank:  character to blank unused portion
 *          name:   default answer
 *          hist:   history to search/add answer to
 * Returns: name:   user's answer
 *          length of the answer
 *          ERROR if user aborted the command
 * Notes:   with the addition of macros in tde, this function became a little
 *           more complicated.  we have to deal with both executing macros
 *           and macros that are the user uses when entering normal text
 *           at the prompt.  i call these local and global macros.  a global
 *           macro is when this function is called from a running macro.
 *           the running macro can enter text and return from this function
 *           w/o any action from the user.  a local macro is when the user
 *           presses a key inside this function, which happens quite often
 *           when keys are assigned to ASCII and Extended ASCII characters.
 *
 * jmh 980718: If a global macro executes another macro, make it the local
 *              macro. If the global macro finishes before entry is complete
 *              (which can only happen from a config definition) then assume
 *              Rturn rather than AbortCommand.
 *
 * jmh 980727: If a global macro pauses, continue input from the keyboard.
 * jmh 980731: Use NextLine and BegNextLine (ie. Shift+ & Ctrl+Enter) to copy
 *              the word and string from the current window.
 * jmh 980813: moved from utils.c and added WordLeft and WordRight functions.
 *             Removed color parameter; always use g_display.message_color.
 *
 * jmh 990424: Added history.
 * jmh 990425: Added filename completion. This filename completion does not
 *              currently support reverse cycle, but does allow wildcards. This
 *              means that "*.tdm", for example, will be used as the pattern,
 *              not "*.tdm*" that would normally occur.
 * jmh 990505: don't display anything in a macro unless Pause is used;
 *             consolidated all the display updates into one place;
 *             allow the name to be longer than the screen; assume if it's file
 *              history the maximum length is PATH_MAX, otherwise MAX_COLS.
 * jmh 990506: deleted lines are placed in the history;
 *             added WordDelete, WordDeleteBack and Transpose operations;
 *             use normal syntax coloring, when appropriate, to display blanks.
 * jmh 990507: the delete words delete a group of whitespace or non-whitespace,
 *              but not both.
 * jmh 010521: fixed bug with copy_word() doing nothing when it was first.
 * jmh 021021: added clipboard support;
 *             added SetDirectory.
 * jmh 021028: made filename completion complete prefix before cycling.
 * jmh 021029: added history completion (when not filename completing).
 * jmh 030226: added ToggleCWD.
 * jmh 030316: return the length of the answer;
 *             added DelEndOfLine and DelBegOfLine.
 * jmh 031115: added column and width parameters, dialog processing;
 *             replaced Tab history completion with Ctrl+Up (ScrollUpLine)
 *              and Ctrl+Down (ScrollDnLine).
 * jmh 031119: recognise StreamDeleteChar;
 *             using (Stream)DeleteChar first will clear the previous answer.
 * jmh 050705: close the handle used by filename completion.
 * jmh 050710: Pause the entire dialog, not just the current edit field.
 * jmh 050721: GotoMark<n> will move to the <n>th edit field.
 */
int  get_string( int col, int line, int width, int normal, int blank,
                 char *name, HISTORY *hist )
{
long c = 0;                     /* character user just typed */
int  func = 0;                  /* function of key pressed */
char *cp;                       /* cursor position in answer */
int  pos;                       /* cursor position in answer */
#if PATH_MAX > MAX_COLS
char answer[PATH_MAX];          /* user's answer */
#else
char answer[MAX_COLS];          /* user's answer */
#endif
int  first = TRUE;              /* first character typed */
register int len;               /* length of answer */
int  max;                       /* maximum length allowed */
int  scol;                      /* start column */
int  stop;                      /* flag to stop getting characters */
char *p;                        /* for copying text in answer */
MACRO *local_macro = NULL;
int  next = 0;   
int  paused = FALSE;            /* Is a global macro wanting keyboard input? */
HISTORY *h;                     /* Position within history list */
int  complete = FALSE;          /* Starting or continuing filename completion */
char cname[NAME_MAX+2];
int  stem = 0;
int  prefix;
FTYPE *ftype = NULL;
FFIND dta;
int  done_empty = FALSE;        /* do dialog callback for empty string */
int  done_one = FALSE;          /* do dialog callback for non-empty string */
int  i;
int  color = Color( Message );
int  displayed = FALSE;
int  reset = TRUE;
char *disp = NULL;              /* the first position to redisplay */
int  dlen = 0;                  /* number of characters to redisplay */
int  dcol = 0;                  /* position to begin redisplay */
int  max_col;                   /* last column on screen */
int  cols;                      /* number of columns available */
char *old_cp;

#define DLEN( len ) if ((len) > dlen) dlen = (len)

   strcpy( answer, name );
   h = hist;
   max = (h == &h_file || h == &h_exec) ? PATH_MAX-1 : MAX_COLS-1;
   max_col = col + width - 1;
   cols = width;

   /*
    * set the case sensitivity of history completion
    */
   sort.order_array = (mode.search_case == IGNORE) ?
                      sort_order.ignore : sort_order.match;

   /*
    * check if a dialog has already been paused.
    */
   if (dlg_active && g_status.macro_executing && g_status.macro_next > 1 &&
       g_status.macro_next-1 < g_status.current_macro->len &&
       getfunc( g_status.current_macro->key.keys[g_status.macro_next-1] )
                == Pause)
      paused = TRUE;

   /*
    * let user edit default string
    */
   scol = col;
   len  = strlen( answer );
   col += len;
   old_cp = cp = answer + len;

   for (stop = FALSE; stop == FALSE;) {
      if (displayed) {
         col += (int)(cp - old_cp);
         old_cp = cp;
         if (reset) {
            if (dlg_active)
               display_dialog( );
            else if (prompt_len >= 0)
               display_prompt( );
            disp = cp - (col - scol);
            dcol = scol;
            dlen = cols;
            reset = FALSE;
         }
         if (col < scol) {
            disp = cp;
            dcol = col = scol;
            dlen = cols;
         } else if (col > max_col) {
            disp = cp - cols + 1;
            col  = max_col;
            dcol = scol;
            dlen = cols;
         }
         if (dlen > 0) {
            if (dcol + dlen > max_col + 1)
               dlen = max_col + 1 - dcol;
            for (; dlen > 0 && *disp; --dlen)
               c_output( *disp++, dcol++, line, color );
            c_repeat( blank, dlen, dcol, line, normal );
            dlen = 0;
         }
         xygoto( col, line );

         if (dlg_proc) {
            if (len == 0) {
               if (!done_empty) {
                  dlg_proc( -dlg_id, answer );
                  done_empty = TRUE;
                  done_one   = FALSE;
               }
            } else {
               if (!done_one) {
                  dlg_proc( -dlg_id, answer );
                  done_one   = TRUE;
                  done_empty = FALSE;
               }
            }
         }
      }
      pos = (int)(cp - answer);

      if (local_macro == NULL) {
         if (g_status.macro_executing && !paused) {
            if (g_status.macro_next < g_status.current_macro->len) {
               c    = g_status.current_macro->key.keys[g_status.macro_next++];
               func = getfunc( c );
               if (func == Pause)
                  paused = TRUE;
            } else if (g_status.current_macro->len == 1) {
               c      = _FUNCTION;
               func   = Pause;
               paused = TRUE;
            } else {
               c    = RTURN;
               func = Rturn;
            }
         } else {
            if (!displayed) {
               displayed = TRUE;
               continue;
            }
            c = getkey( );
            /*
             * User may have redefined the Enter and ESC keys.  Make the Enter
             *  key perform a Rturn in this function.  Make the ESC key do an
             *  AbortCommand.
             * jmh 980809: test for F1/Help here as well.
             */
            func = (c == RTURN) ? Rturn        :
                   (c == ESC)   ? AbortCommand :
                   (c == _F1)   ? Help         :
                   getfunc( c );

            if (mode.record && !paused) {
               record_key( c, (dlg_active && _F1 <= c && c <= _F9)
                              ? PlayBack : func );
               if (func == Pause) {
                  paused = TRUE;
                  macro_pause( NULL );  /* dummy argument */
               }
            }
         }
         if (func == PlayBack && !(dlg_active && _F1 <= c && c <= _F9)) {
            local_macro = g_status.key_macro;
            if (local_macro->len == 1) {
               c = local_macro->key.key;
               local_macro = NULL;
            } else {
               next = 0;
               c    = local_macro->key.keys[next++];
            }
            func = getfunc( c );
         }
      } else {
         if (next < local_macro->len) {
            c = local_macro->key.keys[next++];
            func = getfunc( c );
         } else {
            local_macro = NULL;
            c = _FUNCTION;
            func = 0;
         }
      }

      if (dlg_active && _F1 <= c && c <= _F9)  {
         func = 0;
         i = (int)c - _F1;
         if (i < num_cbox) {
            i += first_cbox;
            if (check_box( i ))
               if (dlg_proc)
                  dlg_proc( i, NULL );
         }
      }

      switch (func) {
         case Help :
            if (show_help( ) == ERROR) {
               reset = TRUE;
               dlg_drawn = FALSE;
            }
            break;

         case ToggleSearchCase :
            toggle_search_case( NULL );      /* Note: dummy argument */
            sort.order_array = (mode.search_case == IGNORE) ?
                               sort_order.ignore : sort_order.match;
            break;

         case ToggleOverWrite :
            toggle_overwrite( NULL );        /* Note: dummy argument */
            break;

         case SetDirectory:
            set_path( g_status.current_window );
            break;

         case ToggleCWD: {
            char cwd[PATH_MAX];
            char buf[PATH_MAX];
            if (get_current_directory( cwd ) == ERROR)
               strcpy( cwd, "." );
            error( INFO, (dlg_active) ? g_display.end_line :
                         (line == 0) ? 1 : line - 1,
                   reduce_string( buf, cwd, g_display.ncols, MIDDLE ) );
            break;
         }

#if defined( __DJGPP__ ) || defined( __WIN32__ )
         case CopyToClipboard:
         case KopyToClipboard:
         case CutToClipboard:
            if (len)
               set_clipboard( answer, len + 1 );
            break;

         case PasteFromClipboard:
#endif
         case CopyWord   :
         case CopyString :
            if (g_status.current_window != NULL
#if defined( __DJGPP__ ) || defined( __WIN32__ )
                ||  func == PasteFromClipboard
#endif
               ) {
               if (first) {
                  old_cp = cp = answer;
                  *cp = '\0';
                  dlen = len;
                  len = 0;
                  pos = 0;
                  col = scol;
               }
               disp = cp;
               dcol = col;
#if defined( __DJGPP__ ) || defined( __WIN32__ )
               if (func == PasteFromClipboard)
                  i = copy_clipboard( cp, max - (mode.insert ? len : pos) );
               else
#endif
                  i = copy_word( g_status.current_window, cp,
                                 max - (mode.insert ? len : pos),
                                 func == CopyString );
               if (i > 0) {
                  if (mode.insert) {
                     DLEN( len - pos + i );
                     len += i;
                  } else {
                     DLEN( i );
                     if (i + pos > len)
                        len = i + pos;
                  }
                  answer[len] = '\0';
                  cp += i;
               }
            }
            break;

         case GotoMark1  :
         case GotoMark2  :
         case GotoMark3  :
            if (!dlg_tab)
               break;
            dlg_move = first_edit + func - GotoMark1 - dlg_id;
            if (dlg_move)
               goto done;
            break;

         case ScreenUp   :
         case ScreenDown :
            if (!dlg_tab)
               break;
            /*
             * finished (with this edit field)
             */
         case Rturn      :
            if (dlg_tab)
               dlg_move = (func == ScreenUp) ? -1 : (func == ScreenDown);
         done:
            if (dlg_proc && dlg_proc( dlg_id, answer ) != OK)
               break;
            strcpy( name, answer );
            add_to_history( name, hist );
            /*
             * finished
             */
         case AbortCommand :
            stop = TRUE;
            break;

         case BackSpace :
            /*
             * delete to left of cursor
             */
            if (pos > 0) {
               for (p = cp-1; p < answer+len; p++)
                  *p = *(p+1);
               if (pos > col - scol) {
                  dlen = col++ - scol;
                  dcol = scol;
                  disp = --cp - dlen;
               } else {
                  disp = --cp;
                  dcol = col - 1;
                  dlen = len - pos + 1;
               }
               --len;
            }
            break;

         case DeleteChar :
         case StreamDeleteChar :
            /*
             * delete char under cursor
             */
            if (first) {
               /*
                * delete previous answer
                */
               disp = cp = answer;
               dcol = scol;
               dlen = len;
               *cp = '\0';
               len = 0;

            } else if (*cp) {
               for (p = cp; p < answer+len; p++)
                  *p = *(p+1);
               disp = cp;
               dcol = col;
               dlen = len-- - pos;
            }
            break;

         case DeleteLine :
            /*
             * delete current line
             */
            add_to_history( answer, hist );
            disp = cp = answer;
            dcol = scol;
            dlen = len;
            *cp = '\0';
            len = 0;
            break;

         case DelEndOfLine :
            /*
             * delete from the cursor to the end
             */
            disp = cp;
            dcol = col;
            dlen = len - pos;
            *cp = '\0';
            len = pos;
            break;

         case DelBegOfLine :
            /*
             * delete from before the cursor to the start
             */
            disp = answer;
            dcol = scol;
            dlen = len;
            len -= pos;
            memcpy( answer, cp, len + 1 );
            cp = answer;
            break;

         case CharLeft :
         case StreamCharLeft :
            /*
             * move cursor left
             */
            if (pos > 0)
               cp--;
            break;

         case CharRight :
         case StreamCharRight :
            /*
             * move cursor right
             */
            if (*cp)
               cp++;
            break;

         case WordLeft       :
         case WordDeleteBack :
            /*
             * move cursor to beginning of word
             */
            if (pos > 0) {
               p = cp--;
               while (cp >= answer && myiswhitespc( *cp ))
                  --cp;
               if (func != WordDeleteBack || cp == p-1)
                  while (cp >= answer && !myiswhitespc( *cp ))
                     --cp;
               ++cp;
               if (func == WordDeleteBack) {
                  i = (int)(p - cp);
                  disp = cp;
                  dcol = col - i;
                  dlen = len - pos + i;
                  len -= i;
                  for (i = len+i - pos; i >= 0; --i)
                     *cp++ = *p++;
                  cp = disp;
               }
            }
            break;

         case WordRight  :
         case WordDelete :
            /*
             * move cursor to beginning of next word.
             * if the next word is off-screen, display eight characters.
             */
            if (*cp) {
               p = cp;
               while (*cp && !myiswhitespc( *cp ))
                  ++cp;
               if (func != WordDelete || cp == p)
                  while (*cp && myiswhitespc( *cp ))
                     ++cp;
               i = (int)(cp - p);
               if (func == WordDelete) {
                  disp = p;
                  dcol = col;
                  dlen = len - pos;
                  len -= i;
                  for (i = len - pos; i >= 0; --i)
                     *p++ = *cp++;
                  cp = disp;
               } else if (col + i > max_col && *cp) {
                  col = max_col - 7;
                  old_cp = cp;
                  disp = cp - (cols - 8);
                  dcol = scol;
                  dlen = cols;
               }
            }
            break;

         case Transpose :
            if (len >= 2) {
             char temp;
               disp = p = cp;
               dcol = col;
               dlen = 2;
               if (pos == 0)
                  ++p;
               else if (!*cp) {
                  --p;
                  disp -= 2;
                  dcol -= 2;
               } else {
                  if (col == scol)
                     dlen = 1;
                  else {
                     --disp;
                     --dcol;
                  }
               }
               temp = *p;
               *p = *(p-1);
               *(p-1) = temp;
            }
            break;

         case BegOfLine :
            /*
             * move cursor to start of line
             */
            cp = answer;
            break;

         case EndOfLine :
            /*
             * move cursor to end of line
             */
            cp = answer + len;
            break;

         case LineUp:
         case LineDown:
            /*
             * move through the history
             */
            if (hist != NULL) {
               do
                  h = (func == LineUp) ? h->prev : h->next;
               while (strcmp( answer, h->str ) == 0 && h != hist);
               strcpy( answer, h->str );
               dlen = len;
               len = h->len;
               DLEN( len );
               old_cp = cp = answer + len;
               col = scol + len;
               disp = answer;
               dcol = scol;
            }
            break;

         case Tab:
         case BackTab:
            /*
             * filename completion
             */
            if (hist == &h_file || hist == &h_exec) {
               if (complete == TRUE + 1) {
                  complete = TRUE + 2;
                  join_strings( answer + stem, cname, "*" );
                  ftype = my_findfirst( answer, &dta, MATCH_DIRS );
               } else if (complete)
                  ftype = my_findnext( &dta );
               else {
                  complete = TRUE;
                  for (stem = len - 1; stem >= 0 && answer[stem] != '/'
#if !defined( __UNIX__ )
                         && answer[stem] != '\\' && answer[stem] != ':'
#endif
                       ; --stem) ;
                  ++stem;
                  strcpy( cname, answer + stem );
                  if (!is_pattern( cname )) {
                     *(answer + len) = '*';
                     *(answer + len + 1) = '\0';
                     ftype = my_findfirst( answer, &dta, MATCH_DIRS );
                     if (ftype != NULL) {
                        prefix = strlen( ftype->fname );
                        strcpy( cname, ftype->fname );
                        for (; (ftype = my_findnext( &dta )) != NULL; ) {
                           for (i = 0; i < prefix; ++i) {
#if defined( __UNIX__ )
                              if (ftype->fname[i] != cname[i])
#else
                              if (bj_tolower( ftype->fname[i] ) !=
                                  bj_tolower( cname[i] ))
#endif
                                 break;
                           }
                           complete = TRUE + 1;
                           cname[prefix = i] = '\0';
                        }
                     }
                  } else
                     ftype = my_findfirst( answer, &dta, MATCH_DIRS );
               }
               if (ftype == NULL) {
                  strcpy( answer + stem, cname );
                  if (complete != TRUE + 1)
                     --complete;
               } else
                  strcpy( answer + stem, ftype->fname );
               dlen = len;
               len = strlen( answer );
               DLEN( len );
               old_cp = cp = answer + len;
               col = scol + len;
               disp = answer;
               dcol = scol;

            } else if (dlg_tab) {
               dlg_move = (func == Tab) ? 1 : -1;
               goto done;
            }
            break;

         case ScrollUpLine:
         case ScrollDnLine:
            if (hist != NULL) {
               /*
                * Complete the current answer from the history.
                * Only complete up to the current position, not the
                * entire string.
                */
               HISTORY *find = search_history( answer, pos, hist, h,
                                               (func == ScrollDnLine) );
               if (find != NULL) {
                  h = find;
                  strcpy( answer, h->str );
                  if (mode.search_case == IGNORE) {
                     dlen = len;
                     len = h->len;
                     DLEN( len );
                     disp = cp - (col - scol);
                     dcol = scol;
                  } else {
                     dlen = len - pos;
                     len  = h->len;
                     DLEN( len - pos );
                     disp = cp;
                     dcol = col;
                  }
               }
            }
            break;

         default :
            if (c < 0x100) {
               /*
                * insert character at cursor
                */
               if (first) {
                  /*
                   * delete previous answer
                   */
                  old_cp = cp = answer;
                  *cp = '\0';
                  dlen = len;
                  len = 0;
                  pos = 0;
                  col = scol;
               }

               /*
                * insert new character
                */
               if (len < max) {
                  if (*cp == '\0') {
                     ++len;
                     *(answer + len) = '\0';
                  } else if (mode.insert) {
                     for (p = answer+len; p >= cp; p--)
                        *(p+1) = *p;
                     ++len;
                     DLEN( len - pos );
                  }
                  *cp = (char)c;
                  DLEN( 1 );
                  disp = cp++;
                  dcol = col;
               }
            }
            break;
      }
      if (func != Pause)
         first = FALSE;
      if (func != Tab)
         complete = FALSE;
   }
   if (displayed && paused)
      show_modes( );
   if (ftype != NULL)
      my_findclose( &dta );

   return( func == AbortCommand ? ERROR : len );
}


static unsigned char *sid;      /* syntax identifier */
static int  isnotid( int c )
{
  return( !(sid[(unsigned char)c] & ID_INWORD) );
}


/*
 * Name:    copy_word
 * Purpose: Copy a word from the current window into a buffer
 * Author:  Jason Hood
 * Date:    July 31, 1998
 * Passed:  window:  current window
 *          buffer:  buffer to store word
 *          len:     length of buffer
 *          str:     non-zero to copy a string
 * Returns: number of characters copied.
 * Notes:   uses the mode.insert flag.
 *          If the current window is not on a word, does nothing.
 *          Expects buffer to be zero-terminated, but does not leave
 *           the new buffer zero-terminated.
 * jmh 990421: correct beyond eol bug.
 * jmh 991126: use the line buffer, if appropriate.
 * jmh 050711: use syntax highlighting's inword if copying a word.
 */
int  copy_word( TDE_WIN *window, char *buffer, int len, int str )
{
text_ptr line = window->ll->line;
int  llen = window->ll->len;
int  rcol;
int  (*space)( int );   /* Function to determine what a word is */
int  end;

   if (llen == EOF)
      return( 0 );

   if (g_status.copied && window->ll == g_status.buff_node) {
      line = g_status.line_buff;
      llen = g_status.line_buff_len;
   }
   rcol = window->rcol;
   if (window->file_info->inflate_tabs)
      rcol = entab_adjust_rcol( line, llen, rcol,
                                window->file_info->ptab_size );
   if (rcol >= llen)
      return( 0 );

   if (!str  &&  window->syntax) {
      sid = window->file_info->syntax->info->identifier;
      space = isnotid;
   } else
      space = (str) ? bj_isspc : myiswhitespc;
   if (space( line[rcol] ))
      return( 0 );

   end = rcol;

   while (--rcol >= 0 && !space( line[rcol] )) ;
   ++rcol;
   while (++end < llen && !space( line[end] )) ;
   llen = end - rcol;
   end = (mode.insert) ? strlen( buffer ) : 0;
   if (llen > len - end)
      llen = len - end;

   if (mode.insert)
      memmove( buffer + llen, buffer, end );
   my_memcpy( (text_ptr)buffer, line + rcol, llen );

   return( llen );
}


#if defined( __DJGPP__ ) || defined( __WIN32__ )
/*
 * Name:    copy_clipboard
 * Purpose: Copy the clipboard into a buffer
 * Author:  Jason Hood per David J Hughes
 * Date:    October 21, 2002
 * Passed:  buffer:  buffer to store word
 *          len:     length of buffer
 * Returns: number of characters copied.
 * Notes:   only copies the first line of the clipboard.
 *          Uses the mode.insert flag.
 *          Expects buffer to be zero-terminated, but does not leave
 *           the new buffer zero-terminated.
 */
int  copy_clipboard( char *buffer, int len )
{
char *text = get_clipboard();
int  clen;
char *p;
int  end;

   if (text == NULL)
      return( 0 );

   if ((p = strchr( text, '\r' )) != NULL)
      clen = p - text;
   else
      clen = strlen( text );

   end = (mode.insert) ? strlen( buffer ) : 0;
   if (clen > len - end)
      clen = len - end;

   if (mode.insert)
      memmove( buffer + clen, buffer, end );
   my_memcpy( (text_ptr)buffer, text, clen );

   free( text );

   return( clen );
}
#endif


/*
 * Name:    get_number
 * Purpose: prompt for a numerical value
 * Author:  Jason Hood
 * Date:    November 6, 2002
 * Passed:  prompt: prompt to offer the user
 *          line:   line to display prompt
 *          num:    default answer
 *          hist:   history to search/add answer to
 * Returns: num:    user's answer
 *          OK if user entered something
 *          ERROR if user aborted the command
 * Notes:   an empty string or invalid number returns ERROR.
 *          if num is -1, no default is used.
 */
int  get_number( const char *prompt, int line, long *num, HISTORY *hist )
{
char answer[MAX_COLS];
int  rc;

   if (*num != -1)
      my_ltoa( *num, answer, 10 );
   else
      *answer = '\0';
   rc = get_name( prompt, line, answer, hist );
   if (!bj_isdigit( *(answer + (*answer == '-')) ))
      rc = ERROR;
   if (rc != ERROR) {
      *num = atol( answer );
      rc = OK;
   }
   return( rc );
}


/*
 * Name:    get_response
 * Purpose: to prompt the user and wait for a key to be pressed
 * Date:    August 13, 1998
 * Author:  Jason Hood
 * Passed:  prompt: prompt to offer the user
 *          line:   line to display prompt
 *          flag:   see below
 *          num:    number of responses
 *          let:    first response letter
 *          res:    first response code
 * Returns: the appropriate response for the letter
 * Notes:   prompt is displayed using set_prompt.
 *          If prompt is NULL, no prompt is output, line has no meaning and the
 *           R_PROMPT flag is ignored. The cursor remains unchanged.
 *          If the flag contains R_PROMPT, the response letters are displayed
 *           in brackets, in lower-case, separated by slashes, followed by a
 *           colon and space.
 *          If the flag contains R_MACRO, allow the response to be read from,
 *           or recorded to, a macro.
 *          If the flag contains R_DEFAULT, pressing RTURN/Rturn will accept
 *           the first response (ie. res).
 *          If the flag contains R_ABORT, pressing ESC/AbortCommand will cancel
 *           the query and return ERROR.
 */
int  get_response( const char* prompt, int line, int flag,
                   int num, int let1, int res1, ... )
{
long c;                 /* the user's response */
register int rc;        /* return code */
int  let[20];           /* twenty responses should be plenty */
int  res[20];
char buf[MAX_COLS+2];
int  col = 0;
int  i;
va_list ap;
DISPLAY_BUFF;

   assert( num < 20 );

   SAVE_LINE( line );

   let[0] = let1;
   res[0] = res1;
   va_start( ap, res1 );
   for (col = 1; col < num; ++col) {
      let[col] = bj_toupper( va_arg( ap, int ) );
      res[col] = va_arg( ap, int );
   }
   va_end( ap );

   if (prompt != NULL) {
      col = strlen( prompt );
      strcpy( buf, prompt );
      if (flag & R_PROMPT) {
         buf[col++] = ' ';
         buf[col++] = '(';
         for (i = 0; i < num; ++i) {
            buf[col++] = bj_tolower( let[i] );
            buf[col++] = '/';
         }
         buf[col-1] = ')';
         buf[col++] = ':';
         buf[col++] = ' ';
         buf[col++] = '\0';
      }
      set_prompt( buf, line );
   }
   for (;;) {
      c  = (flag & R_MACRO) ? getkey_macro( ) : getkey( );
      rc = 0;
      if (c < 256) {
         c = bj_toupper( (int)c );
         for (i = 0; i < num && let[i] != (int)c; ++i) ;
         if (i < num) {
            rc = res[i];
            break;
         }
      } else {
         rc = (c == RTURN) ? Rturn        :
              (c == ESC)   ? AbortCommand :
              getfunc( c );

         if (rc == Rturn && (flag & R_DEFAULT)) {
            rc = res1;
            break;
         } else if (rc == AbortCommand && (flag & R_ABORT)) {
            rc = ERROR;
            break;
         }
      }
   }
   RESTORE_LINE( line );
   return( rc );
}


/*
 * Name:    prompt_key
 * Purpose: display a prompt and get a key
 * Author:  Jason Hood
 * Date:    September 22, 2005
 * Passed:  prompt:      the prompt to display
 *          prompt_line: the line to display it
 * Returns: the key
 * Notes:   works with macros, but will ignore the currently recording key.
 *          doesn't display the prompt when reading from a macro.
 */
long prompt_key( const char *prompt, int prompt_line )
{
DISPLAY_BUFF;
int  display;
long key;

   display = (!g_status.macro_executing  ||
               (g_status.macro_next < g_status.current_macro->len  &&
                getfunc( g_status.current_macro->key.keys[g_status.macro_next] )
                  == Pause));

   if (display) {
      SAVE_LINE( prompt_line );
      set_prompt( prompt, prompt_line );
   }
   for (;;) {
      key = getkey_macro( );
      if (mode.record && key == g_status.recording_key)
         show_avail_strokes( +1 );
      else
         break;
   }
   if (display)
      RESTORE_LINE( prompt_line );

   return( key );
}


/*
 * Name:     get_sort_order
 * Purpose:  To prompt the user and get sort direction
 * Date:     June 5, 1992
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen
 * Passed:   window
 * Returns:  OK if user entered something
 *           ERROR if user aborted the command
 *
 * jmh 980813: moved from utils.c
 */
int  get_sort_order( TDE_WIN *window )
{
register int c;

   /*
    * sort ascending or descending
    */
   c = get_response( utils4, window->bottom_line, R_ALL,
                     2, L_ASCENDING, ASCENDING, L_DESCENDING, DESCENDING );
   if (c != ERROR) {
      sort.direction = c;
      c = OK;
   }
   return( c );
}


/*
 * Name:    add_to_history
 * Purpose: add a string to a history list
 * Author:  Jason Hood
 * Date:    April 24, 1999
 * Passed:  str:  string to add
 *          hist: history to add it to
 * Returns: nothing
 * Notes:   if the string already exists, move it to the end. The comparison
 *           is case sensitive.
 *          Silently fail if there's no memory.
 * 991028:  don't add "=" to file history.
 */
void add_to_history( char *str, HISTORY *hist )
{
HISTORY* h;
int len;
int rc = OK;

   if (hist == NULL || str == NULL || *str == '\0')
      return;

   len = strlen( str );
   if (hist == &h_file && *str == '=' && len == 1)
      return;

   for (h = hist->prev; h != hist; h = h->prev) {
      if (h->len == len && strcmp( h->str, str ) == 0)
         break;
   }
   if (h == hist) {
      h = my_malloc( sizeof(HISTORY) + len, &rc );
      if (rc == ERROR)
         return;
      strcpy( h->str, str );
      h->len = len;
   } else {
      h->prev->next = h->next;
      h->next->prev = h->prev;
   }
   h->prev = hist->prev;
   h->next = hist;
   hist->prev->next = h;
   hist->prev = h;
}


/*
 * Name:    search_history
 * Purpose: search the history for a matching prefix
 * Author:  Jason Hood
 * Date:    October 29, 2002
 * Passed:  prefix:  the prefix to match
 *          len:     the length of the prefix
 *          hist:    the history to search
 *          h:       the current position within history
 *          dir:     TRUE to search forwards
 * Returns: pointer to history line, or NULL if no match
 * Notes:   uses the current search case to perform the comparison
 *
 * 031213:  fixed not matching the start item if it's the only.
 */
static HISTORY *search_history( char *prefix, int len,
                                HISTORY *hist, HISTORY *h, int dir )
{
HISTORY *start = h;
int  rc = 1;

   do {
      h = (dir) ? h->next : h->prev;
      rc = my_memcmp( (text_ptr)prefix, (text_ptr)h->str, len );
   } while (rc != 0 && h != start);

   if (rc == 0)
      return( h );

   return( NULL );
}


/*
 * Name:    do_dialog
 * Purpose: process a dialog box
 * Author:  Jason Hood
 * Date:    November 15, 2003
 * Passed:  dlg:   the dialog to process
 * Returns: OK:    accept values
 *          ERROR: ignore values
 * Notes:   cancelling the dialog does NOT restore the original values.
 */
int  do_dialog( DIALOG* dlg, DLG_PROC proc )
{
char answer[PATH_MAX];
Char *buffer;
int  rc;

   dlg_active = TRUE;

   init_dialog( dlg );
   dlg_proc = proc;
   dlg_tab = (first_edit != last_edit);

   dlg_id = dlg->n;
   if (dlg_id < first_edit || dlg_id > last_edit)
      dlg_id = first_edit;

   g_display.output_space = g_display.frame_space;
   buffer = malloc( (dlg->x + 4) * (dlg->y + 1) * sizeof(Char) );
   if (buffer != NULL)
      save_area( buffer, dlg->x, dlg->y, dlg_row, dlg_col );
   g_display.output_space = FALSE;

   while (TRUE) {
      if (dlg[dlg_id].text != NULL)
         strcpy( answer, dlg[dlg_id].text );
      else
         *answer = '\0';
      hlight_label( dlg_id, TRUE );
      rc = get_string( dlg_col + dlg[dlg_id].x, dlg_row + dlg[dlg_id].y,
                       dlg[dlg_id].n, Color( Dialog ), '_',
                       answer, dlg[dlg_id].hist );
      if (rc == ERROR)
         break;
      rc = set_dlg_text( dlg + dlg_id, answer );
      if (rc == ERROR) {
         error( WARNING, g_display.end_line, main4 );
         break;
      }
      if (dlg_move == 0) {
         if (dlg_proc) {
            rc = dlg_proc( 0, NULL );
            if (rc != OK) {
               if (rc != ERROR)
                  dlg_id = rc;
               continue;
            }
         }
         break;
      }
      hlight_label( dlg_id, FALSE );
      dlg_id += dlg_move;
      if (dlg_id > last_edit)
         dlg_id = first_edit;
      else if (dlg_id < first_edit)
         dlg_id = last_edit;
      dlg->n = dlg_id;
   }
   dlg_tab  = FALSE;
   dlg_move = 0;
   dlg_proc = NULL;

   if (buffer != NULL) {
      g_display.output_space = g_display.frame_space;
      restore_area( buffer, dlg->x, dlg->y, dlg_row, dlg_col );
      g_display.output_space = FALSE;
      free( buffer );
   } else {
      if (g_status.current_window != NULL)
         redraw_screen( g_status.current_window );
   }

   dlg_active = FALSE;
   return( rc );
}


/*
 * Name:    check_box
 * Purpose: process a check box in the current dialog
 * Author:  Jason Hood
 * Date:    November 15, 2003
 * Passed:  id: number of the check box to process
 * Returns: FALSE if check box is disabled, TRUE otherwise.
 */
int  check_box( int id )
{
DIALOG *cb;

   cb = current_dlg + id;
   if (cb->hist == NULL) {
      cb->n = !cb->n;
      if (dlg_drawn)
         c_output( (cb->n) ? CHECK : ' ', dlg_col + cb->x + 4, dlg_row + cb->y,
                   Color( Dialog ) );
   }
   return( cb->hist == NULL );
}


/*
 * Name:    check_box_enabled
 * Purpose: enable or disable a check box
 * Author:  Jason Hood
 * Date:    November 30, 2003
 * Passed:  id:     number of the check box
 *          state:  TRUE to enable, FALSE to disable
 */
void check_box_enabled( int id, int state )
{
DIALOG *cb;

   cb = current_dlg + id;
   cb->hist = (state) ? NULL : (HISTORY*)ERROR;
   if (dlg_drawn)
      hlight_line( dlg_col + cb->x, dlg_row + cb->y, 7 + strlen( cb->text ),
                   (state) ? Color( Dialog ) : Color( Disabled ) );
}


/*
 * Name:    init_dialog
 * Purpose: initialise local global dialog variables
 * Author:  Jason Hood
 * Date:    November 15, 2003
 * Passed:  dlg: dialog to initialise
 * Notes:   in a macro, reset all check boxes and set focus to the first edit
 *           field, to ensure consistency, unless UsesDialog flag is used.
 */
static void init_dialog( DIALOG *dlg )
{
int  macro;
int  j;

   current_dlg = dlg;

   macro = (mode.record  ||
            (g_status.macro_executing  &&
             (!(g_status.current_macro->flag & USESDIALOG)  &&
              g_status.current_macro->len != 1)));

   if (macro)
      dlg->n = 0;

   first_edit = last_edit = 0;
   first_cbox = num_cbox  = 0;
   for (j = 1; dlg[j].x != 0; ++j) {
      /*
       * count the number of check boxes and
       *  set the index of the first
       */
      if (dlg[j].n == FALSE || dlg[j].n == TRUE) {
         if (++num_cbox == 1)
            first_cbox = j;
         if (macro)
            dlg[j].n = FALSE;

      /*
       * set the index of the first and last edit fields.
       */
      } else if (dlg[j].n != ERROR) {
         if (first_edit == 0)
            first_edit = j;
         last_edit = j;
      }
   }
   dlg->n  = first_edit;
   dlg_col = (g_display.ncols - dlg->x) / 2;
   dlg_row = (g_display.mode_line - dlg->y) / 2;
   if (dlg_col < 0)
      dlg_col = 0;
   if (dlg_row < 0)
      dlg_row = 0;

   dlg_drawn = FALSE;
}


/*
 * Name:    display_dialog
 * Purpose: display the current dialog
 * Author:  Jason Hood
 * Date:    November 15, 2003
 * Passed:  nothing, but uses local globals
 */
static void display_dialog( void )
{
int  x, y, n;
char *t;
int  w, c;
int  j, sel;
char line[MAX_COLS];

   if (dlg_drawn)
      return;

   create_frame( current_dlg->x, current_dlg->y, 0, NULL, dlg_col, dlg_row );

   sel = (first_edit == last_edit) ? 0
         : current_dlg->n + first_edit - last_edit - 1;
   for (j = 1; current_dlg[j].x != 0; ++j) {
      x = dlg_col + current_dlg[j].x;
      y = dlg_row + current_dlg[j].y;
      t = current_dlg[j].text;
      n = current_dlg[j].n;
      if (n == ERROR)
         s_output( t, y, x, (j == sel) ? Color( EditLabel ) : Color( Dialog ) );
      else if (n == FALSE || n == TRUE) {
         sprintf( line, "F%d [%c] %s", j-first_cbox+1, (n) ? CHECK : ' ', t );
         s_output( line, y, x, (current_dlg[j].hist == NULL)
                               ? Color( Dialog ) : Color( Disabled ) );
      } else {
         if (t != NULL) {
            w = strlen( t );
            if (w > n) {
               w = n;
               c = t[w];
               t[w] = '\0';
            } else
               c = '\0';
            s_output( t, y, x, Color( Message ) );
            t[w] = c;
            x += w;
            w = n - w;
         } else
            w = n;
         c_repeat( '_', w, x, y, Color( Dialog ) );
      }
   }

   dlg_drawn = TRUE;
}


/*
 * Name:    hlight_label
 * Purpose: change the color of a dialog's label
 * Author:  Jason Hood
 * Date:    August 17, 2005
 * Passed:  id:  number of the edit field
 *          sel: TRUE if id is the selected edit field
 * Notes:   init_dialog should have been called first.
 *          don't highlight if there's only one edit field.
 */
static void hlight_label( int id, int sel )
{
   if (dlg_drawn  &&  first_edit != last_edit) {
      id += first_edit - last_edit - 1;
      if (current_dlg[id].n == ERROR)
         hlight_line( dlg_col + current_dlg[id].x, dlg_row + current_dlg[id].y,
                      strlen( current_dlg[id].text ),
                      (sel) ? Color( EditLabel ) : Color( Dialog ) );
   }
}


/*
 * Name:    set_dlg_text
 * Purpose: set the text of a dialog's edit field
 * Author:  Jason Hood
 * Date:    November 15, 2003
 * Passed:  edit:  the field to set
 *          text:  the text
 * Returns: OK if set, ERROR if out of memory
 */
int  set_dlg_text( DIALOG *edit, const char *text )
{
char *temp;
int  len;
int  rc = OK;

   len = strlen( text );
   if (len == 0) {
      if (edit->text != NULL)
         *edit->text = '\0';
   } else {
      temp = my_realloc( edit->text, len + 1, &rc );
      if (len != ERROR) {
         strcpy( temp, text );
         edit->text = temp;
      }
   }
   return( rc );
}


/*
 * Name:    get_dlg_text
 * Purpose: retrieve the text of an edit field
 * Author:  Jason Hood
 * Date:    November 15, 2003
 * Passed:  edit:  the field to get
 * Returns: the text
 */
char *get_dlg_text( DIALOG *edit )
{
   return( (edit->text == NULL) ? "" : edit->text );
}


/*
 * Name:    set_dlg_num
 * Purpose: set a dialog's edit field to a number
 * Author:  Jason Hood
 * Date:    November 15, 2003
 * Passed:  edit: the field to set
 *          num:  the number
 * Returns: OK if set, ERROR if out of memory
 */
int  set_dlg_num( DIALOG *edit, long num )
{
char str[12];

   return( set_dlg_text( edit, my_ltoa( num, str, 10 ) ) );
}


/*
 * Name:    get_dlg_num
 * Purpose: get a dialog's edit field as a number
 * Author:  Jason Hood
 * Date:    November 15, 2003
 * Passed:  edit:  the field to get
 * Returns: the number
 * Notes:   invalid numbers will return 0
 */
long get_dlg_num( DIALOG *edit )
{
   return( (edit->text == NULL) ? 0 : atol( edit->text ) );
}
