/*    s_direct.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef S_DIRECT_H
#define S_DIRECT_H

#include "sysdep.h"
#include <sys/types.h>

// error handling needs some work

#define fiUNKNOWN   0
#define fiFILE      1
#define fiDIRECTORY 2

class FileInfo {
    StlString name;   // minimum set of file information
    off_t size;
    time_t mtime;
    int type;
    int mode;
    
public:
    FileInfo(const char *Name, int type, off_t Size, time_t MTime, int Mode=-1);
    ~FileInfo();

    const char *Name() const { return name.c_str(); }
    off_t Size() const { return size; }
    int Type() const { return type; }
    int Mode() const { return mode; }
    time_t MTime() const { return mtime; }
};

#define ffFAST       1  // optimization for UNIX (return name only, NO TYPE CHECK), ignored on OS/2 and NT
#define ffFULLPATH   2  // return full path to files
#define ffDIRECTORY  4  // return directories beside files (see ffFAST)
#define ffHIDDEN     8  // return hidden files (dot-files for UNIX)
#define ffLINK      16  // diagnose location of symbolic link, not link itself

class FileFind {
    char *Directory;
    char *Pattern;
    int Flags;

#if defined(USE_DIRENT)
    DIR *dir;
#elif defined(OS2) && !defined(USE_DIRENT)
    unsigned long dir; // should be HDIR, but we don't #include huge os2.h globally
#elif defined(NT) && !defined(USE_DIRENT)
    unsigned long long dir; // should be HANDLE
#endif
    
public:
    FileFind(const char *aDirectory, const char *aPattern, int aFlags);
    ~FileFind();

    int FindFirst(FileInfo **fi);
    int FindNext(FileInfo **fi);
};

#endif // S_DIRECT_H
