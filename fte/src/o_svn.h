/*
 * o_svn.h
 *
 * S.Pinigin copy o_cvs.h and replace cvs/Cvs/CVS to svn/Svn/SVN.
 *
 * Class providing access to most of SVN commands.
 */

#ifndef O_SVN_H
#define O_SVN_H

#include "o_svnbase.h"

#ifdef CONFIG_OBJ_SVN

class ESvn:public ESvnBase {
    public:
        char *LogFile;
        int Commiting;

        ESvn (int createFlags,EModel **ARoot,char *Dir,char *ACommand,char *AOnFiles);
        ESvn (int createFlags,EModel **ARoot);
        ~ESvn ();

        void RemoveLogFile ();
        // Return marked files in allocated space separated list
        char *MarkedAsList ();
        // Return SVN status char of file or 0 if unknown
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

        virtual int GetContext () {return CONTEXT_SVN;}
        virtual EEventMap *GetEventMap ();
};

extern ESvn *SvnView;

#endif // CONFIG_OBJ_SVN

#endif // O_SVN_H
