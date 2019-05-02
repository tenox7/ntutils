/*    e_buffer.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "e_buffer.h"

#include "c_bind.h"
#include "c_config.h"
#include "c_history.h"
#include "e_mark.h"
#include "e_undo.h"
#include "i_modelview.h"
#include "i_view.h"
#include "o_routine.h"
#include "s_util.h"

EBuffer *SSBuffer = 0; // scrap buffer (clipboard)

///////////////////////////////////////////////////////////////////////////////

EBuffer::EBuffer(int createFlags, EModel **ARoot, const char * /*AName*/) :
    EModel(createFlags, ARoot),
    FileName(0),
    Modified(0),
    TP(0,0),
    CP(0,0),
    BB(-1,-1),
    BE(-1,-1),
    PrevPos(-1, -1),
    SavedPos(-1, -1),
    BlockMode(bmStream),
    ExtendGrab(0),
    AutoExtend(0),
    Loaded(0),
    Loading(0),
    RAllocated(0),
    RGap(0),
    RCount(0),
    LL(0),
    VAllocated(0),
    VGap(0),
    VCount(0),
    VV(0),
#ifdef CONFIG_FOLDS
    FCount(0),
    FF(0),
#endif
    Match(-1, -1),
    MatchLen(0),
    MatchCount(0),
    MinRedraw(-1),
    MaxRedraw(-1),
    RedrawToEos(0)
{
#ifdef CONFIG_UNDOREDO
    US.Num = 0;
    US.Data = 0;
    US.Top = 0;
    US.UndoPtr = 0;
    US.NextCmd = 1;
    US.Record = 1;
    US.Undo = 0;
#endif
#ifdef CONFIG_OBJ_ROUTINE
    rlst.Count = 0;
    rlst.Lines = 0;
    Routines = 0;
#endif
    //Name = strdup(AName);
    Allocate(0);
    AllocVis(0);
    Mode = GetModeForName("");
    Flags = (Mode->Flags);
    BFI(this, BFI_Undo) = 0;
    BFI(this, BFI_ReadOnly) = 0;
#ifdef CONFIG_SYNTAX_HILIT
    StartHilit = 0;
    EndHilit = -1;
    HilitProc = 0;
    if (Mode && Mode->fColorize)
        HilitProc = GetHilitProc(Mode->fColorize->SyntaxParser);
#endif
    InsertLine(CP,0,0); /* there should always be at least one line in the edit buffer */
    Flags = (Mode->Flags);
    Modified = 0;
}

EBuffer::~EBuffer() {
#ifdef CONFIG_HISTORY
    if (FileName != 0 && Loaded) {
        UpdateFPos(FileName, VToR(CP.Row), CP.Col);
#ifdef CONFIG_BOOKMARKS
        if (BFI (this,BFI_SaveBookmarks)==3) StoreBookmarks(this);
#endif
    }
#endif
    if (FileName && Loaded)
        markIndex.storeForBuffer(this);
    
    Clear();
    if (LL)
	free(LL);
    //free(Name);
    if (FileName)
	free(FileName);
#ifdef CONFIG_BOOKMARKS
    vector_iterate(EBookmark*, BMarks, it)
	delete *it;
#endif
#ifdef CONFIG_OBJ_ROUTINE
    if (rlst.Lines) {
        free(rlst.Lines);
        rlst.Lines = 0;
    }
    DeleteRelated();
#endif
}

void EBuffer::DeleteRelated() {
#ifdef CONFIG_OBJ_ROUTINE
    if (Routines) {
        ::ActiveView->DeleteModel(Routines);
        Routines = 0;
    }
#endif
}

int EBuffer::Clear() {
    Modified = 1;
#ifdef CONFIG_SYNTAX_HILIT
    EndHilit = -1;
    StartHilit = 0;
    WordList.clear();
#endif
#ifdef CONFIG_OBJ_ROUTINE
    rlst.Count = 0;
    if (rlst.Lines) {
        free(rlst.Lines);
        rlst.Lines = 0;
    }
#endif
    if (LL)
    {
	for (int i = 0; i < RCount; i++)
	    delete LL[GapLine(i, RGap, RCount, RAllocated)];
	free(LL);
	LL = 0;
    }
    RCount = RAllocated = RGap = 0;
    VCount = VAllocated = VGap = 0;
    if (VV)
    {
        free(VV);
	VV = 0;
    }
#ifdef CONFIG_UNDOREDO
    FreeUndo();
#endif

#ifdef CONFIG_FOLDS
    if (FCount != 0) {
        free(FF);
        FCount = 0;
        FF = 0;
    }
#endif

    return 0;
}

#ifdef CONFIG_UNDOREDO
int EBuffer::FreeUndo() {
    for (int j = 0; j < US.Num; j++)
        free(US.Data[j]);
    free(US.Top);
    free(US.Data);
    US.Num = 0;
    US.Data = 0;
    US.Top = 0;
    US.Undo = 0;
    US.Record = 1;
    US.UndoPtr = 0;
    return 1;
}
#endif

int EBuffer::Modify() {
    // if RecheckReadOnly is activated do readonly checking when necessary
    if (RecheckReadOnly != 0)
    {
        if (BFI(this, BFI_ReadOnly)) {
            // File might have been toggled writable outside the editor, or
            //  you might do what I do, and do a Tools/Run/"p4 edit Filename.cpp"
            //  from inside FTE, and it's a pain to manually reopen the file, so
            //  recheck writability here instead. Note that we don't check the
            //  converse, since in reality this is rarely a problem, and the
            //  file save routines will check this (oh well).  --ryan.
            struct stat StatBuf;
            if ((FileName != 0) && FileOk && (stat(FileName, &StatBuf) == 0)) {
                if (!(StatBuf.st_mode & (S_IWRITE | S_IWGRP | S_IWOTH)))
                    BFI(this, BFI_ReadOnly) = 1;
                else
                    BFI(this, BFI_ReadOnly) = 0;
            }
        }
    }

    if (BFI(this, BFI_ReadOnly)) {
        Msg(S_ERROR, "File is read-only.");
        return 0;
    }
    if (Modified == 0) {
        struct stat StatBuf;

        if ((FileName != 0) && FileOk && (stat(FileName, &StatBuf) == 0)) {
            if (FileStatus.st_size != StatBuf.st_size ||
                FileStatus.st_mtime != StatBuf.st_mtime)
            {
                View->MView->Win->Choice(GPC_ERROR, "Warning! Press Esc!",
                                         0,
                                         "File %-.55s changed on disk!", 
                                         FileName);
                switch (View->MView->Win->Choice(0, "File Changed on Disk",
                               2,
                               "&Modify",
                               "&Cancel",
                               "%s", FileName))
                {
                case 0:
                    break;
                case 1:
                case -1:
                default:
                    return 0;
                }
            }
        }
#ifdef CONFIG_UNDOREDO
        if (BFI(this, BFI_Undo))
            if (PushUChar(ucModified) == 0) return 0;
#endif
    }
    Modified++;
    if (Modified == 0) Modified++;
    return 1;
}

int EBuffer::LoadRegion(EPoint * /*A*/, int /*FH*/, int /*StripChar*/, int /*LineChar*/) {
    return 0;
}

/*
int EBuffer::AddLine(const char *AChars) {
    if (InsLine(Pos.Row, 0) == 0) return 0;
    if (InsText(Pos.Row, Pos.Col, ACount, AChars) == 0) return 0;
    return 1;
}
*/

int EBuffer::InsertLine(const EPoint& Pos, size_t ACount, const char *AChars) {
    if (!InsLine(Pos.Row, 0))
        return 0;
    return InsText(Pos.Row, Pos.Col, ACount, AChars);
}

int EBuffer::UpdateMark(EPoint &M, int Type, int Row, int Col, int Rows, int Cols) {
    switch (Type) {
    case umInsert: /* text inserted */
        switch (BlockMode) {
        case bmLine:
        case bmColumn:
            if (M.Row >= Row)
                M.Row += Rows;
            break;
        case bmStream:
            if (Cols) {
                if (M.Row == Row)
                    if (M.Col >= Col)
                        M.Col += Cols;
            }
            if (Rows) {
                if (M.Row >= Row)
                    M.Row += Rows;
            }
            break;
        }
        break;
    case umDelete:
        switch (BlockMode) {
        case bmLine:
        case bmColumn:
            if (M.Row >= Row) {
                if (InRange(Row, M.Row, Row + Rows))
                    M.Row = Row;
                else
                    M.Row -= Rows;
            }
            break;
        case bmStream:
            if (Cols) {
                if (M.Row == Row)
                    if (M.Col >= Col) {
                        if (M.Col < Col + Cols)
                            M.Col = Col;
                        else
                            M.Col -= Cols;
                    }
            }
            if (Rows) {
                if (M.Row >= Row) {
                    if (M.Row < Row + Rows) {
                        M.Row = Row;
                        M.Col = 0;
                    } else M.Row -= Rows;
                }
            }
        }
        break;
    case umSplitLine:
        switch (BlockMode) {
        case bmLine:
        case bmColumn:
            if (M.Row == Row) {
                if (Col <= M.Col) {
                    M.Row++;
                    M.Col -= Col;
                }
            } else if (M.Row > Row) M.Row++;
            break;
        case bmStream:
            if (M.Row == Row) {
                if (Col <= M.Col) {
                    M.Row++;
                    M.Col -= Col;
                }
            } else if (M.Row > Row) M.Row++;
            break;
        }
        break;
    case umJoinLine:
        switch (BlockMode) {
        case bmLine:
        case bmColumn:
            if (M.Row == Row + 1)
                M.Row--;
            else if (M.Row > Row) M.Row--;
            break;
        case bmStream:
            if (M.Row == Row + 1) {
                M.Row--;
                M.Col += Col;
            } else if (M.Row > Row) M.Row--;
            break;
        }
        break;
    }
    return 1;
}
        
int EBuffer::UpdateMarker(int Type, int Row, int Col, int Rows, int Cols) {
    EPoint OldBB = BB, OldBE = BE;
    EView *V;
    
    UpdateMark(SavedPos, Type, Row, Col, Rows, Cols);
    UpdateMark(PrevPos, Type, Row, Col, Rows, Cols);

    UpdateMark(BB, Type, Row, Col, Rows, Cols);
    UpdateMark(BE, Type, Row, Col, Rows, Cols);
    
    V = View;
    while (V) {
        if (V->Model != this)
            assert(1 == 0);
        if (V != View) {
            EPoint M;
            
            M = GetViewVPort(V)->TP;
            UpdateMark(GetViewVPort(V)->TP, Type, Row, Col, Rows, Cols);
            GetViewVPort(V)->TP.Col = M.Col;
            UpdateMark(GetViewVPort(V)->CP, Type, Row, Col, Rows, Cols);
        }
        V = V->NextView;
    }
    
#ifdef CONFIG_OBJ_ROUTINE
    for (int i = 0; i < rlst.Count && rlst.Lines; i++) {
        EPoint M;

        M.Col = 0;
        M.Row = rlst.Lines[i];
        UpdateMark(M, Type, Row, Col, Rows, Cols);
        rlst.Lines[i] = M.Row;
    }
#endif
#ifdef CONFIG_FOLDS
    for (int f = 0; f < FCount; f++) {
        EPoint M;
        
        M.Col = 0;
        M.Row = FF[f].line;
        UpdateMark(M, Type, Row, Col, Rows, Cols);
        FF[f].line = M.Row;
    }
#endif
#ifdef CONFIG_BOOKMARKS
    vector_iterate(EBookmark*, BMarks, it)
        UpdateMark((*it)->GetPoint(), Type, Row, Col, Rows, Cols);
#endif
    
    if (OldBB.Row != BB.Row) {
        int MinL = Min(OldBB.Row, BB.Row);
        int MaxL = Max(OldBB.Row, BB.Row);
        if (MinL != -1 && MaxL != -1)  
            Draw(MinL, MaxL);
    }
    if (OldBE.Row != BE.Row) {
        int MinL = Min(OldBE.Row, BE.Row);
        int MaxL = Max(OldBE.Row, BE.Row);
        if (MinL != -1 && MaxL != -1)  
            Draw(MinL, MaxL);
    }
    return 1;
}

int EBuffer::ValidPos(EPoint Pos) {
    if ((Pos.Col >= 0) &&
        (Pos.Row >= 0) &&
        (Pos.Row < VCount))
        return 1;
    return 0;
}

int EBuffer::RValidPos(EPoint Pos) {
    if ((Pos.Col >= 0) &&
        (Pos.Row >= 0) &&
        (Pos.Row < RCount))
        return 1;
    return 0;
}

int EBuffer::AssertLine(int Row) {
    if (Row == RCount)
        if (!InsLine(RCount, 0))
            return 0;

    return 1;
}

int EBuffer::SetFileName(const char *AFileName, const char *AMode) {
    FileOk = 0;

    free(FileName);
    FileName = strdup(AFileName);
    Mode = 0;
    if (AMode) {
        if (!(Mode = FindMode(AMode))) {
            StlString AMODE(AMode);
            AMODE.toupper();
            Mode = FindMode(AMODE.c_str());
        }
    }
    if (Mode == 0)
        Mode = GetModeForName(AFileName);
    assert(Mode != 0);
    Flags = (Mode->Flags);
#ifdef CONFIG_SYNTAX_HILIT
    HilitProc = 0;
    if (Mode && Mode->fColorize)
        HilitProc = GetHilitProc(Mode->fColorize->SyntaxParser);
#endif
    UpdateTitle();
    return FileName?1:0;
}

int EBuffer::SetPos(int Col, int Row, int tabMode) {
    if (!VCount)
        return 1;
    assert (Col >= 0 && Row >= 0 && Row < VCount);

#ifdef CONFIG_UNDOREDO
    if (BFI(this, BFI_Undo) == 1 && BFI(this, BFI_UndoMoves) == 1) {
        if (PushULong(CP.Col) == 0) return 0;
        if (PushULong(CP.Row) == 0) return 0;
        if (PushUChar(ucPosition) == 0) return 0;
    }
#endif
    if (AutoExtend) {
        BlockExtendBegin();
        AutoExtend = 1;
    }
    PrevPos = CP;
    PrevPos.Row = (CP.Row < VCount) ? VToR(CP.Row) : (CP.Row - VCount + RCount);
    CP.Row = Row;
    CP.Col = Col;
    if (AutoExtend) {
        BlockExtendEnd();
        AutoExtend = 1;
    }
    //        if (View && View->Model == this ) {
    //            View->GetVPort();
    //        }
    if (BFI(this, BFI_CursorThroughTabs) == 0) {
        if (tabMode == tmLeft) {
            if (MoveTabStart() == 0) return 0;
        } else if (tabMode == tmRight) {
            if (MoveTabEnd() == 0) return 0;
        }
    }
    if (ExtendGrab == 0 && AutoExtend == 0 && BFI(this, BFI_PersistentBlocks) == 0) {
        if (CheckBlock() == 1)
            if (BlockUnmark() == 0)
                return 0;
    }
    return 1;
}

int EBuffer::SetPosR(int Col, int Row, int tabMode) {
    assert (Row >= 0 && Row < RCount && Col >= 0);
    
    int L = RToV(Row);
    
    if (L == -1)
        if (ExposeRow(Row) == 0) return 0;
    
    L = RToV(Row);
    
    return SetPos(Col, L, tabMode);
}

int EBuffer::SetNearPos(int Col, int Row, int tabMode) {
    if (Row >= VCount) Row = VCount - 1;
    if (Row < 0) Row = 0;
    if (Col < 0) Col = 0;
    return SetPos(Col, Row, tabMode);
}

int EBuffer::SetNearPosR(int Col, int Row, int tabMode) {
    if (Row >= RCount) Row = RCount - 1;
    if (Row < 0) Row = 0;
    if (Col < 0) Col = 0;
    return SetPosR(Col, Row, tabMode);
}

int EBuffer::CenterPos(int Col, int Row, int tabMode) {
    assert(Row >= 0 && Row < VCount && Col >= 0);
    
    if (SetPos(Col, Row, tabMode) == 0) return 0;
    if (View && View->Model == this) {
        Row -= GetVPort()->Rows / 2;
        if (Row < 0) Row = 0;
        Col -= GetVPort()->Cols - 8;
        if (Col < 0) Col = 0;
        if (GetVPort()->SetTop(Col, Row) == 0) return 0;
        GetVPort()->ReCenter = 1;
    }
    return 1;
}

int EBuffer::CenterPosR(int Col, int Row, int tabMode) {
    int L;
    
    assert(Row >= 0 && Row < RCount && Col >= 0);
    
    L = RToV(Row);
    if (L == -1)
        if (ExposeRow(Row) == 0) return 0;
    L = RToV(Row);
    return CenterPos(Col, L, tabMode);
}

int EBuffer::CenterNearPos(int Col, int Row, int tabMode) {
    if (Row >= VCount) Row = VCount - 1;
    if (Row < 0) Row = 0;
    if (Col < 0) Col = 0;
    return CenterPos(Col, Row, tabMode);
}

int EBuffer::CenterNearPosR(int Col, int Row, int tabMode) {
    if (Row >= RCount) Row = RCount - 1;
    if (Row < 0) Row = 0;
    if (Col < 0) Col = 0;
    return CenterPosR(Col, Row, tabMode);
}

int EBuffer::LineLen(int Row) {
    assert(Row >= 0 && Row < RCount);
    PELine L = RLine(Row);
    return ScreenPos(L, L->Count);
}

int EBuffer::LineChars(int Row) {
    assert(Row >= 0 && Row < RCount);
    return RLine(Row)->Count;
}

int EBuffer::DelLine(int Row, int DoMark) {
    int VLine;
    int GapSize;
//    printf("DelLine: %d\n", Row);
    if (Row < 0) return 0;
    if (Row >= RCount) return 0;
    if (Modify() == 0) return 0;
    
    VLine = RToV(Row);
    if (VLine == -1)
        if (ExposeRow(Row) == 0) return 0;
    VLine = RToV(Row);
    assert(VLine != -1);

#ifdef CONFIG_FOLDS
    if (FindFold(Row) != -1) {
        if (FoldDestroy(Row) == 0) return 0;
    }
#endif

    VLine = RToV(Row);
    assert(VLine != -1);
    
#ifdef CONFIG_UNDOREDO
    if (BFI(this, BFI_Undo) == 1) {
        if (PushUData(RLine(Row)->Chars, RLine(Row)->Count) == 0) return 0;
        if (PushULong(RLine(Row)->Count) == 0) return 0;
        if (PushULong(Row) == 0) return 0;
        if (PushUChar(ucDelLine) == 0) return 0;
    }
#endif
    if (DoMark)
        UpdateMarker(umDelete, Row, 0, 1, 0);
    //puts("Here");

    Draw(Row, -1);
    Hilit(Row);
    assert(RAllocated >= RCount);
    if (RGap != Row) 
        if (MoveRGap(Row) == 0) return 0;
    
    GapSize = RAllocated - RCount;

    delete LL[RGap + GapSize];
    LL[RGap + GapSize] = 0;
    RCount--;
    GapSize++;
    if (RAllocated - RAllocated / 2 > RCount) {
        memmove(LL + RGap + GapSize - RAllocated / 3,
                LL + RGap + GapSize,
                sizeof(PELine) * (RCount - RGap));
        if (Allocate(RAllocated - RAllocated / 3) == 0) return 0;
    }
    
    assert(VAllocated >= VCount);
    if (VGap != VLine)
        if (MoveVGap(VLine) == 0) return 0;
    GapSize = VAllocated - VCount;
    VV[VGap + GapSize] = 0;
    VCount--;
    GapSize++;
    if (VAllocated - VAllocated / 2 > VCount) {
        memmove(VV + VGap + GapSize - VAllocated / 3,
                VV + VGap + GapSize,
                sizeof(VV[0]) * (VCount - VGap));
        if (AllocVis(VAllocated - VAllocated / 3) == 0) return 0;
    }
    return 1;
}

int EBuffer::InsLine(int Row, int DoAppend, int DoMark) {
    PELine L;
    int VLine = -1;
    
    //    printf("InsLine: %d\n", Row);
    
    //if (! LL)
    //   return 0;
    if (Row < 0) return 0;
    if (Row > RCount) return 0;
    if (Modify() == 0) return 0;
    if (DoAppend) {
        Row++;
    }
    if (Row < RCount) {
        VLine = RToV(Row);
        if (VLine == -1)
            if (ExposeRow(Row) == 0) return 0;
        VLine = RToV(Row);
        assert(VLine != -1);
    } else {
        VLine = VCount;
    }
    L = new ELine(0, (char *)0);
    if (L == 0) return 0;
#ifdef CONFIG_UNDOREDO
    if (BFI(this, BFI_Undo) == 1) {
        if (PushULong(Row) == 0) return 0;
        if (PushUChar(ucInsLine) == 0) return 0;
    }
#endif
    if (DoMark)
        UpdateMarker(umInsert, Row, 0, 1, 0);
    Draw(Row, -1);
    Hilit(Row);
    VLine = RToVN(Row);
    assert(RCount <= RAllocated);
//    printf("++ %d G:C:A :: %d - %d - %d\n", Row, RGap, RCount, RAllocated);
    if (RCount == RAllocated) {
        if (Allocate(RCount ? (RCount * 2) : 1) == 0) return 0;
        memmove(LL + RAllocated - (RCount - RGap),
                LL + RGap,
                sizeof(PELine) * (RCount - RGap));
    }
    if (RGap != Row)
        if (MoveRGap(Row) == 0) return 0;
    LL[RGap] = L;
    RGap++;
    RCount++;
    //    printf("-- %d G:C:A :: %d - %d - %d\n", Row, RGap, RCount, RAllocated);
    
    assert(VCount <= VAllocated);
    if (VCount == VAllocated) {
        if (AllocVis(VCount ? (VCount * 2) : 1) == 0) return 0;
        memmove(VV + VAllocated - (VCount - VGap),
                VV + VGap,
                sizeof(VV[0]) * (VCount - VGap));
    }
    if (VGap != VLine)
        if (MoveVGap(VLine) == 0) return 0;
    VV[VGap] = Row - VGap;
    VGap++;
    VCount++;
        
/*    if (AllocVis(VCount + 1) == 0) return 0;
    memmove(VV + VLine + 1, VV + VLine, sizeof(VV[0]) * (VCount - VLine));
    VCount++;
    Vis(VLine, Row - VLine);*/
    return 1;
}

int EBuffer::DelChars(int Row, int Ofs, size_t ACount) {
    PELine L;
    
//    printf("DelChars: %d:%d %d\n", Row, Ofs, ACount);
    if (Row < 0) return 0;
    if (Row >= RCount) return 0;
    L = RLine(Row);
    
    if (Ofs < 0) return 0;
    if (Ofs >= L->Count) return 0;
    if (Ofs + ACount >= L->Count)
        ACount = L->Count - Ofs;
    if (ACount == 0) return 1;
    
    if (Modify() == 0) return 0;
    
#ifdef CONFIG_UNDOREDO
    if (BFI(this, BFI_Undo) == 1) {
        if (PushUData(L->Chars + Ofs, ACount) == 0) return 0;
        if (PushULong(ACount) == 0) return 0;
        if (PushULong(Ofs) == 0) return 0;
        if (PushULong(Row) == 0) return 0;
        if (PushUChar(ucDelChars) == 0) return 0;
    }
#endif
    
    if (L->Count > Ofs + ACount)
        memmove(L->Chars + Ofs, L->Chars + Ofs + ACount, L->Count - Ofs - ACount);
    L->Count -= ACount;
    if (L->Allocate(L->Count) == 0) return 0;
    Draw(Row, Row);
    Hilit(Row);
//    printf("OK\n");
    return 1;
}

int EBuffer::InsChars(int Row, int Ofs, size_t ACount, const char *Buffer) {
    PELine L;
    
//    printf("InsChars: %d:%d %d\n", Row, Ofs, ACount);
    
    assert(Row >= 0 && Row < RCount && Ofs >= 0);
    L = RLine(Row);
    
    if (Ofs < 0) return 0;
    if (Ofs > L->Count) return 0;
    if (ACount == 0) return 1;
    
    if (Modify() == 0) return 0;

#ifdef CONFIG_UNDOREDO
    if (BFI(this, BFI_Undo) == 1) {
        if (PushULong(Row) == 0) return 0;
        if (PushULong(Ofs) == 0) return 0;
        if (PushULong(ACount) == 0) return 0;
        if (PushUChar(ucInsChars) == 0) return 0;
    }
#endif
    if (L->Allocate(L->Count + ACount) == 0) return 0;
    if (L->Count > Ofs)
        memmove(L->Chars + Ofs + ACount, L->Chars + Ofs, L->Count - Ofs);
    if (Buffer == 0) 
        memset(L->Chars + Ofs, ' ', ACount);
    else
        memmove(L->Chars + Ofs, Buffer, ACount);
    L->Count += ACount;
    Draw(Row, Row);
    Hilit(Row);
    // printf("OK\n");
    return 1;
}

int EBuffer::UnTabPoint(int Row, int Col) {
    ELine *L;
    int Ofs, Pos, TPos;
    
    assert(Row >= 0 && Row < RCount && Col >= 0);
    L = RLine(Row);
    Ofs = CharOffset(L, Col);
    if (Ofs >= L->Count)
        return 1;
    if (L->Chars[Ofs] != '\t')
        return 1;
    Pos = ScreenPos(L, Ofs);
    if (Pos < Col) {
        TPos = NextTab(Pos, BFI(this, BFI_TabSize));
        if (DelChars(Row, Ofs, 1) != 1)
            return 0;
        if (InsChars(Row, Ofs, TPos - Pos, 0) != 1)
            return 0;
    }
    return 1;
}

int EBuffer::ChgChars(int Row, int Ofs, size_t ACount, const char * /*Buffer*/) {
    PELine L;
    
    assert(Row >= 0 && Row < RCount && Ofs >= 0);
    L = RLine(Row);
    
    if (Ofs < 0) return 0;
    if (Ofs > L->Count) return 0;
    if (ACount == 0) return 1;
    
    if (Modify() == 0) return 0;
    
#ifdef CONFIG_UNDOREDO
    if (BFI(this, BFI_Undo) == 1) {
        if (PushUData(L->Chars + Ofs, ACount) == 0) return 0;
        if (PushULong(ACount) == 0) return 0;
        if (PushULong(Ofs) == 0) return 0;
        if (PushULong(Row) == 0) return 0;
        if (PushUChar(ucDelChars) == 0) return 0;
        if (PushULong(Row) == 0) return 0;
        if (PushULong(Ofs) == 0) return 0;
        if (PushULong(ACount) == 0) return 0;
        if (PushUChar(ucInsChars) == 0) return 0;
    }
#endif
    Hilit(Row);
    Draw(Row, Row);
    return 1;
}

int EBuffer::DelText(int Row, int Col, size_t ACount, int DoMark) {
    int L, B, C;

//    printf("DelTExt: %d:%d %d\n", Row, Col, ACount);

    assert(Row >= 0 && Row < RCount && Col >= 0);
    if (Modify() == 0) return 0;
    
    if (ACount == 0) return 1;
    L = LineLen(Row);
    if (Col >= L)
        return 1;
    if (ACount == -1 || ACount + Col > L)
        ACount = L - Col;
    if (UnTabPoint(Row, Col) == 0)
        return 0;
    if (UnTabPoint(Row, Col + ACount) == 0)
        return 0;
    B = CharOffset(RLine(Row), Col);
    C = CharOffset(RLine(Row), Col + ACount);
    if ((ACount > 0) && (B != -1) && (C != -1)) {
        if (DelChars(Row, B, C - B) == 0) return 0;
        if (DoMark) UpdateMarker(umDelete, Row, Col, 0, ACount);
    }
//    printf("OK\n");
    return 1;
}

int EBuffer::InsText(int Row, int Col, size_t ACount, const char *ABuffer, int DoMark) {
    int B, L;
    
//    printf("InsText: %d:%d %d\n", Row, Col, ACount);
    assert(Row >= 0 && Row < RCount && Col >= 0);
    if (ACount == 0) return 1;
    if (Modify() == 0) return 0;
    
    if (DoMark) UpdateMarker(umInsert, Row, Col, 0, ACount);
    L = LineLen(Row);
    if (L < Col) {
        if (InsChars(Row, RLine(Row)->Count, Col - L, 0) == 0)
            return 0;
    } else
        if (UnTabPoint(Row, Col) == 0) return 0;
    B = CharOffset(RLine(Row), Col);
    if (InsChars(Row, B, ACount, ABuffer) == 0) return 0;
//    printf("OK\n");
    return 1;
}

int EBuffer::PadLine(int Row, size_t Length) {
    size_t L;
    
    L = LineLen(Row);
    if (L < Length)
        if (InsChars(Row, RLine(Row)->Count, Length - L, 0) == 0)
            return 0;
    return 1;
}
  
int EBuffer::InsLineText(int Row, int Col, size_t ACount, int LCol, PELine Line) {
    int Ofs, Pos, TPos, C, B, L;
    
    //fprintf(stderr, "\n\nInsLineText: %d:%d %d %d", Row, Col, ACount, LCol);
    assert(Row >= 0 && Row < RCount && Col >= 0 && LCol >= 0);
    if (BFI(this, BFI_ReadOnly) == 1)
        return 0;
    
    L = ScreenPos(Line, Line->Count);
    if (LCol >= L) return 1;
    if (ACount == -1) ACount = L - LCol;
    if (ACount + LCol > L) ACount = L - LCol;
    if (ACount == 0) return 1;
    assert(ACount > 0);

    B = Ofs = CharOffset(Line, LCol);
    if (Ofs < Line->Count && Line->Chars[Ofs] == '\t') {
        Pos = ScreenPos(Line, Ofs);
        if (Pos < LCol) {
            TPos = NextTab(Pos, BFI(this, BFI_TabSize));
            if (InsText(Row, Col, TPos - LCol, 0) == 0)
                return 0;
            Col += TPos - LCol;
            ACount -= TPos - LCol;
            LCol = TPos;
            B++;
        }
    }
    C = Ofs = CharOffset(Line, LCol + ACount);
    if (Ofs < Line->Count && Line->Chars[Ofs] == '\t') {
        Pos = ScreenPos(Line, Ofs);
        if (Pos < LCol + ACount) {
            if (InsText(Row, Col, LCol + ACount - Pos, 0) == 0)
                return 0;
        }
    }
    //fprintf(stderr, "B = %d, C = %d\n", B, C);
    C -= B;
    if (C <= 0) return 1;
    if (InsText(Row, Col, C, Line->Chars + B) == 0) return 0;
//    printf("OK\n");
    return 1;
}

int EBuffer::SplitLine(int Row, int Col) {
    int VL;

    assert(Row >= 0 && Row < RCount && Col >= 0);
    
    if (BFI(this, BFI_ReadOnly) == 1) return 0;
    
    VL = RToV(Row);
    if (VL == -1) 
        if (ExposeRow(Row) == 0) return 0;
    if (Row > 0) {
        VL = RToV(Row - 1);
        if (VL == -1)
            if (ExposeRow(Row - 1) == 0) return 0;
    }
    VL = RToV(Row);
    assert(VL != -1);
    if (Col == 0) {
        if (InsLine(Row, 0, 1) == 0) return 0;
    } else {
        UpdateMarker(umSplitLine, Row, Col, 0, 0);
        if (InsLine(Row, 1, 0) == 0) return 0;
#ifdef CONFIG_SYNTAX_HILIT
	RLine(Row)->StateE = short((Row > 0) ? RLine(Row - 1)->StateE : 0);
#endif
        if (Col < LineLen(Row)) {
            int P, L;
            //if (RLine(Row)->ExpandTabs(Col, -2, &Flags) == 0) return 0;
            if (UnTabPoint(Row, Col) != 1)
                return 0;

            P = CharOffset(RLine(Row), Col);
            L = LineLen(Row);

            if (InsText(Row + 1, 0, RLine(Row)->Count - P, RLine(Row)->Chars + P, 0) == 0) return 0;
            if (DelText(Row, Col, L - Col, 0) == 0) return 0;
        }
    }
    Draw(Row, -1);
    Hilit(Row);
    return 1;
}

int EBuffer::JoinLine(int Row, int Col) {
    int Len, VLine;

    if (BFI(this, BFI_ReadOnly) == 1) return 0;
    if (Row < 0 || Row >= RCount - 1) return 0;
    if (Col < 0) return 0;
    Len = LineLen(Row);
    if (Col < Len) Col = Len;
    VLine = RToV(Row);
    if (VLine == -1) {
        if (ExposeRow(Row) == 0) return 0;
        if (ExposeRow(Row + 1) == 0) return 0;
    }
    VLine = RToV(Row);
    if (Col == 0 && RLine(Row)->Count == 0) {
        if (DelLine(Row, 1) == 0) return 0;
    } else {
        if (InsText(Row, Col, RLine(Row + 1)->Count, RLine(Row + 1)->Chars, 0) == 0) return 0;
        if (DelLine(Row + 1, 0) == 0) return 0;
        UpdateMarker(umJoinLine, Row, Col, 0, 0);
    }
    Draw(Row, -1);
    Hilit(Row);
    return 1;
}
