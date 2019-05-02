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
#include <sys/types.h>
#include <utime.h>
#include <errno.h>


/*
 * Name:    my_heapavail
 * Purpose: available free memory
 * Date:    June 5, 1994
 * Notes:   use /proc/meminfo to find available free memory.  let's count
 *            the free memory in the swap space(s), too.
 */
long my_heapavail( void )
{
FILE *f;                /* file pointer */
char buf[82];           /* let's hope lines in /proc/meminfo are < 80 chars */
char *p;                /* pointer walks down line in buf */
long mem;               /* free memory */

   mem = 0;
   if ((f = fopen( "/proc/meminfo", "r" )) != NULL) {
      fgets( buf, 80, f );
      for (;;) {
         fgets( buf, 80, f );
         if (feof( f ))
            break;
         /*
          * assume line looks like:
          *   "mem:   total used free buffers"
          */
         p = buf;
         while (*p++ == ' ');
         while (*p++ != ' ');
         while (*p++ == ' ');
         while (*p++ != ' ');
         while (*p++ == ' ');
         while (*p++ != ' ');
         while (*p++ == ' ');
         --p;
         mem += atol( p );
      }
      fclose( f );
   }
   return( mem );
}


/*
 * Name:    my_ltoa
 * Purpose: ltoa is not ANSI - write our own
 * Date:    November 13, 1993
 * Passed:  lnum:   number to convert to ASCII.  in linux, an int is 32 bits.
 *          s:      pointer to buffer
 *          radix:  2 <= radix <= 36
 * Notes:   store the ascii string in a 33 character stack.
 *
 * jmh 031119: if radix is not 10, treat number as unsigned;
 *             allow radix up to 36 (and minimum must be 2, not 1).
 */
char *my_ltoa( int lnum, char *s, int radix )
{
unsigned n;
static const char digit[] = "0123456789abcdefghijklmnopqrstuvwxyz";
char stack[33];
char *sp;
char *p;

   if (radix < 0)
      radix = -radix;

   /*
    * default an empty string.
    */
   *s = '\0';
   if (radix >= 2 && radix <= 36) {
      p = s;
      if (lnum < 0 && radix == 10) {
         *p++ = '-';
         n = -lnum;
      } else
         n = lnum;

      /*
       * put a '\0' at the beginning of our stack.
       *
       * standard procedure: generate the digits in reverse order.
       */
      sp  = stack;
      *sp = '\0';
      do {
         *++sp = digit[n % radix];
         n /= radix;
      } while (n);

      /*
       * now, pop the ascii digits off the stack.  the '\0' that we stored
       *  at the beginning of the stack terminates the string copy.
       */
      while ((*p++ = *sp--));
   }
   return( s );
}


static char  found_name[NAME_MAX+2];
static FTYPE found = { found_name, 0, 0, 0 };


/*
 * Name:    my_findfirst
 * Purpose: find the first file matching a pattern
 * Date:    August 5, 1997
 * Passed:  path: path and pattern to search for files
 *          dta:  file finding info
 *          dirs: NO_DIRS to ignore all directories
 *                MATCH_DIRS to match directory names
 *                ALL_DIRS to include all directories and return file sizes
 * Notes:   Returns NULL for no matching files or bad pattern;
 *          otherwise a pointer to a static FTYPE, with fname holding the
 *          filename that was found.
 *          If dirs is TRUE fsize will hold the size of the file, and the
 *          name will have a trailing slash ('/') if it's a directory.
 *          Only files with read access will be returned.
 */
FTYPE *my_findfirst( char *path, FFIND *dta, int dirs )
{
int  i;
struct stat fstat;

   dta->dirs = dirs;
   /*
    * Separate the path and pattern
    */
   i = strlen( path ) - 1;
   if (i == -1) {
      get_current_directory( dta->stem );
      strcpy( dta->pattern, "*" );

   } else if (path[i] == '/') {
      strcpy( dta->stem, path );
      strcpy( dta->pattern, "*" );

   } else if (stat( path, &fstat ) == 0 && S_ISDIR( fstat.st_mode )) {
      join_strings( dta->stem, path, "/" );
      strcpy( dta->pattern, "*" );

   } else {
      while (--i >= 0) {
         if (path[i] == '/')
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
   dta->find_info = opendir( dta->stem );
   if (dta->find_info == NULL)
      return( NULL );

   return( my_findnext( dta ) );
}


/*
 * Name:    my_findnext
 * Purpose: find the next file matching a pattern
 * Date:    August 5, 1997
 * Passed:  dta: file finding info
 * Notes:   my_findfirst() MUST be called before calling this function.
 *          Returns NULL if no more matching names;
 *          otherwise same as my_findfirst.
 *          Only files with read access will be returned.
 */
FTYPE *my_findnext( FFIND *dta )
{
struct stat fstat;
struct dirent *dir;
char temp[PATH_MAX];
char *name;

   strcpy( temp, dta->stem );
   name = temp + strlen( temp );

   for (;;) {
      dir = readdir( dta->find_info );
      if (dir == NULL) {
         closedir( dta->find_info );
         return( NULL );
      }
      strcpy( name, dir->d_name );
      if (access( temp, R_OK ) != OK)
         continue;
      stat( temp, &fstat );
      if (S_ISDIR( fstat.st_mode )) {
         if (dta->dirs == NO_DIRS)
            continue;
         /* ignore the "." entry */
         if (dir->d_name[0] == '.' && dir->d_name[1] == '\0')
            continue;
         if (dta->dirs == ALL_DIRS ||
             wildcard( dta->pattern, dir->d_name ) == TRUE)
            break;
      }
      if (S_ISREG( fstat.st_mode ) &&
          wildcard( dta->pattern, dir->d_name ) == TRUE)
         break;
   }
   strcpy( found.fname, dir->d_name );
   if (dta->dirs) {
      found.fsize = fstat.st_size;
      found.fattr = fstat.st_mode;
      found.ftime = fstat.st_mtime;
      if (S_ISDIR( fstat.st_mode ))
         strcat( found.fname, "/" );
   }
   return( &found );
}


/*
 * Name:    get_fattr
 * Purpose: To get unix file attributes
 * Date:    November 13, 1993
 * Passed:  fname: ASCIIZ file name.  Null terminated file name
 *          fattr: pointer to file attributes
 * Returns: 0 if successful, non zero if not
 * Notes:   Use stat( ) to get file attributes.
 *
 * jmh 031027: return 2 if errno == ENOENT, otherwise errno.
 */
int  get_fattr( char *fname, fattr_t *fattr )
{
int  rc;                /* return code */
struct stat fstat;

   assert( fname != NULL  &&  fattr != NULL);

   rc = OK;
   if (stat( fname, &fstat) != ERROR)
      *fattr = fstat.st_mode;
   else {
      *fattr = 0;
      rc = (errno == ENOENT) ? 2 : errno;
   }
   return( rc );
}


/*
 * Name:    set_fattr
 * Purpose: To set unix file attributes
 * Date:    November 13, 1993
 * Passed:  fname: ASCIIZ file name.  Null terminated file name
 *          fattr: mode_t file attributes
 * Returns: OK if successful, ERROR if not
 * Notes:   Use chmod( ) function to set file attributes.  To change a
 *           file mode, the effective user ID of the process must equal the
 *           owner of the file, or be a superuser.
 */
int  set_fattr( char *fname, fattr_t fattr )
{
int  rc;                /* return code */

   assert( fname != NULL );

   rc = chmod( fname, fattr );
   return( rc );
}


/*
 * Name:    get_current_directory
 * Purpose: get current directory
 * Date:    November 13, 1993
 * Passed:  path:  pointer to buffer to store path
 * Notes:   path is expected to be at least PATH_MAX long
 *          append a trailing slash if not present
 */
int  get_current_directory( char *path )
{
register int  rc;

   assert( path != NULL );

   rc = OK;
   if (getcwd( path, PATH_MAX ) == NULL)
      rc = ERROR;
   else {
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
 * Passed:  new_path: directory path
 */
int  set_current_directory( char *new_path )
{
register int  rc;

   assert( new_path != NULL );

   rc = chdir( new_path );
   return( rc );
}


/*
 * Name:    get_full_path
 * Purpose: retrieve the fully-qualified path name for a file
 * Date:    May 3, 1998
 * Passed:  in_path:  path to be canonicalized
 *          out_path: canonicalized path
 * Notes:   out_path is assumed to be PATH_MAX characters.
 *          I don't know if UNIX/gcc provides a function to do this (djgpp has
 *          _fixpath, but is that DJ or gcc?), so I'll just copy it.
 * jmh 990408: use realpath().
 * jmh 021020: preserve a trailing slash.
 */
void get_full_path( char *in_path, char *out_path )
{
int  len, slash;

   len = strlen( in_path ) - 1;
   slash = (in_path[len] == '/'  ||  in_path[len] == '\\');
   if (realpath( in_path, out_path ) == NULL)
      strcpy( out_path, in_path );
   else if (slash) {
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
struct stat st;
int  rc = OK;

   if (stat( fname, &st ) == 0)
      *ftime = st.st_mtime;
   else
      rc = ERROR;

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
 *          also sets the access time to ftime.
 */
int  set_ftime( char *fname, ftime_t *ftime )
{
struct utimbuf ut;

   ut.modtime = ut.actime = *ftime;
   return( (utime( fname, &ut ) == 0) ? OK : ERROR );
}
