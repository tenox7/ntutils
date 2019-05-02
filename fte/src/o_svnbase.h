/*
 * o_svnbase.h
 *
 * S.Pinigin copy o_cvsbase.h and replace cvs/Cvs/CVS to svn/Svn/SVN.
 *
 * Base class for all other SVN-related classes. This is similar to EMessages
 * - starts SVN and shows its messages in list view.
 */

#ifndef O_SVNBASE_H
#define O_SVNBASE_H

#include "fte.h"

#ifdef CONFIG_OBJ_SVN

#include "o_list.h"

class EBuffer;

struct SvnLine {
    char *File; // Relative to view's directory
    int Line;
    char *Msg;
    EBuffer *Buf;
    char Status;
    // bit 0 - hilited
    // bit 1 - marked
    // bit 2 - markable
};

class ESvnBase:public EList {
    public:
        char *Command;
        char *Directory;
        char *OnFiles;
        char *OnFilesPos;

        int LineCount;
        SvnLine **Lines;
        int Running;

        int BufLen;
        int BufPos;
        int PipeId;
        int ReturnCode;
        char MsgBuf[4096];

        ESvnBase (int createFlags,EModel **ARoot,const char *ATitle);
        ~ESvnBase ();

        void FreeLines ();
        void AddLine(const char *file, int line, const char *msg, int hilit = 0);
        void FindBuffer (int line);
        void AssignBuffer (EBuffer *B,int line);
        void FindFileLines (EBuffer *B);
        virtual void NotifyDelete (EModel *Deleting);

        int GetLine(char *Line, size_t max);
        virtual void ParseLine (char *line, size_t len);
        void NotifyPipe (int APipeId);
        // Returns 0 if OK - calls ContinuePipe() several times to complete command for all files
        virtual int RunPipe(const char *Dir, const char *Command, const char *OnFiles);
        // Returns 0 if OK - continue with next files in queue
        virtual int ContinuePipe ();
        // Reads ReturnCode, sets Running to 0, PipeId to -1
        virtual void ClosePipe ();

        void DrawLine (PCell B,int Line,int Col,ChColor color,int Width);
        char *FormatLine (int Line);
        void UpdateList ();
        int Activate (int No);
        int CanActivate (int Line);
        virtual int IsHilited (int Line);
        virtual int IsMarked (int Line);
        virtual int Mark (int Line);
        virtual int Unmark (int Line);

        virtual int ExecCommand(ExCommands Command, ExState &State);
        void ShowLine (EView *V,int err);

        virtual int GetContext () {return CONTEXT_SVNBASE;}
        virtual EEventMap *GetEventMap ();
        virtual void GetName(char *AName, size_t MaxLen);
        virtual void GetInfo(char *AInfo, size_t MaxLen);
        virtual void GetPath(char *APath, size_t MaxLen);
        virtual void GetTitle(char *ATitle, size_t MaxLen, char *ASTitle, size_t SMaxLen);
};

int AddSvnIgnoreRegexp (const char *);
void FreeSvnIgnoreRegexp ();

#endif // CONFIG_OBJ_SVN

#endif // O_SVNBASE_H
