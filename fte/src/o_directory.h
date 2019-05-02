/*    o_directory.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */
#ifndef O_DIRECTORY_H
#define O_DIRECTORY_H

#include "fte.h"

#ifdef CONFIG_OBJ_DIRECTORY

#include "i_search.h"
#include "o_list.h"
#include "s_direct.h"

class EDirectory: public EList {
public:
    StlString Path;
    FileInfo **Files;
    int FCount;
    int SearchLen;
    char SearchName[ExISearch::MAXISEARCH];
    int SearchPos[ExISearch::MAXISEARCH];
    
    EDirectory(int createFlags, EModel **ARoot, char *aPath);
    virtual ~EDirectory();
    
    virtual int GetContext();
    virtual EEventMap *GetEventMap();
    virtual int ExecCommand(ExCommands Command, ExState &State);
    virtual void HandleEvent(TEvent &Event);
    
    virtual void DrawLine(PCell B, int Line, int Col, ChColor color, int Width);
    virtual char *FormatLine(int Line);
    virtual int IsHilited(int Line);

    virtual void RescanList();
//    virtual void UpdateList();
    virtual void FreeList();
    virtual int CanActivate(int Line);
    virtual int Activate(int No);
    
    virtual void GetName(char *AName, size_t MaxLen);
    virtual void GetPath(char *APath, size_t MaxLen);
    virtual void GetInfo(char *AInfo, size_t MaxLen);
    virtual void GetTitle(char *ATitle, size_t MaxLen, char *ASTitle, size_t SMaxLen);
    
    int isDir(int No);
    int FmChDir(const char* Name);
    int FmLoad(const char* Name, EView *View);
    int FmRmDir(const char* Name);
    int ChangeDir(ExState &State);
    int RescanDir();
};
#endif // CONFIG_OBJ_DIRECTORY

#endif // O_DIRECTORY_H
