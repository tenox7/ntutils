/*    c_bind.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef C_BIND_H
#define C_BIND_H

#include "c_mode.h" // EM_MENUS
#include "console.h"
#include "e_regex.h"
#include "o_model.h"

#define ABBREV_HASH      16

class EMode;
class EEventMap;
class EKeyMap;
class EKey;
class EAbbrev;
class EView;
class EColorize;

struct KeySel {
    TKeyCode Mask;
    TKeyCode Key;
};

class EMode {
public:
    EMode *fNext;
    char *fName;
    char *MatchName;
    char *MatchLine;
    RxNode *MatchNameRx;
    RxNode *MatchLineRx;
    EBufferFlags Flags;
    EEventMap *fEventMap;
    EMode *fParent;
#ifdef CONFIG_SYNTAX_HILIT
    EColorize *fColorize;
#endif
    char filename[256];
    
    EMode(EMode *aMode, EEventMap *Map, const char *aName);
    ~EMode();
#ifdef CONFIG_ABBREV
    EAbbrev *FindAbbrev(const char *string);
#endif
};

class EKeyMap {
public:
    EKeyMap *fParent;
    EKey *fKeys;
    
    EKeyMap();
    ~EKeyMap();
    
    void AddKey(EKey *aKey);
    EKey *FindKey(TKeyCode aKey);
};

class EEventMap {
public:
    EEventMap *Next;
    EEventMap *Parent;
    char *Name;
    
    EKeyMap *KeyMap;
    char *Menu[EM_MENUS]; // main + local
    
    EAbbrev *abbrev[ABBREV_HASH];
    
    EEventMap(const char *AName, EEventMap *AParent);
    ~EEventMap();
    void SetMenu(int which, const char *What);
    char *GetMenu(int which);
#ifdef CONFIG_ABBREV
    int AddAbbrev(EAbbrev *ab);
#endif
};

enum CommandType_e {
    CT_COMMAND,
    CT_NUMBER,
    CT_STRING,
    CT_VARIABLE,
    CT_CONCAT /* concatenate strings */
};

struct CommandType {
    CommandType_e type;
    short repeat;
    short ign;
    union {
        long num;
        char *string;
    } u;
};

struct ExMacro {
    char *Name;
    unsigned Count;
    CommandType *cmds;
};

class EKey {
public:
    KeySel fKey;
    int Cmd;
    EKeyMap *fKeyMap;
    EKey *fNext;
    
    EKey(const char *aKey);
    EKey(const char *aKey, EKeyMap *aKeyMap);
    ~EKey();
};

#ifdef CONFIG_ABBREV
class EAbbrev {
public:
    EAbbrev *next;
    int Cmd;
    char *Match;
    char *Replace;
    
    EAbbrev(const char *aMatch, const char *aReplace);
    EAbbrev(const char *aMatch, int aCmd);
    ~EAbbrev();
};
#endif

class ExState { // state of macro execution
public:
    int Macro;
    int Pos;
    
    int GetStrParam(EView *view, char *str, size_t buflen);
    int GetIntParam(EView *view, int *value);
};

extern EMode *Modes;
extern EEventMap *EventMaps;

extern int CMacros;
extern ExMacro *Macros;

int GetCharFromEvent(TEvent &E, char *Ch);

const char *GetCommandName(int Command);
EMode *FindMode(const char *Name);
EEventMap *FindEventMap(const char *Name);
EEventMap *FindActiveMap(EMode *Mode);
EMode *GetModeForName(const char *FileName);
int CmdNum(const char *Cmd);
void ExecKey(EKey *Key);
EKey *SetKey(EEventMap *aMap, const char *Key);
int GetKeyName(char *Key, size_t KeySize, KeySel &ks);

int NewCommand(const char *Name);
int RunCommand(int Command);
int AddCommand(int no, int cmd, int count, int ign);
int AddString(int no, const char *Command);
int AddNumber(int no, long number);
int AddVariable(int no, int number);
int AddConcat(int no);
int HashStr(const char *str, int maxim);
void SetWordChars(char *w, const char *s);

#endif // C_BIND_H
