/*
 * Now that I own both MSC 7.0 and BC 3.1 and have linux, lets rearrange stuff
 * so many compilers can compile TDE.  Several implementation specific
 * functions needed for several environments were gathered into this file.
 *
 * In version 3.2, these functions changed to support unix.
 *
 * Although many PC C compilers have findfirst and findnext functions for
 * finding files, let's write our own to keep a closer watch on
 * critical errors.
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

#if defined( __MSC__ )
#include <dos.h>
#endif


/*
 * Name:    my_heapavail
 * Purpose: available free memory from the far heap
 * Date:    November 13, 1993
 */
long my_heapavail( void )
{
long avail_mem;

#if defined( __MSC__ )
unsigned paragraphs;

   _dos_allocmem( 0xffff, &paragraphs );
   /*
    * A paragraph is 16 bytes.  Convert paragraphs to bytes by shifting left
    * 4 bits.
    */
   avail_mem = (long)paragraphs << 4;
#else

   /*
    * use the Borland farcoreleft( ) function.
    */
   avail_mem = farcoreleft( );
#endif
   return( avail_mem );
}


static int  find_first( DTA *dta, char *path, int f_attr );
static int  find_next(  DTA *dta );
static FTYPE *find_wild( FFIND *dta );

static char  found_name[NAME_MAX+2];
static FTYPE found = { found_name, 0, 0, 0 };


/*
 * Name:    find_first
 * Purpose: find the first file matching a pattern using DOS interrupt
 * Date:    January 6, 1992
 * Passed:  dta:    disk transfer address
 *          path:   path to search for files
 *          f_attr: attributes of files to search for
 * Notes:   return codes for find_first:
 *             0  no error
 *             2  file is invalid or does not exist
 *             3  path is invalid or does not exist
 *            18  no matching directory entry was found
 *            -1  check the critical error flag for critical errors
 *
 * jmh 031202: ignore the "." entry here (it's always found first).
 */
static int find_first( DTA *dta, char *path, int f_attr )
{
void *old_dta;
int  rc;

   ASSEMBLE {

/*
; save the old dta
*/
        mov     ah, 0x2f                /* DOS get dta */
        int     0x21                    /* DOS interrupt */
        mov     WORD PTR old_dta, bx    /* save OFFSET of old DTA */
        mov     WORD PTR old_dta+2, es  /* save SEGMENT of old DTA */

/*
; set the new dta
*/
        push    ds                      /* save ds */

        lds     dx, dta                 /* get SEGMENT & OFFSET of new dta */
        mov     ah, 0x1a                /* DOS set dta */
        int     0x21                    /* DOS interrupt */

/*
; find first matching file
*/
        push    ds                      /* save ds of DTA */
        mov     cx, WORD PTR f_attr     /* file attributes to search for */
        lds     dx, path                /* get SEGMENT & OFFSET of path */
        mov     ah, 0x4e                /* DOS find first file */
        int     0x21                    /* DOS interrupt */
        pop     ds                      /* get back ds of DTA */

/*
; save the return code
*/
        jc      an_error                /* carry is set if an error occured */

/*
; if file name begins with '.' it must be the "." entry - ignore it
*/
        mov     bx, WORD PTR dta        /* offset of DTA */
        cmp     BYTE PTR [bx+30], '.'   /* file name is at offset 30 */
        jne     no_error                /* not dot, found first file */
        mov     ah, 0x4f                /* DOS find next file */
        int     0x21                    /* DOS interrupt */
        jc      an_error
   }
no_error:

   ASSEMBLE {
        xor     ax, ax                  /* zero out ax, return OK if no error */
   }
an_error:

   ASSEMBLE {
        mov     WORD PTR rc, ax         /* save the return code */

/*
; get back old dta
*/
        lds     dx, old_dta             /* get SEGMENT & OFFSET of old dta */
        mov     ah, 0x1a                /* DOS set dta */
        int     0x21                    /* DOS interrupt */

        pop     ds                      /* get back ds */
   }
   if (ceh.flag == ERROR)
      rc = ERROR;
   return( rc );
}


/*
 * Name:    find_next
 * Purpose: find the next file matching a pattern using DOS interrupt
 * Date:    January 6, 1992
 * Passed:  dta:  disk transfer address
 * Notes:   find_first() MUST be called before calling this function.
 *          return codes for find_next (see DOS tech ref manuals):
 *             0  no error
 *             2  path is invalid or does not exist
 *            18  no matching directory entry was found
 *            -1  check the critical error flag for critical errors
 */
static int find_next( DTA *dta )
{
void *old_dta;
int  rc;

   ASSEMBLE {

/*
; save the old dta
*/
        mov     ah, 0x2f                /* DOS get dta */
        int     0x21                    /* DOS interrupt */
        mov     WORD PTR old_dta, bx    /* save OFFSET of old DTA */
        mov     WORD PTR old_dta+2, es  /* save SEGMENT of old DTA */

/*
; set the new dta
*/
        push    ds                      /* save ds */

        lds     dx, dta                 /* get SEGMENT & OFFSET of new dta */
        mov     ah, 0x1a                /* DOS set dta */
        int     0x21                    /* DOS interrupt */

/*
; find next matching file
*/
        mov     ah, 0x4f                /* DOS find next file */
        int     0x21                    /* DOS interrupt */

/*
; save the return code
*/
        jc      an_error                /* carry is set if an error occured */
        xor     ax, ax                  /* zero out ax, return OK if no error */
   }
an_error:

   ASSEMBLE {
        mov     WORD PTR rc, ax         /* save the return code */

/*
; get back old dta
*/
        lds     dx, old_dta             /* get SEGMENT & OFFSET of old dta */
        mov     ah, 0x1a                /* DOS set dta */
        int     0x21                    /* DOS interrupt */

        pop     ds                      /* get back ds */
   }
   if (ceh.flag == ERROR)
      rc = ERROR;
   return( rc );
}


/*
 * Name:    find_wild
 * Purpose: find a file matching an "extended" wildcard pattern
 * Author:  Jason Hood
 * Date:    December 2, 2003
 * Passed:  dta: file finding info
 * Notes:   my_find{first,next} MUST have been successfully called first;
 *          see my_findfirst notes.
 */
static FTYPE *find_wild( FFIND *dta )
{
int  i;
char *name;
char *path;

   i = OK;
   do {
      if (dta->dirs == ALL_DIRS && (dta->find_info.attrib & SUBDIRECTORY))
         break;
      if (wildcard( dta->pattern, dta->find_info.name ) == TRUE)
         break;
      i = find_next( &dta->find_info );
   } while (i == OK);

   if (i != OK)
      return( NULL );   /* all done */

   name = dta->find_info.name;
   path = found.fname;
   while ((*path = bj_tolower( *name )) != '\0') {
      name++;
      path++;
   }
   if (dta->dirs) {
      found.fsize = dta->find_info.size;
      found.fattr = dta->find_info.attrib;
      found.ftime = dta->find_info.time;
      if (dta->find_info.attrib & SUBDIRECTORY) {
         *path   = '/';
         path[1] = '\0';
      }
   }

   return( &found );
}


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
 *          The name is converted to lower-case.
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

   if (find_first( &dta->find_info, temp, fattr ) == OK)
      return( find_wild( dta ) );

   return( NULL );
}


/*
 * Name:    my_findnext
 * Purpose: find the next file matching a pattern
 * Date:    August 5, 1997
 * Passed:  dta: file finding info
 * Notes:   my_findfirst() MUST be called before calling this function.
 *          Returns NULL if no more matching names;
 *          otherwise same as my_findfirst.
 */
FTYPE *my_findnext( FFIND *dta )
{
   return( (find_next( &dta->find_info ) == OK) ? find_wild( dta ) : NULL );
}


/*
 * Name:    get_fattr
 * Purpose: To get dos file attributes
 * Date:    December 26, 1991
 * Passed:  fname: ASCIIZ file name.  Null terminated file name
 *          fattr: pointer to file attributes
 * Returns: 0 if successful, non zero if not
 * Notes:   Uses the DOS function to get file attributes.  I really didn't
 *           like the file attribute functions in the C library:  fstat() and
 *           stat() or access() and chmod().
 *           FYI, File Attributes:
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
 * jmh 020924: remove a trailing (back)slash.
 */
int  get_fattr( char *fname, fattr_t *fattr )
{
int  rc;                /* return code */
int  attr;

   ASSEMBLE {
        push    ds
        lds     dx, fname               /* get SEGMENT & OFFSET of filename */

        mov     bx, dx
        dec     bx
        sub     al, al
   }
find_nul:
   ASSEMBLE {
        inc     bx
        cmp     [bx], al
        jnz     find_nul
        dec     bx                      /* bx is at the last character */
        cmp     byte ptr [bx], '/'
        je      slash
        cmp     byte ptr [bx], 92
        jne     do_chmod
   }
slash:
   ASSEMBLE {
        cmp     bx, dx                  /* make sure it's not the only char. */
        je      do_chmod
        cmp     byte ptr [bx-1], ':'    /* test for the drive specifier */
        je      do_chmod
        mov     [bx], al                /* chop off the trailing slash */
   }
do_chmod:
   ASSEMBLE {
        mov     ax, 0x4300              /* function:  get file attributes */
        int     0x21                    /* DOS interrupt */
        pop     ds

        jnc     no_error                /* save the error code from get attr */
        xor     cx, cx                  /* if error, then zero out cx - attrs */
        jmp     SHORT get_out           /* lets get out */
   }
no_error:


   ASSEMBLE {
        xor     ax, ax                  /* if no carry, no error */
   }
get_out:

   ASSEMBLE {
        mov     WORD PTR rc, ax         /* ax contains error number on error */
        mov     WORD PTR attr, cx       /* cx contains file attributes */
   }
   *fattr = attr;
   if (ceh.flag == ERROR)
      rc = ERROR;
   return( rc );
}


/*
 * Name:    set_fattr
 * Purpose: To set dos file attributes
 * Date:    December 26, 1991
 * Passed:  fname: ASCIIZ file name.  Null terminated file name
 *          fattr: file attributes
 * Returns: 0 if successfull, non zero if not
 * Notes:   Uses the DOS function to get file attributes.
 *           Return codes:
 *              0 = No error
 *              1 = AL not 0 or 1
 *              2 = file is invalid or does not exist
 *              3 = path is invalid or does not exist
 *              5 = Access denied
 */
int  set_fattr( char *fname, fattr_t fattr )
{
int  rc;                /* return code */

   ASSEMBLE {
        push    ds
        lds     dx, fname               /* get SEGMENT & OFFSET of filename */
        mov     cx, WORD PTR fattr      /* cx contains file attributes */
        mov     ax, 0x4301              /* function:  get file attributes */
        int     0x21                    /* DOS interrupt */
        pop     ds

        jc      get_out                 /* save the error code from get attr */
        xor     ax, ax                  /* if no carry, no error */
   }
get_out:

   ASSEMBLE {
        mov     WORD PTR rc, ax         /* ax contains error number on error */
   }
   if (ceh.flag == ERROR)
      rc = ERROR;
   return( rc );
}


/*
 * Name:    get_current_directory
 * Purpose: get current directory
 * Date:    February 13, 1992
 * Passed:  path:  pointer to buffer to store path
 * Notes:   use simple DOS interrupt
 *          path is expected to be at least PATH_MAX long
 *          In the interests of uniform behaviour, convert backslashes to
 *           slashes (ie. '\' to '/') and use lower-case. Append the
 *           directory with a final slash.
 *
 * jmh 980503: store the drive specifier as well.
 */
int  get_current_directory( char *path )
{
int  rc;

   ASSEMBLE {
        push    si                      /* save register vars if any */
        push    ds                      /* save ds */

        lds     si, DWORD PTR path      /* get SEGMENT & OFFSET of path */
        mov     ah, 0x19                /* get default drive, 0 = a,... */
        int     0x21
        add     al, 'a'                 /* make it a lowercase letter */
        mov     BYTE PTR [si], al       /* store the drive specification */
        inc     si
        mov     WORD PTR [si], ':' or ('/' shl 8)
        inc     si
        inc     si
        mov     ah, 0x47                /* function 0x47 == get current dir */
        mov     dl, 0                   /* default drive */
        int     0x21                    /* standard DOS interrupt */
        mov     ax, 0                   /* return 0 if no error */
        sbb     ax, ax                  /* if carry set, then an error */
        pop     ds                      /* get back ds */
        pop     si                      /* get back si */
        mov     WORD PTR rc, ax         /* save return code */
   }
   if (ceh.flag == ERROR)
      rc = ERROR;
   else {
      unixify( path );
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
 * Date:    February 13, 1992
 * Passed:  new_path: directory path, which may include drive letter
 * Notes:   use simple DOS interrupt
 *
 * jmh 021021: change drive, too;
 *             handle the trailing (back)slash (the other OS's can handle it).
 */
int  set_current_directory( char *new_path )
{
int  rc;
char chopped = FALSE;

   ASSEMBLE {
        push    ds                      /* save ds */
        lds     bx, new_path            /* get SEGMENT & OFFSET of new_path */
        mov     dx, bx                  /* strip any trailing slash */
        sub     al, al                  /* NUL character */
        dec     bx                      /* prime for loop */
   }
find_nul:
   ASSEMBLE {
        inc     bx
        cmp     [bx], al
        jnz     find_nul
        dec     bx
        cmp     BYTE PTR [bx], '/'
        je      slash
        cmp     BYTE PTR [bx], 92
        jne     ch_dir
   }
slash:
   ASSEMBLE {
        cmp     bx, dx                  /* make sure it's not the only char. */
        je      ch_dir
        cmp     BYTE PTR [bx-1], ':'    /* test for the drive specifier */
        je      ch_dir
        mov     [bx], al                /* chop off the trailing slash */
        mov     chopped, TRUE           /* indicate we did so */
   }
ch_dir:
   ASSEMBLE {
        mov     ah, 0x3b                /* function 0x3b == set current dir */
        int     0x21                    /* standard DOS interrupt */
        pushf
        cmp     chopped, FALSE
        je      no_slash
        mov     BYTE PTR [bx], '/'      /* too bad if it was a backslash */
   }
no_slash:
   ASSEMBLE {
        popf
        mov     ax, ERROR
        jc      get_out                 /* if carry set, then an error */
        sub     ax, ax                  /* no error if no drive */
        mov     bx, dx
        cmp     BYTE PTR [bx+1], ':'    /* drive as well? */
        jne     get_out
        mov     dl, [bx]                /* the drive letter */
        or      dl, 0x20                /* normalise to lower-case */
        sub     dl, 'a'                 /* letter to number */
        mov     ah, 0x0e                /* function 0x0e == set default drive */
        int     0x21
        mov     ax, 0
        sbb     ax, ax
   }
get_out:
   ASSEMBLE {
        pop     ds                      /* get back ds */
        mov     WORD PTR rc, ax         /* save return code */
   }
   if (ceh.flag == ERROR)
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
 *          out_path is converted to lower-case and backslashes to slashes.
 * 980511:  wrote an assembly version.
 * 051017:  remove and restore trailing slash.
 */
void get_full_path( char *in_path, char *out_path )
{
int  len;
int  slash;

   len = strlen( in_path );
   slash = (in_path[len-1] == '/'  ||  in_path[len-1] == '\\');
   if (slash  &&  len > 1  &&  in_path[len-2] != ':')
      in_path[len-1] = '\0';

   ASSEMBLE {
        push    ds
        push    si
        push    di

        lds     si, DWORD PTR in_path   /* ds:si = in_path */
        les     di, DWORD PTR out_path  /* es:di = out_path */
        mov     ah, 0x60                /* function 0x60 == truename */
        int     0x21
        jnc     get_out                 /* no error */
   }
   chloop:
   ASSEMBLE {
        lodsb                           /* strcpy( out_path, in_path ) */
        stosb
        test    al, al
        jnz     chloop
   }
get_out:
   ASSEMBLE {
        pop     di
        pop     si
        pop     ds
   }
   unixify( out_path );

   if (slash) {
      if (in_path[len-1] == '\0')
         in_path[len-1] = '/';
      len = strlen( out_path );
      if (out_path[len-1] != '/') {
         out_path[len]   = '/';
         out_path[len+1] = '\0';
      }
   }
}


/*
 * Name:    get_ftime
 * Purpose: get the file's timestamp
 * Author:  Jason Hood
 * Date:    March 20, 2003
 * Passed:  fname:  pointer to file's name
 *          ftime:  pointer to store the timestamp
 * Returns: OK if successful, ERROR if not (ftime unchanged).
 */
int  get_ftime( char *fname, ftime_t *ftime )
{
int  rc = ERROR;

   ASSEMBLE {
        push    ds
        lds     dx, fname
        mov     ax, 0x3d00              /* open file, read-only */
        int     0x21
        jc      err
        mov     bx, ax
        mov     ax, 0x5700              /* get timestamp */
        int     0x21
        pushf
        mov     ah, 0x3e                /* close file */
        int     0x21
        popf
        jc      err
        lds     bx, ftime
        mov     [bx], cx                /* time */
        mov     [bx+2], dx              /* date */
        sub     ax, ax
        mov     rc, ax
   }
err:
   ASSEMBLE {
        pop     ds
   }
   return( rc );
}


/*
 * Name:    set_ftime
 * Purpose: set the file's timestamp
 * Author:  Jason Hood
 * Date:    March 20, 2003
 * Passed:  fname:  pointer to file's name
 *          ftime:  pointer to the timestamp
 * Returns: OK if successful, ERROR if not.
 */
int  set_ftime( char *fname, ftime_t *ftime )
{
int  rc = ERROR;

   ASSEMBLE {
        push    ds
        lds     dx, fname
        mov     ax, 0x3d00              /* open file, read-only */
        int     0x21
        jc      err
        lds     bx, ftime
        mov     cx, [bx]                /* time */
        mov     dx, [bx+2]              /* date */
        mov     bx, ax
        mov     ax, 0x5701              /* set timestamp */
        int     0x21
        pushf
        mov     ah, 0x3e                /* close file */
        int     0x21
        popf
        jc      err
        sub     ax, ax
        mov     rc, ax
   }
err:
   ASSEMBLE {
        pop     ds
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
unsigned int ft;

   ft = (unsigned int)(*ftime >> 16);
   t.tm_year = (ft >> 9) + 80;          /* tm is from 1900, ft is from 1980 */
   t.tm_mon  = ((ft >> 5) & 0x0f) - 1;  /* tm is from 0, ft is from 1 */
   t.tm_mday = ft & 0x1f;

   ft = (unsigned int)(*ftime);
   t.tm_hour = ft >> 11;
   t.tm_min  = (ft >> 5) & 0x3f;
   t.tm_sec  = (ft & 0x1f) << 1;        /* file time halves the seconds */

   return( &t );
}


/*
 * Name:    unixify
 * Purpose: convert a DOS path into a UNIX-like path
 * Author:  Jason Hood
 * Date:    June 4, 2001
 * Passed:  path:  path to convert
 * Notes:   backslashes are converted to slashes and letters are lowercased.
 */
void unixify( char *path )
{
   ASSEMBLE {
        les     si, path                /* SI is automatically preserved */
        jmp     short start
   }
uloop:
   ASSEMBLE {
        cmp     al, 92          /* '\\' doesn't work & '\' stuffs up the shl */
        jne     is_letter
        mov     al, '/'
        jmp     short store
   }
is_letter:
   ASSEMBLE {
        push    ax
        call    FAR PTR bj_tolower
        add     sp, 2
        mov     es, WORD PTR path+2
   }
store:
   ASSEMBLE {
        mov     es:[si], al
        inc     si
   }
start:
   ASSEMBLE {
        mov     al, es:[si]
        test    al, al
        jnz     uloop
   }
}
