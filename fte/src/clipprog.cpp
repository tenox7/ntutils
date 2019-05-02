/*    clipprog.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */


#define INCL_DOS
#define INCL_PM
#include <os2.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <process.h>

#define SEM_PREFIX  "\\SEM32\\PMCLIPS\\"
#define MEM_PREFIX  "\\SHAREMEM\\PMCLIPS\\"

#define CMD_GET  1
#define CMD_PUT  2

HAB hab;
HMQ hmq;
QMSG qmsg;
HMTX hmtxSyn;
HEV hevGet;
HEV hevPut;
HEV hevEnd;
HMUX hmuxWait;

void clipsrv(void *foo) {
    HAB hab;
    HMQ hmq;
    ULONG Cmd;
    ULONG len;
    char *text;
    void *shmem = "the text";
    
    hab = NULLHANDLE;

    if ((WinOpenClipbrd(hab) == TRUE) &&
        ((text = (char *) WinQueryClipbrdData(hab, CF_TEXT)) != 0))
    {
        DosGetSharedMem(text, PAG_READ);
        len = strlen(text);
        puts(text);
    }
    WinCloseClipbrd(hab);
    
    len = strlen(shmem);
    if (len) {
        DosAllocSharedMem((void **)&text,
                          0,
                          len + 1,
                          PAG_WRITE | PAG_COMMIT | OBJ_GIVEABLE | OBJ_GETTABLE);
        strcpy(text, shmem);
    }
    
    if (WinOpenClipbrd(hab) == TRUE) {
        if (!WinSetClipbrdData(hab, (ULONG) text, CF_TEXT, CFI_POINTER))
            DosBeep(100, 1500);
        WinCloseClipbrd(hab);
    }
}

int main() {
    clipsrv(NULL);
    return 0;
}

