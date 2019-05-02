/*
 EPSHeader

   File: filmatch.c
   Author: J. Kercheval
   Created: Thu, 03/14/1991  22:22:01
*/

/*
 EPSRevision History

   J. Kercheval  Wed, 02/20/1991  22:29:01  Released to Public Domain
   J. Kercheval  Fri, 02/22/1991  15:29:01  fix '\' bugs (two :( of them)
   J. Kercheval  Sun, 03/10/1991  19:31:29  add error return to matche()
   J. Kercheval  Sun, 03/10/1991  20:11:11  add is_valid_pattern code
   J. Kercheval  Sun, 03/10/1991  20:37:11  beef up main()
   J. Kercheval  Tue, 03/12/1991  22:25:10  Released as V1.1 to Public Domain
   J. Kercheval  Thu, 03/14/1991  22:22:25  remove '\' for DOS file parsing
   J. Kercheval  Thu, 03/28/1991  20:58:27  include filmatch.h

   Jason Hood, 1 August, 1997 - removed error testing in matche()
               4 August, 1997 - included TDE stuff, case testing
                                renamed match() to wildcard()
              10 August, 1997 - case testing in [] (oops)
              31 July, 2005   - match multiple patterns and exclusions
*/

/*
   Wildcard Pattern Matching
*/


#include "tdestr.h"
#include "tdefunc.h"
#include "common.h"


/*
 * jmh 990505: Make UNIX always case sensitive, DOS always case insensitive.
 */
#if defined( __UNIX__ )
#define bj_tolower( ch ) ch
#endif


int matche_after_star (register char *pattern, register char *text);


/*----------------------------------------------------------------------------
*
* Return TRUE if PATTERN has any special wildcard characters
*
----------------------------------------------------------------------------*/

BOOLEAN is_pattern (char *p)
{
    while ( *p ) {
        switch ( *p++ ) {
            case '?':
            case '*':
            case '[':
            case '!':
            case ';':
#if defined( __UNIX__ )
            case ':':
#endif
                return TRUE;
        }
    }
    return FALSE;
}


/*----------------------------------------------------------------------------
*
* Return TRUE if PATTERN has is a well formed regular expression according
* to the above syntax
*
* error_type is a return code based on the type of pattern error.  Zero is
* returned in error_type if the pattern is a valid one.  error_type return
* values are as follows:
*
*   PATTERN_VALID - pattern is well formed
*   PATTERN_RANGE - [..] construct has a no end range in a '-' pair (ie [a-])
*   PATTERN_CLOSE - [..] construct has no end bracket (ie [abc-g )
*   PATTERN_EMPTY - [..] construct is empty (ie [])
*
----------------------------------------------------------------------------*/

BOOLEAN is_valid_pattern (char *p, int *error_type)
{
    
    /* init error_type */
    *error_type = PATTERN_VALID;
    
    /* loop through pattern to EOS */
    while( *p ) {

        /* determine pattern type */
        switch( *p ) {

            /* the [..] construct must be well formed */
            case '[':
                p++;

                /* if the next character is ']' then bad pattern */
                if ( *p == ']' ) {
                    *error_type = PATTERN_EMPTY;
                    return FALSE;
                }
                
                /* if end of pattern here then bad pattern */
                if ( !*p ) {
                    *error_type = PATTERN_CLOSE;
                    return FALSE;
                }

                /* loop to end of [..] construct */
                while( *p != ']' ) {

                    /* check for literal escape */
                    if( *p == '\\' ) {
                        p++;

                        /* if end of pattern here then bad pattern */
                        if ( !*p++ ) {
                            *error_type = PATTERN_ESC;
                            return FALSE;
                        }
                    }
                    else
                        p++;

                    /* if end of pattern here then bad pattern */
                    if ( !*p ) {
                        *error_type = PATTERN_CLOSE;
                        return FALSE;
                    }

                    /* if this a range */
                    if( *p == '-' ) {

                        /* we must have an end of range */
                        if ( !*++p || *p == ']' ) {
                            *error_type = PATTERN_RANGE;
                            return FALSE;
                        }
                        else {

                            /* check for literal escape */
                            if( *p == '\\' )
                                p++;

                            /* if end of pattern here then bad pattern */
                            if ( !*p++ ) {
                                *error_type = PATTERN_ESC;
                                return FALSE;
                            }
                        }
                    }
                }
                break;

            /* all other characters are valid pattern elements */
            case '*':
            case '?':
            default:
                p++;                              /* "normal" character */
                break;
         }
     }

     return TRUE;
}


/*----------------------------------------------------------------------------
*
*  Match the pattern PATTERN against the string TEXT;
*
*  returns MATCH_VALID if pattern matches, or an errorcode as follows
*  otherwise:
*
*            MATCH_PATTERN  - bad pattern
*            MATCH_RANGE    - match failure on [..] construct
*            MATCH_ABORT    - premature end of text string
*            MATCH_END      - premature end of pattern string
*            MATCH_VALID    - valid match
*
*
*  A match means the entire string TEXT is used up in matching.
*
*  In the pattern string:
*       `*' matches any sequence of characters (zero or more)
*       `?' matches any character
*       [SET] matches any character in the specified set,
*       [!SET] or [^SET] matches any character not in the specified set.
*       \ is allowed within a set to escape a character like ']' or '-'
*
*  A set is composed of characters or ranges; a range looks like
*  character hyphen character (as in 0-9 or A-Z).  [0-9a-zA-Z_] is the
*  minimal set of characters allowed in the [..] pattern construct.
*  Other characters are allowed (ie. 8 bit characters) if your system
*  will support them.
*
*  To suppress the special syntactic significance of any of `[]*?!^-\',
*  within a [..] construct and match the character exactly, precede it
*  with a `\'.
*
----------------------------------------------------------------------------*/

int matche ( register char *p, register char *t )
{
    register char range_start, range_end;  /* start and end in range */

    BOOLEAN invert;             /* is this [..] or [!..] */
    BOOLEAN member_match;       /* have I matched the [..] construct? */

    for ( ; *p; p++, t++ ) {

        /* if this is the end of the text then this is the end of the match */
        if (!*t) {
            return ( *p == '*' && *++p == '\0' ) ? MATCH_VALID : MATCH_ABORT;
        }

        /* determine and react to pattern type */
        switch ( *p ) {

            /* single any character match */
            case '?':
                break;

            /* multiple any character match */
            case '*':
                return matche_after_star (p, t);

            /* [..] construct, single member/exclusion character match */
            case '[': {

                /* move to beginning of range */
                p++;

                /* check if this is a member match or exclusion match */
                invert = FALSE;
                if ( *p == '!' || *p == '^') {
                    invert = TRUE;
                    p++;
                }

                member_match = FALSE;

                for ( ;; ) {

                    /* if end of construct then loop is done */
                    if (*p == ']') {
                        break;
                    }

                    /* matching a '!', '^', '-', '\' or a ']' */
                    if ( *p == '\\' ) {
                        ++p;
                    }
                    range_start = range_end = *p;

                    /* check for range bar */
                    if (*++p == '-') {

                        /* special character range end */
                        if (*p == '\\') {
                            ++p;
                        }

                        /* get the range end */
                        range_end = *++p;

                        /* move just beyond this range */
                        p++;
                    }

                    range_start = bj_tolower( range_start );
                    range_end   = bj_tolower( range_end   );

                    /* make sure the range letters have the proper
                       relationship to one another before comparison */
                    if (range_start > range_end) {
                        char temp   = range_start;
                        range_start = range_end;
                        range_end   = temp;
                    }

                    /* if the text character is in range then match found. */
                    if (bj_tolower( *t ) >= range_start &&
                        bj_tolower( *t ) <= range_end) {
                        member_match = TRUE;
                        break;
                    }
                }

                /* if there was a match in an exclusion set then no match */
                /* if there was no match in a member set then no match */
                if ((invert && member_match) ||
                   !(invert || member_match))
                    return MATCH_RANGE;

                /* if this is not an exclusion then skip the rest of the [...]
                    construct that already matched. */
                if (member_match) {
                    while (*p != ']') {

                        /* skip exact match */
                        if (*p == '\\') {
                            p++;
                        }

                        /* move to next pattern char */
                        p++;
                    }
                }

                break;
            }

            /* must match this character exactly */
            default:
                if (bj_tolower( *p ) != bj_tolower( *t ))
                    return MATCH_LITERAL;
        }
    }

    /* if end of text not reached then the pattern fails */
    if ( *t )
        return MATCH_END;
    else
        return MATCH_VALID;
}


/*----------------------------------------------------------------------------
*
* recursively call matche() with final segment of PATTERN and of TEXT.
*
----------------------------------------------------------------------------*/

int matche_after_star (register char *p, register char *t)
{
    register int match = 0;
    register char nextp;

    /* pass over existing ? and * in pattern */
    while ( *p == '?' || *p == '*' ) {

        /* take one char for each ? */
        if ( *p == '?' ) {

            /* if end of text then no match */
            if ( !*t++ ) {
                return MATCH_ABORT;
            }
        }

        /* move to next char in pattern */
        p++;
    }

    /* if end of pattern we have matched regardless of text left */
    if ( !*p ) {
        return MATCH_VALID;
    }

    /* get the next character to match which must be a literal or '[' */
    nextp = bj_tolower( *p );

    /* Continue until we run out of text or definite result seen */
    do {

        /* a precondition for matching is that the next character
           in the pattern match the next character in the text or that
           the next pattern char is the beginning of a range.  Increment
           text pointer as we go here */
        if (nextp == bj_tolower( *t ) || nextp == '[' )
            match = matche(p, t);

        /* if the end of text is reached then no match */
        if ( !*t++ ) match = MATCH_ABORT;

    } while ( match != MATCH_VALID && 
              match != MATCH_ABORT);

    /* return result */
    return match;
}


/*----------------------------------------------------------------------------
*
* match() is a shell to matche() to return only BOOLEAN values.
*
* jmh 050731: allow it to match multiple patterns, separated by ';', and
*              exclusions, separated by '!'.  Use the set notation to treat
*              the character literally.
*             eg: "!*.exe;*.com" will match all files except .EXE & .COM.
*                 "!*.*" will match files with no extension.
*                 "*.txt;*.doc!read*" will match all .TXT & .DOC files,
*                 except those starting with READ.
*                 "[\!]*" will match all files starting with '!'.
*
----------------------------------------------------------------------------*/

BOOLEAN wildcard( char *p, char *t )
{
int  error_type;
char *alt;                      /* alternative */
char *exc;                      /* exclusion */
int  falt, fexc;                /* found ... */
BOOLEAN inverted = FALSE;       /* matching exclusion rather than pattern */
int  rc;

   if (!is_valid_pattern( p, &rc ))
      return( FALSE );

   for (exc = p; *exc && *exc != '!'; ++exc) {
      if (*exc == '[') {
         do {
            if (*++exc == '\\')
               exc += 2;
         } while (*exc != ']');
      }
   }
   fexc = *exc;
   *exc = '\0';
   if (exc == p && fexc)
      p = "*";
   rc = FALSE;
   do {
      for (alt = p; *alt && *alt != ';'
#if defined( __UNIX__)
                                        && *alt != ':'
#endif
                                                      ; ++alt) {
         if (*alt == '[') {
            do {
               if (*++alt == '\\')
                  alt += 2;
            } while (*alt != ']');
         }
      }
      falt = *alt;
      if (falt)                 /* prevent modifying the static string */
         *alt = '\0';
      error_type = matche( p, t );
      if (falt)
         *alt = falt;
      if (error_type == MATCH_VALID) {
         if (inverted) {
            rc = FALSE;
            break;
         }
         rc = TRUE;
         if (!fexc)
            break;
         inverted = TRUE;
         p = exc + 1;
      } else {
         if (!falt)
            break;
         p = alt + 1;
      }
   } while (*p);

   *exc = fexc;

   return( rc );
}


/*
 * The functions below enhance the wildcards to match directory names and
 * to recurse into subdirectories.  I've used djgpp's "..." method for
 * subdirectory traversal.  However, unlike djgpp, "..." (or a directory name)
 * by itself will find all files (it's actually a side-effect of my_findfirst).
 *
 * Eg: .        -> all files in the current directory
 *     t*\*     -> all files in each subdirectory starting with 't'
 *     ...      -> all files in the current directory and all subdirectories
 *     *\...    -> all files in all subdirectories, but none in the current
 *
 * The names are sorted within their directories, according to the current
 * directory list order (ie. by name or extension).
 */


static FileList *list, *last;
static char pathbuf[PATH_MAX+2];

static int  find_files2( char *, char * );


/*
 * Name:    merge_sort
 * Purpose: sort a list of files
 * Author:  Jason Hood
 * Date:    August 10, 2005
 * Passed:  list:  list of files to be sorted
 *          cnt:   number of files
 * Returns: pointer to new head of list
 * Notes:   use the current directory list sort order.
 */
static FileList *merge_sort( FileList *list, int cnt )
{
FileList *half;
static FileList *p, h;          /* none of these are     */
static int  i;                  /*  needed for recursion */

   if (cnt <= 1)
      return( list );

   if (cnt == 2) {
      half = list->next;
      if (dir_cmp( list->name, half->name ) > 0) {
         list->next = NULL;
         half->next = list;
         last = list;
         return( half );
      }
      last = half;
      return( list );
   }

   i = cnt >> 1;
   p = list;
   while (--i > 0)
      p = p->next;
   half = p->next;
   p->next = NULL;

   list = merge_sort( list, cnt >> 1 );
   half = merge_sort( half, (cnt >> 1) + (cnt & 1) );

   p = &h;
   for (;;) {
      if (dir_cmp( list->name, half->name ) <= 0) {
         p = p->next = list;
         list = list->next;
         if (list == NULL) {
            p->next = half;
            break;
         }
      } else {
         p = p->next = half;
         half = half->next;
         if (half == NULL) {
            p->next = list;
            break;
         }
      }
   }

   last = p;
   return( h.next );
}


/*
 * Name:    add_files
 * Purpose: find and add files to the list
 * Author:  Jason Hood
 * Date:    August 7, 2005
 * Passed:  name:  pointer to the name component of the path
 * Returns: OK if all added, ERROR if out of memory
 * Notes:   if the path is a subdirectory add all files in it.
 *          sort the entries based on the current directory list sort mode.
 */
static int  add_files( char *name )
{
FTYPE *ft;
FFIND ff;
FileList *f;
FileList *h;
int  len1, len2;
int  rc = OK;
int  cnt = 0;

   len1 = (int)(name - pathbuf);
   if (file_exists( pathbuf ) == SUBDIRECTORY  &&  *name) {
      len1 += strlen( name ) + 1;
      pathbuf[len1-1] = '/';
      pathbuf[len1] = '\0';
   }
   h = list;
   ft = my_findfirst( pathbuf, &ff, NO_DIRS );
   while (ft != NULL) {
      len2 = strlen( ft->fname );
      f = my_malloc( sizeof(*f) + len1 + len2, &rc );
      if (f != NULL) {
         f->next = NULL;
         memcpy( f->name, pathbuf, len1 );
         memcpy( f->name + len1, ft->fname, len2 + 1 );
         list = list->next = f;
         ++cnt;
      }
      ft = my_findnext( &ff );
   }

   if (cnt > 1) {
      h->next = merge_sort( h->next, cnt );
      do {
         list = last;
         last = last->next;
      } while (last != NULL);
   }

   return( rc );
}


/*
 * Name:    add_dirs
 * Purpose: create a list of directories
 * Author:  Jason Hood
 * Date:    August 10, 2005
 * Passed:  dirs:  how to match directories
 * Returns: pointer to list or NULL if none
 * Notes:   pathbuf contains the pattern.
 *          the list is the directory name only, not the path.
 */
static FileList *add_dirs( int dirs )
{
FTYPE *ft;
FFIND ff;
int  len, cnt;
FileList head, *list, *f;
char *p;

   head.next = NULL;
   list = &head;
   cnt = 0;

   ft = my_findfirst( pathbuf, &ff, dirs );
   /*
    * assume ".." will always be found first ("." is already ignored).
    */
   if (ft != NULL  &&  strcmp( ft->fname, "../" ) == 0)
      ft = my_findnext( &ff );

   while (ft != NULL) {
      len = strlen( ft->fname );
      if (ft->fname[len-1] == '/') {
         f = my_malloc( sizeof(*f) + len, &len );
         if (f != NULL) {
            f->next = NULL;
            memcpy( f->name, ft->fname, len + 1 );
            list = list->next = f;
            ++cnt;
         }
      }
      ft = my_findnext( &ff );
   }

   /*
    * determine if "." or ".." were given explicitly.
    */
   for (p = ff.pattern;; ++p) {
      if (*p == '.') {
         len = 1;
         if (*++p == '.') {
            len = 2;
            ++p;
         }
         if (*p == '\0' || *p == '!' || *p == ';'
#if defined( __UNIX__ )
                                     || *p == ':'
#endif
                                                 ) {
            f = my_malloc( sizeof(*f) + len + 1, &len );
            if (f != NULL) {
               f->next = NULL;
               memset( f->name, '.', len );
               f->name[len] = '/';
               f->name[len+1] = '\0';
               list = list->next = f;
               ++cnt;
            }
         }
      }
      while (*p && *p != '!' && *p != ';'
#if defined( __UNIX__ )
                             && *p != ':'
#endif
                                         ) {
         if (*p == '[') {
            do {
               if (*++p == '\\')
                  p += 2;
            } while (*p != ']');
         }
         ++p;
      }
      if (*p == '\0' || *p == '!')
         break;
   }

   return( merge_sort( head.next, cnt ) );
}


/*
 * Name:    find_dirs
 * Purpose: traverse subdirectories
 * Author:  Jason Hood
 * Date:    August 7, 2005
 * Passed:  rest:  remaining portion of the pattern
 *          name:  where to place directory name
 * Returns: OK if all added, ERROR if out of memory
 * Notes:   find files in the current directory first, then recurse into
 *           each subdirectory.
 */
static int  find_dirs( char *rest, char *name )
{
FileList *f;
int  len;
int  rc = OK;

   rc = find_files2( rest, name );

   *name = '\0';
   for (f = add_dirs( ALL_DIRS ); f != NULL && rc == OK; f = next_file( f )) {
      len = strlen( f->name );
      memcpy( name, f->name, len );
      rc = find_dirs( rest, name + len );
   }

   return( rc );
}


/*
 * Name:    find_files2
 * Purpose: find files across directories
 * Author:  Jason Hood
 * Date:    August 7, 2005
 * Passed:  rest:  pattern
 *          name:  pointer into buffer for path
 * Returns: OK if all files added, ERROR if out of memory or Ctrl-Break pressed
 * Notes:   a path of "..." can be used to traverse subdirectories.
 *          recurses when given a directory wildcard.
 */
static int  find_files2( char *rest, char *name )
{
FileList *f;
int  len;
char *start = name;
int  rc = OK;

   if (g_status.control_break)
      return( ERROR );

   while (*rest) {
      *name = *rest++;
      if (*name == '/') {
         if (name - start == 3  &&  memcmp( start, "...", 3 ) == 0)
            return( find_dirs( rest, start ) );

         *name = '\0';
         if (is_pattern( start )) {
            for (f = add_dirs( MATCH_DIRS ); f != NULL && rc == OK;
                 f = next_file( f )) {
               len = strlen( f->name );
               memcpy( start, f->name, len );
               rc = find_files2( rest, start + len );
            }
            return( rc );
         }
         *name = '/';
         start = name + 1;
      }
      ++name;
   }

   *name = '\0';
   rc = add_files( start );

   return( rc );
}


/*
 * Name:    find_files
 * Purpose: search for files across directories
 * Author:  Jason Hood
 * Date:    August 3, 2005
 * Passed:  pat:  pattern being searched
 * Returns: pointer to head of filename list or NULL if none found
 * Notes:   directories themselves can contain wildcards and "..." can be used
 *           to traverse subdirectories (eg: "...\*.c" finds all C files in
 *           the current directory and all subdirectories).
 */
FileList *find_files( char *pat )
{
FileList head;
char *p;
char *name;
char *pattern;
int  rc;

   if (*pat == '\0'  ||  !is_valid_pattern( pat, &rc ))
      return( NULL );

   rc = strlen( pat ) + 3;      /* two more for "\*" after "..." */
   pattern = my_malloc( rc, &rc );
   if (rc == ERROR)
      return( NULL );
   strcpy( pattern, pat );

#if !defined( __UNIX__ )
   /*
    * now we know the pattern is valid, replace '\' directory separators
    * with '/', leaving '\' escapes intact.
    */
   for (p = pattern; *p; ++p) {
      if (*p == '[') {
         do {
            if (*++p == '\\')
               p += 2;
         } while (*p != ']');
      } else if (*p == '\\')
         *p = '/';
   }
   /*
    * skip over the drive specifier
    */
   if (pattern[1] == ':') {
      pathbuf[0] = pattern[0];
      pathbuf[1] = ':';
      name = pathbuf + 2;
      p = pattern + 2;
   } else
#endif
   {
      name = pathbuf;
      p = pattern;
   }

   /*
    * explicitly treat "..." as "...\*" to simplify things.
    */
   rc = strlen( p ) - 3;
   if (rc >= 0  &&  strcmp( p + rc, "..." ) == 0  &&
       (rc == 0  ||  p[rc-1] == '/'))
      strcpy( p + rc + 3, "/*" );

#if defined( __UNIX__ )
   sort.order_array = sort_order.match;
#else
   sort.order_array = sort_order.ignore;
#endif

   head.next = NULL;
   list = &head;
   find_files2( p, name );

   my_free( pattern );

   return( head.next );
}


/*
 * Name:    next_file
 * Purpose: select the next file found by find_files
 * Author:  Jason Hood
 * Date:    August 3, 2005
 * Passed:  list:  current file
 * Returns: next file, or NULL if finished
 * Notes:   free the memory used by the current file.
 */
FileList *next_file( FileList *list )
{
FileList *next;

   next = list->next;
   my_free( list );
   return( next );
}


/*
 * Name:    is_glob
 * Purpose: determine if a name is a glob pattern
 * Author:  Jason Hood
 * Date:    August 9, 2005
 * Passed:  name:  name to test
 * Returns: TRUE if pattern, FALSE if not
 * Notes:   like is_pattern, but also tests for directories.
 */
int  is_glob( char *name )
{
int  rc = FALSE;
char *p;

   if (is_pattern( name )  ||  file_exists( name ) == SUBDIRECTORY)
      rc = TRUE;
   else if ((p = strstr( name, "..." )) != NULL) {
      if ((p == name  ||  p[-1] == '/'
#if !defined( __UNIX__ )
           ||  p[-1] == '\\'  ||  p[-1] == ':'
#endif
          )  &&  (p[3] == '\0'  ||  p[3] == '/'
#if !defined( __UNIX__ )
                  ||  p[3] == '\\'
#endif
                 ))
         rc = TRUE;
   }
   return( rc );
}
