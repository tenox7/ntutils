/*
 * o_svndiff.h
 *
 * S.Pinigin copy o_cvsdiff.h and replace cvs/Cvs/CVS to svn/Svn/SVN.
 *
 * Class showing output from SVN diff command. Allows copying of lines
 * to clipboard and allows to jump to lines in real sources.
 */

#ifndef O_SVNDIFF_H
#define O_SVNDIFF_H

#include "fte.h"

#ifdef CONFIG_OBJ_SVN

#include "o_svnbase.h"

class ESvnDiff:public ESvnBase {
    public:
        int CurrLine,ToLine,InToFile;
        char *CurrFile;

        ESvnDiff (int createFlags,EModel **ARoot,char *Dir,char *ACommand,char *AOnFiles);
        ~ESvnDiff ();

        void ParseFromTo (char *line,int len);
        virtual void ParseLine (char *line,int len);
        // Returns 0 if OK
        virtual int RunPipe (const char *Dir, const char *Command, const char *Info);

        virtual int ExecCommand(ExCommands Command, ExState &State);
        int BlockCopy (int Append);

        virtual int GetContext () {return CONTEXT_SVNDIFF;}
        virtual EEventMap *GetEventMap ();
};

extern ESvnDiff *SvnDiffView;

#endif // CONFIG_OBJ_SVN

#endif // O_SVNDIFF_H
