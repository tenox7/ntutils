/*    fte.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef FTE_H
#define FTE_H

#ifdef LINUX
/*
 * for support of large file sizes on 32bit system
 * here is the solution for linux and glibc
 */
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE 1
#endif

#include "feature.h"
#include "sysdep.h"

#include <inttypes.h>
#include <sys/types.h>
#include <assert.h>
#include <string.h>

#if defined(_DEBUG) && defined(MSVC) && defined(MSVCDEBUG)
#include <crtdbg.h>

#define new new( _CLIENT_BLOCK, __FILE__, __LINE__)

#endif //_DEBUG && MSVC && MSVCDEBUG

#define FTE_ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))

#include "stl_fte.h"

#endif // FTE_H
