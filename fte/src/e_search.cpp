/*    e_search.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "c_history.h"
#include "e_tags.h"
#include "i_modelview.h"
#include "i_view.h"
#include "o_buflist.h"
#include "s_util.h"
#include "sysdep.h"

#include <ctype.h>

SearchReplaceOptions::SearchReplaceOptions()
{
    memset(this, 0, sizeof(*this));
}

// *INDENT-OFF*
int ParseSearchOption(int replace, char c, unsigned long &opt) {
    switch (tolower(c)) {
    case 'a': opt |= SEARCH_ALL; break;      // search all occurances
    case 'b': opt |= SEARCH_BLOCK; break;    // search in block only
    case 'c': opt &= ~SEARCH_NEXT; break;    // search from current position
    case 'd': opt |= SEARCH_DELETE; break;   // delete found line
    case 'g': opt |= SEARCH_GLOBAL; break;   // search globally
    case 'i': opt |= SEARCH_NCASE; break;    // don't match case
    case 'j': opt |= SEARCH_JOIN; break;     // join found line
    case 'r': opt |= SEARCH_BACK; break;     // search reverse
    case 'w': opt |= SEARCH_WORDBEG | SEARCH_WORDEND; break;
    case '<': opt |= SEARCH_WORDBEG; break;
    case '>': opt |= SEARCH_WORDEND; break;
    case 'x': opt |= SEARCH_RE; break;       // search using regexps
    default:
        if (!replace) return 0;
        switch (c) {
        case 'n': opt |= SEARCH_NASK; break; // don't ask before replacing
        default:
            return 0;
        }
    }
    return 1;
}

static int UnquoteString(char *str) {
    char *s, *d;

    s = str;
    d = str;
    while (*s) {
        if (*s == '\\') {
            s++;
            if (*s == 0) return 0;
        }
        *d++ = *s++;
    }
    *d = 0;
    return 1;
}

int ParseSearchOptions(int replace, const char *str, unsigned long &Options) {
    const char *p = str;

    Options = SEARCH_NEXT;
    while (*p) {
        if (ParseSearchOption(replace, *p, Options) == 0) return 0;
        p++;
    }
    return 1;
}

int ParseSearchReplace(EBuffer *B, const char *str, int replace, SearchReplaceOptions &opt) {
    int where = 0;
    int len = 0;
    const char *p = str;

    memset((void *)&opt, 0, sizeof(opt));
    if (p == 0) return 0;
    if (replace) {
        if (ParseSearchOptions(replace, BFS(B, BFS_DefFindReplaceOpt), opt.Options) == 0) return 0;
        opt.Options |= SEARCH_REPLACE;
    } else {
        if (ParseSearchOptions(replace, BFS(B, BFS_DefFindOpt), opt.Options) == 0) return 0;
    }

    while (*p) {
        switch (*p) {
        case '\\':
            if (where == 0) {
                opt.strSearch[len++] = *p++;
                if (*p == 0) return 0;
                opt.strSearch[len++] = *p++;
            } else if (where == 1) {
                opt.strReplace[len++] = *p++;
                if (*p == 0) return 0;
                opt.strReplace[len++] = *p++;
            } else
                return 0;
            break;
        case '/':
            where++;
            if (!replace && where == 1) where++;
            if (where == 2)
                opt.Options = SEARCH_NEXT;
            if (where > 2) return 0;
            len = 0;
            p++;
            break;
        default:
            if (where == 0)
                opt.strSearch[len++] = *p++;
            else if (where == 1)
                opt.strReplace[len++] = *p++;
            else {
                char c = *p;

                p++;
                if (ParseSearchOption(replace, c, opt.Options) == 0) return 0;
            }
        }
    }
    if (opt.Options & SEARCH_RE);
    else {
        if (UnquoteString(opt.strSearch) == 0) return 0;
        if (opt.Options & SEARCH_REPLACE)
            if (UnquoteString(opt.strReplace) == 0) return 0;
    }

    opt.ok = 1;
    return 1;
}

int EBuffer::FindStr(const char *Data, int Len, int Options) {
    SearchReplaceOptions opt;

    opt.Options = Options;

    return FindStr(Data, Len, opt);
}

int EBuffer::FindStr(const char *Data, int Len, SearchReplaceOptions &opt) {
    int Options = opt.Options;
    int LLen, Start, End;
    int C, L;
    PELine X;
    char *P;

    if (Options & SEARCH_RE)
        return 0;
    if (Len <= 0)
        return 0;

    if (Options & SEARCH_NOPOS) {
        C = Match.Col;
        L = Match.Row;
    } else {
        C = CP.Col;
        L = VToR(CP.Row);
    }
    if (Match.Row != -1)
        Draw(Match.Row, Match.Row);
    Match.Row = -1;
    Match.Col = -1;
    X = RLine(L);
    C = CharOffset(X, C);

    if (Options & SEARCH_NEXT) {
        int CC = MatchCount ? 1 : 0;

        if (Options & SEARCH_BACK) {
            C -= CC;
            if (C < 0) {
                if (L == 0) return 0;
                L--;
                X = RLine(L);
                C = X->Count;
            }
        } else {
            if (Options & SEARCH_REPLACE &&
                opt.lastInsertLen > 0)
            {
                C += CC * opt.lastInsertLen; // 0 or opt.lastInsertLen
            } else
            {
                C += CC;
            }

            if (C >= X->Count) {
                C = 0;
                L++;
                if (L == RCount) return 0;
            }
        }
    }
    MatchLen = 0;
    MatchCount = 0;

    if (Options & SEARCH_BLOCK) {
        if (Options & SEARCH_BACK) {
            if (BlockMode == bmStream) {
                if (L > BE.Row) {
                    L = BE.Row;
                    C = BE.Col;
                }
                if (L == BE.Row && C > BE.Col)
                    C = BE.Col;
            } else {
                if (L >= BE.Row && BE.Row > 0) {
                    L = BE.Row - 1;
                    C = RLine(L)->Count;
                }
                if (BlockMode == bmColumn)
                    if (L == BE.Row - 1 && C >= BE.Col)
                        C = BE.Col;
            }
        } else {
            if (L < BB.Row) {
                L = BB.Row;
                C = 0;
            }
            if (L == BB.Row && C < BB.Col)
                C = BB.Col;
        }
    }
    while (1) {
        if (Options & SEARCH_BLOCK) {
            if (BlockMode == bmStream) {
                if (L > BE.Row || L < BB.Row) break;
            } else
                if (L >= BE.Row || L < BB.Row) break;
        } else
            if (L >= RCount || L < 0) break;

        X = RLine(L);

        LLen = X->Count;
        P = X->Chars;
        Start = 0;
        End = LLen;

        if (Options & SEARCH_BLOCK) {
            if (BlockMode == bmColumn) {
                Start = CharOffset(X, BB.Col);
                End = CharOffset(X, BE.Col);
            } else if (BlockMode == bmStream) {
                if (L == BB.Row)
                    Start = CharOffset(X, BB.Col);
                if (L == BE.Row)
                    End = CharOffset(X, BE.Col);
            }
        }
        if (Options & SEARCH_BACK) {
            if (C >= End - Len)
                C = End - Len;
        } else {
            if (C < Start)
                C = Start;
        }

        while (((!(Options & SEARCH_BACK)) && (C <= End - Len))
               || ((Options & SEARCH_BACK) && (C >= Start))) {
            if ((!(Options & SEARCH_WORDBEG)
                 || (C == 0)
                 || (WGETBIT(Flags.WordChars, P[C - 1]) == 0))
                &&
                (!(Options & SEARCH_WORDEND)
                 || (C + Len >= End)
                 || (WGETBIT(Flags.WordChars, P[C + Len]) == 0))
                &&
                ((!(Options & SEARCH_NCASE)
                  && (P[C] == Data[0])
                  && (memcmp(P + C, Data, Len) == 0))
                 ||
                 ((Options & SEARCH_NCASE)
                  && (toupper(P[C]) == toupper(Data[0]))
                  && (strnicmp(P + C, Data, Len) == 0))) /* && BOL | EOL */
               )
            {
                Match.Col = ScreenPos(X, C);
                Match.Row = L;
                MatchCount = Len;
                MatchLen = ScreenPos(X, C + Len) - Match.Col;
                if (!(Options & SEARCH_NOPOS)) {
                    if (Options & SEARCH_CENTER)
                        CenterPosR(Match.Col, Match.Row);
                    else
                        SetPosR(Match.Col, Match.Row);
                }
                Draw(L, L);
                return 1;
            }
            if (Options & SEARCH_BACK) C--; else C++;
        }
        if (Options & SEARCH_BACK) {
            L--;
            if (L >= 0)
                C = RLine(L)->Count;
        } else {
            C = 0;
            L++;
        }
    }
    //SetPos(OC, OL);
    return 0;
}

int EBuffer::FindRx(RxNode *Rx, SearchReplaceOptions &opt) {
    int Options = opt.Options;
    int LLen, Start, End;
    int C, L;
    char *P;
    PELine X;
    RxMatchRes b;

    if (!(Options & SEARCH_RE))
        return 0;
    if (Options & SEARCH_BACK) { // not supported
        View->MView->Win->Choice(GPC_ERROR, "FindRx", 1, "O&K", "Reverse regexp search not supported.");
        return 0;
    }
    if (Rx == 0)
        return 0;

    if (Match.Row != -1)
        Draw(Match.Row, Match.Row);
    Match.Row = -1;
    Match.Col = -1;

    C = CP.Col;
    L = VToR(CP.Row);
    X = RLine(L);
    C = CharOffset(X, C);

    if (Options & SEARCH_NEXT) {
        int CC = MatchCount ? MatchCount : 1;

        if (Options & SEARCH_BACK) {
            C -= CC;
            if (Options & SEARCH_BLOCK) {
                if (C < BB.Col && L == BB.Row)
                    return 0;
                L--;
                X = RLine(L);
                C = X->Count;
                if (BlockMode == bmColumn)
                    if (BE.Col < C)
                        C = BE.Col;
            } else {
                if (C < 0 && L == 0)
                    return 0;
                L--;
                X = RLine(L);
                C = X->Count;
            }
        } else {
            C += CC;
            if (Options & SEARCH_BLOCK) {
                if (BlockMode == bmStream || BlockMode == bmLine) {
                    if (C >= X->Count) {
                        C = 0;
                        L++;
                        if (BlockMode == bmLine) {
                            if (L == BE.Row) return 0;
                        } else
                            if (L == BE.Row && (C >= BE.Col || C >= X->Count))
                                return 0;
                    }
                } else if (BlockMode == bmColumn) {
                    if (C >= X->Count || C >= BE.Col) {
                        C = BB.Col;
                        L++;
                        if (L == BE.Row) return 0;
                    }
                }
            } else {
                if (C >= X->Count) {
                    C = 0;
                    L++;
                    if (L == RCount) return 0;
                }
            }
        }
    }
    MatchLen = 0;
    MatchCount = 0;

    if (Options & SEARCH_BLOCK) {
        if (Options & SEARCH_BACK) {
            if (BlockMode == bmStream) {
                if (L > BE.Row) {
                    L = BE.Row;
                    C = BE.Col;
                }
                if (L == BE.Row && C > BE.Col)
                    C = BE.Col;
            } else {
                if (L >= BE.Row && BE.Row > 0) {
                    L = BE.Row - 1;
                    C = RLine(L)->Count;
                }
                if (BlockMode == bmColumn)
                    if (L == BE.Row - 1 && C >= BE.Col)
                        C = BE.Col;
            }
        } else {
            if (L < BB.Row) {
                L = BB.Row;
                C = 0;
            }
            if (L == BB.Row && C < BB.Col)
                C = BB.Col;
        }
    }

    while (1) {
        if (Options & SEARCH_BLOCK) {
            if (BlockMode == bmStream) {
                if (L > BE.Row || L < BB.Row) break;
            } else
                if (L >= BE.Row || L < BB.Row) break;
        } else
            if (L >= RCount || L < 0) break;

        X = RLine(L);
        LLen = X->Count;
        P = X->Chars;
        Start = 0;
        End = LLen;

        if (Options & SEARCH_BLOCK) {
            if (BlockMode == bmColumn) {
                Start = CharOffset(X, BB.Col);
                End = CharOffset(X, BE.Col);
            } else if (BlockMode == bmStream) {
                if (L == BB.Row)
                    Start = CharOffset(X, BB.Col);
                if (L == BE.Row)
                    End = CharOffset(X, BE.Col);
            }
            if (End > LLen)
                End = LLen;
        }
        if (Options & SEARCH_BACK) {
            if (C >= End)
                C = End;
        } else {
            if (C < Start)
                C = Start;
        }

        if (Start <= End) {
            if (RxExec(Rx, P + Start, End - Start, P + C, &b, (Options & SEARCH_NCASE) ? 0 : RX_CASE) == 1) {
                C = ScreenPos(X, b.Open[0] + Start);
                Match.Col = C;
                Match.Row = L;
                MatchCount = b.Close[0] - b.Open[0];
                MatchLen = ScreenPos(X, b.Close[0] + Start) - C;
                for (int mm = 0; mm < NSEXPS; mm++) {
                    b.Open[mm] += Start;
                    b.Close[mm] += Start;
                }
                MatchRes = b;
                if (!(Options & SEARCH_NOPOS)) {
                    if (Options & SEARCH_CENTER)
                        CenterPosR(C, L);
                    else
                        SetPosR(C, L);
                }
                Draw(L, L);
                return 1;
            }
        }
        C = 0;
        L++;
    }
    //SetPos(OC, OL);
    return 0;

}

int EBuffer::Find(SearchReplaceOptions &opt) {
    size_t slen = strlen(opt.strSearch);
    int Options = opt.Options;
    size_t rlen = strlen(opt.strReplace);
    RxNode *R = NULL;

    opt.resCount = -1;
    opt.lastInsertLen = 0;

    if (slen == 0) return 0;
    if (Options & SEARCH_BLOCK) {
        if (CheckBlock() == 0) return 0;
    }
    if (Options & SEARCH_RE) {
        R = RxCompile(opt.strSearch);
        if (R == 0) {
            View->MView->Win->Choice(GPC_ERROR, "Find", 1, "O&K", "Invalid regular expression.");
            return 0;
        }
    }
    if (Options & SEARCH_GLOBAL) {
        if (Options & SEARCH_BLOCK) {
            if (Options & SEARCH_BACK) {
                if (SetPosR(BE.Col, BE.Row) == 0) goto error;
            } else {
                if (SetPosR(BB.Col, BB.Row) == 0) goto error;
            }
        } else {
            if (Options & SEARCH_BACK) {
                if (RCount < 1) goto error;
                if (SetPosR(LineLen(RCount - 1), RCount - 1) == 0) goto error;
            } else {
                if (SetPosR(0, 0) == 0) goto error;
            }
        }
    }
    opt.resCount = 0;
    while (1) {
        if (Options & SEARCH_RE) {
            if (FindRx(R, opt) == 0) goto end;
        } else {
            if (FindStr(opt.strSearch, slen, opt) == 0) goto end;
        }
        opt.resCount++;

        if (opt.Options & SEARCH_REPLACE) {
            char ask = 'A';

            if (!(Options & SEARCH_NASK)) {
                char ch;

                while (1) {
                    Draw(VToR(CP.Row), 1);
                    Redraw();
                    switch (View->MView->Win->Choice(0, "Replace",
                                                     5,
                                                     "&Yes",
                                                     "&All",
                                                     "&Once",
                                                     "&Skip",
                                                     "&Cancel",
                                                     "Replace with %s?", opt.strReplace))
                    {
                    case 0: ch = 'Y'; break;
                    case 1: ch = 'A'; break;
                    case 2: ch = 'O'; break;
                    case 3: ch = 'N'; break;
                    case 4:
                    case -1:
                    default:
                        ch = 'Q'; break;
                    }
                    if (ch == 'Y') { ask = 'Y'; goto ok_rep; }
                    if (ch == 'N') { ask = 'N'; goto ok_rep; }
                    if (ch == 'Q') { ask = 'Q'; goto ok_rep; }
                    if (ch == 'A') { ask = 'A'; goto ok_rep; }
                    if (ch == 'O') { ask = 'O'; goto ok_rep; }
                }
            ok_rep:
                if (ask == 'N') goto try_join;
                if (ask == 'Q') goto end;
                if (ask == 'A') Options |= SEARCH_NASK;
            }

            if (Options & SEARCH_RE) {
                PELine L = RLine(Match.Row);
                int P, R;
                char *PR = 0;
                size_t LR = 0;

                R = Match.Row;
                P = Match.Col;
                P = CharOffset(L, P);

                if (0 == RxReplace(opt.strReplace, L->Chars, L->Count, MatchRes, &PR, &LR)) {
                    if (DelText(R, Match.Col, MatchLen) == 0) goto error;
                    if (PR && LR > 0)
                        if (InsText(R, Match.Col, LR, PR) == 0) goto error;
                    if (PR)
                        free(PR);
                    rlen = LR;
                }
            } else {
                if (DelText(Match.Row, Match.Col, MatchLen) == 0) goto error;
                if (InsText(Match.Row, Match.Col, rlen, opt.strReplace) == 0) goto error;

                // Cursor remains at start of inserted string. If there is recursive
                // replace pattern, fte can go it infinite loop.
                // Workaround: Move cursor to end of inserted string
                opt.lastInsertLen = strlen(opt.strReplace);
            }
            if (!(Options & SEARCH_BACK)) {
                MatchLen = rlen;
                MatchCount = rlen;
            }
            if (ask == 'O')
                goto end;
        }
    try_join:
        if (Options & SEARCH_JOIN) {
            char ask = 'A';

            if (!(Options & SEARCH_NASK)) {
                char ch;

                while (1) {
                    Draw(VToR(CP.Row), 1);
                    Redraw();
                    switch (View->MView->Win->Choice(0, "Join Line",
                                                     5,
                                                     "&Yes",
                                                     "&All",
                                                     "&Once",
                                                     "&Skip",
                                                     "&Cancel",
                                                     "Join lines %d and %d?", 1 + VToR(CP.Row), 1 + VToR(CP.Row) + 1))
                    {
                    case 0: ch = 'Y'; break;
                    case 1: ch = 'A'; break;
                    case 2: ch = 'O'; break;
                    case 3: ch = 'N'; break;
                    case 4:
                    case -1:
                    default:
                        ch = 'Q'; break;
                    }
                    if (ch == 'Y') { ask = 'Y'; goto ok_join; }
                    if (ch == 'N') { ask = 'N'; goto ok_join; }
                    if (ch == 'Q') { ask = 'Q'; goto ok_join; }
                    if (ch == 'A') { ask = 'A'; goto ok_join; }
                    if (ch == 'O') { ask = 'O'; goto ok_join; }
                }
            ok_join:
                if (ask == 'N') goto try_delete;
                if (ask == 'Q') goto end;
                if (ask == 'A') Options |= SEARCH_NASK;
            }

            if (JoinLine(Match.Row, Match.Col) == 0) goto error;

            if (ask == 'O')
                goto end;
        }
    try_delete:
        if (Options & SEARCH_DELETE) {
            char ask = 'A';

            if (!(Options & SEARCH_NASK)) {
                char ch;

                while (1) {
                    Draw(VToR(CP.Row), 1);
                    Redraw();
                    switch (View->MView->Win->Choice(0, "Delete Line",
                                                     5,
                                                     "&Yes",
                                                     "&All",
                                                     "&Once",
                                                     "&Skip",
                                                     "&Cancel",
                                                     "Delete line %d?", VToR(CP.Row)))
                    {
                    case 0: ch = 'Y'; break;
                    case 1: ch = 'A'; break;
                    case 2: ch = 'O'; break;
                    case 3: ch = 'N'; break;
                    case 4:
                    case -1:
                    default:
                        ch = 'Q'; break;
                    }
                    if (ch == 'Y') { ask = 'Y'; goto ok_delete; }
                    if (ch == 'N') { ask = 'N'; goto ok_delete; }
                    if (ch == 'Q') { ask = 'Q'; goto ok_delete; }
                    if (ch == 'A') { ask = 'A'; goto ok_delete; }
                    if (ch == 'O') { ask = 'O'; goto ok_delete; }
                }
            ok_delete:
                if (ask == 'N') goto next;
                if (ask == 'Q') goto end;
                if (ask == 'A') Options |= SEARCH_NASK;
            }

            if (Match.Row == RCount - 1) {
                if (DelText(Match.Row, 0, LineLen()) == 0) goto error;
            } else
                if (DelLine(Match.Row) == 0) goto error;

            if (ask == 'O')
                goto end;
            if (!(Options & SEARCH_ALL))
                break;
            goto last;
        }
    next:
        if (!(Options & SEARCH_ALL))
            break;
        Options |= SEARCH_NEXT;
    last:
        ;
    }
end:
    // end of search
    if (R)
        RxFree(R);

    if (Options & SEARCH_ALL)
        Msg(S_INFO, "%d match(es) found.", opt.resCount);
    else {
        if (opt.resCount == 0) {
            Msg(S_INFO, "[%s] not found", opt.strSearch);
            return 0;
        }
    }
    return 1;
error:

    if (R)
    {
        RxFree(R);
    }
    View->MView->Win->Choice(GPC_ERROR, "Find", 1, "O&K", "Error in search/replace.");
    return 0;
}


int EBuffer::CompleteWord() {
#ifdef CONFIG_I_COMPLETE
    return View->MView->Win->ICompleteWord(View);
#else
    PELine L = VLine(CP.Row), M;
    int C, P, X, P1, N, xlen, XL;
    EPoint O = CP;

    if (CP.Col == 0) return 0;

    if (SetPos(CP.Col, CP.Row) == 0) return 0;

    C = CP.Col;
    P = CharOffset(L, C);
    P1 = P;
    N = VToR(CP.Row);

    if (P > L->Count) return 0;

    if (P > 0) {
        while ((P > 0) && ((ChClass(L->Chars[P - 1]) == 1) || (L->Chars[P - 1] == '_'))) P--;
        if (P == P1) return 0;
        xlen = P1 - P;
        C = ScreenPos(L, P);
        Match.Row = N;
        Match.Col = C;
        //if (SetPos(C, CP.Row) == 0) return 0;
        //again:
        if (FindStr(L->Chars + P, xlen, SEARCH_BACK | SEARCH_NEXT | SEARCH_NOPOS | SEARCH_WORDBEG) == 1) {
            M = RLine(Match.Row);
            X = CharOffset(M, Match.Col);
            XL = X;
            //if ((XL > 0) && ((ChClass(M->Chars[XL - 1]) == 1) || (M->Chars[XL - 1] == '_'))) goto again;
            while ((XL < M->Count) && ((ChClass(M->Chars[XL]) == 1) || (M->Chars[XL] == '_'))) XL++;
            {
                char *pp = (char *)malloc(XL - X - xlen);

                if (pp) {
                    memcpy(pp, M->Chars + X + xlen, XL - X - xlen);

                    if (InsText(N, O.Col,
                                XL - X - xlen,
                                pp,
                                1) == 0) return 0;
                    free(pp);
                }
            }
            if (SetPos(O.Col + XL - X - xlen, O.Row) == 0) return 0;
            Draw(Match.Row, Match.Row);
            Match.Row = -1;
            Match.Col = -1;
            MatchLen = 0;
            MatchCount = 0;
            return 1;
        }
    }
    if (Match.Row != -1)
        Draw(Match.Row, Match.Row);
    Match.Col = Match.Row = -1;
    MatchLen = 0;
    MatchCount = 0;
    return 0;
#endif
}

int EBuffer::Search(ExState &State, const char *aString, int Options, int /*CanResume*/) {
    char find[MAXSEARCH+1] = "";
    int Case = BFI(this, BFI_MatchCase) ? 0 : SEARCH_NCASE;
    int Next = 0;
    int erc = 0;
    //int Changed;

    if (aString)
        strcpy(find, aString);
    else
        if (State.GetStrParam(View, find, sizeof(find)) == 0)
            if ((erc = View->MView->Win->GetStr("Find", sizeof(find), find, HIST_SEARCH)) == 0) return 0;
    if (strlen(find) == 0) return 0;

    if (erc == 2)
        Case ^= SEARCH_NCASE;

    //if (Changed == 0 && CanResume)
    //    Next |= SEARCH_NEXT;

    LSearch.ok = 0;
    strcpy(LSearch.strSearch, find);
    LSearch.Options = Case | Next | (Options & ~SEARCH_NCASE);
    LSearch.ok = 1;
    if (Find(LSearch) == 0) return 0;
    return 1;
}

int EBuffer::SearchAgain(ExState &/*State*/, unsigned int Options) {
    if (LSearch.ok == 0) return 0;
    LSearch.Options |= SEARCH_NEXT;
    if ((Options & SEARCH_BACK) != (LSearch.Options & SEARCH_BACK))
        LSearch.Options ^= SEARCH_BACK;
    if (Find(LSearch) == 0) return 0;
    return 1;
}

int EBuffer::SearchReplace(ExState &State, const char *aString, const char *aReplaceString, int Options) {
    char find[MAXSEARCH+1] = "";
    char replace[MAXSEARCH+1] = "";
    int Case = BFI(this, BFI_MatchCase) ? 0 : SEARCH_NCASE;

    if (aString)
        strcpy(find, aString);
    else
        if (State.GetStrParam(View, find, sizeof(find)) == 0)
            if (View->MView->Win->GetStr("Find", sizeof(find), find, HIST_SEARCH) == 0) return 0;
    if (strlen(find) == 0) return 0;
    if (aReplaceString)
        strcpy(replace, aReplaceString);
    else
        if (State.GetStrParam(View, replace, sizeof(replace)) == 0)
            if (View->MView->Win->GetStr("Replace", sizeof(replace), replace, HIST_SEARCH) == 0) return 0;

    LSearch.ok = 0;
    strcpy(LSearch.strSearch, find);
    strcpy(LSearch.strReplace, replace);
    LSearch.Options = Case | (Options & ~SEARCH_NCASE) | SEARCH_ALL | SEARCH_REPLACE;
    LSearch.ok = 1;
    if (Find(LSearch) == 0) return 0;
    return 1;
}

int EBuffer::Search(ExState &State)          { return Search(State, 0, 0, 1); }
int EBuffer::SearchB(ExState &State)         { return Search(State, 0, SEARCH_BACK, 1); }
int EBuffer::SearchRx(ExState &State)        { return Search(State, 0, SEARCH_RE, 1); }
int EBuffer::SearchAgain(ExState &State)     { return SearchAgain(State, 0); }
int EBuffer::SearchAgainB(ExState &State)    { return SearchAgain(State, SEARCH_BACK); }
int EBuffer::SearchReplace(ExState &State)   { return SearchReplace(State, 0, 0, 0); }
int EBuffer::SearchReplaceB(ExState &State)  { return SearchReplace(State, 0, 0, SEARCH_BACK); }
int EBuffer::SearchReplaceRx(ExState &State) { return SearchReplace(State, 0, 0, SEARCH_RE); }

#ifdef CONFIG_OBJ_ROUTINE
int EBuffer::ScanForRoutines() {
    RxNode *regx;
    int line;
    PELine L;
    RxMatchRes res;

    if (BFS(this, BFS_RoutineRegexp) == 0) {
        View->MView->Win->Choice(GPC_ERROR, "Error", 1, "O&K", "No routine regexp.");
        return 0;
    }
    regx = RxCompile(BFS(this, BFS_RoutineRegexp));
    if (regx == 0) {
        View->MView->Win->Choice(GPC_ERROR, "Error", 1, "O&K", "Failed to compile regexp '%s'", BFS(this, BFS_RoutineRegexp));
        return 0;
    }

    if (rlst.Lines) {
        free(rlst.Lines);
        rlst.Lines = 0;
    }
    rlst.Lines = 0;
    rlst.Count = 0;

    Msg(S_BUSY, "Matching %s", BFS(this, BFS_RoutineRegexp));
    for (line = 0; line < RCount; line++) {
        L = RLine(line);
        if (RxExec(regx, L->Chars, L->Count, L->Chars, &res) == 1) {
            rlst.Count++;
            rlst.Lines = (int *) realloc((void *) rlst.Lines, sizeof(int) * (rlst.Count | 0x1F));
            rlst.Lines[rlst.Count - 1] = line;
            Msg(S_BUSY, "Routines: %d", rlst.Count);
        }
    }
    RxFree(regx);
    return 1;
}
#endif

int EBuffer::ShowPosition() {
    int CLine, NLines;
    int CAct, NAct;
    int CColumn, NColumns;
    int CCharPos, NChars;
#ifdef HEAPWALK
    unsigned long MemUsed = 0, MemFree = 0, BlkUsed = 0, BlkFree = 0, BigFree = 0, BigUsed = 0;
#endif

    if (!View)
        return 0;

    CLine = CP.Row + 1;
    NLines = VCount;
    CAct = VToR(CP.Row) + 1;
    NAct = RCount;
    CColumn = CP.Col + 1;
    NColumns = LineLen(CP.Row);
    CCharPos = CharOffset(VLine(CP.Row), CP.Col) + 1;
    NChars = VLine(CP.Row)->Count;

#ifdef HEAPWALK
    if (_heapchk() != _HEAPOK) {
        MemUsed = -1;
    } else {
        _HEAPINFO hi;

        hi._pentry = NULL;
        while (_heapwalk(&hi) == _HEAPOK) {
            if (hi._useflag == _USEDENTRY) {
                BlkUsed++;
                MemUsed += hi._size;
                if (hi._size > BigUsed)
                    BigUsed = hi._size;
                //fprintf(stderr, "USED %d\n", hi._size);
            } else {
                BlkFree++;
                MemFree += hi._size;
                if (hi._size > BigFree)
                    BigFree = hi._size;
                //fprintf(stderr, "FREE %d\n", hi._size);
            }
        }
    }
#endif

#ifdef CONFIG_UNDOREDO
    int NN = -1;
    if (US.UndoPtr > 0)
        NN = US.Top[US.UndoPtr - 1];
#endif
    Msg(S_INFO,
#ifdef HEAPWALK
        "M%ld,%ld B%ld,%ld S%ld,%ld"
#endif
        "L%d/%d G%d/%d/%d A%d/%d C%d/%d P%d/%d "
#ifdef CONFIG_UNDOREDO
        "U%d/%d/%d "
#endif
        "H%d/%d/%d",
#ifdef HEAPWALK
        MemUsed, MemFree, BlkUsed, BlkFree, BigUsed, BigFree,
#endif
        CLine, NLines,
        RGap, RCount, RAllocated,
        CAct, NAct,
        CColumn, NColumns,
        CCharPos, NChars,
#ifdef CONFIG_UNDOREDO
        US.UndoPtr, US.Num, NN,
#endif
        StartHilit, MinRedraw, MaxRedraw);
    return 1;
}

#ifdef CONFIG_BOOKMARKS
int EBuffer::PlaceBookmark(const char *Name, const EPoint &P) {
    assert(P.Row >= 0 && P.Row < RCount && P.Col >= 0);

    vector_iterate(EBookmark*, BMarks, it)
        if ((*it)->IsName(Name)) {
	    (*it)->SetPoint(P);
            return 1;
        }

    EBookmark* b = new EBookmark(Name, P);
    BMarks.push_back(b);

    return 1;
}

int EBuffer::RemoveBookmark(const char *Name) {
    vector_iterate(EBookmark*, BMarks, it)
	if ((*it)->IsName(Name)) {
	    delete (*it);
	    BMarks.erase(it);
            return 1;
        }

    View->MView->Win->Choice(GPC_ERROR, "RemoveBookmark", 1, "O&K", "Bookmark %s not found.", Name);
    return 0;
}

int EBuffer::GetBookmark(const char *Name, EPoint &P) {
    vector_iterate(EBookmark*, BMarks, it)
        if ((*it)->IsName(Name)) {
            P = (*it)->GetPoint();
            return 1;
        }
    return 0;
}

/*
 * Searches bookmark list starting at given index (searchFrom) for next
 * bookmark for line searchForLine. It then returns its name and position
 * and index (used for next search) or -1 if none found. Name is pointer
 * directly into bookmark structure (not copied!). If searchForLine is -1,
 * this function returns any next bookmark -> can be used to enumerate
 * bookmarks.
 */
int EBuffer::GetBookmarkForLine(int searchFrom, int searchForLine, const EBookmark* &b) {
    for (int i = searchFrom; i < (int)BMarks.size(); i++)
	if (searchForLine == -1 || BMarks[i]->GetPoint().Row == searchForLine) {
            b = BMarks[i];
            return i + 1;
        }
    return -1;
}

int EBuffer::GotoBookmark(const char *Name) {
    vector_iterate(EBookmark*, BMarks, it)
        if ((*it)->IsName(Name))
            return CenterNearPosR((*it)->GetPoint().Col, (*it)->GetPoint().Row);

    View->MView->Win->Choice(GPC_ERROR, "GotoBookmark", 1, "O&K", "Bookmark %s not found.", Name);
    return 0;
}
#endif

int EBuffer::GetMatchBrace(EPoint &M, int MinLine, int MaxLine, int show) {
    int StateLen;
    hsState *StateMap = 0;
    int Pos;
    PELine L = VLine(M.Row);
    int dir = 0;
    hsState State;
    char Ch1, Ch2;
    int CountX = 0;
    int StateRow = -1;

    M.Row = VToR(CP.Row);

    Pos = CharOffset(L, M.Col);
    if (Pos >= L->Count) return 0;
    switch(L->Chars[Pos]) {
    case '{': dir = +1; Ch1 = '{'; Ch2 = '}'; break;
    case '[': dir = +1; Ch1 = '['; Ch2 = ']'; break;
    case '<': dir = +1; Ch1 = '<'; Ch2 = '>'; break;
    case '(': dir = +1; Ch1 = '('; Ch2 = ')'; break;
    case '}': dir = -1; Ch1 = '}'; Ch2 = '{'; break;
    case ']': dir = -1; Ch1 = ']'; Ch2 = '['; break;
    case '>': dir = -1; Ch1 = '>'; Ch2 = '<'; break;
    case ')': dir = -1; Ch1 = ')'; Ch2 = '('; break;
    default:
        return 0;
    }
    StateMap = 0;
    if (GetMap(M.Row, &StateLen, &StateMap) == 0) return 0;
    State = StateMap[Pos];
    StateRow = M.Row;

    while (M.Row >= MinLine && M.Row < MaxLine) {
        while (Pos >= 0 && Pos < L->Count) {
            if (L->Chars[Pos] == Ch1 || L->Chars[Pos] == Ch2) {
                // update syntax state if needed
                if (StateRow != M.Row) {
                    free(StateMap);
                    StateMap = 0;
                    GetMap(M.Row, &StateLen, &StateMap);
                    if (StateMap == 0) return 0;
                    StateRow = M.Row;
                }
                if (StateMap[Pos] == State) {
                    if (L->Chars[Pos] == Ch1) CountX++;
                    if (L->Chars[Pos] == Ch2) CountX--;
                    if (CountX == 0) {
                        M.Col = ScreenPos(L, Pos);
                        free(StateMap);
                        return 1;
                    }
                }
            }
            Pos += dir;
        }
        M.Row += dir;
        if (M.Row >= 0 && M.Row < RCount) {
            L = RLine(M.Row);
            Pos = (dir == 1) ? 0 : (L->Count - 1);
        }
    }
    if (StateMap) free(StateMap);
    if (show)
        Msg(S_INFO, "No match (%d missing).", CountX);
    return 0;
}

int EBuffer::MatchBracket() {
    EPoint M = CP;

    if (GetMatchBrace(M, 0, RCount, 1) == 1)
        return SetPosR(M.Col, M.Row);
    return 0;
}

int EBuffer::HilitMatchBracket() {
    EPoint M = CP;

    if (View == 0)
        return 0;

    int Min = VToR(GetVPort()->TP.Row);
    int Max = GetVPort()->TP.Row + GetVPort()->Rows;
    if (Max >= VCount)
        Max = RCount;
    else
        Max = VToR(Max);
    if (Min < 0)
        Min = 0;
    if (Max < Min)
        return 0;
    if (GetMatchBrace(M, Min, Max, 0) == 1) {
        Match = M;
        MatchLen = 1;
        MatchCount = 1;
        Draw(Match.Row, Match.Row);
        return 1;
    }
    return 0;
}

int EBuffer::SearchWord(int SearchFlags) {
    char word[MAXSEARCH + 1];
    PELine L = VLine(CP.Row);
    int P, len = 0;
    int Case = BFI(this, BFI_MatchCase) ? 0 : SEARCH_NCASE;

    P = CharOffset(L, CP.Col);
    while ((P > 0) && ((ChClass(L->Chars[P - 1]) == 1) || (L->Chars[P - 1] == '_')))
        P--;
    while (len < int(sizeof(word)) && P < L->Count && (ChClass(L->Chars[P]) == 1 || L->Chars[P] == '_'))
        word[len++] = L->Chars[P++];
    word[len] = 0;
    if (len == 0)
        return 0;

    return FindStr(word, len, Case | SearchFlags | SEARCH_WORD);
}

int EBuffer::FindTagWord(ExState &State) {
    char word[MAXSEARCH + 1];
    PELine L = VLine(CP.Row);
    int P, len = 0;

    P = CharOffset(L, CP.Col);
    while ((P > 0) && ((ChClass(L->Chars[P - 1]) == 1) || (L->Chars[P - 1] == '_')))
        P--;
    while (len < int(sizeof(word)) && P < L->Count && (ChClass(L->Chars[P]) == 1 || L->Chars[P] == '_'))
        word[len++] = L->Chars[P++];
    word[len] = 0;
    if (len == 0) {
        Msg(S_INFO, "No word at cursor.");
        return 0;
    }

    for (int j = 2; j; j--) {
        if (TagFind(this, View, word))
            return 1;

        if (j == 2) {
            /* Try autoload tags */
            if (!View->ExecCommand(ExTagLoad, State))
                break;
        } else {
            Msg(S_INFO, "Tag '%s' not found.", word);
            break;
        }
    }

    return 0;
}
// *INDENT-ON*
