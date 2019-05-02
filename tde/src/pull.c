/*
 * These functions do the pop-up pull-down command menus.
 *
 * Being stupid, I can't remember what half the function keys do.
 *  Fortunately, it is very easy to write a pop-up pull-down command menu
 *  to run editor commands.  Being lazy, I didn't implement CUA style menus,
 *  because we can provide most of the CUA benefits with a single hot-key.
 *  Also, dumb UNIX terminals don't have a lot function keys to spare.
 *
 * jmh 010624: I'd like every function to be on the menu, but I'm running out
 *              of room. Let's do a pop-out menu, as well.
 * jmh 031027: give each section its own color.
 * jmh 031129: recognise unavailable functions;
               due to the user menu, use keys instead of functions.
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
 *    You may distribute it freely.
 */

#include "tdestr.h"
#include "common.h"
#include "define.h"
#include "tdefunc.h"


extern int  make_window;        /* defined in window.c */
extern int  search_type;        /* defined in findrep.c */
#if defined( __UNIX__ )
extern int  xterm;              /* unix/console.c */
#endif

static int  saved_major = 0;

static int  major_col[MAJOR_MAX];   /* jmh - made these global so it's only */
static int  major_width[MAJOR_MAX]; /*  necessary to calculate them once */

static Char *buffer[2];         /* jmh 991022: allocate the buffer once */
static int  buf_len[2];

static void check_menu( MENU_STR * );
       void make_menu( MENU_STR *, long *, int );
static char *create_menu_key( long, long * );
static void menu_key_name( int, char * );

#if defined( __DJGPP__ )
extern int  test_clipboard( void );     /* djgpp/port.c */
#endif


/*
 * Name:    main_pull_down
 * Purpose: show pull down menu and call function if needed
 * Date:    November 13, 1993
 * Passed:  window:  current window
 * Notes:   keep a record of the last menu choice in local global variables.
 *
 * jmh 980826: record the function selected.
 */
int  main_pull_down( TDE_WIN *window )
{
long key;
int  rc;
DISPLAY_BUFF;
char trigger[4];

   SAVE_LINE( 0 );
   key = lite_bar_menu( &saved_major );
   RESTORE_LINE( 0 );
   if (key != ERROR) {
      if (key >= 0x2121 && key < 0x10000L) {
         g_status.key_pressed = 0;
         g_status.command = PseudoMacro;
         trigger[0] = (unsigned)key >> 8;
         trigger[1] = (int)key & 0xff;
         trigger[2] = '\0';
         add_chars( (text_ptr)trigger, window );
      } else {
         g_status.key_pressed = key;
         g_status.command = getfunc( key );
      }
      g_status.control_break = FALSE;
      record_key( g_status.key_pressed, g_status.command );
      rc = execute( window );
   } else
      rc = ERROR;
   return( rc );
}


/*
 * Name:    lite_bar_menu
 * Purpose: handle major menu choices
 * Date:    November 13, 1993
 * Passed:  maj: pointer to menu bar choice
 * Returns: function if selected, ERROR if aborted.
 * Notes:   set the menu structures
 *
 * jmh 990412: reflect changes in pull_me().
 * jmh 991128: don't pass the minor choice.
 * jmh 010625: due to the recursive nature of pull_me(), set output_space here.
 */
long lite_bar_menu( int *maj )
{
long min;
int  row;
int  ch;
int  spc, spw;
MENU_LIST *ml;

   g_display.output_space = g_display.frame_space;
   /*
    * put the pull-down menu on the first line of screen.
    */
   eol_clear( 0, 0, Color( Menu_header ) );
   draw_lite_head( 0, major_col );
   spc = g_display.frame_space;
   spw = spc * 2;

   for (row = 0; row < menu_cnt; row++)
      menu[row].menu.checked = FALSE;
   for (ml = popout_menu; ml != NULL; ml = ml->next)
      ml->popout.checked = FALSE;

   row = 0;
   min = 0;
   xygoto( -1, -1 );
   ch = -1;
   while (ch != AbortCommand  &&  ch != Rturn) {
      if (ch == CharRight || ch == CharLeft || ch == PullDown ||
          ch == StreamCharLeft || ch == StreamCharRight) {
         hlight_line( major_col[*maj] - spc, row, major_width[*maj] + spw,
                      Color( Menu_header ) );
         if (ch == CharRight || ch == StreamCharRight) {
            if (++*maj == menu_cnt)
               *maj = 0;
         } else if (ch == CharLeft || ch == StreamCharLeft) {
            if (--*maj < 0)
               *maj = menu_cnt - 1;
         } else
            *maj = (int)min;
         ch = -1;
      }
      if (ch == -1)
         hlight_line( major_col[*maj] - spc, row, major_width[*maj] + spw,
                      Color( Menu_sel ) );

      min = pull_me( &menu[*maj].menu, row+1, major_col[*maj], &ch );
   }
   g_display.output_space = FALSE;
   return( min );
}


/*
 * Name:    pull_me
 * Purpose: move cursor up and down the menu bar
 * Date:    November 13, 1993
 * Passed:  menu: menu to navigate
 *          row: row to begin vertical choice
 *          col: column to begin vertical choice
 *          ch:  pointer to current function (key)
 * Returns: function selected, or ERROR if it couldn't save the screen;
 *          ch is set to the function of the last key pressed.
 *          if ch is PullDown then returns new major menu.
 * Notes:   save the text under the pulled down menu.
 *          if the menu is too wide for the screen, bring it back to the
 *           screen edge.
 *          if the menu is too long for the screen, try placing it above the
 *           selection. If that won't fit, stop it at the screen edge. If it's
 *           still too big, place it at the top (but there's no scroll, yet).
 *
 * jmh 990412: use letters (actually windowletters) to select an item directly;
 *             use numbers to select a major menu directly.
 * jmh 990419: corrected a bug with the number selection (remembered wrong pos).
 * jmh 991021: add a space before and after the menu.
 * jmh 991128: don't pass in the minor choice.
 * jmh 010626: modify menu position here.
 */
long pull_me( MENU_STR *menu, int row, int col, int *ch )
{
long key;
int  i;
int  minor_width;
int  cnt;
int  select, old;
int  begin;
int  end;
int  sel, not_sel, unavail, sel_unavail;
long func = ERROR;
static int level = -1;

   ++level;
   if (buffer[level] == NULL)
      buffer[level] = (Char *)malloc( buf_len[level] * sizeof(Char) );

   if (buffer[level] != NULL) {
      cnt = menu->minor_cnt;
      minor_width = menu->width;
      begin   = menu->first;
      select  = menu->current;
      end     = menu->last;
      not_sel = Color( Menu );
      unavail = Color( Menu_dis );
      sel     = Color( Menu_item );
      sel_unavail = Color( Menu_nitem );

      if (!menu->checked) {
         check_menu( menu );
         menu->checked = TRUE;
      }

      if (col + minor_width + 1 > g_display.ncols - 1) {
         col = g_display.ncols - 1 - minor_width;
         if (col < 1)
            col = 1;
      }
      if (row + cnt > g_display.nlines) {
         row -= cnt + 1;
         if (row < 0) {
            row = g_display.nlines - cnt;
            if (row < 0)
               row = 0;
         }
      }
      save_area( buffer[level], minor_width, cnt, row, col );
      for (i = 0; i < cnt; i++) {
         s_output( menu->minor[i].line, row+i, col,
                   (menu->minor[i].disabled) ? unavail : not_sel );
         if (menu->minor[i].disabled) {
            hlight_line( col, row+i, 1, not_sel );
            hlight_line( col+minor_width-1, row+i, 1, not_sel );
         }
      }
      shadow_area( minor_width, cnt, row, col );

      if (select)
         hlight_line( col+1, row+select, minor_width-2,
                      (menu->minor[select].disabled) ? sel_unavail : sel );
   parent_menu:
      key = 256;
      while (*ch != AbortCommand && *ch != Rturn  &&
             ((*ch != CharRight && *ch != StreamCharRight &&
               *ch != CharLeft  && *ch != StreamCharLeft) || make_window)) {
         old = select;
         if (*ch == LineDown) {
            if (select == end)
               select = begin;
            else
               while (menu->minor[++select].minor_func < 0) ;
         } else if (*ch == LineUp) {
            if (select == begin)
               select = end;
            else
               while (menu->minor[--select].minor_func < 0) ;
         } else if (key < 256 && bj_isalnum( key )) {
            if (bj_isalpha( key )) {
               for (i = begin; i <= end; ++i)
                  if (menu->minor[i].line[2] == key)
                     break;
               if (i <= end && !menu->minor[i].disabled) {
                  hlight_line( col+1, row+select, minor_width-2,
                               (menu->minor[select].disabled)?unavail:not_sel );
                  select = i;
                  *ch = Rturn;
                  hlight_line( col+1, row+select, minor_width-2, sel );
                  break;
               }
            } else {
               i = (key == '0') ? 9 : (int)key - '1';
               if (i < menu_cnt) {
                  func = i;
                  *ch = PullDown;
                  break;
               }
            }
         }
         if (old != select) {
            hlight_line( col+1, row+old, minor_width-2,
                         (menu->minor[old].disabled) ? unavail : not_sel );
            hlight_line( col+1, row+select, minor_width-2,
                         (menu->minor[select].disabled) ? sel_unavail : sel );
         }

         key = getkey( );
         *ch = (key == RTURN) ? Rturn        :
               (key == ESC)   ? AbortCommand :
               getfunc( key );

         if (*ch == Rturn && menu->minor[select].disabled)
            *ch = -1;
      }
      menu->current = select;
      if (*ch == AbortCommand)
         func = ERROR;
      else if (*ch == Rturn  &&  menu->minor[select].pop_out != NULL) {
         *ch = -1;
         func = pull_me( menu->minor[select].pop_out,
                           row+select+1, col+5, ch );
         if (*ch == AbortCommand) {
            *ch = -1;
            goto parent_menu;
         }
      } else if (*ch != PullDown)
         func = menu->minor[select].minor_func;
      restore_area( buffer[level], minor_width, cnt, row, col );
   } else {
      func = ERROR;
      *ch = AbortCommand;
   }
   --level;
   return( func );
}


/*
 * Name:    check_menu
 * Purpose: determine the validity of menu items
 * Author:  Jason Hood
 * Date:    November 29, 2003
 * Passed:  menu:  the menu to check
 * Notes:   turns the disabled flag on or off, depending on the function and
 *           the state of the editor.
 */
static void check_menu( MENU_STR *mnu )
{
int  i;
long key;
int  func;
int  flag;
int  dis;

   for (i = mnu->minor_cnt - 2; i >= 1; --i) {

      key = mnu->minor[i].minor_func;

      /*
       * Ignore separators and popout menus.
       */
      if (key < 1)
         continue;

      func = (key >= 0x2121 && key < 0x10000L) ? PseudoMacro :
             getfunc( mnu->minor[i].minor_func );
      flag = func_flag[func];
      dis  = FALSE;

      if ((flag & F_MODIFY) && g_status.current_file->read_only)
         dis = TRUE;

      else if (flag & F_BLOCK) {
         if (!g_status.marked  ||
             (g_status.marked_file->read_only && !(flag & F_BSAFE)))
            dis = TRUE;
         /*
         else if ((func == BlockBegin || func == BlockEnd) &&
                  g_status.marked_file != g_status.current_file)
            dis = TRUE;
         */
         else if (func == BlockBlockComment || func == BlockLineComment ||
                  func == BlockUnComment) {
            if (g_status.marked_file->syntax == NULL)
               dis = TRUE;
            else {
               syntax_info *syn = g_status.marked_file->syntax->info;
               if (func == BlockBlockComment) {
                  if (!syn->comstart[0][0])
                     dis = TRUE;
               } else if (func == BlockLineComment) {
                  if (!syn->comment[0][0] ||
                      g_status.marked_file->block_type == STREAM)
                     dis = TRUE;
               } else { /* func == BlockUnComment */
                  if (!syn->comstart[0][0] && !syn->comment[0][0])
                     dis = TRUE;
               }
            }
#if defined( __DJGPP__ )
         } else if ((func == CopyToClipboard || func == KopyToClipboard ||
                     func == CutToClipboard) && !test_clipboard( )) {
            dis = TRUE;
#endif
         } else {
            int type = g_status.marked_file->block_type;
            if (type == BOX) {
               if (!(flag & F_BOX))
                  dis = TRUE;
            } else if (type == LINE) {
               if (!(flag & F_LINE))
                  dis = TRUE;
            } else /* type == STREAM */
               if (!(flag & F_STREAM))
                  dis = TRUE;
         }

      } else switch (func) {

         case DefineDiff:
         case NextWindow:
         case PreviousWindow:
         case NextHiddenWindow:
         case PrevHiddenWindow:
         {
            TDE_WIN *wp;
            int  win = 0;
            int  state = !(func == NextHiddenWindow||func == PrevHiddenWindow);
            for (wp = g_status.window_list; wp != NULL; wp = wp->next) {
               if (wp->visible == state)
                  if (++win == state + 1)
                     break;
            }
            if (win != state + 1)
               dis = TRUE;
            break;
         }

         case EditNextFile:
            if (g_status.arg >= g_status.argc)
               dis = TRUE;
            break;

         case FileAll:
            if (g_status.output_redir)
               dis = TRUE;
            break;

         case FileAttributes:
         case Revert:
            if (g_status.current_file->file_name[0] == '\0')
               dis = TRUE;
            break;

         /*
         case GotoMark1:
         case GotoMark2:
         case GotoMark3:
            if (!g_status.current_file->marker[func - GotoMark1 + 1].marked)
               dis = TRUE;
            break;
         */

         case EndOfFile:
         case HalfScreenDown:
         /* case ScreenDown: */
            if (g_status.current_window->rline
                   >= g_status.current_file->length + 1
                      - (g_status.current_window->bottom_line
                         - g_status.current_window->cline))
               dis = TRUE;
            break;

         case HalfScreenLeft:
         /* case PanLeft: */
         case ScreenLeft:
            if (g_status.current_window->bcol == 0)
               dis = TRUE;
            break;

         case HalfScreenUp:
         /* case PanUp:    */
         /* case ScreenUp: */
         case TopOfFile:
            if (g_status.current_window->rline
                   == g_status.current_window->cline
                      - g_status.current_window->top_line)
               dis = TRUE;
            break;

         /*
         case MacroMark:
         case Pause:
            if (!mode.record)
               dis = TRUE;
            break;
         */

         case MakeHalfHorizontal:
         case MakeHorizontal:
         case SplitHalfHorizontal:
         case SplitHorizontal:
         {
            int top = (func == MakeHalfHorizontal||func == SplitHalfHorizontal)
                      ? ((g_status.current_window->top_line
                          + g_status.current_window->bottom_line + 1) / 2)
                      : g_status.current_window->cline;

            if (g_status.current_window->top_line == top  ||
                g_status.current_window->bottom_line == top)
               dis = TRUE;
            break;
         }

         case MakeHalfVertical:
         case MakeVertical:
         case SplitHalfVertical:
         case SplitVertical:
         {
            int left = (func == MakeHalfVertical || func == SplitHalfVertical)
                       ? ((g_status.current_window->left
                           + g_status.current_window->end_col + 1) / 2)
                       : g_status.current_window->ccol;

            if (g_status.current_window->left + 15 > left  ||
                g_status.current_window->end_col - 15 < left)
               dis = TRUE;
            break;
         }

#if defined( __DJGPP__ )
         case PasteFromClipboard:
            if (!test_clipboard( ))
               dis = TRUE;
            break;
#endif

         /*
         case PreviousPosition:
            if (!g_status.current_file->marker[0].marked)
               dis = TRUE;
            break;
         */

         /*
         case Redo:
            if (g_status.current_file->undo_count == 0)
               dis = TRUE;
            break;
         */

         case RepeatDiff:
            if (!diff.defined)
               dis = TRUE;
            break;

         case RepeatFindBackward:
         case RepeatFindForward:
            if (bm.search_defined != OK)
               dis = TRUE;
            break;

         case RepeatGrep:
            if (!g_status.sas_defined || g_status.sas_arg >= g_status.sas_argc)
               dis = TRUE;
            break;

         case RepeatRegXBackward:
         case RepeatRegXForward:
            if (regx.search_defined != OK)
               dis = TRUE;
            break;

         case RepeatSearch:
            if (search_type == ERROR)
               dis = TRUE;
            break;

         case RestoreLine:
            if (g_status.buff_node != g_status.current_window->ll)
               dis = TRUE;
            break;

         case RetrieveLine:
            if (g_status.current_file->undo_lines == 0 ||
                g_status.current_window->rline == 0)
               dis = TRUE;
            break;

         case SizeWindow:
            if (!g_status.current_window->vertical &&
                g_status.current_window->bottom_line == g_display.end_line &&
                g_status.current_window->top == mode.display_cwd)
               dis = TRUE;
            break;

         case ToggleSyntax:
            if (g_status.current_file->syntax == NULL)
               dis = TRUE;
            break;

         /*
         case Undo:
            if (g_status.current_file->undo_count == 0)
               dis = TRUE;
            break;
         */

         /*
         case UnMarkBlock:
            if (g_status.current_file->block_type == 0)
               dis = TRUE;
            break;
         */

#if defined( __UNIX__ )
         case UserScreen:
            if (xterm)
               dis = TRUE;
            break;
#endif
      }

      mnu->minor[i].disabled = dis;
   }

   /*
    * See if the User menu has a Language sub-menu.
    */
   if (mnu == &menu[user_idx].menu) {
      i = mnu->minor_cnt - 2;
      if (g_status.current_file->syntax &&
          g_status.current_file->syntax->menu.minor_cnt != 0) {
         mnu->minor[i].pop_out = &g_status.current_file->syntax->menu;
         mnu->minor[i].disabled = FALSE;
         g_status.current_file->syntax->menu.checked = FALSE;
      } else
         mnu->minor[i].disabled = TRUE;
   }
}


/*
 * Name:    get_bar_spacing
 * Purpose: calculate headings for main menu choices
 * Date:    November 13, 1993
 * Passed:  major_col: column to display each menu heading
 *          major_width: width of each menu heading
 * Notes:   assume 6 spaces between the menu items
 * jmh 991025: since I added another menu, use 5 spaces.
 * jmh 031129: with the User menu, drop it down to 4.
 * jmh 050710: with the Help menu, drop it down to 3.
 * jmh 050722: with customisation, calculate it.
 */
void get_bar_spacing( int major_col[], int major_width[] )
{
int  i;
int  spc, col;
char *hdr;

   col = 0;
   for (i = 0; i < menu_cnt; i++) {
      hdr = menu[i].major_name;
      if (*hdr == '\0')
         ++hdr;
      major_width[i] = strlen( hdr );
      col += major_width[i];
   }
   if (menu_cnt == 1)
      spc = 0;
   else {
      spc = (g_display.ncols - 2 - col) / (menu_cnt - 1);
      if (spc <= 0)
         spc = 1;
      else if (spc > 6)
         spc = 6;
   }
   col = (g_display.ncols - 2 - (col + spc * (menu_cnt - 1))) / 2;
   if (col < 0)
      col = 0;
   else if (col > 6)
      col = 6;
   for (i = 0; i < menu_cnt; i++) {
      major_col[i] = col;
      col += spc + major_width[i];
   }
}


/*
 * Name:    draw_lite_head
 * Purpose: slap the main menu choices on the lite bar
 * Date:    November 13, 1993
 * Passed:  row: lite bar row
 *          major_col: column to display each menu heading
 *
 * jmh 991021: add a space before and after the heading.
 */
void draw_lite_head( int row, int major_col[] )
{
int  i;
char *hdr;

   for (i = 0; i < menu_cnt; i++) {
      hdr = menu[i].major_name;
      if (*hdr == '\0')
         ++hdr;
      s_output( hdr, row, major_col[i], Color( Menu_header ) );
   }
   refresh( );
}


/*
 * Name:    get_minor_counts
 * Purpose: determine the first and last valid selections
 * Date:    November 13, 1993
 * Passed:  menu:  pointer to menu
 * jmh 980809: this was used to count the number of selections in the menu,
 *              but that's now done with the sizeof operator.
 *             By the way, it assumes there's something in the menu.
 * jmh 991127: corrected last selection error (forgot to minus one).
 * jmh 010624: set current value, add menu parameter.
 * jmh 031204: the User menu could only contain separators.
 */
void get_minor_counts( MENU_STR* menu )
{
int  cnt;
int  pos;

   /*
    * find first valid minor selection.
    */
   cnt = menu->minor_cnt;
   for (pos = 1; pos < cnt && menu->minor[pos].minor_func < 0; pos++) ;
   if (pos < cnt) {
      menu->first = menu->current = pos;

      /*
       * find last valid minor selection
       */
      for (pos = cnt - 2; menu->minor[pos].minor_func < 0; pos--) ;
      menu->last = pos;

   } else
      menu->current = 0;
}


/*
 * Name:    init_menu
 * Purpose: initialise some menu "constants" and find key definitions
 * Author:  Jason Hood
 * Date:    30 December, 1996
 * Notes:   If a function is defined twice, the first found will be used
 *          Makes a duplicate of the menu, inserting the key definition
 *
 * 980524:  Create the menu frame from scratch.
 * 980803:  Made keys an array, rather than dynamic allocation.
 * 980819:  Create an array of keys based on function.
 *          Recognize two-keys.
 * 990412:  Place letters in menu, using windowletters sequence.
 * 990429:  Prevent viewer functions being placed in the menu.
 * 991019:  Custom frame style.
 * 991022:  Find the largest area to preserve screen.
 * 010624:  Moved actual menu code to make_menu().
 * 031129:  Disable the djgpp clipboard popout if the clipboard's not present.
 * 031130:  With the new line member, update the menu with the new key settings.
 *           The init parameter is TRUE for the first call, FALSE otherwise.
 * 031203:  Get the counts here to prevent the config file resetting current.
 */
void init_menu( int init )
{
long func_key[NUM_FUNCS];
int  i, j, k;
int  key;
MENU_LIST *ml;

   get_bar_spacing( major_col, major_width );

   memset( func_key, 0, sizeof(func_key) );
   for (k = 0; k < MODIFIERS; ++k) {
      key = 256 | (k << 9);
      for (i = 0; i < MAX_KEYS; ++i) {
         j = key_func[k][i];
         if (j != 0 && func_key[j] == 0) {
            if (!viewer( i | key ))
               func_key[j] = i | key;
         }
      }
   }
   for (i = 1; i < NUM_FUNCS; ++i) {
      if (func_key[i] == 0) {
         func_key[i] = cfg_search_tree( i, key_tree.right );
         if (func_key[i] == ERROR || viewer( PARENT_KEY( func_key[i] )))
            func_key[i] = 0;
      }
   }

   for (i = 0; i < menu_cnt; i++) {
      get_minor_counts( &menu[i].menu );
      make_menu( &menu[i].menu, func_key, 0 );
   }
   for (ml = popout_menu; ml != NULL; ml = ml->next) {
      get_minor_counts( &ml->popout );
      make_menu( &ml->popout, func_key, 1 );
   }
   if (init) {
      get_minor_counts( &make_window_menu );
      make_menu( &make_window_menu, NULL, 0 );
   }

   if (saved_major >= menu_cnt)
      saved_major = 0;
}


/*
 * Name:    make_menu
 * Purpose: create the menu outline and add the function key
 * Author:  Jason Hood
 * Date:    June 24, 2001
 * Passed:  menu:      menu being made
 *          func_key:  keys assigned to functions
 *          level:     0 for top-level menu, 1 for popout
 */
void make_menu( MENU_STR* menu, long *func_key, int level )
{
char *new_name;
char *item;
char *old;
int  cnt;
int  len;
int  width;
int  wid;
long func;
int  pos;
int  seps;
int  inner[10];
int  j;
static const char *fc, *fc2;
static int  frame = -1;

   if (frame != g_display.frame_style) {
      frame = g_display.frame_style;
      fc = graphic_char[frame];
      fc2 = (frame < 3) ? fc : graphic_char[2];
   }

   cnt   = menu->minor_cnt;
   pos   = 0;                          /* Position into windowletters */
   width = 0;                          /* Length of the longest function */
   wid   = -2;                         /* Length of the longest key */
   seps  = 0;                          /* Number of separators */
   for (j = 1; j < cnt - 1; ++j) {
      item = menu->minor[j].minor_name;
      if (item && *item == '\0')
         ++item;
      func = menu->minor[j].minor_func;
      if (func == ERROR && (item == NULL || *item != ' '))
         inner[seps++] = j;
      if (item != NULL) {
         len = strlen( item );
         if (len > width)
            width = len;
         item = create_menu_key( func, func_key );
         if (item != NULL) {
            len = strlen( item );
            if (len > wid)
               wid = len;
         }
      }
   }
   /* frame+space, letter+bracket+space, function, two spaces,
      key, space+frame */
   len = 2 + 3 + width + 2 + wid + 2;
   new_name = create_frame( len, cnt, seps, inner, -1, -1 );
   if (new_name == NULL)
      return;

   old = menu->minor[0].line;
   if (old != NULL)
      free( old );

   for (j = 0; j < cnt; new_name += len+1, ++j) {
      item = menu->minor[j].minor_name;
      if (item && *item == '\0')
         ++item;
      func = menu->minor[j].minor_func;
      if (func < 0) {
         if (item != NULL) {
            if (*item == ' ')
               ++item;
            wid = strlen( item );
            memcpy( new_name + (len-wid) / 2, item, wid );
         }
      } else {
         if (windowletters[pos]) {
            new_name[2] = windowletters[pos++];
            new_name[3] = ')';
         }
         memcpy( new_name+5, item, strlen( item ) );
         item = create_menu_key( func, func_key );
         if (item != NULL) {
            wid = strlen( item );
            memcpy( new_name + len-2-wid, item, wid );
         }
      }
      menu->minor[j].line = new_name;
   }

   menu->width = len;

   len = (len + 2 + g_display.shadow_width) * (cnt + 1);
   if (len > buf_len[level]) {
      buf_len[level] = len;
      if (buffer[level]) {
         free( buffer[level] );
         buffer[level] = NULL;
      }
   }
}


/*
 * Name:    create_menu_key
 * Purpose: create the string to be used for the key in the menu
 * Author:  Jason Hood
 * Date:    October 25, 1999
 * Passed:  func:     menu function or key
 *          func_key: array relating functions to keys
 * Returns: pointer to the key string or NULL if no key
 * Notes:   the pointer is static.
 * 010624:  use key 0 for the pop-out menu indicator.
 * 031204:  recognise PseudoMacro triggers (for the User menu).
 */
static char *create_menu_key( long func, long *func_key )
{
static char name[40];
long key;
int  parent;
int  len;

   if (func == ERROR)
      return( NULL );

   if (func == 0) {
      name[0] = POP_OUT;
      name[1] = '\0';

   } else if (func >= 0x2121 && func < 0x10000L) {
      name[0] = (unsigned)func >> 8;
      name[1] = (int)func & 0xff;
      name[2] = '\0';

   } else if (func_key == NULL && (func & _FUNCTION))
      return( NULL );

   else {
      if (func & _FUNCTION) {
         key = func_key[(int)func & ~_FUNCTION];
         if (key == 0)
            return( NULL );
      } else
         key = func;
      parent = PARENT_KEY( key );
      if (parent) {
         menu_key_name( parent, name );
         len = strlen( name );
         name[len++] = ' ';
         menu_key_name( CHILD_KEY( key ), name + len );
      } else
         menu_key_name( (int)key, name );
   }

   return( name );
}


/*
 * Name:    menu_key_name
 * Purpose: create a function key name for the menu
 * Author:  Jason Hood
 * Date:    October 25, 1999
 * Passed:  key:  key to create
 *          buf:  buffer to place name
 * Notes:   function keys only (no two-keys).
 *
 * jmh 021031: combined the '+' directly into the key (which also allows
 *              '-' to be defined in the config, if you'd prefer).
 */
static void menu_key_name( int key, char *buf )
{
   *buf = '\0';
   if (key & _SHIFT) strcat( buf, key_word[KEY( _SHIFTKEY )] );
   if (key & _CTRL)  strcat( buf, key_word[KEY( _CONTROLKEY )] );
   if (key & _ALT)   strcat( buf, key_word[KEY( _ALTKEY )] );
                     strcat( buf, key_word[KEY( key )] );
}


/*
 * Name:    viewer
 * Purpose: determine if a key is a viewer key
 * Author:  Jason Hood
 * Date:    August 18, 2002
 * Passed:  key:  key to test
 * Returns: TRUE for a viewer key, FALSE otherwise.
 * Notes:   used to prevent viewer keys being placed on the menu.
 */
int  viewer( long key )
{
   /*
    * Only unshifted and Shift keys can be viewer.
    */
   if (key & (_CTRL | _ALT))
      return( FALSE );

   key = KEY( key ) | 256;
   if (key == _ESC    ||  key == _BACKSPACE  ||  key == _TAB  ||
       key == _ENTER  ||  key == _GREY_STAR  ||
       (key > _SPACEBAR  &&  key != _LEFT_BACKSLASH))
      return( FALSE );

   return( TRUE );
}
