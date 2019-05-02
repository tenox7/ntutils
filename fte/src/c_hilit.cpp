/*    c_hilit.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "o_buflist.h"

#ifdef CONFIG_SYNTAX_HILIT

#include "sysdep.h"

#include <stdio.h>

static const struct {
    const char Name[8];
    int Num;
    SyntaxProc Proc;
} HilitModes[] = {
{ "PLAIN", HILIT_PLAIN, Hilit_Plain },
#ifdef CONFIG_HILIT_C
{ "C", HILIT_C, Hilit_C },
#endif
#ifdef CONFIG_HILIT_REXX
{ "REXX", HILIT_REXX, Hilit_REXX },
#endif
#ifdef CONFIG_HILIT_PERL
{ "PERL", HILIT_PERL, Hilit_PERL },
#endif
#ifdef CONFIG_HILIT_MAKE
{ "MAKE", HILIT_MAKE, Hilit_MAKE },
#endif
#ifdef CONFIG_HILIT_IPF
{ "IPF", HILIT_IPF, Hilit_IPF },
#endif
#ifdef CONFIG_HILIT_ADA
{ "Ada", HILIT_ADA, Hilit_ADA },
#endif
#ifdef CONFIG_HILIT_MSG
{ "MSG", HILIT_MSG, Hilit_MSG },
#endif
#ifdef CONFIG_HILIT_SH
{ "SH", HILIT_SH, Hilit_SH },
#endif
#ifdef CONFIG_HILIT_PASCAL
{ "PASCAL", HILIT_PASCAL, Hilit_PASCAL },
#endif
#ifdef CONFIG_HILIT_TEX
{ "TEX", HILIT_TEX, Hilit_TEX },
#endif
#ifdef CONFIG_HILIT_FTE
{ "FTE", HILIT_FTE, Hilit_FTE },
#endif
#ifdef CONFIG_HILIT_CATBS
{ "CATBS", HILIT_CATBS, Hilit_CATBS },
#endif
#ifdef CONFIG_HILIT_SIMPLE
{ "SIMPLE", HILIT_SIMPLE, Hilit_SIMPLE },
#endif
};

static const struct {
    const char Name[8];
    int Num;
} IndentModes[] = {
#ifdef CONFIG_INDENT_C
{ "C", INDENT_C },
#endif
#ifdef CONFIG_INDENT_REXX
{ "REXX", INDENT_REXX },
#endif
#ifdef CONFIG_INDENT_SIMPLE
{ "SIMPLE", INDENT_REXX },
#endif
{ "PLAIN", INDENT_PLAIN },
};

EColorize *Colorizers = 0;

int GetIndentMode(const char *Str) {
    for (size_t i = 0; i < FTE_ARRAY_SIZE(IndentModes); ++i)
        if (strcmp(Str, IndentModes[i].Name) == 0)
            return IndentModes[i].Num;
    return 0;
}

int GetHilitMode(const char *Str) {
    for (size_t i = 0; i < FTE_ARRAY_SIZE(HilitModes); ++i)
        if (strcmp(Str, HilitModes[i].Name) == 0)
            return HilitModes[i].Num;
    return HILIT_PLAIN;
}

SyntaxProc GetHilitProc(int id) {
    for (size_t i = 0; i < FTE_ARRAY_SIZE(HilitModes); ++i)
        if (id == HilitModes[i].Num)
            return HilitModes[i].Proc;
    return 0;
}

#ifdef CONFIG_WORD_HILIT
int EBuffer::HilitAddWord(const char *Word) {
    if (HilitFindWord(Word) == 1)
	return 1;
    WordList.push_back(Word);
    FullRedraw();
    return 1;
}

int EBuffer::HilitFindWord(const char *Word) {
    vector_iterate(StlString, WordList, it) {
        if (BFI(this, BFI_MatchCase) == 1) {
            if (strcmp(Word, (*it).c_str()) == 0) return 1;
        } else {
            if (stricmp(Word, (*it).c_str()) == 0) return 1;
        }
    }
    return 0;
}

int EBuffer::HilitRemoveWord(const char *Word) {
    vector_iterate(StlString, WordList, it) {
        if (BFI(this, BFI_MatchCase) == 1) {
            if (strcmp(Word, (*it).c_str()) != 0) continue;
        } else {
            if (stricmp(Word, (*it).c_str()) != 0) continue;
	}
        WordList.erase(it);
        FullRedraw();
        return 1;
    }
    return 0;
}

int EBuffer::HilitWord() {
    PELine L = VLine(CP.Row);
    char s[CK_MAXLEN + 2];
    int P, len = 0;
    
    P = CharOffset(L, CP.Col);
    while ((P > 0) && ((ChClass(L->Chars[P - 1]) == 1) || (L->Chars[P - 1] == '_'))) 
        P--;
    while (len < CK_MAXLEN && P < (int)L->Count && (ChClass(L->Chars[P]) == 1 || L->Chars[P] == '_'))
        s[len++] = L->Chars[P++];
    if (len == 0)
        return 0;
    s[len] = 0;

    return (HilitFindWord(s)) ? HilitRemoveWord(s) : HilitAddWord(s);
}
#endif

/* ======================================================================= */

EColorize::EColorize(const char *AName, const char *AParent) :
    Name(AName),
    Next(Colorizers),
    Parent(FindColorizer(AParent)),
    SyntaxParser(HILIT_PLAIN),
    hm(0)
{
    Colorizers = this;
    memset(&Keywords, 0, sizeof(Keywords));
    if (Parent) {
        SyntaxParser = Parent->SyntaxParser;
        memcpy(Colors, Parent->Colors, sizeof(Colors));
    } else
	memset(Colors, 0, sizeof(Colors));
}

EColorize::~EColorize() {
    for (int i=0; i<CK_MAXLEN; i++)
        free(Keywords.key[i]);

    delete hm;
}

EColorize *FindColorizer(const char *AName) {
    EColorize *p = Colorizers;

    while (p) {
        if (p->Name == AName)
            return p;
        p = p->Next;
    }
    return 0;
}

int EColorize::SetColor(int idx, const char *Value) {
    unsigned int ColBg, ColFg;

    if (sscanf(Value, "%1X %1X", &ColFg, &ColBg) != 2)
        return 0;

    if (idx < 0 || idx >= COUNT_CLR)
        return 0;
    Colors[idx] = ChColor(ColFg | (ColBg << 4));
    return 1;
}

/* ======================================================================= */

void HTrans::InitTrans() {
    match = 0;
    matchLen = 0;
    matchFlags = 0;
    nextState = 0;
    color = 0;
    regexp = 0;
}

/* ======================================================================= */

void HState::InitState() {
    memset((void *)&keywords, 0, sizeof(keywords));
    firstTrans = 0;
    transCount = 0;
    color = 0;
    wordChars = 0;
    options = 0;
    nextKwdMatchedState = -1;
    nextKwdNotMatchedState = -1;
    nextKwdNoCharState = -1;
}

int HState::GetHilitWord(ChColor &clr, const char *str, size_t len) {
    char *p;

    if (len >= CK_MAXLEN || !len)
        return 0;

    p = keywords.key[len];
    if (options & STATE_NOCASE) {
        while (p && *p) {
	    if (strnicmp(p, str, len) == 0) {
                clr = ChColor(COUNT_CLR + ((unsigned char*)p)[len]);
                return 1;
            }
            p += len + 1;
        }
    } else {
        while (p && *p) {
            if (memcmp(p, str, len) == 0) {
                clr = ChColor(COUNT_CLR + ((unsigned char*)p)[len]);
                return 1;
            }
            p += len + 1;
        }
    }
    return 0;
}

/* ======================================================================= */

HMachine::HMachine() :
    stateCount(0),
    transCount(0),
    state(0),
    trans(0)
{
}

HMachine::~HMachine() {

    // free states
    if (state) {
        while (stateCount--) {
            for (unsigned i = 0; i < CK_MAXLEN; ++i)
                free(state[stateCount].keywords.key[i]);

            free(state[stateCount].wordChars);
        }

        free(state);
    }

    // free transes
    if (trans) {
        while (transCount--) {
            if (trans[transCount].match) free(trans[transCount].match);
            if (trans[transCount].regexp) RxFree(trans[transCount].regexp);
        }

        free(trans);
    }

}

void HMachine::AddState(HState &aState) {
    state = (HState *)realloc(state, (stateCount + 1) * sizeof(HState) );
    assert( state );
    state[stateCount] = aState;
    state[stateCount].firstTrans = transCount;
    stateCount++;
}

void HMachine::AddTrans(HTrans &aTrans) {
    assert(stateCount > 0);
    trans = (HTrans *)realloc(trans, (transCount + 1) * sizeof(HTrans) );
    assert( trans );
    state[stateCount - 1].transCount++;
    trans[transCount] = aTrans;
    transCount++;
}

#endif // CONFIG_SYNTAX_HILIT
