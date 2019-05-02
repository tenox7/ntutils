/*    s_util.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef S_UTIL_H
#define S_UTIL_H

#include "fte.h"

#include <sys/types.h>

#define USE_CtrlEnter    1

enum {
    S_BUSY,
    S_INFO,
    S_BOLD,
    S_ERROR
};

class EView;
class EBuffer;
class EModel;

char* MakeBackup(const char *FileName, char *NewName);

int GetPMClip(int clipboard);
int PutPMClip(int clipboard);

int FileLoad(int createFlags, const char *FileName, const  char *Mode, EView *View);
int MultiFileLoad(int createFlags, const char *FileName, const char *Mode, EView *View);
int SetDefaultDirectory(EModel *M);
int GetDefaultDirectory(EModel *M, char *Path, size_t MaxLen);

#endif // S_UTIL_H
