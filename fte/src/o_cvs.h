/*
 * o_cvs.h
 *
 * Contributed by Martin Frydl <frydl@matfyz.cz>
 *
 * Class providing access to most of CVS commands.
 */

#ifndef O_CVS_H
#define O_CVS_H

#include "fte.h"

#ifdef CONFIG_OBJ_CVS

#include "o_cvsbase.h"

class ECvs:public ECvsBase {
    public:
        char *LogFile;
        int Commiting;

        ECvs (int createFlags,EModel **ARoot,char *Dir,char *ACommand,char *AOnFiles);
        ECvs (int createFlags,EModel **ARoot);
        ~ECvs ();

        void RemoveLogFile ();
        // Return marked files in allocated space separated list
        char *MarkedAsList ();
        // Return CVS status char of file or 0 if unknown
        // (if char is lowercase, state was guessed from last command invoked upon file)
        char GetFileStatus(const char *file);

        virtual void ParseLine (char *line,int len);
        // Returns 0 if OK
        virtual int RunPipe(const char *Dir, const char *Command, const char *OnFiles);
        virtual void ClosePipe ();
        // Start commit process (opens message buffer), returns 0 if OK
        int RunCommit(const char *Dir, const char *Command, const char *OnFiles);
        // Finish commit process (called on message buffer close), returns 0 if OK
        int DoneCommit (int commit);

        virtual int CanQuit ();
        virtual int ConfQuit (GxView *V,int multiFile);

        virtual int GetContext () {return CONTEXT_CVS;}
        virtual EEventMap *GetEventMap ();
};

extern ECvs *CvsView;

#endif

#endif // O_CVS_H
