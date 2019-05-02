/*    i_complete.cpp
 *
 *    Copyright (c) 1998, Zdenek Kabelac
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "i_complete.h"

#ifdef CONFIG_I_COMPLETE

#include "e_tags.h"

#include <stdio.h>
#include <stdlib.h>

#define STRCOMPLETE "Complete Word: ["
#define STRNOCOMPLETE "No word for completition..."

#define LOCALE_SORT

#ifdef LOCALE_SORT

#if defined(__IBMCPP__)
static int _LNK_CONV CmpStr(const void *p1, const void *p2) {
#else
static int CmpStr(const void *p1, const void *p2) {
#endif
    //printf("%s  %s  %d\n", *(char **)p1, *(char **)p2,
    //	   strcoll(*(char **)p1, *(char **)p2));
    return strcoll(*(const char **)p1, *(const char **)p2);
}
#endif

/**
 * Create Sorted list of possible word extensions
 */
ExComplete::ExComplete(EBuffer *B) :
    Orig(B->CP),
    Buffer(B),
    WordsLast(0),
    Words(new char *[MAXCOMPLETEWORDS + 2]),
    WordBegin(0),
    WordPos(0),
    WordFixed(0)
{
    RefreshComplete();
}

ExComplete::~ExComplete()
{
//    fprintf(stderr, "W %p %p %p %d\n", Words, WordContinue, WordBegin, WordsLast);
    delete[] WordBegin;

    if (Words != NULL) {
	for(int i = 0; i < WordsLast; i++)
            delete[] Words[i];
	delete[] Words;
    }
}

bool ExComplete::IsSimpleCase()
{
    return (WordsLast < 2) ? true : false;
}

int ExComplete::DoCompleteWord()
{
    int rc = 0;

    if (WordsLast <= 0 || !Words[WordPos]) return rc;

    size_t l = strlen(Words[WordPos]);

    if (Buffer->InsText(Buffer->VToR(Orig.Row), Orig.Col, l, Words[WordPos], 1)
        && Buffer->SetPos(Orig.Col + l, Orig.Row)) {
        Buffer->Draw(Buffer->VToR(Orig.Row), Buffer->VToR(Orig.Row));
        rc = 1;
    }

    return rc;
}

void ExComplete::HandleEvent(TEvent &Event)
{
    unsigned long kb = kbCode(Event.Key.Code);
    int DoQuit = 0;
    int i = 0;

    if (WordsLast < 2) {
	if ((WordsLast == 1) && (kb != kbEsc)) {
	    DoQuit = 1;
	} else {
	    EndExec(0);
	    Event.What = evNone;
	}
    } else if (Event.What == evKeyDown) {
	switch(kb) {
	case kbPgUp:
	case kbLeft:
	    // if there would not be locale sort, we could check only
	    // the next string, but with `locale sort` this is impossible!!
	    // this loop is little inefficient but it's quite short & nice
	    for (i = WordPos; i-- > 0;)
		if (strncmp(Words[WordPos], Words[i], WordFixed) == 0) {
		    WordPos = i;
		    break;
		}
	    Event.What = evNone;
	    break;
	case kbPgDn:
	case kbRight:
	    for(i = WordPos; i++ < WordsLast - 1;)
		if (strncmp(Words[WordPos], Words[i], WordFixed) == 0) {
		    WordPos = i;
		    break;
		}
	    Event.What = evNone;
	    break;
	case kbHome:
	    for (i = 0; i < WordPos; i++)
		if (strncmp(Words[WordPos], Words[i], WordFixed) == 0)
		    WordPos = i;
	    Event.What = evNone;
	    break;
	case kbEnd:
	    for (i = WordsLast - 1; i > WordPos; i--)
		if (strncmp(Words[WordPos], Words[i], WordFixed) == 0)
		    WordPos = i;
	    Event.What = evNone;
	    break;
	case kbTab:
	    while (WordPos < WordsLast - 1) {
		WordPos++;
		if (strncmp(Words[WordPos], Words[WordPos - 1],
			    WordFixed + 1))
		    break;
	    }
	    Event.What = evNone;
	    break;
	case kbTab | kfShift:
	    while (WordPos > 0) {
		WordPos--;
		if (strncmp(Words[WordPos], Words[WordPos + 1],
			    WordFixed + 1))
		    break;
	    }
	    Event.What = evNone;
	    break;
	case kbIns:
	case kbUp:
	    FixedUpdate(1);
	    Event.What = evNone;
	    break;
	case kbBackSp:
	case kbDel:
	case kbDown:
	    FixedUpdate(-1);
	    Event.What = evNone;
	    break;
	case kbEsc:
	    EndExec(0);
	    Event.What = evNone;
	    break;
	case kbEnter:
	case kbSpace:
	case kbTab | kfCtrl:
	    DoQuit = 1;
	    break;
	default:
            if (CheckASCII((char)Event.Key.Code)) {
		char *s = (char *)alloca(WordFixed + 2);
		strncpy(s, Words[WordPos], WordFixed);
		s[WordFixed] = (char)(Event.Key.Code);
		s[WordFixed + 1] = 0;
		for (int i = 0; i < WordsLast; i++)
		    if (strncmp(s, Words[i], WordFixed + 1) == 0) {
			WordPos = i;
			if (WordFixedCount == 1)
			    DoQuit = 1;
			else
			    FixedUpdate(1);
			break;
		    }
		Event.What = evNone;
	    }
	    break;
	}
    }

    if (DoQuit) {
/*	int rc = 0;
	int l = strlen(Words[WordPos]);

	if (Buffer->InsText(Buffer->VToR(Orig.Row), Orig.Col, l, Words[WordPos], 1)
	    && Buffer->SetPos(Orig.Col + l, Orig.Row)) {
	    Buffer->Draw(Buffer->VToR(Orig.Row), Buffer->VToR(Orig.Row));
	    rc = 1;
        }*/

        int rc = DoCompleteWord();
        EndExec(rc);
	Event.What = evNone;
    }
}

void ExComplete::RepaintStatus()
{
    TDrawBuffer B;
    int W, H;

    // Currently use this fixed colors - maybe there are some better defines??
#define COM_NORM 0x17
#define COM_ORIG 0x1C
#define COM_HIGH 0x1E
#define COM_MARK 0x2E
#define COM_ERR  0x1C

    ConQuerySize(&W, &H);
    MoveCh(B, ' ', COM_NORM, W);

    if ((WordsLast > 0) && (WordBegin != NULL) && (Words != NULL)
	&& (Words[WordPos]) != NULL) {
	const char *sc = STRCOMPLETE;
	size_t p = sizeof(STRCOMPLETE) - 1;
	if (W < 35) {
	    // if the width is quite small
	    sc += p - 1; // jump to last character
	    p = 1;
	}
	MoveStr(B, 0, W, sc, COM_NORM, W);
	// int cur = p;
	MoveStr(B, (int)p, W, WordBegin, COM_ORIG, W);
	p += strlen(WordBegin);
	size_t l = strlen(Words[WordPos]);
	if (WordFixed > 0) {
	    MoveStr(B, (int)p, W, Words[WordPos], COM_MARK, W);
	    p += WordFixed;
	    l -= WordFixed;
	}
	MoveStr(B, (int)p, W, Words[WordPos] + WordFixed,
		(WordFixedCount == 1) ? COM_ORIG : COM_HIGH, W);
	p += l;
	char s[100];
	sprintf(s, "] (T:%d/%d  S:%d)", WordPos + 1, WordsLast,
		(int)WordFixedCount);
	MoveStr(B, (int)p, W, s, COM_NORM, W);
	// ConSetCursorPos(cur + WordFixed, H - 1);
    } else
	MoveStr(B, 0, W, STRNOCOMPLETE, COM_ERR, W);
    ConPutBox(0, H - 1, W, 1, B);
    ConShowCursor();
}

int ExComplete::RefreshComplete()
{
    if ((Buffer->CP.Col == 0)
	|| (Buffer->SetPos(Buffer->CP.Col, Buffer->CP.Row) == 0))
	return 0;

    PELine L = Buffer->VLine(Buffer->CP.Row);
    int C = Buffer->CP.Col;
    int P = Buffer->CharOffset(L, C);

    if (!P || P > L->Count)
	return 0;

    int P1 = P;
    while ((P > 0) && CheckASCII(L->Chars[P - 1]))
	P--;

    int wlen = P1 - P;
    if (!wlen)
	return 0;

    WordBegin = new char[wlen + 1];
    if (WordBegin == NULL)
	return 0;

    strncpy(WordBegin, L->Chars + P, wlen);
    WordBegin[wlen] = 0;

    // fprintf(stderr, "Calling %d  %s\n", wlen, WordBegin);
    // Search words in TAGS
    TagComplete(Words, &WordsLast, MAXCOMPLETEWORDS, WordBegin);
    // fprintf(stderr, "Located %d words\n", WordsLast);
    // these words are already sorted

    // Search words in current TEXT
    Buffer->Match.Col = Buffer->Match.Row = 0;
    // this might look strange but it is necessary to catch
    // the first word at position 0,0 for match :-)
    unsigned long mask = SEARCH_NOPOS | SEARCH_WORDBEG;

    while (Buffer->FindStr(L->Chars + P, wlen, mask) == 1) {
	mask |= SEARCH_NEXT;
	PELine M = Buffer->RLine(Buffer->Match.Row);
	int X = Buffer->CharOffset(M, Buffer->Match.Col);

        if ((L->Chars == M->Chars) && (P == X))
            continue;

        int XL = X;

        while ((XL < M->Count) && CheckASCII(M->Chars[XL]))
	    XL++;

        int len = XL - X - wlen;

	if (len == 0)
	    continue;

        char *s = new char[len + 1];

	if (s != NULL) {
	    strncpy(s, M->Chars + X + wlen, len);
	    s[len] = 0;

	    int c = 1, H = 0, L = 0, R = WordsLast;

            // using sort to insert only unique words
	    while (L < R) {
		H = (L + R) / 2;
		c = strcmp(s, Words[H]);
		if (c < 0)
		    R = H;
		else if (c > 0)
		    L = H + 1;
		else
                    break;
            }

	    if (c != 0) {
	        // Loop exited without finding the word. Instead,
	        // it found the spot where the new should be inserted.
		WordsLast++;

                int i = WordsLast;

                while (i > L) {
		    Words[i] = Words[i-1];
		    i--;
                }

                Words[i] = s;

		if (WordsLast >= MAXCOMPLETEWORDS)
		    break;
            } else
            {
                // word was already listed, free duplicate.
                delete[] s;
            }
	}
    }
    Buffer->Match.Row = Buffer->Match.Col = -1;
    Buffer->MatchLen = Buffer->MatchCount = 0;

#ifdef LOCALE_SORT
    // sort by current locales
    qsort(Words, WordsLast, sizeof(Words[0]), CmpStr);
#endif

    FixedUpdate(0);

    //for(int i = 0; i < WordsLast; i++)
	//if (Words[i])
	    //fprintf(stderr, "%3d:\t%10p\t%s\n", i, Words[i], Words[i]);
    //fprintf(stderr, "Words %3d\n", WordsLast);

    return WordsLast;
}

void ExComplete::FixedUpdate(int add)
{
    if (add < 0) {
	if (WordFixed > 0)
	    WordFixed += add;
    } else if (add > 0) {
	if (strlen(Words[WordPos]) > WordFixed)
	    WordFixed += add;
    }

    if (WordFixed > 0) {
	WordFixedCount = 0;
	for(int i = 0; i < WordsLast; i++)
	    if (strncmp(Words[WordPos], Words[i], WordFixed) == 0)
		WordFixedCount++;
    } else
	WordFixedCount = WordsLast;

}
/*
    // Well this was my first idea - but these menus are unusable
    int menu = NewMenu("CW");
    int n;
    n = NewItem(menu, "Word1");
    Menus[menu].Items[n].Cmd = ExFind;
    n = NewItem(menu, "Word2");
    Menus[menu].Items[n].Cmd = ExMoveLineStart;
    n = NewItem(menu, "Word3");
    Menus[menu].Items[n].Cmd = ExMovePageEnd;
    printf(">%d ****\n", View->MView->Win->Parent->PopupMenu("CW"));
 */
#endif
