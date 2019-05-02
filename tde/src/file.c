/*
 * This file contains the file i/o stuff.  These functions get stuff from
 * from the outside world into a form for TDE.  The form TDE uses is a
 * double linked list.  Each node in the list points to the prev and
 * the next nodes, if they exist.  Also in each node is a pointer to a
 * line of text, a line length variable, and a node type indicator.  In
 * earlier versions of TDE, a '\n' was used to terminate a line of text.
 * In this version, we must keep an accurate count of characters in
 * each line of text, as no character is used to terminate the line.
 *
 * Each file must contain at least one node.  That node is called the
 * EOF node.  The EOF node terminates the double linked list for each
 * file.  The EOF node has a NULL pointer in the line field, a NULL
 * pointer in the next field, and an EOF in the len field.  Here's
 * a crude picture of the double linked list structure:
 *
 *              Regular node                             EOF node
 *     --------                                 --------
 *     | prev | ---> pointer to prev node       | prev | ---> unknown
 *     | line | ---> "Hello world"              | line | ---> NULL
 *     | len  | ---> 11                         | len  | ---> EOF
 *     | type | ---> DIRTY | syntax flags       | type | ---> 0
 *     | next | ---> pointer to next node       | next | ---> NULL
 *     --------                                 --------
 *
 * Implicitly, I am assuming that EOF is defined as (-1) in stdio.h.
 *
 * jmh 980701: I've added a top of file marker, so now each file contains
 *             at least two nodes and the first line in the file becomes
 *             line_list->next rather than just line_list, which is the
 *             zeroth line and remains constant. It also causes changes in
 *             other files, which are not documented.
 *
 * The load_file function is probably more complicated than expected, but
 * I was trying to read chunks of text that match the disk cluster size
 * and/or some multiple of the cache in the disk controller.
 *
 * jmh 021101: tried to simplify it. Instead of reading into two buffers and
 *             and swapping the residue between them, read into one buffer
 *             and copy the residue into the other. I also read multiple
 *             binary lines at once.
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


static char file_buf[READ_LENGTH];      /* jmh 021102: file buffer */
static char *cmd_title;                 /* jmh 030331: for -c */

char wksp_file[PATH_MAX];       /* jmh 020802: for -w */
int  wksp_loaded;               /* jmh 020911: for AutoSaveWorkspace */

extern char init_wd[PATH_MAX];  /* defined in main.c */
extern char *line_in;           /* defined in config.c */
extern int  make_window;        /* defined in window.c */

#if defined( __WIN32__ )
extern HANDLE conin;            /* defined in win32/console.c */
#endif

extern int  auto_reload;        /* defined in utils.c */

extern TDE_WIN *results_window; /* defined in findrep.c */
extern file_infos *results_file;
extern file_infos *search_file;
extern long found_rline;

static void write_history( FILE *, HISTORY * ); /* added by jmh 021029 */
static void read_history( FILE *, HISTORY * );  /* ditto */


/*
 * Name:    write_file
 * Purpose: To write text to a file
 * Date:    June 5, 1991
 * Passed:  name:  name of disk file or device
 *          open_mode:  fopen flags to be used in open
 *          file:  pointer to file structure to write
 *          start: first node to write
 *          end:   last node to write
 *          block: write a file or a marked block
 * Returns: OK, or ERROR if anything went wrong
 *
 * jmh:     August 27, 1997 - if name is empty (ie. "") then write to stdout.
 *          This allows TDE to be used in a pipe.
 * jmh 021103: fixed writing the first/last line of a stream block with tabs
 *              (but if inside a tab, it will write the tab, not the spaces).
 */
int  write_file( char *name, int open_mode, file_infos *file, long start,
                 long end, int block )
{
FILE *fp;       /* file to be written */
text_ptr p;
char *z = "\x1a";
register int rc;
int  bc;
int  ec;
int  len;
int  write_z;
int  write_eol;
long number;
line_list_ptr ll;
char *open_string;
char *eol;
size_t eol_count;
int  tabs;
int  tab_size;

   if (block == LINE || block == BOX || block == STREAM) {
      if (g_status.marked_file == NULL)
         return( ERROR );
      file = g_status.marked_file;
   }

   write_z = mode.control_z;
   switch (open_mode) {
      case APPEND :
         open_string = "ab";
         break;
      case OVERWRITE :
      default :
         open_string = "wb";
         break;
   }
   switch (file->crlf) {
      case BINARY :
         eol_count = 0;
         eol = "";
         write_z = FALSE;
         break;
      case CRLF   :
         eol_count = 2;
         eol = "\r\n";
         break;
      case LF     :
         eol_count = 1;
         eol = "\n";
         break;
      default     :
         eol_count = 0;
         eol = "";
         assert( FALSE );
   }
   rc = OK;
   if (*name == '\0') {
      fp = stdout;
#if !defined( __UNIX__ )
      setmode( fileno( stdout ), O_BINARY );
#endif
   }
   else if ((fp = fopen( name, open_string )) == NULL || CEH_ERROR)
      rc = ERROR;
   if (rc == OK) {
      ll = file->line_list->next;
      for (number = 1; number < start && ll->len != EOF; number++)
         ll = ll->next;
      ec = bc = len = 0;
      if (block == BOX || block == STREAM) {
         bc  = file->block_bc;
         ec  = file->block_ec;
         len = ec + 1 - bc;
         if (block == STREAM  &&  start == end  &&  ec != -1)
            block = BOX;
      }
      tabs = file->inflate_tabs;
      tab_size = file->ptab_size;

      if (block == BOX) {
         p = g_status.line_buff;
         for (; start <= end  &&  rc == OK; start++) {
            load_box_buff( p, ll, bc, ec, ' ', tabs, tab_size );
            if (fwrite( p, sizeof( char ), len, fp ) < (unsigned)len ||
                       CEH_ERROR)
               rc = ERROR;
            if ((rc != ERROR  &&
                 fwrite( eol, sizeof( char ), eol_count, fp ) < eol_count)
                           || CEH_ERROR)
               rc = ERROR;
            ll = ll->next;
            if (ll == NULL)
               rc = ERROR;
         }
      } else {
         for (number = start; number <= end  &&  rc == OK; number++) {
            p   = ll->line;
            len = ll->len;
            if (block == STREAM) {
               if (number == start) {
                  if (tabs)
                     bc = entab_adjust_rcol( (text_ptr)p, len, bc, tab_size );
                  if (bc > len)
                     bc = len;
                  p   += bc;
                  len -= bc;
               } else if (number == end && ec != -1) {
                  if (tabs)
                     ec = entab_adjust_rcol( (text_ptr)p, len, ec, tab_size );
                  ++ec;
                  if (ec < len)
                     len = ec;
               }
            }

            if (fwrite( p, sizeof( char ), len, fp ) < (unsigned)len ||
                    CEH_ERROR)
               rc = ERROR;

            /*
             * if a Control-Z is already at EOF, don't write another one.
             */
            write_eol = TRUE;
            if (number == end) {
               if (file->crlf != BINARY) {
                  if (len > 0  &&  *(p + len - 1) == '\x1a') {
                     write_eol = FALSE;
                     write_z = FALSE;
                  }
               }
            }

            if ((write_eol == TRUE  &&  rc != ERROR  &&
                  fwrite( eol, sizeof( char ), eol_count, fp ) < eol_count)
                  || CEH_ERROR)
               rc = ERROR;
            ll = ll->next;
            if (ll == NULL)
               rc = ERROR;
         }
      }
      if (rc != ERROR  &&  write_z) {
         if (fwrite( z, sizeof( char ), 1, fp ) < 1 || CEH_ERROR)
            rc = ERROR;
      }
      g_status.copied = FALSE;
      if (!CEH_ERROR) {
         if (fclose( fp ) != 0)
            rc = ERROR;
      }
   }
   return( rc );
}


/*
 * Name:    hw_save
 * Purpose: To save text to a file
 * Date:    November 11, 1989
 * Passed:  name:  name of disk file
 *          file:  pointer to file structure
 *          start: first character in text buffer
 *          end:   last character (+1) in text buffer
 *          block: type of block defined
 * Returns: OK, or ERROR if anything went wrong
 */
int hw_save( char *name, file_infos *file, long start, long end, int block )
{
   return( write_file( name, OVERWRITE, file, start, end, block ) );
}


/*
 * Name:    hw_append
 * Purpose: To append text to a file.
 * Date:    November 11, 1989
 * Passed:  name:  name of disk file
 *          file:  pointer to file structure
 *          start: first character in text buffer
 *          end:   last character (+1) in text buffer
 *          block: type of defined block
 * Returns: OK, or ERROR if anything went wrong
 */
int hw_append( char *name, file_infos *file, long start, long end, int block )
{
   return( write_file( name, APPEND, file, start, end, block ) );
}


/*
 * Name:    load_file
 * Purpose: To load a file into the array of text pointers.
 * Date:    December 1, 1992
 * Passed:  name:       name of disk file
 *          fp:         pointer to file structure
 *          file_mode:  BINARY or TEXT
 *          bin_len:    if opened in BINARY mode, length of node line
 *          insert:     NULL for new file, pointer to line to insert file
 * Returns: OK, or ERROR if anything went wrong
 *
 * jmh:     August 27, 1997 - if name is empty (ie. "") then read from stdin.
 *           This allows TDE to be used in a pipe.
 *          January 24, 1998 - added insert line pointer to allow a file to be
 *           inserted into another file.
 * jmh 990428: if input is redirected, and output is not, set read-only mode.
 * jmh 990501: don't set mode.trailing = FALSE when loading BINARY (it's taken
 *              care of separately).
 * jmh 020822: removed UNIX' forced text mode for files starting with "#!"
 */
int  load_file( char *name, file_infos *fp, int *file_mode, int bin_len,
                line_list_ptr insert )
{
FILE *stream;                           /* stream to read */
int  rc;
line_list_ptr ll;
line_list_ptr temp_ll;
char *s, *e;
int  len;
int  t1, t2;
int  crlf;
int  prompt_line;

   /*
    * initialize the counters and pointers
    */
   rc = OK;
   prompt_line = g_display.end_line;
   ll = (insert == NULL) ? fp->line_list : insert;

   if (*name == '\0') {
      stream = stdin;
#if !defined( __UNIX__ )
      setmode( fileno( stdin ), O_BINARY );
#endif
   } else if ((stream = fopen( name, "rb" )) == NULL || CEH_ERROR) {
      /*
       * file not found or error loading file
       */
      combine_strings( line_out, main7a, name, main7b );
      error( INFO, prompt_line, line_out );
      rc = ERROR;
   }
   if (rc == OK) {
      if (*file_mode == BINARY) {
         crlf = BINARY;
         if (bin_len < 0  ||  bin_len > READ_LENGTH)
            bin_len = DEFAULT_BIN_LENGTH;

         assert( bin_len < MAX_LINE_LENGTH );

         t2 = (READ_LENGTH / bin_len) * bin_len;
         if (t2 == 0)        /* READ_LENGTH is smaller than MAX_LINE_LENGTH */
            t2 = bin_len;
         while (rc == OK) {
            t1 = fread( file_buf, sizeof(char), t2, stream );
            if (ferror( stream )  ||  CEH_ERROR) {
               combine_strings( line_out, main9, name, "'" );
               error( WARNING, prompt_line, line_out );
               rc = ERROR;
               break;
            }
            if (t1 == 0)
               break;
            s = file_buf;
            do {
               if (t1 < bin_len)     /* will only occur on the last line */
                  bin_len = t1;
               temp_ll = new_line_text( (text_ptr)s, bin_len, 0, &rc);
               if (rc != ERROR) {
                  insert_node( fp, ll, temp_ll );
                  ll = temp_ll;
               } else {
                  rc = show_file_2big( name, prompt_line );
                  break;
               }
               s += bin_len;
               t1 -= bin_len;
            } while (t1 != 0);
         }
      } else {
         crlf = LF;
         len = 0;
         while (rc == OK) {
            t1 = fread( file_buf, 1, READ_LENGTH, stream );
            if (ferror( stream )  ||  CEH_ERROR) {
               combine_strings( line_out, main9, name, "'" );
               error( WARNING, prompt_line, line_out );
               rc = ERROR;
               break;
            }
            if (t1 == 0)
               break;
            s = file_buf;
            do {
               e = memchr( s, '\n', t1 );
               if (e == NULL) {
                  if (len + t1 <= READ_LENGTH) {
                     memcpy( g_status.line_buff + len, s, t1 );
                     len += t1;
                     break;
                  } else {
                     t2 = READ_LENGTH - len;
                     e = s + t2 - 1;
                     t1 -= t2;
                  }
               } else {
                  t2 = (size_t)(e - s);
                  t1 -= t2 + 1;
                  if (t2) {
                     if (e[-1] == '\r') {
                        --t2;
                        crlf = CRLF;
                     }
                  } else if (len) {
                     if (g_status.line_buff[len-1] == '\r') {
                        --len;
                        crlf = CRLF;
                     }
                  }
                  if (len) {
                     if (len + t2 > READ_LENGTH) {
                        t2 = READ_LENGTH - len;
                        t1 += (size_t)(e - (s + t2 - 1));
                        e = s + t2 - 1;
                     }
                  }
               }
               /*
                * allocate space for relocatable array of line pointers
                *   and allocate space for the line we just read.
                */
               if (len) {
                  temp_ll = new_line_text(g_status.line_buff, len + t2, 0, &rc);
                  if (rc != ERROR) {
                     my_memcpy( temp_ll->line + len, s, t2 );
                     len = 0;
                  }
               } else
                  temp_ll = new_line_text( (text_ptr)s, t2, 0, &rc );

               if (rc != ERROR) {
                  insert_node( fp, ll, temp_ll );
                  ll = temp_ll;
               } else {
                  rc = show_file_2big( name, prompt_line );
                  break;
               }
               s = e + 1;
            } while (t1 != 0);
         }
         /*
          * we may have read all lines that end in '\n', but there may
          *   be some residue after the last '\n'.  ^Z is a good example.
          */
         if (len  &&  rc == OK) {
            temp_ll = new_line_text( g_status.line_buff, len, 0, &rc );
            if (rc != ERROR)
               insert_node( fp, ll, temp_ll );
            else
               rc = show_file_2big( name, prompt_line );
         }
         *file_mode = crlf;
      }
   }
   /*
    * Restore input back to the keyboard for shelling.
    */
   if (stream == stdin) {
#if defined( __WIN32__ )
      DWORD mode;
      SetStdHandle( STD_INPUT_HANDLE, conin );
      GetConsoleMode( conin, &mode );
      SetConsoleMode( conin, ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT
                             | (mode & ENABLE_QUICK_EDIT) );
#else
      freopen( STDFILE, "r", stdin );
#endif
      g_status.input_redir = FALSE;
   } else if (stream != NULL)
      fclose( stream );
   return( rc );
}


/*
 * Name:    insert_node
 * Purpose: To insert a node into a double linked list
 * Date:    December 1, 1992
 * Passed:  fp:  pointer to file structure that owns the double linked list
 *          current: pointer to current node in double linked list
 *          new:     pointer to new node to insert into double linked list
 * Notes:   if the current list pointer is the last node in the list, insert
 *            new node behind current node.
 *
 * jmh 031116: increase the file's length.
 */
void insert_node( file_infos *fp, line_list_ptr current, line_list_ptr new )
{
   /*
    * standard double linked list insert
    */
   if (current->next != NULL) {
      current->next->prev = new;
      new->next = current->next;
      current->next = new;
      new->prev = current;
   /*
    * if the current node is the NULL node, insert the new node behind current
    */
   } else {
      new->next = current;
      current->prev->next = new;
      new->prev = current->prev;
      current->prev = new;
   }
   ++fp->length;
}


/*
 * Name:    show_file_2big
 * Purpose: tell user we ran out of room loading file
 * Date:    December 1, 1992
 * Passed:  name:  name of disk file
 *          line:  line to display messages
 * Returns: WARNING
 * Notes:   one or both of the malloc requests overflowed the heap.  free the
 *            dynamic if allocated.
 */
int  show_file_2big( char *name, int prompt_line )
{
   combine_strings( line_out, main7a, name, main10b );
   error( WARNING, prompt_line, line_out );
   return( WARNING );
}


/*
 * Name:    backup_file
 * Purpose: To make a back-up copy of current file.
 * Date:    June 5, 1991
 * Passed:  window:  current window pointer
 *
 * jmh 070430: always succeed, otherwise nothing else is possible.
 */
int  backup_file( TDE_WIN *window )
{
char *old_line_buff;
char *old_tabout_buff;
int  old_line_buff_len;
int  old_tabout_buff_len;
int  old_copied;
file_infos *file;

   file = window->file_info;
   if (file->backed_up == FALSE  &&  file->modified == TRUE) {
      old_line_buff = malloc( MAX_LINE_LENGTH );
      old_tabout_buff = malloc( MAX_LINE_LENGTH );
      if (old_line_buff != NULL  &&  old_tabout_buff != NULL) {
         old_line_buff_len = g_status.line_buff_len;
         old_tabout_buff_len = g_status.tabout_buff_len;
         old_copied = g_status.copied;
         memcpy( old_line_buff, g_status.line_buff, MAX_LINE_LENGTH );
         memcpy( old_tabout_buff, g_status.tabout_buff, MAX_LINE_LENGTH );
         if (save_backup( window ) != ERROR)
            set_ftime( file->backup_fname, &file->ftime );
         memcpy( g_status.line_buff, old_line_buff, MAX_LINE_LENGTH );
         memcpy( g_status.tabout_buff, old_tabout_buff, MAX_LINE_LENGTH );
         g_status.line_buff_len = old_line_buff_len;
         g_status.tabout_buff_len = old_tabout_buff_len;
         g_status.copied = old_copied;
      } else
         error( WARNING, window->bottom_line, main4 );
      if (old_line_buff != NULL)
         free( old_line_buff );
      if (old_tabout_buff != NULL)
         free( old_tabout_buff );
      file->backed_up = TRUE;
   }
   return( OK );
}


/*
 * Name:    edit_file
 * Purpose: To allocate space for a new file structure and set up some
 *           of the relevant fields.
 * Date:    June 5, 1991
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen
 * Passed:  name:  name of the file to edit
 *          file_mode:  BINARY or TEXT
 *          bin_length: if opened in BINARY mode, length of binary lines
 * Returns: OK if file structure could be created
 *          ERROR if out of memory
 *
 * Change:  file->next_letter is initialized to 0 instead 'a'
 *
 * jmh:     August 27, 1997 - if name is empty (ie. "") then read from stdin.
 *          This allows TDE to be used in a pipe.
 *
 * jmh 980503: Use of get_full_path to store the filename.
 * jmh 991111: Honour EndOfLineStyle setting for new files.
 * jmh 010521: removed code to free undo and line lists (done in load_file()).
 * jmh 020724: record the binary length.
 * jmh 021020: read-only files are marked as read-only.
 * jmh 031202: free the memory from an unsuccessful grep.
 * jmh 050705: if file doesn't exist, grep should error, not create new.
 * jmh 050918: likewise if path doesn't exist (same as command line).
 */
int  edit_file( char *name, int file_mode, int bin_length )
{
int  rc;        /* return code */
int  existing;
int  line;
int  rcol;
register file_infos *file; /* file structure for file belonging to new window */
file_infos *fp;
long found_line;
long result_length;
line_list_ptr ll;
line_list_ptr temp_ll;
int  grep;

   grep = (g_status.command == DefineGrep || g_status.command == RepeatGrep);

   line = g_display.end_line;
   rc = OK;
   /*
    * allocate a file structure for the new file
    */
   file = (file_infos *)calloc( 1, sizeof(file_infos) );
   if (file == NULL)
      rc = ERROR;
   else {
      ll = (line_list_ptr)my_malloc( sizeof(line_list_struc), &rc );
      if (rc == OK) {
         ll->line = NULL;
         ll->next = ll->prev = NULL;
         ll->type = 0;
         ll->len  = EOF;
         file->undo_top = file->undo_bot = ll;

         ll = (line_list_ptr)my_malloc( sizeof(line_list_struc), &rc );
         temp_ll = (line_list_ptr)my_malloc( sizeof(line_list_struc), &rc );
         if (rc == OK) {
            ll->type = temp_ll->type = 0;
            ll->len  = temp_ll->len  = EOF;
            ll->line = temp_ll->line = NULL;
            ll->prev = temp_ll->next = NULL;
            ll->next = temp_ll;
            temp_ll->prev = ll;
            file->line_list = ll;
            file->line_list_end = temp_ll;
         } else {
            my_free( ll );
            my_free( file->undo_top );
         }
      }
   }
   if (rc == ERROR) {
      error( WARNING, line, main4 );
      if (file != NULL)
         free( file );
      return( ERROR );
   }

   existing = TRUE;
   if (*name == '\0'  &&  (g_status.command != ScratchWindow &&
                           !(g_status.search & SEARCH_RESULTS)))
      rc = load_file( name, file, &file_mode, bin_length, NULL );

   else if (file_exists( name ) != ERROR) {

      if (!grep)
         rc = load_file( name, file, &file_mode, bin_length, NULL );
      else {
         if (g_status.sas_defined) {
            rc = load_file( name, file, &file_mode, bin_length, NULL );
            if (rc != ERROR) {
               found_line = 1L;
               rcol = 0;
               ll = file->line_list->next;
               result_length = -1;
               if (g_status.search & SEARCH_RESULTS) {
                  get_full_path( name, file->file_name );
                  search_file = file;
                  if (CB_G_LoadAll)
                     result_length = (results_file) ? results_file->length : 0;
               }
               ll = (g_status.sas_search_type == BOYER_MOORE)
                    ? search_forward(      ll, &found_line, &rcol )
                    : regx_search_forward( ll, &found_line, &rcol );
               if (ll == NULL) {
                  if (result_length == -1  ||  results_file == NULL  ||
                      result_length == results_file->length) {
                     rc = ERROR;
                     my_free_group( TRUE );
                     ll = file->line_list;
                     while (ll != NULL) {
                        temp_ll = ll->next;
                        my_free( ll->line );
                        my_free( ll );
                        ll = temp_ll;
                     }
                     my_free_group( FALSE );
                     file->line_list = file->line_list_end = NULL;
                  }
               } else {
                  g_status.sas_rline = found_line;
                  g_status.sas_rcol  = rcol;
                  g_status.sas_ll    = ll;
               }
            }
         } else
            rc = ERROR;
      }
   } else {
      if (CEH_ERROR  ||  (*name && grep))
         rc = ERROR;
      else {
         existing = FALSE;

         if (file_mode == TEXT)
            file_mode = (mode.crlf != NATIVE) ? mode.crlf :
#if defined( __UNIX__ )
                        LF;
#else
                        CRLF;
#endif
      }
   }

   if (rc != ERROR) {
      /*
       * add file into list
       */
      file->prev = NULL;
      file->next = NULL;
      if (g_status.file_list == NULL)
         g_status.file_list = file;
      else {
         if (g_status.current_file == NULL  ||
             (CB_G_LoadAll && (g_status.search & SEARCH_RESULTS) &&
              results_file && results_file->prev == g_status.current_file)) {
            g_status.current_file   = results_file;
            g_status.current_window = results_window;
         }
         fp = g_status.current_file;
         file->prev = fp;
         if (fp->next)
            fp->next->prev = file;
         file->next = fp->next;
         fp->next = file;
      }

      /*
       * set up all the info we need to know about a file.
       */

      assert( file_mode == CRLF  ||  file_mode == LF  ||  file_mode == BINARY );
      assert( strlen( name ) < PATH_MAX );

      if (*name != '\0') {
         get_full_path( name, file->file_name );
         get_fattr( name, &file->file_attrib );
         get_ftime( name, &file->ftime );
         file->scratch = g_option.scratch;

      } else if (g_status.command == ScratchWindow) {
         file->scratch = ++g_status.scratch_count;
         if (file->scratch == 1)
            strcpy( file->file_name + 1, scratch_file );
         else
            sprintf( file->file_name+1, "%s%d", scratch_file, file->scratch-1 );
      } else {
         file->scratch = -1;
         if (g_status.search & SEARCH_RESULTS) {
            file->file_name[1] = RESULTS;
            strcpy( file->file_name + 2,
                    (char *)((g_status.search & SEARCH_REGX)
                              ? regx.pattern : bm.pattern) );
         } else
            strcpy( file->file_name + 1, pipe_file );
      }

      file->block_type  = NOTMARKED;
      file->block_br    = file->block_er = 0L;
      file->block_bc    = file->block_ec = 0;
      file->ref_count   = 0;
      file->modified    = FALSE;
      file->backed_up   = FALSE;
      file->new_file    = !existing;
      file->next_letter = 0;
      file->file_no     = ++g_status.file_count;
      file->crlf        = file_mode;
      file->read_only   = (file_exists( name ) == READ_ONLY);
      file->binary_len  = bin_length;
      file->ltab_size   = mode.ltab_size;
      if (g_option.tab_size == -1) {
         file->ptab_size    = mode.ptab_size;
         file->inflate_tabs = (file_mode == BINARY) ? 0 : mode.inflate_tabs;
      } else if (g_option.tab_size == 0) {
         file->ptab_size    = mode.ptab_size;
         file->inflate_tabs = 0;
      } else {
         file->ptab_size    = g_option.tab_size;
         file->inflate_tabs = (file_mode == BINARY) ? 0 :
                              (mode.inflate_tabs) ? mode.inflate_tabs : 1;
      }
      file->len_len     = (mode.line_numbers) ? numlen( file->length ) + 1 : 0;
      g_status.current_file = file;
      make_backup_fname( file );
   } else {
#if defined( __MSC__ )
      _fheapmin( );
#endif

      my_free( file->undo_top );
      my_free( file->line_list_end );
      my_free( file->line_list );
      free( file );
   }
   return( rc );
}


/*
 * Name:    edit_another_file
 * Purpose: Bring in another file to editor.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   New window replaces old window.  Old window becomes invisible.
 *
 * jmh:     January 24, 1998 - can insert file into current.
 * jmh 981127: if the name is a wildcard, bring up the directory list.
 * jmh 990502: if the name is "=", use the current name.
 * jmh 991028: if the name ends in a slash or backslash, assume a directory
 *              and bring up the list.
 * jmh 030226: added ScratchWindow.
 */
int  edit_another_file( TDE_WIN *window )
{
char fname[PATH_MAX+2];         /* new name for file */
register TDE_WIN *win;          /* put window pointer in a register */
int  rc;
int  file_mode;
int  bin_length;
fattr_t fattr;

   win = window;
   if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
      return( ERROR );

   if (g_status.command == InsertFile && win->ll->next == NULL)
      return( ERROR );

   if (g_status.command == ScratchWindow)
      return( attempt_edit_display( "", LOCAL ) );

   /*
    * read in name, no default
    */
   fname[0] = '\0';
   /*
    * file name to edit
    */
   if ((rc = get_name( (g_status.command == InsertFile) ? ed15a : ed15,
                       win->bottom_line, fname, &h_file )) > 0) {

      --rc;
      if (is_pattern( fname ) || fname[rc] == '/' ||
#if !defined( __UNIX__ )
          fname[rc] == '\\' || fname[rc] == ':' ||
#endif
          file_exists( fname ) == SUBDIRECTORY) {
         rc = list_and_pick( fname, win );
         if (rc != OK)
            return( ERROR );

      } else if (*fname == '=' && fname[1] == '\0')
         strcpy( fname, win->file_info->file_name );

      if (g_status.command == InsertFile) {
         long change = win->file_info->length;
         file_mode  = TEXT;
         bin_length = 0;
         g_option.file_chunk = abs( g_option_all.file_chunk );
         rc = binary_file( fname );
         if (rc != ERROR) {
            if (rc == TRUE) {
               file_mode  = BINARY;
               bin_length = g_option.file_chunk;
            }
            rc = load_file(fname,win->file_info,&file_mode,bin_length,win->ll);
            if (rc == OK) {
               syntax_check_block( change + 1, win->file_info->length,
                                   win->ll->next, win->file_info->syntax );
               change = win->file_info->length - change;
               adjust_windows_cursor( win, change );
               restore_marked_block( win, (int)change );
               win->file_info->modified = TRUE;
               win->file_info->dirty = GLOBAL;
               show_size( win );
            }
         }
      }
      else if (get_fattr( fname, &fattr ) == 3)
         error( WARNING, win->bottom_line, dir2 );
      else
         rc = attempt_edit_display( fname, LOCAL );
   }
   return( rc );
}


/*
 * Name:    reload_file
 * Purpose: discard the current changes and reload the file
 * Author:  Jason Hood
 * Date:    March 18, 2003
 * Notes:   keeps the deleted lines, but resets the undo information.
 *          used explicitly by Revert and implicitly by Shell.
 *
 * 051018:  don't ERROR on reverting scratch windows or choosing not to reload.
 */
int  reload_file( TDE_WIN *window )
{
file_infos *file;
int  prompt_line;
const char *prompt;
int  file_mode;
int  bin_length;
line_list_ptr ll;
line_list_ptr temp_ll;
UNDO *undo;
UNDO *temp_u;
register TDE_WIN *wp;
long n;
int  rc;

   file = window->file_info;
   prompt_line = window->bottom_line;
   /*
    * ignore the pipe and scratch windows
    * make sure the file still exists
    */
   if (g_status.command == Revert) {
      if (!*file->file_name)
         return( OK );
      if (file_exists( file->file_name ) == ERROR) {
         error( WARNING, prompt_line, main23a );
         return( ERROR );
      }
      prompt = main23;
      g_status.copied = FALSE;

   } else {
      if (window->visible)
         prompt = main24;
      else {
         prompt_line = g_display.end_line;
         combine_strings( line_out, main7a, file->file_name, main25b );
         prompt = line_out;
      }
   }
   if (!auto_reload)
      if (get_yn( prompt, prompt_line, R_PROMPT | R_ABORT ) != A_YES)
         return( OK );

   my_free_group( TRUE );
   ll = file->line_list->next;
   while (ll->len != EOF) {
      temp_ll = ll->next;
      my_free( ll->line );
      my_free( ll );
      ll = temp_ll;
   }
   undo = file->undo;
   while (undo != NULL) {
      temp_u = undo->prev;
      if ((unsigned long)undo->text > 255)
         my_free( undo->text );
      my_free( undo );
      undo = temp_u;
   }
   my_free_group( FALSE );

   file->length = 0;
   file->line_list->next = file->line_list_end;
   file->line_list_end->prev = file->line_list;
   file->undo_count = 0;
   file->undo = NULL;

   bin_length = file->binary_len;
   file_mode = (bin_length == 0) ? TEXT : BINARY;

   rc = load_file( file->file_name, file, &file_mode, bin_length,
                   file->line_list );
   if (rc == OK) {
      get_ftime( file->file_name, &file->ftime );
      syntax_init_lines( file );
      for (wp = g_status.window_list; wp; wp = wp->next) {
         if (wp->file_info == file) {
            n = wp->rline;
            if (n > file->length + 1)
               n = file->length + 1;
            first_line( wp );
            move_to_line( wp, n, TRUE );
            check_cline( wp, wp->cline );
         }
      }
   }
   show_size( window );
   show_avail_mem( );
   file->modified = FALSE;
   file->dirty = GLOBAL | RULER;

   return( rc );
}


/*
 * Name:    attempt_edit_display
 * Purpose: try to load then display a file
 * Date:    June 5, 1991
 * Passed:  fname:       file name to load
 *          update_type: update current window or entire screen
 * Notes:   When we first start the editor, we need to update the entire
 *          screen.  When we load in a new file, we only need to update
 *          the current window.
 *
 * jmh:     September 8, 1997 - call init_syntax() to setup syntax highlighting
 * jmh 980801: moved the above from create_window (why didn't I think to put
 *              it here in the first place?)
 * jmh 981111: set deflate tabs when using binary mode.
 * jmh 981127: determine if a file should be loaded in binary directly here,
 *              rather than in all the calling routines.
 * jmh 990425: update file and language history here, to recognize the command
 *              line, dir lister and grep files.
 * jmh 990429: recognize read-only/viewer option.
 * jmh 990501: binary mode deflate tabs moved to edit_file().
 * jmh 020730: if fname is NULL, take g_status.current_file as already having
 *              been loaded (in the workspace).
 * jmh 021023: handle the command line options appropriately.
 * jmh 021103: don't store workspace loaded files in the history.
 * jmh 030303: add a command line flag to update_type.
 */
int  attempt_edit_display( char *fname, int update_type )
{
int  rc;
TDE_WIN *win;
int  file_mode;
int  bin_length;
char *old_language;
int  grep, cmd_line;

   grep = (g_status.command == DefineGrep || g_status.command == RepeatGrep);
   cmd_line = (update_type & CMDLINE);

   file_mode  = TEXT;
   bin_length = 0;

   if (!cmd_line) {
      g_option.language = NULL;
      g_option.scratch  = FALSE;
      if (!grep)
         g_option.file_chunk = abs( g_option.file_chunk );
      /*
       * Let the tab size still apply, since files from the
       *  same directory probably use the same tabs.
       */
      /* g_option.tab_size = -1; */
   }

   if (g_option.file_mode == BINARY  &&  cmd_line) {
      bin_length = g_option.file_chunk;
      if (bin_length != 0)
         file_mode = BINARY;
   } else if (fname != NULL) {
      rc = binary_file( fname );
      if (rc == TRUE) {
         file_mode  = BINARY;
         bin_length = g_option.file_chunk;
      } else if (rc == ERROR)
         return( rc );
   }

   if (fname == NULL)
      rc = OK;
   else
      rc = edit_file( fname, file_mode, bin_length );

   if (rc != ERROR) {
      rc = initialize_window( );
      if (rc != ERROR) {
         win = g_status.current_window;
         if (file_mode == TEXT)
            init_syntax( win );
         if (cmd_line  &&  cmd_title) {
            if (cmd_title[0] == '.' && cmd_title[1] == '\0') {
               if (win->file_info->scratch)
                  old_language = NULL;
               else
                  old_language = win->file_info->file_name;
            } else {
               old_language = cmd_title;
               cmd_title = NULL;
            }
            if (old_language)
               win->title = my_strdup( old_language );
         }

         if ((update_type & O_READ_ONLY) ||
             (g_status.viewer_mode && !(update_type & O_READ_WRITE)))
            win->file_info->read_only = TRUE;
         update_type &= ~(O_READ_WRITE | O_READ_ONLY | CMDLINE);

         set_path( win );

         if (update_type == GLOBAL)
            redraw_screen( win );
         else {
            if (update_type == LOCAL || g_status.window_count > 1) {
               show_file_count( );
               show_window_count( );
               show_avail_mem( );
            }
            if (update_type == LOCAL && !make_window) {
               redraw_current_window( win );
               show_tab_modes( );
            }
         }
         if (grep && !(g_status.search & SEARCH_RESULTS)) {
            find_adjust( win, g_status.sas_ll, g_status.sas_rline,
                              g_status.sas_rcol );
            win->file_info->dirty |= LOCAL;
         }

         if (win->file_info->new_file && !(g_status.search & SEARCH_RESULTS)) {
            g_status.command = AddLine;
            insert_newline( win );
            win->file_info->modified = FALSE;
         }

         /*
          * assume if fname is in the file line buffer, the file
          * is being loaded from the workspace. Don't store it
          * in the history, as otherwise it will probably be there
          * twice: as the complete filename from the workspace and
          * again as the name entered on the command line from the
          * restored history.
          * jmh 031029: don't store the temporary output filename.
          */
         if (fname != line_in + 1  &&  g_status.command != Execute)
            add_to_history( fname, &h_file );
         if (win->file_info->syntax)
            add_to_history( win->file_info->syntax->name, &h_lang );

         /*
          * Process the startup macro.
          */
         if (cmd_line  &&  g_option.macro) {
            g_status.command = PlayBack;
            g_status.key_macro = macro[KEY_IDX( g_option.macro )];
            play_back( win );
         }
      }
   }

   return( rc );
}


/*
 * Name:     file_file
 * Purpose:  To file the current file to disk.
 * Date:     September 17, 1991
 * Modified: August 27, 1997, Jason Hood
 * Passed:   window:  pointer to current window
 *
 * Change:   If output has been redirected then send the file to stdout.
 *           If a block has been marked, that will be sent, not the whole file.
 */
int  file_file( TDE_WIN *window )
{
register file_infos *file;
long first, last;
#if defined( __UNIX__ )
extern int stdoutput;
#endif

   if (!g_status.output_redir) {
      if (save_file( window ) == OK)
         finish( window );
   } else {
      if (un_copy_line( window->ll, window, TRUE, TRUE ) == ERROR)
         return( ERROR );
      file = window->file_info;
      if (file->block_type <= NOTMARKED) {
         first = 1;
         last  = file->length;
      } else {
         first = file->block_br;
         last  = file->block_er;
      }
#if defined( __UNIX__ )
      console_suspend( );
      dup2( stdoutput, fileno( stdout ) );
      hw_save( "", file, first, last, file->block_type );
      freopen( STDFILE, "w", stdout );
      console_resume( FALSE );
#else
      hw_save( "", file, first, last, file->block_type );
#endif
      finish( window );
   }
   return( OK );
}


/*
 * Name:    file_all
 * Purpose: To save all windows and exit the editor.
 * Date:    August 21, 1997
 * Author:  Jason Hood
 * Passed:  window:  pointer to current window
 * Notes:   if any window couldn't be saved, all windows will remain open.
 *
 * 020911:  possibly save the workspace.
 */
int  file_all( TDE_WIN *window )
{
register TDE_WIN *wp;
int  rc = OK;

   if (g_status.output_redir) {
      error( WARNING, g_display.end_line, main22 );
      return( ERROR );
   }

   if (un_copy_line( window->ll, window, TRUE, TRUE ) == ERROR)
      return( ERROR );

   for (wp=g_status.window_list; wp != NULL; wp=wp->next) {
      if (save_file( wp ) == ERROR)
         rc = ERROR;
   }
   if (rc == OK) {
      if (mode.auto_save_wksp)
         rc = save_workspace( window );
      g_status.stop = TRUE;
   }
   return( rc );
}


/*
 * Name:    save_all
 * Purpose: To save all files.
 * Date:    July 15, 2004
 * Author:  Jason Hood
 * Passed:  window:  pointer to current window
 * Notes:   scratch and read-only windows are not saved.
 */
int  save_all( TDE_WIN *window )
{
register TDE_WIN *wp;
int  rc = OK;

   if (un_copy_line( window->ll, window, TRUE, TRUE ) == ERROR)
      return( ERROR );

   for (wp = g_status.window_list; wp != NULL; wp = wp->next) {
      if (!wp->file_info->scratch  &&  !wp->file_info->read_only)
         if (save_file( wp ) == ERROR)
            rc = ERROR;
   }
   return( rc );
}


/*
 * Name:    save_file
 * Purpose: To save the current file to disk.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   If anything goes wrong, then the modified flag is set.
 *          If the file is saved successfully, then modified flag is
 *           cleared.
 *
 * jmh 021023: allow read-only files to be saved. If the file is read-only,
 *              prompt to overwrite; if marked read-only, prompt to save;
 *              if both, only prompt to overwrite.
 * jmh 030318: added SaveUntouched function to preserve the timestamp.
 */
int  save_file( TDE_WIN *window )
{
register file_infos *file;
char *fname;
int  rc = ERROR;
line_list_ptr temp_ll;
int  prompt_line;
fattr_t fattr;
int  restore_fattr = FALSE;
int  save = TRUE;

   file = window->file_info;
   if (file->modified == FALSE)
      return( OK );

   if (un_copy_line( window->ll, window, TRUE, TRUE ) == ERROR)
      return( ERROR );

   fname = file->file_name;
   prompt_line = window->bottom_line;

   /*
    * the file is marked read-only, but is not itself read-only,
    *  so ask to save.
    * jmh 030730: likewise with files loaded as scratch.
    */
   if (file->scratch == -1  ||
       (file->read_only && file_exists( fname ) != READ_ONLY)) {
      combine_strings( line_out, main7a, fname,
                       (file->read_only) ? utils7c : utils7d );
      if (get_yn( line_out, prompt_line, R_PROMPT | R_ABORT ) != A_YES)
         save = FALSE;
   }

   /*
    * see if there was a file name - if not, then make the user
    *  supply one.
    */
   if (*fname == '\0') {
      if (save)
         rc = save_as_file( window );
   } else {
      /*
       * save the file.  prompt to overwrite if it is read-only.
       */
      if (file_exists( fname ) == READ_ONLY) {
         get_fattr( fname, &fattr );
#if !defined( __UNIX__ )
         /*
          * saving a file sets the archive attribute, so make sure
          *  it is set when the read-only attribute is restored.
          */
         fattr |= ARCHIVE;
#endif
         if (change_mode( fname, prompt_line ) == ERROR)
            save = FALSE;
         else
            restore_fattr = TRUE;
      }
      if (save) {
         rc = write_to_disk( window, fname );
         if (restore_fattr)
            set_fattr( fname, fattr );
         if (rc != ERROR) {
            file->modified = FALSE;
            file->new_file = FALSE;

            /*
             * (re)set the timestamp
             */
            if (g_status.command == SaveUntouched)
              set_ftime( fname, &file->ftime );
            else
              get_ftime( fname, &file->ftime );

            /*
             * clear the dirty flags
             */
            if (rc == OK) {
               temp_ll = file->line_list->next;
               for (; temp_ll->len != EOF; temp_ll=temp_ll->next)
                  temp_ll->type &= ~DIRTY;
               file->dirty = GLOBAL;
            }
         }
      }
   }
   if (!save && (g_status.command == File || g_status.command == FileAll))
      /*
       * should closing a read-only file abandon changes?
       */
      if (get_yn( utils12, prompt_line, R_PROMPT | R_ABORT ) == A_YES)
         rc = OK;

   return( rc );
}


/*
 * Name:    save_backup
 * Purpose: To save a backup copy of the current file to disk.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 */
int  save_backup( TDE_WIN *window )
{
   /*
    * set up file name
    */
   return( write_to_disk( window, window->file_info->backup_fname ) );
}


/*
 * Name:    save_as_file
 * Purpose: To save the current file to disk, but under a new name.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 *
 * jmh:     April 29, 1998 - renames and saves the current file.
 * jmh 980803: default to current name.
 * jmh 020722: store full path.
 * jmh 030318: added SaveTo function to keep the current name (like the
 *              original SaveAs).
 * jmh 040716: moved SaveTo to block_write (to allow appending).
 */
int  save_as_file( TDE_WIN *window )
{
int  prompt_line;
int  rc;
register TDE_WIN *win;           /* put window pointer in a register */
register file_infos *file;
char answer[PATH_MAX+2];
line_list_ptr temp_ll;
int  newtitle;
char *title;

   win = window;
   if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
      return( ERROR );
   /*
    * read in name
    */
   prompt_line = win->bottom_line;

   strcpy( answer, win->file_info->file_name );
   /*
    * new file name:
    */
   if ((rc = get_name( utils9, prompt_line, answer, &h_file )) > 0) {

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

      if (rc != ERROR)
         rc = write_to_disk( win, answer );

      if (rc == OK) {
         file = win->file_info;
         newtitle = (win->title && strcmp( win->title, file->file_name ) == 0);
         get_full_path( answer, file->file_name );
         make_backup_fname( file );
         if (newtitle) {
            title = my_strdup( file->file_name );
            if (title != NULL) {
               my_free( win->title );
               win->title = title;
            }
            /* else stick with the old name or turn the title off? */
         }
         show_window_fname( win );
         file->modified = FALSE;
         file->new_file = FALSE;
         file->scratch  = 0;
         get_ftime( answer, &file->ftime );

         temp_ll = file->line_list->next;
         for (; temp_ll->len != EOF; temp_ll=temp_ll->next)
            temp_ll->type &= ~DIRTY;
         file->dirty = GLOBAL;
      }
   }
   return( rc );
}


/*
 * Name:    make_backup_fname
 * Purpose: change the file name's extension to "bak"
 * Date:    January 6, 1992
 * Passed:  file: information allowing access to the current file
 *
 * jmh 010604: if we're in UNIX, or djgpp with LFN, simply append ".bak" (eg:
 *              file.h -> file.h.bak; file.c -> file.c.bak).
 * jmh 021021: determine the pointer to the file name itself.
 * jmh 031117: use it store the current directory of a scratch window, so
 *              browsing can find the right files.
 */
void make_backup_fname( file_infos *file )
{
char *p;
int  len;
#if defined( __MSDOS__ )
char temp[PATH_MAX+2];
int  i;
#endif

   p = file->file_name;
   if (g_status.command == Execute)
      *p = '\0';
   /*
    * if this is a new file or scratch then don't create a backup - can't
    *   backup a nonexisting file.
    */
   if (file->new_file || *p == '\0') {
      file->backed_up = TRUE;
      if (*p == '\0')
         get_current_directory( file->backup_fname );

   /*
    * find the file name extension if it exists
    */
   } else {
      assert( strlen( p ) < PATH_MAX );

#if defined( __UNIX__ ) || defined( __WIN32__ )
      join_strings( file->backup_fname, p, ".bak" );
#else
# if defined( __DJGPP__ )
      if (_USE_LFN)
         join_strings( file->backup_fname, p, ".bak" );
      else {
# endif
      strcpy( temp, p );
      len = strlen( temp );
      for (i=len, p=temp + len; i>=0; i--) {

         /*
          * we found the '.' extension character.  get out
          */
         if (*p == '.')
            break;

         /*
          * we found the drive or directory character.  no extension so
          *  set the pointer to the end of file name string.
          */
         else if (*p == '/' || *p == ':') {
            p = temp + len;
            break;

         /*
          * we're at the beginning of the string - no '.', drive, or directory
          *  char was found.  set the pointer to the end of file name string.
          */
         } else if (i == 0) {
            p = temp + len;
            break;
         }
         --p;
      }
      strcpy( p, ".bak" );
      strcpy( file->backup_fname, temp );
# if defined( __DJGPP__ )
      } /* else !_USE_LFN */
# endif
#endif /* __UNIX__ || __WIN32__ */
   }

   p = file->file_name;
   if (*p == '\0')
      file->fname = p;
   else {
      len = strlen( p ) - 2;    /* start from the second-last character */
      p  += len;
      for (; len >= 0 && *p != '/'
#if !defined( __UNIX__ )
                                   && *p != ':'
#endif
           ; --p, --len) ;
      file->fname = p + 1;
   }
}


/*
 * Name:    write_to_disk
 * Purpose: To write file from memory to disk
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 *          fname:   file name to save on disk
 */
int  write_to_disk( TDE_WIN *window, char *fname )
{
register file_infos *file;
int  rc;
int  prompt_line;
DISPLAY_BUFF;

   file = window->file_info;
   prompt_line = window->bottom_line;
   SAVE_LINE( prompt_line );
   /*
    * saving
    */
   combine_strings( line_out, utils6, fname, "'" );
   set_prompt( line_out, prompt_line );
   if ((rc = hw_save( fname, file, 1L, file->length, NOTMARKED )) == ERROR) {
      if (!CEH_ERROR) {
         if (file_exists( fname ) == READ_ONLY)
            /*
             * file is read only
             */
            combine_strings( line_out, main7a, fname, utils7b );
         else
            /*
             * cannot write to
             */
            combine_strings( line_out, utils8, fname, "'" );
         error( WARNING, prompt_line, line_out );
      }
   }
   RESTORE_LINE( prompt_line );
   return( rc );
}


/*
 * Name:    search_and_seize
 * Purpose: search files for a pattern
 * Date:    October 31, 1992
 * Passed:  window:  pointer to current window
 * Notes:   New window replaces old window.  Old window becomes invisible.
 *
 * jmh 021024: moved the display code into attempt_edit_display().
 * jmh 050809: restore the original path when continuing the search.
 */
int  search_and_seize( TDE_WIN *window )
{
char  *name = NULL;
char  cwd[PATH_MAX];            /* buffer for current directory */
int   i;
int   update_type;
char  *tokens;
register int rc;
register TDE_WIN *win;          /* put window pointer in a register */
TDE_WIN *first_win = NULL;
int   bottom_line;
DISPLAY_BUFF;

   win = window;
   update_type = win == NULL ? GLOBAL : LOCAL;
   if (update_type == LOCAL) {
      bottom_line = win->bottom_line;
      if (!g_status.sas_defined ||
          (g_status.command == DefineGrep && g_status.sas_defined != TRUE+1)) {

         g_status.command = DefineGrep;
         rc = do_dialog( grep_dialog, grep_proc );
         if (rc == ERROR)
            return( ERROR );

         g_status.sas_search_type = (CB_G_RegX) ? REG_EXPRESSION : BOYER_MOORE;

         tokens = get_dlg_text( EF_G_Pattern );
         if (*tokens == '\0')
            return( ERROR );
         strcpy( (char *)((g_status.sas_search_type == BOYER_MOORE)
                          ? sas_bm.pattern : sas_regx.pattern), tokens );

         tokens = get_dlg_text( EF_G_Files );
         if (*tokens == '\0')
            return( ERROR );
         strcpy( g_status.sas_tokens, tokens );
         i = 0;
         tokens = strtok( g_status.sas_tokens, SAS_DELIMITERS );
         while (tokens != NULL) {
            g_status.sas_arg_pointers[i++] = tokens;
            tokens = strtok( NULL, SAS_DELIMITERS );
         }
         if (i == 0)
            return( ERROR );
         g_status.sas_arg_pointers[i] = NULL;
         g_status.sas_argc = i;
         g_status.sas_arg  = 0;
         g_status.sas_argv = g_status.sas_arg_pointers;
         g_status.sas_defined = TRUE;
         if (g_status.sas_search_type == BOYER_MOORE) {
            bm.search_defined = sas_bm.search_defined = OK;
            build_boyer_array( );
         } else
            regx.search_defined = sas_regx.search_defined = OK;

         /*
          * free all the previous names
          */
         while (g_status.sas_dta != NULL)
            g_status.sas_dta = next_file( g_status.sas_dta );

         get_current_directory( cwd );
         if (g_status.sas_path != NULL  &&  g_status.sas_path != init_wd)
            free( g_status.sas_path );
         g_status.sas_path = strdup( cwd );
      }
   } else
      bottom_line = g_display.end_line;

   if (g_status.sas_defined > TRUE) {
      g_status.sas_defined = TRUE + 2;
      update_type |= CMDLINE;
      g_option.file_chunk = g_option_all.file_chunk;
   } else {
      g_option.file_mode  = TEXT;
      g_option.file_chunk = (CB_G_Binary) ? -abs( g_option_all.file_chunk )
                                          : MAX_LINE_LENGTH;
   }

   rc = ERROR;
   if (g_status.sas_defined && g_status.sas_arg < g_status.sas_argc) {
      if (win != NULL)
         un_copy_line( win->ll, win, TRUE, TRUE );

      /*
       * make sure the right search type is used
       */
      g_status.search = (g_status.sas_search_type == BOYER_MOORE)
                        ? 0 : SEARCH_REGX;
      if (CB_G_Results) {
         g_status.search |= SEARCH_RESULTS;
         results_window = NULL;
      }

      /*
       * make sure the right directory is used
       */
      get_current_directory( cwd );
      if (g_status.sas_path != NULL)
         set_current_directory( g_status.sas_path );

      /*
       * while we haven't found a valid file, search thru the command
       * line path.
       * we may have an invalid file name when we finish matching all
       * files according to a pattern.  then, we need to go to the next
       * command line argument if it exists.
       */
      SAVE_LINE( bottom_line );
      while (rc == ERROR && g_status.sas_arg < g_status.sas_argc) {

         /*
          * if we haven't starting searching for a file, check to see if
          * the file is a valid file name.  if no file is found, then let's
          * see if we can find according to a search pattern.
          */
         if (g_status.sas_dta == NULL) {
            name = g_status.sas_argv[g_status.sas_arg];
            if (is_glob( name )) {
               if (strstr( name, "..." ))
                  set_prompt( win18, bottom_line );
               g_status.sas_dta = find_files( name );
               if (g_status.sas_dta != NULL) {
                  rc = OK;
                  name = g_status.sas_dta->name;
               } else {
                  ++g_status.sas_arg;
                  if (win != NULL)
                     /*
                      * invalid path or file name
                      */
                     error( WARNING, bottom_line, win8 );
               }
            } else {
               rc = file_exists( name );
               if (rc == READ_ONLY)
                  rc = OK;
               ++g_status.sas_arg;
            }
         } else {

            /*
             * we already found one file with wild card characters,
             * find the next matching file.
             */
            g_status.sas_dta = next_file( g_status.sas_dta );
            rc = (g_status.sas_dta == NULL) ? ERROR : OK;
            if (rc == OK)
               name = g_status.sas_dta->name;
            else
               ++g_status.sas_arg;
         }

         /*
          * if everything is everything so far, set up the file
          * and window structures and bring the file into the editor.
          */
         if (rc == OK) {
            join_strings( line_out, win19, name );
            set_prompt( line_out, bottom_line );
            rc = attempt_edit_display( name, update_type );
            if (rc == OK  &&  CB_G_LoadAll) {
               rc = ERROR;
               if (first_win == NULL)
                  first_win = g_status.current_window;
            }
         }

         /*
          * either there are no more matching files or we had an
          * invalid file name, set rc to ERROR and let's look at the
          * next file name or pattern on the command line.
          */
         else
            rc = ERROR;
      }
      if (!mode.track_path) {
         set_current_directory( cwd );
         if (rc != ERROR  &&  !make_window)
            show_window_fname( g_status.current_window );
      }
      if (rc == ERROR) {
         if (CB_G_Results && results_window != NULL) {
            rc = OK;
            if (win == NULL && first_win == NULL) {
               g_status.current_window = results_window;
               g_status.current_file   = results_file;
               redraw_screen( results_window );
            } else
               change_window( g_status.current_window, results_window );

         } else if (first_win != NULL) {
            rc = OK;
            change_window( g_status.current_window, first_win );
            found_rline = 0;
            show_curl_line( g_status.current_window );

         } else
            RESTORE_LINE( bottom_line );
      }
      g_status.search = 0;
   }
   if (rc == ERROR &&  g_status.sas_arg >= g_status.sas_argc  && win != NULL)
      /*
       * no more files to load
       * pattern not found
       */
      error( WARNING, bottom_line,
             (g_status.command == RepeatGrep) ? win9 : win9b );

   return( rc );
}


/*
 * Name:    grep_proc
 * Purpose: dialog callback for DefineGrep
 * Author:  Jason Hood
 * Date:    November 30, 2003
 * Notes:   verify the pattern if it's regx.
 */
int  grep_proc( int id, char *text )
{
int  rc = OK;

   if (id == IDE_G_PATTERN || id == 0) {
      if (CB_G_RegX) {
         if (id == 0)
            text = get_dlg_text( EF_G_Pattern );
         if (*text != '\0') {
            strcpy( (char *)regx.pattern, text );
            if (build_nfa( ) == ERROR) {
               regx.search_defined = g_status.sas_defined = FALSE;
               rc = IDE_G_PATTERN;
            }
         }
      }
   }
   return( rc );
}


/*
 * Name:     edit_next_file
 * Purpose:  edit next file on command line.
 * Date:     January 6, 1992
 * Modified: December 29, 1996 by Jason Hood
 * Passed:   window:  pointer to current window
 * Notes:    New window replaces old window.  Old window becomes invisible.
 *
 * jmh 981107: pipe now recognizes binary option.
 * jmh 990425: binary option applies to all following files, until "-b-";
 *              "-b" will always use DEFAULT_BIN_LENGTH, not the previous
 *              value of "-b".
 *             add line number to history.
 * jmh 990429: added -r option to load subsequent files read-only; use "-r-"
 *              to open files read-write.
 * jmh 990921: modified line number to also recognize column (line:col) or
 *              binary offset (+offset). Now uses strtol (allowing hex/octal
 *              values), not atoi (which was technically a bug, anyway).
 * jmh 010528: added -t option to specify physical tab size (-t or -t0 means
 *              use deflate mode, -tn sets inflate mode if using deflate, -t-
 *              restores normal behavior).
 * jmh 021021: with the new path tracking behaviour, use the initial TDE
 *              directory to load files.
 * jmh 021023: added -e option to execute a "startup" macro.
 * jmh 021024: handle all the command line options here and display warnings
 *              for unknown options; swapped -L and -L- to remain consistent
 *              (ie: -L will now disable syntax highlighting, -L- will restore
 *               filename matching.)
 * jmh 021028: have -t default to tab size of 8; allow tabs greater than
 *              half the screen (same as config).
 * jmh 021102: if window is NULL, return 1 if no options, 2 if options but
 *              no files.
 * jmh 030303: added -n option to create a scratch window;
 *             use a switch to process the options.
 * jmh 030331: added -c option to title (caption) the window.
 * jmh 030730: added -s option to load a file as scratch;
 *             modified option processing to have "one-shot" options (using
 *              '+') and options after the file (using upper-case or '-' in
 *              the case of position). Note that "one-shot" options with
 *              wildcards will only apply to the first file found; options
 *              after the wildcard will not be processed at all.
 * jmh 030820: ignore empty ("") arguments.
 * jmh 031026: fixed -e option (changed to _CONTROLKEY for global and added
 *              _SHIFTKEY for local).
 * jmh 040715: added -a option to load all files at once.
 */
int  edit_next_file( TDE_WIN *window )
{
char *name = NULL;
char namebuf[PATH_MAX];
int  i;
int  update_type;
register int rc = ERROR;
register TDE_WIN *win;          /* put window pointer in a register */
fattr_t fattr;
char cwd[PATH_MAX];
char *arg;
int  minus;
int  filename;
int  seen_file = FALSE;
TDE_WIN *first_win = NULL;

   win = window;
   update_type = CMDLINE | (win == NULL ? GLOBAL : LOCAL);
   if (g_option_all.read_only)
      update_type |= g_option_all.read_only;
   else if (g_status.viewer_mode)
      update_type |= O_READ_ONLY;

   if (g_status.arg < g_status.argc) {
      if (win != NULL) {
         if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
            return( ERROR );
      }

      /*
       * Restore the initial path, to ensure we can still find
       *  files specified on the command line.
       */
      get_current_directory( cwd );
      set_current_directory( init_wd );

      /*
       * Reset the "local" options to the "global" options.
       */
      g_option = g_option_all;

      /*
       * while we haven't found a valid file, search thru the command
       * line path.
       * we may have an invalid file name when we finish matching all
       * files according to a pattern.  then, we need to go to the next
       * command line argument if it exists.
       */
      while (rc == ERROR  &&  g_status.arg < g_status.argc) {

         /*
          * if we haven't started searching for a file, check to see if
          * the file is a valid file name.  if no file is found, then let's
          * see if we can find according to a search pattern.
          */
         if (g_status.dta == NULL) {

            /*
             * Read the pipe before any other files.
             */
            if (g_status.input_redir) {
               name = "";
               rc = OK;
               filename = TRUE;
            } else
               filename = FALSE;
            for (; g_status.arg < g_status.argc; ++g_status.arg) {

               arg = g_status.argv[g_status.arg];

               i = *arg;
               if (i == '-'  ||  i == '+') {
                  minus = (i == '-');
                  if (filename) {
                     if (bj_islower( arg[1] ) || arg[1] == 'F' || arg[1] == 'G')
                        break;
                     if (!minus) {
                        char j = arg[1];
                        if (j == '-' || j == '+' || j == ':')
                           j = arg[2];
                        if (j >= '0' && j <= '9')
                           break;
                     }
                  }
                  i = bj_tolower( arg[1] );
                  if (i == 'n') {
                     if (filename)
                        break;
                     filename = TRUE;
                     name = "";
                     rc = OK;
                     g_status.command = ScratchWindow;
                     continue;
                  }
                  switch (i) {
                     case 'f':
                     case 'g':
                        /*
                         * search and seize requires at least 4 arg's, eg:
                         *    tde -f findme *.c
                         */
                        if (g_status.argc - g_status.arg >= 3) {
                           mode.search_case = (i == arg[1]) ? IGNORE : MATCH;
                           CB_G_Results = (arg[2] == '=');
                           CB_G_RegX    = (i == 'g');
                           CB_G_LoadAll = g_option.all;
                           CB_G_Binary  = !(g_option.file_mode == TEXT  &&
                                       g_option.file_chunk == MAX_LINE_LENGTH);

                           arg = g_status.argv[++g_status.arg];
                           assert( strlen( arg ) < MAX_COLS );

                           g_status.command = DefineGrep;
                           if (i == 'f') {
                              g_status.sas_search_type = BOYER_MOORE;
                              strcpy( (char *)sas_bm.pattern, arg );
                           } else {
                              g_status.sas_search_type = REG_EXPRESSION;
                              strcpy( (char *)regx.pattern, arg );
                           }
                           add_to_history( arg, &h_find );

                           g_status.sas_argv = g_status.argv + ++g_status.arg;
                           g_status.sas_argc = g_status.argc - g_status.arg;
                           g_status.arg      = g_status.argc;
                           g_status.sas_arg  = 0;
                           g_status.sas_path = init_wd;
                           while (g_status.sas_dta != NULL)
                              g_status.sas_dta = next_file( g_status.sas_dta );

                           if (g_status.sas_search_type == BOYER_MOORE) {
                              g_status.sas_defined = TRUE + 1;
                              bm.search_defined = sas_bm.search_defined = OK;
                              build_boyer_array( );
                              i = OK;
                           } else {
                              i = build_nfa( );
                              if (i == OK) {
                                 g_status.sas_defined = TRUE + 1;
                                 g_status.sas_search_type = REG_EXPRESSION;
                                 regx.search_defined =
                                 sas_regx.search_defined = OK;
                              }
                           }
                           if (i != ERROR)
                              rc = search_and_seize( g_status.current_window );
                        } else {
                           /*
                            * missing pattern or file for grep
                            */
                           g_status.errmsg = ed22;
                        }
                        goto file_found;

                     case 'l':
                        if (arg[2] == '-' && arg[3] == '\0')
                           g_option.language = NULL;
                        else
                           g_option.language = arg + 2;
                        break;

                     case 'b':
                        g_option.file_mode = BINARY;
                        i = arg[2];
                        if (i == '\0')
                           g_option.file_chunk = DEFAULT_BIN_LENGTH;
                        else if (i == '-' && arg[3] == '\0') {
                           g_option.file_mode = TEXT;
                           if (g_option.file_chunk < 0)
                              g_option.file_chunk = -g_option.file_chunk;
                        } else if (i == '!') {
                           g_option.file_mode = TEXT;
                           g_option.file_chunk = MAX_LINE_LENGTH;
                        } else {
                           i = atoi( arg + 2 );
                           if (i >= MAX_LINE_LENGTH) {
                              i = DEFAULT_BIN_LENGTH;
                              /*
                               * invalid binary length
                               */
                              g_status.errmsg = ed23a;
                           }
                           if (i < 0)
                              g_option.file_mode = TEXT;
                           g_option.file_chunk = i;
                        }
                        break;

                     case 'r':
                        update_type &= ~(O_READ_WRITE | O_READ_ONLY);
                        g_option.read_only = (arg[2] == '-') ? O_READ_WRITE
                                                             : O_READ_ONLY;
                        update_type |= g_option.read_only;
                        break;

                     case 's':
                        g_option.scratch = (arg[2] == '-') ? 0 : -1;
                        break;

                     case 't':
                        i = arg[2];
                        if (i == '\0')
                           g_option.tab_size = 8;
                        else if (i == '-')
                           g_option.tab_size = -1;
                        else {
                           i = atoi( arg + 2 );
                           if (!bj_isdigit(arg[2])  ||
                               i < 0 || i > MAX_TAB_SIZE) {
                              /*
                               * invalid tab size
                               */
                              g_status.errmsg = ed23b;
                           } else
                              g_option.tab_size = i;
                        }
                        break;

                     case 'e':
                        g_option.macro = (minus) ? _CONTROLKEY : _SHIFTKEY;
                        i = arg[2];
                        if (i == '-') {
                           clear_previous_macro( g_option.macro );
                           g_option.macro = 0;
                        } else {
                           if (i == '\0')
                              arg = g_status.argv[++g_status.arg];
                           else
                              arg += 2;
                           if (arg != NULL) {
                              parse_cmdline_macro( arg, (win == NULL)
                                                        ? g_display.end_line
                                                        : win->bottom_line );
                           } else {
                              /*
                               * missing macro
                               */
                              g_status.errmsg = ed23c;
                           }
                        }
                        break;

                     case 'c':
                        if (arg[2] == '-' && arg[3] == '\0')
                           cmd_title = NULL;
                        else if (arg[2] != '\0')
                           cmd_title = arg + 2;
                        else
                           cmd_title = g_status.argv[++g_status.arg];
                        break;

                     case 'a':
                        g_option.all = (arg[2] != '-');
                        if (!g_option.all  &&  first_win != NULL)
                           goto file_found;
                        break;

                     default:
                        if (i == '-' || i == '+' || i == ':')
                           i = arg[2];
                        if (i >= '0' && i <= '9') {
                         char *colon;
                         long num;
                           num = strtol( arg + 1, &colon, 0 );
                           if (arg[1] == '+')
                              g_status.jump_off = num;
                           else {
                              g_status.jump_to = num;
                              if (*colon == ':') {
                                 ++colon;
                                 i = (*colon == '-') ? colon[1] : *colon;
                                 if (i >= '0' && i <= '9')
                                    g_status.jump_col = atoi( colon );
                              }
                           }
                           add_to_history( arg + 1, &h_line );
                        } else {
                           /*
                            * unknown option
                            */
                           g_status.errmsg = ed23;
                        }
                        break;
                  }
                  if (minus)
                     g_option_all = g_option;

                  if (g_status.errmsg) {
                     if (win != NULL) {
                        error( WARNING, win->bottom_line, g_status.errmsg );
                        g_status.errmsg = NULL;
                     } else
                        return( ERROR );
                  }
                  continue;
               }
               else if (i == '\0')
                 continue;
               else if (filename)
                  break;

               seen_file = filename = TRUE;
               name = arg;
               if (file_exists( name ) == SUBDIRECTORY) {
                  strcpy( namebuf, name );
                  name = namebuf;
                  rc = list_and_pick( name, win );
               } else if (is_glob( name )) {
                  if (strstr( name, "..." ))
                     set_prompt( win18, win == NULL ? g_display.end_line
                                                    : win->bottom_line );
                  g_status.dta = find_files( name );
                  if (g_status.dta != NULL) {
                     name = g_status.dta->name;
                     rc = OK;
                     /* point back to it; makes it easier to find the rest */
                     --g_status.arg;
                  } else {
                     if (win != NULL)
                        /*
                         * invalid path or file name
                         */
                        error( WARNING, win->bottom_line, win8 );
                  }
               } else {
                  rc = get_fattr( name, &fattr );
                  /*
                   * a new or blank (unfound) file generates a return code of 2.
                   * an invalid file (bad name) generates a return code of 3.
                   */
                  if (rc == 2)
                     rc = OK;
               }
            }
         } else {

            /*
             * we already found one file with wild card characters,
             * find the next matching file.
             */
            g_status.dta = next_file( g_status.dta );
            rc = (g_status.dta == NULL) ? ERROR : OK;
            if (rc == OK)
               name = g_status.dta->name;
            else
               ++g_status.arg;
         }

         /*
          * if everything is everything so far, set up the file
          * and window structures and bring the file into the editor.
          */
         if (rc == OK) {

            assert( strlen( name ) < PATH_MAX );

            rc = attempt_edit_display( name, update_type );
            if (g_option.all  &&  rc == OK) {
               rc = ERROR;
               if (first_win == NULL)
                  first_win = g_status.current_window;
            }
         }

         /*
          * either there are no more matching files or we had an
          * invalid file name, set rc to ERROR and let's look at the
          * next file name or pattern on the command line.
          */
         else
            rc = ERROR;
      }
   file_found:
      /*
       * Restore the directory and update the window's filename.
       */
      if (!mode.track_path) {
         set_current_directory( cwd );
         if (rc != ERROR  &&  !g_status.stop  &&  !make_window)
            show_window_fname( g_status.current_window );
      }
      if (first_win != NULL) {
         rc = OK;
         change_window( g_status.current_window, first_win );
      }
   } else
      if (win == NULL)
         rc = 1;

   if (rc == ERROR) {
      if (g_status.sas_defined) {
         if (win != NULL)
            /*
             * use <Grep> key to load files (jmh)
             */
            error( WARNING, win->bottom_line, win9a );

      } else if (g_status.arg >= g_status.argc) {
         if (win != NULL)
            /*
             * no more files to load
             */
            error( WARNING, win->bottom_line, win9 );
         else if (!seen_file)
            rc = 2;
      }
   }
   if (g_status.stop)
      rc = ERROR;
   return( rc );
}


/*
 * Name:    file_exists
 * Purpose: to determine if a file exists
 * Author:  Jason Hood
 * Date:    October 20, 2002
 * Passed:  name:  name of the file
 * Returns: ERROR:        the file does not exist
 *          OK:           the file exists
 *          READ_ONLY:    the file exists and is read-only
 *          SUBDIRECTORY: the file exists and is a directory (or special)
 * Notes:   replaces hw_fattrib() in port.c.
 */
int  file_exists( char *name )
{
fattr_t fattr;
int  rc;

   rc = get_fattr( name, &fattr );
   if (rc == OK) {
#if defined( __UNIX__ )
      if (!S_ISREG( fattr ))
         rc = SUBDIRECTORY;
      else if (!(fattr & (S_IWUSR | S_IWGRP | S_IWOTH)))
         rc = READ_ONLY;
#else
      if (fattr & SUBDIRECTORY)
         rc = SUBDIRECTORY;
      else if (fattr & READ_ONLY)
         rc = READ_ONLY;
#endif
   } else
      rc = ERROR;
   return( rc );
}


/*
 * Name:    change_mode
 * Purpose: To prompt for file access mode.
 * Date:    January 11, 1992
 * Passed:  name:  name of file
 *          line:  line to display message
 * Returns: OK if file could be changed
 *          ERROR otherwise
 * Notes:   function is used to change file attributes for save_as function.
 *
 * jmh 021020: moved from port.c;
 *             preserve the other attributes.
 */
int  change_mode( char *name, int line )
{
fattr_t fattr;
register int rc;

   if (file_exists( name ) == READ_ONLY) {
      /*
       * file is write protected. overwrite anyway?
       */
      rc = ERROR;
      if (get_yn( main6, line, R_PROMPT | R_ABORT ) == A_YES) {
         if (get_fattr( name, &fattr ) == OK)
#if defined( __UNIX__ )
            rc = set_fattr( name, fattr | (S_IWUSR | S_IWGRP | S_IWOTH) );
#else
            rc = set_fattr( name, fattr & ~READ_ONLY );
#endif
      }
   } else
      rc = OK;
   return( rc );
}


/*
 * Name:    binary_file
 * Purpose: To determine if a file is binary
 * Date:    November 27, 1998
 * Passed:  name: name of the file to test
 * Returns: TRUE for a binary file, FALSE for a text file, ERROR if aborted.
 * Notes:   Examines the first 256 bytes for "binary" characters. If one is
 *           found, ask for the line length (record size).
 *          If there's an error reading the file, return FALSE and let the
 *           caller handle the problem.
 * jmh 010521: recognize EOF and ESC as text.
 * jmh 010522: return ERROR if length entry was cancelled.
 * jmh 030922: recognize backspace as text.
 * jmh 050816: only say binary if more than 5% is binary, testing 1024 bytes.
 * jmh 060828: assume stdin is not binary (avoid reading it twice).
 */
int  binary_file( char *name )
{
FILE *file;
char data[1024];
long len;
int  j;
int  rc = FALSE;
int  bin = 0, asc = 0;

   if (*name == '\0')
      return FALSE;

   if ((file = fopen( name, "rb" )) != NULL) {
      j = fread( data, sizeof(char), sizeof(data), file );
      while (--j >= 0) {
         if (bj_isalnum(data[j]) || bj_ispunct(data[j]) || bj_isspace(data[j])
             || data[j] == '\b' || data[j] == '\x1a' || data[j] == '\x1b')
            ++asc;
         else
            ++bin;
      }
      fclose( file );
      if (bin > (asc >> 4))
         rc = (g_option.file_chunk == MAX_LINE_LENGTH) ? ERROR : TRUE;
   }

   if (rc == TRUE) {
      len = g_option.file_chunk;
      if (len < 0)
         g_option.file_chunk = (int)-len;
      else {
         /*
          * BINARY line length (0 for text) :
          */
         rc = get_number( ed18, (g_status.current_window == NULL)
                                 ? g_display.end_line
                                 : g_status.current_window->bottom_line,
                          &len, NULL );
         if (rc == OK) {
            if (len < 0 || len >= MAX_LINE_LENGTH)
               len = DEFAULT_BIN_LENGTH;
            g_option.file_chunk = (int)len;
            rc = (len != 0);
         }
      }
   }

   return( rc );
}


/*
 * Name:    change_fattr
 * Purpose: To change the file attributes
 * Date:    December 31, 1991
 * Passed:  window:  pointer to current window
 *
 * jmh 991128: moved from *\port.c; use portable process_fattr(), instead.
 * jmh 021020: moved process_fattr() directly into here:
 *              restored the loop;
 *              UNIX: if the first character is a digit, assume the octal
 *               representation (eg: 644 --> rw-r--r--)
 * jmh 031123: ignore scratch windows (that aren't files).
 */
int  change_fattr( TDE_WIN *window )
{
file_infos *file;
fattr_t fattr;
TDE_WIN *wp;
char *p;
int  rc;
#if defined( __UNIX__ )
int  j;
#endif

   file = window->file_info;
   if (*file->file_name == '\0')
      return( OK );

   fattr = file->file_attrib;
#if defined( __UNIX__ )
   for (j = 0; j < 9; ++j)
      CB_Attr( j ) = (fattr & (1 << (8 - j))) ? TRUE : FALSE;
#else
   CB_Archive  = (fattr & ARCHIVE)   ? TRUE : FALSE;
   CB_System   = (fattr & SYSTEM)    ? TRUE : FALSE;
   CB_Hidden   = (fattr & HIDDEN)    ? TRUE : FALSE;
   CB_Readonly = (fattr & READ_ONLY) ? TRUE : FALSE;
#endif

   rc = do_dialog( fattr_dialog, fattr_proc );
   if (rc == OK) {
      p = get_dlg_text( EF_Attr );

#if defined( __UNIX__ )
      /*
       * turn off rwx for user, group, and other.
       */
      fattr &= ~(S_IRWXU | S_IRWXG | S_IRWXO);

      if (*p >= '0' && *p <= '7')
         fattr |= strtol( p, NULL, 8 );
      else if (*p == '\0') {
         for (j = 0; j < 9; ++j)
            fattr |= CB_Attr( j ) << (8 - j);
      } else do {
         switch (bj_toupper( *p )) {
            case L_UNIX_READ    : fattr |= S_IRUSR | S_IRGRP | S_IROTH; break;
            case L_UNIX_WRITE   : fattr |= S_IWUSR | S_IWGRP | S_IWOTH; break;
            case L_UNIX_EXECUTE : fattr |= S_IXUSR | S_IXGRP | S_IXOTH; break;
         }
      } while (*++p);
#else
      fattr = 0;
      if (*p == '\0') {
         if (CB_Archive)  fattr |= ARCHIVE;
         if (CB_System)   fattr |= SYSTEM;
         if (CB_Hidden)   fattr |= HIDDEN;
         if (CB_Readonly) fattr |= READ_ONLY;
      } else do {
         switch (bj_toupper( *p )) {
            case L_DOS_ARCHIVE   : fattr |= ARCHIVE;   break;
            case L_DOS_SYSTEM    : fattr |= SYSTEM;    break;
            case L_DOS_HIDDEN    : fattr |= HIDDEN;    break;
            case L_DOS_READ_ONLY : fattr |= READ_ONLY; break;
         }
      } while (*++p);
#endif

      if (set_fattr( file->file_name, fattr )) {
         /*
          * new file attributes not set
          */
         error( WARNING, window->bottom_line, utils15 );
         rc = ERROR;
      } else {
         file->file_attrib = fattr;
         for (wp = g_status.window_list; wp != NULL; wp = wp->next) {
            if (wp->file_info == file && wp->visible)
               show_window_fname( wp );
         }
      }
   }
   return( rc );
}


/*
 * Name:    fattr_proc
 * Purpose: dialog callback for FileAttributes
 * Author:  Jason Hood
 * Date:    December 1, 2003
 * Notes:   disable all check boxes if a string has been entered.
 */
int  fattr_proc( int id, char *text )
{
int  state;
int  i;

   if (id == -IDE_ATTR) {
      state = (*text == '\0');
      for (i = IDC_ATTR; i < IDC_ATTR + NUM_ATTR; ++i)
         check_box_enabled( i, state );
   }
   return( OK );
}


/*
 * Name:    str_fattr
 * Purpose: convert a file attribute to a string
 * Author:  Jason Hood
 * Date:    October 20, 2002
 * Passed:  buffer:  place to store string
 *          fattr:   attribute to convert
 * Returns: buffer
 * Notes:   UNIX only uses the user attributes.
 */
char *str_fattr( char *buffer, fattr_t fattr )
{
char *p = buffer;

#if defined( __UNIX__ )
   *p++ =  fattr & S_IRUSR   ? L_UNIX_READ     : '-';
   *p++ =  fattr & S_IWUSR   ? L_UNIX_WRITE    : '-';
   *p++ =  fattr & S_IXUSR   ? L_UNIX_EXECUTE  : '-';
#else
   *p++ =  fattr & ARCHIVE   ? L_DOS_ARCHIVE   : '-';
   *p++ =  fattr & SYSTEM    ? L_DOS_SYSTEM    : '-';
   *p++ =  fattr & HIDDEN    ? L_DOS_HIDDEN    : '-';
   *p++ =  fattr & READ_ONLY ? L_DOS_READ_ONLY : '-';
#endif
   *p   = '\0';

   return( buffer );
}


/*
 * Name:    save_workspace
 * Purpose: save the currently loaded files and their cursor position
 * Author:  Jason Hood
 * Date:    July 22, 2002
 * Passed:  window:  pointer to current window
 * Notes:   does not save the pipe or scratch windows, which may cause problems
 *           if it's part of a split window.
 *
 * 020911:  only prompt if called from SaveWorkspace.
 * 021029:  save all the histories;
 *          once the workspace has been saved, continue to auto-save it.
 * 021031:  if auto-saving without having loaded a workspace,
 *           create a global workspace.
 * 050708:  save scratch windows and files.
 */
int  save_workspace( TDE_WIN *window )
{
FILE *fp;
TDE_WIN *wp;
file_infos *file;
int  *files;
int  j;
char answer[PATH_MAX];
int  rc = OK;

   if (g_status.command == SaveWorkspace) {
      strcpy( answer, wksp_file );
      /*
       * save workspace to:
       */
      rc = get_name( ed24, g_display.end_line, answer, NULL );
      if (rc <= 0)
         return( ERROR );

      wksp_loaded = TRUE;
   } else if (wksp_loaded)
      strcpy( answer, wksp_file );
   else
      join_strings( answer, g_status.argv[0], WORKSPACEFILE );

   fp = fopen( answer, "w" );
   if (fp != NULL) {
      fprintf( fp, "%s\n", WORKSPACE_SIG );

      get_full_path( answer, wksp_file );
      files = my_calloc( g_status.file_count * sizeof(int) );
      fprintf( fp, "%d\n", g_status.file_count );
      for (wp = g_status.window_list; wp != NULL; wp = wp->next) {
         file = wp->file_info;
         fprintf( fp, "%c %ld:%d %d:%d:%d %d:%d:%d:%d:%d %d:%d:%d %s\n",
                  (wp == window) + ' ',
                  wp->rline, wp->rcol + 1, wp->cline, wp->ccol, wp->bcol,
                  wp->top, wp->left, wp->bottom_line, wp->end_col,
                  wp->vertical, wp->visible, wp->ruler, wp->syntax,
                  (wp->title) ? wp->title : "" );
         fprintf( fp, "%d", file->file_no );
         j = FALSE;
         if (files == NULL) {
            if (file->ref_count > 1) {
             TDE_WIN *win;
               for (win = wp->prev; win != NULL; win = win->prev) {
                  if (win->file_info == file) {
                     j = TRUE;
                     break;
                  }
               }
            }
         } else {
            j = files[file->file_no-1];
            files[file->file_no-1] = TRUE;
         }
         if (!j) {
            fprintf( fp, " %d %d:%d:%d %d:%d:%d:%ld:%d:%ld:%ld %s\n",
                     (file->crlf == BINARY) ? file->binary_len : 0,
                     file->inflate_tabs, file->ptab_size, file->ltab_size,
                     (g_status.marked && g_status.marked_file == file),
                     file->block_type, file->block_bc, file->block_br,
                     file->block_ec, file->block_er, file->break_point,
                     (file->syntax) ? file->syntax->name : "" );
            for (j = 0; j < NO_MARKERS; ++j) {
               fprintf( fp, " %ld:%d:%d:%d:%d:%d",
                        file->marker[j].rline, file->marker[j].cline,
                        file->marker[j].rcol, file->marker[j].ccol,
                        file->marker[j].bcol, file->marker[j].marked );
            }
            if (*file->file_name)
               fprintf( fp, "\n%c%s",
                            ' ' + (file->read_only) + (file->scratch & 4),
                            file->file_name );
            else
               fprintf( fp, "\n#%d%c%s", file->scratch,
                            ' ' + (file->read_only), file->file_name+1 );
         }
         fputc( '\n', fp );
      }
      my_free( files );

      write_history( fp, &h_file );
      write_history( fp, &h_find );
      write_history( fp, &h_border );
      write_history( fp, &h_stamp );
      write_history( fp, &h_line );
      write_history( fp, &h_lang );
      write_history( fp, &h_grep );
      write_history( fp, &h_win );
      write_history( fp, &h_exec );
      write_history( fp, &h_parm );
      write_history( fp, &h_fill );

      fclose( fp );
   } else {
      /*
       * cannot write to
       */
      combine_strings( line_out, utils8, answer, "'" );
      error( WARNING, g_display.mode_line, line_out );
      rc = ERROR;
   }
   return( rc );
}


/*
 * Name:    load_workspace
 * Purpose: load previously saved files into TDE
 * Author:  Jason Hood
 * Date:    July 22, 2002
 * Returns: FALSE if there is no workspace file;
 *          TRUE if the workspace was loaded;
 *          ERROR if the file is not a workspace.
 * Notes:   reads the workspace from the current directory.
 *          only used if no files were specified on the command line.
 * 020817:  if an invalid signature is detected, set g_status.errmsg.
 * 020911:  set the loaded flag, but only if everything loaded OK.
 * 021029:  load all the histories.
 * 021210:  fixed problem loading a removed (or externally edited) file (more
 *           cline troubles).
 * 031030:  fixed problem when the current window was scratch.
 * 050708:  restore timestamp, scratch windows and files.
 */
int  load_workspace( void )
{
FILE *fp;
int  curwin, update, block, vis = FALSE;
TDE_WIN win;
file_infos file;
file_infos **files, *fi;
char language[MAX_COLS+2];
char title[MAX_COLS+2];
TDE_WIN *current = NULL, *previous = NULL;
int  loaded;
int  j, len;
char *pos;
int  scratch;

   fp = fopen( wksp_file, "r" );
   if (fp == NULL)
      return( FALSE );

   read_line( fp );
   if (strcmp( line_in, WORKSPACE_SIG ) != 0) {
      g_status.errmsg = ed25;
      return( ERROR );
   }

   wksp_loaded = TRUE;

   read_line( fp );
   sscanf( line_in, "%d", &len );
   files = my_malloc( len * sizeof(file_infos), &j );

   g_status.screen_display = FALSE;
   update = GLOBAL | CMDLINE;
   g_option.file_mode = BINARY;
   while (read_line( fp ) != EOF  &&  *line_in != '\0') {
      curwin = (*line_in != ' ');
#if defined( __TURBOC__ )
      /*
       * it seems Borland has a bug in processing sscanf - when it gets
       * to the end of the string, it apparently ignores the rest of the
       * format, forgetting that %n is still valid.
       */
      len = strlen( line_in+1 );
#endif
      sscanf( line_in+1, "%ld:%d %d:%d:%d %d:%d:%d:%d:%d %d:%d:%d %n",
              &g_status.jump_to, &g_status.jump_col,
              &win.cline, &win.ccol, &win.bcol,
              &win.top, &win.left, &win.bottom_line, &win.end_col,
              &win.vertical, &win.visible, &win.ruler, &win.syntax, &len );
      if (line_in[1+len]) {

         assert( strlen( line_in+1 + len ) < MAX_COLS );

         strcpy( title, line_in+1 + len );
         cmd_title = title;
      } else
         cmd_title = NULL;

      read_line( fp );
      loaded = FALSE;
      pos = NULL;
      scratch = 0;
      g_option.scratch = 0;
#if defined( __TURBOC__ )
      len = strlen( line_in );
#endif
      if (sscanf( line_in, "%d %d %d:%d:%d %d:%d:%d:%ld:%d:%ld:%ld %n",
                  &file.file_no,
                  &g_option.file_chunk,
                  &file.inflate_tabs, &file.ptab_size, &file.ltab_size,
                  &block, &file.block_type, &file.block_bc, &file.block_br,
                  &file.block_ec, &file.block_er, &file.break_point,
                  &len ) == 1) {
         loaded = TRUE;
         if (files == NULL) {
            fi = g_status.current_file;
            while (fi != NULL  &&  fi->file_no != file.file_no)
               fi = fi->prev;
         } else
            fi = files[file.file_no-1];
         if (fi == NULL)
            loaded = ERROR;
         else
            g_status.current_file = fi;
      } else {
         if (line_in[len]) {

            assert( strlen( line_in + len ) < MAX_COLS );

            strcpy( language, line_in + len );
            g_option.language = language;
         } else
            g_option.language = NULL;
         read_line( fp );
         pos = line_in;
         for (j = 0; j < NO_MARKERS; ++j) {
            sscanf( pos, "%ld:%d:%d:%d:%d:%d%n",
                    &file.marker[j].rline, &file.marker[j].cline,
                    &file.marker[j].rcol, &file.marker[j].ccol,
                    &file.marker[j].bcol, &file.marker[j].marked, &len );
            pos += len;
         }
         read_line( fp );
         if (*line_in == '#') {
            scratch = (int)strtol( line_in+1, &pos, 10 );
            g_status.command = ScratchWindow;
         } else
            pos = line_in;
         if (*pos & 1)
            update |= O_READ_ONLY;
         if (*pos & 4)
            g_option.scratch = -1;
         ++pos;
      }
      if (loaded != ERROR  &&
          attempt_edit_display( (scratch) ? "" : pos, update ) != ERROR) {
         if (previous != NULL)
            previous->visible = vis;
         previous = g_status.current_window;
         vis = win.visible;
         g_status.current_window->ccol = win.ccol;
         g_status.current_window->bcol = win.bcol;
         g_status.current_window->top = win.top;
         g_status.current_window->left = win.left;
         g_status.current_window->bottom_line = win.bottom_line;
         g_status.current_window->end_col = win.end_col;
         g_status.current_window->vertical = win.vertical;
         g_status.current_window->visible = win.visible;
         g_status.current_window->ruler = win.ruler;
         if (g_status.current_file->syntax)
            g_status.current_window->syntax = win.syntax;
         setup_window( g_status.current_window );
         check_cline( g_status.current_window, win.cline );
         if (!loaded) {
            files[file.file_no-1] = g_status.current_file;
            g_status.current_file->file_no = file.file_no;
            g_status.current_file->inflate_tabs = file.inflate_tabs;
            g_status.current_file->ptab_size = file.ptab_size;
            g_status.current_file->ltab_size = file.ltab_size;
            g_status.current_file->block_type = file.block_type;
            g_status.current_file->block_bc = file.block_bc;
            g_status.current_file->block_br = file.block_br;
            g_status.current_file->block_ec = file.block_ec;
            g_status.current_file->block_er = file.block_er;
            g_status.current_file->break_point = file.break_point;
            for (j = 0; j < NO_MARKERS; ++j)
               g_status.current_file->marker[j] = file.marker[j];
            if (block) {
               g_status.marked = TRUE;
               g_status.marked_file = g_status.current_file;
            }
            if (scratch) {
               g_status.current_file->scratch = scratch;
               strcpy( g_status.current_file->file_name + 1, pos );
               if (scratch == -1)
                  --g_status.scratch_count;
            }
         }

         if (curwin)
            current = g_status.current_window;
         update = LOCAL | CMDLINE;
      } else
         wksp_loaded = ERROR;
   }
   my_free( files );

   read_history( fp, &h_file );
   read_history( fp, &h_find );
   read_history( fp, &h_border );
   read_history( fp, &h_stamp );
   read_history( fp, &h_line );
   read_history( fp, &h_lang );
   read_history( fp, &h_grep );
   read_history( fp, &h_win );
   read_history( fp, &h_exec );
   read_history( fp, &h_parm );
   read_history( fp, &h_fill );

   fclose( fp );

   strcpy( mode.stamp, h_stamp.prev->str );

   if (current)
      change_window( g_status.current_window, current );
   else
      g_status.current_window->visible = TRUE;
   g_status.screen_display = TRUE;
   redraw_screen( g_status.current_window );
   return( TRUE );
}


/*
 * Name:    write_history
 * Purpose: save a history list to file
 * Author:  Jason Hood
 * Date:    October 29, 2002
 * Passed:  file:  the file to write to
 *          hist:  the history to write
 * Notes:   uses the constant blank line as a separator
 */
static void write_history( FILE *file, HISTORY *hist )
{
HISTORY *begin = hist;

   do {
      fprintf( file, "%s\n", hist->str );
      hist = hist->next;
   } while (hist != begin);
}


/*
 * Name:    read_history
 * Purpose: load a history list from file
 * Author:  Jason Hood
 * Date:    October 29, 2002
 * Passed:  file:  the file to read from
 *          hist:  the history to read
 * Notes:   reads from the current line up to the next blank line, or EOF.
 */
static void read_history( FILE *file, HISTORY *hist )
{
   while (read_line( file ) != EOF  &&  *line_in != '\0')
      add_to_history( line_in, hist );
}


/*
 * Name:    set_path
 * Purpose: set cwd to that of the file in window
 * Author:  Jason Hood per David J Hughes
 * Date:    October 21, 2002
 * Passed:  window:  pointer to (current) window
 * Notes:   used for TrackPath config setting and SetDirectory function.
 *          since the pipe has no path, leave cwd as-is.
 *
 * jmh 031117: if window is NULL, don't change dir, but update all headers.
 */
int  set_path( TDE_WIN *window )
{
char *name;
char ch;

   /*
    * If both are true, or neither are, no need to set the path
    *  (if both, TrackPath has already done it; if neither,
    *   it doesn't need to be done).
    */
   if (window == NULL  ||
       ((mode.track_path  ^  (g_status.command == SetDirectory))  &&
        *(name = window->file_info->fname))) {
      if (window != NULL) {
         ch = *name;
         *name = '\0';
         set_current_directory( window->file_info->file_name );
         *name = ch;
      }
      /*
       * Re-display all the windows' filenames, reflecting the new path.
       */
      for (window = g_status.window_list; window != NULL; window = window->next)
         if (window->visible)
            show_window_fname( window );

      if (mode.display_cwd)
         show_cwd( );
   }
   return( OK );
}
