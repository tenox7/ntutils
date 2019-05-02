/*    c_bind.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "c_bind.h"
#include "s_string.h"
#include "sysdep.h"

#include <ctype.h>

//////////////////////////////////////////////////////////////////////////////

EMode *Modes = 0;
EEventMap *EventMaps = 0;

int CMacros = 0;
ExMacro *Macros = 0;

//////////////////////////////////////////////////////////////////////////////

#include "c_cmdtab.h"

//////////////////////////////////////////////////////////////////////////////

static const struct {
    const char Name[8];
    TKeyCode Key;
} KeyList[] = {
    { "Esc", kbEsc },
    { "Tab", kbTab },
    { "Space", kbSpace },
    { "Enter", kbEnter },
    { "BackSp", kbBackSp },
    { "F1", kbF1 },
    { "F2", kbF2 },
    { "F3", kbF3 },
    { "F4", kbF4 },
    { "F5", kbF5 },
    { "F6", kbF6 },
    { "F7", kbF7 },
    { "F8", kbF8 },
    { "F9", kbF9 },
    { "F10", kbF10 },
    { "F11", kbF11 },
    { "F12", kbF12 },
    { "Left", kbLeft },
    { "Right", kbRight },
    { "Up", kbUp },
    { "Down", kbDown },
    { "Home", kbHome },
    { "End", kbEnd },
    { "PgUp", kbPgUp },
    { "PgDn", kbPgDn },
    { "Ins", kbIns },
    { "Del", kbDel },
    { "Center", kbCenter },
    { "Break", kbBreak },
    { "Pause", kbPause },
    { "PrtScr", kbPrtScr },
    { "SysReq", kbSysReq },
};

static int ParseKey(const char *Key, KeySel &ks) {
    TKeyCode KeyFlags = 0;

    ks.Mask = 0;
    ks.Key = 0;
    while (Key[0] && ((Key[1] == '+') || (Key[1] == '-'))) {
        if (Key[1] == '-') {
            switch (Key[0]) {
            case 'A': ks.Mask |= kfAlt; break;
            case 'C': ks.Mask |= kfCtrl; break;
            case 'S': ks.Mask |= kfShift; break;
            case 'G': ks.Mask |= kfGray; break;
            case 'X': ks.Mask |= kfSpecial; break;
            }
        } else if (Key[1] == '+') {
            switch (Key[0]) {
            case 'A': KeyFlags |= kfAlt; break;
            case 'C': KeyFlags |= kfCtrl; break;
            case 'S': KeyFlags |= kfShift; break;
            case 'G': KeyFlags |= kfGray; break;
            case 'X': KeyFlags |= kfSpecial; break;
            }
        }
        Key += 2;
    }

    for (size_t i = 0; i < FTE_ARRAY_SIZE(KeyList); ++i)
        if (strcmp(Key, KeyList[i].Name) == 0) {
            ks.Key = KeyList[i].Key;
            break;
        }

    if (ks.Key == 0)
        ks.Key = *Key;
    if ((KeyFlags & kfCtrl) && !(KeyFlags & kfSpecial)) {
        if (ks.Key < 256) {
            if (ks.Key < 32)
                ks.Key += 64;
            else
                ks.Key = toupper(ks.Key);
        }
    }
    ks.Key |= KeyFlags;
    return 0;
}

int GetKeyName(char *Key, size_t KeySize, KeySel &ks) {
    strlcpy(Key, "", KeySize);

    if (ks.Key  & kfAlt)   strlcat(Key, "A+", KeySize);
    if (ks.Mask & kfAlt)   strlcat(Key, "A-", KeySize);
    if (ks.Key  & kfCtrl)  strlcat(Key, "C+", KeySize);
    if (ks.Mask & kfCtrl)  strlcat(Key, "C-", KeySize);
    if (ks.Key  & kfGray)  strlcat(Key, "G+", KeySize);
    if (ks.Mask & kfGray)  strlcat(Key, "G-", KeySize);
    if (ks.Key  & kfShift) strlcat(Key, "S+", KeySize);
    if (ks.Mask & kfShift) strlcat(Key, "S-", KeySize);

    if (keyCode(ks.Key) < 256) {
	char c[2] = { (char)ks.Key, 0 };

        //if (ks.Key & kfCtrl)
        //    if (c[0] < ' ')
        //        c[0] += '@';
        if (c[0] == 32)
            strlcat(Key, "Space", KeySize);
        else
            strlcat(Key, c, KeySize);
    } else {
        for (size_t i = 0; i < FTE_ARRAY_SIZE(KeyList); ++i)
            if (KeyList[i].Key == keyCode(ks.Key)) {
                strlcat(Key, KeyList[i].Name, KeySize);
                break;
            }
    }
    return 0;
}

const char *GetCommandName(int Command) {
    if (Command & CMD_EXT) {
        Command &= ~CMD_EXT;
        if ((Command < 0) ||
            (Command >= CMacros))
            return "?INVALID?";
        if (Macros[Command].Name)
            return Macros[Command].Name;
        else
            return "?NONE?";
    }
    for (size_t i = 0; i < FTE_ARRAY_SIZE(Command_Table); ++i)
        if (Command_Table[i].CmdId == Command)
            return Command_Table[i].Name;
    return "?invalid?";
}

int CmdNum(const char *Cmd) {
    for (size_t i = 0; i < FTE_ARRAY_SIZE(Command_Table); ++i)
        if (strcmp(Cmd, Command_Table[i].Name) == 0)
            return Command_Table[i].CmdId;
    for (int i = 0; i < CMacros; i++)
        if (Macros[i].Name && (strcmp(Cmd, Macros[i].Name)) == 0)
            return i | CMD_EXT;
    return 0; // Nop
}

EMode *FindMode(const char *Name) {
    EMode *m = Modes;

    //fprintf(stderr, "Searching mode %s\n", Name);
    while (m) {
        if (strcmp(Name, m->fName) == 0)
            return m;
        m = m->fNext;
    }
    return 0;
}

EEventMap *FindEventMap(const char *Name) {
    EEventMap *m = EventMaps;

    //fprintf(stderr, "Searching map %s\n", Name);
    while (m) {
        if (strcmp(Name, m->Name) == 0)
            return m;
        m = m->Next;
    }
    return 0;
}

EEventMap *FindActiveMap(EMode *Mode) {
    while (Mode) {
        if (Mode->fEventMap)
            return Mode->fEventMap;
        Mode = Mode->fParent;
    }
    return 0;
}

EKey *SetKey(EEventMap *aMap, const char *aKey) {
    EKeyMap **map = &aMap->KeyMap, *pm, *parent = 0;
    EKey *k;
    char Key[256];
    char *p, *d;
    EEventMap *xm = aMap;

    // printf("Setting key %s\n", Key);
    strcpy(Key, aKey);

    // if mode has parent, get parent keymap
    while (xm && xm->Parent && (parent == 0)) {
        parent = xm->Parent->KeyMap;
        //        printf("%s : %s : %d\n", xm->fName, xm->fParent->fName, parent);
        xm = xm->Parent;
    }

    d = Key;
    while (d) {
        // parse key combination
        p = d;
        d = strchr(p, '_');
        if (d) {
            if (d[1] == 0 || d[1] == '_')
                d++;

            if (*d == 0)
                d = 0;
            else {
                *d = 0;
                d++;
            }
        }

        // if lastkey

        if (d == 0) {
            k = new EKey(p);
            if (*map) {
                (*map)->AddKey(k);
            } else {
                *map = new EKeyMap();
                (*map)->fParent = parent;
                (*map)->AddKey(k);
            }
            return k;

        } else {
            // if introductory key

            if (*map == 0) { // first key in mode, create map
                //                printf("new map key = %s, parent %d\n", p, parent);
                k = new EKey(p, 0);
                *map = new EKeyMap();
                (*map)->fParent = parent;
                (*map)->AddKey(k);
            } else {
                KeySel ks;

                ParseKey(p, ks);
                if ((k = (*map)->FindKey(ks.Key)) == 0) { // check if key exists
                    // add it if not
                    k = new EKey(p, 0);
                    (*map)->AddKey(k);
                }
            }
            map = &k->fKeyMap; // set current map to key's map

            // get parent keymap
            pm = parent;
            parent = 0;
            //            printf("Searching %s\n", p);
            while (pm) { // while exists
                KeySel ks;
                EKey *pk;

                ParseKey(p, ks);
                if ((pk = pm->FindKey(ks.Key)) != 0) { // if key exists, find parent of it
                    parent = pk->fKeyMap;
                    //                    printf("Key found %d\n", parent);
                    break;
                }
                pm = pm->fParent; // otherwise find parent of current keymap
            }
        }
    }
    return 0;
}

static void InitWordChars() {
    static int init = 0;
    if (init == 0) {
	for (int i = 0; i < 256; i++)
            // isalnum???
	    //if (isdigit(i) || isalpha(i)
	    //    || (i >= 'A' && i <= 'Z')
            //    || (i >= 'a' && i <= 'z') || (i == '_')) {
            if (isalnum(i) || (i == '_')) {
                WSETBIT(DefaultBufferFlags.WordChars, i, 1);
                // Can someone tell me why we check A through Z?
                // This won't work should someone port to EBCDIC (why, though?)
                // besides, isupper is usually a #define that will compile to something
                // even faster.
                if (/*(i >= 'A' && i <= 'Z') || */ isupper(i))
                    WSETBIT(DefaultBufferFlags.CapitalChars, i, 1);
            }
        init = 1;
    }
}

void SetWordChars(char *w, const char *s) {
    const char *p = s;
    memset(w, 0, 32);

    while (p && *p) {
        if (*p == '\\') {
            p++;
            if (*p == 0) return;
        } else if (p[1] == '-') {
            if (p[2] == 0) return ;
            for (int i = p[0]; i < p[2]; i++)
                WSETBIT(w, i, 1);
            p += 2;
        }
        WSETBIT(w, *p, 1);
        p++;
    }
}

EMode::EMode(EMode *aMode, EEventMap *Map, const char *aName) :
    fNext(0),
    fName(strdup(aName)),
    fEventMap(Map),
    fParent(aMode)
{
    InitWordChars();
    if (aMode) {
#ifdef CONFIG_SYNTAX_HILIT
        fColorize = aMode->fColorize;
#endif
        Flags = aMode->Flags;

        // duplicate strings in flags to allow them be freed
        for (int i=0; i<BFS_COUNT; i++)
        {
            if (aMode->Flags.str[i] != 0)
                Flags.str[i] = strdup(aMode->Flags.str[i]);
        }

        MatchName = 0;
        MatchLine = 0;
        MatchNameRx = 0;
        MatchLineRx = 0;
        if (aMode->MatchName) {
            MatchName = strdup(aMode->MatchName);
            MatchNameRx = RxCompile(MatchName);
        }
        if (aMode->MatchLine) {
            MatchLine = strdup(aMode->MatchLine);
            MatchLineRx = RxCompile(MatchLine);
        }
    } else {
        MatchName = 0;
        MatchLine = 0;
        MatchNameRx = 0;
        MatchLineRx = 0;
#ifdef CONFIG_SYNTAX_HILIT
        fColorize = 0;
#endif
        Flags = DefaultBufferFlags;

        // there is no strings in default settings...
    }
}

EMode::~EMode() {

    // fEventMap is just pointer to EventMaps list, so do not destroy it
    // fColorize is also just a pointer

    free(fName);

    free(MatchName);
    RxFree(MatchNameRx);

    free(MatchLine);
    RxFree(MatchLineRx);

    // free strings from flags
    for (unsigned i = 0; i < BFS_COUNT; i++)
        free(Flags.str[i]);
}

EKeyMap::EKeyMap() :
    fParent(NULL),
    fKeys(NULL)
{
}

EKeyMap::~EKeyMap() {
    // free keys
    EKey *e;

    while ((e = fKeys) != NULL) {
	fKeys = fKeys->fNext;
	delete e;
    }
}

void EKeyMap::AddKey(EKey *aKey) {
    aKey->fNext = fKeys;
    fKeys = aKey;
}


static int MatchKey(TKeyCode aKey, KeySel aSel) {
    long flags = aKey & ~ 0xFFFF;
    long key = aKey & 0xFFFF;

    flags &= ~kfAltXXX;

    if (flags & kfShift) {
        if (key < 256) {
            if (flags == kfShift)
                flags &= ~kfShift;
            else if (isascii(key))
                key = toupper(key);
        }
    }
    if ((flags & kfCtrl) && !(flags & kfSpecial))
        if (key < 32)
            key += 64;

    flags &= ~aSel.Mask;

    if (aSel.Mask & kfShift) {
        if (key < 256)
            if (isascii(key))
                key = toupper(key);
    }
    aKey = key | flags;
    if (aKey == aSel.Key)
        return 1;
    return 0;
}

EKey *EKeyMap::FindKey(TKeyCode aKey) {
    EKey *p = fKeys;

    while (p) {
        if (MatchKey(aKey, p->fKey)) return p;
        p = p->fNext;
    }
    return 0;
}

EEventMap::EEventMap(const char *AName, EEventMap *AParent) {
    Name = strdup(AName);
    Parent = AParent;
    KeyMap = 0;
    Next = EventMaps;
    EventMaps = this;
    memset(Menu, 0, sizeof(Menu));
    memset(abbrev, 0, sizeof(abbrev));
}

EEventMap::~EEventMap() {
    free(Name);

    // free menu[]
    for (int i = 0; i < EM_MENUS; i++)
	free(Menu[i]);

#ifdef CONFIG_ABBREV
    // free Abbrev's
    EAbbrev *ab;

    for (int i = 0; i < ABBREV_HASH; i++)
	while ((ab = abbrev[i]) != NULL) {
	    abbrev[i] = abbrev[i]->next;
	    delete ab;
	}
#endif
    // free keymap's
    delete KeyMap;
}

void EEventMap::SetMenu(int which, const char *What) {
    if (which < 0 || which >= EM_MENUS)
        return;
    if (Menu[which] != 0)
        free(Menu[which]);
    Menu[which] = strdup(What);
}

char *EEventMap::GetMenu(int which) {
    if (which < 0 || which >= EM_MENUS)
        return 0;
    if (Menu[which] || Parent == 0)
        return Menu[which];
    else
        return Parent->GetMenu(which);
}

#ifdef CONFIG_ABBREV
int EEventMap::AddAbbrev(EAbbrev *ab) {
    int i = HashStr(ab->Match, ABBREV_HASH);
    ab->next = abbrev[i];
    abbrev[i] = ab;
    return 1;
}

EAbbrev *EMode::FindAbbrev(const char *string) {
    EEventMap *Map = fEventMap;
    EAbbrev *ab;
    int i;

    if (string == 0)
        return 0;
    i = HashStr(string, ABBREV_HASH);
    while (Map) {
        ab = Map->abbrev[i];
        while (ab != 0) {
            if (ab->Match && (strcmp(string, ab->Match) == 0))
                return ab;
            ab = ab->next;
        }
        Map = Map->Parent;
    }
    return 0;
}
#endif

EKey::EKey(const char *aKey) :
    Cmd(-1),
    fKeyMap(0),
    fNext(0)
{
    ParseKey(aKey, fKey);
}

EKey::EKey(const char *aKey, EKeyMap *aKeyMap) :
    Cmd(-1),
    fKeyMap(aKeyMap),
    fNext(0)
{
    ParseKey(aKey, fKey);
}

EKey::~EKey()
{
    // if there is child keymaps delete them
    delete fKeyMap;
}

#ifdef CONFIG_ABBREV
EAbbrev::EAbbrev(const char *aMatch, const char *aReplace) :
    next(0),
    Cmd(-1),
    Match(strdup(aMatch)),
    Replace(strdup(aReplace))
{
}

EAbbrev::EAbbrev(const char *aMatch, int aCmd) :
    next(0),
    Cmd(aCmd),
    Match(strdup(aMatch)),
    Replace(0)
{
}

EAbbrev::~EAbbrev() {
    free(Match);
    free(Replace);
}
#endif

int AddCommand(int no, int Command, int count, int ign) {
    if (count == 0) return 0;
    if (Command == 0) return 0;
    Macros[no].cmds = (CommandType *)realloc(Macros[no].cmds, sizeof(CommandType) * (Macros[no].Count + 1));
    Macros[no].cmds[Macros[no].Count].type = CT_COMMAND;
    Macros[no].cmds[Macros[no].Count].u.num = Command;
    Macros[no].cmds[Macros[no].Count].repeat = short(count);
    Macros[no].cmds[Macros[no].Count].ign = short(ign);
    Macros[no].Count++;
    return 1;
}

int AddString(int no, const char *String) {
    Macros[no].cmds = (CommandType *)realloc(Macros[no].cmds, sizeof(CommandType) * (Macros[no].Count + 1));
    Macros[no].cmds[Macros[no].Count].type = CT_STRING;
    Macros[no].cmds[Macros[no].Count].u.string = strdup(String);
    Macros[no].cmds[Macros[no].Count].repeat = 0;
    Macros[no].cmds[Macros[no].Count].ign = 0;
    Macros[no].Count++;
    return 1;
}

int AddNumber(int no, long number) {
    Macros[no].cmds = (CommandType *)realloc(Macros[no].cmds, sizeof(CommandType) * (Macros[no].Count + 1));
    Macros[no].cmds[Macros[no].Count].type = CT_NUMBER;
    Macros[no].cmds[Macros[no].Count].u.num = number;
    Macros[no].cmds[Macros[no].Count].repeat = 0;
    Macros[no].cmds[Macros[no].Count].ign = 0;
    Macros[no].Count++;
    return 1;
}

int AddConcat(int no) {
    Macros[no].cmds = (CommandType *)realloc(Macros[no].cmds, sizeof(CommandType) * (Macros[no].Count + 1));
    Macros[no].cmds[Macros[no].Count].type = CT_CONCAT;
    Macros[no].cmds[Macros[no].Count].u.num = 0;
    Macros[no].cmds[Macros[no].Count].repeat = 0;
    Macros[no].cmds[Macros[no].Count].ign = 0;
    Macros[no].Count++;
    return 1;
}

int AddVariable(int no, int number) {
    Macros[no].cmds = (CommandType *)realloc(Macros[no].cmds, sizeof(CommandType) * (Macros[no].Count + 1));
    Macros[no].cmds[Macros[no].Count].type = CT_VARIABLE;
    Macros[no].cmds[Macros[no].Count].u.num = number;
    Macros[no].cmds[Macros[no].Count].repeat = 0;
    Macros[no].cmds[Macros[no].Count].ign = 0;
    Macros[no].Count++;
    return 1;
}

int NewCommand(const char *Name) {
    Macros = (ExMacro *) realloc(Macros, sizeof(ExMacro) * (1 + CMacros));
    Macros[CMacros].Count = 0;
    Macros[CMacros].cmds = 0;
    Macros[CMacros].Name = (Name != NULL) ? strdup(Name) : 0;
    CMacros++;
    return CMacros - 1;
}

int ExState::GetStrParam(EView *view, char *str, size_t maxlen) {
    if (Macro == -1
	|| Pos == -1
	|| Pos >= Macros[Macro].Count)
	return 0;
    if (Macros[Macro].cmds[Pos].type == CT_STRING) {
        if (maxlen > 0)
            strlcpy(str, Macros[Macro].cmds[Pos].u.string, maxlen);
        Pos++;
    } else if (view && Macros[Macro].cmds[Pos].type == CT_VARIABLE) {
        //puts("variable\x7");
        if (view->GetStrVar(Macros[Macro].cmds[Pos].u.num, str, maxlen) == 0)
            return 0;
        Pos++;
    } else
        return 0;
    if (Pos < Macros[Macro].Count) {
        if (Macros[Macro].cmds[Pos].type == CT_CONCAT) {
            Pos++;
            size_t len = strlen(str);

            if (len >= maxlen)
                return 0;
            size_t left = maxlen - len;

            //puts("concat\x7");
            if (GetStrParam(view, str + len, left) == 0)
                return 0;
        }
    }
    return 1;
}

int ExState::GetIntParam(EView *view, int *value) {
    if (Macro == -1
	|| Pos == -1
	|| Pos >= Macros[Macro].Count)
	return 0;
    if (Macros[Macro].cmds[Pos].type == CT_NUMBER) {
        *value = Macros[Macro].cmds[Pos].u.num;
        Pos++;
    } else if (view && Macros[Macro].cmds[Pos].type == CT_VARIABLE) {
        if (view->GetIntVar(Macros[Macro].cmds[Pos].u.num, value) == 0)
            return 0;
        Pos++;
    } else
        return 0;
    return 1;
}

int HashStr(const char *p, int maxim) {
    unsigned int i = 1;

    while (p && *p)
	i += i ^ (i << 3) ^ (unsigned int)(*p++) ^ (i >> 3);

    return i % maxim;
}
