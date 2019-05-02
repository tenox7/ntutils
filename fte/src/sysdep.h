/*    sysdep.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef SYSDEP_H
#define SYSDEP_H

#include "fte.h"

#if !defined(OS2) && \
    !defined(NT) && \
    !defined(DOSP32) && \
    !defined(LINUX) && \
    !defined(HPUX) && \
    !defined(AIX) && \
    !defined(IRIX) && \
    !defined(SCO) && \
    !defined(SUNOS) && \
    !defined(NCR)
#    error Target not supported.
#endif


#include <stdlib.h>

#if defined(AIX) || defined(SCO) || defined(NCR)
#include <strings.h>
#endif

#ifdef DBMALLOC
#include <malloc.h>
#endif

#if defined(UNIX) || defined(DJGPP)
#    define USE_DIRENT
#endif

#if defined(USE_DIRENT) // also needs fnmatch
#    include <dirent.h>
#endif

#if defined(UNIX)
#    include <unistd.h>
#    include <pwd.h>
#    if defined(__CYGWIN__)
#        include "fnmatch.h"
#    else
#        include <fnmatch.h>
#    endif
#    define strnicmp strncasecmp
#    define stricmp strcasecmp
#    define filecmp strcmp
     //#    define memicmp strncasecmp   // FIX, fails for nulls
     extern "C" int memicmp(const void *s1, const void *s2, size_t n);
#endif

#if defined(OS2)
#    include <malloc.h>
#    if !defined(__TOS_OS2__)
#        include <dos.h>
#    endif
#    include <io.h>
#    include <process.h>
#    if defined(BCPP) || defined(WATCOM) || defined(__TOS_OS2__)
#        include <direct.h>
#    endif
#    if defined(BCPP)
#        include <dir.h>
#    endif
#    define filecmp stricmp
#    if !defined(__EMX__)
#        define NO_NEW_CPP_FEATURES
#    endif
#endif

#if defined(DOS) || defined(DOSP32)
#    include <malloc.h>
#    include <dos.h>
#    include <io.h>
#    include <process.h>
#    define NO_NEW_CPP_FEATURES
#    if defined(BCPP)
#        include <dir.h>
#    endif
#    if defined(WATCOM)
#        include <direct.h>
#    endif
#    if defined(DJGPP)
#        include <dir.h>
#        include <unistd.h>
#        undef MAXPATH
         extern "C" int memicmp(const void *s1, const void *s2, size_t n);
#    endif
#    define filecmp stricmp
#endif

#if defined(NT)
#    include <malloc.h>
#    include <io.h>
#    include <process.h>
#    include <fcntl.h>
#    if defined(MSVC)
#        include <direct.h>
//#        define snprintf _snprintf
#        define ssize_t SSIZE_T
#    endif
#    if defined(WATCOM)
#        include <direct.h>
#    endif
#    if defined(BCPP)
#        include <dir.h>
#        define ssize_t SSIZE_T
#    endif
#    if defined(MINGW)
#        include <dir.h>
#        define HAVE_BOOL // older versions of MingW may not have it
#    endif
#    define filecmp stricmp
#    define popen _popen
#    define pclose _pclose
#endif

#ifndef MAXPATH
#    define MAXPATH 1024
#endif

#ifndef O_BINARY
#    define O_BINARY 0   /* defined on OS/2, no difference on unix */
#endif

#if defined(OS2) || defined(NT)
#    if defined(__EMX__) || defined(WATCOM) || defined(__TOS_OS2__)
#        define FAKE_BEGINTHREAD_NULL NULL,
#    else
#        define FAKE_BEGINTHREAD_NULL
#    endif
#endif

#if (!defined(__IBMC__) && !defined(__IBMCPP__)) || !defined(OS2)
#    define _LNK_CONV
#endif

#define PT_UNIXISH   0
#define PT_DOSISH    1

#ifndef S_ISDIR  // NT, DOS, DOSP32
#    ifdef S_IFDIR
#        define S_ISDIR(mode)  ((mode) & S_IFDIR)
#    else
#        define S_ISDIR(mode)  ((mode) & _S_IFDIR)
#    endif
#endif

#ifndef S_IWGRP
#define S_IWGRP 0
#define S_IWOTH 0
#endif

#if defined(OS2) || defined(NT) || defined(DOSP32) || defined(DOS)
#define PATHTYPE   PT_DOSISH
#else
#define PATHTYPE   PT_UNIXISH
#endif

#if defined __cplusplus && __cplusplus >= 199707L
#define HAVE_BOOL
#endif

#if defined __GNUC__ && (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7))
#define HAVE_BOOL
#endif

#if defined _G_HAVE_BOOL
#define HAVE_BOOL
#endif

#if defined __BORLANDC__ && __BORLANDC__ >= 0x0500
#define HAVE_BOOL
#endif

#if defined __BORLANDC__ && defined __OS2__
#define popen _popen
#define pclose _pclose
#define ftruncate _ftruncate
#endif

#undef HAVE_STRLCPY
#undef HAVE_STRLCAT

#ifndef HAVE_BOOL
#define bool  int
#define true  1
#define false 0
#endif

#endif // SYSDEP_H
