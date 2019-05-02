/*
 * The "macro" routines in TDE are not really macro routines in the classical
 * sense.  In TDE, "macros" are just recordings of key sequences.  But,
 * actually, I find the ability to record and playback keystrokes generally
 * more useful than writing a macro.  Being that I'm so stupid, it takes me
 * half the morning to write and half the afternoon to debug a simple classical
 * macro.  For most of my routine, little jobs, I find it more convenient
 * to record my keystrokes and playback those keystrokes at appropriate places
 * in a file.  It just seems easier for me to "teach" the editor what to do
 * rather than "instruct" the editor what to do.
 *
 *********************  start of Jason's comments  *********************
 *
 * Rhide has a feature called "pseudo-macros". These macros are triggered
 * by the two characters before the cursor. Eg: if you type in "ma" and
 * hit the pseudo-macro key, you could get:
 *
 *      int main(int argc, char* argv[])
 *      {
 *        _
 *
 *        return 0;
 *      }
 *
 * with the cursor placed at the underscore. I've incorporated two types
 * of pseudo-macros in TDE: global (defined in tde.cfg) and language-
 * dependent (defined in tde.shl). Since these macros are created at run-
 * time, I've done away with the strokes buffer and allocated them from
 * the heap. This means that DOS must include the config file and that
 * macros can no longer be stored permanently.
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
 * This modification of Douglas Thomson's code is released into the
 * public domain, Frank Davis.  You may distribute it freely.
 */


#include "tdestr.h"             /* tde types */
#include "common.h"
#include "define.h"
#include "tdefunc.h"
#include "config.h"


extern long found_rline;        /* defined in findrep.c */


/*
 *              keystroke record functions
 */

/*
 * Name:    record_on_off
 * Purpose: save keystrokes in keystroke buffer
 * Date:    April 1, 1992
 * Passed:  window:  pointer to current window
 * Notes:   rewritten by Jason Hood, July 17, 1998.
 *          if the PseudoMacro key is chosen as play back, record
 *           a pseudo-macro.
 *          Pseudo-macros and two-keys are assumed to be recorded in
 *           the one window (or at least the one language).
 *
 * jmh 021029: disable any previous macro mark.
 */
int  record_on_off( TDE_WIN *window )
{
long key;
int  func = 0;
MACRO *mac    = NULL;
TREE  *branch = NULL;
TREE  *tree;
int  line;
char temp[MAX_COLS+2];
DISPLAY_BUFF;
int no_memory = FALSE;
int existing = FALSE;           /* pseudo-macro or two-key already exists */
long *keys;
int  rc;

   tree = (window != NULL && window->syntax) ?
          &window->file_info->syntax->key_tree : &key_tree;

   mode.record = !mode.record;
   if (mode.record == TRUE) {
      line = window->bottom_line;
      SAVE_LINE( line );
      /*
       * press key that will play back recording
       */
      set_prompt( main11, line );

      /*
       * get the candidate macro key and look up the function assigned to it.
       */
      g_status.viewer_key = TRUE;
      key = getkey( );
      g_status.key_macro = NULL;
      if (PARENT_KEY( key )) {
         branch = search_tree( key, tree->right );
         if (branch != NULL) {
            existing = TRUE;
            func = branch->func;
            g_status.key_macro = branch->macro;
         }
      } else
         func = getfunc( key );

      if (key == ESC || func == AbortCommand || key == _CONTROL_BREAK)
         mode.record = FALSE;

      /*
       * the key must be an unused, recognized function key or a function
       *  key assigned to a previously defined macro.
       */
      else if (key <= 256 || (func != 0 && func != PlayBack &&
                                           func != PseudoMacro)) {
         /*
          * cannot assign a recording to this key
          */
         join_strings( temp, main12, main12a );
         error( WARNING, line, temp );
         mode.record = FALSE;
      }
      if (mode.record == TRUE && func == PseudoMacro) {
         key = find_combination( window );
         if (key == 0) {
            /*
             * cannot assign a recording to this combination
             */
            join_strings( temp, main12, main12b );
            error( WARNING, line, temp );
            mode.record = FALSE;
         } else {
            branch = search_tree( key, tree->left );
            if (branch != NULL) {
               existing = TRUE;
               g_status.key_macro = branch->macro;
            }
         }
      }
      if (mode.record == TRUE) {
         /*
          * check for a previous macro
          */
         if ((mac = g_status.key_macro) != NULL) {
            /*
             * overwrite recording?
             */
            if (get_yn( main14, line, R_PROMPT | R_ABORT ) != A_YES)
               mode.record = FALSE;
         }
      }
      if (mode.record == TRUE) {
         if (mac != NULL) {
            /*
             * key has already been assigned to a macro, clear macro def.
             */
            if (mac->len > 1)
               my_free( mac->key.keys );
            mac->len = 0;
         } else {
            if ((func == PseudoMacro || PARENT_KEY( key )) && !existing) {
               if ((branch = my_calloc( sizeof(TREE) )) == NULL) {
                  mode.record = FALSE;
                  no_memory   = TRUE;
               }
            }
            if (mode.record == TRUE) {
               if ((mac = my_calloc( sizeof(MACRO) )) == NULL) {
                  mode.record = FALSE;
                  no_memory   = TRUE;
               }
            }
         }
      }
      if (mode.record == TRUE) {
         /*
          * Try allocating STROKE_LIMIT first. If it fails, halve
          * it each time and try again.
          */
         g_status.stroke_count = STROKE_LIMIT;
         while ((mac->key.keys = my_malloc( g_status.stroke_count *
                                              sizeof(long), &rc )) == NULL  &&
                 g_status.stroke_count > 1)
            g_status.stroke_count /= 2;
         if (mac->key.keys == NULL) {
            mode.record = FALSE;
            no_memory   = TRUE;
         }
      }
      if (mode.record == TRUE) {
         g_status.recording_key = key;
         g_status.rec_macro     = mac;

         mac->mode[0] = mode.insert;
         mac->mode[1] = mode.smart_tab;
         mac->mode[2] = mode.indent;
         g_status.macro_mark.marked = FALSE;

         if (func == PseudoMacro || PARENT_KEY( key )) {
            branch->macro = mac;
            if (!existing) {
               branch->key = key;
               branch->func = PlayBack;
               add_branch( branch, tree );
            }
         } else {
            macro[KEY_IDX( key )] = mac;
            key_func[KEY_IDX( key )] = PlayBack;
         }

         show_recording( );

      } else if (no_memory) {
         /*
          * out of memory
          */
         error( WARNING, line, main4 );
         my_free( mac );
         if (!existing)
            my_free( branch );
      }
      RESTORE_LINE( line );
   }

   if (mode.record == FALSE && (mac = g_status.rec_macro) != NULL) {
      /*
       * the flashing "Recording" and the stroke count write over the modes.
       *  when we get thru defining a macro, redisplay the modes.
      */
      show_modes( );
      /*
       * let's look at the macro.  if no strokes were recorded, free the
       * macro structure; if one stroke was recorded use the pointer to
       * keep it; otherwise realloc to the size required.
       */
      if (mac->len == 0) {
         my_free( mac->key.keys );
         my_free( mac );
         key = g_status.recording_key;
         /*
          * retain the branch because I really couldn't be
          * bothered to go to the trouble of removing it.
          */
         if (key >= 0x2121) {
            branch = search_tree( key, tree );
            branch->func = 0;
            branch->macro = NULL;
         } else {
            macro[KEY_IDX( key )] = NULL;
            key_func[KEY_IDX( key )] = 0;
         }
      } else if (mac->len == 1) {
         key = *mac->key.keys;
         my_free( mac->key.keys );
         mac->key.key = key;
      } else {
         keys = my_realloc( mac->key.keys, mac->len * sizeof(long), &rc );
         if (keys != NULL)
            mac->key.keys = keys;
         /*
          * Finally, move the cursor to the mark, if one was specified.
          */
         g_status.command = MacroMark;
         goto_marker( window );
      }
      g_status.recording_key = 0;
      g_status.rec_macro     = NULL;
   }
   return( OK );
}


/*
 * Name:    find_combination
 * Purpose: determine the two-character key for a pseudo-macro
 * Author:  Jason Hood
 * Date:    July 18, 1998
 * Passed:  window: current window
 * Returns: 0 for an invalid combination;
 *          the key value otherwise.
 */
unsigned find_combination( TDE_WIN *window )
{
line_list_ptr line;
text_ptr      text;
int  len;
unsigned key = 0;
int  rcol;

   rcol = window->rcol;
   if (g_status.copied) {
      text = g_status.line_buff;
      len  = g_status.line_buff_len;
   } else {
      line = window->ll;
      text = line->line;
      len  = line->len;
   }
   if (window->file_info->inflate_tabs)
      rcol = entab_adjust_rcol( text, len, rcol, window->file_info->ptab_size );
   if (len >= 2) {
      if (rcol >= 2 && rcol <= len && text[rcol-1] > ' ' && text[rcol-2] > ' ')
         key = (text[rcol-2] << 8) | text[rcol-1];
   }
   return( key );
}


/*
 * Name:    search_tree
 * Purpose: search a tree for a matching key
 * Author:  Jason Hood
 * Date:    July 29, 1998
 * Passed:  key:   value to find
 *          tree:  tree to search
 * Returns: pointer to the branch if found;
 *          NULL if not found.
 */
TREE *search_tree( long key, TREE *tree )
{
   while (tree != NULL && key != tree->key)
      tree = (key < tree->key) ? tree->left : tree->right;

   return( tree );
}


/*
 * Name:    find_pseudomacro
 * Purpose: search the trees for a matching key
 * Author:  Jason Hood
 * Date:    July 20, 1998
 * Passed:  key:    pseudo-macro to find
 *          window: current window
 * Returns: pointer to the macro if found;
 *          NULL if not found.
 * Notes:   if it can't be found in the language tree, search the global.
 */
MACRO *find_pseudomacro( unsigned key, TDE_WIN *window )
{
TREE *branch;
MACRO *mac = NULL;
LANGUAGE *language;

   if (window != NULL && window->syntax) {
      language = window->file_info->syntax;
      do {
         branch = search_tree( key, language->key_tree.left );
         if (branch != NULL)
            mac = branch->macro;
         language = language->parent;
      } while (mac == NULL && language != NULL);
   }
   if (mac == NULL) {
      branch = search_tree( key, key_tree.left );
      if (branch != NULL)
         mac = branch->macro;
   }
   return( mac );
}


/*
 * Name:    add_branch
 * Purpose: add a branch to the tree
 * Author:  Jason Hood
 * Date:    July 29, 1998
 * Passed:  branch: pointer to branch to be added
 *          tree:   pointer to tree to add it
 * Notes:   assumes tree is not NULL.
 */
void add_branch( TREE *branch, TREE *tree )
{
TREE **new_branch = &tree;

   do
      new_branch = (branch->key < (*new_branch)->key) ? &(*new_branch)->left :
                                                        &(*new_branch)->right;
   while (*new_branch != NULL);

   *new_branch = branch;
}


/*
 * Name:    record_key
 * Purpose: save keystrokes in keystroke buffer
 * Date:    April 1, 1992
 * Passed:  key:  key to record
 *          func: function of key
 * Notes:   rewritten by Jason Hood, July 17, 1998.
 * jmh 980722: add a bit of "intelligence" - don't record a null function key
 *              and test for backspacing over a character.
 * jmh 980726: use the bottom line of current window to display an error.
 * jmh 980809: don't record the Help function.
 * jmh 980826: when possible, store the function rather than the key.
 *             don't record the PullDown function.
 */
void record_key( long key, int func )
{
MACRO *mac;

   if (mode.record == TRUE && (func != 0 || key < 256) &&
       func != RecordMacro    && func != SaveMacro &&
       func != ClearAllMacros && func != LoadMacro &&
       func != Help           && func != PullDown) {

      mac = g_status.rec_macro;

      if (func == BackSpace && mac->len > 0 && mac->key.keys[mac->len-1] < 256)
         show_avail_strokes( +1 );

      else if (g_status.stroke_count == 0)
         /*
          * no more room in recording buffer
          */
         error( WARNING, g_status.current_window->bottom_line, main13 );

      else {
         mac->key.keys[mac->len] = (func == 0 || func == PlayBack)
                                   ? key : (func | _FUNCTION);
         show_avail_strokes( -1 );
      }
   }
}


/*
 * Name:    show_avail_strokes
 * Purpose: show available free key strokes in lite bar at bottom of screen
 * Date:    April 1, 1992
 * jmh 980722: update the macro length and stroke count according to update.
 * jmh 981129: moved it back one position.
 */
void show_avail_strokes( int update )
{
   g_status.rec_macro->len -= update;
   g_status.stroke_count   += update;

   s_output( main16, g_display.mode_line, 32, Color( Mode ) );
   n_output( g_status.stroke_count, 0, 50, g_display.mode_line, Color( Mode ) );
}


/*
 * Name:    save_strokes
 * Purpose: save strokes to a file
 * Date:    November 13, 1993
 * Passed:  window:  pointer to current window
 * Notes:   rewritten by Jason Hood, July 17, 1998
 *          This is just basically a config file. Search the config defs
 *           for the appropriate string.
 *          If syntax highlighting is on just write the language macros,
 *           otherwise write the global macros.
 * jmh 980723: provide a default filename.
 * jmh 980821: create an array of key strings and function names.
 * jmh 020904: the key strings have been defined in cfgfile.c.
 * jmh 020923: likewise the function strings.
 */
int  save_strokes( TDE_WIN *window )
{
FILE *fp;                       /* file to be written */
register int rc;
int  prompt_line;
char answer[PATH_MAX+2];
int  key;
TREE *tree;
int  i, j;

   prompt_line = window->bottom_line;

   /*
    * Supply a default name - either the language or "global".
    */
   if (window->syntax) {
#if defined( __UNIX__ )  ||  defined( __WIN32__ )
      join_strings( answer, window->file_info->syntax->name, ".tdm" );
#else
      key = strlen( window->file_info->syntax->name );
#if defined( __DJGPP__ )
      if (!_USE_LFN)
#endif
         if (key > 8)
            key = 8;
      memcpy( answer, window->file_info->syntax->name, key );
      strcpy( answer+key, ".tdm" );
#endif
   } else
      strcpy( answer, "global.tdm" );
   /*
    * name for macro file
    */
   if ((rc = get_name( main19, prompt_line, answer, &h_file )) > 0) {
      /*
       * make sure it is OK to overwrite any existing file
       */
      rc = file_exists( answer );
      if (rc == SUBDIRECTORY)
         rc = ERROR;
      else if (rc == OK || rc == READ_ONLY) {
         /*
          * overwrite existing file?
          */
         if (get_yn( utils10, prompt_line, R_PROMPT | R_ABORT ) != A_YES  ||
             change_mode( answer, prompt_line ) == ERROR)
            rc = ERROR;
      } else
         /*
          * file name does not exist. take a chance on a valid file name.
          */
         rc = OK;

      if (rc != ERROR) {
         if ((fp = fopen( answer, "w" )) != NULL) {
            format_time( main20c, answer, NULL );

            if (!window->syntax) {
               fprintf( fp, main20a, answer );
               for (j = 0; j < MODIFIERS; ++j) {
                  key = 256 | (j << 9);
                  for (i = 0; i < MAX_KEYS; ++i) {
                     if (macro[j][i] != NULL) {
                        fprintf( fp, "%s %s", key_name( i | key, answer ),
                                              func_str[PlayBack] );
                        write_macro( fp, macro[j][i], i | key );
                     }
                  }
               }
               tree = &key_tree;
            } else {
               fprintf( fp, main20b, window->file_info->syntax->name, answer );
               tree = &window->file_info->syntax->key_tree;
            }
            write_pseudomacro( fp, tree->left );
            write_twokeymacro( fp, tree->right );
            fclose( fp );
         }
      }
   }
   return( OK );
}


/*
 * Name:    write_macro
 * Purpose: write a macro structure to a file (helper function)
 * Author:  Jason Hood
 * Date:    July 17, 1998
 * Passed:  file:     pointer to file structure
 *          macro:    pointer to macro to be written
 *          mkey:     key of macro
 * Notes:   if the macro is only one character, write it on the same line;
 *          otherwise use separate lines with two-space indentation.
 *
 * 050709:  recognise recursive definitions.
 * 051222:  write a literal on the same line as its presumed function;
 *          split literals at "\n".
 */
void write_macro( FILE *file, MACRO *macro, long mkey )
{
int  j;
long key;
int  func;
int  literal = FALSE;
int  function = FALSE;
char str[KEY_STR_MAX];
char ch;

   if (macro->len == 1) {
      key = macro->key.key;
      if (key < 256) {
         fprintf( file, " \"%c\"\n\n", (int)key );
         return;
      }
   }

   if (macro->mode[0] == FALSE)
      fputs( " Overwrite", file );
   if (macro->mode[1] == FALSE)
      fputs( " FixedTabs", file );
   if (macro->mode[2] == FALSE)
      fputs( " NoIndent", file );

   if (macro->flag & WRAP)
      fputs( " Wrap", file );
   if (macro->flag & NOWRAP)
      fputs( " NoWrap", file );
   if (macro->flag & NOWARN)
      fputs( " NoWarn", file );
   if ((macro->flag & NEEDBLOCK) == NEEDBLOCK)
      fputs( " NeedBlock", file );
   else {
      if (macro->flag & NEEDBOX)
         fputs( " NeedBox", file );
      if (macro->flag & NEEDLINE)
         fputs( " NeedLine", file );
      if (macro->flag & NEEDSTREAM)
         fputs( " NeedStream", file );
   }
   if (macro->flag & USESBLOCK)
      fputs( " UsesBlock", file );
   if (macro->flag & USESDIALOG)
      fputs( " UsesDialog", file );

   for (j = 0; j < macro->len; ++j) {
      key = (macro->len == 1) ? macro->key.key : macro->key.keys[j];
      if (key == mkey  &&  !literal) {       /* why a recursive pseudomacro? */
         fputs( "\nPlayBack\n\n", file );    /* func_str is Macro */
         return;                             /* must be last */
      }
      if (key < 256) {
         if (!literal) {
            if (function)
               fputc( ' ', file );
            else
               fputs( "\n  ", file );
            fputc( '\"', file );
            literal = TRUE;
            function = FALSE;
         }
         fputc( (int)key, file );
         if ((int)key == '\\' || (int)key == '\"')
            fputc( (int)key, file );
      } else {
         func = getfunc( key );
         if (literal) {
            ch = '\"';
            switch (func) {
               case BackSpace   : ch = MAC_BackSpace; break;
               case CharLeft    : ch = MAC_CharLeft;  break;
               case CharRight   : ch = MAC_CharRight; break;
               case Rturn       : ch = MAC_Rturn;     break;
               case PseudoMacro : ch = MAC_Pseudo;    break;
               case Tab         : ch = MAC_Tab;       break;
               case MacroMark   : ch = '0';           break;
               case SetMark1    : ch = '1';           break;
               case SetMark2    : ch = '2';           break;
               case SetMark3    : ch = '3';           break;
            }
            if (ch != '\"')
               fputc( '\\', file );
            else
               literal = FALSE;
            fputc( ch, file );
            if (ch == MAC_Rturn) {
               fputc( '\"', file );
               literal = FALSE;
               function = FALSE;
               continue;
            }
            if (literal)
               continue;
         }
         fputs( "\n  ", file );
         if (key & _FUNCTION)
            fputs( func_str[func], file );
         else
            fputs( key_name( key, str ), file );
         function = TRUE;
      }
   }
   if (literal)
      fputc( '\"', file );

   fprintf( file, "\n%s\n\n", func_str[RecordMacro] );
}


/*
 * Name:    write_pseudomacro
 * Purpose: write the list of pseudo-macros to file
 * Author:  Jason Hood
 * Date:    July 17, 1998
 * Passed:  file:     pointer to file being written
 *          pmacro:   pointer to pseudo-macro being written
 * Notes:   recursive function traverses the tree, writing macros
 *           in order, smallest first.
 * 020819:  remove tail recursion.
 * 050709:  recognise semicolon and quote.
 */
void write_pseudomacro( FILE *file, TREE *pmacro )
{
int  t1, t2;

   while (pmacro != NULL) {
      write_pseudomacro( file, pmacro->left );
      if (pmacro->macro != NULL) {
         fputs( func_str[PseudoMacro], file );
         fputc( ' ', file );
         t1 = (int)pmacro->key >> 8;
         t2 = (int)pmacro->key & 0xff;
         if (t1 == '\"' || t2 == '\"') {
            fputc( '\"', file );
            if (t1 == '\"')
               fputs( "\"\"", file );
            else
               fputc( t1, file );
            if (t2 == '\"')
               fputs( "\"\"", file );
            else
               fputc( t2, file );
            fputc( '\"', file );
         } else if (t1 == ';' || t2 == ';')
            fprintf( file, "\"%c%c\"", t1, t2 );
         else
            fprintf( file, "%c%c", t1, t2 );
         write_macro( file, pmacro->macro, pmacro->key );
      }
      pmacro = pmacro->right;
   }
}


/*
 * Name:    write_twokeymacro
 * Purpose: write the list of two-key macros to file
 * Author:  Jason Hood
 * Date:    July 29, 1998
 * Passed:  file:     pointer to file being written
 *          twokey:   pointer to two-key macro being written
 * Notes:   recursive function traverses the tree, writing macros
 *           in order, smallest first.
 * 020819:  remove tail recursion.
 */
void write_twokeymacro( FILE *file, TREE *twokey )
{
char buf[KEY_STR_MAX];

   while (twokey != NULL) {
      write_twokeymacro( file, twokey->left );
      if (twokey->macro != NULL) {
         fprintf( file, "%s %s", key_name( twokey->key, buf ),
                                 func_str[PlayBack] );
         write_macro( file, twokey->macro, twokey->key );
      }
      twokey = twokey->right;
   }
}


/*
 * Name:    key_name
 * Purpose: determine the string associated with a key
 * Author:  Jason Hood
 * Date:    August 21, 1998
 * Passed:  key:     key number to convert
 *          buffer:  place to store the string
 * Returns: buffer
 * Notes:   minimum buffer size should be KEY_STR_MAX characters (see config.h)
 *
 * 990428:  place ';' in quotes.
 *          recognize the viewer keys.
 * 990429:  write '"' correctly ("""").
 */
char *key_name( long key, char *buffer )
{
int  twokey;
int  j = 0;

   twokey = PARENT_KEY( key );
   if (twokey) {
      j = strlen( key_name( twokey, buffer ) );
      buffer[j++] = ' ';
      key = CHILD_KEY( key );
   }

   if (key & _SHIFT) {
      buffer[j++] = 's';
      buffer[j++] = '+';
   }
   if (key & _CTRL) {
      buffer[j++] = 'c';
      buffer[j++] = '+';
   }
   if (key & _ALT) {
      buffer[j++] = 'a';
      buffer[j++] = '+';
   }
   strcpy( buffer+j, cfg_key[KEY( key )] );

   return( buffer );
}


/*
 * Name:    clear_macros
 * Purpose: reset all macro buffers, pointers, functions.
 * Date:    April 1, 1992
 * Notes:   reset the available macro stroke count.  reset all fields in
 *           macro structure.  clear any keys assigned to macros in the
 *           function assignment array.
 * July 17, 1998 (jmh): if syntax highlighting is on, only clear the
 *                       language macros; conversely, if syntax highlighting
 *                       is off, only clear the global macros.
 */
int  clear_macros( TDE_WIN *window )
{
register int i, j;
TREE *tree;

   if (!window->syntax) {
      for (j = 0; j < MODIFIERS; ++j) {
         for (i = 0; i < MAX_KEYS; i++) {
            if (macro[j][i] != NULL) {
               my_free( macro[j][i]->key.keys );
               my_free( macro[j][i] );
               macro[j][i] = NULL;
               key_func[j][i] = 0;
            }
         }
      }
      tree = &key_tree;
   } else
      tree = &window->file_info->syntax->key_tree;

   delete_pseudomacro( tree->left );
   tree->left = NULL;

   delete_twokeymacro( tree->right );

   return( OK );
}


/*
 * Name:    delete_pseudomacro
 * Purpose: remove the pseudo-macro tree
 * Author:  Jason Hood
 * Date:    July 17, 1998
 * Passed:  tree: tree to be deleted
 * Notes:   recursively delete all pseudo-macros
 */
void delete_pseudomacro( TREE *tree )
{
   if (tree == NULL)
      return;

   if (tree->left == NULL && tree->right == NULL) {
      if (tree->macro != NULL) {
         if (tree->macro->len > 1)
            my_free( tree->macro->key.keys );
         my_free( tree->macro );
      }
      my_free( tree );
   } else {
      delete_pseudomacro( tree->left );
      tree->left = NULL;
      delete_pseudomacro( tree->right );
      tree->right = NULL;
      delete_pseudomacro( tree );
   }
}


/*
 * Name:    delete_twokeymacro
 * Purpose: delete all two-key macros
 * Author:  Jason Hood
 * Date:    July 30, 1998
 * Passed:  tree: tree to be deleted
 * Notes:   unlike delete_pseudomacro, this does not actually delete anything,
 *           since the two-key functions need to be preserved.
 * 020819:  remove tail recursion.
 */
void delete_twokeymacro( TREE *tree )
{
   while (tree != NULL) {
      if (tree->macro != NULL) {
         if (tree->macro->len > 1)
            my_free( tree->macro->key.keys );
         my_free( tree->macro );
         tree->func = 0;
         tree->macro = NULL;
      }
      delete_twokeymacro( tree->left );
      tree = tree->right;
   }
}


/*
 *              keystroke play back functions
 */


/*
 * Name:    test_for_html
 * Class:   macro helper function
 * Purpose: special pseudo-macro case for HTML
 * Author:  Jason Hood
 * Date:    June 1, 2001
 * Passed:  window:  current window
 * Returns: TRUE if special case was found, ERROR otherwise.
 * Notes:   if a pseudo-macro was not found, see if we're on a word that begins
 *           with '<'; if so, use it to form an HTML pair. Eg:
 *           <font --> <font></font> with the cursor left unchanged (ie. at the
 *           first '>').
 *          only takes the word from before the cursor.
 *          always uses insert mode.
 * 050908:  fix cursor position due to scrolling.
 */
static int  test_for_html( TDE_WIN *window )
{
LANGUAGE *language;
line_list_ptr line;
text_ptr      text;
int  len;
text_t buf[MAX_LINE_LENGTH];
int  rcol, ccol;
int  pos;
int  ins;
int  rc = ERROR;

   language = window->file_info->syntax;
   if (language == NULL)
      return( ERROR );

   do {
      /* no need for my_strcmp, since "html" is English */
      if (stricmp( language->name, "html" ) == 0)
         break;
      language = language->parent;
   } while (language != NULL);
   if (language == NULL)
      return( ERROR );

   rcol = window->rcol;
   if (g_status.copied) {
      text = g_status.line_buff;
      len  = g_status.line_buff_len;
   } else {
      line = window->ll;
      text = line->line;
      len  = line->len;
   }
   if (window->file_info->inflate_tabs)
      rcol = entab_adjust_rcol( text, len, rcol, window->file_info->ptab_size );
   if (len >= 2 && rcol >= 2 && rcol <= len) {
      len = 0;
      while (--rcol > 0  &&  myisnotwhitespc( text[rcol] ))
         ++len;
      if (text[rcol] == '<' && len > 0) {
         buf[0] = '>';
         buf[1] = '<';
         buf[2] = '/';
         pos = 3;
         do
            buf[pos++] = text[++rcol];
         while (--len != 0);
         buf[pos++] = '>';
         buf[pos]   = '\0';
         rcol = window->rcol;
         ccol = window->ccol;
         ins  = mode.insert;
         mode.insert = TRUE;
         rc = add_chars( buf, window );
         mode.insert = ins;
         check_virtual_col( window, rcol, ccol );
      } else
         rc = ERROR;
   }
   return( rc );
}


static BLOCK block;


/*
 * Name:    play_back
 * Purpose: play back a series of keystrokes assigned to key
 * Date:    April 1, 1992
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen
 * Notes:   go thru the macro key list playing back the recorded keystrokes.
 *          to let macros call other macros, we have to 1) save the next
 *           keystroke of the current macro in a stack, 2) execute the
 *           the called macro, 3) pop the key that saved by the calling
 *           macro, 4) continue executing the macro, beginning with the
 *           key we just popped.
 *          use a stack to store keys.
 *          Rewritten by Jason Hood, July 18, 1998.
 * jmh 980726: with the repeat function, this function needs to return
 *              an appropriate status.
 * jmh 990214: recognize the break-point for recursive macros (why wasn't it
 *              already done??)
 * jmh 990410: changed (rc == OK) conditions to (rc != ERROR) since not all
 *              functions return OK (such as set_*_margin() ).
 * jmh 991027: corrected a bug where using a macro to play a macro twice
 *              would result in a false recursive macro.
 * jmh 010601: special case for pseudo-macros and HTML.
 * jmh 021210: have recursive macros return OK (so other macros can call them).
 * jmh 050709: test for valid block type.
 */
int  play_back( TDE_WIN *window )
{
int    rc = OK;
int    popped;          /* flag is set when macro is popped */
MACRO *mac = NULL;
int    cur_mode[3];     /* remember the current editor settings */
unsigned pkey = 0;
MACRO *pmac = NULL;
MACRO_STACK ms;
MACRO_STACK *old_ms = NULL;
int    html = FALSE;    /* are we using the HTML language? */
file_infos *file;
int    recursed;

   if (g_status.macro_executing) {
      /*
       * jmh 021024: It is possible for the startup macro to recurse
       *             (typically by ending it with File); if that is the case,
       *             restart it and let the current play_back() execute it.
       */
      if (g_status.current_macro == macro[KEY_IDX( _CONTROL_BREAK )]) {
         g_status.macro_next = 0;
         return( ERROR );
      }

      old_ms   = g_status.mstack;
      g_status.mstack = NULL;
   }
   ms.macro = g_status.current_macro;   /* these are part of the above if */
   ms.pos   = g_status.macro_next;      /* but moved outside to avoid GCC */
   ms.mark  = g_status.macro_mark;      /* uninitialised warnings         */

   /*
    * Determine if it is a valid pseudo-macro.
    * If so, set the key pressed to the combination.
    * If not, and we're recording, forget the key.
    */
   if (g_status.command == PseudoMacro) {
      if ((pkey = find_combination( window )) == 0 ||
          (pmac = find_pseudomacro( pkey, window )) == NULL) {
         rc = test_for_html( window );
         if (rc == OK)
            html = TRUE;
         else if (mode.record == TRUE)
            show_avail_strokes( +1 );
      } else {
         g_status.key_pressed = pkey;
         g_status.key_macro   = pmac;
      }
   }

   /*
    * if we are recording a macro, let's just return if we do a recursive
    *  definition.  Otherwise, we end up executing our recursive macro
    *  while we are defining it.
    * jmh 980722: may as well stop recording altogether.
    */
   if (mode.record == TRUE && g_status.key_pressed == g_status.recording_key) {
      record_on_off( window );
      rc = ERROR;
   }

   if (rc == OK  &&  !html  &&  (g_status.key_macro->flag & NEEDBLOCK)) {
      const char *errmsg = NULL;
      if (g_status.marked) {
         int type;
         switch (g_status.marked_file->block_type) {
            case BOX:    type = NEEDBOX;    break;
            case LINE:   type = NEEDLINE;   break;
            case STREAM: type = NEEDSTREAM; break;
            default:
               type = 0;
               assert( FALSE );
         }
         if (!(type & g_status.key_macro->flag))
            errmsg = main18;
      } else
         errmsg = main17;
      if (errmsg) {
         error( WARNING, window->bottom_line, errmsg );
         rc = ERROR;
      }
   }

   if (rc == OK  &&  !html) {
      cur_mode[0] = mode.insert;
      cur_mode[1] = mode.smart_tab;
      cur_mode[2] = mode.indent;

      file = window->file_info;

      /*
       * set the global macro flags, so other routines will know
       *  if a macro is executing.
       * initialize the popped flag to FALSE being that we haven't
       *  popped any thing off the stack, yet.
       */
      g_status.macro_executing = TRUE;
      popped = FALSE;
      rc     = OK;
      while (rc != ERROR) {

         /*
          * the first time thru the loop, popped is FALSE.
          */
         if (popped == FALSE) {
            /*
             * find the first keystroke in the macro.  when we pop the stack,
             *  all this stuff is reset by the pop -- do not reset it again.
             */
            g_status.macro_next = 0;
            mac = g_status.current_macro = g_status.key_macro;
            mode.insert    = mac->mode[0];
            mode.smart_tab = mac->mode[1];
            mode.indent    = mac->mode[2];
            g_status.macro_mark.marked = FALSE;
            if (mac->flag & USESBLOCK) {
               block.marked = g_status.marked;
               block.file   = g_status.marked_file;
               block.type   = file->block_type;
               block.bc     = file->block_bc;
               block.br     = file->block_br;
               block.ec     = file->block_ec;
               block.er     = file->block_er;
               unmark_block( window );
            }
         }
         popped = recursed = FALSE;
         while (rc != ERROR  &&  g_status.macro_next < mac->len) {

            /*
             * set up all editor variables as if we were entering
             *  keys from the keyboard.
             */
            window = g_status.current_window;
            display_dirty_windows( window );
            CEH_OK;
            g_status.key_pressed = (mac->len == 1) ? mac->key.key :
                                   mac->key.keys[g_status.macro_next];
            g_status.command = getfunc( g_status.key_pressed );
            if (g_status.wrapped) {
               g_status.wrapped = FALSE;
               show_search_message( CLR_SEARCH );
            }
            if (found_rline) {
               found_rline = 0;
               show_curl_line( window );
            }

            /*
             * while there are no errors or Control-Breaks, let's keep on
             *  executing a macro.  g_status.control_break is a global
             *  editor flag that is set in our Control-Break interrupt
             *  handler routine.
             */
            if (g_status.control_break == TRUE) {
               rc = ERROR;
               break;
            }

            /*
             * we haven't called any editor function yet.  we need
             *  to look at the editor command that is to be executed.
             *  if the command is PlayBack, we need to break out of
             *  this inner do loop and start executing the macro
             *  from the beginning (the outer do loop).
             *
             * if we don't break out now from a recursive macro, we will
             *  recursively call PlayBack and we will likely overflow
             *  the main (as opposed to the macro) stack.
             */
            if (g_status.command == PlayBack ||
                g_status.command == PseudoMacro) {

               if (g_status.command == PseudoMacro) {
                  if ((pkey = find_combination( window )) == 0 ||
                      (pmac = find_pseudomacro( pkey, window )) == NULL) {
                     ++g_status.macro_next;
                     continue;
                  }
                  g_status.key_macro = pmac;
               }
               /*
                * recursive macros are handled differently from
                *  macros that call other macros.
                * recursive macros - continue the macro from the beginning.
                * standard macros - save the next instruction of this
                *  macro on the stack and begin executing the called macro.
                */
               if (g_status.current_macro != g_status.key_macro) {
                  if (push_macro_stack( ) != OK) {
                     error( WARNING, window->bottom_line, ed16 );
                     rc = ERROR;
                  }
                  recursed = TRUE;
                  break;
               } else {
                  if (file->break_point != 0 &&
                      file->break_point == window->rline) {
                     rc = ERROR;
                     break;
                  }
                  g_status.macro_next = 0;
                  continue;
               }
            }

            ++g_status.macro_next;
            rc = execute( window );
            /*
             * jmh - A recursive macro that is closing windows could
             *       close the editor.
             */
            if (g_status.stop == TRUE)
               rc = ERROR;
         }

         /*
          * restore the block, irrespective of how the macro finished.
          */
         if (!recursed  &&  !g_status.stop  &&  (mac->flag & USESBLOCK)) {
            unmark_block( window );
            g_status.marked_file = block.file;
            g_status.marked  = block.marked;
            file->block_type = block.type;
            file->block_bc   = block.bc;
            file->block_br   = block.br;
            file->block_ec   = block.ec;
            file->block_er   = block.er;
            if (g_status.marked && g_status.marked_file != file) {
               g_status.marked_file->dirty = GLOBAL;
               g_status.marked_file->block_type = -block.file->block_type;
            }
         }

         /*
          * A recursive macro will always finish with an error condition.
          * Make it OK instead, to allow other macros to call it and continue.
          */
         if (g_status.current_macro == g_status.key_macro  &&
             !g_status.stop && !g_status.control_break) {
            rc = OK;
            g_status.macro_next = mac->len;
         }

         /*
          * If the macro finished, goto the marker if one was set.
          * If there's nothing on the stack, then get out.
          */
         if (rc != ERROR && g_status.macro_next == mac->len) {
            g_status.command = MacroMark;
            goto_marker( window );
            if (g_status.mstack == NULL)
               rc = ERROR;
            else if (pop_macro_stack( ) != OK) {
               error( WARNING, window->bottom_line, ed17 );
               rc = ERROR;
            } else {
               popped = TRUE;
               mac = g_status.current_macro;
            }
         }
      }
      /*
       * If the macro successfully finished, the last command
       * will be MacroMark.
       */
      if (g_status.command == MacroMark)
         rc = OK;
      /*
       * If the macro was aborted, tidy up the macro stack.
       */
      else while (g_status.mstack != NULL) {
         MACRO_STACK *ms = g_status.mstack;
         g_status.mstack = ms->prev;
         free( ms );
      }

      g_status.macro_executing = FALSE;
      mode.insert    = cur_mode[0];
      mode.smart_tab = cur_mode[1];
      mode.indent    = cur_mode[2];
      if (!mode.record) {
         show_insert_mode( );
         show_tab_modes( );
         show_indent_mode( );
      }
   }

   if (old_ms != NULL) {
      g_status.mstack = old_ms;
      g_status.macro_executing = TRUE;
      g_status.current_macro   = ms.macro;
      g_status.macro_next = ms.pos;
      g_status.macro_mark = ms.mark;
   }

   return( rc );
}


/*
 * Name:    push_macro_stack
 * Purpose: push the next key in a currently executing macro on local stack
 * Date:    October 31, 1992
 * Notes:   finally got tired of letting macros only "jump" and not call
 *           other macros.
 *          the first time in, mstack is NULL.
 *          Rewritten by Jason Hood, July 18, 1998.
 */
int  push_macro_stack( void )
{
MACRO_STACK *ms;

   /*
    * first, make sure we have room to push the key.
    */
   if ((ms = malloc( sizeof(MACRO_STACK) )) != NULL) {

      /*
       * Store the current macro, its next key and its marker.
       * Point to the previous macro on the stack and make this macro
       *  the top of the stack.
       */
      ms->macro = g_status.current_macro;
      ms->pos   = g_status.macro_next+1;
      ms->mark  = g_status.macro_mark;
      ms->block = block;
      ms->prev  = g_status.mstack;
      g_status.mstack = ms;
      return( OK );
   } else
      return( STACK_OVERFLOW );
}


/*
 * Name:    pop_macro_stack
 * Purpose: pop currently executing macro on local stack
 * Date:    October 31, 1992
 * Notes:   finally got tired of letting macros only "jump" and not "call"
 *           other macros.
 *          pop the macro stack.  stack pointer is pointing to last key saved
 *           on stack.
 *          Rewritten by Jason Hood, July 18, 1998.
 */
int  pop_macro_stack( void )
{
MACRO_STACK *ms = g_status.mstack;

   /*
    * before we pop the stack, make sure there is something in it.
    */
   if (ms != NULL) {

      /*
       * Pop the macro, its position and marker. Point to the previous macro.
       * Restore the editor modes to the original macro.
       */
      g_status.current_macro = ms->macro;
      g_status.macro_next = ms->pos;
      g_status.macro_mark = ms->mark;
      block               = ms->block;
      g_status.mstack     = ms->prev;
      free( ms );
      mode.insert    = g_status.current_macro->mode[0];
      mode.smart_tab = g_status.current_macro->mode[1];
      mode.indent    = g_status.current_macro->mode[2];
      return( OK );
   } else
      return( STACK_UNDERFLOW );
}


/*
 * Name:    macro_pause
 * Purpose: Enter pause state for macros
 * Date:    June 5, 1992
 * Passed:  arg_filler:  argument to satisfy function prototype
 * Returns: ERROR if the ESC key was pressed, OK otherwise.
 * Notes:   this little function is quite useful in macro definitions.  if
 *          it is called near the beginning of a macro, the user may decide
 *          whether or not to stop the macro.
 *
 * jmh 980525: only pause if a macro is recording or playing.
 * jmh 031114: don't display halt message if recording;
 *             test being called from a prompt
 */
int  macro_pause( TDE_WIN *arg_filler )
{
long c = 0;     /* 0 != ESC so returns OK if no macro */

   if (mode.record || g_status.macro_executing) {
      /*
       * tell user we are paused.
       */
      s_output( paused1, g_display.mode_line, 22, Color( Special ) );
      s_output( (mode.record) ? paused1a : paused2, g_display.mode_line,
                22 + strlen( paused1 ), Color( Mode ) );

      /*
       * get the user's response and restore the mode line.
       */
      if (g_status.command == Pause) {
         c = getkey( );
         show_modes( );
      }
   }
   return( c == ESC ? ERROR : OK );
}


/*
 * Name:    getkey_macro
 * Purpose: read a key from a macro or the keyboard
 * Author:  Jason Hood
 * Date:    July 26, 1998
 * Passed:  nothing
 * Returns: key read
 * Notes:   If no macro is executing, or its next function is Pause,
 *           or it is finished, read the key from the keyboard.
 *          Record the key unless it's from a macro.
 *          If the key read from the keyboard is the pause function,
 *           and it's record mode, record it, wait for another key,
 *           but don't record that.
 */
long getkey_macro( void )
{
long key = ERROR;
int  func;

   if (g_status.macro_executing &&
       g_status.macro_next < g_status.current_macro->len) {
      key = g_status.current_macro->key.keys[g_status.macro_next++];
      if (getfunc( key ) == Pause)
         key = ERROR;
   }

   if (key == ERROR) {
      key = getkey( );
      if (!g_status.macro_executing && mode.record) {
         func = getfunc( key );
         record_key( key, func );
         if (func == Pause)
            key = getkey( );
      }
   }

   return( key );
}


/*
 * Name:    set_break_point
 * Purpose: set or remove a break point
 * Author:  Jason Hood
 * Date:    August 15, 1998
 * Passed:  window:  pointer to current window
 * Notes:   if the break point is already set, either move it to the new
 *           line, or clear it.
 *          The break-point is used to stop recursive/infinite macros,
 *           or an infinite repeat.
 *
 * jmh 991006: moved from findrep.c, since searches no longer use it.
 */
int  set_break_point( TDE_WIN *window)
{
file_infos *file;

   if (window->ll->len == EOF)
      return( ERROR );

   file = window->file_info;
   file->break_point = (file->break_point == window->rline) ? 0 :
                       window->rline;
   file->dirty = GLOBAL;
   return( OK );
}
