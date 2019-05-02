/*    s_files.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "s_files.h"

#include "sysdep.h"
#include "s_util.h"
#include "s_string.h"

#include <stdlib.h> // getenv
#include <ctype.h>
#include <sys/stat.h>

#if defined(OS2)
#define INCL_DOS
#include <os2.h>
#endif

#if defined(NT)
#   define  WIN32_LEAN_AND_MEAN 1
#   include <windows.h>
#endif

#if defined(DOSP32)
#   include "port.h"
#endif


#if defined(DJGPP)
static inline int is_end(int c)
{
    return c == 0 || c == '.' || c == '/';
}

static inline int is_filename_char(int c)
{
    return (strchr("+<>",c) == NULL);
}


static void my_fixpath(const char *in, char *out) {
    // this does most of the cleanup
    _fixpath(in,out);
    if (_USE_LFN)
        return;

    // handle 8+3 restrictions
    char tmp[MAXPATH];
    char *t = tmp;
    char *o = out;
    if (o[0] && o[1] == ':') {
        *t++ = *o++;
        *t++ = *o++;
    }
    while (*o) {
        int i;
        // copy over slash
        if (*o == '/')
            *t++ = *o++;
        // copy over filename (up to 8 chars)
        for (i = 0; i < 8 && !is_end(*o); o++)
            if (is_filename_char(*o))
                *t++ = *o, i++;
                // copy over extension (up to 3 chars)
                if (*o == '.') {
                    // don't copy a trailing '.' unless following a ':'
                    if (o[1] == 0 && o > out && o[-1] != ':')
                        break;
                    *t++ = (i > 0 ? '.' : '_');
                    o++;
                    for (i = 0; i < 3 && !is_end(*o); o++)
                        if (is_filename_char(*o))
                            *t++ = *o, i++;
                }
                // find next slash
                while (*o && *o != '/')
                    o++;
    }
    *t++ = 0;
#if 0
    if (strcmp(out,tmp) != 0)
        fprintf(stderr,"fix: '%s'->'%s'\n",out,tmp);
#endif
    strcpy(out,tmp);
}
#endif


char *Slash(char *Path, int Add) {
    size_t len = strlen(Path);

    if (Add) {
        if ((len == 0) || !ISSLASH(Path[len - 1])) {
            Path[len] = SLASH;
            Path[len+1] = 0;
        }
    } else {
        if ((len > 1)
#if PATHTYPE == PT_DOSISH
            && ((len > 3) || (Path[1] != ':'))
#endif
           ) {
            if (ISSLASH(Path[len - 1])) {
                Path[len - 1] = 0;
            }
        }
    }
    return Path;
}

char *SlashDir(char *Path) {
    size_t len = strlen(Path);
    if (len > 1) {
#if PATHTYPE == PT_DOSISH
        if ((len == 2) && Path[1] == ':') {
#ifdef DJGPP
            char tmp[MAXPATH];
            strcpy(tmp,Path);
            my_fixpath(tmp,Path);
            strcat(Path,SSLASH);
#else
            Path[2] = SLASH;
            Path[3] = 0;
#endif
        }
        else
#endif
            if (!ISSLASH(Path[len - 1])) {
                struct stat statbuf;
                if (stat(Path, &statbuf) == 0) {
                    if (S_ISDIR(statbuf.st_mode)) {
                        Path[len] = SLASH;
                        Path[len+1] = 0;
                    }
                }
            }
    }
    return Path;
}

int IsDirectory(const char *Path) {
    size_t len = strlen(Path);
    if (len > 0) {
#if PATHTYPE == PT_DOSISH
        if ((len == 2) && Path[1] == ':')
            return 1;
        else
#endif
            if (!ISSLASH(Path[len - 1])) {
                int res;
                struct stat statbuf;
#if PATHTYPE == PT_UNIXISH
                if (Path[0] == '~') {
                    char Expanded[MAXPATH];
                    if (ExpandPath(Path, Expanded, sizeof(Expanded)) == -1) return 0;
                    res = stat(Expanded, &statbuf);
                } else
#endif
                    res = stat(Path, &statbuf);
                if (res == 0) {
                    if (S_ISDIR(statbuf.st_mode)) {
                        return 1;
                    }
                }
                return 0;
            } else
                return 1;
    }
    return 0;
}

#if PATHTYPE == PT_DOSISH
static int GetDiskCurDir(int drive, char *dir) {
    if (drive < 1 || drive > 26)
        return -1;

#if defined(__EMX__)
    return (_getcwd1(dir, (char)(drive + 'A' - 1)) == 0) ? 0 : -1;
#elif defined(OS2)
    {
        ULONG len = MAXPATH - 4; // 'c:\0'

        return (DosQueryCurrentDir(drive, dir, &len) == 0) ? 0 : -1;
    }
#elif defined(NT)
    {
        char orig[MAXPATH], newdrive[MAXPATH];

        // set to new drive, get directory and restore original directory
        if (!GetCurrentDirectory(sizeof(orig), orig))
            return -1;
        newdrive[0] = drive + 'A' - 1;
        newdrive[1] = ':';
        newdrive[2] = 0;
        if (!SetCurrentDirectory(newdrive))
            return -1;
        if (!GetCurrentDirectory(sizeof(newdrive), newdrive))
            return -1;
        strcpy(dir, newdrive + 3);

        SetCurrentDirectory(orig); // ? check
        return 0;
    }
#elif defined(DOS) || defined(DOSP32)
    return (plGetcurdir(drive, dir) != 0);
#endif
}

static int SetDrive(int drive) { // 1 = A, 2 = B, 3 = C, ...
    if (drive < 1 || drive > 26)
        return -1;

    // mess
    // _chdrive seems to be most portable between dosish systems,
    // but seem to take different arguments ???

#if defined(__EMX__)
    return _chdrive(drive + 'A' - 1);
#elif defined(OS2)
    return (DosSetDefaultDisk(drive) == 0) ? 0 : -1;
#elif defined(NT)
    char buf[3];

    buf[0] = (char)(drive + 'A' - 1);
    buf[1] = ':';
    buf[2] = '\0';
    return SetCurrentDirectory(buf) ? 0 : -1;
#elif defined(DOS) || defined(DOSP32)
    unsigned int ndr;

    _dos_setdrive(drive, &ndr);
    return 0; //(ndr == drive) ? 0 : -1; // ?
#endif
}
#endif

#if PATHTYPE == PT_UNIXISH
int RemoveDots(const char *p, char* Dest) {
    char *d = Dest;

    while (*p) {
        // if ((strncmp(p, SSLASH SSLASH, 2) == 0)) {
        if (ISSLASH(p[0]) && ISSLASH(p[1])) {
            p++;
            // } else if ((strncmp(p, SSLASH ".." SSLASH, 4) == 0) || (strcmp(p, SSLASH "..") == 0)) {
        } else if (ISSLASH(p[0]) && p[1] == '.' && p[2] == '.' &&
                   (ISSLASH(p[3]) || p[3] == 0)) {
            p += 3;
            while ((d > Dest) && !ISSEP(*d)) d--;
            *d = 0;
            continue;
            // } else if ((strncmp(p, SSLASH "." SSLASH, 3) == 0) || (strcmp(p, SSLASH ".") == 0)) {
        } else if (ISSLASH(p[0]) && p[1] == '.' &&
                   (ISSLASH(p[2]) || p[2] == 0)) {
            p += 2;
            continue;
        }
        *d++ = *p++;
        *d = 0;
    }
    *d = 0;
    return 0;
}
#endif

int ExpandPath(const char *Path, char *Expand, size_t ExpandSize) {
    char Name[MAXPATH];

    if (Path[0] == 0) {
        Expand[0] = 0;
        return 0;
    }
#if PATHTYPE == PT_DOSISH
    int slashed = 0;

    strlcpy(Name, Path, sizeof(Name));
    if (Name[0] != 0)
        slashed = ISSLASH(Name[strlen(Name)-1]);
    Slash(Name, 0);
#if defined(DJGPP)
    my_fixpath(Name, Expand);
#else
    //puts(Name);
    if (Name[0] && Name[1] == ':' && Name[2] == 0) { // '?:'
        int drive = Name[0];

        strlcpy(Expand, Name, ExpandSize);
        Expand[2] = '\\';
        Expand[3] = 0;

        drive = (int)(toupper(Name[0]) - 'A' + 1);

        if (GetDiskCurDir(drive, Expand + 3) == -1)
            return -1;
    } else {
#if defined(__EMX__)
        if (_fullpath(Expand, Name, MAXPATH) == -1) return -1;
#else
        if (_fullpath(Expand, Name, MAXPATH) == NULL) return -1;
#endif
    }
#endif
#if defined(__EMX__)
    {
        char *p = Expand;

        if (p && *p) do {
            if (ISSLASH(*p))
                *p = SLASH;
        } while (*p++);
    }
#endif
    if (slashed)
        SlashDir(Expand);
    //puts(Expand);
    return 0;
#endif
#if PATHTYPE == PT_UNIXISH
    char Name2[MAXPATH];
    char *path, *p;

    strlcpy(Name, Path, sizeof(Name));
    switch (Name[0]) {
    case SLASH:
        break;
    case '~':
        if (Name[1] == SLASH || Name[1] == 0) {
            path = Name + 1;
            strlcpy(Name2, getenv("HOME"), sizeof(Name2));
        } else {
            struct passwd *pwd;

            p = Name;
            p++;
            while (*p && (*p != SLASH)) p++;
            if (*p == SLASH) {
                path = p + 1;
                *p = 0;
            } else {
                path = p;
            }
            pwd = getpwnam(Name + 1);
            if (pwd == NULL)
                return -1;
            strlcpy(Name2, pwd->pw_dir, sizeof(Name2));
        }
        if (path[0] != SLASH)
            Slash(Name2, 1);
        strlcat(Name2, path, sizeof(Name2));
        strlcpy(Name, Name2, sizeof(Name));
        break;
    default:
        if (getcwd(Name, MAXPATH) == NULL) return -1;
        Slash(Name, 1);
        strlcat(Name, Path, sizeof(Name));
        break;
    }
    return RemoveDots(Name, Expand);
#endif
}

int IsSameFile(const char *Path1, const char *Path2) {
    char p1[MAXPATH], p2[MAXPATH];

    if (ExpandPath(Path1, p1, sizeof(p1)) == -1) return -1;
    if (ExpandPath(Path2, p2, sizeof(p2)) == -1) return -1;
    if (filecmp(p1, p2) == 0) return 1;
    return 0;
}

int JustDirectory(const char *Path, char *Dir, size_t DirSize) {
    char *p;

    if (ExpandPath(Path, Dir, DirSize) == -1)
        strlcpy(Dir, Path, DirSize);
    p = SepRChr(Dir);
    if (p) { p[1] = 0; }
    else Dir[0] = 0;
    return 0;
}

int JustLastDirectory(const char *Path, char *Dir, size_t DirSize) {
    size_t lastSlash = strlen(Path);
    while (lastSlash > 0 && !ISSEP(Path[lastSlash])) lastSlash--;

    size_t secondLastSlash = lastSlash;
    while (secondLastSlash > 0 && !ISSEP(Path[secondLastSlash - 1])) secondLastSlash--;

    if ((lastSlash - secondLastSlash) >= (DirSize-1))
    {
        // how unfortunate, we didn't have enough space for directory name
        // just copy as many characters as possible while leaving room for NUL.
        lastSlash = secondLastSlash + DirSize - 2;
    }

    strncpy(Dir, Path + secondLastSlash, lastSlash - secondLastSlash);
    Dir[lastSlash - secondLastSlash] = 0;

    return 0;
}

int JustFileName(const char *Path, char *Name, size_t NameSize) {
    size_t len = strlen(Path);

    while (len > 0 && !ISSEP(Path[len - 1])) len--;
    strlcpy(Name, Path + len, NameSize);
    return 0;
}

int JustRoot(const char *Path, char *Root, size_t RootSize) {
#if PATHTYPE == PT_UNIXISH
    strlcpy(Root, SSLASH, RootSize);
#else
    if (RootSize >= 4)
        strlcpy(Root, Path, 3);

#endif
    return 0;
}

int FileExists(const char *Path) {
    return (access(Path, 0) == 0) ? 1 : 0;
}

int IsFullPath(const char *Path) {
    if (ISSLASH(Path[0])
#if PATHTYPE == PT_DOSISH
        || (Path[0] != 0 && Path[1] == ':')
#endif
       )
        return 1;
    else
        return 0;
}

const char *ShortFName(const char *Path, size_t len) {
    static char P[MAXPATH];
    char p1[MAXPATH];
    size_t l1;

    if (len < 10) len = 10;
    if (len > (int)(sizeof(P) - 1)) len = sizeof(P) - 1;

    if (ExpandPath(Path, p1, sizeof(p1)) == -1) return Path;
    l1 = strlen(p1);
    if (l1 < len) {
        strlcpy(P, p1, sizeof(P));
    } else {
        strncpy(P, p1, 3);
        strcpy(P + 3, "...");
        strncpy(P + 6, p1 + l1 - len, len - 6);
    }
    return P;
}


int ChangeDir(const char *Dir) {
    char *dir = (char*)alloca(strlen(Dir) + 10);

    strcpy(dir, Dir);
    Slash(dir, 0); // remove last \ except in case of ?:
#if PATHTYPE == PT_DOSISH
    if (dir[0] && dir[1] == ':')
        if (SetDrive(toupper(dir[0]) - 'A' + 1) == -1)
            return -1;
#endif
    return (chdir(dir) == 0) ? 0 : -1;
}

int JoinDirFile(char *Dest, const char *Dir, const char *Name) {
    strcpy(Dest, Dir);
    Slash(Dest, 1);
    strcat(Dest, Name);
    return 0;
}

char *SepRChr(const char *Dir)
{
    if (Dir && Dir[0]) {
        for (const char *p = Dir + strlen(Dir); p > Dir; p--)
            if (ISSEP(p[-1]))
                return const_cast<char*>(p - 1);
    }
    return NULL;
}

// Returns relative path of Path with respect to Dir
// Ex: Dir = /home/martin/src/ Path = /home/martin/src/Fte/s_files.cpp -> Fte/s_files.cpp
int RelativePathName(const char *Dir, const char *Path, char *RelPath, int RelPathSize) {
    char d[MAXPATH], p[MAXPATH], c;
    size_t dl, pl;

    if (ExpandPath(Dir, d, sizeof(d)) == -1) return -1;
    if (ExpandPath(Path, p, sizeof(p)) == -1) return -1;
    dl = strlen (d); pl = strlen (p);

    if (dl <= pl) {
        c = p[dl]; p[dl] = 0;
        if (filecmp(d, p) == 0) {
            p[dl] = c;
            strlcpy(RelPath, p + dl, RelPathSize);
            return 0;
        }
    }
    return -1;
}
