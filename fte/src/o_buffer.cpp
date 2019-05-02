/*    o_buffer.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "c_color.h"
#include "c_config.h"
#include "c_history.h"
#include "e_mark.h"
#include "e_tags.h"
#include "e_undo.h"
#include "ftever.h"
#include "i_modelview.h"
#include "i_view.h"
#include "o_cvsdiff.h"
#include "o_messages.h"
#include "o_svndiff.h"
#include "s_files.h"
#include "s_string.h"
#include "s_util.h"

#include <ctype.h>
#include <time.h>

SearchReplaceOptions LSearch;
int suspendLoads = 0;

EViewPort *EBuffer::CreateViewPort(EView *V) {
    V->Port = new EEditPort(this, V);
    AddView(V);

    if (Loaded == 0 && !suspendLoads) {
        Load();

#ifdef CONFIG_OBJ_MESSAGES
        if (CompilerMsgs)
            CompilerMsgs->FindFileErrors(this);
#endif
#ifdef CONFIG_OBJ_CVS
        if (CvsDiffView) CvsDiffView->FindFileLines(this);
#endif

#ifdef CONFIG_OBJ_SVN
        if (SvnDiffView) SvnDiffView->FindFileLines(this);
#endif

        markIndex.retrieveForBuffer(this);

#ifdef CONFIG_HISTORY
        int r, c;

        if (RetrieveFPos(FileName, r, c) == 1)
            SetNearPosR(c, r);
        //printf("setting to c:%d r:%d f:%s", c, r, FileName);
        V->Port->GetPos();
        V->Port->ReCenter = 1;

#ifdef CONFIG_BOOKMARKS
        if (BFI (this,BFI_SaveBookmarks)==3) RetrieveBookmarks(this);
#endif
#endif
    }
    return V->Port;
}

EEditPort::EEditPort(EBuffer *B, EView *V) :
    EViewPort(V),
    Buffer(B),
    OldTP(-1, -1),
    Rows(0),
    Cols(0)
{
    GetPos();

    if (V && V->MView && V->MView->Win) {
        V->MView->ConQuerySize(&Cols, &Rows);
        Rows--;
    }
}

EEditPort::~EEditPort() {
    StorePos();
}

void EEditPort::Resize(int Width, int Height) {
    Cols = Width;
    Rows = Height - 1;
    RedrawAll();
}

int EEditPort::SetTop(int Col, int Line) {
    int A, B;

    if (Line >= Buffer->VCount) Line = Buffer->VCount - 1;
    if (Line < 0) Line = 0;

    A = Line;
    B = Line + Rows;

    TP.Row = Line;
    TP.Col = Col;

    if (A >= Buffer->VCount) A = Buffer->VCount - 1;
    if (B >= Buffer->VCount) {
        B = Buffer->VCount - 1;
    }
    Buffer->Draw(Buffer->VToR(A), -1);
    return 1;
}

void EEditPort::StorePos() {
    Buffer->CP = CP;
    Buffer->TP = TP;
}

void EEditPort::GetPos() {
    CP = Buffer->CP;
    TP = Buffer->TP;
}

void EEditPort::ScrollY(int Delta) {
    // optimization
    // no need to scroll (clear) entire window which we are about to redraw
    if (Delta >= Rows || -Delta >= Rows)
        return ;

    if (Delta < 0) {
        Delta = -Delta;
        if (Delta > Rows) return;
        View->MView->ConScroll(csDown, 0, 0, Cols, Rows, hcPlain_Background, Delta);
    } else {
        if (Delta > Rows) return;
        View->MView->ConScroll(csUp, 0, 0, Cols, Rows, hcPlain_Background, Delta);
    }
}

void EEditPort::DrawLine(int L, TDrawBuffer B) {
    if (L < TP.Row) return;
    if (L >= TP.Row + Rows) return;
    if (View->MView->Win->GetViewContext() == View->MView)
        View->MView->ConPutBox(0, L - TP.Row, Cols, 1, B);
    //    printf("%d %d (%d %d %d %d)\n", 0, L - TP.Row, view->sX, view->sY, view->sW, view->sH);
}

void EEditPort::RedrawAll() {
    Buffer->Draw(TP.Row, -1);
    ///    Redraw(0, 0, Cols, Rows);
}

int EBuffer::GetContext() {
    return CONTEXT_FILE;
}

void EEditPort::HandleEvent(TEvent &Event) {
    EViewPort::HandleEvent(Event);
    switch (Event.What) {
    case evKeyDown:
        {
            char Ch;
            if (GetCharFromEvent(Event, &Ch)) {
                if (Buffer->BeginMacro() == 0)
                    return ;
                Buffer->TypeChar(Ch);
                Event.What = evNone;
            }
        }
        break;
    case evCommand:
        switch (Event.Msg.Command) {
        case cmVScrollUp:
            Buffer->ScrollDown(Event.Msg.Param1);
            Event.What = evNone;
            break;
        case cmVScrollDown:
            Buffer->ScrollUp(Event.Msg.Param1);
            Event.What = evNone;
            break;
        case cmVScrollPgUp:
            Buffer->ScrollDown(Rows);
            Event.What = evNone;
            break;
        case cmVScrollPgDn:
            Buffer->ScrollUp(Rows);
            Event.What = evNone;
            break;
        case cmVScrollMove:
            {
                int ypos;

//                fprintf(stderr, "Pos = %d\n\x7", Event.Msg.Param1);
                ypos = Buffer->CP.Row - TP.Row;
                Buffer->SetNearPos(Buffer->CP.Col, Event.Msg.Param1 + ypos);
                SetTop(TP.Col, Event.Msg.Param1);
                RedrawAll();
            }
            Event.What = evNone;
            break;
        case cmHScrollLeft:
            Buffer->ScrollRight(Event.Msg.Param1);
            Event.What = evNone;
            break;
        case cmHScrollRight:
            Buffer->ScrollLeft(Event.Msg.Param1);
            Event.What = evNone;
            break;
        case cmHScrollPgLt:
            Buffer->ScrollRight(Cols);
            Event.What = evNone;
            break;
        case cmHScrollPgRt:
            Buffer->ScrollLeft(Cols);
            Event.What = evNone;
            break;
        case cmHScrollMove:
            {
                int xpos;

                xpos = Buffer->CP.Col - TP.Col;
                Buffer->SetNearPos(Event.Msg.Param1 + xpos, Buffer->CP.Row);
                SetTop(Event.Msg.Param1, TP.Row);
                RedrawAll();
            }
            Event.What = evNone;
            break;
        }
        break;
#ifdef CONFIG_MOUSE
    case evMouseDown:
    case evMouseMove:
    case evMouseAuto:
    case evMouseUp:
        HandleMouse(Event);
        break;
#endif
    }
}
#ifdef CONFIG_MOUSE
void EEditPort::HandleMouse(TEvent &Event) {
    int x, y, xx, yy, W, H;

    View->MView->ConQuerySize(&W, &H);

    x = Event.Mouse.X;
    y = Event.Mouse.Y;

    if (Event.What != evMouseDown || y < H - 1) {
        xx = x + TP.Col;
        yy = y + TP.Row;
        if (yy >= Buffer->VCount) yy = Buffer->VCount - 1;
        if (yy < 0) yy = 0;
        if (xx < 0) xx = 0;

        switch (Event.What) {
        case evMouseDown:
            if (Event.Mouse.Y == H - 1)
                break;
            if (View->MView->Win->CaptureMouse(1))
                View->MView->MouseCaptured = 1;
            else
                break;

            View->MView->MouseMoved = 0;

            if (Event.Mouse.Buttons == 1) {
                // left mouse button down
                Buffer->SetNearPos(xx, yy);
                switch (Event.Mouse.Count % 5) {
                case 1:
                    break;
                case 2:
                    Buffer->BlockSelectWord();
                    break;
                case 3:
                    Buffer->BlockSelectLine();
                    break;
                case 4:
                    Buffer->BlockSelectPara();
                    break;
                }
                //            Window->Buffer->Redraw();
                if (SystemClipboard) {
                    // note: copy to second clipboard
                    Buffer->NextCommand();
                    Buffer->BlockCopy(0, 1);
                }
                Event.What = evNone;
            } else if (Event.Mouse.Buttons == 2) {
                // right mouse button down
                Buffer->SetNearPos(xx, yy);
            }
            break;
        case evMouseAuto:
        case evMouseMove:
            if (View->MView->MouseCaptured) {
                if (Event.Mouse.Buttons == 1) {
                    // left mouse button move
                    if (!View->MView->MouseMoved) {
                        if (Event.Mouse.KeyMask == kfCtrl) Buffer->BlockMarkColumn();
                        else if (Event.Mouse.KeyMask == kfAlt) Buffer->BlockMarkLine();
                        else Buffer->BlockMarkStream();
                        Buffer->BlockUnmark();
                        if (Event.What == evMouseMove)
                            View->MView->MouseMoved = 1;
                    }
                    Buffer->BlockExtendBegin();
                    Buffer->SetNearPos(xx, yy);
                    Buffer->BlockExtendEnd();
                } else if (Event.Mouse.Buttons == 2) {
                    // right mouse button move
                    if (Event.Mouse.KeyMask == kfAlt) {
                    } else {
                        Buffer->SetNearPos(xx, yy);
                    }
                }

                Event.What = evNone;
            }
            break;
/*        case evMouseAuto:
            if (View->MView->MouseCaptured) {
                Event.What = evNone;
            }
            break;*/
        case evMouseUp:
            if (View->MView->MouseCaptured)
                View->MView->Win->CaptureMouse(0);
            else
                break;
            View->MView->MouseCaptured = 0;
            if (Event.Mouse.Buttons == 1) {
                // left mouse button up
                if (View->MView->MouseMoved)
                    if (SystemClipboard) {
                        // note: copy to second clipboard
                        Buffer->NextCommand();
                        Buffer->BlockCopy(0, 1);
                    }
            }
            if (Event.Mouse.Buttons == 2) {
                // right mouse button up
                if (!View->MView->MouseMoved) {
                    EEventMap *Map = View->MView->Win->GetEventMap();
                    const char *MName = 0;

                    if (Map)
                        MName = Map->GetMenu(EM_LocalMenu);
                    if (MName == 0)
                        MName = "Local";
                    View->MView->Win->Parent->PopupMenu(MName);
                }
            }
            if (Event.Mouse.Buttons == 4) {
                // middle mouse button up
                if (SystemClipboard) {
                    // note: copy to second clipboard
                    Buffer->NextCommand();
                    if (Event.Mouse.KeyMask == 0)
                        Buffer->BlockPasteStream(1);
                    else if (Event.Mouse.KeyMask == kfCtrl)
                        Buffer->BlockPasteColumn(1);
                    else if (Event.Mouse.KeyMask == kfAlt)
                        Buffer->BlockPasteLine(1);
                }
            }
            Event.What = evNone;
            break;
        }
    }
}
#endif

void EEditPort::UpdateView() {
    Buffer->Redraw();
}

void EEditPort::RepaintView() {
    RedrawAll();
}

void EEditPort::UpdateStatus() {
}

void EEditPort::RepaintStatus() {
    //Buffer->Redraw();
}

EEventMap *EBuffer::GetEventMap() {
    return FindActiveMap(Mode);
}

int EBuffer::BeginMacro() {
    return NextCommand();
}

// *INDENT-OFF*
int EBuffer::ExecCommand(ExCommands Command, ExState &State) {
    switch (Command) {
    case ExMoveUp:                return MoveUp();
    case ExMoveDown:              return MoveDown();
    case ExMoveLeft:              return MoveLeft();
    case ExMoveRight:             return MoveRight();
    case ExMovePrev:              return MovePrev();
    case ExMoveNext:              return MoveNext();
    case ExMoveWordLeft:          return MoveWordLeft();
    case ExMoveWordRight:         return MoveWordRight();
    case ExMoveWordPrev:          return MoveWordPrev();
    case ExMoveWordNext:          return MoveWordNext();
    case ExMoveWordEndLeft:       return MoveWordEndLeft();
    case ExMoveWordEndRight:      return MoveWordEndRight();
    case ExMoveWordEndPrev:       return MoveWordEndPrev();
    case ExMoveWordEndNext:       return MoveWordEndNext();
    case ExMoveWordOrCapLeft:     return MoveWordOrCapLeft();
    case ExMoveWordOrCapRight:    return MoveWordOrCapRight();
    case ExMoveWordOrCapPrev:     return MoveWordOrCapPrev();
    case ExMoveWordOrCapNext:     return MoveWordOrCapNext();
    case ExMoveWordOrCapEndLeft:  return MoveWordOrCapEndLeft();
    case ExMoveWordOrCapEndRight: return MoveWordOrCapEndRight();
    case ExMoveWordOrCapEndPrev:  return MoveWordOrCapEndPrev();
    case ExMoveWordOrCapEndNext:  return MoveWordOrCapEndNext();
    case ExMoveLineStart:         return MoveLineStart();
    case ExMoveLineEnd:           return MoveLineEnd();
    case ExMovePageStart:         return MovePageStart();
    case ExMovePageEnd:           return MovePageEnd();
    case ExMovePageUp:            return MovePageUp();
    case ExMovePageDown:          return MovePageDown();
    case ExMovePageLeft:          return MovePageLeft();
    case ExMovePageRight:         return MovePageEnd();
    case ExMoveFileStart:         return MoveFileStart();
    case ExMoveFileEnd:           return MoveFileEnd();
    case ExMoveBlockStart:        return MoveBlockStart();
    case ExMoveBlockEnd:          return MoveBlockEnd();
    case ExMoveFirstNonWhite:     return MoveFirstNonWhite();
    case ExMoveLastNonWhite:      return MoveLastNonWhite();
    case ExMovePrevEqualIndent:   return MovePrevEqualIndent();
    case ExMoveNextEqualIndent:   return MoveNextEqualIndent();
    case ExMovePrevTab:           return MovePrevTab();
    case ExMoveNextTab:           return MoveNextTab();
    case ExMoveTabStart:          return MoveTabStart();
    case ExMoveTabEnd:            return MoveTabEnd();
    case ExMoveLineTop:           return MoveLineTop();
    case ExMoveLineCenter:        return MoveLineCenter();
    case ExMoveLineBottom:        return MoveLineBottom();
    case ExMoveBeginOrNonWhite:   return MoveBeginOrNonWhite();
    case ExMoveBeginLinePageFile: return MoveBeginLinePageFile();
    case ExMoveEndLinePageFile:   return MoveEndLinePageFile();
    case ExScrollLeft:            return ScrollLeft(State);
    case ExScrollRight:           return ScrollRight(State);
    case ExScrollDown:            return ScrollDown(State);
    case ExScrollUp:              return ScrollUp(State);
    case ExKillLine:              return KillLine();
    case ExKillChar:              return KillChar();
    case ExKillCharPrev:          return KillCharPrev();
    case ExKillWord:              return KillWord();
    case ExKillWordPrev:          return KillWordPrev();
    case ExKillWordOrCap:         return KillWordOrCap();
    case ExKillWordOrCapPrev:     return KillWordOrCapPrev();
    case ExKillToLineStart:       return KillToLineStart();
    case ExKillToLineEnd:         return KillToLineEnd();
    case ExKillBlock:             return KillBlock();
    case ExBackSpace:             return BackSpace();
    case ExDelete:                return Delete();
    case ExCharCaseUp:            return CharCaseUp();
    case ExCharCaseDown:          return CharCaseDown();
    case ExCharCaseToggle:        return CharCaseToggle();
    case ExLineCaseUp:            return LineCaseUp();
    case ExLineCaseDown:          return LineCaseDown();
    case ExLineCaseToggle:        return LineCaseToggle();
    case ExLineInsert:            return LineInsert();
    case ExLineAdd:               return LineAdd();
    case ExLineSplit:             return LineSplit();
    case ExLineJoin:              return LineJoin();
    case ExLineNew:               return LineNew();
    case ExLineIndent:            return LineIndent();
    case ExLineTrim:              return LineTrim();
    case ExLineCenter:            return LineCenter();
    case ExInsertSpacesToTab:
        {
            int no;

            if(State.GetIntParam(View, &no) == 0)
                no = 0;
            return InsertSpacesToTab(no);
        }
    case ExInsertTab:             return InsertTab();
    case ExInsertSpace:           return InsertSpace();
    case ExWrapPara:
#ifdef CONFIG_WORDWRAP
        return WrapPara();
#else
        return 0;
#endif
    case ExInsPrevLineChar:       return InsPrevLineChar();
    case ExInsPrevLineToEol:      return InsPrevLineToEol();
    case ExLineDuplicate:         return LineDuplicate();
    case ExBlockBegin:            return BlockBegin();
    case ExBlockEnd:              return BlockEnd();
    case ExBlockUnmark:           return BlockUnmark();
    case ExBlockCut:              return BlockCut(0);
    case ExBlockCopy:             return BlockCopy(0);
    case ExBlockCutAppend:        return BlockCut(1);
    case ExBlockCopyAppend:       return BlockCopy(1);
    case ExClipClear:             return ClipClear();
    case ExBlockPaste:            return BlockPaste();
    case ExBlockKill:             return BlockKill();
    case ExBlockIndent:
        {
            int saved_persistence, ret_code;

            saved_persistence = BFI(this, BFI_PersistentBlocks);
            BFI_SET(this, BFI_PersistentBlocks, 1);
            ret_code = BlockIndent();
            BFI_SET(this, BFI_PersistentBlocks, saved_persistence);
            return ret_code;
        }
    case ExBlockUnindent:
        {
            int saved_persistence, ret_code;

            saved_persistence = BFI(this, BFI_PersistentBlocks);
            BFI_SET(this, BFI_PersistentBlocks, 1);
            ret_code = BlockUnindent();
            BFI_SET(this, BFI_PersistentBlocks, saved_persistence);
            return ret_code;
        }
    case ExBlockClear:            return BlockClear();
    case ExBlockMarkStream:       return BlockMarkStream();
    case ExBlockMarkLine:         return BlockMarkLine();
    case ExBlockMarkColumn:       return BlockMarkColumn();
    case ExBlockCaseUp:           return BlockCaseUp();
    case ExBlockCaseDown:         return BlockCaseDown();
    case ExBlockCaseToggle:       return BlockCaseToggle();
    case ExBlockExtendBegin:      return BlockExtendBegin();
    case ExBlockExtendEnd:        return BlockExtendEnd();
    case ExBlockReIndent:         return BlockReIndent();
    case ExBlockSelectWord:       return BlockSelectWord();
    case ExBlockSelectLine:       return BlockSelectLine();
    case ExBlockSelectPara:       return BlockSelectPara();
    case ExBlockUnTab:            return BlockUnTab();
    case ExBlockEnTab:            return BlockEnTab();
#ifdef CONFIG_UNDOREDO
    case ExUndo:                  return Undo();
    case ExRedo:                  return Redo();
#else
    case ExUndo:                  return 0;
    case ExRedo:                  return 0;
#endif
    case ExMatchBracket:          return MatchBracket();
    case ExMovePrevPos:           return MovePrevPos();
    case ExMoveSavedPosCol:       return MoveSavedPosCol();
    case ExMoveSavedPosRow:       return MoveSavedPosRow();
    case ExMoveSavedPos:          return MoveSavedPos();
    case ExSavePos:               return SavePos();
    case ExCompleteWord:          return CompleteWord();
    case ExBlockPasteStream:      return BlockPasteStream();
    case ExBlockPasteLine:        return BlockPasteLine();
    case ExBlockPasteColumn:      return BlockPasteColumn();
    case ExBlockPasteOver:        return BlockPasteOver();
    case ExShowPosition:          return ShowPosition();
    case ExFoldCreate:            return FoldCreate(VToR(CP.Row));
    case ExFoldDestroy:           return FoldDestroy(VToR(CP.Row));
    case ExFoldDestroyAll:        return FoldDestroyAll();
    case ExFoldPromote:           return FoldPromote(VToR(CP.Row));
    case ExFoldDemote:            return FoldDemote(VToR(CP.Row));
    case ExFoldOpen:              return FoldOpen(VToR(CP.Row));
    case ExFoldOpenNested:        return FoldOpenNested();
    case ExFoldClose:             return FoldClose(VToR(CP.Row));
    case ExFoldOpenAll:           return FoldOpenAll();
    case ExFoldCloseAll:          return FoldCloseAll();
    case ExFoldToggleOpenClose:   return FoldToggleOpenClose();
    case ExFoldCreateAtRoutines:  return FoldCreateAtRoutines();
    case ExMoveFoldTop:           return MoveFoldTop();
    case ExMoveFoldPrev:          return MoveFoldPrev();
    case ExMoveFoldNext:          return MoveFoldNext();
    case ExFileSave:              return Save();
    case ExFilePrint:             return FilePrint();
    case ExBlockPrint:            return BlockPrint();
    case ExBlockTrim:             return BlockTrim();
    case ExFileTrim:              return FileTrim();
    case ExHilitWord:
#ifdef CONFIG_WORD_HILIT
        return HilitWord();
#else
        return 0;
#endif
    case ExSearchWordPrev:        return SearchWord(SEARCH_BACK | SEARCH_NEXT);
    case ExSearchWordNext:        return SearchWord(SEARCH_NEXT);
    case ExHilitMatchBracket:     return HilitMatchBracket();
    case ExToggleAutoIndent:      return ToggleAutoIndent();
    case ExToggleInsert:          return ToggleInsert();
    case ExToggleExpandTabs:      return ToggleExpandTabs();
    case ExToggleShowTabs:        return ToggleShowTabs();
    case ExToggleUndo:            return ToggleUndo();
    case ExToggleReadOnly:        return ToggleReadOnly();
    case ExToggleKeepBackups:     return ToggleKeepBackups();
    case ExToggleMatchCase:       return ToggleMatchCase();
    case ExToggleBackSpKillTab:   return ToggleBackSpKillTab();
    case ExToggleDeleteKillTab:   return ToggleDeleteKillTab();
    case ExToggleSpaceTabs:       return ToggleSpaceTabs();
    case ExToggleIndentWithTabs:  return ToggleIndentWithTabs();
    case ExToggleBackSpUnindents: return ToggleBackSpUnindents();
    case ExToggleWordWrap:        return ToggleWordWrap();
    case ExToggleTrim:            return ToggleTrim();
    case ExToggleShowMarkers:     return ToggleShowMarkers();
    case ExToggleHilitTags:       return ToggleHilitTags();
    case ExToggleShowBookmarks:   return ToggleShowBookmarks();
    case ExToggleMakeBackups:     return ToggleMakeBackups();
    case ExSetLeftMargin:         return SetLeftMargin();
    case ExSetRightMargin:        return SetRightMargin();
    case ExSetIndentWithTabs:     return SetIndentWithTabs(State);

        // stuff with UI
    case ExMoveToLine:          return MoveToLine(State);
    case ExMoveToColumn:        return MoveToColumn(State);
    case ExFoldCreateByRegexp:  return FoldCreateByRegexp(State);
#ifdef CONFIG_BOOKMARKS
    case ExPlaceBookmark:       return PlaceBookmark(State);
    case ExRemoveBookmark:      return RemoveBookmark(State);
    case ExGotoBookmark:        return GotoBookmark(State);
#else
    case ExPlaceBookmark:       return 0;
    case ExRemoveBookmark:      return 0;
    case ExGotoBookmark:        return 0;
#endif
    case ExPlaceGlobalBookmark: return PlaceGlobalBookmark(State);
    case ExPushGlobalBookmark:  return PushGlobalBookmark();
    case ExInsertString:        return InsertString(State);
    case ExSelfInsert:          return SelfInsert(State);
    case ExFileReload:          return FileReload(State);
    case ExFileSaveAs:          return FileSaveAs(State);
    case ExFileWriteTo:         return FileWriteTo(State);
    case ExBlockRead:           return BlockRead(State);
    case ExBlockReadStream:     return BlockReadStream(State);
    case ExBlockReadLine:       return BlockReadLine(State);
    case ExBlockReadColumn:     return BlockReadColumn(State);
    case ExBlockWrite:          return BlockWrite(State);
    case ExBlockSort:           return BlockSort(0);
    case ExBlockSortReverse:    return BlockSort(1);
    case ExFind:                return Find(State);
    case ExFindReplace:         return FindReplace(State);
    case ExFindRepeat:          return FindRepeat(State);
    case ExFindRepeatOnce:      return FindRepeatOnce(State);
    case ExFindRepeatReverse:   return FindRepeatReverse(State);
    case ExSearch:              return Search(State);
    case ExSearchB:             return SearchB(State);
    case ExSearchRx:            return SearchRx(State);
    case ExSearchAgain:         return SearchAgain(State);
    case ExSearchAgainB:        return SearchAgainB(State);
    case ExSearchReplace:       return SearchReplace(State);
    case ExSearchReplaceB:      return SearchReplaceB(State);
    case ExSearchReplaceRx:     return SearchReplaceRx(State);
    case ExInsertChar:          return InsertChar(State);
    case ExTypeChar:            return TypeChar(State);
    case ExChangeMode:          return ChangeMode(State);
    //case ExChangeKeys:          return ChangeKeys(State);
    case ExChangeFlags:         return ChangeFlags(State);
    case ExChangeTabSize:       return ChangeTabSize(State);
    case ExChangeLeftMargin:    return ChangeLeftMargin(State);
    case ExChangeRightMargin:   return ChangeRightMargin(State);
    case ExASCIITable:
#ifdef CONFIG_I_ASCII
        return ASCIITable(State);
#else
        return 0;
#endif
    case ExCharTrans:           return CharTrans(State);
    case ExLineTrans:           return LineTrans(State);
    case ExBlockTrans:          return BlockTrans(State);

#ifdef CONFIG_TAGS
    case ExTagFind:             return FindTag(State);
    case ExTagFindWord:         return FindTagWord(State);
#endif

    case ExSetCIndentStyle:     return SetCIndentStyle(State);

    case ExBlockMarkFunction:   return BlockMarkFunction();
    case ExIndentFunction:      return IndentFunction();
    case ExMoveFunctionPrev:    return MoveFunctionPrev();
    case ExMoveFunctionNext:    return MoveFunctionNext();
    case ExInsertDate:          return InsertDate(State);
    case ExInsertUid:           return InsertUid();
    case ExShowHelpWord:        return ShowHelpWord(State);
    default:
        ;
    }
    return EModel::ExecCommand(Command, State);
}
// *INDENT-ON*

void EBuffer::HandleEvent(TEvent &Event) {
    EModel::HandleEvent(Event);
}

int EBuffer::MoveToLine(ExState &State) {
    int No = 0;

    if (State.GetIntParam(View, &No) == 0) {
        char Num[10];

        sprintf(Num, "%d", VToR(CP.Row) + 1);
        if (View->MView->Win->GetStr("Goto Line", sizeof(Num), Num, HIST_POSITION) == 0)
            return 0;
        No = atol(Num);
    }
    return SetNearPosR(CP.Col, No - 1);
}

int EBuffer::MoveToColumn(ExState &State) {
    int No = 0;

    if (State.GetIntParam(View, &No) == 0) {
        char Num[10];

        sprintf(Num, "%d", CP.Col + 1);
        if (View->MView->Win->GetStr("Goto Column", 8, Num, HIST_POSITION) == 0) return 0;
        No = atol(Num);
    }
    return SetNearPos(No - 1, CP.Row);
}

int EBuffer::FoldCreateByRegexp(ExState &State) {
    char strbuf[1024] = "";

    if (State.GetStrParam(View, strbuf, sizeof(strbuf)) == 0) {
        if (View->MView->Win->GetStr("Create Fold Regexp", sizeof(strbuf), strbuf, HIST_REGEXP) == 0) return 0;
    }
    return FoldCreateByRegexp(strbuf);
}

#ifdef CONFIG_BOOKMARKS
int EBuffer::PlaceUserBookmark(const char *n,EPoint P) {
    char name[256+4] = "_BMK";
    int result;
    EPoint prev;

    strcpy (name+4,n);
    if (GetBookmark (name,prev)==0) {
        prev.Row=-1;prev.Col=-1;
    }
    result=PlaceBookmark(name, P);
    if (result) {
        if (BFI (this,BFI_ShowBookmarks)) {
            FullRedraw ();
        }
        if (BFI (this,BFI_SaveBookmarks)==1||BFI (this,BFI_SaveBookmarks)==2) {
            if (!Modify ()) return result;   // Never try to save to read-only
#ifdef CONFIG_UNDOREDO
            if (BFI(this, BFI_Undo)) {
                if (PushULong(prev.Row) == 0) return 0;
                if (PushULong(prev.Col) == 0) return 0;
                if (PushUData(n, strlen(n) + 1) == 0) return 0;
                if (PushULong(strlen (n)+1) == 0) return 0;
                if (PushUChar(ucPlaceUserBookmark) == 0) return 0;
            }
#endif
        }
    }
    return result;
}

int EBuffer::RemoveUserBookmark(const char *n) {
    char name[256+4] = "_BMK";
    int result;
    EPoint p;

    strcpy (name+4,n);
    GetBookmark (name,p);       // p is valid only if remove is successful
    result=RemoveBookmark(name);
    if (result) {
        if (BFI (this,BFI_ShowBookmarks)) {
            FullRedraw ();
        }
        if (BFI (this,BFI_SaveBookmarks)==1||BFI (this,BFI_SaveBookmarks)==2) {
            if (!Modify ()) return result;   // Never try to save to read-only
#ifdef CONFIG_UNDOREDO
            if (PushULong(p.Row) == 0) return 0;
            if (PushULong(p.Col) == 0) return 0;
            if (PushUData((void *)n,strlen (n)+1) == 0) return 0;
            if (PushULong(strlen (n)+1) == 0) return 0;
            if (PushUChar(ucRemoveUserBookmark) == 0) return 0;
#endif
        }
    }
    return result;
}

int EBuffer::GotoUserBookmark(const char *n) {
    char name[256+4] = "_BMK";

    strcpy (name+4,n);
    return GotoBookmark(name);
}

int EBuffer::GetUserBookmarkForLine(int searchFrom, int searchForLine, const EBookmark* &eb) {
    int i = searchFrom;

    while ((i = GetBookmarkForLine(i, searchForLine, eb) != -1))
        if (strncmp(eb->GetName(), "_BMK", 4) == 0)
            return i;

    return -1;
}

int EBuffer::PlaceBookmark(ExState &State) {
    char name[256] = "";
    EPoint P = CP;

    P.Row = VToR(P.Row);

    if (State.GetStrParam(View, name, sizeof(name)) == 0)
        if (View->MView->Win->GetStr("Place Bookmark", sizeof(name), name, HIST_BOOKMARK) == 0) return 0;
    return PlaceUserBookmark(name, P);
}

int EBuffer::RemoveBookmark(ExState &State) {
    char name[256] = "";

    if (State.GetStrParam(View, name, sizeof(name)) == 0)
        if (View->MView->Win->GetStr("Remove Bookmark", sizeof(name), name, HIST_BOOKMARK) == 0) return 0;
    return RemoveUserBookmark(name);
}

int EBuffer::GotoBookmark(ExState &State) {
    char name[256] = "";

    if (State.GetStrParam(View, name, sizeof(name)) == 0)
        if (View->MView->Win->GetStr("Goto Bookmark", sizeof(name), name, HIST_BOOKMARK) == 0) return 0;
    return GotoUserBookmark(name);
}
#endif

int EBuffer::PlaceGlobalBookmark(ExState &State) {
    char name[256] = "";
    EPoint P = CP;

    P.Row = VToR(P.Row);

    if (State.GetStrParam(View, name, sizeof(name)) == 0)
        if (View->MView->Win->GetStr("Place Global Bookmark", sizeof(name), name, HIST_BOOKMARK) == 0) return 0;
    if (markIndex.insert(name, this, P) == 0) {
        Msg(S_ERROR, "Error placing global bookmark %s.", name);
    }
    return 1;
}

int EBuffer::PushGlobalBookmark() {
    EPoint P = CP;

    P.Row = VToR(P.Row);
    EMark *m = markIndex.pushMark(this, P);
    if (m)
         Msg(S_INFO, "Placed bookmark %s", m->GetName());
    return m ? 1 : 0;
}

int EBuffer::InsertChar(ExState &State) {
    char Ch;
    int No;

    if (State.GetIntParam(View, &No) == 0) {
        TEvent E;
        E.What = evKeyDown;
        E.Key.Code = View->MView->Win->GetChar("Quote Char:");
        if (!GetCharFromEvent(E, &Ch)) return 0;
        No = Ch;
    }
    if (No < 0 || No > 255) return 0;
    Ch = char(No);
    return InsertChar(Ch);
}

int EBuffer::TypeChar(ExState &State) {
    char Ch;
    int No;

    if (State.GetIntParam(View, &No) == 0) {
        TEvent E;
        E.What = evKeyDown;
        E.Key.Code = View->MView->Win->GetChar(0);
        if (!GetCharFromEvent(E, &Ch)) return 0;
        No = Ch;
    }
    if (No < 0 || No > 255) return 0;
    Ch = char(No);
    return TypeChar(Ch);
}

int EBuffer::InsertString(ExState &State) {
    char strbuf[1024] = "";

    if (State.GetStrParam(View, strbuf, sizeof(strbuf)) == 0) {
        if (View->MView->Win->GetStr("Insert String", sizeof(strbuf), strbuf, HIST_DEFAULT) == 0)
            return 0;
    }
    return InsertString(strbuf, strlen(strbuf));
}

extern int LastEventChar;

int EBuffer::SelfInsert(ExState &/*State*/) {
    if (LastEventChar != -1)
        return TypeChar(char(LastEventChar));
    return 0;
}

int EBuffer::FileReload(ExState &/*State*/) {
    if (Modified) {
        switch (View->MView->Win->Choice(GPC_ERROR, "File Modified",
                       2,
                       "&Reload",
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
//    GetNewNumber();
    return Reload();
}

int EBuffer::FileSaveAs(const char *FName) {
    char Name[MAXPATH];

    if (ExpandPath(FName, Name, sizeof(Name)) == -1) {
        View->MView->Win->Choice(GPC_ERROR, "Error", 1, "O&K", "Invalid path: %s.", FName);
        return 0;
    }
    if (FindFile(Name) == 0) {
        if (FileExists(Name)) {
            switch (View->MView->Win->Choice(GPC_ERROR, "File Exists",
                           2,
                           "&Overwrite",
                           "&Cancel",
                           "%s", Name))
            {
            case 0:
                break;
            case 1:
            case -1:
            default:
                return 0;

            }
        }
        free(FileName);
        FileName = strdup(Name);
        UpdateTitle();
        return Save();
    } else {
        View->MView->Win->Choice(GPC_ERROR, "Error", 1, "O&K", "Already editing '%s.'", Name);
        return 0;
    }
}

int EBuffer::FileSaveAs(ExState &State) {
    char FName[MAXPATH];

    strcpy(FName, FileName);
    if (State.GetStrParam(View, FName, sizeof(FName)) == 0)
        if (View->MView->Win->GetFile("Save As", sizeof(FName), FName, HIST_PATH, GF_SAVEAS) == 0)
            return 0;
    return FileSaveAs(FName);
}

int EBuffer::FileWriteTo(const char *FName) {
    char Name[MAXPATH];

    if (ExpandPath(FName, Name, sizeof(Name)) == -1) {
        View->MView->Win->Choice(GPC_ERROR, "Error", 1, "O&K", "Invalid path: %s.", FName);
        return 0;
    }
    if (FindFile(Name) == 0) {
        if (FileExists(Name)) {
            switch (View->MView->Win->Choice(GPC_ERROR, "File Exists",
                           2,
                           "&Overwrite",
                           "&Cancel",
                           "%s", Name))
            {
            case 0:
                break;
            case 1:
            case -1:
            default:
                return 0;
            }
        }
        return SaveTo(Name);
    } else {
        View->MView->Win->Choice(GPC_ERROR, "Error", 1, "O&K", "Already editing '%s.'", Name);
        return 0;
    }
}

int EBuffer::FileWriteTo(ExState &State) {
    char FName[MAXPATH];

    strcpy(FName, FileName);
    if (State.GetStrParam(View, FName, sizeof(FName)) == 0)
        if (View->MView->Win->GetFile("Write To", sizeof(FName), FName, HIST_PATH, GF_SAVEAS) == 0) return 0;
    return FileWriteTo(FName);
}

int EBuffer::BlockReadX(ExState &State, int blockMode) {
    char Name[MAXPATH];
    char FName[MAXPATH];

    if (JustDirectory(FileName, FName, sizeof(FName)) == -1) return 0;
    SlashDir(FName);
    if (State.GetStrParam(View, FName, sizeof(FName)) == 0)
        if (View->MView->Win->GetFile("Read block", sizeof(FName), FName, HIST_PATH, GF_OPEN) == 0) return 0;

    if (ExpandPath(FName, Name, sizeof(Name)) == -1) {
        View->MView->Win->Choice(GPC_ERROR, "Error", 1, "O&K", "Invalid path: %s.", FName);
        return 0;
    }
    return BlockReadFrom(FName, blockMode);
}

int EBuffer::BlockRead(ExState &State) {
    return BlockReadX(State, BlockMode);
}

int EBuffer::BlockReadStream(ExState &State) {
    return BlockReadX(State, bmStream);
}

int EBuffer::BlockReadLine(ExState &State) {
    return BlockReadX(State, bmLine);
}

int EBuffer::BlockReadColumn(ExState &State) {
    return BlockReadX(State, bmColumn);
}

int EBuffer::BlockWrite(ExState &State) {
    char Name[MAXPATH];
    char FName[MAXPATH];
    int Append = 0;

    if (JustDirectory(FileName, FName, sizeof(FName)) == -1) return 0;
    SlashDir(FName);
    if (State.GetStrParam(View, FName, sizeof(FName)) == 0)
        if (View->MView->Win->GetFile("Write block", sizeof(FName), FName, HIST_PATH, GF_SAVEAS) == 0)
            return 0;

    if (ExpandPath(FName, Name, sizeof(Name)) == -1) {
        View->MView->Win->Choice(GPC_ERROR, "Error", 1, "O&K", "Invalid path: %s.", FName);
        return 0;
    }
    if (FindFile(Name) == 0) {
        if (FileExists(Name)) {
            switch (View->MView->Win->Choice(GPC_ERROR, "File Exists",
                           3,
                           "&Overwrite",
                           "&Append",
                           "&Cancel",
                           "%s", Name))
            {
            case 0:
                break;
            case 1:
                Append = 1;
                break;
            case 2:
            case -1:
            default:
                return 0;

            }
        }
    } else {
        View->MView->Win->Choice(GPC_ERROR, "Error", 1, "O&K", "Already editing '%s.'", Name);
        return 0;
    }
    return BlockWriteTo(Name, Append);
}

int EBuffer::Find(ExState &State) {
    char find[MAXSEARCH+1] = "";
    char options[32] = "";

    if (State.GetStrParam(View, find, sizeof(find)) != 0) {
        if (State.GetStrParam(View, options, sizeof(options)) == 0)
            strcpy(options, BFS(this, BFS_DefFindOpt));

        LSearch.ok = 0;
        strcpy(LSearch.strSearch, find);
        LSearch.strReplace[0] = 0;
        LSearch.Options = 0;
        if (ParseSearchOptions(0, options, LSearch.Options) == 0) return 0;
        LSearch.ok = 1;
    } else if ((HaveGUIDialogs & GUIDLG_FIND) && GUIDialogs) {
        LSearch.ok = 0;
        LSearch.strSearch[0] = 0;
        LSearch.strReplace[0] = 0;
        LSearch.Options = 0;
        if (BFS(this, BFS_DefFindOpt))
            strcpy(options, BFS(this, BFS_DefFindOpt));
        if (ParseSearchOptions(0, options, LSearch.Options) == 0)
            LSearch.Options = 0;

        if (DLGGetFind(View->MView->Win, LSearch) == 0)
            return 0;
    } else {
        if (BFS(this, BFS_DefFindOpt))
            strcpy(options, BFS(this, BFS_DefFindOpt));
        if (View->MView->Win->GetStr("Find", sizeof(find), find, HIST_SEARCH) == 0) return 0;
        if (View->MView->Win->GetStr("Options (All/Block/Cur/Delln/Glob/Igncase/Joinln/Rev/Word/regX)", sizeof(options), options, HIST_SEARCHOPT) == 0) return 0;

        LSearch.ok = 0;
        strcpy(LSearch.strSearch, find);
        LSearch.strReplace[0] = 0;
        LSearch.Options = 0;
        if (ParseSearchOptions(0, options, LSearch.Options) == 0) return 0;
        LSearch.ok = 1;
    }
    if (LSearch.ok == 0) return 0;
    LSearch.Options |= SEARCH_CENTER;
    if (Find(LSearch) == 0) return 0;
    return 1;
}

int EBuffer::FindReplace(ExState &State) {
    char find[MAXSEARCH+1] = "";
    char replace[MAXSEARCH+1] = "";
    char options[32] = "";

    if (State.GetStrParam(View, find, sizeof(find)) != 0) {
        if (State.GetStrParam(View, replace, sizeof(replace)) == 0)
            return 0;
        if (State.GetStrParam(View, options, sizeof(options)) == 0)
            return 0;

        LSearch.ok = 0;
        strcpy(LSearch.strSearch, find);
        strcpy(LSearch.strReplace, replace);
        LSearch.Options = 0;
        if (ParseSearchOptions(1, options, LSearch.Options) == 0) return 0;
        LSearch.Options |= SEARCH_REPLACE;
        LSearch.ok = 1;
    } else if ((HaveGUIDialogs & GUIDLG_FINDREPLACE) && GUIDialogs) {
        LSearch.ok = 0;
        LSearch.strSearch[0] = 0;
        LSearch.strReplace[0] = 0;
        LSearch.Options = 0;
        if (BFS(this, BFS_DefFindReplaceOpt))
            strcpy(options, BFS(this, BFS_DefFindReplaceOpt));
        if (ParseSearchOptions(1, options, LSearch.Options) == 0)
            LSearch.Options = 0;
        if (DLGGetFindReplace(View->MView->Win, LSearch) == 0)
            return 0;
    } else {
        if (BFS(this, BFS_DefFindReplaceOpt))
            strcpy(options, BFS(this, BFS_DefFindReplaceOpt));
        if (State.GetStrParam(View, find, sizeof(find)) == 0)
            if (View->MView->Win->GetStr("Find", sizeof(find), find, HIST_SEARCH) == 0) return 0;
        if (State.GetStrParam(View, replace, sizeof(replace)) == 0)
            if (View->MView->Win->GetStr("Replace", sizeof(replace), replace, HIST_SEARCH) == 0) return 0;
        if (State.GetStrParam(View, options, sizeof(options)) == 0)
            if (View->MView->Win->GetStr("Options (All/Block/Cur/Delln/Glob/Igncase/Joinln/Rev/Noask/Word/regX)", sizeof(options), options, HIST_SEARCHOPT) == 0) return 0;

        LSearch.ok = 0;
        strcpy(LSearch.strSearch, find);
        strcpy(LSearch.strReplace, replace);
        LSearch.Options = 0;
        if (ParseSearchOptions(1, options, LSearch.Options) == 0) return 0;
        LSearch.Options |= SEARCH_REPLACE;
        LSearch.ok = 1;
    }
    if (LSearch.ok == 0) return 0;
    LSearch.Options |= SEARCH_CENTER;
    if (Find(LSearch) == 0) return 0;
    return 1;
}

int EBuffer::FindRepeat(ExState &State) {
    if (LSearch.ok == 0) return Find(State);
    LSearch.Options |= SEARCH_NEXT;
    LSearch.Options &= ~SEARCH_GLOBAL;
    if (Find(LSearch) == 0) return 0;
    return 1;
}

int EBuffer::FindRepeatReverse(ExState &State) {
    int rc;

    if (LSearch.ok == 0) return Find(State);
    LSearch.Options |= SEARCH_NEXT;
    LSearch.Options &= ~SEARCH_GLOBAL;
    LSearch.Options ^= SEARCH_BACK;
    rc = Find(LSearch);
    LSearch.Options ^= SEARCH_BACK;
    return rc;
}

int EBuffer::FindRepeatOnce(ExState &State) {
    if (LSearch.ok == 0) return Find(State);
    LSearch.Options |= SEARCH_NEXT;
    LSearch.Options &= ~SEARCH_GLOBAL;
    LSearch.Options &= ~SEARCH_ALL;
    if (Find(LSearch) == 0) return 0;
    return 1;
}

int EBuffer::ChangeMode(ExState &State) {
    char Mode[32] = "";
    int rc;

    if (State.GetStrParam(View, Mode, sizeof(Mode)) == 0)
        if (View->MView->Win->GetStr("Mode", sizeof(Mode), Mode, HIST_SETUP) == 0) return 0;

    rc = ChangeMode(Mode);
    FullRedraw();
    return rc;
}

int EBuffer::ChangeKeys(ExState &State) {
    int rc;
    char Mode[32] = "";

    if (State.GetStrParam(View, Mode, sizeof(Mode)) == 0)
        if (View->MView->Win->GetStr("Mode", sizeof(Mode), Mode, HIST_SETUP) == 0) return 0;

    rc = ChangeKeys(Mode);
    FullRedraw();
    return rc;
}

int EBuffer::ChangeFlags(ExState &State) {
    int rc;
    char Mode[32] = "";

    if (State.GetStrParam(View, Mode, sizeof(Mode)) == 0)
        if (View->MView->Win->GetStr("Mode", sizeof(Mode), Mode, HIST_SETUP) == 0) return 0;

    rc = ChangeFlags(Mode);
    FullRedraw();
    return rc;
}

int EBuffer::ChangeTabSize(ExState &State) {
    int No;

    if (State.GetIntParam(View, &No) == 0) {
        char Num[10];

        sprintf(Num, "%d", BFI(this, BFI_TabSize));
        if (View->MView->Win->GetStr("TabSize", sizeof(Num), Num, HIST_SETUP) == 0) return 0;
        No = atol(Num);
    }
    if (No < 1) return 0;
    if (No > 32) return 0;
    BFI(this, BFI_TabSize) = No;
    FullRedraw();
    return 1;
}

int EBuffer::SetIndentWithTabs(ExState &State) {
    int No;

    if (State.GetIntParam(View, &No) == 0) return 0;
    Flags.num[BFI_IndentWithTabs] = No ? 1 : 0;
    return 1;
}

int EBuffer::ChangeRightMargin(ExState &State) {
    char Num[10];
    int No;

    if (State.GetIntParam(View, &No) == 0) {
        sprintf(Num, "%d", BFI(this, BFI_RightMargin) + 1);
        if (View->MView->Win->GetStr("RightMargin", sizeof(Num), Num, HIST_SETUP) == 0) return 0;
        No = atol(Num) - 1;
    }
    if (No <= 1) return 0;
    BFI(this, BFI_RightMargin) = No;
    Msg(S_INFO, "RightMargin set to %d.", No + 1);
    return 1;
}

int EBuffer::ChangeLeftMargin(ExState &State) {
    char Num[10];
    int No;

    if (State.GetIntParam(View, &No) == 0) {
        sprintf(Num, "%d", BFI(this, BFI_LeftMargin) + 1);
        if (View->MView->Win->GetStr("LeftMargin", sizeof(Num), Num, HIST_SETUP) == 0) return 0;
        No = atol(Num) - 1;
    }
    if (No < 0) return 0;
    BFI(this, BFI_LeftMargin) = No;
    Msg(S_INFO, "LeftMargin set to %d.", No + 1);
    return 1;
}


int EBuffer::CanQuit() {
    if (Modified)
        return 0;
    else
        return 1;
}

int EBuffer::ConfQuit(GxView *V, int multiFile) {
    if (Modified) {
        if (multiFile) {
            switch (V->Choice(GPC_ERROR,
                              "File Modified",
                              5,
                              "&Save",
                              "&As",
                              "A&ll",
                              "&Discard",
                              "&Cancel",
                              "%s", FileName))
            {
            case 0: /* Save */
                if (Save() == 0) return 0;
                break;
            case 1: /* As */
                {
                    char FName[MAXPATH];
                    strcpy(FName, FileName);
                    if (V->GetFile("Save As", sizeof(FName), FName, HIST_PATH, GF_SAVEAS) == 0) return 0;
                    if (FileSaveAs(FName) == 0) return 0;
                }
                break;
            case 2: /* Save all */
                return -2;
            case 3: /* Discard */
                break;
            case 4: /* Cancel */
            case -1:
            default:
                return 0;
            }
        }else {
            switch (V->Choice(GPC_ERROR,
                              "File Modified",
                              4,
                              "&Save",
                              "&As",
                              "&Discard",
                              "&Cancel",
                              "%s", FileName))
            {
            case 0: /* Save */
                if (Save() == 0) return 0;
                break;
            case 1: /* As */
                {
                    char FName[MAXPATH];
                    strcpy(FName, FileName);
                    if (V->GetFile("Save As", sizeof(FName), FName, HIST_PATH, GF_SAVEAS) == 0) return 0;
                    if (FileSaveAs(FName) == 0) return 0;
                }
                break;
            case 2: /* Discard */
                break;
            case 3: /* Cancel */
            case -1:
            default:
                return 0;
            }
        }
    }
    return 1;
}

void EBuffer::GetName(char *AName, size_t MaxLen) {
    strlcpy(AName, FileName, MaxLen);
}

void EBuffer::GetPath(char *APath, size_t MaxLen) {
    JustDirectory(FileName, APath, MaxLen);
}

void EBuffer::GetInfo(char *AInfo, size_t /*MaxLen*/) {
    char buf[256] = {0};
    char winTitle[256] = {0};

    JustFileName(FileName, buf, sizeof(buf));
    if (buf[0] == '\0') // if there is no filename, try the directory name.
        JustLastDirectory(FileName, buf, sizeof(buf));

    if (buf[0] != 0) // if there is a file/dir name, stick it in here.
    {
        strlcat(winTitle, buf, sizeof(winTitle));
        strlcat(winTitle, " - ", sizeof(winTitle));
    }
    strlcat(winTitle, FileName, sizeof(winTitle));

    sprintf(AInfo,
            "%2d %04d:%03d%c%-150s ",
            ModelNo,
            1 + CP.Row, 1 + CP.Col,
            Modified ? '*': ' ',
            winTitle);
}

void EBuffer::GetTitle(char *ATitle, size_t MaxLen, char *ASTitle, size_t SMaxLen) {

    strlcpy(ATitle, FileName, MaxLen);
    char *p = SepRChr(FileName);
    strlcpy(ASTitle, (p) ? p + 1 : FileName, SMaxLen);
}

#ifdef CONFIG_I_ASCII
int EBuffer::ASCIITable(ExState &/*State*/) {
    int rc;

    rc = View->MView->Win->PickASCII();
    if (rc != -1)
        return InsertChar(char(rc));

    return 0;
}
#endif

int EBuffer::ScrollLeft(ExState &State) {
    int Cols;

    if (State.GetIntParam(View, &Cols) == 0)
        Cols = 8;
    return ScrollLeft(Cols);
}

int EBuffer::ScrollRight(ExState &State) {
    int Cols;

    if (State.GetIntParam(View, &Cols) == 0)
        Cols = 8;
    return ScrollRight(Cols);
}

int EBuffer::ScrollDown(ExState &State) {
    int Rows;

    if (State.GetIntParam(View, &Rows) == 0)
        Rows = 1;
    return ScrollDown(Rows);
}

int EBuffer::ScrollUp(ExState &State) {
    int Rows;

    if (State.GetIntParam(View, &Rows) == 0)
        Rows = 1;
    return ScrollUp(Rows);
}

#ifdef CONFIG_TAGS
int EBuffer::FindTag(ExState &State) {
    char Tag[MAXSEARCH] = "";

    if (State.GetStrParam(View, Tag, sizeof(Tag)) == 0)
        if (View->MView->Win->GetStr("Find tag", sizeof(Tag), Tag, HIST_SEARCH) == 0) return 0;

    int j = 2;
    while (j--) {
        int i;

        i = TagFind(this, View, Tag);
        if (i > 0)
            return 1;
        else if (j && (i < 0)) {
            /* Try autoload tags */
            if (View->ExecCommand(ExTagLoad, State) == 0)
                break;
        } else {
            Msg(S_INFO, "Tag '%s' not found.", Tag);
            break;
        }
    }
    return 0;

}
#endif

// these two will probably be replaced in the future
int EBuffer::InsertDate(ExState &State) {
    char strArg[128] = "";
    char buf[128], *p;

    time_t t;

    time(&t);

    if (State.GetStrParam(View, strArg, sizeof(strArg))) {
        struct tm *tt = localtime(&t);
        strftime(buf, sizeof(buf), strArg, tt);
        buf[sizeof(buf) - 1] = 0;
    } else {
        //** 012345678901234567890123
        //** Wed Jan 02 02:23:54 1991
        p = ctime(&t);
        sprintf(buf, "%.10s %.4s", p, p + 20);
    }
    //puts(buf);

    return InsertString(buf, strlen(buf));
}


int EBuffer::InsertUid() {
    const char *p = getenv("USER");
    if (p == 0) p = getenv("NAME");
    if (p == 0) p = getenv("ID");
    // mostly for Windows.  Why they can't just be standard, I don't know...
    if (p == 0) p = getenv("USERNAME");
    if (p == 0) {
        Msg(S_INFO, "User ID not set ($USER).");
        //return 0;
	p = "UNKNOWN USER";
    }
    return InsertString(p, strlen(p));
}

int EBuffer::ShowHelpWord(ExState &State) {
    //** Code for BlockSelectWord to find the word under the cursor,
    const char *achr = "+-_."; // these are accepted characters
    char    buf[128];
    int     Y = VToR(CP.Row);
    PELine  L = RLine(Y);
    int     P;

    P = CharOffset(L, CP.Col);

    // fix \b for the case of CATBS
    for(int i = 0; i < P; i++) {
        //printf("%d - %d  %d   %c %c\n", i, P, L->Chars[i],
        //L->Chars[i], L->Chars[P]);
        if ((L->Chars[i] == '\b') && (P < (L->Count - 2)))
            P += 2;
    }
    size_t len = 0;
    if (P < L->Count) {
        // To start of word,
        while ((P > 0)
               && ((L->Chars[P - 1] == '\b') || isalnum(L->Chars[P - 1])
                   || (strchr(achr, L->Chars[P - 1]) != NULL)))
            P--; // '_' for underline is hidden in achr
        if ((P < (L->Count - 1)) && (L->Chars[P] == '\b'))
            P++;
        // To end of word,
        while ((len < (sizeof(buf) - 1)) && (P < L->Count)) {
            if (((P + 1) < L->Count) && (L->Chars[P + 1] == '\b'))
                P += 2;
            else if (isalnum(L->Chars[P])
                     || (strchr(achr, L->Chars[P]) != NULL))
                buf[len++] = L->Chars[P++];
            else
                break;
        }
    }
    buf[len] = 0;
    //printf("Word: %s\n", buf);
    //if (buf[0] == 0) {
    //    Msg(INFO, "No valid word under the cursor.");
    //    return 0;
    //}
    return View->SysShowHelp(State, buf[0] ? buf : 0);
}

int EBuffer::GetStrVar(int var, char *str, size_t buflen) {
    if (buflen == 0)
        return 0;
    //puts("variable EBuffer\x7");
    switch (var) {
    case mvFilePath:
        //puts("variable FilePath\x7");
        strncpy(str, FileName, buflen);
        str[buflen - 1] = 0;
        return 1;

    case mvFileName:
        JustFileName(FileName, str, buflen);
        return 1;

    case mvFileDirectory:
        JustDirectory(FileName, str, buflen);
        return 1;
    case mvFileBaseName:
        {
            char buf[MAXPATH];
            char *dot, *dot2;

            JustFileName(FileName, buf, sizeof(buf));

            dot = strchr(buf, '.');
            while ((dot2 = strchr(dot + 1, '.')) != NULL)
                dot = dot2;
            if (dot)
                *dot = 0;
            strlcpy(str, buf, buflen);
        }
        return 1;

    case mvFileExtension:
        {
            char buf[MAXPATH];
            char *dot;

            JustFileName(FileName, buf, sizeof(buf));

            dot = strrchr(buf, '.');
            if (dot)
                strlcpy(str, dot, buflen);
            else
                str[0] = 0;
        }
        return 1;

    case mvChar:
        {
            PELine L;
            int P;

            L = RLine(CP.Row);
            P = CharOffset(L, CP.Col);

            strlcpy(str, "", buflen);

            if (ChClass(L->Chars[P]))
            {
                char tmp[2];

                // make copy of character
                tmp[0] = L->Chars[P];
                tmp[1] = 0;

                strlcat(str, tmp, buflen);
            }
        }
        return 1;

    case mvWord:
        {
            PELine L;
            int P, C;
            int wordBegin, wordEnd;

            L = RLine(CP.Row);
            P = CharOffset(L, CP.Col);

            strlcpy(str, "", buflen);

            if (ChClass(L->Chars[P]))
            {
                C = ChClassK(L->Chars[P]);

                // search start of word
                while ((P>0) && (C == ChClassK(L->Chars[P-1]))) P--;

                wordBegin = P;

                // search end of word
                while ((P< L->Count) && (C == ChClassK(L->Chars[P]))) P++;

                wordEnd = P;

                // calculate total length for buffer copy
                size_t length = wordEnd - wordBegin;

                if ((length + 1) < buflen)
                {
                    length++;
                } else
                {
                    length = buflen;
                }

                // copy word to buffer
                strlcpy(str, &L->Chars[wordBegin], length);
            }
        }
        return 1;

    case mvLine:
        {
            PELine L;

            L = RLine(CP.Row);

            strlcpy(str, "", buflen);

            if (L->Count > 0)
            {
                // calculate total length for buffer copy
                size_t length = L->Count;

                if ((length + 1) < buflen)
                {
                    length++;
                } else
                {
                    length = buflen;
                }

                // copy word to buffer
                strlcpy(str, L->Chars, length);
            }
        }
        return 1;

    case mvFTEVer:
        strlcpy(str, VERSION, buflen);
        return 1;
    }

    return EModel::GetStrVar(var, str, buflen);
}

int EBuffer::GetIntVar(int var, int *value) {
    switch (var) {
    case mvCurRow: *value = VToR(CP.Row) + 1; return 1;
    case mvCurCol: *value = CP.Col; return 1;
    }
    return EModel::GetIntVar(var, value);
}
