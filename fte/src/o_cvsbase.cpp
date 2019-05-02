/*
 * o_cvsbase.cpp
 *
 * Contributed by Martin Frydl <frydl@matfyz.cz>
 *
 * Base class for all other CVS-related classes. This is similar to EMessages
 * - starts CVS and shows its messages in list view.
 */

#include "o_cvsbase.h"

#ifdef CONFIG_OBJ_CVS

#include "c_commands.h"
#include "i_view.h"
#include "o_buflist.h"
#include "s_files.h"
#include "s_string.h"
#include "s_util.h"
#include "sysdep.h"

#include <stdio.h>

#define MAXREGEXP 32

static int CvsIgnoreRegexpCount=0;
static RxNode *CvsIgnoreRegexp[MAXREGEXP];

int AddCvsIgnoreRegexp (const char *regexp) {
    if (CvsIgnoreRegexpCount>=MAXREGEXP) return 0;
    if ((CvsIgnoreRegexp[CvsIgnoreRegexpCount]=RxCompile (regexp))==NULL) return 0;
    CvsIgnoreRegexpCount++;
    return 1;
}

void FreeCvsIgnoreRegexp () {
    while (CvsIgnoreRegexpCount--) {
        RxFree (CvsIgnoreRegexp[CvsIgnoreRegexpCount]);
    }
}

ECvsBase::ECvsBase (int createFlags,EModel **ARoot, const char *title) :
    EList (createFlags, ARoot, title),
    Command(0),
    Directory(0),
    OnFiles(0),
    LineCount(0),
    Lines(0),
    Running(0),
    BufLen(0),
    BufPos(0),
    PipeId(-1),
    ReturnCode(-1)
{
}

ECvsBase::~ECvsBase () {
    gui->ClosePipe (PipeId);
    FreeLines ();
    free (Command);
    free (Directory);
    free (OnFiles);
}

void ECvsBase::FreeLines () {
    if (Lines) {
        for (int i=0;i<LineCount;i++) {
            if (Lines[i]->Buf&&Lines[i]->Line>=0) {
                // Has buffer and line == bookmark -> remove it
                char book[16];
                sprintf (book,"_CVS.%d",i);
                Lines[i]->Buf->RemoveBookmark (book);
            }
            free (Lines[i]->Msg);
            free (Lines[i]->File);
            free (Lines[i]);
        }
        free (Lines);
    }
    LineCount=0;
    Lines=0;
    BufLen=BufPos=0;
}

void ECvsBase::AddLine(const char *file, int line, const char *msg, int status) {
    CvsLine *l;

    l=(CvsLine *)malloc (sizeof (CvsLine));
    if (l!=0) {
        l->File=file?strdup (file):0;
        l->Line=line;
        l->Msg=msg?strdup (msg):0;
        l->Buf=0;
        l->Status=status;

        LineCount++;
        Lines=(CvsLine **)realloc (Lines,sizeof (CvsLine *)*LineCount);
        Lines[LineCount-1]=l;
        FindBuffer (LineCount-1);

        UpdateList ();
    }
}

void ECvsBase::FindBuffer (int line) {
    assert (line>=0&&line<LineCount);
    if (Lines[line]->File!=0) {
        EBuffer *B;
        Lines[line]->Buf=0;
        char path[MAXPATH];
        strcpy (path,Directory);Slash (path,1);strcat (path,Lines[line]->File);
        B=FindFile (path);
        if (B!=0&&B->Loaded!=0) AssignBuffer (B,line);
    }
}

void ECvsBase::AssignBuffer (EBuffer *B,int line) {
    assert (line>=0&&line<LineCount);

    char book[16];
    EPoint P;

    Lines[line]->Buf=B;
    if (Lines[line]->Line>=0) {
        sprintf (book,"_CVS.%d",line);
        P.Col=0;
        P.Row=Lines[line]->Line;
        if (P.Row>=B->RCount)
            P.Row=B->RCount-1;
        B->PlaceBookmark (book,P);
    }
}

void ECvsBase::FindFileLines (EBuffer *B) {
    char path[MAXPATH];
    char *pos;
    strcpy (path,Directory);Slash (path,1);pos=path+strlen (path);
    for (int i=0;i<LineCount;i++)
        if (Lines[i]->Buf==0&&Lines[i]->File!=0) {
            strcpy (pos,Lines[i]->File);
            if (filecmp (B->FileName,path)==0) {
                AssignBuffer (B,i);
            }
        }
}

void ECvsBase::NotifyDelete (EModel *Deleting) {
    for (int i=0;i<LineCount;i++) {
        if (Lines[i]->Buf==Deleting) {
            Lines[i]->Buf=0;
        }
    }
}

int ECvsBase::GetLine(char *Line, size_t max) {
    int rc;
    char *p;
    int l;

    //fprintf(stderr, "GetLine: %d\n", Running);

    *Line = 0;
    if (Running && PipeId != -1) {
        rc = gui->ReadPipe(PipeId, MsgBuf + BufLen, sizeof(MsgBuf) - BufLen);
        //fprintf(stderr, "GetLine: ReadPipe rc = %d\n", rc);
        if (rc == -1) {
            ContinuePipe ();
        }
        if (rc > 0)
            BufLen += rc;
    }
    l = max - 1;
    if (BufLen - BufPos < l)
        l = BufLen - BufPos;
    //fprintf(stderr, "GetLine: Data %d\n", l);
    p = (char *)memchr(MsgBuf + BufPos, '\n', l);
    if (p) {
        *p = 0;
        strcpy(Line, MsgBuf + BufPos);
        l = strlen(Line);
        if (l > 0 && Line[l - 1] == '\r')
            Line[l - 1] = 0;
        BufPos = p + 1 - MsgBuf;
        //fprintf(stderr, "GetLine: Line %d\n", strlen(Line));
    } else if (Running && sizeof(MsgBuf) != BufLen) {
        memmove(MsgBuf, MsgBuf + BufPos, BufLen - BufPos);
        BufLen -= BufPos;
        BufPos = 0;
        //fprintf(stderr, "GetLine: Line Incomplete\n");
        return 0;
    } else {
        if (l == 0)
            return 0;
        memcpy(Line, MsgBuf + BufPos, l);
        Line[l] = 0;
        if (l > 0 && Line[l - 1] == '\r')
            Line[l - 1] = 0;
        BufPos += l;
        //fprintf(stderr, "GetLine: Line Last %d\n", l);
    }
    memmove(MsgBuf, MsgBuf + BufPos, BufLen - BufPos);
    BufLen -= BufPos;
    BufPos = 0;
    //fprintf(stderr, "GetLine: Got Line\n");
    return 1;
}

void ECvsBase::ParseLine(const char *line, size_t) {
    AddLine (0,-1,line);
}

void ECvsBase::NotifyPipe (int APipeId) {
    if (APipeId==PipeId) {
        char line[1024];
        RxMatchRes RM;
        int i;

        while (GetLine(line, sizeof(line))) {
            size_t len = strlen(line);
            if (len>0&&line[len-1]=='\n') line[--len]=0;
            for (i=0;i<CvsIgnoreRegexpCount;i++)
                if (RxExec (CvsIgnoreRegexp[i],line,len,line,&RM)==1) break;
            if (i==CvsIgnoreRegexpCount) ParseLine (line,len);
        }
        if (!Running) {
            char s[30];

            sprintf (s,"[done, status=%d]",ReturnCode);
            AddLine (0,-1,s);
        }
    }
}

int ECvsBase::RunPipe(const char *ADir, const char *ACommand, const char *AOnFiles) {
    free (Command);
    free (Directory);
    free (OnFiles);

    Command=strdup (ACommand);
    Directory=strdup (ADir);
    OnFiles=strdup (AOnFiles);

    ReturnCode=-1;
    Row=LineCount-1;
    OnFilesPos=OnFiles;

    {
        char s[2*MAXPATH*4];

        sprintf (s,"[running cvs in '%s']",Directory);
        AddLine (0,-1,s);
    }

    ChangeDir (Directory);
    return ContinuePipe ();
}

int ECvsBase::ContinuePipe () {
    char RealCommand[2048];
    size_t space;

    if (!OnFilesPos) {
        // At the end of all files, terminate
        ClosePipe ();
        return 0;
    } else if (Running) {
        // Already running, close the pipe and continue
        ReturnCode=gui->ClosePipe (PipeId);
    } else {
        // Not running -> set to Running mode
        Running=1;
    }

    // Make real command with some files from OnFiles, update OnFilesPos
    strcat (strcpy (RealCommand,Command)," ");
    space=sizeof (RealCommand)-strlen (RealCommand)-1;
    if (space>=strlen (OnFilesPos)) {
        strcat (RealCommand,OnFilesPos);
        OnFilesPos=NULL;
    } else {
        char c=OnFilesPos[space];
        OnFilesPos[space]=0;
        char *s=strrchr (OnFilesPos,' ');
        OnFilesPos[space]=c;
        if (!s) {
            ClosePipe ();
            return 0;
        }
        *s=0;
        strcat (RealCommand,OnFilesPos);
        OnFilesPos=s+1;
        while (*OnFilesPos==' ') OnFilesPos++;
        if (!*OnFilesPos) OnFilesPos=NULL;
    }

    BufLen=BufPos=0;

    {
        char s[sizeof (RealCommand)+32];

        sprintf (s,"[continuing: '%s']",RealCommand);
        AddLine (0,-1,s);
    }

    PipeId=gui->OpenPipe (RealCommand,this);
    return 0;
}

void ECvsBase::ClosePipe () {
    ReturnCode = gui->ClosePipe(PipeId);
    PipeId = -1;
    Running = 0;
}

void ECvsBase::DrawLine (PCell B,int Line,int Col,ChColor color,int Width) {
    if (Line<LineCount)
        if (Col<(int)strlen (Lines[Line]->Msg)) {
            char str[1024];
            int len;

            len=UnTabStr (str,sizeof (str),Lines[Line]->Msg,strlen (Lines[Line]->Msg));
            if (len>Col) MoveStr (B,0,Width,str+Col,color,Width);
        }
}

char *ECvsBase::FormatLine (int Line) {
    if (Line<LineCount) return strdup (Lines[Line]->Msg);else return 0;
}

void ECvsBase::UpdateList () {
    if (LineCount<=Row||Row>=Count-1) Row=LineCount-1;
    Count=LineCount;
    EList::UpdateList ();
}

int ECvsBase::Activate (int No) {
    ShowLine (View,No);
    return 1;
}

int ECvsBase::CanActivate(int Line) {
    return Line<LineCount&&Lines[Line]->File;
}

int ECvsBase::IsHilited (int Line) {
    return Line<LineCount&&(Lines[Line]->Status&1);
}

int ECvsBase::IsMarked (int Line) {
    return Line<LineCount&&(Lines[Line]->Status&2);
}

int ECvsBase::Mark (int Line) {
    if (Line<LineCount) {
        if (Lines[Line]->Status&4) Lines[Line]->Status|=2;
        return 1;
    } else return 0;
}

int ECvsBase::Unmark (int Line) {
    if (Line<LineCount) {
        if (Lines[Line]->Status&4) Lines[Line]->Status&=~2;
        return 1;
    } else return 0;
}

int ECvsBase::ExecCommand(ExCommands Command, ExState &State) {
    switch (Command) {
    case ExChildClose:
        if (Running == 0 || PipeId == -1)
            break;
        ClosePipe ();
        {
            char s[30];

            sprintf(s, "[aborted, status=%d]", ReturnCode);
            AddLine(0, -1, s);
        }
        return 1;

    case ExActivateInOtherWindow:
        ShowLine(View->Next, Row);
        return 1;
    default:
	;
    }
    return EList::ExecCommand(Command, State);
}

void ECvsBase::ShowLine (EView *V,int line) {
    if (line>=0&&line<LineCount&&Lines[line]->File) {
        if (Lines[line]->Buf!=0) {
            V->SwitchToModel (Lines[line]->Buf);
            if (Lines[line]->Line!=-1) {
                char book[16];
                sprintf(book,"_CVS.%d",line);
                Lines[line]->Buf->GotoBookmark (book);
            }
        } else {
            char path[MAXPATH];
            strcpy (path,Directory);Slash (path,1);strcat (path,Lines[line]->File);
            if (FileLoad (0,path,0,V)==1) {
                V->SwitchToModel (ActiveModel);
                if (Lines[line]->Line!=-1) ((EBuffer *)ActiveModel)->CenterNearPosR (0,Lines[line]->Line);
            }
        }
    }
}

// Event map - this name is used in config files when defining eventmap
EEventMap *ECvsBase::GetEventMap () {
    return FindEventMap ("CVSBASE");
}

// Shown in "Closing xxx..." message when closing model
void ECvsBase::GetName(char *AName, size_t MaxLen) {
    strlcpy(AName, Title, MaxLen);
}

// Shown in buffer list
void ECvsBase::GetInfo (char *AInfo, size_t MaxLen) {
    char format[128];

    sprintf(format, "%2d %04d/%03d %s (%%.%is) ",
	    ModelNo, Row, Count, Title, (int)(MaxLen - 24 - strlen(Title)));
    snprintf(AInfo, MaxLen, format, Command);
}

// Used to get default directory of model
void ECvsBase::GetPath(char *APath, size_t MaxLen) {
    strlcpy(APath, Directory, MaxLen);
    Slash(APath, 0);
}

// Normal and short title (normal for window, short for icon in X)
void ECvsBase::GetTitle(char *ATitle, size_t MaxLen, char *ASTitle, size_t SMaxLen) {
    char format[128];

    sprintf(format,"%s: %%.%is", Title, (int)(MaxLen - 4 - strlen(Title)));
    sprintf(ATitle, format, Command);
    strlcpy(ASTitle, Title, SMaxLen);
}

#endif
