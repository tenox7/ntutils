/*
 * o_cvsdiff.h
 *
 * Contributed by Martin Frydl <frydl@matfyz.cz>
 *
 * Class showing output from CVS diff command. Allows copying of lines
 * to clipboard and allows to jump to lines in real sources.
 */

#ifndef O_CVSDIFF_H
#define O_CVSDIFF_H

#include "fte.h"

#ifdef CONFIG_OBJ_CVS

#include "o_cvsbase.h"

class ECvsDiff: public ECvsBase {
    public:
        int CurrLine,ToLine,InToFile;
        char *CurrFile;

        ECvsDiff (int createFlags,EModel **ARoot,char *Dir,char *ACommand,char *AOnFiles);
        ~ECvsDiff ();

        void ParseFromTo (char *line,int len);
        virtual void ParseLine (char *line,int len);
        // Returns 0 if OK
        virtual int RunPipe(const char *Dir, const char *Command, const char *Info);

        virtual int ExecCommand(ExCommands Command, ExState &State);
        int BlockCopy (int Append);

        virtual int GetContext () {return CONTEXT_CVSDIFF;}
        virtual EEventMap *GetEventMap ();
};

extern ECvsDiff *CvsDiffView;

#endif

#endif // O_CVSDIFF_H
