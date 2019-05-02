/*    e_line.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "o_buflist.h"
#include "sysdep.h"

ELine::ELine(size_t ACount, const char *AChars) :
    Count(ACount),
    Chars(NULL)
#ifdef CONFIG_SYNTAX_HILIT
    , StateE(0)
#endif
{
    Allocate(Count);
    if (AChars)
        memcpy(Chars, AChars, Count);
    else
        memset(Chars, ' ', Count);
}

ELine::ELine(char *AChars, size_t ACount) :
    Count(ACount),
    Chars(AChars)
#ifdef CONFIG_SYNTAX_HILIT
    , StateE(0)
#endif
{
}

ELine::~ELine() {
    free(Chars);
}

int ELine::Allocate(size_t Bytes) {
    Chars = (char*)realloc(Chars, Bytes | CHAR_TRESHOLD);
    return (Chars != NULL);
}

int EBuffer::ScreenPos(ELine *L, int Offset) {
    int ExpandTabs = BFI(this, BFI_ExpandTabs);
    int TabSize = BFI(this, BFI_TabSize);

    if (!ExpandTabs)
        return Offset;

    char *p = L->Chars;
    int Len = L->Count;
    int Pos = 0;
    int Ofs = Offset;

    if (Ofs > Len) {
        while (Len > 0) {
            if (*p++ != '\t')
                Pos++;
            else
                Pos = NextTab(Pos, TabSize);
            Len--;
        }
        Pos += Ofs - L->Count;
    } else
        while (Ofs > 0) {
            if (*p++ != '\t')
                Pos++;
            else
                Pos = NextTab(Pos, TabSize);
            Ofs--;
        }

    return Pos;
}

int EBuffer::CharOffset(ELine *L, int ScreenPos) {
    int ExpandTabs = BFI(this, BFI_ExpandTabs);
    int TabSize = BFI(this, BFI_TabSize);

    if (!ExpandTabs)
        return ScreenPos;

    int Pos = 0;
    int Ofs = 0;
    char *p = L->Chars;
    int Len = L->Count;

    while (Len > 0) {
        if (*p++ != '\t')
            Pos++;
        else
            Pos = NextTab(Pos, TabSize);
        if (Pos > ScreenPos)
            return Ofs;
        Ofs++;
        Len--;
    }

    return Ofs + ScreenPos - Pos;
}

int EBuffer::Allocate(int ACount) {
    PELine *L;

    if (ACount > 10 * 1024 * 1024) // FIXME: - let's crash, before OOOM kills us
        return 0;

    L = (PELine *) realloc(LL, sizeof(PELine) * (ACount + 1));
    if (!L && ACount != 0)
        return 0;
    RAllocated = ACount;
    LL = L;

    return 1;
}

int EBuffer::MoveRGap(int RPos) {
    int GapSize = RAllocated - RCount;

    if (RGap == RPos)
        return 1;
    if (RPos < 0 || RPos > RCount)
        return 0;

    if (RGap < RPos) {
        if (RPos - RGap == 1)
            LL[RGap] = LL[RGap + GapSize];
        else
            memmove(LL + RGap,
                LL + RGap + GapSize,
                sizeof(PELine) * (RPos - RGap));
    } else {
        if (RGap - RPos == 1)
            LL[RPos + GapSize] = LL[RPos];
        else
            memmove(LL + RPos + GapSize,
                    LL + RPos,
                    sizeof(PELine) * (RGap - RPos));
    }
    RGap = RPos;

    return 1;
}

int EBuffer::AllocVis(int ACount) {
    int *V = (int *) realloc(VV, sizeof(int) * (ACount + 1));
    if (!V && ACount != 0)
        return 0;
    VAllocated = ACount;
    VV = V;

    return 1;
}

int EBuffer::MoveVGap(int VPos) {
    int GapSize = VAllocated - VCount;

    if (VGap == VPos) return 1;
    if (VPos < 0 || VPos > VCount) return 0;

    if (VGap < VPos) {
        if (VPos - VGap == 1)
            VV[VGap] = VV[VGap + GapSize];
        else
            memmove(VV + VGap,
                    VV + VGap + GapSize,
                    sizeof(VV[0]) * (VPos - VGap));
    } else {
        if (VGap - VPos == 1) {
            VV[VPos + GapSize] = VV[VPos];
        } else {
            memmove(VV + VPos + GapSize,
                    VV + VPos,
                    sizeof(VV[0]) * (VGap - VPos));
        }
    }
    VGap = VPos;
    return 1;
}

int EBuffer::RToV(int No) {
    int L = 0, R = VCount, M, V;

    if (No > Vis(VCount - 1) + VCount - 1)   // beyond end
        return -1;
    if (No < VCount) // no folds before (direct match)
        if (Vis(No) == 0) return No;

    while (L < R) {
        M = (L + R) / 2;
        V = Vis(M) + M;
        if (V == No)
            return M;
        if (V > No)
            R = M;
        else
            L = M + 1;
    }
    return -1;
}

int EBuffer::RToVN(int No) {
    int L = 0, R = VCount, M, V;

    if (No == RCount)
        return VCount;
    if (No > Vis(VCount - 1) + VCount - 1)
        return VCount - 1;
    if (No < VCount)
        if (Vis(No) == 0) return No;

    while (L < R) {
        M = (L + R) / 2;
        V = Vis(M) + M;
        if (V == No)
            return M;
        else if (V > No)
            R = M;
        else {
            if (M == VCount - 1)
                return M;
            else if (Vis(M + 1) + M + 1 > No)
                return M;
            L = M + 1;
        }
    }
    return R;
}
