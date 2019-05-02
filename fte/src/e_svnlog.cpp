/*
 * e_svnlog.cpp
 *
 * S.Pinigin copy o_cvslog.cpp and replace cvs/Cvs/CVS to svn/Svn/SVN.
 *
 * Subclass of EBuffer for writing log for SVN commit. Creates temporary file
 * used for commit which is deleted when view is closed. Asks for commit or
 * discard on view close.
 */

#include "e_svnlog.h"

#ifdef CONFIG_OBJ_SVN

#include "c_config.h"
#include "i_view.h"
#include "o_svn.h"
#include "s_string.h"
#include "sysdep.h"

#include <ctype.h>
#include <stdio.h>

ESvnLog *SvnLogView;

ESvnLog::ESvnLog(int createFlags,EModel **ARoot,char *Directory,char *OnFiles) :
    EBuffer(createFlags,ARoot,NULL)
{
    int i,j,p;
    char msgFile[MAXPATH];

    SvnLogView=this;
    // Create filename for message
#ifdef UNIX
    // Use this in Unix - it says more to user
    sprintf(msgFile,"/tmp/fte%d-svn-msg",getpid());
#else
    tmpnam(msgFile);
#endif
    SetFileName(msgFile,SvnLogMode);

    // Preload buffer with info
    InsertLine(0,0, "");
    InsertLine(1,60, "SVN: -------------------------------------------------------");
    InsertLine(2,59, "SVN: Enter log. Lines beginning with 'SVN:' will be removed");
    InsertLine(3,4, "SVN:");
    InsertLine(4,18, "SVN: Commiting in ");
    InsText(4,18,strlen(Directory),Directory);
    if (OnFiles[0]) {
        p=5;
        // Go through files - use GetFileStatus to show what to do with files
        // First count files
        int cnt=0;i=0;
        while (1) {
            if (OnFiles[i]==0||OnFiles[i]==' ') {
                while (OnFiles[i]==' ') i++;
                cnt++;
                if (!OnFiles[i]) break;
            } else i++;
        }
        int *position=new int[cnt];
        int *len=new int[cnt];
        char *status=new char[cnt];
        // Find out position and status for each file
        i=j=0;position[0]=0;
        while (1) {
            if (OnFiles[i]==0||OnFiles[i]==' ') {
                // This is not thread-safe!
                len[j]=i-position[j];
                char c=OnFiles[i];
                OnFiles[i]=0;
                status[j]=SvnView->GetFileStatus(OnFiles+position[j]);
                if (status[j]==0) status[j]='x';
                OnFiles[i]=c;
                while (OnFiles[i]==' ') i++;
                if (!OnFiles[i]) break;
                position[++j]=i;
            } else i++;
        }
        // Go through status
        int fAdded=0,fRemoved=0,fModified=0,fOther=0;
	for (i=0;i<cnt;i++)
	    switch (toupper(status[i])) {
	    case 'A': fAdded++; break;
	    case 'R': fRemoved++; break;
	    case 'M': fModified++; break;
	    default:fOther++;
	    }

        // Now list files with given status
        ListFiles(p, fAdded, "Added", cnt, position, len, status, OnFiles, "Aa");
        ListFiles(p, fRemoved, "Removed", cnt, position, len, status, OnFiles, "Rr");
        ListFiles(p, fModified, "Modified", cnt, position, len, status, OnFiles, "Mm");
        ListFiles(p, fOther, "Other", cnt, position, len, status, OnFiles, "AaRrMm", 1);
        delete position;delete len;delete status;
    } else {
        InsertLine(5,4, "SVN:");
        InsertLine(6,30, "SVN: Commiting whole directory");
        p=7;
    }
    InsertLine(p,4, "SVN:");
    InsertLine(p+1,60, "SVN: -------------------------------------------------------");
    SetPos(0,0);
    FreeUndo();
    Modified=0;
}

ESvnLog::~ESvnLog() {
    SvnLogView=0;
}

void ESvnLog::ListFiles(int &p,const int fCount,const char *title,const int cnt,const int *position,
                        const int *len,const char *status,const char *list,const char *excinc,const int exc) {
    if (fCount) {
        size_t i = strlen(title);
        InsertLine(p++, 4, "SVN:");
        InsertLine(p, 5, "SVN: ");
        InsText (p,5,i, title);
        InsText (p,i+=5,5, " file");
        i+=5;
        if (fCount!=1) InsText (p,i++,1, "s");
        InsText (p++,i,1, ":");
        for (i=0;i<cnt;i++)
            if (!!strchr(excinc,status[i])^!!exc) {
                // Should be displayed
                InsertLine(p,9, "SVN:     ");
                InsText(p,9,1, status+i);InsText (p,10,1, " ");
                InsText(p++,11,len[i], list+position[i]);
            }
    }
}

// Overridden because we don't want to load file
EViewPort *ESvnLog::CreateViewPort(EView *V) {
    V->Port = new EEditPort(this, V);
    AddView(V);
    return V->Port;
}

int ESvnLog::CanQuit() {
    return 0;
}

int ESvnLog::ConfQuit(GxView *V,int /*multiFile*/) {
    int i;

    switch (V->Choice(GPC_ERROR,"SVN commit pending",3,"C&ommit","&Discard","&Cancel","")) {
        case 0: // Commit
            // First save - this is just try
            if (Save ()==0) return 0;
            // Now remove SVN: lines and really save
            for (i=0;i<RCount;) {
                PELine l=RLine (i);
                if (l->Count>=4&&strncmp (l->Chars,"SVN:",4)==0) DelLine (i);else i++;
            }
            Save ();
            // DoneCommit returns 0 if OK
            return !SvnView->DoneCommit (1);
        case 1: // Discard
            SvnView->DoneCommit (0);
            return 1;
        case 2: // Cancel
        default:
            return 0;
    }
}

// Shown in "Closing xxx..." message when closing model
void ESvnLog::GetName (char *AName, size_t MaxLen) {
    strlcpy(AName, "SVN log", MaxLen);
}

// Shown in buffer list
void ESvnLog::GetInfo (char *AInfo, size_t MaxLen) {
    snprintf(AInfo, MaxLen, "%2d %04d:%03d%cSVN log: %s",
	     ModelNo, 1 + CP.Row, 1 + CP.Col,
	     Modified ? '*' : ' ', FileName);
}

// Normal and short title (normal for window, short for icon in X)
void ESvnLog::GetTitle(char *ATitle, size_t MaxLen, char *ASTitle, size_t SMaxLen) {
    strlcpy (ATitle, "SVN log", MaxLen);
    strlcpy (ASTitle, "SVN log", SMaxLen);
}

#endif
