/*
 * Now that I own both MSC 7.0 and BC 3.1 and have linux, lets rearrange stuff
 * so many compilers can compile TDE.  Several implementation specific
 * functions needed for several environments were gathered into this file.
 *
 * In version 3.2, these functions changed to support unix.
 * In version 5.1g, these functions changed to support Win32 console.
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


/*
 * Name:    my_heapavail
 * Purpose: available free memory
 * Date:    August 6, 2002
 * Notes:   just return the physical memory available
 */
long my_heapavail( void )
{
MEMORYSTATUS mst;

   mst.dwLength = sizeof(MEMORYSTATUS);
   GlobalMemoryStatus( &mst );
   return( mst.dwAvailPhys );
}


static char  found_name[NAME_MAX+2];
static FTYPE found = { found_name, 0, 0, { 0, 0 } };


/*
 * Name:    find_wild
 * Purpose: find a file matching an "extended" pattern
 * Author:  Jason Hood
 * Date:    December 3, 2003
 * Passed:  dta: file finding info
 * Notes:   my_find{first,next} MUST be successfully called first.
 *          see my_findfirst notes.
 */
static FTYPE *find_wild( FFIND *dta, WIN32_FIND_DATA *wfd )
{
int  i;

   i = !0;
   do {
      if (wfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
         if (dta->dirs == NO_DIRS)
            continue;
         if (dta->dirs == ALL_DIRS)
            break;
      }
      if (wildcard( dta->pattern, wfd->cFileName ) == TRUE)
         break;
   } while ((i = FindNextFile( dta->find_info, wfd )) != 0);

   if (i == 0) {
      FindClose( dta->find_info );
      return( NULL );   /* all done */
   }

   strcpy( found.fname, wfd->cFileName );
   if (dta->dirs) {
      found.fsize = wfd->nFileSizeLow;
      found.fattr = wfd->dwFileAttributes;
      found.ftime = wfd->ftLastWriteTime;
      if (wfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
         strcat( found.fname, "/" );
   }
   return( &found );
}


/*
 * Name:    my_findfirst
 * Purpose: find the first file matching a pattern
 * Date:    August 6, 2002
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
 *          Assumes a 32-bit file size.
 */
FTYPE *my_findfirst( char *path, FFIND *dta, int dirs )
{
int  i;
char temp[PATH_MAX];
WIN32_FIND_DATA wfd;

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
   join_strings( temp, dta->stem, "*" );
   dta->find_info = FindFirstFile( temp, &wfd );
   if (dta->find_info == INVALID_HANDLE_VALUE)
      return( NULL );   /* empty directory? */

   /* ignore the "." entry (assuming it to be found in this initial scan) */
   if (dirs && wfd.cFileName[0] == '.' && wfd.cFileName[1] == '\0')
      return( my_findnext( dta ) );

   return( find_wild( dta, &wfd ) );
}


/*
 * Name:    my_findnext
 * Purpose: find the next file matching a pattern
 * Date:    August 6, 2002
 * Passed:  dta: file finding info
 * Notes:   my_findfirst() MUST be called before calling this function.
 *          Returns NULL if no more matching names;
 *          otherwise same as my_findfirst.
 */
FTYPE *my_findnext( FFIND *dta )
{
WIN32_FIND_DATA wfd;

   if (FindNextFile( dta->find_info, &wfd ) != 0)
      return( find_wild( dta, &wfd ) );

   FindClose( dta->find_info );
   return( NULL );   /* all done */
}


/*
 * Name:    get_fattr
 * Purpose: To get dos file attributes
 * Date:    August 6, 2002
 * Passed:  fname: ASCIIZ file name.  Null terminated file name
 *          fattr: pointer to file attributes
 * Returns: 0 if successful, non zero if not
 *
 * jmh 031027: return proper error code:
 *                 2 (ERROR_FILE_NOT_FOUND; standard DOS error code)
 *                 3 (ERROR_PATH_NOT_FOUND; standard DOS error code)
 *               161 (ERROR_BAD_PATHNAME; extended Windows error code?)
 */
int  get_fattr( char *fname, fattr_t *fattr )
{
int  rc;                /* return code */

   assert( fname != NULL  &&  fattr != NULL);

   rc = OK;
   *fattr = GetFileAttributes( fname );
   if (*fattr == 0xFFFFFFFF) {
      *fattr = 0;
      rc = GetLastError( );
   }
   return( rc );
}


/*
 * Name:    set_fattr
 * Purpose: To set dos file attributes
 * Date:    August 6, 2002
 * Passed:  fname: ASCIIZ file name.  Null terminated file name
 *          fattr: file attributes
 * Returns: OK if successful, ERROR if not
 */
int  set_fattr( char *fname, fattr_t fattr )
{
int  rc;                /* return code */

   assert( fname != NULL );

   rc = SetFileAttributes( fname, fattr );
   rc = (rc == 0) ? ERROR : OK;
   return( rc );
}


/*
 * Name:    get_current_directory
 * Purpose: get current directory
 * Date:    August 6, 2002
 * Passed:  path:  pointer to buffer to store path
 * Notes:   append a trailing slash ('/') if it's not root
 *          path is expected to be at least PATH_MAX long
 */
int  get_current_directory( char *path )
{
   assert( path != NULL );

   if (GetCurrentDirectory( PATH_MAX, path ) == 0)
      return( ERROR );

   get_full_path( path, path );

   path += strlen( path );
   if (path[-1] != '/') {
      *path   = '/';
      path[1] = '\0';
   }

   return( OK );
}


/*
 * Name:    set_current_directory
 * Purpose: set current directory
 * Date:    August 6, 2002
 * Passed:  new_path: directory path, which may include drive letter
 (
 * 050706:  set it in the environment as well, to preserve it across
 *           drive changes.
 */
int  set_current_directory( char *new_path )
{
register int  rc;
char var[4], cwd[PATH_MAX+2];

   assert( new_path != NULL );

   rc = (SetCurrentDirectory( new_path ) == 0) ? ERROR : OK;
   if (rc == OK) {
      GetCurrentDirectory( PATH_MAX, cwd );
      var[0] = '=';
      var[1] = *cwd;
      var[2] = ':';
      var[3] = '\0';
      SetEnvironmentVariable( var, cwd );
   }

   return( rc );
}


/*
 * Name:    get_full_path
 * Purpose: retrieve the fully-qualified path name for a file
 * Date:    August 10, 2002
 * Passed:  in_path:  path to be canonicalized
 *          out_path: canonicalized path
 * Notes:   out_path is assumed to be PATH_MAX characters.
 *          leave UNC paths alone.
 *          I'm sure I'm doing this wrong, but I was unable to find an
 *           API function that guaranteed a long name.
 *
 * 021019:  preserve a trailing slash.
 */
void get_full_path( char *in_path, char *out_path )
{
char fullpath[MAX_PATH];
char *name, *part;
WIN32_FIND_DATA wfd;
HANDLE h;
int  len, slash;

   len = strlen( in_path ) - 1;
   slash = (in_path[len] == '/'  ||  in_path[len] == '\\');

   if ((in_path[0] == '/' || in_path[0] == '\\')  &&
       (in_path[1] == '/' || in_path[1] == '\\')) {
      do
         *out_path++ = (*in_path == '/') ? '\\' : *in_path;
      while (*in_path++);

   } else if (GetFullPathName(in_path, sizeof(fullpath), fullpath, &name) == 0)
      strcpy( out_path, in_path );

   else {
      out_path[0] = (*fullpath < 'a') ? *fullpath + 32 : *fullpath;
      out_path[1] = ':';
      out_path[2] = '\0';
      part = fullpath + 2;
      for (;;) {
         strcat( out_path, "/" );
         part = strchr( part+1, '\\' );
         if (part != NULL)
            *part = '\0';
         h = FindFirstFile( fullpath, &wfd );
         if (h == INVALID_HANDLE_VALUE) {
            if (name != NULL)
               strcat( out_path, name );
            break;
         } else
            FindClose( h );
         strcat( out_path, wfd.cFileName );
         if (part == NULL)
            break;
         *part = '\\';
      }
   }

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
HANDLE handle;
int  rc = ERROR;

   handle = CreateFile( fname, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );
   if (handle != INVALID_HANDLE_VALUE) {
      if (GetFileTime( handle, NULL, NULL, ftime ))
         rc = OK;
      CloseHandle( handle );
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
HANDLE handle;
int  rc = ERROR;

   handle = CreateFile( fname, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL );
   if (handle != INVALID_HANDLE_VALUE) {
      if (SetFileTime( handle, NULL, NULL, ftime ))
         rc = OK;
      CloseHandle( handle );
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
FILETIME ft;
SYSTEMTIME st;

   FileTimeToLocalFileTime( ftime, &ft );
   FileTimeToSystemTime( &ft, &st );
   t.tm_year = st.wYear - 1900;      /* tm is from 1900, st is the year */
   t.tm_mon  = st.wMonth - 1;        /* tm is from 0, st is from 1 */
   t.tm_mday = st.wDay;
   t.tm_hour = st.wHour;
   t.tm_min  = st.wMinute;
   t.tm_sec  = st.wSecond;

   return( &t );
}


/*
 * Name:    set_clipboard
 * Purpose: put text onto the Windows clipboard
 * Author:  Jason Hood
 * Date:    August 26, 2002
 * Passed:  text:  the text to place
 *          size:  the length of the text (including NUL)
 * Returns: OK if successfully placed, ERROR otherwise
 */
int  set_clipboard( char *text, int size )
{
HANDLE clip;
char *data;
int  rc = ERROR;

   if (OpenClipboard( NULL )) {
      EmptyClipboard( );
      clip = GlobalAlloc( GMEM_DDESHARE, size );
      if (clip != NULL) {
         data = GlobalLock( clip );
         if (data != NULL) {
            memcpy( data, text, size );
            GlobalUnlock( clip );
            SetClipboardData( CF_OEMTEXT, clip );
            rc = OK;
         }
      }
      CloseClipboard( );
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
HANDLE clip;
char *text = NULL;

   if (IsClipboardFormatAvailable( CF_OEMTEXT )  &&
       OpenClipboard( NULL )) {
      clip = GetClipboardData( CF_OEMTEXT );
      if (clip != NULL) {
         text = GlobalLock( clip );
         if (text != NULL) {
            text = strdup( text );
            GlobalUnlock( clip );
         }
      }
      CloseClipboard( );
   }

   return( text );
}
