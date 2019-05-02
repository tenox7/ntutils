/*    c_desktop.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef C_DESKTOP_H
#define C_DESKTOP_H

#include "fte.h"

#ifdef CONFIG_DESKTOP

#ifdef UNIX
#    define        DESKTOP_NAME       ".fte-desktop"
#else
#    define        DESKTOP_NAME       "fte.dsk"
#endif

extern char DesktopFileName[256];

int SaveDesktop(const char *FileName);
int LoadDesktop(const char *FileName);

#endif

#endif // C_DESKTOP_H
