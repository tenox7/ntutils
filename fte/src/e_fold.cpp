/*    e_fold.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "e_undo.h"
#include "o_buflist.h"
#include "sysdep.h"

int EBuffer::FindFold(int Line) { // optimize /*FOLD00*/
    int f = FindNearFold(Line);
    if (f != -1)
        if (FF[f].line == Line)
            return f;
    return -1;
}

int EBuffer::FindNearFold(int Line) { /*FOLD00*/
    int b = 0, B = FCount - 1, c;

    while (b <= B) {
        c = (b + B) / 2;
//        printf("%d %d %d %d %d\n", b, B, c, Line, FF[c].line);
        if (FF[c].line == Line)
            return c;
        if (c < FCount - 1) {
            if (FF[c].line <= Line && FF[c + 1].line > Line)
                return c;
        } else {
            if (FF[c].line <= Line)
                return c;
        }
        if (FF[c].line < Line)
            b = c + 1;
        else
            B = c - 1;
        if (b > B)
            break;
    }
    return -1;
}

int EBuffer::ShowRow(int Row) { /*FOLD00*/
    int V = RToVN(Row), GapSize;

//    printf("Showing row %d\n", Row);

    assert(Row >= 0 && Row < RCount); // 0 cannot be hidden

    if (V + Vis(V) == Row) return 1; // already visible

    assert(VCount <= VAllocated);
    if (VCount == VAllocated) {
        if (AllocVis(VCount ? (VCount * 2) : 1) == 0) return 0;
        memmove(VV + VAllocated - (VCount - VGap),
                VV + VGap,
                sizeof(int) * (VCount - VGap));
    }
    if (VGap != V + 1)
        if (MoveVGap(V + 1) == 0) return 0;
    VV[VGap] = Row - (VGap);
    VGap++;
    VCount++;

    GapSize = VAllocated - VCount;
    if (VGap != V + 2)
        if (MoveVGap(V + 2) == 0) return 0;
    for (int i = V + 2; i < VCount; i++)
        VV[i + GapSize]--;
//        Vis(i, Vis(i) - 1);
    UpdateVisible(Row, 1);
//    if (CP.Row > Row)
//        if (SetPos(CP.Col, CP.Row + 1) == 0) return 0;
    Draw(Row, -1);
    return 1;
}

int EBuffer::HideRow(int Row) { /*FOLD00*/
    int V = RToV(Row), GapSize;

    assert(Row > 0 && Row < RCount); // 0 cannot be hidden

    if (V == -1) return 1; // already hidden
    UpdateVisible(Row, -1);

    if (VGap != V)
        if (MoveVGap(V) == 0) return 0;
    GapSize = VAllocated - VCount;
    VV[VGap + GapSize] = 0;
    VCount--;
    GapSize++;
    if (VAllocated - VAllocated / 2 > VCount) {
        memmove(VV + VGap + GapSize - VAllocated / 3,
                VV + VGap + GapSize,
                sizeof(int) * (VCount - VGap));
        if (AllocVis(VAllocated - VAllocated / 3) == 0) return 0;
    }
    GapSize = VAllocated - VCount;
    if (VGap != V)
        if (MoveVGap(V) == 0) return 0;
    for (int i = V; i < VCount; i++)
        VV[i + GapSize]++;
//        Vis(i, Vis(i) + 1);
//    if (CP.Row > Row)
//        if (SetPos(CP.Col, CP.Row - 1) == 0) return 0;
    Draw(Row, -1);
    return 1;
}

int EBuffer::ExposeRow(int Row) { /*FOLD00*/
    int V;
    int f, level, oldlevel = 100;

    //DumpFold();

    assert(Row >= 0 && Row < RCount); // range

    V = RToV(Row);
    if (V != -1) return 1; // already exposed

    f = FindNearFold(Row);
    assert(f != -1); // if not visible, must be folded

    while (f >= 0) {
        level = FF[f].level;
        if (level < oldlevel) {
            if (FF[f].open == 0) {
//                printf("opening fold %d\n", f);
                if (FoldOpen(FF[f].line) == 0) return 0;
            }
            oldlevel = level;
        }
        f--;
        if (level == 0) break;
    }

    V = RToV(Row);
//    if (V == -1) {
//        printf("Expose Row = %d\n", Row);
//        DumpFold();
//    }
    assert (V != -1);
    return 1;
}

void EBuffer::UpdateVis(EPoint &M, int Row, int Delta) { /*FOLD00*/
    if (Delta < 0) {
        if (M.Row > Row) {
            if (M.Row < Row - Delta)
                M.Row = Row;
            else
                M.Row += Delta;
        }
    } else if (M.Row >= Row)
        M.Row += Delta;
}

void EBuffer::UpdateVisible(int Row, int Delta) { /*FOLD00*/
    EView *w;

    Row = RToV(Row);
    UpdateVis(CP, Row, Delta);
    w = View;
    if (w) do {
        UpdateVis(GetViewVPort(w)->TP, Row, Delta);
        UpdateVis(GetViewVPort(w)->CP, Row, Delta);
        w = w->Next;
    } while (w != View);
}

int EBuffer::FoldCreate(int Line) { /*FOLD00*/
    int n;

    if (Modify() == 0) return 0;

    if (FindFold(Line) != -1) return 1; // already exists

#ifdef CONFIG_UNDOREDO
    if (BFI(this, BFI_Undo)) {
        if (PushULong(Line) == 0) return 0;
        if (PushUChar(ucFoldCreate) == 0) return 0;
    }
#endif

    n = FindNearFold(Line);
    n++;
    FF = (EFold *) realloc((void *)FF, sizeof(EFold) * ((1 + FCount) | 7));
    assert(FF != 0);
    memmove(FF + n + 1, FF + n, sizeof(EFold) * (FCount - n));
    FCount++;
    FF[n].line = Line;
    FF[n].level = 0;
    FF[n].open = 1;
    FF[n].flags = 0;
    Draw(Line, Line);
    return 1;
}

int EBuffer::FoldCreateByRegexp(const char *Regexp) { /*FOLD00*/
    RxNode *R;
    int err = 1;

    if (Modify() == 0) return 0;

    R = RxCompile(Regexp);
    if (R != NULL) {
        PELine X;
        int first = -1;
        int L;

        for (L = 0; L < RCount; L++) {
            RxMatchRes RM;

            X = RLine(L);
            if (RxExec(R, X->Chars, X->Count, X->Chars, &RM) == 1) {
                if (first >= 0) {
                    int i;

                    for(i = L; i > 0; i--) {
                        PELine Y;

                        Y = RLine(i);
                        if ((Y->Count == 0) || strrchr(Y->Chars, '}')) {
                            if ((L - i) > 2) {
                                while ((i > 0) && (RLine(i - 1)->Count == 0))
                                    i--;
                                if ((first >= 0) && i
                                    && (FoldCreate(i) == 0))
                                    err = 0;
                            }
                            break;
                        }
                    }
                } else
                    first = L;
                if (FoldCreate(L) == 0) {
                    err = 0;
                    break;
                }
            }
        }
        RxFree(R);
    }
    return err;
}

int EBuffer::FoldCreateAtRoutines() { /*FOLD00*/
    if (BFS(this, BFS_RoutineRegexp) == 0)
        return 0;
    return FoldCreateByRegexp(BFS(this, BFS_RoutineRegexp));
}

int EBuffer::FoldDestroy(int Line) { /*FOLD00*/
    int f = FindFold(Line);

    if (Modify() == 0) return 0;

    if (f == -1) return 0;
    if (FF[f].open == 0)
        if (FoldOpen(Line) == 0) return 0;

#ifdef CONFIG_UNDOREDO
    if (BFI(this, BFI_Undo)) {
        if (PushULong(FF[f].level) == 0) return 0;
        if (PushULong(Line) == 0) return 0;
        if (PushUChar(ucFoldDestroy) == 0) return 0;
    }
#endif

    memmove(FF + f, FF + f + 1, sizeof(EFold) * (FCount - f - 1));
    FCount--;
    FF = (EFold *) realloc((void *)FF, sizeof(EFold) * (FCount | 7));
    Draw(Line, Line);
    return 1;
}

int EBuffer::FoldDestroyAll() { /*FOLD00*/
    int l;

    if (Modify() == 0) return 0;

    for (l = 0; l < RCount; l++)
        if (FindFold(l) != -1)
            if (FoldDestroy(l) == 0) return 0;
    return 1;
}

int EBuffer::FoldPromote(int Line) { /*FOLD00*/
    int f = FindFold(Line);

    if (Modify() == 0) return 0;

    if (f == -1) return 0;
    if (FF[f].open == 0) return 0;
    if (FF[f].level == 0) return 0;

#ifdef CONFIG_UNDOREDO
    if (BFI(this, BFI_Undo)) {
        if (PushULong(Line) == 0) return 0;
        if (PushUChar(ucFoldPromote) == 0) return 0;
    }
#endif

    if ((FF[f].line > 0) && (ExposeRow(FF[f].line - 1) == 0))
        return 0;

    FF[f].level--;
    Draw(Line, Line);
    return 1;
}

int EBuffer::FoldDemote(int Line) { /*FOLD00*/
    int f = FindFold(Line);

    if (Modify() == 0) return 0;

    if (f == -1) return 0;
    if (FF[f].open == 0) return 0;
    if (FF[f].level == 99) return 0;

#ifdef CONFIG_UNDOREDO
    if (BFI(this, BFI_Undo)) {
        if (PushULong(Line) == 0) return 0;
        if (PushUChar(ucFoldDemote) == 0) return 0;
    }
#endif
    if ((FF[f].line > 0) && (ExposeRow(FF[f].line - 1) == 0))
        return 0;

    FF[f].level++;
    Draw(Line, Line);
    return 1;
}

int EBuffer::FoldOpen(int Line) { /*FOLD00*/
    int f = FindFold(Line);
    int l;
    int level, toplevel;
    int top;

    if (f == -1) return 0;
    if (FF[f].open == 1) return 1; // already open

    if (Modify() == 0) return 0;

#ifdef CONFIG_UNDOREDO
    if (BFI(this, BFI_Undo)) {
        if (PushULong(Line) == 0) return 0;
        if (PushUChar(ucFoldOpen) == 0) return 0;
    }
#endif

    FF[f].open = 1;
    top = FF[f].line;
    toplevel = FF[f].level;
    //    printf("Fold starts with %d\n", FF[f].line);
    if (ShowRow(FF[f].line) == 0) return 0;
    while (f < FCount) {
        level = FF[f].level;
        if (FF[f].open == 1) {
            // fold is open
            if (f == FCount - 1) {
                for (l = FF[f].line; l < RCount; l++)
                    if (l != top)
                        if (ShowRow(l) == 0) return 0;
            } else {
                for (l = FF[f].line; l < FF[f + 1].line; l++)
                    if (l != top)
                        if (ShowRow(l) == 0) return 0;
            }
            f++;
        } else { // fold is closed
            // show head line
            if (ShowRow(FF[f].line) == 0) return 0;
            // skip closed folds
            while ((f < FCount) && (level < FF[f + 1].level))
                f++;
            f++;
        }
        if (f < FCount && FF[f].level <= toplevel)
            break;
    }
    return 1;
}

int EBuffer::FoldOpenAll() { /*FOLD00*/
    for (int l = 0; l < RCount; ++l)
        if ((FindFold(l) != -1)
            && !FoldOpen(l))
            return 0;

    return 1;
}

int EBuffer::FoldOpenNested() { /*FOLD00*/
    int Line = VToR(CP.Row);
    int f = FindFold(Line);
    int l;
    int level;

    if (f == -1) return 0;
    level = FF[f].level;

    while (f + 1 < FCount && FF[f + 1].level > level) f++;

    if (f + 1 == FCount) {
        if (FoldOpen(Line) == 0) return 0;
    } else {
        for (l = Line; l < RCount && l < FF[f + 1].line; l++) {
            if (FindFold(l) != -1)
                if (FoldOpen(l) == 0) return 0;
        }
    }
    return 0;
}

int EBuffer::FoldClose(int Line) { /*FOLD00*/
    int f = FindNearFold(Line);
    int l, top;
    int level;

    if (f == -1) return 0;
    if (FF[f].open == 0) return 1; // already closed

    if (Modify() == 0) return 0;

    if (SetPosR(CP.Col, FF[f].line, tmLeft) == 0) return 0;

#ifdef CONFIG_UNDOREDO
    if (BFI(this, BFI_Undo)) {
        if (PushULong(Line) == 0) return 0;
        if (PushUChar(ucFoldClose) == 0) return 0;
    }
#endif

    FF[f].open = 0;
    top = FF[f].line;
    level = FF[f].level;
    while ((f < FCount - 1) && (FF[f + 1].level > level)) f++;

    /* performance tweak: do it in reverse (we'll see if it helps) */

    if (f == FCount - 1) {
        for (l = RCount - 1; l > top; l--)
            if (HideRow(l) == 0) return 0;
    } else {
        for (l = FF[f + 1].line - 1; l > top; l--)
            if (HideRow(l) == 0) return 0;
    }

    /* yup, it does. try below for a (MUCH!) slower version */

    /*if (f == FCount - 1) {
     for (l = top + 1; l < RCount; l++)
     if (HideRow(l) == 0) return 0;
     } else {
     for (l = top + 1; l < FF[f + 1].line; l++)
     if (HideRow(l) == 0) return 0;
     }*/
    return 1;
}

int EBuffer::FoldCloseAll() { /*FOLD00*/
    for (int l = RCount - 1; l >= 0; --l)
        if ((FindFold(l) != -1)
            && !FoldClose(l))
            return 0;
    return 1;
}

int EBuffer::FoldToggleOpenClose() { /*FOLD00*/
    int Line = VToR(CP.Row);
    int f = FindNearFold(Line);

    if (f == -1)
        return 0;

    if (FF[f].open) {
        if (!FoldClose(Line))
            return 0;
    } else {
        if (!FoldOpen(Line))
            return 0;
    }

    return 1;
}

int EBuffer::MoveFoldTop() { /*FOLD00*/
    int f = FindNearFold(VToR(CP.Row));

    if (f <= 0)
        return 0;

    if (FF[f].line == VToR(CP.Row))
        return 1;

    return SetPosR(CP.Col, FF[f].line, tmLeft);
}

int EBuffer::MoveFoldPrev() { /*FOLD00*/
    int f = FindNearFold(VToR(CP.Row));

    if (f <= 0)
        return 0;

    if (FF[f].line == VToR(CP.Row)) {
        for (;;) {
            if (--f < 0)
                return 0;
            if (RToV(FF[f].line) != -1)
                break;
        }
    }

    return SetPosR(CP.Col, FF[f].line, tmLeft);
}

int EBuffer::MoveFoldNext() { /*FOLD00*/
    int f = FindNearFold(VToR(CP.Row));

    if ((f == (FCount - 1)) || (f == -1))
        return 0;

    while (++f < FCount)
        if (RToV(FF[f].line) != -1)
            return SetPosR(CP.Col, FF[f].line, tmLeft);

    return 0;
}
