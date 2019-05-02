/*    o_routine.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef O_ROUTINE_H
#define O_ROUTINE_H

#include "fte.h"

#ifdef CONFIG_OBJ_ROUTINE

#include "o_list.h"

class EBuffer;

class RoutineView: public EList {
public:
    EBuffer *Buffer;
    
    RoutineView(int createFlags, EModel **ARoot, EBuffer *AB);
    virtual ~RoutineView();
    virtual EEventMap *GetEventMap();
    virtual int ExecCommand(ExCommands Command, ExState &State);
    virtual void DrawLine(PCell B, int Line, int Col, ChColor color, int Width);
    virtual char* FormatLine(int Line);
    virtual int Activate(int No);
    virtual void RescanList();
    void UpdateList();
    virtual int GetContext();
    virtual void GetName(char *AName, size_t MaxLen);
    virtual void GetInfo(char *AInfo, size_t MaxLen);
    virtual void GetTitle(char *ATitle, size_t MaxLen, char *ASTitle, size_t SMaxLen);
};
#endif

#endif // O_ROUTINE_H
