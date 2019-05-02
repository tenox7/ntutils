/*    clipserv.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#define INCL_DOS
#define INCL_PM
#include "sysdep.h"

#include <os2.h>
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

void _LNK_CONV clipsrv(void *foo) {
    HAB hab;
    HMQ hmq;
    
    hab = WinInitialize(0);
    hmq = WinCreateMsgQueue(hab, 0);
    
    while (1) {
        ULONG ulPostCount;
        ULONG Cmd;
        ULONG len;
        char *text;
        void *shmem;
        
        WinWaitMuxWaitSem(hmuxWait, SEM_INDEFINITE_WAIT, &Cmd);
        switch (Cmd) {
        case CMD_GET:
            DosResetEventSem(hevGet, &ulPostCount);
            
            shmem = 0;
            if ((WinOpenClipbrd(hab) == TRUE) &&
                ((text = (char *) WinQueryClipbrdData(hab, CF_TEXT)) != 0))
            {
                len = strlen(text);
                puts(text);
                if (0 == DosAllocSharedMem(&shmem,
                                           MEM_PREFIX "CLIPDATA",
                                           len + 5,
                                           PAG_COMMIT | PAG_WRITE | PAG_READ))
                {
                    memcpy(shmem, (void *)&len, 4);
                    memcpy((void *)(((char *)shmem) + sizeof(ULONG)), text, len + 1);
                } else {
                    DosBeep(200, 500);
                }
            } else {
                /*DosBeep(100, 1500);*/
                len = 0;
                if (0 == DosAllocSharedMem(&shmem,
                                           MEM_PREFIX "CLIPDATA",
                                           4,
                                           PAG_COMMIT | PAG_WRITE | PAG_READ))
                {
                    memcpy(shmem, (void *)&len, 4);
                }
            }
            WinCloseClipbrd(hab);
            DosPostEventSem(hevEnd);
            DosWaitEventSem(hevGet, SEM_INDEFINITE_WAIT);
            DosResetEventSem(hevGet, &ulPostCount);
            if (shmem) DosFreeMem(shmem);
            break;
            
        case CMD_PUT:
            DosResetEventSem(hevPut, &ulPostCount);
            
            if (0 == DosGetNamedSharedMem(&shmem, 
                                          MEM_PREFIX "CLIPDATA",
                                          PAG_READ | PAG_WRITE))
            {
                if (WinOpenClipbrd(hab) == TRUE) {
                    WinEmptyClipbrd(hab);
                    len = strlen((char *)shmem + 4);
                    if (len) {
                        DosAllocSharedMem((void **)&text,
                                          0,
                                          len + 1,
                                          PAG_WRITE | PAG_COMMIT | OBJ_GIVEABLE);
                        strcpy(text, ((char *)shmem) + 4);
                        if (!WinSetClipbrdData(hab, (ULONG) text, CF_TEXT, CFI_POINTER))
                            DosBeep(100, 1500);
                        
                    }
                    WinCloseClipbrd(hab);
                } else {
                    DosBeep(1500, 3500);
                }
            } else {
                DosBeep(500, 7500);
            }
            DosPostEventSem(hevEnd);
            DosWaitEventSem(hevPut, SEM_INDEFINITE_WAIT);
            DosResetEventSem(hevPut, &ulPostCount);
            if (shmem) DosFreeMem(shmem);
            break;
        }
    }
    WinDestroyMsgQueue(hmq);
    WinTerminate(hab);
}

int main() {
    SEMRECORD sem[2];
    int rc;
    
    rc = DosCreateMutexSem(SEM_PREFIX "CLIPSYN", 
                           &hmtxSyn,
                           0,
                           0);
    if (rc != 0) return 1;
    puts("CLIPSYN");
    
    rc = DosCreateEventSem(SEM_PREFIX "CLIPEND", 
                           &hevEnd,
                           0, 
                           0);
    if (rc != 0) return 1;
    puts("CLIPEND");
    
    rc = DosCreateEventSem(SEM_PREFIX "CLIPGET", 
                           &hevGet,
                           0, 
                           0);
    if (rc != 0) return 1;
    puts("CLIPGET");
    
    rc = DosCreateEventSem(SEM_PREFIX "CLIPPUT",
                           &hevPut,
                           0,
                           0);
    if (rc != 0) return 1;
    puts("CLIPPUT");
    
    sem[0].hsemCur = (PULONG) hevGet;
    sem[0].ulUser = CMD_GET;
    sem[1].hsemCur = (PULONG) hevPut;
    sem[1].ulUser = CMD_PUT;
    
    rc = DosCreateMuxWaitSem(0,
                             &hmuxWait,
                             2,
                             sem,
                             DCMW_WAIT_ANY);
    if (rc != 0) return 1;
    puts("CLIPMUX");
    
    hab = WinInitialize(0);
    
    hmq = WinCreateMsgQueue(hab, 0);
#if defined(__EMX__) || defined(__TOS_OS2__)
    _beginthread(clipsrv, NULL, 8192, 0);
#else
    _beginthread(clipsrv, 8192, 0);
#endif
    while (WinGetMsg (hab, &qmsg, 0, 0, 0))
        WinDispatchMsg (hab, &qmsg);
    
    
    WinDestroyMsgQueue(hmq);
    WinTerminate(hab);
    return 0;
}
