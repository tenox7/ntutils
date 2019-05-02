/*    o_buflist.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef O_BUFLIST_H
#define O_BUFLIST_H

#include "o_list.h"
#include "i_search.h"

class BufferView: public EList {
public:
    char **BList;
    int BCount;
    int SearchLen;
    char SearchString[ExISearch::MAXISEARCH];
    int SearchPos[ExISearch::MAXISEARCH];

    BufferView(int createFlags, EModel **ARoot);
    virtual ~BufferView();
    virtual EEventMap *GetEventMap();
    virtual int GetContext();
    virtual void DrawLine(PCell B, int Line, int Col, ChColor color, int Width);
    virtual char* FormatLine(int Line);
    virtual void UpdateList();
    EModel *GetBufferById(int No);
    virtual int ExecCommand(ExCommands Command, ExState &State);
    virtual void HandleEvent(TEvent &Event);
    int getMatchingLine (int start, int direction);
    virtual int Activate(int No);
    void CancelSearch();
    virtual void GetInfo(char *AInfo, size_t MaxLen);
    virtual void GetTitle(char *ATitle, size_t MaxLen, char *ASTitle, size_t SMaxLen);
};

#endif // O_BUFLIST_H
