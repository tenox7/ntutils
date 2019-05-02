/*
 * I wrote this function because I'm so stupid, I constantly forget
 *  file names and directory stuff.  The main function prompts for a
 *  subdirectory name or a search path.  The default search path is the
 *  cwd (current working directory).  In addition to being stupid, I'm also
 *  lazy.  If the user types a subdirectory name, I think we can assume he
 *  wants to list all files w/o having to type *.*   Let's save the cwd on
 *  whatever drive the user wishes to search, so we can restore it we get
 *  thru dir'ing.  Use the standard DOS functions to get and set directories.
 *
 * The search pattern can contain wild card chars, valid file names, or a
 *  valid subdirectory name.
 *
 * Being that TDE 2.2 now handles binary files, lets make .EXE and .COM files
 *  autoload in binary mode.
 *
 * Before matching files are displayed on the screen, file names are sorted
 *  using the easy-to-implement and fairly fast Shellsort algorithm.
 *
 * See:
 *
 *   Donald Lewis Shell, "A High-Speed Sorting Procedure."  _Communications of
 *     the ACM_ 2 (No. 2): 30-32, 1959.
 *
 * See also:
 *
 *   Donald Ervin Knuth, _The Art of Computer Programming; Volume 3:  Sorting
 *     and Searching_, Addison-Wesley, Reading, Mass., 1973, Chapter 5,
 *     "Sorting", pp 84-95.  ISBN 0-201-03803-X.
 *
 *   Robert Sedgewick, _Algorithms in C_, Addison-Wesley, Reading, Mass.,
 *     1990, Chapter 8, "Elementary Sorting Methods", pp 107-111.
 *     ISBN 0-201-51425-7.
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
 *    You may distribute it freely.
 */

#include "tdestr.h"
#include "common.h"
#include "define.h"
#include "tdefunc.h"
#include <time.h>


static int  row_ofs;                    /* row of first name (jmh 060830) */
static int  fofs;                       /* offset in the file list for the */
                                        /*  first file (jmh 980523)        */

static int  win_cmp( char *, char * );  /* GotoWindow sort function */
static int  shl_cmp( char *, char * );  /* SyntaxSelect sort function */
static int  name_ofs;                   /* GotoWindow offset to window name */


/*
 * Name:    dir_help
 * Purpose: To prompt the user and list the directory contents
 * Date:    February 13, 1992
 * Passed:  window:  pointer to current window
 *
 * jmh 980528: disable command line language;
 *             use return code of attempt_edit_display.
 * jmh 020723: auto-detect binary files (disable command line -b).
 * jmh 021023: handle command line options in attempt_edit_display().
 */
int  dir_help( TDE_WIN *window )
{
char dname[PATH_MAX+2]; /* directory search pattern */
int  rc;
int  prompt_line;

   if (window != NULL) {
      if (un_copy_line( window->ll, window, TRUE, TRUE ) == ERROR)
         return( ERROR );
      prompt_line = window->bottom_line;
   } else
      prompt_line = g_display.end_line;

   /*
    * search path or pattern
    */
   dname[0] = '\0';
   rc = get_name( dir1, prompt_line, dname, &h_file );

   if (rc != ERROR) {
      rc = list_and_pick( dname, window );

      /*
       * if everything is everything, load in the file selected by user.
       */
      if (rc == OK)
         rc = attempt_edit_display( dname, (window != NULL) ? LOCAL : GLOBAL );
   }
   return( rc );
}


/*
 * Name:    dir_help_name
 * Purpose: To display name of directory
 * Date:    February 13, 1992
 * Passed:  window:  pointer to current window
 */
int  dir_help_name( TDE_WIN *window, char *name )
{
char dname[PATH_MAX+2]; /* directory search pattern */
int  rc;

   rc = OK;
   if (window != NULL) {
      if (un_copy_line( window->ll, window, TRUE, TRUE ) == ERROR)
         return( ERROR );
   }

   strcpy( dname, name );
   rc = list_and_pick( dname, window );

   /*
    * if everything is everything, load in the file selected by user.
    */
   if (rc == OK)
      rc = attempt_edit_display( dname, (window != NULL) ? LOCAL : GLOBAL );

   return( rc );
}


/*
 * Name:    list_and_pick
 * Purpose: To show matching file names and let user pick a file
 * Date:    February 13, 1992
 * Passed:  dname:   directory search pattern
 *          window:  pointer to current window
 * Returns: return code from pick.  rc = OK, then edit a new file.
 * Notes:   real work routine of this function.  save the cwd and let the
 *           user search upwards or downwards thru the directory structure.
 *          since we are doing DOS directory functions, we need to check the
 *           return code after each DOS call for critical errors.
 *
 * jmh 020924: made FTYPE.fname a pointer, allocating exactly as much as space
 *              as required. Tweaked the loop to code the allocation once.
 * jmh 021019: display the full path, rather than what was initially entered;
 *             don't actually change directory (use the stem);
 *             rather than abort, don't allow selection of an empty directory.
 */
int  list_and_pick( char *dname, TDE_WIN *window )
{
int  rc = OK;
FFIND dta;              /* file finding info */
DIRECTORY dir;          /* contains all info for dir display */
unsigned int cnt;       /* number of matching files */
LIST *flist, *pl;       /* pointer to list of matching files */
FTYPE *pf, *ffind;
char dbuff[PATH_MAX];   /* temporary directory buff */
int  stop;
int  len;
char *n;
int  prompt_line;

   prompt_line = (window != NULL) ? window->bottom_line : g_display.end_line;

   ffind = my_findfirst( dname, &dta, ALL_DIRS );
   if (ffind == NULL) {
      error( WARNING, prompt_line, dir2 );
      return( ERROR );
   }

   xygoto( -1, -1 );

   stop = FALSE;
   while (stop == FALSE) {
      /*
       * Some algorithms alloc the maximum possible number of files in
       *  a directory, eg. 256 or 512.  Let's count the number of matching
       *  files so we know exactly how much memory to request from malloc.
       */
      cnt = len = 0;
      do {
         ++cnt;
         len += strlen( ffind->fname ) + 1;
         ffind = my_findnext( &dta );
      } while (ffind != NULL);
      flist = malloc( cnt * (sizeof(LIST) + sizeof(FTYPE)) + len );
      if (flist == NULL) {
         /*
          * out of memory
          */
         error( WARNING, prompt_line, main4 );
         rc = ERROR;
         break;
      }
      /*
       * If we had enough memory, find all matching file names.
       *
       * when we get here, we have already done: 1) my_findfirst and
       *  my_findnext, 2) counted the number of matching files, and
       *  3) allocated space.
       */
      pl = flist;
      pf = (FTYPE *)(pl + cnt);
      n  = (char *)(pf + cnt);
      ffind = my_findfirst( dname, &dta, ALL_DIRS );
      do {

         /*
          * pl is pointer that walks down the list info structure.
          * pf is pointer that walks down the file info structure.
          *  save the file name and file size for each matching
          *  file we find. jmh 980527: save attribute as well.
          *  jmh 031202: save time.
          */

         pl->name  = pf->fname = n;
         pl->color =
#if !defined( __UNIX__ )
                     (ffind->fattr & (HIDDEN | SYSTEM)) ? Color( Disabled ) :
#endif
                      Color( Dialog );
         pl->data  = pf;
         ++pl;
#if defined( __DJGPP__ ) || defined( __TURBOC__ )
         n = stpcpy( pf->fname, ffind->fname ) + 1;
#else
         strcpy( pf->fname, ffind->fname );
         n += strlen( pf->fname ) + 1;
#endif
         pf->fsize = ffind->fsize;
         pf->fattr = ffind->fattr;
         pf->ftime = ffind->ftime;
         ++pf;
         ffind = my_findnext( &dta );
      } while (ffind != NULL);

      shell_sort( flist, cnt );

      /*
       * figure out number of rows, cols, etc... then display dir list
       */
      setup_directory_window( &dir, flist, cnt );
      write_directory_list( flist, &dir );

#if !defined( __UNIX__ )
      /*
       * If only a drive was specified, append the current directory,
       *  to have the trailing slash for get_full_path().
       */
      if (dta.stem[1] == ':' && dta.stem[2] == '\0')
         strcpy( dta.stem + 2, "./" );
#endif
      /*
       * Let user select file name or another search directory.
       */
      get_full_path( dta.stem, dname );
      strcat( dname, dta.pattern );
      rc = select_file( flist, dname, &dir );

      if (rc == OK) {
         assert( strlen(dta.stem)+strlen(flist[dir.select].name) < PATH_MAX );
         get_full_path( dta.stem, dbuff );
         strcat( dbuff, flist[dir.select].name );
      }

      /*
       *  give memory back.
       */
      free( flist );

      if (rc == ERROR)
         stop = TRUE;

      else if (rc == TRUE) {
         /*
          * Prompt for a new path/pattern.
          */
         strcpy( dbuff, dname );
         len = DIR3_COL + strlen( dir3 );
         if (get_string( dir.col+len, dir.row+DIR3_ROW,
                         dir.wid - len - 2, Color( Dialog ), '_',
                         dbuff, &h_file ) == ERROR) {
            stop = TRUE;
            rc = ERROR;
         } else
            xygoto( -1, -1 );
      }
      /*
       * If rc is not OK or ERROR, a drive was selected.
       */
#if !defined( __UNIX__ )
      else if (rc != OK) {
         dbuff[0] = rc;
         dbuff[1] = ':';
         strcpy( dbuff + 2, dta.pattern );
      }
#endif
      else {
         /*
          * If the last character in a file name is '/' then let's
          *  do a dir on selected directory.
          */
         len = strlen( dbuff );
         if (dbuff[len-1] == '/')
            strcpy( dbuff + len, dta.pattern );
         else {
            /*
             * user selected a file.
             */
            stop = TRUE;
            strcpy( dname, dbuff );
         }
      }
      if (!stop) {
         /*
          * Ensure there is something to display. If not, redisplay the
          *  current directory. (This will probably only happen on floppies.)
          */
         ffind = my_findfirst( dbuff, &dta, ALL_DIRS );
         if (ffind == NULL)
            ffind = my_findfirst( dname, &dta, ALL_DIRS );
         else
            strcpy( dname, dbuff );
      }
   }

   if (window != NULL)
      redraw_screen( window );

   return( rc );
}


/*
 * Name:    setup_directory_window
 * Purpose: set number of rows and cols in directory window
 * Date:    February 13, 1992
 * Passed:  dir:   pointer to directory structure
 *          list:  list of names to be displayed
 *          cnt:   number of files
 * Notes:   set up stuff we need to know about how to display files.
 *
 * jmh 980524: displayed a few more lines.
 * jmh 991019: custom frame style based on graphic set.
 * jmh 991025: use 15 lines if display is over 30;
 *             center.
 */
void setup_directory_window( DIRECTORY *dir, LIST *list, int cnt )
{
int  i, j;
int  cols[5] = { 0, 0, 0, 0, 0 };       /* number of columns to use */
int  wid;
const char *fc1;
const char *fc2;
int  DIR8_ROW;
int  inner[2];
char temp[40];

   /*
    * setup the fixed vars used in dir display.
    *    dir->col =      physical upper left column of dir screen
    *    dir->row =      physical upper left row or line of dir screen
    *    dir->wid =      width of physical screen
    *    dir->hgt =      height of physical screen
    *    dir->max_cols   number of columns of files in dir screen
    *    dir->max_lines  number of lines of files in each column in dir screen
    *    dir->cnt        number of files in list
    */
   dir->cnt = cnt;

   if (g_status.command == GotoWindow) {
      /*
       * Use at most two-thirds of the screen for the window list.
       */
      dir->max_lines = g_display.nlines * 2 / 3;
      if (cnt < dir->max_lines)
         dir->max_lines = cnt;

      /*
       * Find the longest window name.
       */
      for (i = 0; i < cnt; ++i) {
         j = strlen( list[i].name );
         if (j > cols[0])
            cols[0] = j;
      }

      /*
       * Determine the number of columns and lines to use.  If they'll all fit
       * in only one column, use that.  Otherwise use the smaller of the
       * number of columns that fits the lines, or the number of columns that
       * fits the width.  Adjust the lines to fit those columns.
       */
      if (cnt <= dir->max_lines)
         dir->max_cols = 1;
      else {
         cols[1] = (cnt - 1) / dir->max_lines + 1;
         cols[2] = (g_display.ncols - 4 - 4 + 3) / (cols[0] + 3);
         dir->max_cols =  cols[1] < cols[2] ? cols[1] : cols[2];
         dir->max_lines = cnt / dir->max_cols + (cnt % dir->max_cols ? 1 : 0);
         if (dir->max_lines > g_display.nlines - 6)
            dir->max_lines = g_display.nlines - 6;
      }

      /*
       * Now set the dimensions of the surrounding frame and each column.
       */
      dir->hgt = dir->max_lines + 2;
      dir->wid = dir->max_cols * (cols[0] + 3) - 3 + 4;
      dir->len = cols[0];

   } else {
      dir->max_lines =  g_display.nlines > 30 ? 15 : 10;
      dir->hgt = dir->max_lines + (g_status.command == SyntaxSelect ? 6 : 9);
      dir->wid = 76;

#if defined( __DOS16__ )
      dir->len      = 12;
      dir->max_cols = 5;

#else
      /*
       * Figure out the width of each column (ie. the highlight bar/filename
       * length). We'll make 12 the smallest ("standard" 8.3) for five columns.
       * If the overall width is limited to 76, then the longest name can be
       * 72 (two characters for the frame, space each side). However, let's try
       * and be clever - if the majority of files are "short", use more columns
       * and truncate the longer names.
       */
      for (i = 0; i < cnt; ++i) {
         j = strlen( list[i].name );
              if (j <= 12) ++cols[4];      /* depending on the length */
         else if (j <= 16) ++cols[3];      /* of the name, increase   */
         else if (j <= 22) ++cols[2];      /* the appropriate number  */
         else if (j <= 35) ++cols[1];      /* of columns              */
         else              ++cols[0];
      }
      /*
       * The number of columns used most is the one selected, unless there's
       * enough space to display all files without truncation.
       */
      for (i = 0; cols[i] == 0; ++i) ;     /* assumes cnt > 0 */
      dir->max_cols = i+1;
      if (cnt > (i+1) * dir->max_lines) {
         j = cols[i];
         for (++i; i < 5; ++i) {
            if (cols[i] > j) j = cols[i], dir->max_cols = i+1;
         }
      }
      /*
       * Set the actual width of the column to the largest possible.
       */
           if (dir->max_cols == 5) dir->len = 12;
      else if (dir->max_cols == 4) dir->len = 16;
      else if (dir->max_cols == 3) dir->len = 22;
      else if (dir->max_cols == 2) dir->len = 35;
      else                         dir->len = 72;
#endif
   }

   dir->col = (g_display.ncols - dir->wid) / 2;
   dir->row = (g_display.mode_line - dir->hgt) / 2;
   if (dir->col < 0)
      dir->col = 0;
   if (dir->row < 0)
      dir->row = 0;

   /*
    * Find out how many lines in each column are needed to display
    *  matching files.
    */
   dir->lines = dir->cnt / dir->max_cols + (dir->cnt % dir->max_cols ? 1 : 0);
   if (dir->lines > dir->max_lines)
      dir->lines = dir->max_lines;

   /*
    * Find out how many columns of file names we need.
    */
   dir->cols = dir->cnt / dir->lines + (dir->cnt % dir->lines ? 1 : 0);
   if (dir->cols > dir->max_cols)
      dir->cols = dir->max_cols;

   /*
    * Find the maximum number of file names we can display in help screen.
    */
   dir->avail = dir->lines * dir->cols;

   /*
    * Now find the number of file names we do have on the screen.  Every
    *  time we slide the "window", we have to calculate a new nfiles.
    */
   dir->nfiles = dir->cnt > dir->avail ? dir->avail : dir->cnt;

   /*
    * A lot of times, the number of matching files will not fit evenly
    *  in our help screen.  The last column on the right will be partially
    *  filled, hence the variable name prow (partial row).  When there are
    *  more file names than can fit on the screen, we have to calculate
    *  prow every time we slide the "window" of files.
    */
   dir->prow = dir->lines - (dir->avail - dir->nfiles);

   /*
    * Find out how many "virtual" columns of file names we have.  If
    *  all the files can fit in the dir screen, there will be no
    *  virtual columns.
    */
   if (dir->cnt <= dir->avail)
      dir->vcols = 0;
   else
      dir->vcols =  (dir->cnt - dir->avail) / dir->max_lines +
                   ((dir->cnt - dir->avail) % dir->max_lines ? 1 : 0);

   /*
    * Find the physical display column in dir screen.
    */
   dir->flist_col[0] = dir->col + 2;
   for (i = 1; i < dir->max_cols; i++)
      dir->flist_col[i] = dir->flist_col[i-1] + dir->len + 3;

   if (g_status.command == GotoWindow) {
      fofs = 0;
      row_ofs = 0;
   } else if (g_status.command == SyntaxSelect) {
      fofs = 0;
      row_ofs = 4;
   } else {
      /*
       * Find the offset for the first file (ie. skip the directories).
       * If there are no files, point to the directories.
       */
      for (fofs = 0; fofs < cnt; ++fofs) {
         if (list[fofs].name[strlen( list[fofs].name ) - 1] != '/')
            break;
      }
      if (fofs == cnt)
         fofs = 0;
      row_ofs = 5;
   }

   /*
    * Now, draw the borders of the dir screen.
    */
   wid = dir->wid;
   if (g_status.command == GotoWindow) {
      create_frame( wid, dir->hgt, 0, NULL, dir->col, dir->row );
      inner[0] = 0;
      inner[1] = dir->hgt - 1;
      fc1 = fc2 = (g_display.frame_style == 3)
                  ? graphic_char[4] : graphic_char[g_display.frame_style];
   } else if (g_status.command == SyntaxSelect) {
      inner[0] = DIR12_ROW + 1;
      inner[1] = dir->hgt - 1;
      create_frame( wid, dir->hgt, 1, inner, dir->col, dir->row );
      if (g_display.frame_style == 3) {
         fc1 = graphic_char[1];
         fc2 = graphic_char[4];
      } else
         fc1 = fc2 = graphic_char[g_display.frame_style];
      s_output( dir10, dir->row+DIR10_ROW, dir->col+DIR10_COL, Color(Dialog) );
      s_output( dir11, dir->row+DIR11_ROW, dir->col+DIR11_COL, Color(Dialog) );
      s_output( dir12, dir->row+DIR12_ROW, dir->col+DIR12_COL, Color(Dialog) );

      i = sprintf( temp, "%s%d", dir13, dir->cnt );
      s_output( temp, dir->row+DIR13_ROW, dir->col+wid-2-i, Color( Dialog ) );

   } else {
      DIR8_ROW = dir->hgt - 2;
      inner[0] = DIR6_ROW + 1;
      inner[1] = DIR8_ROW - 1;
      create_frame( wid, dir->hgt, 2, inner, dir->col, dir->row );
      fc1 = fc2 = (g_display.frame_style == 3)
                  ? graphic_char[1] : graphic_char[g_display.frame_style];

      /*
       * Write headings in help screen.
       */
      s_output( dir3, dir->row+DIR3_ROW, dir->col+DIR3_COL, Color( Dialog ) );
      s_output( dir4, dir->row+DIR4_ROW, dir->col+DIR4_COL, Color( Dialog ) );
      s_output( dir5, dir->row+DIR5_ROW, dir->col+DIR5_COL, Color( Dialog ) );
      s_output( dir6, dir->row+DIR6_ROW, dir->col+DIR6_COL, Color( Dialog ) );
      s_output( dir8, dir->row+DIR8_ROW, dir->col+DIR8_COL, Color( Dialog ) );

      /*
       * Display the file count right justified.
       */
      i = sprintf( temp, "%s%d", dir7, dir->cnt );
      s_output( temp, dir->row+DIR7_ROW, dir->col+wid-2-i, Color( Dialog ) );
   }

   /*
    * Display the column separators.
    */
   for (j = dir->len + 3; j < wid - 1; j += dir->len + 3) {
      c_output( fc1[TOP_T],    dir->col+j, dir->row+inner[0], Color(Dialog) );
      c_output( fc2[BOTTOM_T], dir->col+j, dir->row+inner[1], Color(Dialog) );
      c_repeat( fc1[VERTICAL_LINE], inner[0]+1 - (inner[1]-1) - 1,
                dir->col+j, dir->row+inner[0]+1, Color( Dialog ) );
   }
}


/*
 * Name:    write_directory_list
 * Purpose: given directory list, display matching files
 * Date:    February 13, 1992
 * Passed:  flist: pointer to list
 *          dir:   directory display structure
 * Notes:   blank out the previous file name and display the new one.
 *
 * jmh 980523: Display the lines for each column, rather than the columns for
 *             each line. This corrects a one-column bug.
 */
void write_directory_list( LIST *flist, DIRECTORY *dir )
{
int  i;
int  j;
int  k;
int  end;
int  line;
int  col;
char temp[NAME_MAX+2];

   end = FALSE;
   for (k = 1, j = 0; j < dir->cols; ++j) {
      col  = dir->flist_col[j];
      line = dir->row + row_ofs + 1;
      for (i = 0; i < dir->lines; ++k, ++flist, ++line, ++i) {
         /*
          * We need to blank out all lines and columns used to display
          *  files, because there may be some residue from a previous dir
          */
         c_repeat( ' ', dir->len, col, line, Color( Dialog ) );
         if (!end) {
            s_output( reduce_string( temp, flist->name, dir->len, END ),
                      line, col, flist->color );
            if (k >= dir->nfiles)
               end = TRUE;
         }
      }
   }
}


/*
 * Name:    select_file
 * Purpose: To let user select a file from dir list
 * Date:    February 13, 1992
 * Passed:  flist: pointer to list of files
 *          stem:  base directory and wildcard pattern
 *          dir:   directory display stuff
 * Notes:   let user move thru the file names with the cursor keys
 *
 * jmh 980527: display attribute as well, restructured.
 * jmh 980805: use SortBoxBlock to toggle between name and extension sorting.
 * jmh 021019: allow ":<letter>" or "<letter>:" to select a drive, returning
 *              the (lowercase) letter (not in UNIX).
 * jmh 031202: display file date & time.
 * jmh 031203: press Tab to select a new directory (return TRUE).
 */
int  select_file( LIST *flist, char *stem, DIRECTORY *dir )
{
long ch;                /* input character from user */
int  func;              /* function of character input by user */
int  fno;               /* index into flist of the file under cursor */
int  r;                 /* current row of cursor */
int  c;                 /* current column of cursor */
int  offset;            /* offset into file list */
int  lastoffset;        /* largest possible offset */
int  stop;              /* stop indicator */
int  max_len;           /* maximum allowed length of name */
int  color;             /* color of help screen */
int  file_color;        /* color of current file */
int  change;            /* boolean, hilite another file? */
int  oldr;              /* old row */
int  oldc;              /* old column */
int  col;               /* column to display directory, selected file, size */
int  len;               /* length of file name to display */
char temp[PATH_MAX];
text_t prefix[PATH_MAX];/* consecutively typed letters */
int  prelen;            /* length of above */
clock_t ticks;          /* time between letters */
int  name_row;

/*
 * Convert the file list column and row into a screen column and row.
 */
#define colrow(c, r) (c-1)*(dir->len+3)+dir->col+2, r+dir->row+row_ofs

   /*
    * initialise everything.
    */
   color      = Color( Dialog );
   file_color = Color( Hilited_file );
   fno        =
   func       =
   offset     = 0;
   lastoffset = dir->vcols * dir->lines;
   c          =
   r          =
   oldc       =
   oldr       = 1;
   stop       = FALSE;
   change     = TRUE;
   prelen     = 0;
   prefix[0]  = '\0';
   ticks      = 0;

   /*
    * Some names could be quite long - let's truncate the ones that don't fit.
    * Assume the directory, selected file and file size messages (dir3 to 6)
    * are all equal length and in the same column.
    */
   if (g_status.command == SyntaxSelect) {
      col = DIR10_COL + strlen( dir10 );
      name_row = DIR10_ROW;
   } else {
      col = DIR3_COL + strlen( dir3 );
      name_row = DIR4_ROW;
   }
   max_len = dir->wid - col - 2;
   col += dir->col;
   if (g_status.command != GotoWindow && g_status.command != SyntaxSelect)
      s_output( reduce_string( temp, stem, max_len, MIDDLE ),
                dir->row+DIR3_ROW, col, color );

   while (stop == FALSE) {
      if (change) {
         hlight_line( colrow(oldc, oldr), dir->len, flist[fno].color );
         hlight_line( colrow(c, r), dir->len, file_color );

         fno = offset + (c-1)*dir->lines + (r-1);

         if (g_status.command != GotoWindow) {
            len = strlen( flist[fno].name );
            if (len <= max_len) {
               s_output( flist[fno].name, dir->row+name_row, col, color);
               c_repeat( ' ', max_len-len, col+len, dir->row+name_row, color );
            } else {
               int chop;
               if (len <= dir->len)
                  /*
                   * It was fully displayed in the list
                   * so just truncate in the selected.
                   */
                  chop = END;
               else if (len <= dir->len + max_len - 3)
                  /*
                   * The beginning of the name is in the list, the end of it
                   * (and perhaps the end of the beginning) is in the selected.
                   */
                  chop = BEGIN;
               else
                  /*
                   * list+selected is still not all the name.
                   */
                  chop = MIDDLE;
               s_output( reduce_string( temp, flist[fno].name, max_len, chop ),
                         dir->row+name_row, col, color );
            }

            if (g_status.command == SyntaxSelect) {
               LTYPE *p = flist[fno].data;
               int plen = strlen( p->parent );
               if (plen  &&  len + 1 + 2 + plen <= max_len) {
                  c_output( '(', col+len+1, dir->row+DIR10_ROW, color );
                  s_output( p->parent, dir->row+DIR10_ROW, col+len+2, color );
                  c_output( ')', col+len+2+plen, dir->row+DIR10_ROW, color );
               }
               len = strlen( reduce_string( temp, p->pattern, max_len, END ) );
               s_output( temp, dir->row+DIR11_ROW, col, color );
               c_repeat( ' ', max_len-len, col+len, dir->row+DIR11_ROW, color);
               assert( (unsigned)p->icase < 3 );
               s_output( dir14[p->icase], dir->row+DIR12_ROW, col, color );
            } else {
               FTYPE *p = flist[fno].data;
               format_time( dir5a, temp, ftime_to_tm( &p->ftime ) );
               s_output( temp, dir->row+DIR5_ROW, col, color );
               n_output( p->fsize, 10, col, dir->row+DIR6_ROW, color );
               s_output( str_fattr( temp, p->fattr ),
                         dir->row+DIRA_ROW, dir->col+DIRA_COL, color );
            }
         }
         change = FALSE;
      }
      oldr = r;
      oldc = c;
      ch   = getkey( );
      /*
       * User may have redefined the Enter and ESC keys.  Make the Enter key
       *  perform a Rturn in this function. Make the ESC key do an AbortCommand.
       */
      func = (ch == RTURN) ? Rturn        :
             (ch == ESC)   ? AbortCommand :
             getfunc( ch );

      /*
       * jmh 980523: if a normal character has been entered, search the file
       *              list for a name starting with that character.
       * jmh 980805: made more awkward when sorting by extension, since the
       *              names are no longer in order.
       * jmh 020630: allow more than one character (within half-a-second).
       */
#if !defined( __UNIX__ )
      if (g_status.command != GotoWindow && g_status.command != SyntaxSelect
          &&  ch == ':'  &&  prelen <= 1) {
       long rc;
         if (prelen == 1)
            rc = bj_tolower( prefix[0] );
         else
            rc = getkey( );
         if (rc >= 'a' && rc <= 'z')
            return( (int)rc );
      }
#endif
      if (ch < 256) {
       text_ptr fn;
       int i = fno;
       int rc;
         /*
          * Borland defines CLOCKS_PER_SEC as 18.2; let's avoid floating point.
          */
         if (clock() - ticks > (int)CLOCKS_PER_SEC / 2) {
            prefix[0] = (text_t)ch;
            prelen = 1;
         } else if ((text_t)ch != prefix[0] || (g_status.command == GotoWindow
                                                && bj_isdigit( *prefix )))
            prefix[prelen++] = (text_t)ch;
         ticks = clock();
         fn = (text_ptr)flist[i].name;
         if (g_status.command == GotoWindow) {
            if (bj_isdigit( *prefix )) {
               fn += 2;
               while (*fn == ' ')
                  ++fn;
            } else
               fn += name_ofs;
         }
         /*
          * multiple instances of the first letter
          * cycle through that letter
          */
         if ((rc = my_memcmp( fn, prefix, prelen )) == 0  &&  i >= fofs) {
            if (prelen == 1) {
               text_t let = sort.order_array[(int)ch];
               if (mode.dir_sort == SORT_NAME
                   && g_status.command != GotoWindow) {
                  if (++i == dir->cnt
                      || sort.order_array[(text_t)flist[i].name[0]] != let)
                     i = dir->cnt;
               } else {
                  while (++i < dir->cnt) {
                     fn = (text_ptr)flist[i].name;
                     if (g_status.command == GotoWindow) {
                        if (bj_isdigit( *prefix )) {
                           fn += 2;
                           while (*fn == ' ')
                              ++fn;
                        } else
                           fn += name_ofs;
                     }
                     if (sort.order_array[*fn] == let)
                        break;
                  }
               }
               if (i == dir->cnt) {
                  for (i = fofs; ; ++i) {
                     fn = (text_ptr)flist[i].name;
                     if (g_status.command == GotoWindow) {
                        if (bj_isdigit( *prefix )) {
                           fn += 2;
                           while (*fn == ' ')
                              ++fn;
                        } else
                           fn += name_ofs;
                     }
                     if (sort.order_array[*fn] == let)
                        break;
                  }
               }
            }
         } else {
            if (mode.dir_sort == SORT_NAME && g_status.command != GotoWindow) {
               if (rc < 0) {
                  while (++i < dir->cnt  &&
                         ((rc = my_memcmp( (text_ptr)flist[i].name,
                                           prefix, prelen )) < 0  ||
                          (i < fofs && rc > 0))) ;
                  if (i == dir->cnt)
                     func = EndOfFile;
               }
               else
                  for (i = fofs; my_memcmp( (text_ptr)flist[i].name,
                                            prefix, prelen ) < 0; ++i) ;
            } else {
             int start = i;
               while (++i < dir->cnt) {
                  fn = (text_ptr)flist[i].name;
                  if (g_status.command == GotoWindow) {
                     if (bj_isdigit( *prefix )) {
                        fn += 2;
                        while (*fn == ' ')
                           ++fn;
                     } else
                        fn += name_ofs;
                  }
                  if (my_memcmp( fn, prefix, prelen ) == 0)
                     break;
               }
               if (i == dir->cnt) {
                  for (i = fofs; i < start; ++i) {
                     fn = (text_ptr)flist[i].name;
                     if (g_status.command == GotoWindow) {
                        if (bj_isdigit( *prefix )) {
                           fn += 2;
                           while (*fn == ' ')
                              ++fn;
                        } else
                           fn += name_ofs;
                     }
                     if (my_memcmp( fn, prefix, prelen ) == 0)
                        break;
                  }
                  if (i == start)
                     func = ERROR;      /* do nothing */
               }
            }
         }
         if (func == 0) {
            if (i >= offset && i < offset+dir->avail) {
               /* do nothing */
            } else {
               offset = i - (i % dir->lines);
               if (dir->max_cols == 3 || dir->max_cols == 4)
                  offset -= dir->lines;
               else if (dir->max_cols == 5)
                  offset -= dir->lines * 2;
               if (offset < 0)
                  offset = 0;
               else if (offset > lastoffset)
                  offset = lastoffset;
               recalculate_dir( dir, flist, offset );
            }
            i     -= offset;
            c      = i / dir->lines + 1;
            r      = i % dir->lines + 1;
            change = TRUE;
         }
      }
      switch (func) {
         case Rturn        :
         case NextLine     :
         case BegNextLine  :
         case AbortCommand :
         case Tab          :
            stop = TRUE;
            if (func == Tab) {
               if (g_status.command == GotoWindow ||
                   g_status.command == SyntaxSelect)
                  stop = FALSE;
               else
                  hlight_line( colrow(oldc, oldr), dir->len, flist[fno].color );
            }
            break;

         case LineUp :
            if (r > 1) {
               --r;
            } else {
               r = dir->lines;
               if (c > 1) {
                  --c;
               } else if (offset > 0) {
                  /*
                   * recalculate the dir display stuff.
                   */
                  offset -= dir->lines;
                  recalculate_dir( dir, flist, offset );
               }
            }
            change = TRUE;
            break;

         case LineDown :
            if (r < dir->prow) {
               ++r;
            } else if (r < dir->lines && c != dir->cols) {
               ++r;
            } else {
               r = 1;
               if (c < dir->cols) {
                  ++c;
               } else if (offset < lastoffset) {
                  offset += dir->lines;
                  recalculate_dir( dir, flist, offset );
               }
            }
            change = TRUE;
            break;

         case CharLeft :
         case StreamCharLeft :
            if (c > 1) {
               --c;
            } else if (offset > 0) {
               offset -= dir->lines;
               recalculate_dir( dir, flist, offset );
            } else {
               c = dir->cols;
               if (r > dir->prow)
                  --c;
            }
            change = TRUE;
            break;

         case CharRight :
         case StreamCharRight :
            if (c < dir->cols) {
               ++c;
               if (c == dir->cols) {
                  if (r > dir->prow)
                     r = dir->prow;
               }
            } else if (offset < lastoffset) {
               offset += dir->lines;
               recalculate_dir( dir, flist, offset );
               if (r > dir->prow)
                  r = dir->prow;
            } else
               c = 1;
            change = TRUE;
            break;

         case BegOfLine :
            if (c != 1 || r != 1) {
               c = r = 1;
               change = TRUE;
            }
            break;

         case EndOfLine :
            if (c != dir->cols || r != dir->prow) {
               c = dir->cols;
               r = dir->prow;
               change = TRUE;
            }
            break;

         case ScreenDown :
            if (offset < lastoffset) {
               offset += dir->avail;
               if (offset > lastoffset)
                  offset = lastoffset;
               recalculate_dir( dir, flist, offset );
               if (c == dir->cols && r > dir->prow)
                  r = dir->prow;
               change = TRUE;
            }
            break;

         case ScreenUp :
            if (offset > 0) {
               offset -= dir->avail;
               if (offset < 0)
                  offset = 0;
               recalculate_dir( dir, flist, offset );
               change = TRUE;
            }
            break;

         case TopOfFile:                /* added by jmh 980523 */
            if (c != 1 || r != 1) {     /* added conditions jmh 980527 */
               c = r = 1;
               change = TRUE;
            }
            if (offset != 0) {
               offset = 0;
               recalculate_dir( dir, flist, offset );
               change = TRUE;
            }
            break;

         case EndOfFile:                /* added by jmh 980523 */
            if (offset != lastoffset) { /* added conditions jmh 980527 */
               offset = lastoffset;
               recalculate_dir( dir, flist, offset );
               change = TRUE;
            }
            if (c != dir->cols || r != dir->prow) {
               c = dir->cols;
               r = dir->prow;
               change = TRUE;
            }
            break;

         case BackSpace:
            /*
             * jmh 980523: use BackSpace to select the "../" entry, which
             *             will be first, except in root directory.
             */
            if (strcmp( flist[0].name, "../" ) == 0) {
               fno  = 0;
               stop = TRUE;
            }
            break;

         case SortBoxBlock:
            if (g_status.command == GotoWindow ||
                g_status.command == SyntaxSelect)
               break;
            mode.dir_sort = (SORT_NAME + SORT_EXT) - mode.dir_sort;
            shell_sort( flist, dir->cnt );
            write_directory_list( flist+offset, dir );
            change = TRUE;
            break;

         default :
            break;
      }
   }
   dir->select = fno;
   return( func == AbortCommand ? ERROR : (func == Tab ? TRUE : OK) );
}


/*
 * Name:    recalculate_dir
 * Purpose: To recalcute dir structure when cursor goes ahead or behind screen
 * Date:    November 13, 1993
 * Passed:  dir:    pointer to file structure
 *          flist:  pointer to file structure
 *          offset: number of files from beginning of flist
 * Notes:   Find new number of files on the screen.  Then, find out
 *           how many files names are in the last column.
 */
void recalculate_dir( DIRECTORY *dir, LIST *flist, int offset )
{
register int off;

   off = offset;
   dir->nfiles = (dir->cnt - off) > dir->avail ? dir->avail :
                                                 (dir->cnt - off);
   dir->prow   = dir->lines - (dir->avail - dir->nfiles);
   write_directory_list( flist+off, dir );
}


/*
 * Name:     shell_sort
 * Purpose:  To sort file names
 * Date:     February 13, 1992
 * Modified: November 13, 1993, Frank Davis per Byrial Jensen
 * Passed:   flist: pointer to list structure
 *           cnt:   number of items to sort
 * Notes:    this implementation of Shellsort is based on the one by Robert
 *            Sedgewick on page 109, _Algorithms in C_.
 *
 * Change:   Use of my_memcmp instead of memcmp
 *           (In somes cases not-english people wants to use their own letters
 *           even in filenames; I am such one, Byrial).
 *
 * jmh 980805: place the ../ entry first and exclude it from the sort.
 */
void shell_sort( LIST *flist, int cnt )
{
int  i;
register int j;
register int inc;
LIST temp;
LIST *fl;
int (*cmp)( char *, char * );

   sort.order_array = (mode.search_case == IGNORE) ?
                      sort_order.ignore : sort_order.match;

   if (cnt > 1) {

      if (g_status.command == GotoWindow) {
         char* n;
         cmp = win_cmp;
         n = flist[0].name + 2;       /* [!+-] and space */
         while (*n == ' ')            /* padding spaces for the number */
            ++n;
         while (bj_isdigit( *n ))     /* the number */
            ++n;
         n += 6;                      /* the letter, space, !#* and space */
         name_ofs = (int)(n - flist[0].name);

      } else if (g_status.command == SyntaxSelect) {
         cmp = shl_cmp;

      } else {
         cmp = dir_cmp;
         for (j = 0; j < cnt; ++j)
            if (strcmp( flist[j].name, "../" ) == 0)
               break;
         if (j < cnt) {
            if (j != 0) {
               flist[j].name = flist[0].name;
               flist[0].name = "../";
            }
            --cnt;
            ++flist;
         }
      }

      fl = flist;

      /*
       * figure the increments, per Donald Knuth, _Sorting and Searching_, and
       *  Robert Sedgewick, _Algorithms in C_.
       */
      j = cnt / 9;
      for (inc=1; inc <= j; inc = 3 * inc + 1);

      /*
       * now, Shellsort the directory file names.
       */
      for (; inc > 0; inc /= 3) {
         for (i=inc; i < cnt; i++) {
            j = i;
            memcpy( &temp, fl+j, sizeof(LIST) );
            while (j >= inc  &&  cmp( fl[j-inc].name, temp.name ) > 0) {
               memcpy( fl+j, fl+j-inc, sizeof(LIST) );
               j -= inc;
            }
            memcpy( fl+j, &temp, sizeof(LIST) );
         }
      }
   }
}


/*
 * Name:    dir_cmp
 * Purpose: compare two filenames for the directory sort
 * Author:  Jason Hood
 * Date:    August 5, 1998
 * Passed:  name1: pointer to first filename
 *          name2: pointer to second filename
 * Returns: < 0 if name1 comes before name2
 *            0 if name1 and name2 are equal (should never occur)
 *          > 0 if name1 comes after name2
 * Notes:   directories are placed before files.
 *          the strings are assumed to be NUL-terminated.
 *          names that begin with a dot are not treated as extensions.
 */
int  dir_cmp( char *name1, char *name2 )
{
int len;
int d;
int e1;
int e2;

   len = strlen( name1 ) + 1;           /* include the NUL */
   e2  = strlen( name2 ) - 1;           /* last character */
   d   = (name2[e2] == '/') - (name1[len-2] == '/');
   if (d == 0) {
      /*
       * Both files are either directories or names
       */
      if (mode.dir_sort == SORT_NAME)
         d = my_memcmp( (text_ptr)name1, (text_ptr)name2, len );
      else {
         /*
          * Search for the extension
          */
         for (e1 = len-2; e1 > 0 && name1[e1] != '.'; --e1) ;
         for (;           e2 > 0 && name2[e2] != '.'; --e2) ;

         if (!e1 &&  e2) return -1;     /* no extension before extension */
         if ( e1 && !e2) return +1;     /* extension after no extension */
         if (!e1 && !e2)                /* neither have an extension */
            d = my_memcmp( (text_ptr)name1, (text_ptr)name2, len );
         else {
            /*
             * Sort the extension first. If the extension is the same,
             * sort by name.
             */
            d = my_memcmp( (text_ptr)(name1+e1), (text_ptr)(name2+e2), len-e1 );
            if (d == 0) {
               name1[e1] = name2[e2] = '\0';
               d = my_memcmp( (text_ptr)name1, (text_ptr)name2, len );
               name1[e1] = name2[e2] = '.';
            }
         }
      }
   }
   return( d );
}


/*
 * Count the directory components of name.  Helper function for win_cmp.
 */
static int dircnt( const char *name )
{
int  cnt;

   for (cnt = 0; *name; ++name)
      if (*name == '/')
         ++cnt;

   return( cnt );
}


/*
 * Name:    win_cmp
 * Purpose: compare two names for the window list sort
 * Author:  Jason Hood
 * Date:    August 29, 2006
 * Notes:   compares filenames first (current before sub before drive) then
 *           window number (if the title is the same).
 */
static int  win_cmp( char *name1, char *name2 )
{
int  dir1, dir2;
char *n1, *n2;

   n1 = name1 + name_ofs;
   n2 = name2 + name_ofs;

   /*
    * place relative paths before absolute paths
    */
#if defined( __UNIX__ )
   if (*n1 == '/') {
      if (*n2 != '/')
         return( 1 );
   }
   else if (*n2 == '/')
      return( -1 );
#else
   if (n1[1] == ':') {
      if (n2[1] != ':')
         return( 1 );
      if (*n1 != *n2)
         return( *n1 - *n2 );
   }
   else if (n2[1] == ':')
      return( -1 );
#endif

   /*
    * sort by path depth
    */
   dir1 = dircnt( n1 );
   dir2 = dircnt( n2 );
   if (dir1 != dir2)
      return( dir1 - dir2 );

   /*
    * sort by name
    */
   dir1 = my_strcmp( (text_ptr)n1, (text_ptr)n2 );
   if (dir1 != 0)
      return( dir1 );

   /*
    * sort by window number
    */
   return( strcmp( name1 + 2, name2 + 2 ) );
}


/*
 * Name:    shl_cmp
 * Purpose: compare two languages for the language list
 * Author:  Jason Hood
 * Date:    September 13, 2006
 */
static int  shl_cmp( char *name1, char *name2 )
{
   return my_strcmp( (text_ptr)name1, (text_ptr)name2 );
}
