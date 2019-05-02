/*
 * o_cvsdiff.cpp
 *
 * Contributed by Martin Frydl <frydl@matfyz.cz>
 *
 * Class showing output from CVS diff command. Allows copying of lines
 * to clipboard and allows to jump to lines in real sources.
 */

#include "o_cvsdiff.h"

#ifdef CONFIG_OBJ_CVS

#include "c_bind.h"
#include "c_config.h"
#include "e_buffer.h"
#include "s_util.h"
#include "sysdep.h"

ECvsDiff *CvsDiffView=0;

ECvsDiff::ECvsDiff (int createFlags, EModel **ARoot, char *ADir, char *ACommand, char *AOnFiles) :
    ECvsBase (createFlags, ARoot, "CVS diff"),
    CurrLine(0),
    ToLine(0),
    InToFile(0),
    CurrFile(0)
{
    CvsDiffView=this;
    RunPipe (ADir,ACommand,AOnFiles);
}

ECvsDiff::~ECvsDiff () {
    CvsDiffView=0;
    free (CurrFile);
}

void ECvsDiff::ParseFromTo (char *line,int /*len*/) {
    char *end;
    CurrLine=strtol (line+4,&end,10)-1;
    if (*end==',') ToLine=atoi (end+1);else ToLine=CurrLine+1;
    if (!(CurrLine<ToLine&&ToLine>0)) CurrLine=ToLine=0;
}

void ECvsDiff::ParseLine (char *line,int len) {
    if (len>7&&strncmp (line,"Index: ",7)==0) {
        // Filename
        free (CurrFile);CurrFile=strdup (line+7);
        CurrLine=ToLine=InToFile=0;
        AddLine (CurrFile,-1,line);
    } else if (len>8&&strncmp (line,"*** ",4)==0) {
        // From file or from hunk
        if (strcmp (line+len-5," ****")==0) {
            // From hunk
            ParseFromTo (line,len);
        }
        InToFile=0;
        AddLine (0,-1,line);
    } else if (len>8&&strncmp (line,"--- ",4)==0) {
        // To file or to hunk
        if (strcmp (line+len-5," ----")==0) {
            // To hunk
            if (CurrFile) {
                ParseFromTo (line,len);
                AddLine (CurrFile,CurrLine,line,1);
            } else AddLine (0,-1,line);
        } else {
            // To-file
            AddLine (CurrFile,-1,line);
        }
        InToFile=1;
    } else if (strcmp (line,"***************")==0) {
        // Hunk start
        CurrLine=ToLine=0;
        AddLine (0,-1,line);
    } else if (CurrLine<ToLine) {
        // Diff line (markable, if CurrFile is set, also hilited)
        if (InToFile) AddLine (CurrFile,CurrLine,line,5);
        else AddLine (0,CurrLine,line,4);
        CurrLine++;
    } else AddLine (0,-1,line);
}

int ECvsDiff::RunPipe(const char *ADir, const char *ACommand, const char *AOnFiles) {
    FreeLines ();
    free (CurrFile);
    CurrLine=ToLine=InToFile=0;
    CurrFile=0;
    return ECvsBase::RunPipe (ADir,ACommand,AOnFiles);
}

int ECvsDiff::ExecCommand(ExCommands Command, ExState &State) {
    switch (Command) {
    case ExBlockCopy:
        return BlockCopy(0);
    case ExBlockCopyAppend:
        return BlockCopy(1);
    default:
        ;
    }
    return EList::ExecCommand(Command, State);
}

int ECvsDiff::BlockCopy (int Append) {
    if (SSBuffer==0) return 0;
    if (Append) {
        if (SystemClipboard) GetPMClip(0);
    } else SSBuffer->Clear ();
    SSBuffer->BlockMode=bmLine;
    // How to set these two ?
    BFI (SSBuffer,BFI_TabSize)=8;
    BFI (SSBuffer,BFI_ExpandTabs)=0;
    BFI (SSBuffer,BFI_Undo)=0;
    // Go through list of marked lines
    int last=-1,tl=0;
    for (int i=0;i<LineCount;i++) {
        if (Lines[i]->Status&2) {
            // Marked
            if (last!=i-1&&tl) {
                // Gap between this and last marked line
                SSBuffer->InsLine (tl++,0);
            }
            SSBuffer->InsertLine (tl++,strlen (Lines[i]->Msg+2),Lines[i]->Msg+2);
            last=i;
        }
    }
    if (SystemClipboard) PutPMClip (0);
    return 1;
}

// Event map - this name is used in config files when defining eventmap
EEventMap *ECvsDiff::GetEventMap () {
    return FindEventMap ("CVSDIFF");
}

#endif
