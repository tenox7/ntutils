
/*    s_util.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "s_util.h"

#include "o_buflist.h"
#include "s_direct.h"
#include "s_files.h"
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUF_SZ (sizeof(FileBuffer))

extern RxNode *CompletionFilter;

#ifdef CONFIG_BACKUP
// should use DosCopy under OS/2...
static int copyfile(char const* f1, char const* f2) { // from F1 to F2
    void *buffer;
    int fd1, fd2;
    int rd;

    buffer = FileBuffer;

    if ((fd1 = open(f1, O_RDONLY | O_BINARY)) == -1)
        return -1;
    if ((fd2 = open(f2, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, 0666)) == -1) {
        close(fd1);
        return -1;
    }
    while ((rd = read(fd1, buffer, BUF_SZ)) > 0) {
        if (write(fd2, buffer, rd) != rd) {
            close(fd1);
            close(fd2);
            unlink(f2);
            return -1;
        }
    }
    close(fd2);
    close(fd1);
    return 0;
}

char *MakeBackup(const char *FileName, char *NewName) {
//    static char NewName[260];
    size_t l = strlen(FileName);
    if (!l)
        return NULL;

    /* try 1 */
    strcpy(NewName, FileName);
    strcat(NewName, "~");
    if (!IsSameFile(FileName,NewName)) {
        if (access(NewName, 0) == 0)                 // Backup already exists?
            unlink(NewName);                         // Then delete the file..
        if (access(FileName, 0) != 0)                // Original found?
            return NewName;
        if (copyfile(FileName, NewName) == 0)
            return NewName;
#if 0
        if (errno == 2)
            return NewName; /* file not found */
#endif
    }
    
    /* try 2: 8.3 */
    strcpy(NewName, FileName);
    NewName[l-1] = '~';
    if (!IsSameFile(FileName,NewName)) {
        if (access(NewName, 0) == 0)                   // Backup exists?
            unlink(NewName);                           // Then delete;
        if (access(FileName, 0) != 0)                  // Original exists?
            return NewName;                            // If not-> return base..
        if (copyfile(FileName, NewName) == 0)
            return NewName;
#if 0
        if (errno == 2)
            return NewName;
#endif
    }

    return NULL;
}
#endif

int GetCharFromEvent(TEvent &E, char *Ch) {
    *Ch = 0;
    if (E.Key.Code & kfModifier)
        return 0;
    if (kbCode(E.Key.Code) == kbEsc) { *Ch = 27; return 1; }
    if (kbCode(E.Key.Code) == kbEnter) { *Ch = 13; return 1; }
    if (kbCode(E.Key.Code) == (kbEnter | kfCtrl)) { *Ch = 10; return 1; }
    if (kbCode(E.Key.Code) == kbBackSp) { *Ch = 8; return 1; }
    if (kbCode(E.Key.Code) == (kbBackSp | kfCtrl)) { *Ch = 127; return 1; }
    if (kbCode(E.Key.Code) == kbTab) { *Ch = 9; return 1; }
    if (kbCode(E.Key.Code) == kbDel) { *Ch = 127; return 1; }
    if (keyType(E.Key.Code) == kfCtrl) {
        *Ch = (char) (E.Key.Code & 0x1F);
        return 1;
    }
    if (isAscii(E.Key.Code)) {
        *Ch = (char)E.Key.Code;
        return 1;
    }
    return 0;
}

int CompletePath(const char *Base, char *Match, int Count) {
    char Name[MAXPATH];
    const char *dirp;
    char *namep;
    size_t len;
    int count = 0;
    char cname[MAXPATH];
    int hascname = 0;
    RxMatchRes RM;
    FileFind *ff;
    FileInfo *fi;
    int rc;

    if (strcmp(Base, "") == 0) {
        if (ExpandPath(".", Name, sizeof(Name)) != 0) return -1;
    } else {
        if (ExpandPath(Base, Name, sizeof(Name)) != 0) return -1;
    }
//    SlashDir(Name);
    dirp = Name;
    namep = SepRChr(Name);
    if (namep == Name) {
        dirp = SSLASH;
        namep = Name + 1;
    } else if (namep == NULL) {
        namep = Name;
        dirp = SDOT;
    } else {
        *namep = 0;
        namep++;
    }
    
    len = strlen(namep);
    strcpy(Match, dirp);
    SlashDir(Match);
    cname[0] = 0;

    ff = new FileFind(dirp, "*",
                      ffDIRECTORY | ffHIDDEN
#if defined(USE_DIRENT)
                      | ffFAST // for SPEED
#endif
                     );
    if (ff == 0)
        return 0;
    rc = ff->FindFirst(&fi);
    while (rc == 0) {
	const char *dname = fi->Name();

        // filter out unwanted files
        if ((strcmp(dname, ".") != 0) &&
            (strcmp(dname, "..") != 0) &&
            (!CompletionFilter || RxExec(CompletionFilter, dname, strlen(dname), dname, &RM) != 1))
        {
            if ((
#if defined(UNIX)
                strncmp
#else // os2, nt, ...
                strnicmp
#endif
                (namep, dname, len) == 0)
                && (dname[0] != '.' || namep[0] == '.'))
            {
                count++;
                if (Count == count) {
                    Slash(Match, 1);
                    strcat(Match, dname);
                    if (
#if defined(USE_DIRENT) // for SPEED
                        IsDirectory(Match)
#else
                        fi->Type() == fiDIRECTORY
#endif
                       )
                        Slash(Match, 1);
                } else if (Count == -1) {
                    
                    if (!hascname) {
                        strcpy(cname, dname);
                        hascname = 1;
                    } else {
                        int o = 0;
#ifdef UNIX
                        while (cname[o] && dname[o] && (cname[o] == dname[o])) o++;
#endif
#if defined(OS2) || defined(NT) || defined(DOS) || defined(DOSP32)
                        while (cname[o] && dname[o] && (toupper(cname[o]) == toupper(dname[o]))) o++;
#endif
                        cname[o] = 0;
                    }
                }
            }
        }
        delete fi;
        rc = ff->FindNext(&fi);
    }
    delete ff;
    if (Count == -1) {
        Slash(Match, 1);
        strcat(Match, cname);
        if (count == 1) SlashDir(Match);
    }
    return count;
}
