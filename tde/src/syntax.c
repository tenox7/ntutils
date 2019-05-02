/*
 * Editor:      TDE, the Thomson-Davis Editor
 * Filename:    syntax.c
 * Author:      Jason Hood
 * Date:        August 30, 1997 to September 13
 *
 * All the syntax highlighting functions.
 *
 * My implementation of syntax highlighting is a hybrid of Elvis', by Steve
 * Kirkendall, and Rhide's, by Salvador E. Tropea and Robert Hoehne.
 *
 * There are four stages for a file to have syntax highlighting: locate the
 * language, parse it, set the initial flags, and finally display it. The
 * languages are in the "tde.shl" (_S_yntax _H_igh_L_ighting) file. If it can't
 * be found in the current directory, try HOME or the executable's directory
 * before giving up. The file is then parsed and syntatic features and keywords
 * (or just words to be highlighted) are noted. That's what I borrowed from
 * Elvis. When Elvis displays lines, it searches backwards to see if it's in a
 * comment or string. I didn't really like this idea, so I decided to use
 * Rhide's method of storing the type of each line. It's only required for the
 * multi-line stuff - everything else can be calculated on the fly. (The types
 * are explained in syntax_check()). To display lines requires re-calculating
 * the type of this line (since it's probably been modified), checking if that
 * would affect other lines (eg. starting a multi-line comment) and finally
 * displaying all the affected lines. The actual display is still done in
 * update_line(), but the checking is done here, as is determining what color
 * each character of the line will be (the syntax_attr() function).
 *
 * I was wondering how to handle C's 'l', 'u' and 'f' suffixes. Initially I was
 * going to handle it specially, along with Pascal and Ada's '#' character
 * (character constant for Pascal, based number for Ada). Later I realized that
 * PERL and Ada can have underscores in their numbers, so I added the "innumber"
 * command. It's not a perfect solution, but it's better to have invalid numbers
 * in the integer color, rather than valid numbers in bad.
 *
 * 980531: look for the syntax file in the HOME/executable directory and then
 *          the current directory. This allows languages to be defined globally
 *          with local add-ons.
 * 980716: made the language and keyword strings use FAR pointers; as a
 *          consequence use of my_{malloc,strcmp,free}.
 * 980813: Allow languages to "inherit" from another language; the current
 *          directory is again scanned first.
 * 990426: allocated the language name as part of the structure.
 * 011122: allow two of each comment type, using four characters maximum.
 */


#include "tdestr.h"     /* typedefs for global variables */
#include "common.h"
#include "define.h"     /* editor function defs */
#include "tdefunc.h"    /* prototypes for all functions in tde */
#include "syntax.h"     /* syntax highlighting definitions */
#include "config.h"     /* syntax highlighting file processing */

#define NO_LANGUAGE     -2      /* return codes for scan_syntax */
#define LANGUAGE_EXISTS  1

/*
 * Some globals for file parsing.
 */
static int  initialized = FALSE; /* have the colors been defined? */

static file_infos *syntax_file; /* file containing syntax highlighting */

static int  rescan = FALSE;     /* rescan an already loaded language */

extern char *line_in;           /* input line buffer */
extern char line_out[];         /* output line buffer */
extern int  line_no;            /* current line number */
static int  error_no = 0;       /* errors before this already displayed */
extern int  process_macro;
extern MACRO *mac;
extern TREE *branch;
extern TREE *cfg_key_tree;
static int  prompt_line;
static int  in_macro = FALSE;
extern int  background;
extern MENU_STR *user_menu;
extern void make_menu( MENU_STR *, long *, int );

static int  out_of_memory;      /* no room for keywords */

static void lang_defaults( syntax_info * );
static int  language_list( char * );
static void find_languages( char *, LIST **, int * );


/*
 * Name:    init_syntax
 * Purpose: see if the file has a recognized language; if so initialise
 *           the syntax highlighting.
 * Passed:  window: pointer to current window
 * Returns: OK for a recognized language;
 *          NO_LANGUAGE for no recognized language/pattern;
 *          ERROR otherwise.
 * Notes:   if g_option.language is NULL, test the filename for a pattern;
 *           if empty (ie "") then syntax highlighting is disabled;
 *           otherwise load the language specified. If it couldn't be found
 *           set g_option.language to NULL.
 *
 * 980526:  Modified to suit the changed syntax_select.
 * 031128:  Ignore non-file windows.
 */
int init_syntax( TDE_WIN *window )
{
LANGUAGE *ll;
char fname[PATH_MAX];           /* location of syntax highlighting file */
int  rc;

   if (g_option.language && *g_option.language == '\0')
      return( NO_LANGUAGE );

   syntax_file = window->file_info;
   if (!g_option.language && syntax_file->file_name[0] == '\0')
      return( NO_LANGUAGE );

   /*
    * See if the language has already been loaded.
    */
   sort.order_array = sort_order.ignore;
   if (g_option.language && !rescan) {
      for (ll = g_status.language_list; ll != NULL; ll = ll->next) {
         if (!my_strcmp( (text_ptr)g_option.language, (text_ptr)ll->name ))
            break;
      }
      if (ll != NULL) {
         syntax_file->syntax = ll;
         syntax_init_lines( syntax_file );
         window->syntax = TRUE;
         return( OK );
      }
   }

   prompt_line = window->bottom_line;
   out_of_memory = FALSE;

   rc = NO_LANGUAGE;
   if (file_exists( SYNTAXFILE ) == 0)
      rc = scan_syntax( SYNTAXFILE, g_option.language, TRUE );
   if (rc == NO_LANGUAGE) {
      join_strings( fname, g_status.argv[0], SYNTAXFILE );
      rc = scan_syntax( fname, g_option.language, FALSE );
   }
   
   if (rc == OK)
      syntax_init_lines( syntax_file );
   else if (rc == NO_LANGUAGE && g_option.language) {
      combine_strings( line_out, syntax13a, g_option.language, syntax13b );
      error( WARNING, prompt_line, line_out );
      if (g_option.language == g_option_all.language)
         g_option_all.language = NULL;
   }

   /*
    * Don't turn off syntax highlighting if the new language wasn't found.
    */
   if (!window->syntax)
      window->syntax = (rc == OK) ? TRUE : FALSE;

   return( rc );
}


/*
 * Name:    scan_syntax
 * Purpose: to determine if the file should have syntax highlighting
 * Passed:  name:     name of the syntax highlighting file
 *          language: language to find, or NULL to match filename pattern
 *          local:    is the syntax file in the current directory?
 * Returns: OK if the file has syntax highlighting;
 *          NO_LANGUAGE if the language couldn't be identified;
 *          ERROR otherwise (no memory).
 *
 * 980526:  Modified to suit the changed syntax_select.
 * 980815:  If the file is local, prepend the current directory to each pattern.
 */
int scan_syntax( char *name, char *language, int local )
{
FILE *fp;
char *line;                     /* pointer to the line read */
char lang[MAX_COLS+2];          /* buffer to hold the language found */
char parent[MAX_COLS+2];        /* buffer for the parent language */
char dir[PATH_MAX];             /* buffer for current directory/pattern */
char *pat;                      /* pointer into above for pattern */
int  key_no;                    /* index into key array */
register LANGUAGE *ll;          /* list of languages already loaded */
int  rc = NO_LANGUAGE;
int  load = TRUE;
unsigned old_line;
int  cnt;

   /*
    * Open the file in binary mode, since I seek in syntax_init_colors().
    */
   if ((fp = fopen( name, "rb" )) == NULL)
      return( NO_LANGUAGE );

   sort.order_array = sort_order.ignore;
   if (local) {
      get_current_directory( dir );
      if (!strcmp( dir, g_status.argv[0] )) {
         *dir  = '\0';
         local = FALSE;
      }
   } else
      *dir = '\0';
   pat = dir + strlen( dir );

   line_no = 0;
   process_macro = FALSE;
   while (rc == NO_LANGUAGE) {
      do
         key_no = syntax_read_line( fp, &line );
      while (key_no != SHL_LANGUAGE && key_no != ERROR);
      if (key_no == ERROR)
         break;
      line = parse_token( line, lang );
      if (language != NULL) {
         if (!my_strcmp( (text_ptr)lang, (text_ptr)language ))
            rc = OK;
         else
            continue;
      }
      /*
       * Search for the parent language
       */
      if (line != NULL) {
         /*
          * Skip the optional "from"
          */
         line = parse_token( line, parent );
         if (line != NULL)
            parse_token( line, parent );
      } else
         *parent = '\0';

      key_no = syntax_read_line( fp, &line );
      if (key_no == SHL_PATTERN) {
         while (rc == NO_LANGUAGE && line != NULL) {
            line = parse_token( line, pat );
            if (wildcard( dir, syntax_file->file_name ))
               rc = OK;
         }
      } else if (line_no > error_no) {
         ERRORLINE( syntax2, lang );    /* expecting pattern after language */
         error_no = line_no;
      }
   }
   if (rc == OK) {
      for (ll = g_status.language_list; ll != NULL; ll = ll->next) {
         if (!my_strcmp( (text_ptr)lang, (text_ptr)ll->name ))
            break;
      }
      if (ll == NULL) {
         if (*parent) {
            rescan = FALSE;
            old_line = line_no;
            rc = scan_syntax( name, parent, local );
            if (rc == NO_LANGUAGE && local) {
               join_strings( dir, g_status.argv[0], SYNTAXFILE );
               rc = scan_syntax( dir, parent, FALSE );
            }
            line_no = old_line;
            if (rc == OK) {
               ll = add_language( lang, syntax_file->syntax );
               if (ll == NULL)
                  load = FALSE;
               else
                  syntax_file->syntax = ll;
            } else if (rc == NO_LANGUAGE) {
               load = FALSE;
               combine_strings( line_out, syntax13a, parent, syntax13b );
               ERRORLINE( line_out, dir );
            }
         } else {
            if ((ll = add_language( lang, NULL )) == NULL) {
               rc   = ERROR;
               load = FALSE;
            } else
               syntax_file->syntax = ll;
         }
      } else {
         syntax_file->syntax = ll;
         load = rescan;
      }

      if (load) {
         cfg_key_tree = &ll->key_tree;
         user_menu = &ll->menu;
         if (user_menu->minor_cnt == 0  &&
             ll->parent && ll->parent->menu.minor_cnt != 0) {
            /*
             * Make a duplicate of the parent's menu.
             */
            cnt = user_menu->minor_cnt = ll->parent->menu.minor_cnt;
            cnt *= sizeof(MINOR_STR);
            user_menu->minor = malloc( cnt );
            if (user_menu->minor != NULL) {
               memcpy( user_menu->minor, ll->parent->menu.minor, cnt );
               user_menu->minor->line = NULL;
            }
         }
         /*
          * A new language is being loaded. If this is the first language
          * initialize all the colors.
          */
         if (!initialized) {
            syntax_init_colors( fp );
            initialized = TRUE;
         }
         if (ll->info->icase == MATCH)
            sort.order_array = sort_order.match;
         syntax_parse_file( fp, syntax_file->syntax );

         if (user_menu->minor_cnt != 0) {
            get_minor_counts( user_menu );
            make_menu( user_menu, NULL, 1 );
         }
      }
   }
   fclose( fp );
   return( rc );
}


/*
 * Name:    syntax_read_line
 * Purpose: read a line from the syntax file
 * Passed:  fp:   file to read
 *          line: pointer to the next token
 * Returns: the key number of the first token, or ERROR for eof or error.
 *          line points past the first token, or NULL for eof or error.
 * Notes:   global variable line_no is updated.
 *          blank lines and comments are skipped.
 *          unrecognized settings display a warning but continue.
 *          macros ignore lines until another valid syntax feature.
 *
 * 980526:  if the second last character is '\r', make it '\n'.
 * 990421:  recognize new color parsing.
 */
int  syntax_read_line( FILE *fp, char **line )
{
char token[1042];
int  key;
const char *errmsg;

   do {
      errmsg = NULL;
      do {
         ++line_no;
         if ((key = read_line( fp )) == EOF) {
            *line = NULL;
            return( ERROR );
         }
#if !defined( __UNIX__ )
         /*
          * need to remove the CR manually, since I'm reading as binary
          */
         if (line_in[key-1] == '\r')
            line_in[key-1] = '\0';
#endif
         *line = parse_token( line_in, token );
      } while (*token == '\0');

      if (process_macro)
         return( SHL_MACRO );

      if (*line == NULL)
         errmsg = config1;      /* setting without value */
      else {
         key = search( token, valid_syntax, SHL_NUM_FEATURES-1 );
         if (key != ERROR)
            in_macro = (key == SHL_MACRO);
         else if (!in_macro) {
            key = parse_key( token );
            if (key != ERROR) {
               if (key_func[KEY_IDX( key )] != TwoCharKey)
                  errmsg = syntax4;     /* syntax macro requires two-key */
               else {
                  key = SHL_MACRO;
                  in_macro = TRUE;
               }
            } else {
               *line = line_in;
               key = parse_color( line, token, prompt_line, syntax1 );
            }
         }
      }
      if (errmsg != NULL) {
         key = ERROR;
         if (!in_macro && line_no > error_no) {
            ERRORLINE( errmsg, token );
            error_no = line_no;
         }
      }
   } while (key == ERROR);

   return( key );
}


/*
 * Name:    add_language
 * Purpose: add a language to the list
 * Passed:  lang:   the language to add
 *          parent: parent language or NULL for no parent
 * Returns: pointer to new language on success; NULL on error (no memory)
 *
 * 980531:  added the defaults.
 */
LANGUAGE *add_language( char *lang, LANGUAGE *parent )
{
LANGUAGE *new_lang;
syntax_info *info = NULL;
int  rc  = OK;
int  len = strlen( lang );

   new_lang = (LANGUAGE *)calloc( 1, sizeof(LANGUAGE) + len );
   if (new_lang == NULL)
      rc = ERROR;
   else if (parent == NULL) {
      info = (syntax_info *)calloc( 1, sizeof(syntax_info) );
      if (info == NULL) {
         rc = ERROR;
         free( new_lang );
      } else
         new_lang->info = info;
   } else {
      new_lang->info   = parent->info;
      new_lang->parent = parent;
   }
   if (rc == OK) {
      new_lang->next = g_status.language_list;
      g_status.language_list = new_lang;
      memcpy( new_lang->name, lang, len + 1 );

      new_lang->key_tree.key = 0x10000L;
      
      /*
       * Use config setting for tab mode if this is not set.
       */
      new_lang->inflate_tabs = ERROR;

      if (info != NULL)
         lang_defaults( info );

   } else
      /*
       * out of memory
       */
      error( WARNING, prompt_line, main4 );

   return( rc == OK ) ? new_lang : NULL;
}


/*
 * Name:    lang_defaults
 * Purpose: set default language information
 * Author:  Jason Hood
 * Date:    December 2, 2003
 * Passed:  info:  information being set
 * Notes:   assumes info has been cleared on entry
 */
static void lang_defaults( syntax_info *info )
{
int  j;

   /*
    * Characters have only one character (plus the closing quote character).
    */
   info->charnum = 2;

   /*
    * Case sensitive.
    */
   info->icase = MATCH;

   /*
    * Identifiers can start with a letter and contain letters and digits.
    * Digits are set with their appropriate bases.
    */
   for (j = 0; j < 256; ++j) {
      if (bj_isalpha( j ))
         info->identifier[j] = ID_STARTWORD | ID_INWORD;
      else if (bj_isdigit( j ))
         info->identifier[j] = ID_INWORD | ID_DIGIT | ID_DECIMAL;
      if (bj_isxdigit( j ))
         info->identifier[j] |= ID_HEX | ID_DIGIT;
   }
   info->identifier[(int)'0'] |= ID_BINARY | ID_OCTAL;
   info->identifier[(int)'1'] |= ID_BINARY | ID_OCTAL;
   for (j = '2'; j < '8'; ++j)
      info->identifier[j] |= ID_OCTAL;
}


/*
 * Name:    syntax_parse_file
 * Purpose: grab all the syntax highlighting info for a language
 * Passed:  fp:     file pointer of syntax file
 *          syntax: place to store the info
 * Notes:   reads from the current position till the next "language" or eof.
 *          Some components are either setting it, or using it as a color for
 *           a list of words (eg. "comment //", "comment rem"). If the first
 *           character can start a word, assume the latter. This also implies
 *           that startword should be done before keywords.
 *
 * 980531:  moved the defaults to add_language.
 * 990426:  turn in_macro off.
 * 990503:  added the U macro for consistent array referencing.
 * 011122:  expanded comments to four characters and allow two of each type.
 * 020623:  fixed a bug in recognizing the second line comment.
 */
#define U( c ) (text_t)(c)
void syntax_parse_file( FILE *fp, LANGUAGE *syntax )
{
syntax_info *info = syntax->info;
char key[1042];                 /* buffer to hold any token that we parse */
char *residue;                  /* pointer to next item in line, if it exists */
int  key_no;                    /* index into key array */
char *number;                   /* pointer to one of the bases */
int  i, j;

   for (;;) {
      key_no = syntax_read_line( fp, &residue );
      if (key_no == SHL_LANGUAGE || key_no == ERROR)
         break;

      if (key_no == SHL_MACRO) {
         parse_line( line_in, prompt_line );
         while (process_macro) {
            if (syntax_read_line( fp, &residue ) == ERROR) {
               /*
                * Unexpected end of file.
                * Tidy up the macro definition.
                */
               check_macro( mac );
               error( WARNING, prompt_line, config26 );
            } else
               parse_macro( line_in, prompt_line );
         }
         in_macro = FALSE;
         continue;

      } else if (key_no == SHL_BACKGROUND) {
         key_no = parse_color( &residue, key, prompt_line, config6a );
         if (key_no != ERROR) {
            key_no >>= 4;
            background = key_no & 15;
         }
         continue;
      }

      residue = parse_token( residue, key );
      switch (key_no) {
         case SHL_PATTERN:
            /*
             * Setting is in the wrong place.
             */
            ERRORLINE( syntax5, key );
            break;

         case SHL_CASE:
            key_no = search( key, case_modes, 1 );
            if (key_no == ERROR) {
               ERRORLINE( syntax6, key ); /* case setting not recognized */
            } else {
               info->icase = key_no;
               sort.order_array = (key_no == MATCH) ? sort_order.match :
                                                      sort_order.ignore;
            }
            break;

         case SHL_ESCAPE:
            if (strlen( key ) != 1 || bj_isalnum( *key )) {
               ERRORLINE( syntax7, key ); /* expecting one non-alphanumeric */
            } else
               info->escape = *key;
            break;

         case SHL_STARTWORD:
         case SHL_INWORD:
            /*
             * Allow a range using '-', provided it's not at the beginning
             * or end of the key and the end-character is greater than the
             * start-character.
             */
            key_no = (key_no == SHL_STARTWORD) ? ID_STARTWORD :
                                                 ID_INWORD;
            while (*key) {
               for (i = 0; key[i]; ++i) {
                  if (key[i] == '-' && (i != 0 && key[i+1] != '\0') &&
                      U( key[i+1] ) > U( key[i-1] )) {
                     ++i;
                     for (j = U( key[i-2] ) + 1; j < U( key[i] ); ++j)
                        info->identifier[j] |= key_no;
                  }
                  info->identifier[U( key[i] )] |= key_no;
               }
               residue = parse_token( residue, key );
            }
            break;

         case SHL_INNUMBER:
            /*
             * Different from the above in that numbers are case insensitive,
             * so transform both cases of a letter.
             */
            key_no = ID_DIGIT | ID_BINARY | ID_OCTAL | ID_DECIMAL | ID_HEX;
            while (*key) {
               for (i = 0; key[i]; ++i) {
                  if (key[i] == '-' && (i != 0 && key[i+1] != '\0') &&
                      U( key[i+1] ) > U( key[i-1] )) {
                     ++i;
                     for (j = U( key[i-2] + 1 ); j < U( key[i] ); ++j) {
                        info->identifier[bj_tolower( j )] |= key_no;
                        info->identifier[bj_toupper( j )] |= key_no;
                     }
                  }
                  info->identifier[bj_tolower( key[i] )] |= key_no;
                  info->identifier[bj_toupper( key[i] )] |= key_no;
               }
               residue = parse_token( residue, key );
            }
            break;

         case SHL_COMMENT:
            if (info->identifier[U( *key )] & ID_STARTWORD)
               add_keyword_list( syntax, key_no, key, residue );
            else {
               if (strlen( key ) > 4) {
                  ERRORLINE( syntax8b, key ); /*only one to four chars allowed*/
               /*
                * comment has two forms: "comment one" or "comment one two".
                * The first is a line comment; the second is the start and end
                * of a multi-line comment.
                */
               } else {
                  char com[5];
                  strcpy( com, key );
                  parse_token( residue, key );
                  if (*key) {
                     if (strlen( key ) > 4) {
                        ERRORLINE( syntax8b, key );
                     } else if (info->comstart[1][0] != '\0') {
                        ERRORLINE( syntax14a, key );
                     } else {
                        int com_num = (info->comstart[0][0] != '\0');
                        strcpy( (char *)info->comstart[com_num], com );
                        strcpy( (char *)info->comend[com_num], key );
                     }
                  } else if (info->comment[1][0] != '\0') {
                     ERRORLINE( syntax14b, key );
                  } else {
                     int com_num = (info->comment[0][0] != '\0');
                     strcpy( (char *)info->comment[com_num], com );
                  }
               }
            }
            break;

         case SHL_FUNCTION:
            if (info->identifier[U( *key )] & ID_STARTWORD)
               add_keyword_list( syntax, key_no, key, residue );
            else {
               if (strlen( key ) > 1) {
                  ERRORLINE( syntax9, key );    /* only one char allowed */
               } else
                  info->function = *key;
            }
            break;

         case SHL_PREPRO:
            if (info->identifier[U( *key )] & ID_STARTWORD)
               add_keyword_list( syntax, key_no, key, residue );
            else {
               if (strlen( key ) > 2) {
                  ERRORLINE( syntax8a, key );
               } else {
                  info->prepro[0] = key[0];
                  info->prepro[1] = key[1];
                  residue = parse_token( residue, key );
                  if (*key && *key != ';') {
                     if (search( key, valid_string, 1 ) != SHL_SPANLINE) {
                        ERRORLINE( syntax5, key ); /* setting in wrong place */
                     } else {
                        parse_token( residue, key );
                        if (strlen( key ) != 1 || bj_isalnum( *key )) {
                           ERRORLINE( syntax7, key );
                        } else
                           info->prepspan = *key;
                     }
                  }
               }
            }
            break;

         case SHL_BINARY:
         case SHL_OCTAL:
         case SHL_HEX:
            if ((info->identifier[U( *key )] & ID_STARTWORD) &&
                (key[1] != '-' && key[2] != '-'))
               add_keyword_list( syntax, key_no, key, residue );
            else {
               if (key_no == SHL_BINARY)
                  number = (char *)info->binary;
               else if (key_no == SHL_OCTAL)
                  number = (char *)info->octal;
               else
                  number = (char *)info->hex;
               /*
                * The based numbers can have two forms: "base a[b]-" or
                * "base -a[b]". The former specifies a one- or two-character
                * prefix (eg. a[b]123); the latter is a suffix (123a[b]).
                */
               residue = key;
               if (residue[0] == '-') {
                  number[0] = SUFFIX;
                  ++residue;
               }
               number[1] = bj_tolower( residue[0] );
               if (residue[1] != '-') {
                  number[2] = bj_tolower( residue[1] );
                  ++residue;
               }
               if (residue[1] != '-' && number[0] != SUFFIX) {
                  ERRORLINE( syntax10, key );   /* requires prefix or suffix */
               } else if (residue[1] == '-') {
                  if (number[0] == SUFFIX) {
                     ERRORLINE( syntax11, key ); /* can't be prefix & suffix */
                  } else
                     number[0] = PREFIX;
               }
            }
            break;

         case SHL_STRING:
            if (info->identifier[U( *key )] & ID_STARTWORD)
               add_keyword_list( syntax, key_no, key, residue );
            else {
               if (strlen( key ) > 2) {
                  ERRORLINE( syntax8a, key );
               } else {
                  /*
                   * Strings can have three forms: "string a", "string ab" or
                   * "string a b". The first indicates that the open and close
                   * quote character are the same. The other two specify the
                   * quote characters individually. Strings can also have
                   * "newline" and "spanline a" (where "a" is non-alphanumeric)
                   * modifiers.
                   */
                  if (key[1] == '\0') {
                     info->strstart = key[0];
                     number = parse_token( residue, key );
                     if (key[0] != '\0' && key[1] == '\0') {
                        info->strend = key[0];
                        residue = number;
                     } else
                        info->strend = info->strstart;
                  } else {
                     info->strstart = key[0];
                     info->strend   = key[1];
                  }
                  for (;;) {
                     residue = parse_token( residue, key );
                     if (*key == '\0' || *key == ';')
                        break;
                     key_no = search( key, valid_string, 1 );
                     if (key_no == ERROR) {
                        ERRORLINE( syntax5, key );
                        continue;
                     }
                     if (key_no == SHL_NEWLINE)
                        info->strnewl = TRUE;
                     else {
                        residue = parse_token( residue, key );
                        if (strlen( key ) != 1 || bj_isalnum( *key )) {
                           ERRORLINE( syntax7, key );
                        } else
                           info->strspan = *key;
                     }
                  }
               }
            }
            break;

         case SHL_CHARACTER:
            if (info->identifier[U( *key )] & ID_STARTWORD)
               add_keyword_list( syntax, key_no, key, residue );
            else {
               if (strlen( key ) > 2) {
                  ERRORLINE( syntax8a, key );
               } else {
                  /*
                   * Characters are the same as strings, but they also have a
                   * length limit.
                   */
                  if (key[1] == '\0') {
                     info->charstart = key[0];
                     number = parse_token( residue, key );
                     if (*key && key[1] == '\0' && !bj_isdigit( *key ) ) {
                        info->charend = key[0];
                        residue = number;
                     } else
                        info->charend = info->charstart;
                  } else {
                     info->charstart = key[0];
                     info->charend   = key[1];
                  }
                  for (;;) {
                     residue = parse_token( residue, key );
                     if (*key == '\0' || *key == ';')
                        break;
                     if (bj_isdigit( *key )) {
                        info->charnum = atoi( key );
                        if (info->charnum)
                           ++info->charnum;   /* add one for closing quote */
                        continue;
                     }
                     key_no = search( key, valid_string, 1 );
                     if (key_no == ERROR) {
                        ERRORLINE( syntax5, key );
                        continue;
                     }
                     if (key_no == SHL_NEWLINE)
                        info->charnewl = TRUE;
                     else {
                        residue = parse_token( residue, key );
                        if (strlen( key ) != 1 || bj_isalnum( *key )) {
                           ERRORLINE( syntax7, key );
                        } else
                           info->charspan = *key;
                     }
                  }
               }
            }
            break;

         case SHL_MENU:
         {
            const char* errmsg = new_user_menu( residue, key, TRUE );
            if (errmsg != NULL) {
               ERRORLINE( errmsg, key );
            }
            break;
         }

         case SHL_INFLATETABS:
            key_no = search( key, valid_tabs, 2 );
            if (key_no == ERROR) {
               ERRORLINE( config16, key );
               continue;
            }
            syntax->inflate_tabs = key_no;
            break;

         case SHL_PTABSIZE :
         case SHL_LTABSIZE :
            i = atoi( key );
            if (i > MAX_TAB_SIZE || i < 1) {
               ERRORLINE( config8, key );
               continue;
            }
            if (key_no == SHL_PTABSIZE)
               syntax->ptab_size = i;
            else
               syntax->ltab_size = i;
            break;

         case SHL_KEYWORD:
         case SHL_NORMAL:
         case SHL_BAD:
         case SHL_SYMBOL:
         case SHL_INTEGER:
         case SHL_REAL:
         default:
            add_keyword_list( syntax, key_no, key, residue );
      }
   }
}
#undef U


/*
 * Name:    add_keyword_list
 * Purpose: add a line of keywords to the syntax highlighting info
 * Passed:  syntax:  syntax structure containing the keywords
 *          color:   color of the keywords
 *          key:     first keyword
 *          residue: rest of the keywords
 */
void add_keyword_list( LANGUAGE *syntax, int color, char *key, char *residue )
{
   if (color >= 256)
      color = syntax_color[color - 256];
   while (*key != '\0' && !out_of_memory) {
      add_keyword( syntax, key, color );
      residue = parse_token( residue, key );
   }
}


/*
 * Name:    add_keyword
 * Purpose: update the keyword list
 * Passed:  syntax: syntax structure containing the keywords
 *          key:    keyword
 *          color:  color of the keyword
 * Notes:   uses a hash table based on the first two characters.
 *          The first character is the color, the rest is the word itself.
 */
void add_keyword( LANGUAGE *syntax, char *key, int color )
{
text_ptr keyword;
int  hash;
text_ptr *hashed;
int  i;
int  len;
int  rc = OK;

   /*
    * Add the keyword if it doesn't already exist.
    */
   if ((keyword = is_keyword( syntax, key, FALSE )) == NULL) {
      len = strlen( key ) + 1;
      keyword = (text_ptr)my_malloc( len + 1, &rc );
      if (rc == OK) {
         my_memcpy( keyword+1, key, len );
         hash = KWHASH( key );
         /*
          * Create a bigger array.
          */
         if (syntax->keyword[hash]) {
            /*
             * Find the number of words already allocated.
             */
            for (i = 0; syntax->keyword[hash][i]; ++i) ;
            hashed = (text_ptr *)my_malloc( (i+2) * sizeof(text_ptr), &rc );
            if (rc == OK) {
               for (; i >= 0; --i)
                  hashed[i+1] = syntax->keyword[hash][i];
               my_free( syntax->keyword[hash] );
            }
         } else {
            /*
             * Start a new list.
             */
            hashed = (text_ptr *)my_malloc( 2 * sizeof(text_ptr), &rc );
            if (rc == OK)
               hashed[1] = NULL;
         }
         if (hashed == NULL)
            my_free( keyword );
         else {
            syntax->keyword[hash]    = hashed;
            syntax->keyword[hash][0] = keyword;
         }
      }
      if (rc == ERROR) {
         /*
          * out of memory
          */
         error( WARNING, prompt_line, main4 );
         out_of_memory = TRUE;
         return;
      }
   }
   keyword[0] = color;
}


/*
 * Name:    is_keyword
 * Purpose: determine if a string is a keyword
 * Passed:  syntax: pointer to the syntax info
 *          word:   string to test
 *          parent: TRUE to scan the parent language's keywords.
 * Returns: pointer to the keyword if it exists, NULL otherwise
 */
text_ptr is_keyword( LANGUAGE *syntax, char *word, int parent )
{
int hash;
int i;

   hash = KWHASH( word );
   if (syntax->keyword[hash] != NULL)
      for (i = 0; syntax->keyword[hash][i]; ++i)
         if (!my_strcmp( syntax->keyword[hash][i]+1, (text_ptr)word ))
            return( syntax->keyword[hash][i] );

   return( (parent && syntax->parent) ?
           is_keyword( syntax->parent, word, parent ) : NULL );
}


/*
 * Name:    syntax_init_colors
 * Purpose: set the colors for the syntax highlighting components
 * Passed:  fp: file pointer of syntax file
 * Notes:   goes back to the start of the file, reads the colors, and
 *           restores the file position.
 * 990421:  modified to use the new parse_color() function.
 */
void syntax_init_colors( FILE *fp )
{
long pos;                       /* current position in the file */
unsigned old_line;              /* current line number of the file */
char *color;
int  key;
char col[1042];
int  color_no;
const char *errmsg = NULL;

   pos = ftell( fp );
   rewind( fp );
   old_line = line_no;
   line_no  = 0;

   for (;;) {
      key = syntax_read_line( fp, &color );
      if (key == SHL_LANGUAGE)
         break;

      if ((key > SHL_SYMBOL || key < SHL_NORMAL) && key != SHL_BACKGROUND)
         errmsg = syntax3;
      else {
         color_no = parse_color( &color, col, prompt_line, config6a );
         if (color_no != ERROR) {
            if (key == SHL_BACKGROUND) {
               color_no >>= 4;
               background = color_no & 15;
            } else
               syntax_color[key - 256] = color_no;
         }
      }

      if (errmsg != NULL) {
         ERRORLINE( errmsg, col );
         errmsg = NULL;
      }
   }
   fseek( fp, pos, SEEK_SET );
   line_no = old_line;
}


/*
 * Name:    syntax_init_lines
 * Purpose: set the initial line flags for syntax highlighting
 * Passed:  file: pointer to the file to be initialized
 *
 * 060915:  set the tab settings.
 */
void syntax_init_lines( file_infos *file )
{
syntax_info *syntax;
line_list_ptr ll;

   /*
    * No point doing this if the file doesn't have highlighting.
    */
   if (file->syntax != NULL) {
      syntax = file->syntax->info;
      ll     = file->line_list->next;
      while (ll->len != EOF) {
         syntax_check( ll, syntax );
         ll = ll->next;
      }

      if (g_option.tab_size == -1) {
         if (file->syntax->inflate_tabs != ERROR)
            file->inflate_tabs = file->syntax->inflate_tabs;
         if (file->syntax->ptab_size)
            file->ptab_size = file->syntax->ptab_size;
      }
      if (file->syntax->ltab_size)
         file->ltab_size = file->syntax->ltab_size;
   }
}


/*
 * Name:    match_comment
 * Purpose: determine if the line contains a comment
 * Author:  Jason Hood
 * Date:    November 23, 2001
 * Passed:  comment:  comment to test
 *          line:     line to test against
 *          pos:      position in line to start test
 *          len:      length of line
 * Returns: length of the comment, or 0 if no match
 * Notes:   comment should exist (ie. be at least one character).
 */
static int  match_comment( text_ptr comment, text_ptr line, int pos, int len )
{
text_ptr start = comment;

   line += pos;
   len -= pos;
   while (*comment == *line++  &&  *++comment  &&  --len);
   if (*comment == '\0')
      return( (int)(comment - start) );
   return( 0 );
}


/*
 * Name:    syntax_check
 * Purpose: determine the syntax highlighting flags for a line
 * Passed:  line:   line to test
 *          syntax: language to test against
 * Notes:   The previous line must have the right flags.
 *
 * This function is used to determine what type of line a line is. It enables
 * us to jump to any part of the file, without the need to backtrack to see if
 * we're in a comment or string, like Elvis does. As such, it's only required
 * for the stuff that uses multiple lines, namely comments, strings, characters
 * and some preprocessors (like C). However, if these start and finish in the
 * one line, there's no need to type them, since they have no effect on
 * subsequent lines.
 *
 * Comments (COM), strings (STR) and characters (CHR) consist of three
 * components: _START, _WHOLE and _END. _START means it begins somewhere on the
 * line and extends to the end of the line; _WHOLE means the entire line is
 * occupied; _END means it continues from the beginning of the line, but stops
 * somewhere. _START and _WHOLE are used to "inherit" the new line type from
 * the previous line. _WHOLE and _END are used to indicate that this line is
 * still (partly) a comment, string or character, for display purposes (ie.
 * syntax_attr()) - which is the whole point of doing this typing.
 *
 * Strings and characters also have START_VALID, WHOLE_VALID and END_VALID
 * values, which indicates if it is actually legally allowed to span multiple
 * lines. (Note that they always do span lines, they never stop at eol, like
 * some versions of BASIC allowed.)
 *
 * Preprocessor is a little different, since it always occupies the whole line,
 * but can still have the above components. So it has PREPRO to indicate that
 * the line is a preprocessor, and _START and _END components to indicate the
 * first and last lines of the preprocessor. The new line will be PREPRO if the
 * last line was PREPRO, but not PREPRO_END. PREPRO_START is used in
 * syntax_attr() to indicate that we should still test for preprocessor, so
 * leading blanks will be done in normal, and preprocessor will be recognized
 * before comment (with Pascal's {$ and shell scripts' #!, for example).
 *
 * jmh 030411: fixed bug with spanning (this wasn't set correctly).
 */
void syntax_check( line_list_ptr line, syntax_info *syntax )
{
text_ptr ll;
int  len;
long old_type;          /* type of the previous line */
long type;              /* type of this line */
int  in_comment;
int  in_string;
int  in_character;
int  in_prepro;
text_t this, next;
int  com_len, com_num;
long com_bit;
int  i;

   if (g_status.copied && line == g_status.buff_node) {
      ll  = g_status.line_buff;
      len = g_status.line_buff_len;
   } else {
      ll  = line->line;
      len = line->len;
   }
   old_type     = line->prev->type;
   in_prepro    = (old_type & PREPRO) && !(old_type & PREPRO_END);
   in_comment   = ((old_type & (COM_START | COM_WHOLE)) != 0);
   in_string    = ((old_type & (STR_START | STR_WHOLE)) != 0);
   in_character = ((old_type & (CHR_START | CHR_WHOLE)) != 0);
   type         = line->type & DIRTY; /* keep the dirty flag, clear the rest */

   /*
    * If this line continues the preprocessor then indicate as such.
    */
   if (in_prepro)
      type |= PREPRO;

   i = 0;
   /*
    * Skip leading blanks and test for preprocessor.
    */
   if (!in_prepro && !in_comment && !in_string && !in_character) {
      for (; i < len && bj_isspace( ll[i] ); ++i) ;
      this = (i   < len) ? ll[i]   : '\0';
      next = (i+1 < len) ? ll[i+1] : '\0';
      if (  syntax->prepro[0] && this == syntax->prepro[0] &&
          (!syntax->prepro[1] || next == syntax->prepro[1])) {
         type |= PREPRO | PREPRO_START;
         in_prepro = TRUE;
         ++i;
         if (syntax->prepro[1])
            ++i;
      }
   }

   com_bit = old_type & COM_NUMBER;
   com_num = (com_bit != 0);

   for (; i < len; ++i) {
      while (bj_isspace( ll[i] ) && ++i < len);
      if (i == len)
         break;
      this = ll[i];
      next = (i+1 < len) ? ll[i+1] : '\0';

      /*
       * Search for the end of the comment.
       */
      if (in_comment) {
         com_len = match_comment( syntax->comend[com_num], ll, i, len );
         if (com_len) {
            if (type & COM_START)
               type &= ~(COM_START | COM_NUMBER);
            else
               type |= COM_END | com_bit;
            in_comment = FALSE;
            i += com_len - 1;
         }
         continue;
      }

      /*
       * Search for the end of the string.
       */
      if (in_string) {
         if (syntax->escape && this == syntax->escape)
            ++i;
         else if (this == syntax->strend) {
            if (type & STR_START)
               type &= ~STR_START;
            else {
               type |= STR_END;
               if (old_type & (START_VALID | WHOLE_VALID))
                  type |= END_VALID;
            }
            in_string = FALSE;
         }
         continue;
      }

      /*
       * Search for the end of a character literal/string.
       */
      if (in_character) {
         if (syntax->escape && this == syntax->escape)
            ++i;
         else if (this == syntax->charend) {
            if (type & CHR_START)
               type &= ~CHR_START;
            else {
               type |= CHR_END;
               if (old_type & (START_VALID | WHOLE_VALID))
                  type |= END_VALID;
            }
            in_character = FALSE;
         }
         continue;
      }

      /*
       * If it's a line comment, the rest of the line can be ignored.
       */
      if (syntax->comment[0][0]) {
         if (match_comment( syntax->comment[0], ll, i, len ))
            break;
         if (syntax->comment[1][0]) {
            if (match_comment( syntax->comment[1], ll, i, len ))
               break;
         }
      }

      /*
       * Start of a multi-line comment?
       */
      if (syntax->comstart[0][0]) {
         com_num = 0;
         com_bit = 0;
         com_len = match_comment( syntax->comstart[0], ll, i, len );
         if (!com_len  &&  syntax->comstart[1][0]) {
            com_num = 1;
            com_bit = COM_NUMBER;
            com_len = match_comment( syntax->comstart[1], ll, i, len );
         }
         if (com_len) {
            type |= COM_START | com_bit;
            in_comment = TRUE;
            i += com_len - 1;
            continue;
         }
      }

      /*
       * Start of a string?
       */
      if (syntax->strstart && this == syntax->strstart) {
         type |= STR_START;
         in_string = TRUE;
         continue;
      }

      /*
       * Start of a character literal/string?
       */
      if (syntax->charstart && this == syntax->charstart) {
         type |= CHR_START;
         in_character = TRUE;
         if (syntax->charnum) {
            ++i;
            if (syntax->escape && next == syntax->escape)
               ++i;
         }
         continue;
      }

      /*
       * Skip all characters in identifiers and numbers. This is primarily so
       * PERL's package naming will be correctly handled (eg. main'dumpvar is
       * one word, not a word and the start of a string).
       */
      if (syntax->identifier[this] & (ID_STARTWORD | ID_DIGIT)) {
         while (++i < len &&
                (syntax->identifier[ll[i]] & (ID_INWORD | ID_DIGIT)));
         --i;
      }
   }

   /*
    * If we finish in a comment/string/character, and it didn't start on this
    * line, then it must occupy the whole line.
    */
   if (in_comment) {
      if (!(type & COM_START))
         type |= COM_WHOLE | com_bit;
   } else if (in_string) {
      if (!(type & STR_START))
         type |= STR_WHOLE;
   } else if (in_character) {
      if (!(type & CHR_START))
         type |= CHR_WHOLE;
   }

   /*
    * Determine the validity of a spanning string/character.
    */
   this = (len) ? ll[len-1] : '\0';
   if ((in_string &&
       (syntax->strnewl || (syntax->strspan && this == syntax->strspan))) ||
       (in_character &&
       (syntax->charnewl || (syntax->charspan && this == syntax->charspan)))) {
      if (type & (STR_START | CHR_START))
         type |= START_VALID;
      else if (old_type & (START_VALID | WHOLE_VALID))
         type |= WHOLE_VALID;
   }

   /*
    * Does the preprocessor stop here?
    */
   if (in_prepro && !in_string && !in_character)
      if (in_comment || !syntax->prepspan || this != syntax->prepspan)
         type |= PREPRO_END;

   line->type = type;
}


/*
 * Name:    syntax_check_lines
 * Purpose: update a line's syntax flags and see if the change propagates
 * Passed:  line:   first line to check
 *          syntax: info to check against
 * Returns: the number of lines after this one that were changed.
 * Notes:   propagation occurs when something affects following lines, such as
 *           adding an opening multi-line comment.
 * jmh 980728: Changed return type to long from int.
 * jmh 980729: Test for eof.
 */
long syntax_check_lines( line_list_ptr line, LANGUAGE *syntax )
{
syntax_info *info;
long old_type, new_type;
long count = -1;                        /* Don't count the first line */

   if (syntax == NULL || line == NULL || line->len == EOF)
      return 0;

   info = syntax->info;
   do {
      old_type = line->type | DIRTY;    /* DIRTY is or'ed for the comparison */
      syntax_check( line, info );
      new_type = line->type | DIRTY;
      ++count;
      line = line->next;
   } while (new_type != old_type  &&  line->len != EOF);

   return( count );
}


/*
 * Name:    syntax_check_block
 * Purpose: check a number of lines
 * Author:  Jason Hood
 * Date:    July 29, 1998
 * Passed:  br:     number of first line to check
 *          er:     number of last line
 *          line:   pointer to first line
 *          syntax: info to check against
 * Returns: none.
 * Notes:   Checks lines modified by block functions.
 *          br and er need not be actual line numbers, just a line range.
 *
 * jmh 031027: test for EOF.
 */
void syntax_check_block( long br, long er, line_list_ptr line,
                         LANGUAGE *syntax )
{
syntax_info *info;

   if (syntax != NULL) {
      info = syntax->info;
      for (; br <= er  &&  line->len != EOF; ++br) {
         syntax_check( line, info );
         line = line->next;
      }
      if (line->len != EOF)
         syntax_check_lines( line, syntax );
   }
}


/*
 * Name:    syntax_attr
 * Purpose: determine the attributes (ie. colors) of a line
 * Passed:  line:   the line under consideration
 *          attr:   buffer to hold the attributes
 *          syntax: syntax highlighting info
 * Returns: attribute to use for the trailing blanks
 * Notes:   used by update_line().
 *          attr should be at least as long as the line.
 *          The line should be detabbed.
 *          I've assumed that characters that can span lines don't have a
 *           length limit. Characters that have a limit must have at least one
 *           character. ie. the empty character string is not allowed (eg. '').
 *           This allows Ada's apostrophe "'''" to work correctly.
 *          In rare circumstances, strings and characters that use escapes and
 *           span lines will fail. Eg. with C, \ is used to quote the next
 *           character and to continue the string on the next line. A string
 *           that ends like "string end \\" (where the terminating quote is
 *           actually eol) can be treated in two ways: a quoted \, which causes
 *           an unterminated string (BC3.1 does this, which is what I would
 *           expect); or as a continuing string, where the character to be
 *           quoted starts on the next line (this is what gcc 2.7.2.1 does).
 *           For simplicity, I do the latter. (Actually, gcc allows all lines
 *           to have continuing backslashes - including line comments.)
 *          The character limit could fail with the \nnn & \xnn escapes.
 *           Eg. if the limit is 2 (C, with 16-bit ints) then the legitimate
 *           character literal '\377' will register as 3 characters. (The
 *           supplied C and C++ limits are set to 4.)
 *          Everything in a preprocessor is in that color, except comments and
 *           bad items (currently strings, characters and numbers).
 *          attr should really be int (to be consistent), but char is a lot
 *           easier (in update_line()), and allows memset() to be used.
 *
 *          Note that _START can't be used to memset everything from here to
 *          the end, since there's no guarantee that this is the one to which
 *          the _START applies. eg:
 *
 *              "string"   "multi-line
 *
 *          If STR_START is used to memset and return, then it would do so at
 *          the `"string"' part, not the `"multi-line' part.
 *          Ask me how I know this. :)
 */
int syntax_attr( text_ptr text, int len, long type,
                 unsigned char *attr, LANGUAGE *syntax )
{
syntax_info *info;
int  i;
int  col = syntax_color[COL_NORMAL];
int  in_comment;
int  in_string;
int  in_character;
int  in_prepro;
int  start = -1;        /* position where something started */
int  count = 0;         /* number of characters in that something */
int  char_count;        /* are we counting characters? */
char word[1042];        /* Store a possible keyword */
text_ptr keyword;
int  base;              /* base for numbers */
unsigned char this, next;
int  com_len, com_num = 0;
long com_bit;

   info = syntax->info;
   sort.order_array = (info->icase == MATCH) ? sort_order.match :
                                               sort_order.ignore;

   in_prepro    = (type & PREPRO) && !(type & PREPRO_START);
   in_comment   = ((type & (COM_WHOLE | COM_END)) != 0);
   in_string    = ((type & (STR_WHOLE | STR_END)) != 0);
   in_character = ((type & (CHR_WHOLE | CHR_END)) != 0);

   if (in_prepro)
      col = syntax_color[COL_PREPRO];

   if (in_comment) {
      col = syntax_color[COL_COMMENT];
      com_bit = type & COM_NUMBER;
      com_num = (com_bit != 0);

   } else if (in_string || in_character) {
      if (type & (WHOLE_VALID | END_VALID)) {
         if (!in_prepro)
            col = syntax_color[in_string ? COL_STRING : COL_CHARACTER];
      } else
         col = syntax_color[COL_BAD];
   }

   if (type & (COM_WHOLE | STR_WHOLE | CHR_WHOLE)) {
      memset( attr, col, len );
      return( col );
   }

   char_count = (info->charnum && !(type & (CHR_WHOLE | CHR_END)));
   i = 0;
   /*
    * Skip leading blanks and test for preprocessor.
    */
   if (!in_prepro && !in_comment && !in_string && !in_character) {
      for (; i < len && bj_isspace( text[i] ); ++i)
         attr[i] = col;
      this = (i   < len) ? text[i]   : '\0';
      next = (i+1 < len) ? text[i+1] : '\0';
      if (  info->prepro[0] && this == info->prepro[0] &&
          (!info->prepro[1] || next == info->prepro[1])) {
         col = syntax_color[COL_PREPRO];
         attr[i++] = col;
         if (info->prepro[1])
            attr[i++] = col;
         in_prepro = TRUE;
      }
   }
   for (; i < len; ++i) {
      while (i < len && bj_isspace( text[i] ))
         attr[i++] = col;
      if (i == len)
         break;
      this = text[i];
      next = (i+1 < len) ? text[i+1] : '\0';

      /*
       * Search for the end of the comment.
       */
      if (in_comment) {
         attr[i] = col;
         com_len = match_comment( info->comend[com_num], text, i, len );
         if (com_len) {
            while (--com_len != 0)
               attr[++i] = col;
            in_comment = FALSE;
            col = syntax_color[(in_prepro) ? COL_PREPRO : COL_NORMAL];
         }
         continue;
      }

      /*
       * Search for the end of the string.
       */
      if (in_string) {
         attr[i] = col;
         if (info->escape && this == info->escape)
            attr[++i] = col;
         else if (this == info->strend) {
            in_string = FALSE;
            col = syntax_color[(in_prepro) ? COL_PREPRO : COL_NORMAL];
         }
         continue;
      }

      /*
       * Search for the end of a character literal/string.
       */
      if (in_character) {
         if (char_count && ++count > info->charnum) {
            col = syntax_color[COL_BAD];
            memset( attr+start, col, i - start );
         }
         attr[i] = col;
         if (info->escape && this == info->escape)
            attr[++i] = col;
         else if (this == info->charend) {
            in_character = FALSE;
            col = syntax_color[(in_prepro) ? COL_PREPRO : COL_NORMAL];
         }
         continue;
      }

      /*
       * If it's a line comment, everything from here is a comment.
       */
      if (info->comment[0][0]) {
         if (match_comment( info->comment[0], text, i, len )  ||
             (info->comment[1][0] &&
              match_comment( info->comment[1], text, i, len ))) {
            col = syntax_color[COL_COMMENT];
            memset( attr+i, col, len - i );
            return( col );
         }
      }

      /*
       * Start of a multi-line comment?
       */
      if (info->comstart[0][0]) {
         com_num = 0;
         com_bit = 0;
         com_len = match_comment( info->comstart[0], text, i, len );
         if (!com_len  &&  info->comstart[1][0]) {
            com_num = 1;
            com_bit = COM_NUMBER;
            com_len = match_comment( info->comstart[1], text, i, len );
         }
         if (com_len) {
            col = syntax_color[COL_COMMENT];
            in_comment = TRUE;
            --i;
            do {
               attr[++i] = col;
            } while (--com_len != 0);
            continue;
         }
      }

      /*
       * Start of a string?
       */
      if (info->strstart && this == info->strstart) {
         if (!in_prepro)
            col    = syntax_color[COL_STRING];
         attr[i]   = col;
         in_string = TRUE;
         start     = i;
         continue;
      }

      /*
       * Start of a character literal/string?
       */
      if (info->charstart && this == info->charstart) {
         if (!in_prepro)
            col       = syntax_color[COL_CHARACTER];
         attr[i]      = col;
         in_character = TRUE;
         start        = i;
         if (char_count) {
            count = 1;
            if (++i < len) {
               attr[i] = col;
               if (info->escape && text[i] == info->escape)
                  attr[++i] = col;
            }
         }
         continue;
      }

      /*
       * Check for a keyword or function
       */
      if (info->identifier[this] & ID_STARTWORD) {
         start = i;
         count = 0;
         do
            word[count++] = text[i++];
         while (i < len && (info->identifier[text[i]] & ID_INWORD));
         word[count] = '\0';
         if (!in_prepro)
            keyword = is_keyword( syntax, word, TRUE );
         else
            keyword = NULL;
         /*
          * A keyword has been found.
          * jmh 980814: allow color 0 to remove inherited keywords.
          */
         if (keyword != NULL && *keyword != 0)
            col = *keyword;

         /*
          * Check for a function.
          */
         else {
            while (i < len && bj_isspace( text[i] ))
               ++i, ++count;
            if (!in_prepro && info->function &&
                i < len && text[i] == info->function)
               col = syntax_color[COL_FUNCTION];
         }

         memset( attr+start, col, count );
         col = syntax_color[(in_prepro) ? COL_PREPRO : COL_NORMAL];
         --i;
         continue;
      }

      this = bj_tolower( this );        /* case insensitive for numbers */
      next = bj_tolower( next );
      base = 0;
      /*
       * Test for a binary prefix.
       */
      if (info->binary[0] == PREFIX && this == info->binary[1] &&
                  (!info->binary[2] || next == info->binary[2])) {
         base  = ID_BINARY;
         col   = syntax_color[COL_BINARY];
         start = i++;
         if (info->binary[2])
            ++i;

      /*
       * Test for a hexadecimal prefix. This is before octal so C can recog-
       * nize "0x" before "0".
       */
      } else if (info->hex[0] == PREFIX && this == info->hex[1] &&
                         (!info->hex[2] || next == info->hex[2])) {
         base  = ID_HEX;
         col   = syntax_color[COL_HEX];
         start = i++;
         if (info->hex[2])
            ++i;

      /*
       * Test for an octal prefix.
       */
      } else if (info->octal[0] == PREFIX && this == info->octal[1] &&
                         (!info->octal[2] || next == info->octal[2])) {
         base  = ID_OCTAL;
         col   = syntax_color[COL_OCTAL];
         start = i++;
         if (info->octal[2])
            ++i;

      /*
       * Try all the bases, see if there's a suffix.
       */
      } else if (bj_isdigit( this )) {
         base  = ID_DIGIT;
         col   = syntax_color[COL_INTEGER];
         start = i;
      }

      /*
       * Now verify the number.
       * jmh 991026: ignore exponents if 'e' is in_number (ie. binary).
       */
      if (base) {
      test_real:
         while (i < len && (info->identifier[this = text[i]] & base)) {
            if (base & ID_DIGIT) {
               if ((this == 'e' || this == 'E')  &&
                   !(info->identifier[this] & ID_BINARY)) {
                  if (i+1 < len && (text[i+1] == '-' || text[i+1] == '+'))
                     ++i;
                  if (base & ID_EXPONENT)
                     col = 0;
                  else
                     base |= ID_EXPONENT;
               } else if (info->identifier[this] & ID_BINARY)
                  base |= ID_BINARY;
               else if (info->identifier[this] & ID_OCTAL)
                  base |= ID_OCTAL;
               else if (info->identifier[this] & ID_DECIMAL)
                  base |= ID_DECIMAL;
               else if (info->identifier[this] & ID_HEX)
                  base |= ID_HEX;
            }
            ++i;
         }
         /*
          * Test for a decimal point or exponent. I've complicated the point
          * test somewhat, because two points in a row makes a symbol (eg.
          * Pascal's range). I've used the next character rather than the
          * previous so I can keep the real part. If I have 2.3..4 using a
          * previous test, the first point turns on real, the second point
          * makes it bad, the third point turns off the second point, which
          * would also turn off the first point, causing the number to be an
          * integer. Using the next character avoids this, at the expense of a
          * funny-looking condition. If the next character is a point then it's
          * a symbol, but if there is no next character, it's a point.
          */
         if (i < len &&
             ((this == '.' && !(i+1 < len && text[i+1] == '.')) ||
              this == 'e' || this == 'E')) {
            if (this == '.') {
               if (base & ID_REAL)
                  col = 0;
               else
                  /*
                   * Add the decimal numbers to get around that damn annoying
                   * leading octal 0, which makes 0.89 bad.
                   */
                  base |= ID_POINT | ID_DECIMAL;
            } else {
               if (i+1 < len && (text[i+1] == '-' || text[i+1] == '+'))
                  ++i;
               if (base & ID_EXPONENT)
                  col = 0;
               else
                  base |= ID_EXPONENT;
            }
            ++i;
            goto test_real;
         }
         /*
          * If I haven't got a prefix, test for a suffix.
          */
         if (base & ID_DIGIT) {
            int num_start = i;
            /*
             * Go back at most two characters to test for suffixes. This allows
             * assembly's binary "b" suffix, rather than hex's "b" digit. eg.
             * "100b" is good binary, not bad hex. It's not perfect, as it will
             * allow other hex digits - "123b" is bad (decimal digits), but
             * "1ab" is good, because I turn off the hex digit test so "100b"
             * will be recognized as good. Sigh.
             * 991026: "1ba" is good if radix 16 is used, so only test two
             *         characters if necessary.
             */
            count = 2;
         test_suffix:   /* Dear, oh dear, two gotos in the one function. */
            this = (i   < len) ? bj_tolower( text[i]   ) : '\0';
            next = (i+1 < len) ? bj_tolower( text[i+1] ) : '\0';
            /*
             * Test for a binary suffix.
             */
            if (info->binary[0] == SUFFIX && (info->binary[2] || count > 0) &&
                                     this == info->binary[1] &&
                (!info->binary[2] || next == info->binary[2])) {
               ++i;
               if (info->binary[2])
                  ++i;
               if ((base & (ID_OCTAL | ID_DECIMAL)) ||
                   ((base & ID_HEX) && count == 2))
                  col = 0;
               else
                  col = syntax_color[COL_BINARY];

            /*
             * Test for an octal suffix.
             */
            } else if (info->octal[0] == SUFFIX &&
                       (info->octal[2] || count > 0) &&
                                           this == info->octal[1] &&
                       (!info->octal[2] || next == info->octal[2])) {
               ++i;
               if (info->octal[2])
                  ++i;
               if ((base & ID_DECIMAL) || ((base & ID_HEX) && count == 2))
                  col = 0;
               else
                  col = syntax_color[COL_OCTAL];

            /*
             * Test for a hexadecimal suffix.
             */
            } else if (info->hex[0] == SUFFIX && (info->hex[2] || count > 0) &&
                                         this == info->hex[1] &&
                       (!info->hex[2] || next == info->hex[2])) {
               ++i;
               if (info->hex[2])
                  ++i;
               /*
                * There's no base greater than hex, so don't bother testing it.
                */
               col = syntax_color[COL_HEX];

            /*
             * There's no suffix and no prefix, so there are two possibilities:
             * it's a bad number; or the suffix was treated as a hex digit. Go
             * back a character and try again.
             */
            } else {
               if (!bj_isdigit( text[i-1] ) && count > 0) {
                  --count;
                  --i;
                  goto test_suffix;
               }
               /*
                * I still didn't recognize anything, so go back to where I was.
                */
               i = num_start;
               /*
                * Now I know there is no prefix and no suffix. If there were
                * hex digits, it's a bad number.
                */
               if (base & ID_HEX)
                  col = 0;
            }

         /*
          * If I have a prefix without any following digits, it's bad. Unless
          * that prefix is a one-character digit (damn C's prefixes).
          */
         } else {
            if (i-1 == start) {         /* a one-character prefix */
               if (((base & ID_BINARY) && bj_isdigit( info->binary[1] )) ||
                   ((base & ID_OCTAL ) && bj_isdigit( info->octal[1] )) ||
                   ((base & ID_HEX   ) && bj_isdigit( info->hex[1] )))
                  col = syntax_color[COL_INTEGER];
               else
                  col = 0;
            /*
             * Either a one-character prefix with one digit, or a two-character
             * prefix with no digits. Let's assume that two-character prefixes
             * are not both digits.
             */
            } else if (i-2 == start) {
               if (((base & ID_BINARY) && info->binary[2] ) ||
                   ((base & ID_OCTAL ) && info->octal[2] ) ||
                   ((base & ID_HEX   ) && info->hex[2] ))
                  col = 0;
            }
         }

         /*
          * If the number is bad, leave it that way, but if it is real, use that
          * color instead of the base color, unless the exponent has no digits.
          */
         if (col && (base & ID_REAL)) {
            if (text[i-1] == 'e' || text[i-1] == 'E' ||
                text[i-1] == '-' || text[i-1] == '+')
               col = 0;
            else
               col = syntax_color[COL_REAL];
         }
         if (col == 0 ||
             (i < len && (info->identifier[text[i]] & ID_INWORD))) {
            col = syntax_color[COL_BAD];
            while (i < len && (info->identifier[text[i]] & ID_INWORD))
               ++i;

         } else if (in_prepro)
            col = syntax_color[COL_PREPRO];

         memset( attr+start, col, i - start );
         col = syntax_color[(in_prepro) ? COL_PREPRO : COL_NORMAL];
         --i;
         continue;
      }

      attr[i] = (in_prepro) ? col : syntax_color[COL_SYMBOL];
   }

   /*
    * If we finished in a string/character, see if it is bad.
    */
   if (in_string || in_character)
      if (!(type & START_VALID)) {      /* WHOLE_VALID was done at the start */
         col = syntax_color[COL_BAD];
         memset( attr+start, col, len - start );
      }

   return( (in_comment || in_string || in_character || in_prepro) ? col :
           syntax_color[COL_NORMAL] );
}


/*
 * Name:    syntax_toggle
 * Purpose: to turn syntax highlighting on or off
 * Passed:  window: pointer to current window
 *
 * 980728:  display window rather than redraw.
 */
int  syntax_toggle( TDE_WIN *window )
{
   if (window->file_info->syntax != NULL) {
      window->syntax = !window->syntax;
      display_current_window( window );
   }
   return( OK );
}


/*
 * Name:    syntax_select
 * Purpose: select a language for syntax highlighting
 * Passed:  window: pointer to current window
 * Notes:   affects all windows pointing to this window's file
 *
 * 980526:  If the new language is the same as the current, reload it.
 *          Show the available memory.
 * 980531:  Use the current language as the default.
 * 060913:  Bring up a list if the answer is empty.
 */
int  syntax_select( TDE_WIN *window )
{
char answer[MAX_COLS+2];
LANGUAGE *old_syntax;
int  prompt_line;
int  i, j;
int  rc;

   prompt_line = window->bottom_line;

   old_syntax = window->file_info->syntax;
   if (old_syntax == NULL)
      answer[0] = '\0';
   else
      strcpy( answer, old_syntax->name );
      
   /*
    * Syntax highlighting language:
    */
   rc = get_name( syntax12, prompt_line, answer, &h_lang );
   if (rc == 0)
      rc = language_list( answer );
   if (rc >= 0) {
      g_option.language = answer;
      sort.order_array = sort_order.ignore;
      if (old_syntax != NULL &&
         my_strcmp( (text_ptr)old_syntax->name, (text_ptr)answer ) == 0) {
         /*
          * The same language is being requested, so reload the colors and
          * remove the keywords.
          * 011123: reset the comments.
          * 020821: don't reset the comments of inherited languages.
          * 031202: reset all information to defaults.
          */
         rescan      = TRUE;
         initialized = FALSE;
         if (old_syntax->parent == NULL) {
            memset( old_syntax->info, 0, sizeof(syntax_info) );
            lang_defaults( old_syntax->info );
         }
         my_free_group( TRUE );
         for (i = 0; i < 128; ++i) {
            if (old_syntax->keyword[i]) {
               for (j = 0; old_syntax->keyword[i][j]; ++j)
                  my_free( old_syntax->keyword[i][j] );
               my_free( old_syntax->keyword[i] );
               old_syntax->keyword[i] = NULL;
            }
         }
         my_free_group( FALSE );
      } else
         rescan = FALSE;

      if (init_syntax( window ) == OK) {
         if (rescan) {
            redraw_screen( window );
            rescan = FALSE;
         } else
            window->file_info->dirty = GLOBAL;
         show_avail_mem( );
      }
   }
   return( OK );
}


/*
 * Name:    my_strcmp
 * Purpose: compare two NUL-terminated strings
 * Author:  Jason Hood
 * Date:    July 16, 1998
 * Passed:  s1: pointer to first string
 *          s2: pointer to second string
 * Notes:   uses my_memcmp to do the actual comparison; as such it depends
 *           on sort.order_array for case sensitive/insensitive matching.
 */
int  my_strcmp( text_ptr s1, text_ptr s2 )
{
int len1 = strlen( (char *)s1 ) + 1;
int len2 = strlen( (char *)s2 ) + 1;

   return( my_memcmp( s1, s2, (len1 >= len2) ? len1 : len2 ));
}


/*
 * Name:    language_list
 * Purpose: list all languages and select one
 * Author:  Jason Hood
 * Date:    September 13, 2006
 * Passed:  answer:  pointer to store selected language
 * Returns: OK if language selected, ERROR if cancelled
 */
static int  language_list( char *answer )
{
LIST *list;
int  size;
char path[PATH_MAX];
DIRECTORY dir;
int  rc;

   list = NULL;
   size = 0;
   if (file_exists( SYNTAXFILE ) == 0)
      find_languages( SYNTAXFILE, &list, &size );
   get_current_directory( path );
   if (strcmp( path, g_status.argv[0] )) {
      join_strings( path, g_status.argv[0], SYNTAXFILE );
      find_languages( path, &list, &size );
   }
   if (size == 0)
      return( ERROR );

   shell_sort( list, size );
   setup_directory_window( &dir, list, size );
   write_directory_list( list, &dir );
   xygoto( -1, -1 );
   rc = select_file( list, "", &dir );
   redraw_screen( g_status.current_window );

   if (rc == OK)
      strcpy( answer, list[dir.select].name );

   my_free_group( TRUE );
   while (--size >= 0)
      my_free( list[size].data );
   my_free_group( FALSE );
   free( list );

   return( rc );
}


/*
 * Name:    find_languages
 * Purpose: extract language names from a syntax highlighting file
 * Author:  Jason Hood
 * Date:    September 13, 2006
 * Passed:  name: the name of the file to search
 *          list: pointer to array to store languages
 *          size: pointer to size of list
 * Notes:   list & size are updated
 */
static void find_languages( char *name, LIST **list, int *size )
{
FILE *fp;
char *line;
char lang[MAX_COLS+2];
char parent[MAX_COLS+2];
char pat[PATH_MAX];
char key[MAX_COLS+2];
int  icase;
int  len1, len2, len3;
int  key_no;
int  rc = OK;
LTYPE *item;

   fp = fopen( name, "rb" );
   if (fp == NULL)
      return;

   line_no = 0;
   process_macro = FALSE;
   for (key_no = 0;;) {
      while (key_no != SHL_LANGUAGE && key_no != ERROR)
         key_no = syntax_read_line( fp, &line );
      if (key_no == ERROR)
         break;
      line = parse_token( line, lang );

      /*
       * Is there a parent language?
       */
      if (line != NULL) {
         /*
          * Skip the optional "from"
          */
         line = parse_token( line, parent );
         if (line != NULL)
            parse_token( line, parent );
         /*
          * Derived languages take their case from the parent,
          * which I don't scan for in this list.
          */
         icase = 0;
      } else {
         *parent = '\0';
         icase = MATCH;
      }
      /*
       * Create the pattern as a (semi)colon separated list.
       */
      key_no = syntax_read_line( fp, &line );
      if (key_no == SHL_PATTERN) {
         char *p = pat;
         while (line != NULL) {
            line = parse_token( line, p );
            p += strlen( p );
#if defined( __UNIX__ )
            *p++ = ':';
#else
            *p++ = ';';
#endif
         }
         p[-1] = '\0';
      } else
         *pat = '\0';

      /*
       * Search for the case
       */
      while (key_no != SHL_LANGUAGE && key_no != ERROR) {
         key_no = syntax_read_line( fp, &line );
         if (key_no == SHL_CASE) {
            parse_token( line, key );
            icase = search( key, case_modes, 1 );
            if (icase == ERROR)
               icase = MATCH;
            break;
         }
      }

      /*
       * Add the language to the list
       */
      if ((*size % 50) == 0) {
         LIST *nl = realloc( *list, (*size + 50) * sizeof(LIST) );
         if (nl == NULL)
            break;
         *list = nl;
      }
      len1 = strlen( lang ) + 1;
      len2 = strlen( parent ) + 1;
      len3 = strlen( pat ) + 1;
      item = my_malloc( sizeof(LTYPE) + len1 + len2 + len3, &rc );
      if (rc == ERROR)
         break;
      (*list)[*size].name  = (char*)item + sizeof(LTYPE);
      strcpy( (*list)[*size].name, lang );
      (*list)[*size].color = Color( Dialog );
      (*list)[*size].data  = item;
      item->parent  = (char*)item + sizeof(LTYPE) + len1;
      strcpy( item->parent, parent );
      item->pattern = item->parent + len2;
      strcpy( item->pattern, pat );
      item->icase   = icase;
      ++*size;
   }

   fclose( fp );
}
