/*    o_modemap.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "o_modemap.h"

#include "s_string.h"
#include "sysdep.h"

#include <stdio.h>

EventMapView *TheEventMapView = 0;

void EventMapView::AddLine(const char *Line) {
    if (BList) {
        BCount++;
        BList = (char **)realloc((void *)BList, sizeof(char *) * BCount);
    } else {
        BCount = 1;
        BList = (char **)malloc(sizeof(char *));
    }
    assert(BList);
    BList[BCount - 1] = strdup(Line);
}

void EventMapView::DumpKey(const char *aPrefix, EKey *Key) {
    char KeyName[128] = "";
    char Entry[2048] = "";
    char *p;
    int id;
    
    if (aPrefix) {
        strcpy(KeyName, aPrefix);
        strcat(KeyName, "_");
    }
    GetKeyName(KeyName + strlen(KeyName), sizeof(KeyName)-strlen(KeyName), Key->fKey);
    sprintf(Entry, "%13s   ", KeyName);
    id = Key->Cmd;
    for (unsigned i = 0; i < Macros[id].Count; ++i) {
        p = Entry + strlen(Entry);
        if (Macros[id].cmds[i].type == CT_COMMAND) {
            if (Macros[id].cmds[i].repeat > 1)
                sprintf(p, "%d:%s ", Macros[id].cmds[i].repeat, GetCommandName((int)Macros[id].cmds[i].u.num));
            else
                sprintf(p, "%s ", GetCommandName((int)Macros[id].cmds[i].u.num));
        } else if (Macros[id].cmds[i].type == CT_NUMBER) {
            sprintf(p, "%ld ", Macros[id].cmds[i].u.num);
        } else if (Macros[id].cmds[i].type == CT_STRING) {
            sprintf(p, "'%s' ", Macros[id].cmds[i].u.string);
        } else if (Macros[id].cmds[i].type == CT_CONCAT) {
            strcat(p, ". ");
        } else if (Macros[id].cmds[i].type == CT_VARIABLE) {
            sprintf(p, "$(%ld) ", Macros[id].cmds[i].u.num);
        }
        if (strlen(Entry) > 70) {
            if ((i + 1) != Macros[id].Count) {
                // not the last entry
                AddLine(Entry);
                sprintf(Entry, "%13s   ", "");
            }
        }
    }
    AddLine(Entry);
}
    
void EventMapView::DumpMap(const char *aPrefix, EKeyMap *aKeyMap) {
    EKey *Key;
    
    Key = aKeyMap->fKeys;
    while (Key) {
        if (Key->fKeyMap) {
            char Prefix[32] = "";
            
            if (aPrefix) {
                strcpy(Prefix, aPrefix);
                strcat(Prefix, "_");
            }
            GetKeyName(Prefix + strlen(Prefix), sizeof(Prefix)-strlen(Prefix), Key->fKey);
            DumpMap(Prefix, Key->fKeyMap);
        } else {
            DumpKey(aPrefix, Key);
        }
        Key = Key->fNext;
    }
}
    
void EventMapView::DumpEventMap(EEventMap *aEventMap) {
    while (aEventMap) {
	StlString name(aEventMap->Name);

        if (aEventMap->Parent) {
            name += ": ";
            name += aEventMap->Parent->Name;
        }

        AddLine(name.c_str());

        if (aEventMap->KeyMap)
            DumpMap(0, aEventMap->KeyMap);
        aEventMap = aEventMap->Parent;
        if (aEventMap != 0)
            AddLine("");
    }
}

EventMapView::EventMapView(int createFlags, EModel **ARoot, EEventMap *Map) :
    EList(createFlags, ARoot, "Event Map"),
    BList(0),
    BCount(0)
{
    DumpEventMap(EMap = Map);
    TheEventMapView = this;
}

EventMapView::~EventMapView() {
    FreeView();
    TheEventMapView = 0;
}

void EventMapView::FreeView() {
    if (BList) {
        for (int i = 0; i < BCount; i++)
            if (BList[i])
                free(BList[i]);
        free(BList);
    }
    BCount = 0;
    BList = 0;
}

void EventMapView::ViewMap(EEventMap *Map) {
    FreeView();
    DumpEventMap(EMap = Map);
}

EEventMap *EventMapView::GetEventMap() {
    return FindEventMap("EVENTMAPVIEW");
}

int EventMapView::ExecCommand(ExCommands Command, ExState &State) {
    return EList::ExecCommand(Command, State);
}

int EventMapView::GetContext() {
    return CONTEXT_MAPVIEW;
}

void EventMapView::DrawLine(PCell B, int Line, int Col, ChColor color, int Width) {
    if (Line < BCount) 
        if (Col < int(strlen(BList[Line])))
            MoveStr(B, 0, Width, BList[Line] + Col, color, Width);
}

char *EventMapView::FormatLine(int Line) {
    return strdup(BList[Line]);
}

void EventMapView::UpdateList() {
    Count = BCount;
    EList::UpdateList();
}

int EventMapView::CanActivate(int /*Line*/) {
    return 0;
}

void EventMapView::GetName(char *AName, size_t MaxLen) {
    strlcpy(AName, "EventMapView", MaxLen);
}

void EventMapView::GetInfo(char *AInfo, size_t MaxLen) {
    snprintf(AInfo, MaxLen, "%2d %04d/%03d EventMapView (%s)",
            ModelNo, Row + 1, Count, EMap->Name);
}

void EventMapView::GetTitle(char *ATitle, size_t MaxLen, char *ASTitle, size_t SMaxLen) {
    snprintf(ATitle, MaxLen, "EventMapView: %s", EMap->Name);
    strlcpy(ASTitle, "EventMapView", SMaxLen);
}
