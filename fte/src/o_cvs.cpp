/*
 * o_cvs.cpp
 *
 * Contributed by Martin Frydl <frydl@matfyz.cz>
 *
 * Class providing access to most of CVS commands.
 */

#include "o_cvs.h"

#ifdef CONFIG_OBJ_CVS

#include "i_view.h"
#include "e_cvslog.h"
#include "o_buflist.h"
#include "s_files.h"
#include "sysdep.h"

#include <stdio.h>

static int SameDir(const char *D1, const char *D2)
{
    if (!D1||!D2) return 0;
    size_t l1=strlen (D1);
    size_t l2=strlen (D2);
    if (l1<l2) return strncmp (D1,D2,l1)==0&&strcmp (D2+l1,SSLASH)==0;
    else if (l1==l2) return !strcmp (D1,D2);
    else return strncmp (D1,D2,l2)==0&&strcmp (D1+l1,SSLASH)==0;
}

static const char CvsStatusChars[] = "?UPMCAR";
ECvs *CvsView=0;

ECvs::ECvs(int createFlags, EModel **ARoot, char* ADir, char* ACommand,
	   char* AOnFiles) :
    ECvsBase(createFlags, ARoot, "CVS"),
    LogFile(0),
    Commiting(0)
{
    CvsView=this;
    RunPipe (ADir, ACommand, AOnFiles);
}

ECvs::ECvs (int createFlags,EModel **ARoot) :
    ECvsBase (createFlags, ARoot, "CVS"),
    LogFile(0)
{
    CvsView=this;
}

ECvs::~ECvs () {
    CvsView=0;
    RemoveLogFile ();
}

void ECvs::RemoveLogFile()
{
    if (LogFile) {
        unlink (LogFile);
        free (LogFile);
        LogFile=0;
    }
}

char* ECvs::MarkedAsList()
{
    int i;
    size_t len=0;
    // First pass - calculate size
    for (i=0;i<LineCount;i++) if (Lines[i]->Status&2) len+=strlen (Lines[i]->File)+1;
    if (len==0) {
        // Nothing marked, use the file at cursor
        if (Lines[Row]->Status&4) return strdup (Lines[Row]->File);
        else return NULL;
    }
    char *s=(char *)malloc (len+1);
    s[0]=0;
    for (i=0;i<LineCount;i++) if (Lines[i]->Status&2) strcat (strcat (s,Lines[i]->File)," ");
    s[strlen (s)-1]=0;
    return s;
}

char ECvs::GetFileStatus(const char *file)
{
    // Search backward, file can be present several times (old messages)
    for (int i=LineCount-1;i>=0;i--)
        if (Lines[i]->File&&filecmp (Lines[i]->File,file)==0) return Lines[i]->Msg[0];
    return 0;
}

void ECvs::ParseLine(char* line, int len)
{
    if (len>2&&line[1]==' '&&strchr (CvsStatusChars,line[0])) {
        AddLine (line+2,-1,line,5);
    } else AddLine (0,-1,line);
}

int ECvs::RunPipe(const char *ADir, const char *ACommand, const char *AOnFiles)
{
    Commiting=0;
    if (!SameDir (Directory,ADir)) FreeLines ();
    return ECvsBase::RunPipe (ADir,ACommand,AOnFiles);
}

void ECvs::ClosePipe()
{
    ECvsBase::ClosePipe ();
    if (Commiting&&!ReturnCode) {
        // Successful commit - reload files
        // Is it safe to do this ? Currently not done, manual reload is needed
    }
    Commiting=0;
}

int ECvs::RunCommit(const char *ADir, const char *ACommand, const char *AOnFiles)
{
    if (!SameDir (Directory,ADir)) FreeLines ();

    free (Command);
    free (Directory);
    free (OnFiles);

    Command=strdup (ACommand);
    Directory=strdup (ADir);
    OnFiles=strdup (AOnFiles);

    RemoveLogFile ();
    // Disallow any CVS command while commiting
    Running=1;

    // Create message buffer
    ECvsLog *cvslog=new ECvsLog (0,&ActiveModel,Directory,OnFiles);
    LogFile=strdup (cvslog->FileName);
    View->SwitchToModel (cvslog);

    AddLine (LogFile,-1,"CVS commit start - enter message text",1);

    return 0;
}

extern BufferView *BufferList;//!!!

int ECvs::DoneCommit(int commit)
{
    Running=0;
    // Remove line with link to log
    free (Lines[LineCount-1]->File);
    free (Lines[LineCount-1]->Msg);
    LineCount--;
    UpdateList ();

    if (commit) {
        // We need a copy of Command/Directory/OnFiles because RunPipe deletes them!
        char *ACommand=(char *)malloc (strlen (Command)+strlen (LogFile)+10);
        char *ADirectory=strdup (Directory);
        char *AOnFiles=strdup (OnFiles);
        sprintf (ACommand,"%s -F %s",Command,LogFile);
        int ret=RunPipe (ADirectory,ACommand,AOnFiles);
        free (ACommand);free (ADirectory);free (AOnFiles);
        // We set Commiting after RunPipe since it sets it to 0
        // This is OK since FTE is not multi-threaded
        Commiting=1;

        if (ActiveView->Model==CvsLogView) {
            // CvsLogView is currently active, move CvsView just after it
            if (CvsLogView->Next!=CvsView) {
                // Need to move, is not at right place yet
                // Here we use the fact that if current model is closed,
                // the one just after it (Next) is focused.
                CvsView->Prev->Next=CvsView->Next;
                CvsView->Next->Prev=CvsView->Prev;
                CvsView->Next=CvsLogView->Next;
                CvsLogView->Next->Prev=CvsView;
                CvsLogView->Next=CvsView;
                CvsView->Prev=CvsLogView;
            }
        }
        // else - CvsLogView is not active, there is need to make CvsView
        // active some other way. However, SwitchToModel() or SelectModel()
        // calls does not work. Currently I don't know how to do this. !!!

        return ret;
    } else {
        RemoveLogFile();
        UpdateList();
        return 0;
    }
}

// If running, can't be closed without asking
int ECvs::CanQuit()
{
    if (Running) return 0;else return 1;
}

// Ask user if we can close this model
int ECvs::ConfQuit(GxView* V, int multiFile)
{
    if (CvsLogView) {
        // Log is open
        if (CvsLogView->ConfQuit(V,multiFile)) {
            // Commit confirmed or discarded - depends on Running
            ActiveView->DeleteModel(CvsLogView);
        } else return 0;
    }
    if (Running) {
        // CVS command in progress
        switch (V->Choice(GPC_ERROR,"CVS command is running",2,"&Kill","&Cancel","")) {
            case 0: // Kill
                return 1;
            case 1: // Cancel
            default:
                return 0;
        }
    } else return 1;
}

// Event map - this name is used in config files when defining eventmap
EEventMap *ECvs::GetEventMap()
{
    return FindEventMap("CVS");
}

#endif
