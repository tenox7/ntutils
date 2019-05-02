/*******************  start of original comments  ********************/
/*
 * This file contains the utilities to read in a ".tdecfg" file.
 *
 * Most of this stuff is duplicated from the cfgfile.c functions.  In
 *  Linux, this utility searches the CWD first then it searches the
 *  HOME directory for the ".tdecfg" file.
 *
 * Many thanks to <intruder@link.hacktic.nl> for the idea and sample code
 *  for this function.
 *
 *********************  start of Jason's comments  *********************
 *
 * DJGPP also requires the configuration file ("tde.cfg"), so to provide
 *  the same functionality across versions, DOS gets it as well.
 *  I also added a custom help file (".tdehlp" and "tde.hlp").
 *
 * Load the file from the HOME or executable directory first, and then
 *  from the current directory. This allows global and local settings.
 *  The help file is loaded from the current directory, or the HOME /
 *  executable directory.
 *
 *********************   end of Jason's comments   *********************
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


#include "tdestr.h"             /* tde types */
#include "common.h"
#include "define.h"
#include "tdefunc.h"
#include "config.h"


       char *line_in = NULL;    /* input line buffer */
static int  line_in_len = 0;    /* length of above */
       int  line_no;            /* global variable for line count */
       int  process_macro;      /* multi-line macro definition */
static int  stroke_count;       /* global variable for macro strokes */
static long macro_key;
       MACRO *mac;
static TREE *branch;
       TREE *cfg_key_tree;
static int  existing_macro;
static int  key_num;            /* current menu key (jmh 020831) */

static int  need_a_redraw;      /* if we redefined colors, then redraw screen */
static int  need_mode_line;     /* if we redefined modes,  then redraw line */
static int  need_rulers;        /* if we redefined rulers, then redraw ruler */
static int  need_line_numbers;  /* redefining line numbers? */
static int  need_cwd;           /* redefining the cwd display? */

       int  background;         /* default background color (jmh 990420) */

       char *cmd_config = NULL; /* config file from command line (jmh 010605) */

       MENU_STR *user_menu;     /* User menu or Language sub-menu (jmh 031129)*/
extern MENU_LIST popout_default[];
extern MENU_LIST *popout_last;

#if defined( __WIN32__ )
extern HANDLE conin;
#endif

static void new_key_name( char *, char * );
static const char *new_scancode_map( char *, char * );
static int  new_help_screen( FILE*, int );
static const char *process_menu( char *, char * );
static const char *new_menu_item( MENU_STR *, char *, char * );
static void free_menu( MENU_STR * );


/*
 * Name:    tdecfgfile
 * Date:    June 5, 1994
 * Notes:   read in a configuration file at any time.
 *          read in a configuration file when we first fire up TDE in
 *           a linux (unix) and djgpp environment.
 *          August 20, 1997, Jason Hood - read in a help file as well.
 * jmh 980721: replaces load_strokes.
 * jmh 981127: only bring up the directory list if the macro name contains
 *              a wildcard pattern.
 * jmh 990430: blank out help screen if the file contains fewer lines;
 *             read in a viewer help screen.
 * jmh 010604: moved help to readfile();
 *             prompt for a config file;
 *             update the mono "colors" if using MDA.
 * jmh 031130: update the menu.
 */
int  tdecfgfile( TDE_WIN *window )
{
int  rc = OK;
int  prompt_line;
char fname[PATH_MAX];          /* new name for file  */
const char *prompt;
int  i, j;

   prompt_line = (window != NULL) ? window->bottom_line : g_display.end_line;

   need_a_redraw  = FALSE;
   need_mode_line = FALSE;
   need_rulers    = FALSE;
   need_line_numbers = mode.line_numbers;
   need_cwd          = mode.display_cwd;

   key_num = 0;
   background = 1;

   user_menu = &menu[user_idx].menu;

   if (window == NULL) {
      cfg_key_tree = &key_tree;
      /*
       * CONFIGFILE is defined in tdestr.h
       */
      join_strings( fname, g_status.argv[0], CONFIGFILE );
      rc = readfile( fname, prompt_line );
      /*
       * jmh 990213: See if the current directory and executable directory
       *             are the same. If so, don't read the config file twice.
       */
      get_current_directory( fname );
      if (strcmp( fname, g_status.argv[0] ) != 0)
         rc = readfile( CONFIGFILE, prompt_line );
      /*
       * jmh 010605: Read the config file specified on the command line.
       *             Assume it's different to those above.
       */
      if (cmd_config != NULL) {
         rc = readfile( cmd_config, prompt_line );
         cmd_config = NULL;
      }
   } else {
      if (g_status.command == LoadMacro) {
         /*
          * search path or file name for macro file
          */
         prompt = main21;
         strcpy( fname, "*.tdm" );
         cfg_key_tree = (window->syntax) ?
                        &window->file_info->syntax->key_tree :
                        &key_tree;
      } else {
         /*
          * search path or file name for config file
          */
         prompt = main21a;
         strcpy( fname, CONFIGFILE );
         cfg_key_tree = &key_tree;
      }
      if (get_name( prompt, prompt_line, fname, &h_file ) > 0) {
         if (is_pattern( fname ))
            rc = list_and_pick( fname, window );
         /*
          * if everything is everything, load in the file selected by user.
          */
         if (rc == OK)
            rc = readfile( fname, prompt_line );
      } else
         rc = ERROR;
   }
   if (rc == OK) {
      if (need_a_redraw) {
         g_display.color = colour[(g_display.adapter != MDA)];
         if (g_display.adapter != MDA)
            set_overscan_color( Color( Overscan ) );
      }
      if (window != NULL) {
         if (need_a_redraw)
            redraw_screen( window );
         if (need_mode_line && !need_a_redraw)
            show_modes( );
         if (need_rulers)
            show_all_rulers( );
         if (need_line_numbers != mode.line_numbers)
            toggle_line_numbers( NULL );
         if (need_cwd != mode.display_cwd)
            toggle_cwd( NULL );
      } else {
         make_ruler( );
         mode.line_numbers = need_line_numbers;
         mode.display_cwd  = need_cwd;
      }
   }

   if (user_idx == -1)
      process_menu( NULL, "User" );
   if (user_menu && user_menu->minor_cnt == 0)
      new_user_menu( NULL, (char*)config32, FALSE );  /* <Empty> */

   /*
    * remove empty menus (apart from User)
    */
   for (i = menu_cnt - 1; i >= 0; --i) {
      if (menu[i].menu.minor_cnt == 0  &&  i != user_idx) {
         if (*menu[i].major_name != '\0')
            my_free( menu[i].major_name );
         memcpy( menu + i, menu + i+1, (menu_cnt - (i+1)) * sizeof(*menu) );
         if (user_idx > i)
            --user_idx;
         --menu_cnt;
      }
   }
   for (i = menu_cnt - 1; i >= 0; --i) {
      for (j = menu[i].menu.minor_cnt - 2; j >= 1; --j) {
         if (menu[i].menu.minor[j].pop_out  &&
             menu[i].menu.minor[j].pop_out->minor_cnt == 0) {
            MENU_LIST *ml, *po;
            po = NULL;
            for (ml = popout_menu; ml != NULL; ml = ml->next) {
               if (&ml->popout == menu[i].menu.minor[j].pop_out) {
                  if (po == NULL)
                     popout_menu = ml->next;
                  else
                     po->next = ml->next;
                  my_free( ml );
                  my_free( menu[i].menu.minor[i].minor_name );
                  memcpy( menu[i].menu.minor + j, menu[i].menu.minor + j+1,
                          (menu[i].menu.minor_cnt - (j+1)) * sizeof(*menu) );
                  --menu[i].menu.minor_cnt;
                  break;
               }
               po = ml;
            }
         }
      }
   }

   /*
    * initialise the pull-down menus (jmh)
    */
   init_menu( window == NULL );

   return( rc );
}


/*
 * Name:    readfile
 * Date:    May 31, 1998
 * Notes:   helper function for tdecfgfile
 *
 * jmh 010604: read the help file based on the config file - replace trailing
 *              "cfg" with "hlp" (eg: tde.cfg -> tde.hlp; .tdecfg -> .tdehlp;
 *              tde.tdm -> no help file); "cfg" is case sensitive in UNIX;
 *             if the first line of the help file is the separator, don't blank
 *              out the editor help screen; similarly, if there is no separator,
 *              don't blank out the viewer help screen. This allows for global
 *              editor and viewer screens, but only one local screen;
 *             no error is returned if the help file could not be opened, but
 *              the warning is still given;
 *             don't load the help if the config could not be opened;
 *             it's okay to have a help file without a config file, but the
 *              config name must still be given.
 */
int  readfile( char *fname, int prompt_line )
{
FILE *config;
char hname[PATH_MAX];
int  i;
int  rc = OK;

   if (file_exists( fname ) != ERROR) {
      if ((config = fopen( fname, "r" )) == NULL) {
         rc = ERROR;
         combine_strings( line_out, main7a, fname, main7b );
         error( WARNING, prompt_line, line_out );
      } else {
         line_no = 1;
         process_macro = FALSE;
         while (read_line( config ) != EOF  &&  !g_status.control_break) {
            parse_line( line_in, prompt_line );
            ++line_no;
         }
         if (process_macro) {
            /*
             * Unexpected end of file. Tidy up the macro definition.
             */
            check_macro( mac );
            error( WARNING, prompt_line, config26 );
         }

         fclose( config );
      }
   }
   /*
    * See if there's a help file.
    */
   if (rc == OK) {
      strcpy( hname, fname );
      i = strlen( hname );
      if (i >= 3  &&
#if defined( __UNIX__ )
                      strcmp( hname+i-3, "cfg" ) == 0)
#else
                      stricmp( hname+i-3, "cfg" ) == 0)
#endif
         strcpy( hname+i-3, "hlp" );
      if (file_exists( hname ) != ERROR) {
         if ((config = fopen( hname, "r" )) == NULL) {
            combine_strings( line_out, main7a, hname, main7b );
            error( WARNING, prompt_line, line_out );
         } else {
            if (new_help_screen( config, 0 ))
               new_help_screen( config, 1 );
            fclose( config );
         }
      }
   }
   return( rc );
}


/*
 * Name:    read_line
 * Purpose: read a line from a file
 * Date:    October 31, 2002
 * Passed:  file:  the file to read
 * Returns: the length of the line, or EOF.
 * Notes:   reads a line into the global line_in buffer, replacing the line
 *           ending with a NUL. (also replaces remove_cr() used in UNIX.)
 */
int  read_line( FILE *file )
{
int  len = 0;
int  ch  = EOF;

   for (;;) {
      if (len == line_in_len) {
         char *temp = realloc( line_in, line_in_len + 80 );
         if (temp == NULL) {
            if (len == 0)
               return( EOF );
            else {
               --len;
               break;
            }
         }
         line_in = temp;
         line_in_len += 80;
      }
      ch = fgetc( file );
#if defined( __UNIX__ )
      if (ch == '\r') {
         if ((ch = fgetc( file )) != '\n') {
            ungetc( ch, file );
            ch = '\r';
         }
      }
#endif
      if (ch == '\n'  ||  ch == EOF)
         break;
      line_in[len++] = ch;
   }
   line_in[len] = '\0';

   return( (ch == EOF  &&  len == 0) ? EOF : len );
}


/*
 * Name:     parse_line
 * Purpose:  real work horse of the configuration utility, figure out what
 *           we need to do with each line of the config file.
 * Date:     June 5, 1994
 * Passed:   line:  line that contains the text to parse
 * jmh 980720: take into account multi-line macro definitions
 * jmh 980730: I really don't like a lot of if-else-if conditionals, so I
 *              restructured with goto bad_config.
 * jmh 990404: Added the explicit setting of insert/overwrite cursor sizes:
 *              "CursorStyle Large Medium" will use a solid block for insert
 *              and a half-block for overwrite.
 * jmh 990428: Recognize the viewer character functions.
 * jmh 020818: Treat function keys as prefixes and key name;
 *             other modifications based on the new key mapping.
 */
void parse_line( char *line, int prompt_line )
{
char key[1042];         /* buffer to hold any token that we parse */
char *residue;          /* pointer to next item in line, if it exists */
int  key_no;            /* index into key array */
long two_key;           /* two-combination keys */
int  color;             /* color field */
int  mode_index;        /* index in mode array */
int  func_no;           /* function number we want to assign to a key */
int  color_no;          /* attribute we want to assign to a color field */
int  mode_no;           /* mode number we want to assign to a mode */
int  found;             /* boolean, did we find a valid key, color, or mode? */
int  i;
const char *errmsg = NULL;

   if (process_macro) {
      parse_macro( line, prompt_line );
      return;
   }

   /*
    * find the first token and put it in key.  residue points to next token.
    */
   residue = parse_token( line, key );
   if (*key == '\0')
      return;

   /*
    * jmh 980730: there must be something else on the line (except for macros,
    *              which are taken care of above).
    */
   if (residue == NULL) {
      errmsg = config1;         /* setting without value */
      goto bad_config;
   }

   found = FALSE;
   /*
    * try to find a valid key.
    */
   key_no = parse_key( key );
   if (key_no != ERROR) {
      /*
       * find the function assignment
       */
      found = TRUE;
      residue = parse_token( residue, key );
      /*
       * clear any previous macro or key assignment.
       */
      func_no = search( key, valid_func, CFG_FUNCS );
      if (func_no != ERROR) {
         clear_previous_macro( key_no );
         key_func[KEY_IDX( key_no )] = func_no;
         if (func_no == PlayBack) {
            macro_key = key_no;
            if (parse_macro( residue, prompt_line ) == ERROR)
               errmsg = main4;          /* out of memory */
         }
      } else if (key_func[KEY_IDX( key_no )] == TwoCharKey && residue != NULL) {
         two_key = key_no;
         if (*key == '^' && key[1] != '\0') {
            key_no = parse_key( key+1 );
            i = TRUE;
         } else {
            key_no = parse_key( key );
            i = FALSE;
         }
         if (key_no == ERROR) {
            errmsg = config22;       /* unrecognized key */
            goto bad_config;
         }
         two_key = CREATE_TWOKEY( two_key, key_no );
         residue = parse_token( residue, key );
         func_no = search( key, valid_func, CFG_FUNCS );
         if (func_no == PlayBack) {
            macro_key = two_key;
            if (parse_macro( residue, prompt_line ) == ERROR)
               errmsg = main4;
         } else
            errmsg = (func_no == ERROR) ? config3 : /* unrecognized function */
                     (func_no == TwoCharKey) ? config2 : /*two-key not allowed*/
                     (add_twokey( two_key, func_no ) == ERROR) ? main4   :
                                                                 NULL;
         if (i  &&  errmsg == NULL) {
            if (!initialize_macro( two_key ^ _CTRL ))
               errmsg = main4;
            else {
               cfg_record_keys( mac, two_key, 0, prompt_line );
               check_macro( mac );
            }
         }
      } else
         errmsg = config3;
   }

   /*
    * valid key not found, now try a pseudo-macro
    */
   if (!found) {
      if (stricmp( key, func_str[PseudoMacro] ) == 0) {
         found = TRUE;
         residue = parse_token( residue, key );
         if (strlen( key ) != 2 || (text_t)key[0] <= ' ' ||
                                   (text_t)key[1] <= ' ') {
            errmsg = config25;          /* invalid combination */
            goto bad_config;
         }
         macro_key = (unsigned)(((text_t)key[0] << 8) | (text_t)key[1]);
         if (parse_macro( residue, prompt_line ) == ERROR)
            errmsg = main4;
      }
   }

   /*
    * pseudo-macro not found, now try a valid color
    */
   if (!found) {
      color = search( key, valid_colors, (NUM_COLORS * 2) );
      if (color != ERROR) {
         found = TRUE;
         i = (*key == 'c' || *key == 'C');
         color_no = parse_color( &residue, key, prompt_line, config6a );
         if (color_no != ERROR) {
            if (color == 255)
               background = color_no & 15;
            else {
               if (color == Overscan_color)
                  color_no &= 15;
               colour[i][color] = color_no;
               need_a_redraw = TRUE;
            }
         }
      } else {
         /*
          * see if this is a color pair for Linux curses.
          */
         mode_no = search( key, valid_pairs, 7 );
         if (mode_no != ERROR) {

#if 0 //defined( __UNIX__ )
            residue = parse_token( residue, key );
            color = search( key, valid_curse, 7 );
            if (color != ERROR && residue != NULL) {
               /*
                *  so far, we got   pairx  COLOR_y.
                *    now get the "on".
                */
               residue = parse_token( residue, key );
               if (residue != NULL) {
                  /*
                   * now, get the background color.
                   */
                  parse_token( residue, key );
                  color_no = search( key, valid_curse, 7 );

                  /*
                   * we just parsed a color pair line:
                   *    pairx COLOR_y on COLOR_z.
                   */
                  if (color_no != ERROR) {
                     found = TRUE;
                     need_a_redraw = TRUE;
                     init_pair( mode_no, color, color_no );
                     mode_no *= 16;
                     for (i=0; i<8; i++)
                        tde_color_table[mode_no + i]  =  COLOR_PAIR( mode_no );

                     for (i=8; i<16; i++)
                        tde_color_table[mode_no + i]  =
                                   COLOR_PAIR( mode_no ) | A_BOLD;
                  }
               }
            }
            if (found == FALSE)
               errmsg = config24;       /* error parsing color pair */
#else
            /*
             * if we are in MSDOS, don't even bother with parsing
             *   curses junk.
             */
            found = TRUE;
#endif
         }
      }
   }

   /*
    * valid color not found, now try a valid mode
    */
   if (!found) {
      mode_index = search( key, valid_modes, NUM_MODES-1 );
      if (mode_index != ERROR) {
         found = TRUE;

         /*
          * if we find a valid mode, we need to search different
          *   option arrays before we find a valid assignment.
          */
         if (mode_index != KeyName && mode_index != Scancode)
            residue = parse_token( residue, key );
         mode_no = ERROR;
         switch ( mode_index ) {
            case InsertMode         :
            case IndentMode         :
            case SmartTabMode       :
            case TrimTrailingBlanks :
            case Backups            :
            case Ruler              :
            case JustifyRightMargin :
            case CursorCross        :
            case FrameSpace         :
            case Shadow             :
            case LineNumbers        :
            case UndoGroup          :
            case UndoMove           :
            case AutoSaveWorkspace  :
            case TrackPath          :
            case DisplayCWD         :
            case QuickEdit          :
               mode_no = search( key, off_on, 1 );
               if (mode_no == ERROR) {
                  errmsg = config7;     /* off/on error */
                  goto bad_config;
               }
               switch ( mode_index ) {
                  case InsertMode  :
                     mode.insert = mode_no;
                     need_mode_line = TRUE;
                     break;
                  case IndentMode  :
                     mode.indent = mode_no;
                     need_mode_line = TRUE;
                     break;
                  case SmartTabMode:
                     mode.smart_tab = mode_no;
                     need_mode_line = TRUE;
                     break;
                  case TrimTrailingBlanks :
                     mode.trailing = mode_no;
                     need_mode_line = TRUE;
                     break;
                  case Backups     :
                     mode.do_backups = mode_no;
                     break;
                  case Ruler       :
                     mode.ruler = mode_no;
                     need_rulers = TRUE;
                     break;
                  case JustifyRightMargin :
                     mode.right_justify = mode_no;
                     break;
                  case CursorCross :
                     if (mode.cursor_cross != mode_no) {
                        mode.cursor_cross = mode_no;
                        need_a_redraw = TRUE;
                     }
                     break;
                  case FrameSpace :
                     g_display.frame_space = mode_no;
                     break;
                  case Shadow :
                     g_display.shadow = mode_no;
                     break;
                  case LineNumbers :
                     need_line_numbers = mode_no;
                     break;
                  case UndoGroup :
                     mode.undo_group = mode_no;
                     break;
                  case UndoMove :
                     mode.undo_move = mode_no;
                     break;
                  case AutoSaveWorkspace :
                     mode.auto_save_wksp = mode_no;
                     break;
                  case TrackPath :
                     mode.track_path = mode_no;
                     break;
                  case DisplayCWD :
                     need_cwd = mode_no;
                     break;
                  case QuickEdit  :
#if defined( __WIN32__ )
                     {
                     DWORD mode;
                        GetConsoleMode( conin, &mode );
                        mode |= ENABLE_EXTENDED_FLAGS;
                        if (mode_no)
                           mode |= ENABLE_QUICK_EDIT;
                        else
                           mode &= ~ENABLE_QUICK_EDIT;
                        SetConsoleMode( conin, mode );
                     }
#endif
                     break;
               }
               break;

            case DisplayEndOfLine :
               mode_no = search( key, valid_eol, 2 );
               if (mode_no == ERROR) {
                  errmsg = config17;    /* eol display error */
                  goto bad_config;
               }
               mode.show_eol = mode_no;
               need_a_redraw = TRUE;
               break;

            case LTabSize :
            case PTabSize :
               mode_no = atoi( key );
               if (mode_no > MAX_TAB_SIZE || mode_no < 1) {
                  errmsg = config8;     /* tab error */
                  goto bad_config;
               }
               if (mode_index == LTabSize)
                  mode.ltab_size = mode_no;
               else
                  mode.ptab_size = mode_no;
               need_mode_line = TRUE;
               break;

            case LeftMargin :
               mode_no = atoi( key );
               if (mode_no < 1 || mode_no > mode.right_margin) {
                  errmsg = config9;     /* left margin error */
                  goto bad_config;
               }
               mode.left_margin = --mode_no;
               need_rulers = TRUE;
               break;

            case ParagraphMargin :
               mode_no = atoi( key );
               if (mode_no < 1 || mode_no > mode.right_margin) {
                  errmsg = config10;    /* paragraph margin error */
                  goto bad_config;
               }
               mode.parg_margin = --mode_no;
               need_rulers = TRUE;
               break;

            case RightMargin :
               mode_no = atoi( key );
               if (mode_no < mode.left_margin || mode_no > 1040) {
                  errmsg = config11;    /* right margin error */
                  goto bad_config;
               }
               mode.right_margin = --mode_no;
               need_rulers = TRUE;
               break;

            case InflateTabs :
               mode_no = search( key, valid_tabs, 2 );
               if (mode_no == ERROR) {
                  errmsg = config16;    /* inflate tabs error */
                  goto bad_config;
               }
               mode.inflate_tabs = mode_no;
               need_mode_line = TRUE;
               need_a_redraw  = TRUE;
               break;

            case EndOfLineStyle :
               mode_no = search( key, valid_crlf, 3 );
               if (mode_no == ERROR) {
                  errmsg = config12;    /* crlf or lf error */
                  goto bad_config;
               }
               mode.crlf = mode_no;
               break;

            case WordWrapMode :
               mode_no = search( key, valid_wraps, 2 );
               if (mode_no == ERROR) {
                  errmsg = config13;    /* word wrap error */
                  goto bad_config;
               }
               mode.word_wrap = mode_no;
               need_mode_line = TRUE;
               break;

            case CursorStyle :
               mode_no = search( key, valid_cursor, 2 );
               if (mode_no == ERROR) {
                  errmsg = config14;    /* cursor size error */
                  goto bad_config;
               }
               i = mode_no;
               if (residue != NULL) {
                  residue = parse_token( residue, key );
                  mode_no = search( key, valid_cursor, 2 );
                  if (mode_no == ERROR) {
                     errmsg = config14;    /* cursor size error */
                     goto bad_config;
                  }
                  i |= mode_no << 2;
               } else {
                  if (i == SMALL_CURSOR)
                     i |= MEDIUM_CURSOR << 2;
                  else
                     /*
                      * Use the medium cursor to provide compatibility
                      * with previous versions that have set Large cursor.
                      */
                     i = MEDIUM_CURSOR | (SMALL_CURSOR << 2);
               }
               mode.cursor_size = i;
               g_display.insert_cursor = g_display.cursor[i & 3];
               g_display.overw_cursor  = g_display.cursor[i >> 2];
               need_mode_line = TRUE;
               break;

            case ControlZ:
               mode_no = search( key, valid_z, 1 );
               if (mode_no == ERROR) {
                  errmsg = config15;    /* control z error */
                  goto bad_config;
               }
               mode.control_z = mode_no;
               need_mode_line = TRUE;
               break;

            case TimeStamp :
               mode_no = OK;
               strcpy( mode.stamp, key );
               add_to_history( key, &h_stamp ); /* jmh 991020 */
               break;

            case InitialCaseMode :
               mode_no = search( key, case_modes, 1 );
               if (mode_no == ERROR) {
                  errmsg = config18;    /* initial case mode error */
                  goto bad_config;
               }
               mode.search_case = mode_no;
               need_mode_line = TRUE;
               break;

            case CaseMatch :
               mode_no = OK;
               for (i=0; i<256; i++)
                  sort_order.match[i] = (text_t)i;
               new_sort_order( (text_ptr)key, sort_order.match );
               break;

            case CaseIgnore :
               mode_no = OK;
               for (i=0; i<256; i++)
                  sort_order.ignore[i] = (text_t)i;
               for (i=65; i<91; i++)
                  sort_order.ignore[i] = (text_t)(i + 32);
               new_sort_order( (text_ptr)key, sort_order.ignore );
               break;

            case DirSort :
               mode_no = search( key, valid_dir_sort, 1 );
               if (mode_no == ERROR) {
                  errmsg = config27;    /* directory sort error */
                  goto bad_config;
               }
               mode.dir_sort = mode_no;
               break;

            case CaseConvert :
               mode_no = OK;
               new_upper_lower( (text_ptr)key );
               break;

            case CharDef :
               mode_no = OK;
               new_bj_ctype( key );
               break;

            case FrameStyle :
               mode_no = search( key, valid_frame, 3 );
               if (mode_no == ERROR) {
                  errmsg = config29;    /* invalid frame style */
                  goto bad_config;
               }
               g_display.frame_style = mode_no;
               break;

            case KeyName :
               mode_no = OK;
               new_key_name( residue, key );
               break;

            case Scancode :
               mode_no = OK;
               errmsg = new_scancode_map( residue, key );
               break;

            case UserMenu :
               mode_no = OK;
               if (user_idx == -1)
                  process_menu( NULL, "User" );
               errmsg = new_user_menu( residue, key, FALSE );
               break;

            case HelpFile :
               mode_no = OK;
               get_full_path( key, mode.helpfile );
               break;

            case HelpTopic :
               mode_no = OK;
               assert( strlen( key ) < MAX_COLS );
               strcpy( mode.helptopic, key );
               break;

            case Menu :
               mode_no = OK;
               errmsg = process_menu( residue, key );
               break;
         }
         if (mode_no == ERROR)
            errmsg = config19;          /* unknown mode */
      }
   }

   if (!found)
      errmsg = config20;

   if (errmsg != NULL) {
bad_config:
      ERRORLINE( errmsg, key );
   }
}


/*
 * Name:    parse_token
 * Purpose: given an input line, find the first token
 * Date:    June 5, 1994
 * Passed:  line:  line that contains the text to parse
 *          token: buffer to hold token
 * Returns: pointer in line to start next token search.
 * Notes:   assume tokens are delimited by spaces.
 *
 * jmh:     September 9, 1997 - if line is NULL, *token is set to '\0'.
 *          This allows easier parsing in my syntax highlighting.
 *
 * jmh 980730: test for comments. If the line starts with ';', then token is
 *              set empty and return NULL.
 *             Test for trailing spaces and comments when returning residue.
 *             To include a ';', enclose it in quotes (eg. c+k ";" Macro ...).
 */
char *parse_token( char *line, char *token )
{
   if (line == NULL) {
      *token = '\0';
      return( NULL );
   }

   /*
    * skip over any leading spaces.
    */
   while (*line == ' ' || *line == '\t')
      ++line;

   if (*line == ';') {
      *token = '\0';
      return( NULL );
   }

   /*
    * jmh 980521: test for literal strings.
    */
   if (*line == '\"') {
      line = parse_literal( line, token );
      if (line == NULL)
         return( NULL );
   } else {
      /*
       * put the characters into the token array until we run into a space
       *   or the terminating '\0';
       */
      while (*line != ' ' && *line != '\t' && *line != '\0')
         *token++ = *line++;
      *token = '\0';
   }

   /*
    * return what's left on the line, if anything.
    */
   while (*line == ' ' || *line == '\t')
      ++line;
   if (*line != ';' && *line != '\0')
      return( line );
   else
      return( NULL );
}


/*
 * Name:    parse_literal
 * Purpose: get all letters in a literal
 * Date:    June 5, 1994
 * Passed:  line:       current line position
 *          literal:    buffer to hold literal
 * Returns: pointer in line to start next token search.
 * Notes:   a literal begins with a '"'.  to include a '"', precede a '"'
 *           with a '"'.
 * jmh 980721: modified to return residue and display a warning.
 */
char *parse_literal( char *line, char *literal )
{
int  end_quote = 0;     /* flag to indicate the end of the literal */
char temp[10];          /* storage for the line number */
int  prompt_line;

   line++;
   /*
    * put the characters into the literal array until we run into the
    *   end of literal or terminating '\0';
    */
   while (*line != '\0') {
      if (*line == '\"') {
         line++;
         if (*line != '\"') {
            ++end_quote;
            break;
         }
      }
      *literal++ = *line++;
   }
   *literal = '\0';

   /*
    * return what's left on the line, if anything.
    */
   if (*line != '\0')
      return( line );
   else {
      if (!end_quote) {
         prompt_line = (g_status.current_window == NULL) ? g_display.end_line :
                       g_status.current_window->bottom_line;
         ERRORLINE( config21, temp );   /* unterminated quote */
      }
      return( NULL );
   }
}


/*
 * Name:    search
 * Purpose: binary search a CONFIG_DEFS structure
 * Date:    June 5, 1994
 * Passed:  token:  token to search for
 *          list:   list of valid tokens
 *          num:    number of last valid token in list
 * Returns: value of token assigned to matching token.
 * Notes:   do a standard binary search.
 *          instead of returning mid, lets return the value of the token
 *          assigned to mid.
 */
int  search( char *token, const CONFIG_DEFS list[], int num )
{
int  bot;
int  mid;
int  top;
int  rc;

   bot = 0;
   top = num;
   while (bot <= top) {
      mid = (bot + top) / 2;
      rc = stricmp( token, list[mid].key );
      if (rc == 0)
         return( list[mid].key_index );
      else if (rc < 0)
         top = mid - 1;
      else
         bot = mid + 1;
   }
   return( ERROR );
}


/*
 * Name:    clear_previous_macro
 * Purpose: clear any macro previously assigned to a key
 * Date:    June 5, 1994
 * Passed:  macro_key:  key that we are a assigning a macro to
 * Notes:   rewritten by Jason Hood, July 19, 1998.
 */
void clear_previous_macro( int macro_key )
{
MACRO **mac;

   mac = &macro[KEY_IDX( macro_key )];
   if (*mac != NULL) {
      if ((*mac)->len > 1)
         my_free( (*mac)->key.keys );
      my_free( *mac );
      *mac = NULL;
   }
}


/*
 * Name:    parse_macro
 * Purpose: separate literals from keys in a macro definition
 * Date:    June 5, 1994
 * Passed:  residue: pointer to macro defs
 * Notes:   for each token in macro def, find out if it's a literal or a
 *             function key.
 * jmh 980719: allow function names to be used and multi-line definitions.
 * jmh 980820: allow literals to contain escaped function shortcuts.
 * jmh 990429: continue processing multi-line macros when out of memory to
 *              avoid other error messages.
 * jmh 021024: modified error display to suit the startup macro.
 */
int  parse_macro( char *residue, int prompt_line )
{
int  rc;
char literal[1042];
char temp[42];
char *l;
long key_no = 0;
int  twokey;
int  func;
int  empty = TRUE;
TREE *twokey_func;
const char *errmsg;
int  exit_code = OK;

   /*
    * allocate storage for the keys.
    */
   if (!process_macro)
      if (initialize_macro( macro_key ) == NULL) {
         stroke_count = -1;
         exit_code = ERROR;
         if (line_no == 0)
            g_status.errmsg = main4;
      }

   while (residue != NULL) {
      /*
       * skip over any leading spaces.
       */
      while (*residue == ' ' || *residue == '\t')
         ++residue;

      /*
       * done if we hit eol or a comment
       */
      if (*residue == ';' || *residue == '\0')
         residue = NULL;

      /*
       * check for a literal.
       */
      else if (*residue == '\"') {
         empty   = FALSE;
         residue = parse_literal( residue, literal );
         l  = literal;
         rc = OK;
         while (*l != '\0'  &&  rc == OK) {
            func = 0;
            if (*l == '\\' && *(l+1) != '\0') {
               switch (*(++l)) {
                  case MAC_BackSpace : func = BackSpace;   break;
                  case MAC_CharLeft  : func = CharLeft;    break;
                  case MAC_CharRight : func = CharRight;   break;
                  case MAC_Pseudo    : func = PseudoMacro; break;
                  case MAC_Rturn     : func = Rturn;       break;
                  case MAC_Tab       : func = Tab;         break;
                  case '0'           : func = MacroMark;   break;
                  case '1'           : func = SetMark1;    break;
                  case '2'           : func = SetMark2;    break;
                  case '3'           : func = SetMark3;    break;
                  case '\\'          : ++l;                break;
               }
               if (func == 0)
                  --l;
            }
            rc = cfg_record_keys( mac, *l, func, prompt_line );
            ++l;
         }
      } else {
         /*
          * check for a function.
          */
         errmsg  = NULL;
         residue = parse_token( residue, literal );
         func    = search( literal, valid_func, CFG_FUNCS );
         if (func != ERROR) {
            if (func == RecordMacro && process_macro) {
               empty = process_macro = FALSE;
               break;
            }
            if (func == PlayBack) {
               key_no = macro_key;
               if (key_no >= 0x2121 && key_no < 0x10000L) {
                  cfg_record_keys( mac, (unsigned)key_no >> 8, 0, prompt_line );
                  cfg_record_keys( mac, (int)key_no & 0xff, 0, prompt_line );
                  func = PseudoMacro;
               }
               residue = NULL;
               empty = process_macro = FALSE;
            }
         } else {
            /*
             * check for a function key.
             */
            key_no = parse_key( literal );
            if (key_no == ERROR) {
               /*
                * check for mode definition.
                */
               key_no = search( literal, valid_macro_modes, NUM_MACRO_MODES );
               if ((int)key_no != ERROR && mac != NULL) {
                  if ((int)key_no & 0x8000)
                     mac->flag |= (int)key_no & ~0x8000;
                  else
                     mac->mode[(int)key_no / 2] = (int)key_no & 1;
                  continue;
               }
            } else {
               func = 0;
               if (key_func[KEY_IDX( key_no )] == TwoCharKey) {
                  if (residue == NULL) {
                     key_no = ERROR;
                     errmsg = config28;
                  } else {
                     residue = parse_token( residue, literal );
                     twokey = parse_key( literal );
                     if (twokey != ERROR) {
                        key_no = CREATE_TWOKEY( key_no, twokey );
                        twokey_func = search_tree(key_no, cfg_key_tree->right);
                        if (twokey_func != NULL)
                           func = twokey_func->func;
                     } else {
                        key_no = ERROR;
                        errmsg = config22;
                     }
                  }
               }
            }
         }
         if (key_no != ERROR) {
            cfg_record_keys( mac, key_no, func, prompt_line );
            empty = FALSE;
         } else {
            if (errmsg == NULL)
               errmsg = config31;
            if (line_no == 0)
               g_status.errmsg = errmsg;
            else {
               combine_strings( line_out, errmsg, config0,
                                my_ltoa( (line_no < 0) ? -line_no : line_no,
                                         temp, 10 ) );
               if (line_no > 0)
                  error( WARNING, prompt_line, line_out );
               else
                  g_status.errmsg = line_out;
            }
         }
      }
   }
   if (empty)
      process_macro = TRUE;

   if (!process_macro)
      check_macro( mac );

   return( exit_code );
}


/*
 * Name:    initialize_macro
 * Purpose: initialize the first key of a macro def
 * Date:    June 5, 1994
 * Passed:  key:  key number of macro that we are initializing
 * Returns: pointer to macro or NULL if no memory.
 * Notes:   Rewritten by Jason Hood, July 19/31, 1998.
 */
MACRO *initialize_macro( long key )
{
int  rc;

   mac            = NULL;
   branch         = NULL;
   existing_macro = FALSE;

   if (key >= 0x2121) {
      branch = search_tree( key, cfg_key_tree );
      if (branch == NULL) {
         branch = my_calloc( sizeof(TREE) );
         if (branch == NULL)
            return( NULL );
         branch->key = key;
      } else {
         existing_macro = TRUE;
         if (branch->macro != NULL) {
            mac = branch->macro;
            if (mac->len > 1)
               my_free( mac->key.keys );
            mac->len = 0;
         }
      }
      branch->func = PlayBack;  /* required for the two-keys */
   }
   if (mac == NULL) {
      mac = my_calloc( sizeof(MACRO) );
      if (mac == NULL) {
         if (!existing_macro)
            my_free( branch );
         return( NULL );
      }
      if (branch != NULL)
         branch->macro = mac;
   }

   /*
    * Try allocating STROKE_LIMIT first. If it fails, halve
    * it each time and try again.
    */
   stroke_count = STROKE_LIMIT;
   while ((mac->key.keys = my_malloc( stroke_count * sizeof(long), &rc ))
                         == NULL  &&  stroke_count > 1)
      stroke_count /= 2;

   if (mac->key.keys == NULL) {
      stroke_count = 0;
      my_free( mac );
      mac = NULL;
      if (!existing_macro)
         my_free( branch );
   } else                       /* set default modes to: */
      mac->mode[0] =            /* insert                */
      mac->mode[1] =            /* smart tabs            */
      mac->mode[2] = TRUE;      /* indent                */

   return( mac );
}


/*
 * Name:    check_macro
 * Purpose: see if macro def has any valid key.  if not, clear macro
 * Date:    June 5, 1994
 * Passed:  mac:  macro that we are checking
 * Notes:   Rewritten by Jason Hood, July 19, 1998.
 * jmh 990429: test for mac == NULL.
 */
void check_macro( MACRO *mac )
{
long key;
long *keys;
int  rc;

   if (mac == NULL)
      return;

   if (mac->len == 0) {
      my_free( mac->key.keys );
      my_free( mac );
      if (!existing_macro)
         my_free( branch );

   } else {
      if (mac->len == 1) {
         key = *mac->key.keys;
         my_free( mac->key.keys );
         mac->key.key = key;

      } else {
         keys = my_realloc( mac->key.keys, mac->len * sizeof(long), &rc );
         if (keys != NULL)
            mac->key.keys = keys;
      }

      if (branch == NULL)
         macro[KEY_IDX( macro_key )] = mac;
      else if (!existing_macro)
         add_branch( branch, cfg_key_tree );
   }
}


/*
 * Name:    cfg_record_keys
 * Purpose: save keystrokes in keystroke buffer
 * Date:    June 5, 1994
 * Passed:  line: line to display prompts
 * Notes:   Rewritten by Jason Hood, July 19, 1998.
 *
 * jmh 980826: when possible, store the function rather than the key.
 */
int  cfg_record_keys( MACRO *mac, long key, int func, int prompt_line )
{
int  rc;
char temp[42];

   rc = OK;
   if (stroke_count-- > 0) {
      if (func != RecordMacro    && func != SaveMacro  &&
          func != ClearAllMacros && func != LoadMacro) {
         mac->key.keys[mac->len++] = (func == 0 || func == PlayBack)
                                     ? key : (func | _FUNCTION);
      }
   } else {
      rc = ERROR;
      if (stroke_count == -1) {
         ERRORLINE( config23, temp );   /* no more room in macro buffer */
      }
   }
   return( rc );
}


/*
 * Name:    cfg_search_tree
 * Purpose: search the two-key tree for a function
 * Author:  Jason Hood
 * Date:    July 31, 1998
 * Passed:  func: function to find
 *          tree: pointer to tree
 * Returns: key if found; ERROR if not found
 * Notes:   recursively traverse the tree.
 * jmh 020819: removed tail recursion.
 */
long cfg_search_tree( int func, TREE *tree )
{
long key = ERROR;

   while (tree != NULL) {
      if (tree->func == func)
         return( tree->key );

      key = cfg_search_tree( func, tree->left );
      if (key == ERROR)
         tree = tree->right;
      else
         break;
   }
   return( key );
}


/*
 * Name:    new_sort_order
 * Purpose: change the sort order
 * Date:    October 31, 1992
 * Notes:   New sort order starts from ! (ie. skips space and control chars)
 */
void new_sort_order( text_ptr residue, text_ptr sort )
{
int  i;

   sort += 33;
   for (i = 33; *residue != '\0'  &&  i <= 255; i++)
      *sort++ = *residue++;
}


/*
 * Name:    add_twokey
 * Purpose: find an open slot and insert the new two-key combination
 * Date:    April 1, 1993
 * Passed:  key:  two-key
 *          func: function number to assign to this combo
 * Notes:   Rewritten by Jason Hood, July 30, 1998.
 */
int  add_twokey( long key, int func )
{
TREE *twokey;
int  rc = OK;

   twokey = search_tree( key, cfg_key_tree->right );
   if (twokey == NULL) {
      twokey = my_calloc( sizeof(TREE) );
      if (twokey == NULL)
         rc = ERROR;
      else {
         twokey->key = key;
         twokey->func = func;
         add_branch( twokey, cfg_key_tree );
      }
   } else {
      if (twokey->macro != NULL) {
         if (twokey->macro->len > 1)
            my_free( twokey->macro->key.keys );
         my_free( twokey->macro );
         twokey->macro = NULL;
      }
      twokey->func = func;
   }

   return( rc );
}


/*
 * Name:    new_upper_lower
 * Purpose: Define the conversion between upper and lower cases
 * Author:  Jason Hood
 * Date:    November 27, 1998
 * Notes:   Ignores space and the control characters.
 *          Dot ('.') is used as a place filler.
 */
void new_upper_lower( text_ptr residue )
{
int  i;

   for (i = 33; *residue != '\0'  &&  i <= 255; i++) {
      upper_lower[i] = (*residue == '.') ? 0 : *residue;
      ++residue;
   }
   for (; i < 256; ++i)
      upper_lower[i] = 0;
}


/*
 * Name:    new_bj_ctype
 * Purpose: Define the type of a character
 * Author:  Jason Hood
 * Date:    November 27, 1998
 * Notes:   Space and the control characters are always the same.
 *          Dot ('.') is used as a place filler.
 */
void new_bj_ctype( char *residue )
{
int  i;
int  t;

   for (i = 33; *residue != '\0'  &&  i <= 255; i++) {
      switch (*residue) {
         case 'L' : t = BJ_lower;             break;
         case 'U' : t = BJ_upper;             break;
         case 'D' : t = BJ_digit | BJ_xdigit; break;
         case 'S' : t = BJ_space;             break;
         case 'C' : t = BJ_cntrl;             break;
         case 'P' : t = BJ_punct;             break;
         case 's' : t = BJ_space | BJ_cntrl;  break;
         case 'X' : t = BJ_upper | BJ_xdigit; break;
         case 'x' : t = BJ_lower | BJ_xdigit; break;
         default  : t = 0;
      }
      bj_ctype[i] = t;
      ++residue;
   }
   for (; i < 256; ++i)
      bj_ctype[i] = 0;
}


/*
 * Name:    parse_color
 * Purpose: to recognize color numbers, or color names
 * Author:  Jason Hood
 * Date:    April 20, 1999
 * Passed:  line:        current line position
 *          key:         buffer to store tokens
 *          prompt_line: line to display error message
 *          not_color:   message to display if color is unrecognized
 * Returns: color number or ERROR if invalid
 *          line points to token after color
 *
 * jmh 020923: allow bright background.
 */
int  parse_color( char **line, char *key,
                  int prompt_line, const char *not_color )
{
int  fg,
     bg = background;
char *residue;

   *line = parse_token( *line, key );

   /*
    * we found a color field and attribute.  now, make sure
    *   everything is everything before we assign the attribute
    *   to the color field.
    */
   if (bj_isdigit( *key )) {
      fg = atoi( key );
      if (fg < 0 || fg > 255){
         ERRORLINE( config6, key );     /* color number out of range */
         return( ERROR );
      }
      return( fg );
   }

   fg = search( key, valid_color, 20 );
   if (fg == ERROR) {
      ERRORLINE( not_color, key );      /* unrecognized color name */
      return( ERROR );                  /* or unrecognized syntax setting */
   }

   if (fg >= 256)                       /* mono attribute */
      return( fg - 256 );

   if (*line != NULL) {
      /*
       * Check if there's a background.
       */
      residue = parse_token( *line, key );
      if (residue != NULL && stricmp( key, off_on[1].key ) == 0) {
         *line = parse_token( residue, key );
         bg = search( key, valid_color, 20 );
         if (bg == ERROR) {
            ERRORLINE( config6a, key ); /* unrecognized color name */
            return( ERROR );
         }
      }
   }
   return( fg | (bg << 4) );
}


/*
 * Name:    parse_key
 * Purpose: translate a config key string into a function key number
 * Author:  Jason Hood
 * Date:    August 19, 2002
 * Passed:  key:  the string to parse
 * Returns: the function key or ERROR if unable to parse
 * Notes:   assumes key contains at least one character.
 */
int  parse_key( char *key )
{
int  shift;
int  key_no;
int  c;

   shift = 0;
   for (; *++key == '+'; ++key) {
      c = bj_tolower( *(key-1) );
      if (c == 's')
         shift |= _SHIFT;
      else if (c == 'c')
         shift |= _CTRL;
      else if (c == 'a')
         shift |= _ALT;
      else
         return( ERROR );
   }

   key_no = search( key-1, valid_keys, AVAIL_KEYS-1 );
   if (key_no != ERROR)
      key_no |= shift;

   return( key_no );
}


/*
 * Name:    new_key_name
 * Purpose: set the key strings for the menu
 * Author:  Jason Hood
 * Date:    August 31, 2002
 * Passed:  line:  the line containing the strings
 *          buf:   a buffer to store a string
 * Notes:   reads the strings in scancode order.
 *          use an empty string to leave the current key unchanged.
 *          if the string starts with '=' and is followed by a number,
 *           take that number as the next scancode.
 *          simply ignores extra keys.
 *          currently ignores memory problems (if there's no memory
 *           for this, there's probably no memory for anything).
 */
static void new_key_name( char *line, char *buf )
{
char *tmp;
int  num;

   if (key_num >= MAX_KEYS)
      return;

   do {
      line = parse_token( line, buf );
      if (*buf  &&  strcmp( buf, key_word[key_num] ) != 0) {
         if (*buf == '=') {
            num = (int)strtol( buf+1, &tmp, 0 ) - 1;
            if (*tmp == '\0'  &&  num >= 0 && num < MAX_KEYS) {
               key_num = num;
               continue;
            }
         }
         tmp = my_strdup( buf );
         if (tmp != NULL)
            key_word[key_num] = tmp;
      }
      ++key_num;
   } while (line != NULL);
}


/*
 * Name:    new_scancode_map
 * Purpose: redefine the keyboard hardware sequence
 * Author:  Jason Hood
 * Date:    September 3, 2002
 * Passed:  line:  the line containing the strings
 *          buf:   a buffer to store a string
 * Returns: NULL or an error message
 * Notes:   translates pairs of keys - the first key is what TDE says it is
 *           (eg: via config/key.tdm); the second is the actual key.
 *          Eg: if you press Y on your keyboard, but TDE displays Z, you
 *              should add "Scancode Z Y" to your config file.
 *          Multiple pairs are allowed ("Scancode Z Y  Y Z").
 */
static const char *new_scancode_map( char *line, char *buf )
{
int  key;
int  newkey;

   do {
      line = parse_token( line, buf );
      if (line == NULL)
         return( config30 );    /* scancode requires pairs */
      key = parse_key( buf );
      line = parse_token( line, buf );
      newkey = parse_key( buf );
      if (key == ERROR || newkey == ERROR)
         return( config22 );
      scancode_map[KEY( key )] = KEY( newkey ) | 256; /* in case of modifiers */
   } while (line != NULL);

   return( NULL );
}


/*
 * Name:    parse_cmdline_macro
 * Purpose: process the startup macro from the command line
 * Author:  Jason Hood
 * Date:    October 24, 2002
 * Passed:  line:        the line or file to process
 *          prompt_line: line for error messages
 * Returns: nothing, but sets g_status.errmsg if an error occurs.
 * Notes:   if line is a file, read the macro from the file contents;
 *           otherwise treat line as a macro.  In the unlikely event
 *           that you want a single command that is also an existing file,
 *           simply add a space to the definition, eg: '-e " filefunction"'.
 *          Use the unavailable Control-Break key to store it.
 *
 * jmh 031026: use the local/global option value.
 */
void parse_cmdline_macro( char *line, int prompt_line )
{
FILE *fp;

   macro_key = g_option.macro;
   line_no = 0;

   if (file_exists( line ) == ERROR) {
      parse_macro( line, prompt_line );
      return;
   }

   if ((fp = fopen( line, "r" )) == NULL) {
      combine_strings( line_out, main7a, line, main7b );
      g_status.errmsg = line_out;
      return;
   }

   /*
    * Force the start of a multi-line macro definition.
    */
   parse_macro( "", prompt_line );

   while (g_status.errmsg == NULL  &&  read_line( fp ) != EOF) {
      /*
       * use negative line numbers to prevent parse_macro() from
       * displaying error messages.
       */
      --line_no;
      parse_macro( line_in, prompt_line );
   }
   fclose( fp );

   /*
    * Force the end of a multi-line macro definition.
    */
   parse_macro( (char *)func_str[RecordMacro], prompt_line );
}


/*
 * Name:    new_user_menu
 * Purpose: add an item to the User (Language) menu
 * Author:  Jason Hood
 * Date:    November 29, 2003
 * Passed:  line:   current line position
 *          token:  name of the menu, as well as buffer for key
 *          shl:    TRUE if creating Language sub-menu
 * Returns: NULL if okay, pointer to error message if not.
 * Notes:   requires user_menu to point to the appropriate menu.
 *          if the name already exists, overwrite the current.
 */
const char *new_user_menu( char *line, char *token, int shl )
{
int  rc = OK;
MINOR_STR *mnu;
char *name;
int  cnt;
long key;
int  twokey;
int  len;
int  i;
static int exists = FALSE;

   if (token == NULL || *token == '\0' || (*token == '-' && token[1] == '\0')) {
      if (exists == TRUE) {
         /*
          * The previous item already exists, so assume
          * this separator also already exists after it.
          */
         return( NULL );
      }
      i = len = 0;
   } else {
      len = strlen( token ) + 1;
      cnt = user_menu->minor_cnt - ((shl) ? 2 : 4);
      if (cnt == 1 && !shl &&
          strcmp( user_menu->minor[1].minor_name, config32 ) == 0) {
         i = 1;
         exists = ERROR;
      } else {
         for (i = cnt; i >= 1; --i) {
            name = user_menu->minor[i].minor_name;
            if (name != NULL)
               if (memcmp( name, token, len ) == 0)
                  break;
         }
         exists = (i > 0);
      }
   }

   if (!exists) {
      if (user_menu->minor_cnt == 0) {
         i   = TRUE;
         cnt = (shl) ? 3 : 5;
      } else {
         i   = FALSE;
         cnt = user_menu->minor_cnt + 1;
      }
      mnu = realloc( user_menu->minor, cnt * sizeof(MINOR_STR) );
      if (mnu == NULL)
         return( main4 );
      user_menu->minor = mnu;
      user_menu->minor_cnt = cnt;
      if (i) {
         user_menu->minor[0].line = NULL;
         for (i = 0; i < cnt; ++i) {
            user_menu->minor[i].minor_name = NULL;
            user_menu->minor[i].minor_func = ERROR;
            user_menu->minor[i].pop_out    = NULL;
            user_menu->minor[i].disabled   = FALSE;
         }
         if (!shl) {
            user_menu->minor[3].minor_name = (char *)config33; /* Language */
            user_menu->minor[3].minor_func = 0;
         }
         i = 1;
      } else {
         if (shl) {
            user_menu->minor[cnt-1] = user_menu->minor[cnt-2];
            i = cnt - 2;
         } else for (i = cnt - 1; i >= cnt - 3; --i)
            user_menu->minor[i] = user_menu->minor[i-1];
      }
   }
   if (i) {
      if (exists != TRUE) {
         user_menu->minor[i].minor_name = my_malloc( len, &rc );
         if (rc == ERROR)
            return( main4 );
         memcpy( user_menu->minor[i].minor_name, token, len );
      }
      if (line != NULL) {
         line = parse_token( line, token );
         if (*token != '\0') {
            key = parse_key( token );
            if (key == ERROR) {
               key = search( token, valid_func, CFG_FUNCS );
               if (key == ERROR) {
                  if (strlen( token ) != 2 || (text_t)token[0] <= ' ' ||
                                              (text_t)token[1] <= ' ')
                     return( config31 );
                  key = (unsigned)(((text_t)token[0] << 8) | (text_t)token[1]);
               } else
                  key |= _FUNCTION;

            } else if (key_func[KEY_IDX( key )] == TwoCharKey) {
               if (line == NULL)
                  return( config28 );
               parse_token( line, token );
               twokey = parse_key( token );
               if (twokey == ERROR)
                  return( config22 );
               key = CREATE_TWOKEY( key, twokey );
            }
            user_menu->minor[i].minor_func = key;
         }
      }
   }
   return( NULL );
}


/*
 * Name:    process_menu
 * Purpose: handle the Menu configuration option
 * Author:  Jason Hood
 * Date:    July 22, 2005
 * Passed:  line:  current line position
 *          token: what to do
 * Returns: NULL if okay, pointer to error message if not
 * Notes:   token is reused as a buffer.
 *          all names are case sensitive.
 */
static const char *process_menu( char *line, char *token )
{
int  opt;
char *hdr;
int  i;
static MENU_STR *cmenu = NULL;
static MENU_LIST *popout = NULL;
static int  erred = FALSE;

   opt = search( token, valid_menu, VALID_MENU_DEFS );
   /*
    * if it doesn't exist, treat it as the name of a header
    */
   if (opt == ERROR)
      opt = MENU_HEADER;
   else
      line = parse_token( line, token );

   switch (opt) {
      case MENU_CLEAR :
         cmenu = NULL;
         popout = NULL;
         if (*token == '\0') {
            for (i = 0; i < menu_cnt; ++i) {
               free_menu( &menu[i].menu );
               if (*menu[i].major_name != '\0')
                  my_free( menu[i].major_name );
            }
            menu_cnt = 0;
            user_idx = -1;
            user_menu = NULL;
            break;
         }
         /* fall through */

      case MENU_HEADER :
         if (*token == '\0')
            return( config1 );
         cmenu = NULL;
         popout = NULL;
         erred = FALSE;
         for (i = 0; i < menu_cnt; ++i) {
            hdr = menu[i].major_name;
            if (*hdr == '\0')
               ++hdr;
            if (strcmp( hdr, token ) == 0) {
               cmenu = &menu[i].menu;
               if (opt == MENU_CLEAR) {
                  free_menu( cmenu );
                  if (i == user_idx) {
                     user_idx = -1;
                     user_menu = NULL;
                  }
               }
               break;
            }
         }
         if (cmenu == NULL) {
            /*
             * find an empty menu, assuming it was just cleared to be renamed
             */
            for (i = 0; i < menu_cnt; ++i) {
               if (menu[i].menu.minor_cnt == 0  &&  i != user_idx)
                  break;
            }
            if (i == MAJOR_MAX)
               return( config34 );      /* too many menus */
            menu[i].major_name = my_strdup( token );
            if (menu[i].major_name == NULL)
               return( main4 );
            cmenu = &menu[i].menu;
            if (token == "User" ||
                (parse_token( line, token ), stricmp( token, "User" )) == 0) {
               user_idx = i;
               user_menu = &menu[user_idx].menu;
            }
            if (i == menu_cnt)
               ++menu_cnt;
         }
         break;

      case MENU_ITEM :
         if (popout)
            popout = NULL;
         if (cmenu)
            return( new_menu_item( cmenu, line, token ) );
         /*
          * menu item not in a menu
          */
         if (!erred) {
            erred = TRUE;
            return( config35 );
         }
         break;

      case MENU_POPOUT :
         if (cmenu) {
            const char *errmsg = new_menu_item( cmenu, NULL, token );
            if (errmsg != NULL)
               return( errmsg );
            popout = my_calloc( sizeof(MENU_LIST) );
            if (popout == NULL)
               return( main4 );
            popout->next = popout_menu;
            popout_menu = popout;
            cmenu->minor[cmenu->minor_cnt-2].pop_out = &popout->popout;
            cmenu->minor[cmenu->minor_cnt-2].minor_func = 0;
            break;
         }
         if (!erred) {
            erred = TRUE;
            return( config35 );
         }
         break;

      case MENU_POPITEM :
         if (popout)
            return( new_menu_item( &popout->popout, line, token ) );
         if (!erred) {
            erred = TRUE;
            return( config35 );
         }
         break;
   }

   return( NULL );
}


/*
 * Name:    new_menu_item
 * Purpose: add an item to a menu
 * Author:  Jason Hood
 * Date:    July 23, 2005
 * Passed:  mnu:    menu being updated
 *          line:   line containing item details
 *          token:  name of the item, as well as buffer for key/function
 * Returns: NULL if okay, pointer to error message if not.
 * Notes:   if the key/function already exists, replace the name.
 */
static const char *new_menu_item( MENU_STR *mnu, char *line, char *token )
{
char *name;
int  cnt;
long key;
int  twokey;
int  i;
MINOR_STR *minor;

   /*
    * "Menu Item" and "Menu Item -" both generate a separator, but
    * "Menu Item - func" will treat "-" as an item name, not separator
    */
   if (*token == '-' && token[1] == '\0' && line == NULL)
      *token = '\0';

   if (*token) {
      name = my_strdup( token );
      if (name == NULL)
         return( main4 );
   } else
      name = NULL;

   key = ERROR;
   line = parse_token( line, token );
   if (*token) {
      key = parse_key( token );
      if (key == ERROR) {
         key = search( token, valid_func, CFG_FUNCS );
         if (key == ERROR) {
            if (strlen( token ) != 2 || (text_t)token[0] <= ' ' ||
                                        (text_t)token[1] <= ' ') {
               my_free( name );
               return( config31 );
            }
            key = (unsigned)(((text_t)token[0] << 8) | (text_t)token[1]);
         } else
            key |= _FUNCTION;

      } else if (key_func[KEY_IDX( key )] == TwoCharKey) {
         if (line == NULL) {
            my_free( name );
            return( config28 );
         }
         parse_token( line, token );
         twokey = parse_key( token );
         if (twokey == ERROR) {
            my_free( name );
            return( config22 );
         }
         key = CREATE_TWOKEY( key, twokey );
      }
   }

   if (key != ERROR) {
      for (i = mnu->minor_cnt - 1; i >= 1; --i) {
         if (mnu->minor[i].minor_func == key) {
            if (*mnu->minor[i].minor_name != '\0')
               my_free( mnu->minor[i].minor_name );
            mnu->minor[i].minor_name = name;
            return( NULL );
         }
      }
   }

   if (mnu->minor_cnt == 0) {
      i   = TRUE;
      cnt = 3;
      minor = malloc( cnt * sizeof(MINOR_STR) );
   } else {
      i   = FALSE;
      cnt = mnu->minor_cnt + 1;
      if (mnu->minor[0].minor_func == ERROR)
         minor = realloc( mnu->minor, cnt * sizeof(MINOR_STR) );
      else {
         minor = malloc( cnt * sizeof(MINOR_STR) );
         if (minor) {
            memcpy( minor, mnu->minor, mnu->minor_cnt * sizeof(MINOR_STR) );
            minor[0].minor_func = ERROR;
         }
      }
   }
   if (minor == NULL) {
      my_free( name );
      return( main4 );
   }

   mnu->minor = minor;
   mnu->minor_cnt = cnt;
   if (i) {
      mnu->minor[0].line = NULL;
      for (i = cnt - 1; i >= 0; --i) {
         mnu->minor[i].minor_name = NULL;
         mnu->minor[i].minor_func = ERROR;
         mnu->minor[i].pop_out    = NULL;
         mnu->minor[i].disabled   = FALSE;
      }
      i = 1;
   } else {
      i = cnt - 2;
      mnu->minor[i+1] = mnu->minor[i];
   }

   mnu->minor[i].minor_name = name;
   mnu->minor[i].minor_func = key;

   return( NULL );
}


/*
 * Name:    free_menu
 * Purpose: release memory used by a menu
 * Author:  Jason Hood
 * Date:    July 22, 2005
 * Passed:  menu:  the menu to free
 */
static void free_menu( MENU_STR *menu )
{
int  i;
MENU_LIST *ml, *po;

   if (menu->minor == NULL)
      return;

   if (menu->minor[0].line)
      free( menu->minor[0].line );
   for (i = menu->minor_cnt - 1; i >= 1; --i) {
      if (menu->minor[i].minor_name && *menu->minor[i].minor_name != '\0')
         my_free( menu->minor[i].minor_name );
      if (menu->minor[i].pop_out) {
         po = NULL;
         for (ml = popout_menu; ml != NULL; ml = ml->next) {
            if (&ml->popout == menu->minor[i].pop_out) {
               if (po == NULL)
                  popout_menu = ml->next;
               else
                  po->next = ml->next;
               break;
            }
            po = ml;
         }
         free_menu( menu->minor[i].pop_out );
         if (ml < popout_default || ml > popout_last)
            my_free( ml );
      }
   }
   if (menu->minor_cnt) {
      if (menu->minor[0].minor_func != ERROR-1)
         free( menu->minor );
      menu->minor_cnt = 0;
   }
}


/*
 * Name:    new_help_screen
 * Purpose: read in a new help screen
 * Author:  Jason Hood
 * Date:    July 10, 2005
 * Passed:  file:  file containing screen
 *          type:  0 for normal help, 1 for viewer
 * Returns: TRUE if viewer help follows
 */
static int  new_help_screen( FILE *file, int type )
{
int  viewer = FALSE;
int  wid;
int  c;
int  i;

   wid = 0;
   for (c = 0; c < HELP_HEIGHT; c++) {
      if ((i = read_line( file )) == EOF) {
         viewer = ERROR;
         break;
      }
      if (strcmp( line_in, "<<<>>>" ) == 0) {
         viewer = TRUE;
         break;
      }
      if (i > HELP_WIDTH)
         i = HELP_WIDTH;
      memcpy( help_screen[type][c], line_in, i );
      memset( help_screen[type][c]+i, ' ', HELP_WIDTH-i );
      if (i > wid)
         wid = i;
   }
   if (c != 0) {
      help_dim[type][0] = c;
      help_dim[type][1] = wid;
      while (--c >= 0)
         help_screen[type][c][wid] = '\0';
   }

   if (viewer != ERROR) {
      if (!viewer)
         /*
          * the help screen had too many lines, skip the extra
          */
         while (read_line( file ) != EOF)
            if (strcmp( line_in, "<<<>>>" ) == 0) {
               viewer = TRUE;
               break;
            }
   }

   return( viewer == TRUE );
}
