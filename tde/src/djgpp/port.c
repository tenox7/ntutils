/*
 * Now that I own both MSC 7.0 and BC 3.1 and have linux, lets rearrange stuff
 * so many compilers can compile TDE.  Several implementation specific
 * functions needed for several environments were gathered into this file.
 *
 * In version 3.2, these functions changed to support unix.
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
 * You may use and distribute it freely.
 */

#include "tdestr.h"
#include "common.h"
#include "tdefunc.h"
#include "define.h"

#include <io.h>                         /* for attributes (_chmod) */
#define __dj_include_pc_h_
#include <dos.h>                        /* for drive (_dos_[gs]etdrive) */
#include <fcntl.h>                      /* for lfn querying (_USE_LFN) */
#include <sys/stat.h>                   /* for full path (_fixpath _truename) */
#include <errno.h>                      /* for _doserrno */
#include <dirent.h>                     /* for _lfn_find_close */


static char  found_name[NAME_MAX+2];
static FTYPE found = { found_name, 0, 0 };
static FTYPE* find_wild( FFIND * );

static int  no_clipboard;
static int  open_clipboard( void );
static void close_clipboard( void );


/*
 * Name:    my_heapavail
 * Purpose: available free memory
 * Date:    July 8, 1997
 * Notes:   approximation only - there will be at least this amount available
 */
long my_heapavail( void )
{
   return( _go32_dpmi_remaining_physical_memory( ) );
}


/*
 * Name:    find_wild
 * Purpose: find a file matching an "extended" pattern
 * Author:  Jason Hood
 * Date:    December 2, 2003
 * Passed:  dta: file finding info
 * Notes:   my_find{first,next} MUST be successfully called first.
 *          see my_findfirst notes.
 */
static FTYPE *find_wild( FFIND *dta )
{
int  i;

   i = OK;
   do {
      if (dta->dirs == ALL_DIRS && (dta->find_info.ff_attrib & SUBDIRECTORY))
         break;
      if (wildcard( dta->pattern, dta->find_info.ff_name ) == TRUE)
         break;
      i = findnext( &dta->find_info );
   } while (i == OK);

   if (i != OK)
      return( NULL );   /* all done */

   if (!_USE_LFN) {
      for (i = 0; dta->find_info.ff_name[i]; ++i)
         found.fname[i] = bj_tolower( dta->find_info.ff_name[i] );
      found.fname[i] = '\0';
   }
   else {
      i = strlen( dta->find_info.ff_name );
      memcpy( found.fname, dta->find_info.ff_name, i + 1 );
   }

   if (dta->dirs) {
      found.fsize = dta->find_info.ff_fsize;
      found.fattr = dta->find_info.ff_attrib;
      found.ftime = *(ftime_t*)(char*)(&dta->find_info.ff_ftime);
      if (dta->find_info.ff_attrib & SUBDIRECTORY) {
         found.fname[i] = '/';
         found.fname[i+1] = '\0';
      }
   }
   return( &found );
}


/*
 * Name:    my_findfirst
 * Purpose: find the first file matching a pattern
 * Date:    August 4, 1997
 * Passed:  path: path and pattern to search for files
 *          dta:  file finding info
 *          dirs: NO_DIRS to ignore all directories
 *                MATCH_DIRS to match directory names
 *                ALL_DIRS to include all directories and return file sizes
 * Notes:   Returns NULL for no matching files or bad pattern;
 *           otherwise a pointer to a static FTYPE, with fname holding the
 *           filename that was found.
 *          If dirs is TRUE fsize will hold the size of the file, and the
 *           name will have a trailing slash ('/') if it's a directory.
 *          DOS systems will convert the name to lower-case; LFN systems
 *           will leave it as-is.
 */
FTYPE *my_findfirst( char *path, FFIND *dta, int dirs )
{
int  i;
fattr_t fattr;
char temp[PATH_MAX];

   dta->dirs = dirs;
   /*
    * Separate the path and pattern
    */
   i = strlen( path ) - 1;
   if (i == -1) {
      get_current_directory( dta->stem );
      strcpy( dta->pattern, "*" );

   } else if (path[i] == '/' || path[i] == '\\' || path[i] == ':') {
      strcpy( dta->stem, path );
      strcpy( dta->pattern, "*" );

   } else if (file_exists( path ) == SUBDIRECTORY) {
      join_strings( dta->stem, path, "/" );
      strcpy( dta->pattern, "*" );

   } else {
      while (--i >= 0) {
         if (path[i] == '/' || path[i] == ':')
            break;
         /* if it's a backslash, it could be an escape for the pattern */
         if (path[i] == '\\' && strchr( "!^-\\]", path[i+1] ) == NULL)
            break;
      }
      if (i >= 0) {
         strncpy( dta->stem, path, ++i );
         dta->stem[i] = '\0';
         strcpy( dta->pattern, path+i );
      } else {
         get_current_directory( dta->stem );
         strcpy( dta->pattern, path );
      }
      if (!is_valid_pattern( dta->pattern, &i ))
         return( NULL );
   }
   /*
    * Start scanning the directory
    */
   join_strings( temp, dta->stem, "*.*" );
   fattr = NORMAL | READ_ONLY | HIDDEN | SYSTEM | ARCHIVE;
   if (dirs)
      fattr |= SUBDIRECTORY;

   if (findfirst( temp, &dta->find_info, fattr ) != OK)
      return( NULL );   /* nothing matched */

   /* ignore the "." entry (assuming it to be found in this initial scan) */
   if (dirs && dta->find_info.ff_name[0] == '.' &&
               dta->find_info.ff_name[1] == '\0')
      return( my_findnext( dta ) );

   return( find_wild( dta ) );
}


/*
 * Name:    my_findnext
 * Purpose: find the next file matching a pattern
 * Date:    August 4, 1997
 * Passed:  dta: file finding info
 * Notes:   my_findfirst() MUST be called before calling this function.
 *          Returns NULL if no more matching names;
 *          otherwise same as my_findfirst.
 */
FTYPE *my_findnext( FFIND *dta )
{
   return( (findnext( &dta->find_info ) == OK) ? find_wild( dta ) : NULL );
}


/*
 * Name:    my_findclose
 * Purpose: close the handle used by LFN finding
 * Author:  Jason Hood
 * Date:    July 5, 2005
 * Passed:  dta: file finding info
 */
void my_findclose( FFIND *dta )
{
   if (_USE_LFN  &&  dta->find_info.lfn_handle != 0) {
      _lfn_find_close( dta->find_info.lfn_handle );
      dta->find_info.lfn_handle = 0;
   }
}


/*
 * Name:    get_fattr
 * Purpose: To get dos file attributes
 * Date:    August 20, 1997
 * Passed:  fname: ASCIIZ file name.  Null terminated file name
 *          fattr: pointer to file attributes
 * Returns: 0 if successful, non zero if not
 * Notes:   FYI, File Attributes:
 *              0x00 = Normal.  Can be read or written w/o restriction
 *              0x01 = Read-only.  Cannot be opened for write; a file with
 *                     the same name cannot be created.
 *              0x02 = Hidden.  Not found by directory search.
 *              0x04 = System.  Not found by directory search.
 *              0x08 = Volume Label.
 *              0x10 = Directory.
 *              0x20 = Archive.  Set whenever the file is changed, or
 *                     cleared by the Backup command.
 *           Return codes:
 *              0 = No error
 *              1 = AL not 0 or 1
 *              2 = file is invalid or does not exist
 *              3 = path is invalid or does not exist
 *              5 = Access denied
 *
 * jmh 031027: return DOS error code.
 */
int  get_fattr( char *fname, fattr_t *fattr )
{
int  rc;                /* return code */

   assert( fname != NULL  &&  fattr != NULL);

   rc = OK;
   *fattr = _chmod( fname, 0 );
   if (*fattr == -1) {
      *fattr = 0;
      rc = _doserrno;
   }
   return( rc );
}


/*
 * Name:    set_fattr
 * Purpose: To set dos file attributes
 * Date:    August 20, 1997
 * Passed:  fname: ASCIIZ file name.  Null terminated file name
 *          fattr: file attributes
 * Returns: OK if successful, ERROR if not
 */
int  set_fattr( char *fname, fattr_t fattr )
{
int  rc;                /* return code */

   assert( fname != NULL );

   rc = _chmod( fname, 1, fattr );
   if (rc != ERROR)
      rc = OK;
   return( rc );
}


/*
 * Name:    get_current_directory
 * Purpose: get current directory
 * Date:    August 7, 1997
 * Passed:  path:  pointer to buffer to store path
 * Notes:   append a trailing slash ('/') if it's not root
 *          path is expected to be at least PATH_MAX long
 *
 * 991123:  get the full path before appending the slash (corrects a
 *           problem when LFN fails).
 */
int  get_current_directory( char *path )
{
register int rc;

   assert( path != NULL );

   rc = OK;
   if (getcwd( path, PATH_MAX ) == NULL)
      rc = ERROR;
   else {
      /*
       * In LFN, the returned directory is whatever was used to get there.
       * Whilst tolerable for the dir lister (actually, I didn't notice), it's
       * no good when comparing the path for the filename display.
       */
      if (_USE_LFN)
         get_full_path( path, path );
      path += strlen( path );
      if (path[-1] != '/') {
         *path   = '/';
         path[1] = '\0';
      }
   }
   return( rc );
}


/*
 * Name:    set_current_directory
 * Purpose: set current directory
 * Date:    November 13, 1993
 * Passed:  new_path: directory path, which may include drive letter
 * Notes:   changes drive as well.
 */
int  set_current_directory( char *new_path )
{
register int  rc;

   assert( new_path != NULL );

   rc = OK;
   if (chdir( new_path ) == ERROR)
      rc = ERROR;
   return( rc );
}


/*
 * Name:    get_full_path
 * Purpose: retrieve the fully-qualified path name for a file
 * Date:    May 3, 1998
 * Passed:  in_path:  path to be canonicalized
 *          out_path: canonicalized path
 * Notes:   out_path is assumed to be PATH_MAX characters.
 *
 * 980511:  added test for non-existant file in LFN.
 * 990122:  correct bug in above.
 * 020721:  test for /dev/ paths.
 * 020727:  rewrote the LFN part using _truename.
 * 021019:  preserve a trailing slash.
 */
void get_full_path( char *in_path, char *out_path )
{
int  i;
int  rc = ERROR;
char filename[NAME_MAX+1];
int  len, slash;

   len = strlen( in_path ) - 1;
   slash = (in_path[len] == '/'  ||  in_path[len] == '\\');

   /*
    * _fixpath doesn't seem to handle LFN too well; it converts all to lower-
    * case and doesn't expand the extended parent directories ("..." etc).
    */
   if (_USE_LFN)
   {
      if (_truename( in_path, out_path ) != NULL) {
         if (file_exists( out_path ) == ERROR) {
            /*
             * _truename will successfully convert the path of a non-existant
             * file, but it's probably a mixture of long and short components.
             * Find the end of the path and convert it.
             */
            for (i = strlen( out_path ) - 1; i > 1; --i)
               if (out_path[i] == '\\')
                  break;
            if (i == 2) {
               /*
                * In the root directory - do nothing
                */
            } else {
               strcpy( filename, out_path+i+1 );
               out_path[i+1] = '\0';
               _truename( out_path, out_path );
               strcat( out_path, filename );
            }
         }
         /*
          * Now I have to convert to lowercase and slashes myself.
          */
         out_path[0] = bj_tolower( out_path[0] );
         for (i = strlen( out_path ) - 1; i > 1; --i)
            if (out_path[i] == '\\')
               out_path[i] = '/';
         rc = OK;
      }
   }
   if (rc == ERROR)
      _fixpath( in_path, out_path );

   if (slash) {
      len = strlen( out_path );
      if (out_path[len-1] != '/') {
         out_path[len++] = '/';
         out_path[len]   = '\0';
      }
   }
}


/*
 * Name:    get_ftime
 * Purpose: get the file's (modified) timestamp
 * Author:  Jason Hood
 * Date:    March 20, 2003
 * Passed:  fname:  pointer to file's name
 *          ftime:  pointer to store the timestamp
 * Returns: OK if successful, ERROR if not (ftime unchanged).
 */
int  get_ftime( char *fname, ftime_t *ftime )
{
int  handle;
int  rc = ERROR;

   handle = open( fname, O_RDONLY );
   if (handle >= 0) {
      if (getftime( handle, (struct ftime*)ftime ) == 0)
         rc = OK;
      close( handle );
   }
   return( rc );
}


/*
 * Name:    set_ftime
 * Purpose: set the file's (modified) timestamp
 * Author:  Jason Hood
 * Date:    March 20, 2003
 * Passed:  fname:  pointer to file's name
 *          ftime:  pointer to the timestamp
 * Returns: OK if successful, ERROR if not.
 */
int  set_ftime( char *fname, ftime_t *ftime )
{
int  handle;
int  rc = ERROR;

   handle = open( fname, O_RDONLY );
   if (handle >= 0) {
      if (setftime( handle, (struct ftime*)ftime ) == 0)
         rc = OK;
      close( handle );
   }
   return( rc );
}


/*
 * Name:    ftime_to_tm
 * Purpose: convert the file time to the tm structure
 * Author:  Jason Hood
 * Date:    March 20, 2003
 * Passed:  ftime:  the file time to convert
 * Returns: pointer to the tm structure
 * Notes:   only the date and time fields are set.
 */
struct tm *ftime_to_tm( const ftime_t *ftime )
{
static struct tm t;
struct ftime *ft;

   ft = (struct ftime*)ftime;
   t.tm_year = ft->ft_year + 80;        /* tm is from 1900, ft is from 1980 */
   t.tm_mon  = ft->ft_month - 1;        /* tm is from 0, ft is from 1 */
   t.tm_mday = ft->ft_day;
   t.tm_hour = ft->ft_hour;
   t.tm_min  = ft->ft_min;
   t.tm_sec  = ft->ft_tsec * 2;         /* file time halves the seconds */

   return( &t );
}


/*
 * Name:    test_clipboard
 * Purpose: determine if the clipboard is available
 * Author:  Jason Hood
 * Date:    November 29, 2003
 * Returns: TRUE clipboard is available, FALSE if not
 */
int  test_clipboard( void )
{
__dpmi_regs regs;

   regs.x.ax = 0x1700;
   __dpmi_int( 0x2f, &regs );
   no_clipboard = (regs.x.ax == 0x1700);

   return( !no_clipboard );
}


/*
 * Name:    open_clipboard
 * Purpose: open the Windows clipboard
 * Author:  Jason Hood
 * Date:    August 27, 2002
 * Returns: TRUE if opened, FALSE otherwise
 * Notes:   expects test_clipboard to have been called first.
 */
static int  open_clipboard( void )
{
__dpmi_regs regs;

   if (no_clipboard)
      return( FALSE );

   regs.x.ax = 0x1701;
   __dpmi_int( 0x2f, &regs );

   return( regs.x.ax != 0 );
}


/*
 * Name:    close_clipboard
 * Purpose: close the Windows clipboard
 * Author:  Jason Hood
 * Date:    August 27, 2002
 */
static void close_clipboard( void )
{
__dpmi_regs regs;

   regs.x.ax = 0x1708;
   __dpmi_int( 0x2f, &regs );
}


/*
 * Name:    set_clipboard
 * Purpose: put text onto the Windows clipboard
 * Author:  Jason Hood
 * Date:    August 27, 2002
 * Passed:  text:  the text to place
 *          size:  the length of the text (including NUL)
 * Returns: OK if successfully placed, ERROR otherwise
 */
int  set_clipboard( char *text, int size )
{
int  seg, sel;
__dpmi_regs regs;
int  rc = ERROR;

   if (open_clipboard( )) {
      seg = __dpmi_allocate_dos_memory( (size + 15) >> 4, &sel );
      if (seg != -1) {
         dosmemput( text, size, seg << 4 );
         regs.x.ax = 0x1702;            /* empty clipboard */
         __dpmi_int( 0x2f, &regs );
         regs.x.ax = 0x1703;            /* place data on clipboard */
         regs.x.dx = 7;                 /* OEM text */
         regs.x.es = seg;               /* ES:BX pointer to data */
         regs.x.bx = 0;
         regs.x.si = (unsigned)size >> 16; /* SI:CX size of data */
         regs.x.cx = size & 0xffff;
         __dpmi_int( 0x2f, &regs );
         if (regs.x.ax != 0)
            rc = OK;
         __dpmi_free_dos_memory( sel );
      }
      close_clipboard( );
   }
   return( rc );
}


/*
 * Name:    get_clipboard
 * Purpose: get text from the Windows clipboard
 * Author:  Jason Hood
 * Date:    August 26, 2002
 * Returns: pointer to the text or NULL if none/error
 * Notes:   the caller should free the returned pointer
 */
char *get_clipboard( void )
{
int  seg, sel;
int  size;
__dpmi_regs regs;
char *text = NULL;

   if (open_clipboard( )) {
      regs.x.ax = 0x1704;               /* clipboard size in DX:AX */
      regs.x.dx = 7;                    /* OEM text */
      __dpmi_int( 0x2f, &regs );
      size = (regs.x.dx << 16) + regs.x.ax;
      if (size) {
         seg = __dpmi_allocate_dos_memory( (size + 15) >> 4, &sel );
         if (seg != -1) {
            regs.x.ax = 0x1705;         /* get data from clipboard */
            regs.x.dx = 7;              /* OEM Text */
            regs.x.es = seg;            /* ES:BX pointer to buffer */
            regs.x.bx = 0;
            __dpmi_int( 0x2f, &regs );
            if (regs.x.ax != 0) {
               text = malloc( size );
               if (text != NULL)
                  dosmemget( seg << 4, size, text );
            }
            __dpmi_free_dos_memory( sel );
         }
      }
      close_clipboard( );
   }
   return( text );
}
