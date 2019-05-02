/*    e_redraw.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "c_bind.h"
#include "c_color.h"
#include "c_config.h"
#include "e_tags.h"
#include "i_modelview.h"
#include "i_view.h"
#include "o_buflist.h"

int EBuffer::GetMap(int Row, int *StateLen, hsState **StateMap) {
    hlState State = 0;

    Rehilit(Row);

    *StateLen = LineChars(Row);
    if (Row > 0) State = RLine(Row - 1)->StateE;
    if (*StateLen > 0) {
        PELine L = RLine(Row);
        int ECol;

        *StateMap = (hsState *) malloc(*StateLen);
        if (*StateMap == 0) return 0;

#ifdef CONFIG_SYNTAX_HILIT
        if (BFI(this, BFI_HilitOn) == 1 && HilitProc != 0)
            HilitProc(this, Row, 0, 0, *StateLen, L, State, *StateMap, &ECol);
        else
#endif
            Hilit_Plain(this, Row, 0, 0, *StateLen, L, State, *StateMap, &ECol);
        //        if (L->StateE != State) {
        //            L->StateE = State;
        //        }
    } else {
        *StateLen = 1;
        *StateMap = (hsState *) malloc(1);
        if (*StateMap == 0) return 0;
        (*StateMap)[0] = (hsState) (State & 0xFF);
    }
    return 1;
}

void EBuffer::FullRedraw() { // redraw all views
    EView *V = View;
    EEditPort *W;
    int Min, Max;

    while (V) {
        W = GetViewVPort(V);
        // Need to use real lines, not virtual
        // (similar to HilitMatchBracket)
        Min = VToR(W->TP.Row);
        Max = W->TP.Row + W->Rows;
        if (Max >= VCount)
            Max = RCount;
        else
            Max = VToR(Max);
        Draw(Min, Max);
        V = V->Next;
        if (V == View)
            break;
    }
}

void EBuffer::Hilit(int FromRow) {
    if (FromRow != -1) {
        if (StartHilit == -1)
            StartHilit = FromRow;
        else if (FromRow < StartHilit)
            StartHilit = FromRow;
    }
}

void EBuffer::Rehilit(int ToRow) {
    hlState State;
    int HilitX;
    PELine L;
    int ECol;

    if (StartHilit == -1)   // all ok
        return ;

    if (BFI(this, BFI_MultiLineHilit) == 0) // everything handled in redisplay
        return;

    if (ToRow <= StartHilit) // will be handled in redisplay
        return;

    if (ToRow >= RCount)
        ToRow = RCount;

    HilitX = 1;
    while ((StartHilit < RCount) && ((StartHilit < ToRow) || HilitX)) {
        L = RLine(StartHilit);

        HilitX = 0;
        if (StartHilit > 0)
            State = RLine(StartHilit - 1)->StateE;
        else
            State = 0;

        if (BFI(this, BFI_HilitOn) == 1 && HilitProc != 0) {
            HilitProc(this, StartHilit, 0, 0, 0, L, State, 0, &ECol);
        } else {
            Hilit_Plain(this, StartHilit, 0, 0, 0, L, State, 0, &ECol);
        }
        if (L->StateE != State) {
            HilitX = 1;
            L->StateE = State;
        }
        Draw(StartHilit, StartHilit);  // ?
        if (StartHilit > EndHilit)
            EndHilit = StartHilit;
        if (HilitX == 0) // jump over (can we always do this ?)
            if (StartHilit < EndHilit) {
                StartHilit = EndHilit;
            }
        StartHilit++;
    }
}

void EBuffer::Draw(int Row0, int RowE) {
    //printf("r0 = %d, re = %d\n", Row0, RowE);
    //printf("m = %d, max = %d, rts = %d\n", MinRedraw, MaxRedraw, RedrawToEos);
    if (Row0 == -1) Row0 = 0;
    if ((Row0 < MinRedraw) || (MinRedraw == -1)) {
        MinRedraw = Row0;
        if (MaxRedraw == -1) MaxRedraw = MinRedraw;
    }
    if (RowE == -1) {
        RedrawToEos = 1;
        MaxRedraw = MinRedraw;
    } else if (((RowE > MaxRedraw) || (MaxRedraw == -1)) && (RowE != -1))
	MaxRedraw = RowE;
    //printf("m = %d, max = %d, rts = %d\n", MinRedraw, MaxRedraw, RedrawToEos);
}

void EBuffer::DrawLine(TDrawBuffer B, int VRow, int C, int W, int &HilitX) {
    hlState State;
    int StartPos, EndPos;

    HilitX = 0;
    assert(W >= 0);

    MoveChar(B, 0, W, ' ', hcPlain_Background, W);
    //    if ((VRow == VCount - 1) && !BFI(this, BFI_ForceNewLine)) {
    // if (BFI(this, BFI_ShowMarkers))
    //     MoveChar(B, 0, W, EOF_MARKER, hcPlain_Markers, W);
    //    }
    if (VRow < VCount) {
        int Row = VToR(VRow);
        PELine L = RLine(Row);
        int ECol = 0;

        if (Row > 0) State = RLine(Row - 1)->StateE;
        else State = 0;
#ifdef CONFIG_SYNTAX_HILIT
        if (BFI(this, BFI_HilitOn) == 1 && HilitProc != 0)
            HilitProc(this, Row, B, C, W, L, State, 0, &ECol);
        else
#endif
            Hilit_Plain(this, Row, B, C, W, L, State, 0, &ECol);
        if (L->StateE != State) {
            HilitX = 1;
            L->StateE = State;
        }
        if (BFI(this, BFI_ShowMarkers)) {
            MoveChar(B, ECol - C, W, ConGetDrawChar((Row == RCount - 1) ? DCH_EOF : DCH_EOL), hcPlain_Markers, 1);
            ECol += 1;
        }
        if (Row < RCount) {
            int f;
            int Folded = 0;
            char fold[20];
            int l;

            f = FindFold(Row);
            if (f != -1) {
                ChColor foldColor;
                if (FF[f].level<5) foldColor=hcPlain_Folds[FF[f].level];
                else foldColor=hcPlain_Folds[4];
                if (FF[f].open == 1) {
                    l = sprintf(fold, "[%d]", FF[f].level);
                    MoveStr(B, ECol - C + 1, W, fold, foldColor, 10);
                    ECol += l;
                } else {
                    if (VRow < VCount - 1) {
                        Folded = Vis(VRow + 1) - Vis(VRow) + 1;
                    } else if (VRow < VCount) {
                        Folded = RCount - (VRow + Vis(VRow));
                    }
                    l = sprintf(fold, "(%d:%d)", FF[f].level, Folded);
                    MoveStr(B, ECol - C + 1, W, fold, foldColor, 10);
                    ECol += l;
                    MoveAttr(B, 0, W, foldColor, W);
                }
            }
        }
        if (BB.Row != -1 && BE.Row != -1 && Row >= BB.Row && Row <= BE.Row) {
            switch(BlockMode) {
            case bmLine:
                StartPos = 0;
                if (Row == BE.Row) EndPos = 0;
                else EndPos = W;
                break;
            case bmColumn:
                StartPos = BB.Col - C;
                if (Row == BE.Row) EndPos = BB.Col - C;
                else EndPos = BE.Col - C;
                break;
            case bmStream:
                if (Row == BB.Row && Row == BE.Row) {
                    StartPos = BB.Col - C;
                    EndPos = BE.Col - C;
                } else if (Row == BB.Row) {
                    StartPos = BB.Col - C;
                    EndPos = W;
                } else if (Row == BE.Row) {
                    StartPos = 0;
                    EndPos = BE.Col - C;
                } else {
                    StartPos = 0;
                    EndPos = W;
                }
                break;
            default:
                StartPos = EndPos = 0;
                break;
	    }
	    if (EndPos > StartPos) {
		if (BFI(this, BFI_SeeThruSel))
		    MoveBgAttr(B, StartPos, W, hcPlain_Selected, EndPos - StartPos);
		else
		    MoveAttr(B, StartPos, W, hcPlain_Selected, EndPos - StartPos);
	    }
	}
#ifdef CONFIG_BOOKMARKS
        if (BFI(this, BFI_ShowBookmarks)) {
            int i = 0;
            const EBookmark* eb;
            while ((i = GetBookmarkForLine(i, Row, eb)) != -1) {
                if (strncmp(eb->GetName(), "_BMK", 4) == 0) {
                    // User bookmark, hilite line
                    if (BFI(this, BFI_SeeThruSel))
                        MoveBgAttr(B, 0, W, hcPlain_Bookmark, W);
                    else
                        MoveAttr(B, 0, W, hcPlain_Bookmark, W);
                    break;
                }
            }
        }
#endif
        if (Match.Row != -1 && Match.Col != -1) {
            if (Row == Match.Row) {
                if (BFI(this, BFI_SeeThruSel))
                    MoveBgAttr(B, Match.Col - C, W, hcPlain_Found, MatchLen);
                else
                    MoveAttr(B, Match.Col - C, W, hcPlain_Found, MatchLen);
            }
        }
    } else if (VRow == VCount) {
        if (BFI(this, BFI_ShowMarkers))
            MoveChar(B, 0, W, ConGetDrawChar(DCH_END), hcPlain_Markers, W);
    }
}

void EBuffer::Redraw() {
    int HilitX;
    EView *V;
    EEditPort *W;
    int Row;
    TDrawBuffer B;
    char s[256];
    ChColor SColor;
    int RowA, RowZ;
    int W1, H1;
    int first;

    //printf("EBuffer::Redraw1\n");
    if (CP.Row >= VCount) CP.Row = VCount - 1;
    if (CP.Row < 0) CP.Row = 0;

    //printf("EBuffer::Redraw t:%p  %p   %p   %p  %p\n", this, View, View ? View->MView : (void*)1, Next, Prev);
    //assert(View != 0);

    CheckBlock();
    /* check some window data */
    if (!View || !View->MView) {
        MinRedraw = MaxRedraw = -1;
        RedrawToEos = 0;
        return;
    }

    View->MView->ConQuerySize(&W1, &H1);
    if (H1 < 1 || W1 < 1)
	return;
    //printf("EBuffer::Redraw4\n");

    first = 1;
    for (V = View; V && (first || V != View); V = V->NextView) {
	if (!V->MView || !V->MView->Win)
	    continue;

        //        printf("Checking\x7\n");
        if (V->Model != this)
            assert(1 == 0);

        W = GetViewVPort(V);

        if (W->Rows < 1 || W->Cols < 1)
            continue;

        if (V == View) {
            int scrollJumpX = Min(ScrollJumpX, W->Cols / 2);
            int scrollJumpY = Min(ScrollJumpY, W->Rows / 2);
            int scrollBorderX = Min(ScrollBorderX, W->Cols / 2);
            int scrollBorderY = Min(ScrollBorderY, W->Rows / 2);

            W->CP = CP;
            TP = W->TP;

            if (W->ReCenter) {
                W->TP.Row = CP.Row - W->Rows / 2;
                W->TP.Col = CP.Col - W->Cols + 8;
                W->ReCenter = 0;
            }

            if (W->TP.Row + scrollBorderY > CP.Row) W->TP.Row = CP.Row - scrollJumpY + 1 - scrollBorderY;
            if (W->TP.Row + W->Rows - scrollBorderY <= CP.Row) W->TP.Row = CP.Row - W->Rows + 1 + scrollJumpY - 1 + scrollBorderY;
            if (!WeirdScroll)
                if (W->TP.Row + W->Rows >= VCount) W->TP.Row = VCount - W->Rows;
            if (W->TP.Row < 0) W->TP.Row = 0;

            if (W->TP.Col + scrollBorderX > CP.Col) W->TP.Col = CP.Col - scrollJumpX - scrollBorderX;
            if (W->TP.Col + W->Cols - scrollBorderX <= CP.Col) W->TP.Col = CP.Col - W->Cols + scrollJumpX + scrollBorderX;
            if (W->TP.Col < 0) W->TP.Col = 0;

            if (W->OldTP.Row != -1 && W->OldTP.Col != -1 && RedrawToEos == 0) {

                if ((W->OldTP.Row != W->TP.Row) || (W->OldTP.Col != W->TP.Col)) {
                    int A, B;
                    int DeltaX, DeltaY;
                    int Rows = W->Rows;
                    int Delta1 = 0, Delta2 = 0;

                    DeltaY = W->TP.Row - W->OldTP.Row ;
                    DeltaX = W->TP.Col - W->OldTP.Col;

                    if ((DeltaX == 0) && (-Rows < DeltaY) && (DeltaY < Rows)) {
                        if (DeltaY < 0) {
                            W->ScrollY(DeltaY);
                            A = W->TP.Row;
			    B = W->TP.Row - DeltaY;
                        } else {
                            W->ScrollY(DeltaY);
                            A = W->TP.Row + Rows - DeltaY;
                            B = W->TP.Row + Rows;
                        }
                    } else {
                        A = W->TP.Row;
                        B = W->TP.Row + W->Rows;
                    }
                    if (A >= VCount) {
                        Delta1 = A - VCount + 1;
                        A = VCount - 1;
                    }
                    if (B >= VCount) {
                        Delta2 = B - VCount + 1;
                        B = VCount - 1;
                    }
                    if (A < 0) A = 0;
                    if (B < 0) B = 0;
		    Draw(VToR(A) + Delta1, VToR(B) + Delta2);
                }
            } else {
                int A = W->TP.Row;
                int B = A + W->Rows;
                int Delta = 0;

		if (!VCount)
		    continue;

                if (B > VCount) {
                    Delta += B - VCount;
                    B = VCount;
                }
		int LastV = VToR(VCount - 1);
                int B1 = (B == VCount) ? RCount : VToR(B);

                if (B1 >= LastV) {
                    Delta += B1 - LastV;
                    B1 = LastV;
                }
                if (B1 < 0) B1 = 0;
                Draw(VToR(A), B1 + Delta);
            }

            W->OldTP = W->TP;
            TP = W->TP;
        }
        if (W->CP.Row >= VCount) W->CP.Row = VCount - 1;
        if (W->CP.Row < 0) W->CP.Row = 0;
        if (W->TP.Row > W->CP.Row) W->TP.Row = W->CP.Row;
        if (W->TP.Row < 0) W->TP.Row = 0;

        if (V->MView->IsActive()) // hack
            SColor = hcStatus_Active;
        else
            SColor = hcStatus_Normal;
        MoveChar(B, 0, W->Cols, ' ', SColor, W->Cols);

	if (V->MView->Win->GetViewContext() == V->MView) {
            V->MView->Win->SetSbVPos(W->TP.Row, W->Rows, VCount + (WeirdScroll ? W->Rows - 1 : 0));
            V->MView->Win->SetSbHPos(W->TP.Col, W->Cols, 1024 + (WeirdScroll ? W->Cols - 1 : 0));
        }

        if (V->CurMsg == 0) {
                int CurLine = W->CP.Row;
                int ActLine = VToR(W->CP.Row);
                int CurColumn = W->CP.Col;
                int CurPos = CharOffset(RLine(ActLine), CurColumn);
                int NumLines = RCount;
                int NumChars = RLine(ActLine)->Count;
                //            int NumColumns = ScreenPos(Line(CurLine), NumChars);
                char *fName = FileName;
                unsigned char CurCh = 0xFF;
                int lf = strlen(fName);
                char CCharStr[20] = "";

                if (lf > 34) fName += lf - 34;

                if (CurPos < NumChars) {
                    CurCh = VLine(CurLine)->Chars[CurPos];
                    sprintf(CCharStr, "%3u,%02X", CurCh, CurCh);
                } else {
                    if (CurPos > NumChars) strcpy(CCharStr, "      ");
                    else if (CurLine < NumLines - 1) strcpy(CCharStr, "   EOL");
                    else strcpy(CCharStr, "   EOF");
                }

                sprintf(s, "%04d:%02d %c%c%c%c%c %.6s %c"
#ifdef DOS
                        " %lu "
#endif
                        ,
                        //                    CurLine + 1,
                        ActLine + 1,
                        CurColumn + 1,
                        //                    CurPos + 1,
                        (BFI(this, BFI_Insert)) ? 'I' : ' ',
                        (BFI(this, BFI_AutoIndent)) ? 'A' : ' ',
                        //                    (BFI(this, BFI_ExpandTabs))?'T':' ',
                        (BFI(this, BFI_MatchCase)) ? 'C' : ' ',
                        AutoExtend ?
                        (
                         (BlockMode == bmStream) ? 's' :
                         (BlockMode == bmLine) ? 'l' : 'c'
                        ) :
                        ((BlockMode == bmStream) ? 'S' :
                         (BlockMode == bmLine) ? 'L': 'C'
                        ),
#ifdef CONFIG_WORDWRAP
                        (BFI(this, BFI_WordWrap) == 3) ? 't' :
                        (BFI(this, BFI_WordWrap) == 2) ? 'W' :
                        (BFI(this, BFI_WordWrap) == 1) ? 'w' :
                        ' ',
#endif
                        //                    (BFI(this, BFI_Undo))?'U':' ',
                        //                    (BFI(this, BFI_Trim))?'E':' ',
                        //                    (Flags.KeepBackups)?'B':' ',
                        Mode->fName,
                        (Modified != 0)?'*':(BFI(this, BFI_ReadOnly))?'%':' '
#ifdef DOS
                        ,coreleft()
#endif
                       );

                int l = strlen(s);
                int fw = W->Cols - l;
                int fl = strlen(FileName);
                char num[100]; /* seriously, let's not skimp. */

                MoveStr(B, 0, W->Cols, s, SColor, W->Cols);
                sprintf(num, " %s %d", CCharStr, ModelNo);
                MoveStr(B, W->Cols - strlen(num), W->Cols, num, SColor, W->Cols);

                fw -= strlen(num);

                if (fl > fw) {
                    MoveStr(B, l, W->Cols, FileName + fl - fw, SColor, W->Cols);
                } else {
                    MoveStr(B, l, W->Cols, FileName, SColor, W->Cols);
                }
        } else {
            MoveStr(B, 0, W->Cols, V->CurMsg, SColor, W->Cols);
        }
        if (V->MView->Win->GetStatusContext() == V->MView) {
            V->MView->ConPutBox(0, W->Rows, W->Cols, 1, B);
            if (V->MView->IsActive()) {
                V->MView->ConShowCursor();
                V->MView->ConSetCursorPos(W->CP.Col - W->TP.Col, W->CP.Row - W->TP.Row);
                if (BFI(this, BFI_Insert)) {
                    V->MView->ConSetCursorSize(CursorInsSize[0], CursorInsSize[1]);
                } else {
                    V->MView->ConSetCursorSize(CursorOverSize[0], CursorOverSize[1]);
                }
            }
        }
    }

    Rehilit(VToR(CP.Row));

    if (BFI(this, BFI_AutoHilitParen) == 1) {
        if (Match.Row == -1 && Match.Col == -1)
            HilitMatchBracket();
    }

    //    if ((Window == WW) && (MinRedraw == -1))
    //        MaxRedraw = MinRedraw = VToR(CP.Row);

    //printf("\n\nMinRedraw = %d, MaxRedraw = %d", MinRedraw, MaxRedraw);
    if (MinRedraw == -1 || !VCount)
        return;

    //    printf("Will redraw: %d to %d, to eos = %d\n", MinRedraw, MaxRedraw, RedrawToEos);
    if (MinRedraw >= VCount) MinRedraw = VCount - 1;
    if (MinRedraw < 0) MinRedraw = 0;
    //    puts("xxx\x7");
    //    printf("%d\n", MinRedraw);
    Row = RowA = RToVN(MinRedraw);
    //    puts("xxx\x7");
    RowZ = MaxRedraw;
    if (MaxRedraw != -1) {
        int Delta = 0;

        if (MaxRedraw >= RCount) {
            Delta = MaxRedraw - RCount + 1;
            MaxRedraw = RCount - 1;
        }
        if (MaxRedraw < 0) MaxRedraw = 0;
        //        printf("%d\n", MaxRedraw);
        RowZ = RToVN(MaxRedraw) + Delta;
    }
    //    puts("xxx\x7");
    //printf("\nRowA = %d, RowZ = %d", RowA, RowZ);

    first = 1;
    for (V = View; V && (first || V != View); V = V->NextView) {
        first = 0;
	if (!V->MView || !V->MView->Win)
            continue;

        if (V->Model != this)
            assert(1 == 0);

        W = GetViewVPort(V);

        for (int R = W->TP.Row; R < W->TP.Row + W->Rows; R++) {
            Row = R;
            if ((Row >= RowA) &&
                (RedrawToEos || Row <= RowZ))
            {
                DrawLine(B, Row, W->TP.Col, W->Cols, HilitX);
                W->DrawLine(Row, B);
                if (HilitX && Row == RowZ)
                    RowZ++;
            }
        }
    }
    MinRedraw = MaxRedraw = -1;
    RedrawToEos = 0;
}

int EBuffer::GetHilitWord(ChColor &clr, const char *str, size_t len, int IgnCase) {
    char *p;

    if (Mode == 0 || Mode->fColorize == 0)
        return 0;

    if (len >= CK_MAXLEN)
        return 0;

#ifdef CONFIG_WORD_HILIT
    {
        char s[CK_MAXLEN + 1];
        memcpy(s, str, len);
        s[len] = 0;
        if (HilitFindWord(s)) {
	    clr = ChColor(COUNT_CLR + hcPlain_HilitWord);
            return 1;
        }
    }
#endif
    if (len < 1) return 0;
    p = Mode->fColorize->Keywords.key[len];
    if (IgnCase) {
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
                //printf("PLEN %d  %d\n", p[len], COUNT_CLR);
		return 1;
            }
            p += len + 1;
        }
    }
    if (len < 128) {
        char s[128];

        memcpy(s, str, len);
        s[len] = 0;
        if (BFI(this, BFI_HilitTags)&&TagDefined(s)) {
	    clr = CLR_HexNumber;// Mode->fColorize->Colors[];
            return 1;
        }
    }
    
    return 0;
}

EEditPort *EBuffer::GetViewVPort(EView *V) {
    return (EEditPort *)V->Port;
}

EEditPort *EBuffer::GetVPort() {
    return (EEditPort *)View->Port;
}
