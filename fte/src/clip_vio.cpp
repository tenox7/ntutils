/*    clip_vio.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

// OS/2 does not allow VIO programs to use the clipboard :-(

#include "fte.h"
#include "clip.h"

#define INCL_DOS
#include <os2.h>
#include <string.h>
#include <stdlib.h>

#define SEM_PREFIX  "\\SEM32\\PMCLIPS\\"
#define MEM_PREFIX  "\\SHAREMEM\\PMCLIPS\\"

#define CMD_GET  1
#define CMD_PUT  2

static HMTX hmtxSyn;
static HEV hevGet;
static HEV hevPut;
static HEV hevEnd;

int GetClipText(ClipData *cd) {
    int rc;
    ULONG PostCount;
    char *mem;
    
    rc = DosOpenMutexSem(SEM_PREFIX "CLIPSYN", &hmtxSyn);
    if (rc != 0) return 0;
    rc = DosOpenEventSem(SEM_PREFIX "CLIPGET", &hevGet);
    if (rc != 0) return 0;
/*    rc = DosOpenEventSem(SEM_PREFIX "CLIPPUT", &hevPut);*/
/*    if (rc != 0) return -1;*/
    rc = DosOpenEventSem(SEM_PREFIX "CLIPEND", &hevEnd);
    if (rc != 0) return 0;
    
    DosRequestMutexSem(hmtxSyn, SEM_INDEFINITE_WAIT);
    DosResetEventSem(hevEnd, &PostCount);
    DosPostEventSem(hevGet);
    DosWaitEventSem(hevEnd, SEM_INDEFINITE_WAIT);
    if (0 == DosGetNamedSharedMem((void **)&mem, MEM_PREFIX "CLIPDATA", PAG_READ | PAG_WRITE)) {
        cd->fLen = *(ULONG*)mem;
        cd->fChar = strdup(mem + 4);
        DosFreeMem(mem);
    } else {
        cd->fLen = 0;
        cd->fChar = 0;
    }
    DosPostEventSem(hevGet);
    DosReleaseMutexSem(hmtxSyn);
/*    DosCloseEventSem(hevPut);*/
    DosCloseEventSem(hevGet);
    DosCloseEventSem(hevEnd);
    DosCloseMutexSem(hmtxSyn);
    return 1;
}

int PutClipText(ClipData *cd) {
    int rc;
    ULONG PostCount;
    char *mem;
    
    rc = DosOpenMutexSem(SEM_PREFIX "CLIPSYN", &hmtxSyn);
    if (rc != 0) return 0;
/*    rc = DosOpenEventSem(SEM_PREFIX "CLIPGET", &hevGet);*/
/*    if (rc != 0) return -1;*/
    rc = DosOpenEventSem(SEM_PREFIX "CLIPPUT", &hevPut);
    if (rc != 0) return 0;
    rc = DosOpenEventSem(SEM_PREFIX "CLIPEND", &hevEnd);
    if (rc != 0) return 0;
    
    DosRequestMutexSem(hmtxSyn, SEM_INDEFINITE_WAIT);
    DosResetEventSem(hevEnd, &PostCount);
    if (0 == DosAllocSharedMem((void **)&mem,
                               MEM_PREFIX "CLIPDATA",
                               cd->fLen + 5,
                               PAG_COMMIT | PAG_READ | PAG_WRITE))
    {
        ULONG L = cd->fLen;
        memcpy((void *)mem, (void *)&L, 4);
        strcpy(mem + 4, cd->fChar);
    }
    DosPostEventSem(hevPut);
    DosWaitEventSem(hevEnd, SEM_INDEFINITE_WAIT);
    DosPostEventSem(hevPut);
    DosReleaseMutexSem(hmtxSyn);
    DosCloseEventSem(hevPut);
/*    DosCloseEventSem(hevGet); */
    DosCloseEventSem(hevEnd);
    DosCloseMutexSem(hmtxSyn);
    if (mem)
        DosFreeMem(mem);
    return 1;
    
}
