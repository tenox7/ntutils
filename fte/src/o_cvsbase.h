/*
 * o_cvsbase.h
 *
 * Contributed by Martin Frydl <frydl@matfyz.cz>
 *
 * Base class for all other CVS-related classes. This is similar to EMessages
 * - starts CVS and shows its messages in list view.
 */

#ifndef O_CVSBASE_H
#define O_CVSBASE_H

#include "fte.h"

#ifdef CONFIG_OBJ_CVS

#include "o_list.h"

class EBuffer;

struct CvsLine {
    char *File; // Relative to view's directory
    int Line;
    char *Msg;
    EBuffer *Buf;
    char Status;
    // bit 0 - hilited
    // bit 1 - marked
    // bit 2 - markable
};

class ECvsBase:public EList {
    public:
        char *Command;
        char *Directory;
        char *OnFiles;
        char *OnFilesPos;

        int LineCount;
        CvsLine **Lines;
        int Running;

        size_t BufLen;
        int BufPos;
        int PipeId;
        int ReturnCode;
        char MsgBuf[4096];

        ECvsBase (int createFlags,EModel **ARoot,const char *ATitle);
        ~ECvsBase ();

        void FreeLines ();
        void AddLine (const char *file, int line, const char *msg, int hilit = 0);
        void FindBuffer (int line);
        void AssignBuffer (EBuffer *B,int line);
        void FindFileLines (EBuffer *B);
        virtual void NotifyDelete (EModel *Deleting);

        int GetLine (char *Line, size_t max);
        virtual void ParseLine (const char *line, size_t len);
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

        virtual int GetContext () {return CONTEXT_CVSBASE;}
        virtual EEventMap *GetEventMap ();
        virtual void GetName(char *AName, size_t MaxLen);
        virtual void GetInfo(char *AInfo, size_t MaxLen);
        virtual void GetPath(char *APath, size_t MaxLen);
        virtual void GetTitle(char *ATitle, size_t MaxLen, char *ASTitle, size_t SMaxLen);
};

int AddCvsIgnoreRegexp (const char *);
void FreeCvsIgnoreRegexp ();

#endif // CONFIG_OBJ_CVS

#endif // O_CVSBASE_H
