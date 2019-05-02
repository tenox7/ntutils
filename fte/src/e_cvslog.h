/*
 * e_cvslog.h
 *
 * Contributed by Martin Frydl <frydl@matfyz.cz>
 *
 * Subclass of EBuffer for writing log for CVS commit. Creates temporary file
 * used for commit which is deleted when view is closed. Asks for commit or
 * discard on view close.
 */

#ifndef E_CVSLOG_H
#define E_CVSLOG_H

#include "e_buffer.h"

#ifdef CONFIG_OBJ_CVS

#include "c_config.h"

class ECvsLog:public EBuffer {
    public:
        ECvsLog (int createFlags,EModel **ARoot,char *Directory,char *OnFiles);
        ~ECvsLog ();

        // List files into buffer
        // p        - line where to print
        // fCount   - number of files which will be printed
        // title    - title
        // cnt      - total number of files
        // position - positions of files in list
        // len      - length of files
        // status   - status of files
        // list     - list of filenames
        // incexc   - status of files to print/not to print
        // exc      - incexc is exclusion
	void ListFiles(int &p, const int fCount, const char *title, int cnt,
		       const int *position, const int *len, const char *status,
		       const char *list, const char *excinc, const int exc = 0);

        virtual int CanQuit ();
        virtual int ConfQuit (GxView *V,int multiFile=0);
        virtual EViewPort *CreateViewPort (EView *V);

        virtual void GetName(char *AName, size_t MaxLen);
        virtual void GetInfo(char *AInfo, size_t MaxLen);
        virtual void GetTitle(char *ATitle, size_t MaxLen, char *ASTitle, size_t SMaxLen);
};

extern ECvsLog *CvsLogView;

#endif // CONFIG_OBJ_CVS

#endif // E_CVSLOG_H
