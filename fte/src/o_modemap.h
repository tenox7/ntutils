/*    o_modemap.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef O_MODEMAP_H
#define O_MODEMAP_H

#include "o_list.h"
#include "c_bind.h"

class EventMapView: public EList {
public:
    char **BList;
    int BCount;
    EEventMap *EMap;
    
    void AddLine(const char *Line);
    void DumpKey(const char *aPrefix, EKey *Key);
    void DumpMap(const char *aPrefix, EKeyMap *aKeyMap);
    void DumpEventMap(EEventMap *aEventMap);
    
    EventMapView(int createFlags, EModel **ARoot, EEventMap *Map);
    virtual ~EventMapView();

    void FreeView();
    void ViewMap(EEventMap *Map);

    virtual int ExecCommand(ExCommands Command, ExState &State);
    virtual EEventMap *GetEventMap();
    virtual int GetContext();

    virtual void DrawLine(PCell B, int Line, int Col, ChColor color, int Width);
    virtual char* FormatLine(int Line);
    virtual void UpdateList();
    virtual int CanActivate(int Line);
    virtual void GetName(char *AName, size_t MaxLen);
    virtual void GetInfo(char *AInfo, size_t MaxLen);
    virtual void GetTitle(char *ATitle, size_t MaxLen, char *ASTitle, size_t SMaxLen);
};

extern EventMapView *TheEventMapView;

#endif // O_MODEMAP_H
