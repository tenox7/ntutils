/*    e_block.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "c_config.h"
#include "e_undo.h"
#include "i_modelview.h"
#include "i_view.h"
#include "o_buflist.h"
#include "s_files.h"
#include "s_util.h"

#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////
// Block Commands                                                            //
///////////////////////////////////////////////////////////////////////////////

int EBuffer::SetBB(const EPoint& M) {
    EPoint OldBB = BB;
    int MinL, MaxL;

    if (BB.Row == M.Row && BB.Col == M.Col) return 1;
#ifdef CONFIG_UNDOREDO
    if (PushBlockData() == 0) return 0;
#endif
    BB = M;
    if (OldBB.Row == -1) OldBB = BE;
    if ((OldBB.Col != BB.Col) && (BlockMode == bmColumn)) BlockRedraw();
    MinL = Min(OldBB.Row, BB.Row);
    MaxL = Max(OldBB.Row, BB.Row);
    if (MinL != -1)
        if (MinL <= MaxL) Draw(MinL, MaxL);
    return 1;
}

int EBuffer::SetBE(const EPoint& M) {
    EPoint OldBE = BE;
    int MinL, MaxL;

    if (BE.Row == M.Row && BE.Col == M.Col) return 1;
#ifdef CONFIG_UNDOREDO
    if (PushBlockData() == 0) return 0;
#endif
    BE = M;
    if (OldBE.Row == -1) OldBE = BB;
    if ((OldBE.Col != BE.Col) && (BlockMode == bmColumn)) BlockRedraw();
    MinL = Min(OldBE.Row, BE.Row);
    MaxL = Max(OldBE.Row, BE.Row);
    if (MinL != -1)
        if (MinL <= MaxL) Draw(MinL, MaxL);
    return 1;
}

// check if there is active or valid selection
int EBuffer::CheckBlock() {
    if (BB.Row == -1 && BE.Row == 1) {
        BB.Col = -1;
        BE.Col = -1;
        return 0;
    }
    if (BB.Row == -1 || BE.Row == -1) return 0;
    if (BB.Row >= RCount) BB.Row = RCount - 1;
    if (BE.Row >= RCount) BE.Row = RCount - 1;
    switch(BlockMode) {
    case bmLine:
        BB.Col = 0;
        BE.Col = 0;
        if (BB.Row >= BE.Row) return 0;
        break;
    case bmColumn:
        if (BB.Col >= BE.Col) return 0;
        if (BB.Row >= BE.Row) return 0;
        break;
    case bmStream:
        if (BB.Row > BE.Row) return 0;
        if (BB.Row == BE.Row && BB.Col >= BE.Col) return 0;
        break;
    }
    return 1;
}

int EBuffer::BlockRedraw() {
    if (BB.Row == -1 || BE.Row == -1) return 0;
    Draw(BB.Row, BE.Row);
    return 1;
}


int EBuffer::BlockBegin() {
    EPoint X;

    X.Row = VToR(CP.Row);
    X.Col = CP.Col;
    CheckBlock();
    SetBB(X);
    return 1;
}

int EBuffer::BlockEnd() {
    EPoint X;

    X.Row = VToR(CP.Row);
    X.Col = CP.Col;
    CheckBlock();
    SetBE(X);
    return 1;
}

int EBuffer::BlockUnmark() {
    EPoint Null(-1,-1);

    SetBB(BE);
    SetBE(Null);
    SetBB(Null);
    AutoExtend = 0;
    return 1;
}

int EBuffer::BlockCut(int Append) {
    if (BlockCopy(Append) && BlockKill()) return 1;
    return 0;
}

int EBuffer::BlockCopy(int Append, int clipboard) {
    EPoint B, E;
    int L;
    int SL, OldCount;

    AutoExtend = 0;
    if (CheckBlock() == 0) return 0;
    if (RCount == 0) return 0;
    if (SSBuffer == 0) return 0;
    if (Append) {
        if (SystemClipboard)
            GetPMClip(clipboard);
    } else
        SSBuffer->Clear();
    SSBuffer->BlockMode = BlockMode;
    BFI(SSBuffer, BFI_TabSize) = BFI(this, BFI_TabSize);
    BFI(SSBuffer, BFI_ExpandTabs) = BFI(this, BFI_ExpandTabs);
    BFI(SSBuffer, BFI_Undo) = 0;
    B = BB;
    E = BE;
    OldCount = SL = SSBuffer->RCount;
    switch (BlockMode) {
    case bmLine:
        for (L = B.Row; L < E.Row; L++) {
            if (SSBuffer->InsLine(SL, 0) == 0) return 0;
            if (SSBuffer->InsLineText(SL, 0, -1, 0, RLine(L)) == 0) return 0;
            SL++;
        }
        break;

    case bmColumn:
        for (L = B.Row; L < E.Row; L++) {
            if (SSBuffer->InsLine(SL, 0) == 0) return 0;
            if (SSBuffer->InsLineText(SL, 0, E.Col - B.Col, B.Col, RLine(L)) == 0) return 0;
            if (SSBuffer->PadLine(SL, E.Col - B.Col) == 0) return 0;
            SL++;
        }
        break;

    case bmStream:
        if (B.Row == E.Row) {
            if (SSBuffer->InsLine(SL, 0) == 0) return 0;
            if (SSBuffer->InsLineText(SL, 0, E.Col - B.Col, B.Col, RLine(B.Row)) == 0) return 0;
        } else {
            if (SSBuffer->InsLine(SL, 0) == 0) return 0;
            if (SSBuffer->InsLineText(SL, 0, -1, B.Col, RLine(B.Row)) == 0) return 0;
            SL++;
            for (L = B.Row + 1; L < E.Row; L++) {
                if (SSBuffer->InsLine(SL, 0) == 0) return 0;
                if (SSBuffer->InsLineText(SL, 0, -1, 0, RLine(L)) == 0) return 0;
                SL++;
            }
            if (SSBuffer->InsLine(SL, 0) == 0) return 0;
            if (SSBuffer->InsLineText(SL, 0, E.Col, 0, RLine(E.Row)) == 0) return 0;
        }
        if (Append && OldCount > 0)
            if (SSBuffer->JoinLine(OldCount - 1, 0) == 0)
                return 0;
        break;
    }
    if (SystemClipboard)
        PutPMClip(clipboard);
    return 1;
}

int EBuffer::BlockPasteStream(int clipboard) {
    BlockMode = bmStream;
    return BlockPaste(clipboard);
}

int EBuffer::BlockPasteLine(int clipboard) {
    BlockMode = bmLine;
    return BlockPaste(clipboard);
}

int EBuffer::BlockPasteColumn(int clipboard) {
    BlockMode = bmColumn;
    return BlockPaste(clipboard);
}

int EBuffer::BlockPaste(int clipboard) {
    EPoint B, E;
    int L, BL;

    if (SystemClipboard)
        GetPMClip(clipboard);

    if (SSBuffer == 0) return 0;
    if (SSBuffer->RCount == 0) return 0;
    AutoExtend = 0;
    BFI(SSBuffer, BFI_TabSize) = BFI(this, BFI_TabSize);
    BFI(SSBuffer, BFI_ExpandTabs) = BFI(this, BFI_ExpandTabs);
    BFI(SSBuffer, BFI_Undo) = 0;
    BlockUnmark();
    B.Row = VToR(CP.Row);
    B.Col = CP.Col;
    BL = B.Row;
    switch(BlockMode) {
    case bmLine:
        B.Col = 0;
        for (L = 0; L < SSBuffer->RCount; L++) {
            if (InsLine(BL, 0) == 0) return 0;
            if (InsLineText(BL, 0, SSBuffer->LineLen(L), 0, SSBuffer->RLine(L)) == 0) return 0;
            BL++;
        }
        E.Row = BL;
        E.Col = 0;
        SetBB(B);
        SetBE(E);
        break;

    case bmColumn:
        for (L = 0; L < SSBuffer->RCount; L++) {
            if (AssertLine(BL) == 0) return 0;
            if (InsLineText(BL, B.Col, SSBuffer->LineLen(L), 0, SSBuffer->RLine(L)) == 0) return 0;
            if (TrimLine(BL) == 0) return 0;
            BL++;
        }
        if (AssertLine(BL) == 0) return 0;
        E.Row = BL;
        E.Col = B.Col + SSBuffer->LineLen(0);
        SetBB(B);
        SetBE(E);
        break;

    case bmStream:
        if (SSBuffer->RCount > 1)
            if (SplitLine(B.Row, B.Col) == 0) return 0;
        if (InsLineText(B.Row, B.Col, SSBuffer->LineLen(0), 0, SSBuffer->RLine(0)) == 0) return 0;
        E = B;
        E.Col += SSBuffer->LineLen(0);
        BL++;
        if (SSBuffer->RCount > 1) {
            for (L = 1; L < SSBuffer->RCount - 1; L++) {
                if (InsLine(BL, 0) == 0) return 0;
                if (InsLineText(BL, 0, SSBuffer->LineLen(L), 0, SSBuffer->RLine(L)) == 0) return 0;
                BL++;
            }
            L = SSBuffer->RCount - 1;
            if (InsLineText(BL, 0, SSBuffer->LineLen(L), 0, SSBuffer->RLine(L)) == 0) return 0;
            E.Col = SSBuffer->LineLen(L);
            E.Row = BL;
        }
        SetBB(B);
        SetBE(E);
        break;
    }
    return 1;
}

int EBuffer::BlockKill() {
    EPoint B, E;
    int L;
    int Y = -1;

    AutoExtend = 0;
    if (CheckBlock() == 0) return 0;
    if (RCount <= 0) return 0;
    B = BB;
    E = BE;
    Draw(B.Row, -1);
    //    if (MoveToPos(B.Col, B.Row) == 0) return 0;

#ifdef CONFIG_UNDOREDO
    if (BFI(this, BFI_Undo) == 1) {
        if (PushULong(CP.Col) == 0) return 0;
        if (PushULong(CP.Row) == 0) return 0;
        if (PushUChar(ucPosition) == 0) return 0;
    }
#endif

    switch (BlockMode) {
    case bmLine:
        Y = VToR(CP.Row);
        if (Y >= B.Row) {
            if (Y >= E.Row) {
                if (SetPosR(CP.Col, Y - (E.Row - B.Row)) == 0) return 0;
            } else {
                if (SetPosR(CP.Col, B.Row) == 0) return 0;
            }
        }
        for (L = B.Row; L < E.Row; L++)
            if (DelLine(B.Row) == 0) return 0;
        break;

    case bmColumn:
        Y = VToR(CP.Row);
        if (Y >= B.Row && Y < E.Row) {
            if (CP.Col >= B.Col) {
                if (CP.Col >= E.Col) {
                    if (SetPos(CP.Col - (E.Col - B.Col), CP.Row) == 0) return 0;
                } else {
                    if (SetPos(B.Col, CP.Row) == 0) return 0;
                }
            }
        }
        for (L = B.Row; L < E.Row; L++)
            if (DelText(L, B.Col, E.Col - B.Col) == 0) return 0;
        break;

    case bmStream:
        Y = VToR(CP.Row);

        if (B.Row == E.Row) {
            if (Y == B.Row) {
                if (CP.Col >= B.Col) {
                    if (CP.Col >= E.Col) {
                        if (SetPos(CP.Col - (E.Col - B.Col), CP.Row) == 0) return 0;
                    } else {
                        if (SetPos(B.Col, CP.Row) == 0) return 0;
                    }
                }
            }
            if (DelText(B.Row, B.Col, E.Col - B.Col) == 0) return 0;
        } else {
            if (Y >= B.Row) {
                if (Y > E.Row || (Y == E.Row && E.Col == 0)) {
                    if (SetPosR(CP.Col, Y - (E.Row - B.Row)) == 0) return 0;
                } else if (Y == E.Row) {
                    if (CP.Col >= E.Col) {
                        if (SetPosR(CP.Col - E.Col + B.Col, B.Row) == 0) return 0;
                    } else {
                        if (SetPosR(B.Col, B.Row) == 0) return 0;
                    }
                } else {
                    if (SetPosR(B.Col, B.Row) == 0) return 0;
                }
            }
            if (DelText(E.Row, 0, E.Col) == 0) return 0;
            for (L = B.Row + 1; L < E.Row; L++)
                if (DelLine(B.Row + 1) == 0) return 0;
            if (DelText(B.Row, B.Col, -1) == 0) return 0;
            if (JoinLine(B.Row, B.Col) == 0) return 0;
        }
        break;
    }
    return BlockUnmark();
}

// remove selected text and paste information from clipboard to replace it
int EBuffer::BlockPasteOver(int clipboard) {
    // if there is existing selection, remove it's contents
    if (CheckBlock())
    {
        BlockKill();
    }

    // paste text from clipboard
    if (BlockPaste(clipboard))
    {
        // go to end of selection
        SetPos(BE.Col, BE.Row);

        // remove selection
        return BlockUnmark();
    }

    return 0;
}

// XXX clipboard ???
int EBuffer::ClipClear(int clipboard) {
    if (SSBuffer == 0)
        return 0;
    SSBuffer->Clear();
    if (SystemClipboard)
        PutPMClip(clipboard);
    return 1;
}

int EBuffer::BlockIndent() {
    EPoint B, E;
    int L;

    AutoExtend = 0;
    if (CheckBlock() == 0) return 0;
    if (RCount <= 0) return 0;
    B = BB;
    E = BE;
    Draw(B.Row, E.Row);
    if (SetPosR(B.Col, B.Row) == 0) return 0;
    for (L = B.Row; L <= E.Row; L++) {
        switch (BlockMode) {
        case bmStream:
        case bmLine:
            if (L < E.Row || E.Col != 0) {
                int I = LineIndented(L) + 1;
                IndentLine(L, I);
            }
            break;
        case bmColumn:
            if (L < E.Row) {
                if (InsText(L, B.Col, 1, 0) == 0) return 0;
                if (DelText(L, E.Col, 1) == 0) return 0;
            }
            break;
        }
    }
    if (SetPosR(B.Col, B.Row) == 0) return 0;
    return 1;
}

int EBuffer::BlockUnindent() {
    EPoint B, E;
    int L;

    AutoExtend = 0;
    if (CheckBlock() == 0) return 0;
    if (RCount <= 0) return 0;
    B = BB;
    E = BE;
    Draw(B.Row, E.Row);
    if (SetPosR(B.Col, B.Row) == 0) return 0;
    for (L = B.Row; L <= E.Row; L++) {
        switch (BlockMode) {
        case bmStream:
        case bmLine:
            if (L < E.Row || E.Col != 0) {
                int I = LineIndented(L) - 1;
                if (I >= 0)
                    IndentLine(L, I);
            }
            break;
        case bmColumn:
            if (L < E.Row) {
                if (InsText(L, E.Col, 1, 0) == 0) return 0;
                if (DelText(L, B.Col, 1) == 0) return 0;
            }
            break;
        }
    }
    if (SetPosR(B.Col, B.Row) == 0) return 0;
    return 1;
}

int EBuffer::BlockClear() {
    return 0;
}

int EBuffer::BlockMarkStream() {
    if (BlockMode != bmStream) BlockUnmark();
    BlockMode= bmStream;
    if (AutoExtend) AutoExtend = 0;
    else {
        BlockUnmark();
        AutoExtend = 1;
    }
    return 1;
}

int EBuffer::BlockMarkLine() {
    if (BlockMode != bmLine) BlockUnmark();
    BlockMode= bmLine;
    if (AutoExtend) AutoExtend = 0;
    else {
        BlockUnmark();
        AutoExtend = 1;
    }
    return 1;
}

int EBuffer::BlockMarkColumn() {
    if (BlockMode != bmColumn) BlockUnmark();
    BlockMode= bmColumn;
    if (AutoExtend) AutoExtend = 0;
    else {
        BlockUnmark();
        AutoExtend = 1;
    }
    return 1;
}

int EBuffer::BlockExtendBegin() {
    CheckBlock();
    ExtendGrab = 0;
    AutoExtend = 0;
    int Y = VToR(CP.Row);

    switch (BlockMode) {
    case bmStream:
        if ((Y == BB.Row) && (CP.Col == BB.Col)) ExtendGrab |= 1;
        if ((Y == BE.Row) && (CP.Col == BE.Col)) ExtendGrab |= 2;
        break;
    case bmLine:
        if (Y == BB.Row) ExtendGrab |= 1;
        if (Y == BE.Row) ExtendGrab |= 2;
        break;
    case bmColumn:
        if (Y == BB.Row) ExtendGrab |= 1;
        if (Y == BE.Row) ExtendGrab |= 2;
        if (CP.Col == BB.Col) ExtendGrab |= 4;
        if (CP.Col == BE.Col) ExtendGrab |= 8;
        break;
    }

    if (ExtendGrab == 0) {
        BlockBegin();
        BlockEnd();
        if (BlockMode == bmColumn)
            ExtendGrab = 1 | 2 | 4 | 8;
        else
            ExtendGrab = 1 | 2;
    }
    return 1;
}

int EBuffer::BlockExtendEnd() {
    EPoint T, B, E;

    CheckBlock();
    B = BB;
    E = BE;
    switch (BlockMode) {
    case bmLine:
        if (ExtendGrab & 1) { B.Row = VToR(CP.Row); B.Col = 0; }
        else if (ExtendGrab & 2) { E.Row = VToR(CP.Row); E.Col = 0; }
        if (B.Row > E.Row) {
            T = B;
            B = E;
            E = T;
        }
        break;
    case bmStream:
        if (ExtendGrab & 1) { B.Col = CP.Col; B.Row = VToR(CP.Row); }
        else if (ExtendGrab & 2) { E.Col = CP.Col; E.Row = VToR(CP.Row); }
        if ((B.Row > E.Row) ||
            ((B.Row == E.Row) && (B.Col > E.Col))) {
            T = B;
            B = E;
            E = T;
        }
        break;
    case bmColumn:
        if (ExtendGrab & 1) B.Row = VToR(CP.Row);
        else if (ExtendGrab & 2) E.Row = VToR(CP.Row);
        if (ExtendGrab & 4) B.Col = CP.Col;
        else if (ExtendGrab & 8) E.Col = CP.Col;
        if (B.Row > E.Row) {
            int T;

            T = B.Row;
            B.Row = E.Row;
            E.Row = T;
        }
        if (B.Col > E.Col) {
            int T;

            T = B.Col;
            B.Col = E.Col;
            E.Col = T;
        }
        break;
    }
    SetBB(B);
    SetBE(E);
    ExtendGrab = 0;
    AutoExtend = 0;
    return 1;
}

int EBuffer::BlockIsMarked() {
    if ((BB.Row != -1) && (BE.Row != -1) && (BB.Col != -1) && (BE.Col != -1)) return 1;
    return 0;
}

int EBuffer::BlockReIndent() {
    EPoint P = CP;
    EPoint B, E;

    AutoExtend = 0;
    if (CheckBlock() == 0) return 0;
    if (RCount <= 0) return 0;
    B = BB;
    E = BE;
    Draw(B.Row, E.Row);
    for (int i = B.Row; i < E.Row; i++) {
        if (SetPosR(0, i) == 0) return 0;
        if (LineIndent() == 0) return 0;
    }
    return SetPos(P.Col, P.Row);
}

int EBuffer::BlockSelectWord() {
    int Y = VToR(CP.Row);
    PELine L = RLine(Y);
    int P;
    int C;

    if (BlockUnmark() == 0) return 0;
    BlockMode = bmStream;

    P = CharOffset(L, CP.Col);

    if (P >= L->Count) return 0;
    C = ChClassK(L->Chars[P]);

    while ((P > 0) && (C == ChClassK(L->Chars[P - 1]))) P--;
    if (SetBB(EPoint(Y, ScreenPos(L, P))) == 0) return 0;
    while ((P < L->Count) && (C == ChClassK(L->Chars[P]))) P++;
    if (SetBE(EPoint(Y, ScreenPos(L, P))) == 0) return 0;
    return 1;
}

int EBuffer::BlockSelectLine() {
    int Y = VToR(CP.Row);
    if (BlockUnmark() == 0) return 0;
    BlockMode = bmStream;

    if (SetBB(EPoint(Y, 0)) == 0) return 0;
    if (Y == RCount - 1) {
        if (SetBE(EPoint(Y, LineLen(Y))) == 0) return 0;
    } else {
        if (SetBE(EPoint(Y + 1, 0)) == 0) return 0;
    }
    return 1;
}

int EBuffer::BlockSelectPara() {
    return 1;
}

int EBuffer::BlockWriteTo(const char *AFileName, int Append) {
    //int error = 0;
    EPoint B, E;
    int L;
    PELine LL;
    int A, Z;
    FILE *fp;
    int bc = 0, lc = 0, oldc = 0;

    AutoExtend = 0;
    if (CheckBlock() == 0) return 0;
    if (RCount == 0) return 0;
    B = BB;
    E = BE;
    Msg(S_INFO, "Writing %s...", AFileName);
    fp = fopen(AFileName, Append ? "ab" : "wb");
    if (fp == NULL) goto error;
    setvbuf(fp, FileBuffer, _IOFBF, sizeof(FileBuffer));
    for (L = B.Row; L <= E.Row; L++) {
        A = -1;
        Z = -1;
        LL = RLine(L);
        switch (BlockMode) {
        case bmLine:
            if (L < E.Row) {
                A = 0;
                Z = LL->Count;
            }
            break;
        case bmColumn:
            if (L < E.Row) {
                A = CharOffset(LL, B.Col);
                Z = CharOffset(LL, E.Col);
            }
            break;
        case bmStream:
            if (B.Row == E.Row) {
                A = CharOffset(LL, B.Col);
                Z = CharOffset(LL, E.Col);
            } else if (L == B.Row) {
                A = CharOffset(LL, B.Col);
                Z = LL->Count;
            } else if (L < E.Row) {
                A = 0;
                Z  = LL->Count;
            } else if (L == E.Row) {
                A = 0;
                Z = CharOffset(LL, E.Col);
            }
            break;
        }
        if (A != -1 && Z != -1) {
            if (A < LL->Count) {
                if (Z > LL->Count)
                    Z = LL->Count;
                if (Z > A) {
                    if ((int)fwrite(LL->Chars + A, 1, Z - A, fp) != Z - A) {
                        goto error;
                    } else
                        bc += Z - A;
                }
            }
            if (BFI(this, BFI_AddCR) == 1) {
                if (fputc(13, fp) < 0) goto error;
                else
                    bc++;
            }
            if (BFI(this, BFI_AddLF) == 1) {
                if (fputc(10, fp) < 0)
                    goto error;
                else {
                    bc++;
                    lc++;
                }
            }
            if (bc > 65536 + oldc) {
                Msg(S_INFO, "Writing %s, %d lines, %d bytes.", AFileName, lc, bc);
                oldc = bc;
            }
        }
    }
    fclose(fp);
    Msg(S_INFO, "Wrote %s, %d lines, %d bytes.", AFileName, lc, bc);
    return 1;
error:
    if(fp != NULL)
    {
        fclose(fp);
        unlink(AFileName);
    }
    View->MView->Win->Choice(GPC_ERROR, "Error", 1, "O&K", "Failed to write block to %s", AFileName);
    return 0;
}

int EBuffer::BlockReadFrom(const char *AFileName, int blockMode) {
    EBuffer *B;
    int savesys;
    int rc;

    if (FileExists(AFileName) == 0) {
        View->MView->Win->Choice(GPC_ERROR, "Error", 1, "O&K", "File not found: %s", AFileName);
        return 0;
    }

    B = new EBuffer(0, (EModel **)&SSBuffer, AFileName);
    if (B == 0) return 0;
    B->SetFileName(AFileName, 0);
    if (B->Load() == 0) {
        delete B;
        return 0;
    }

    savesys = SystemClipboard;
    SystemClipboard = 0;

    switch (blockMode) {
    case bmColumn: rc = BlockPasteColumn(); break;
    case bmLine:   rc = BlockPasteLine(); break;
    default:
    case bmStream: rc = BlockPasteStream(); break;
    }

    SystemClipboard = savesys;

    if (rc == 0)
        return 0;
    delete B;
    return 1;
}

static EBuffer *SortBuffer;
static int SortReverse;
static int *SortRows = 0;
static int SortMinRow;
static int SortMaxRow;
static int SortMinCol;
static int SortMaxCol;

static int _LNK_CONV SortProc(const void *A, const void *B) {
    int *AA = (int *)A;
    int *BB = (int *)B;
    ELine *LA = SortBuffer->RLine(*AA);
    ELine *LB = SortBuffer->RLine(*BB);
    int rc;

    if (SortMinCol == -1) {
        int lA = LA->Count;
        int lB = LB->Count;

        if (BFI(SortBuffer, BFI_MatchCase) == 1)
            rc = memcmp(LA->Chars, LB->Chars, (lA < lB) ? lA : lB);
        else
            rc = memicmp(LA->Chars, LB->Chars, (lA < lB) ? lA : lB);
        if (rc == 0) {
            if (lA > lB)
                rc = 1;
            else
                rc = -1;
        }
    } else {
        int lA = LA->Count;
        int lB = LB->Count;
        int PA = SortBuffer->CharOffset(LA, SortMinCol);
        int PB = SortBuffer->CharOffset(LB, SortMinCol);

        lA -= PA;
        lB -= PB;
        if (lA < 0 && lB < 0)
            rc = 0;
        else if (lA < 0 && lB > 0)
            rc = -1;
        else if (lA > 0 && lB < 0)
            rc = 1;
        else {
            if (SortMaxCol != -1) {
                if (lA > SortMaxCol - SortMinCol)
                    lA = SortMaxCol - SortMinCol;
                if (lB > SortMaxCol - SortMinCol)
                    lB = SortMaxCol - SortMinCol;
            }
            if (BFI(SortBuffer, BFI_MatchCase) == 1)
                rc = memcmp(LA->Chars+ PA, LB->Chars + PB, (lA < lB) ? lA : lB);
            else
                rc = memicmp(LA->Chars + PA, LB->Chars + PB, (lA < lB) ? lA : lB);
            if (rc == 0) {
                if (lA > lB)
                    rc = 1;
                else
                    rc = -1;
            }
        }
    }

    if (SortReverse)
        return -rc;
    return rc;
}

int EBuffer::BlockSort(int Reverse) {
    int rq;
    ELine *oldL;

    if (CheckBlock() == 0) return 0;
    if (RCount == 0) return 0;

    SortMinRow = BB.Row;
    SortMaxRow = BE.Row;
    if (BlockMode != bmStream || BE.Col == 0)
        SortMaxRow--;

    if (SortMinRow >= SortMaxRow)
        return 1;

    SortBuffer = this;
    SortReverse = Reverse;
    switch (BlockMode) {
    case bmLine:
    case bmStream:
        SortMinCol = -1;
        SortMaxCol = -1;
        break;

    case bmColumn:
        SortMinCol = BB.Col;
        SortMaxCol = BE.Col;
        break;
    }

    SortRows = (int *)malloc((SortMaxRow - SortMinRow + 1) * sizeof(int));
    if (SortRows == 0) {
        free(SortRows);
        return 0;
    }
    for (rq = 0; rq <= SortMaxRow - SortMinRow; rq++)
        SortRows[rq] = rq + SortMinRow;

    qsort(SortRows, SortMaxRow - SortMinRow + 1, sizeof(int), SortProc);

    // now change the order of lines according to new order in Rows array.

    for (rq = 0; rq <= SortMaxRow - SortMinRow; rq++) {
        oldL = RLine(SortRows[rq]);
        if (InsLine(1 + rq + SortMaxRow, 0) == 0)
            return 0;
        if (InsChars(1 + rq + SortMaxRow, 0, oldL->Count, oldL->Chars) == 0)
            return 0;
    }

    for (rq = 0; rq <= SortMaxRow - SortMinRow; rq++)
        if (DelLine(SortMinRow) == 0)
            return 0;

    free(SortRows);
    return 1;
}

int EBuffer::BlockUnTab() {
    EPoint B, E;
    ELine *L;
    int O, C;

    AutoExtend = 0;
    if (CheckBlock() == 0) return 0;
    if (RCount <= 0) return 0;
    B = BB;
    E = BE;
    Draw(B.Row, E.Row);
    for (int i = B.Row; i < E.Row; i++) {
        L = RLine(i);
        O = 0;
        C = 0;
        while (O < L->Count) {
            if (L->Chars[O] == '\t') {
                C = NextTab(C, BFI(this, BFI_TabSize));

                if (DelChars(i, O, 1) != 1)
                    return 0;
                if (InsChars(i, O, C - O, 0) != 1)
                    return 0;
                O = C;
            } else {
                O++;
                C++;
            }
        }
    }
    return 1;
}

int EBuffer::BlockEnTab() {
    EPoint B, E;
    ELine *L;
    int O, C, O1, C1;
    char tab = '\t';

    AutoExtend = 0;
    if (CheckBlock() == 0) return 0;
    if (RCount <= 0) return 0;
    B = BB;
    E = BE;
    Draw(B.Row, E.Row);
    for (int i = B.Row; i < E.Row; i++) {
        L = RLine(i);
        O = C = 0;
        O1 = C1 = 0;
        while (O < L->Count) {
            if (L->Chars[O] == '\t') { // see if there are spaces to remove
                int C2 = NextTab(C, BFI(this, BFI_TabSize));
                int N = BFI(this, BFI_TabSize) - (C2 - C);
                if (O - O1 < N)
                    N = O - O1;
                if (N > 0) {
                    if (DelChars(i, O - N, N) != 1)
                        return 0;
                    O -= N;
                    C = C2;
                    O++;
                    C1 = C;
                    O1 = O;
                } else {
                    O++;
                    C = C2;
                    O1 = O;
                    C1 = C;
                }
            } else if (L->Chars[O] != ' ') { // nope, cannot put tab here
                O++;
                C++;
                C1 = C;
                O1 = O;
            } else if (((C % BFI(this, BFI_TabSize)) == (BFI(this, BFI_TabSize) - 1)) &&
                       (C - C1 > 0))
            { // reached a tab and can put one
                int N = BFI(this, BFI_TabSize);
                if (O - O1 + 1 < N) {
                    N = O - O1 + 1;
                } else if (O - O1 + 1 > N) {
                    O1 = O - N + 1;
                }
                if (DelChars(i, O1, N) != 1)
                    return 0;
                if (InsChars(i, O1, 1, &tab) != 1)
                    return 0;
                O1++;
                O = O1;
                C++;
                C1 = C;
            } else {
                O++;
                C++;
            }
        }
    }
    return 1;
}

// FindFunction -- search for line matching 'RoutineRegexp'
// starting from current line + 'delta'. 'way' should be +1 or -1.
int EBuffer::FindFunction(int delta, int way) {
    RxNode     *regx;
    int         line;
    PELine      L;
    RxMatchRes  res;

    if (BFS(this, BFS_RoutineRegexp) == 0) {
        View->MView->Win->Choice(GPC_ERROR, "Error", 1,
                                 "O&K", "No routine regexp.");
        return -1;
    }
    regx = RxCompile(BFS(this, BFS_RoutineRegexp));
    if (regx == 0) {
        View->MView->Win->Choice(GPC_ERROR, "Error", 1,
                                 "O&K", "Failed to compile regexp '%s'",
                                 BFS(this, BFS_RoutineRegexp));
        return -1;
    }

    //** Scan backwards from the current cursor position,
    Msg(S_BUSY, "Matching %s", BFS(this, BFS_RoutineRegexp));
    line = VToR(CP.Row) + delta;
    while (line >= 0 && line < RCount) {
        L = RLine(line);
        if (RxExec(regx, L->Chars, L->Count, L->Chars, &res) == 1)
            break;
        line += way;
    }
    if (line < 0)
        line = 0;
    if (line >= RCount)
        line = RCount - 1;
    RxFree(regx);
    return line;
}

// Selects the current function.
int EBuffer::BlockMarkFunction() {
    int by, ey;

    if (BlockUnmark() == 0)
        return 0;

    if ((by = FindFunction( 0, -1)) == -1)
        return 0;
    if ((ey = FindFunction(+1, +1)) == -1)
        return 0;

    //** Start and end are known. Set the block;
    BlockMode = bmStream;
    if (SetBB(EPoint(by, 0)) == 0)
        return 0;
    if (SetBE(EPoint(ey, 0)) == 0)
        return 0;

    return 1;
}

int EBuffer::IndentFunction() {
    EPoint P = CP;
    int by, ey;

    if ((by = FindFunction( 0, -1)) == -1)
        return 0;
    if ((ey = FindFunction(+1, +1)) == -1)
        return 0;

    //Draw(by, ey); ?
    for (int i = by; i < ey; i++) {
        if (SetPosR(0, i) == 0)
            return 0;
        if (LineIndent() == 0)
            return 0;
    }
    return SetPos(P.Col, P.Row);
}

int EBuffer::MoveFunctionPrev() {
    int line = FindFunction(-1, -1);

    if (line == -1)
        return 0;

    return CenterPosR(0, line);
}

int EBuffer::MoveFunctionNext() {
    int line = FindFunction(+1, +1);

    if (line == -1)
        return 0;

    return CenterPosR(0, line);
}
