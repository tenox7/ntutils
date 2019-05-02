/*    clip_pm.cpp
 *
 *    Copyright (c) 1994-1998, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "fte.h"
#include "clip.h"

#define INCL_WIN
#define INCL_DOS
#define INCL_ORDINALS
#include <os2.h>
#include <stdlib.h>

/*
 *     Pointers to PM functions
 *     Instead of calling PM directly, we are obtaining these pointers
 * on first access to clipboard.
 *     All these prototypes are copied from OS2.H
 *     Note: all prototypes are 32-bit version only
 */

#define ORD_WIN32ADDATOM                 700
#define ORD_WIN32ALARM                   701
#define ORD_WIN32BEGINENUMWINDOWS        702
#define ORD_WIN32BEGINPAINT              703
#define ORD_WIN32CALCFRAMERECT           704
#define ORD_WIN32CANCELSHUTDOWN          705
#define ORD_WIN32CLOSECLIPBRD            707
#define ORD_WIN32COMPARESTRINGS          708
#define ORD_WIN32COPYACCELTABLE          709
#define ORD_WIN32COPYRECT                710
#define ORD_WIN32CPTRANSLATECHAR         711
#define ORD_WIN32CPTRANSLATESTRING       712
#define ORD_WIN32CREATEACCELTABLE        713
#define ORD_WIN32CREATEATOMTABLE         714
#define ORD_WIN32CREATECURSOR            715
#define ORD_WIN32CREATEMSGQUEUE          716
#define ORD_WIN32CREATEPOINTER           717
#define ORD_WIN32DDEINITIATE             718
#define ORD_WIN32DDEPOSTMSG              719
#define ORD_WIN32DDERESPOND              720
#define ORD_WIN32DELETEATOM              721
#define ORD_WIN32DELETELIBRARY           722
#define ORD_WIN32DESTROYACCELTABLE       723
#define ORD_WIN32DESTROYATOMTABLE        724
#define ORD_WIN32DESTROYCURSOR           725
#define ORD_WIN32DESTROYMSGQUEUE         726
#define ORD_WIN32DESTROYPOINTER          727
#define ORD_WIN32DESTROYWINDOW           728
#define ORD_WIN32DISMISSDLG              729
#define ORD_WIN32DRAWBITMAP              730
#define ORD_WIN32DRAWBORDER              731
#define ORD_WIN32DRAWPOINTER             732
#define ORD_WIN32EMPTYCLIPBRD            733
#define ORD_WIN32ENABLEPHYSINPUT         734
#define ORD_WIN32ENABLEWINDOW            735
#define ORD_WIN32ENABLEWINDOWUPDATE      736
#define ORD_WIN32ENDENUMWINDOWS          737
#define ORD_WIN32ENDPAINT                738
#define ORD_WIN32ENUMCLIPBRDFMTS         739
#define ORD_WIN32ENUMDLGITEM             740
#define ORD_WIN32EQUALRECT               741
#define ORD_WIN32EXCLUDEUPDATEREGION     742
#define ORD_WIN32FILLRECT                743
#define ORD_WIN32FINDATOM                744
#define ORD_WIN32FLASHWINDOW             745
#define ORD_WIN32FOCUSCHANGE             746
#define ORD_WIN32FREEERRORINFO           748
#define ORD_WIN32GETCLIPPS               749
#define ORD_WIN32GETCURRENTTIME          750
#define ORD_WIN32GETERRORINFO            751
#define ORD_WIN32GETKEYSTATE             752
#define ORD_WIN32GETLASTERROR            753
#define ORD_WIN32GETMAXPOSITION          754
#define ORD_WIN32GETMINPOSITION          755
#define ORD_WIN32GETNEXTWINDOW           756
#define ORD_WIN32GETPS                   757
#define ORD_WIN32GETPHYSKEYSTATE         758
#define ORD_WIN32GETSCREENPS             759
#define ORD_WIN32GETSYSBITMAP            760
#define ORD_WIN32INSENDMSG               761
#define ORD_WIN32INFLATERECT             762
#define ORD_WIN32INITIALIZE              763
#define ORD_WIN32INTERSECTRECT           764
#define ORD_WIN32INVALIDATERECT          765
#define ORD_WIN32INVALIDATEREGION        766
#define ORD_WIN32INVERTRECT              767
#define ORD_WIN32ISCHILD                 768
#define ORD_WIN32ISPHYSINPUTENABLED      769
#define ORD_WIN32ISRECTEMPTY             770
#define ORD_WIN32ISTHREADACTIVE          771
#define ORD_WIN32ISWINDOW                772
#define ORD_WIN32ISWINDOWENABLED         773
#define ORD_WIN32ISWINDOWSHOWING         774
#define ORD_WIN32ISWINDOWVISIBLE         775
#define ORD_WIN32LOADACCELTABLE          776
#define ORD_WIN32LOADLIBRARY             777
#define ORD_WIN32LOADMENU                778
#define ORD_WIN32LOADMESSAGE             779
#define ORD_WIN32LOADPOINTER             780
#define ORD_WIN32LOADSTRING              781
#define ORD_WIN32LOCKVISREGIONS          782
#define ORD_WIN32LOCKWINDOWUPDATE        784
#define ORD_WIN32MAKEPOINTS              785
#define ORD_WIN32MAKERECT                786
#define ORD_WIN32MAPDLGPOINTS            787
#define ORD_WIN32MAPWINDOWPOINTS         788
#define ORD_WIN32MESSAGEBOX              789
#define ORD_WIN32MSGSEMWAIT              790
#define ORD_WIN32NEXTCHAR                791
#define ORD_WIN32OFFSETRECT              792
#define ORD_WIN32OPENCLIPBRD             793
#define ORD_WIN32OPENWINDOWDC            794
#define ORD_WIN32PREVCHAR                795
#define ORD_WIN32PROCESSDLG              796
#define ORD_WIN32PTINRECT                797
#define ORD_WIN32QUERYACCELTABLE         798
#define ORD_WIN32QUERYACTIVEWINDOW       799
#define ORD_WIN32QUERYANCHORBLOCK        800
#define ORD_WIN32QUERYATOMLENGTH         801
#define ORD_WIN32QUERYATOMNAME           802
#define ORD_WIN32QUERYATOMUSAGE          803
#define ORD_WIN32QUERYCAPTURE            804
#define ORD_WIN32QUERYCLASSNAME          805
#define ORD_WIN32QUERYCLIPBRDDATA        806
#define ORD_WIN32QUERYCLIPBRDFMTINFO     807
#define ORD_WIN32QUERYCLIPBRDOWNER       808
#define ORD_WIN32QUERYCLIPBRDVIEWER      809
#define ORD_WIN32QUERYCP                 810
#define ORD_WIN32QUERYCPLIST             811
#define ORD_WIN32QUERYCURSORINFO         812
#define ORD_WIN32QUERYDESKTOPWINDOW      813
#define ORD_WIN32QUERYDLGITEMSHORT       814
#define ORD_WIN32QUERYDLGITEMTEXT        815
#define ORD_WIN32QUERYDLGITEMTEXTLENGTH  816
#define ORD_WIN32QUERYFOCUS              817
#define ORD_WIN32QUERYMSGPOS             818
#define ORD_WIN32QUERYMSGTIME            819
#define ORD_WIN32QUERYOBJECTWINDOW       820
#define ORD_WIN32QUERYPOINTER            821
#define ORD_WIN32QUERYPOINTERINFO        822
#define ORD_WIN32QUERYPOINTERPOS         823
#define ORD_WIN32QUERYQUEUEINFO          824
#define ORD_WIN32QUERYQUEUESTATUS        825
#define ORD_WIN32QUERYSYSCOLOR           826
#define ORD_WIN32QUERYSYSMODALWINDOW     827
#define ORD_WIN32QUERYSYSPOINTER         828
#define ORD_WIN32QUERYSYSVALUE           829
#define ORD_WIN32QUERYSYSTEMATOMTABLE    830
#define ORD_WIN32QUERYUPDATERECT         831
#define ORD_WIN32QUERYUPDATEREGION       832
#define ORD_WIN32QUERYVERSION            833
#define ORD_WIN32QUERYWINDOW             834
#define ORD_WIN32QUERYWINDOWDC           835
#define ORD_WIN32QUERYWINDOWPOS          837
#define ORD_WIN32QUERYWINDOWPROCESS      838
#define ORD_WIN32QUERYWINDOWPTR          839
#define ORD_WIN32QUERYWINDOWRECT         840
#define ORD_WIN32QUERYWINDOWTEXT         841
#define ORD_WIN32QUERYWINDOWTEXTLENGTH   842
#define ORD_WIN32QUERYWINDOWULONG        843
#define ORD_WIN32QUERYWINDOWUSHORT       844
#define ORD_WIN32REGISTERUSERDATATYPE    845
#define ORD_WIN32REGISTERUSERMSG         846
#define ORD_WIN32RELEASEPS               848
#define ORD_WIN32SCROLLWINDOW            849
#define ORD_WIN32SETACCELTABLE           850
#define ORD_WIN32SETACTIVEWINDOW         851
#define ORD_WIN32SETCAPTURE              852
#define ORD_WIN32SETCLASSMSGINTEREST     853
#define ORD_WIN32SETCLIPBRDDATA          854
#define ORD_WIN32SETCLIPBRDOWNER         855
#define ORD_WIN32SETCLIPBRDVIEWER        856
#define ORD_WIN32SETCP                   857
#define ORD_WIN32SETDLGITEMSHORT         858
#define ORD_WIN32SETDLGITEMTEXT          859
#define ORD_WIN32SETFOCUS                860
#define ORD_WIN32SETMSGINTEREST          861
#define ORD_WIN32SETMSGMODE              862
#define ORD_WIN32SETMULTWINDOWPOS        863
#define ORD_WIN32SETOWNER                864
#define ORD_WIN32SETPARENT               865
#define ORD_WIN32SETPOINTER              866
#define ORD_WIN32SETPOINTERPOS           867
#define ORD_WIN32SETRECT                 868
#define ORD_WIN32SETRECTEMPTY            869
#define ORD_WIN32SETSYNCHROMODE          870
#define ORD_WIN32SETSYSCOLORS            871
#define ORD_WIN32SETSYSMODALWINDOW       872
#define ORD_WIN32SETSYSVALUE             873
#define ORD_WIN32SETWINDOWBITS           874
#define ORD_WIN32SETWINDOWPOS            875
#define ORD_WIN32SETWINDOWPTR            876
#define ORD_WIN32SETWINDOWTEXT           877
#define ORD_WIN32SETWINDOWULONG          878
#define ORD_WIN32SETWINDOWUSHORT         879
#define ORD_WIN32SHOWCURSOR              880
#define ORD_WIN32SHOWPOINTER             881
#define ORD_WIN32SHOWTRACKRECT           882
#define ORD_WIN32SHOWWINDOW              883
#define ORD_WIN32STARTTIMER              884
#define ORD_WIN32STOPTIMER               885
#define ORD_WIN32SUBSTITUTESTRINGS       886
#define ORD_WIN32SUBTRACTRECT            887
#define ORD_WIN32TERMINATE               888
#define ORD_WIN32TRACKRECT               890
#define ORD_WIN32UNIONRECT               891
#define ORD_WIN32UPDATEWINDOW            892
#define ORD_WIN32UPPER                   893
#define ORD_WIN32UPPERCHAR               894
#define ORD_WIN32VALIDATERECT            895
#define ORD_WIN32VALIDATEREGION          896
#define ORD_WIN32WAITMSG                 897
#define ORD_WIN32WINDOWFROMDC            898
#define ORD_WIN32WINDOWFROMID            899
#define ORD_WIN32WINDOWFROMPOINT         900
#define ORD_WIN32BROADCASTMSG            901
#define ORD_WIN32POSTQUEUEMSG            902
#define ORD_WIN32SENDDLGITEMMSG          903
#define ORD_WIN32TRANSLATEACCEL          904
#define ORD_WIN32CALLMSGFILTER           905
#define ORD_WIN32CREATEFRAMECONTROLS     906
#define ORD_WIN32CREATEMENU              907
#define ORD_WIN32CREATESTDWINDOW         908
#define ORD_WIN32CREATEWINDOW            909
#define ORD_WIN32DEFDLGPROC              910
#define ORD_WIN32DEFWINDOWPROC           911
#define ORD_WIN32DISPATCHMSG             912
#define ORD_WIN32DRAWTEXT                913
#define ORD_WIN32GETDLGMSG               914
#define ORD_WIN32GETMSG                  915
#define ORD_WIN32MSGMUXSEMWAIT           916
#define ORD_WIN32MULTWINDOWFROMIDS       917
#define ORD_WIN32PEEKMSG                 918
#define ORD_WIN32POSTMSG                 919
#define ORD_WIN32SENDMSG                 920
#define ORD_WIN32SETKEYBOARDSTATETABLE   921
#define ORD_WIN32CREATEDLG               922
#define ORD_WIN32DLGBOX                  923
#define ORD_WIN32LOADDLG                 924
#define ORD_WIN32QUERYCLASSINFO          925
#define ORD_WIN32REGISTERCLASS           926
#define ORD_WIN32RELEASEHOOK             927
#define ORD_WIN32SETHOOK                 928
#define ORD_WIN32SUBCLASSWINDOW          929
#define ORD_WIN32SETCLASSTHUNKPROC       930
#define ORD_WIN32QUERYCLASSTHUNKPROC     931
#define ORD_WIN32SETWINDOWTHUNKPROC      932
#define ORD_WIN32QUERYWINDOWTHUNKPROC    933
#define ORD_WIN32QUERYWINDOWMODEL        934
#define ORD_WIN32SETDESKTOPBKGND         935
#define ORD_WIN32QUERYDESKTOPBKGND       936
#define ORD_WIN32POPUPMENU               937
#define ORD_WIN32SETPRESPARAM            938
#define ORD_WIN32QUERYPRESPARAM          939
#define ORD_WIN32REMOVEPRESPARAM         940
#define ORD_WIN32REALIZEPALETTE          941
#define ORD_WIN32CREATEPOINTERINDIRECT   942
#define ORD_WIN32SAVEWINDOWPOS           943
#define ORD_WIN32GETERASEPS              952
#define ORD_WIN32RELEASEERASEPS          953
#define ORD_WIN32SETPOINTEROWNER         971
#define ORD_WIN32STRETCHPOINTER          968
#define ORD_WIN32SETERRORINFO            977
#define ORD_WIN32WAITEVENTSEM            978
#define ORD_WIN32REQUESTMUTEXSEM         979
#define ORD_WIN32WAITMUXWAITSEM          980


#ifndef INCL_32
#error Prototypes are for 32-bit compiler only
#endif

HAB    (APIENTRY *p_WinInitialize)(ULONG flOptions);
BOOL   (APIENTRY *p_WinTerminate)(HAB hab);
HMQ    (APIENTRY *p_WinCreateMsgQueue)(HAB hab, LONG cmsg);
BOOL   (APIENTRY *p_WinDestroyMsgQueue)(HMQ hmq);
BOOL   (APIENTRY *p_WinEmptyClipbrd)(HAB hab);
BOOL   (APIENTRY *p_WinOpenClipbrd)(HAB hab);
BOOL   (APIENTRY *p_WinCloseClipbrd)(HAB hab);
BOOL   (APIENTRY *p_WinSetClipbrdData)(HAB hab, ULONG ulData, ULONG fmt, ULONG rgfFmtInfo);
ULONG  (APIENTRY *p_WinQueryClipbrdData)(HAB hab, ULONG fmt);

static struct impentry {
    ULONG ordinal;
    PFN *pointer;
} imported_functions[] = {
    { ORD_WIN32INITIALIZE,     (PFN *) &p_WinInitialize       },
    { ORD_WIN32TERMINATE,      (PFN *) &p_WinTerminate        },
    { ORD_WIN32CREATEMSGQUEUE, (PFN *) &p_WinCreateMsgQueue   },
    { ORD_WIN32DESTROYMSGQUEUE,(PFN *) &p_WinDestroyMsgQueue  },
    { ORD_WIN32EMPTYCLIPBRD,   (PFN *) &p_WinEmptyClipbrd     },
    { ORD_WIN32OPENCLIPBRD,    (PFN *) &p_WinOpenClipbrd      },
    { ORD_WIN32CLOSECLIPBRD,   (PFN *) &p_WinCloseClipbrd     },
    { ORD_WIN32SETCLIPBRDDATA, (PFN *) &p_WinSetClipbrdData   },
    { ORD_WIN32QUERYCLIPBRDDATA,(PFN *)&p_WinQueryClipbrdData },
    { 0, 0 }
};

/*
 *    Load PMWIN.DLL and get pointers to all reqd PM functions
 */

static BOOL loadDLL() {
    static BOOL loaded = FALSE;
    static BOOL loaded_ok = FALSE;
    static HMODULE pmwin;

    char error[200];

    if (loaded == TRUE)
        return loaded_ok;

    loaded = TRUE;

    if (DosLoadModule((PSZ)error, sizeof(error), (PSZ)"PMWIN.DLL", &pmwin) == 0) {
        struct impentry *imp;

        for (imp = imported_functions; imp->ordinal; imp++)
            if (DosQueryProcAddr(pmwin, imp->ordinal, NULL, imp->pointer) != 0)
                return FALSE;

        loaded_ok = TRUE;
    }

    return loaded_ok;
}

/*
 AccessPmClipboard:
 Purpose: perform all necessary PM stuff and open clipboard for access;
 Return : TRUE on success, FALSE on error;
 Note   : on error, clipboard is released automatically.

 LeavePmClipboard:
 Purpose: releases previously opened clipboard and clear all PM stuff
 Return : none
 */

static struct {
    PPIB ppib;
    HAB hab;
    HMQ hmq;
    ULONG savedtype;
    BOOL opened;
} PmInfo;

static void LeavePmClipboard() {

    if (PmInfo.opened)
        p_WinCloseClipbrd(PmInfo.hab);
    if (PmInfo.hmq)
        p_WinDestroyMsgQueue(PmInfo.hmq);
    if (PmInfo.hab)
        p_WinTerminate(PmInfo.hab);
    PmInfo.ppib->pib_ultype = PmInfo.savedtype;
}

static BOOL AccessPmClipboard() {
    PTIB ptib;

    if (loadDLL() == FALSE) {
        DosBeep(100, 1500);
        return FALSE;
    }

    memset(&PmInfo, 0, sizeof(PmInfo));

    // mutate into PM application for clipboard (Win**) functions access
    DosGetInfoBlocks(&ptib, &PmInfo.ppib);
    PmInfo.savedtype = PmInfo.ppib->pib_ultype;
    PmInfo.ppib->pib_ultype = PROG_PM;

    if ((PmInfo.hab = p_WinInitialize(0)) != NULLHANDLE) {
        if ((PmInfo.hmq = p_WinCreateMsgQueue(PmInfo.hab, 0)) != NULLHANDLE) {
            if (p_WinOpenClipbrd(PmInfo.hab) == TRUE) {
                PmInfo.opened = TRUE;
            }
        }
    }
    if (PmInfo.opened != TRUE) {
        LeavePmClipboard();
        DosBeep(100, 1500);
    }
    return PmInfo.opened;
}

int GetClipText(ClipData *cd) {
    int rc = 0;
    char *text;

    cd->fLen = 0;
    cd->fChar = 0;

    if (!AccessPmClipboard())
        return rc;

    if ((text = (char *) p_WinQueryClipbrdData(PmInfo.hab, CF_TEXT)) != 0) {
        cd->fLen = strlen(text);
        cd->fChar = strdup(text);
        rc = 1;
    }

    LeavePmClipboard();
    return rc;
}

int PutClipText(ClipData *cd) {
    ULONG len;
    void *text;
    int rc = 0;

    if (!AccessPmClipboard())
        return rc;

    p_WinEmptyClipbrd(PmInfo.hab);
    len = cd->fLen;

    if (len) {
        DosAllocSharedMem((void **)&text,
                          0,
                          len + 1,
                          PAG_READ | PAG_WRITE | PAG_COMMIT | OBJ_GIVEABLE);
        strncpy((char *)text, cd->fChar, len + 1);
        if (!p_WinSetClipbrdData(PmInfo.hab, (ULONG) text, CF_TEXT, CFI_POINTER))
            DosBeep(100, 1500);
        else
            rc = 1;
    }

    LeavePmClipboard();
    return rc;
}
