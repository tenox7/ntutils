/*    e_undo.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "e_undo.h"

#include "o_buflist.h"
#include "s_util.h"
#include "sysdep.h"

#include <stdio.h>

int EBuffer::NextCommand() {
    if (Match.Row != -1) {
        Draw(Match.Row, Match.Row);
        Match.Col = Match.Row = -1;
    }
    if (View)
        View->SetMsg(0);
#ifdef CONFIG_UNDOREDO
    return BeginUndo();
#else
    return 1;
#endif
}

#ifdef CONFIG_UNDOREDO

int EBuffer::PushBlockData() {
    if (BFI(this, BFI_Undo) == 0) return 1;
    if (PushULong(BB.Col) == 0) return 0;
    if (PushULong(BB.Row) == 0) return 0;
    if (PushULong(BE.Col) == 0) return 0;
    if (PushULong(BE.Row) == 0) return 0;
    if (PushULong(BlockMode) == 0) return 0;
    if (PushUChar(ucBlock) == 0) return 0;
    return 1;
}

int EBuffer::BeginUndo() {
    US.NextCmd = 1;
    return 1;
}

int EBuffer::EndUndo() {
    int N = US.Num - 1;
    
    assert(N >= 0);
    if (N >= 1) {
        int Order = 1;
        
        while (Order < N) Order <<= 1;

        US.Data = (void **) realloc(US.Data, sizeof(void *) * Order);
        US.Top = (int *) realloc(US.Top, sizeof(int) * Order);
        US.Num--;
    } else {
        free(US.Data); US.Data = 0;
        free(US.Top); US.Top = 0;
        US.Num = 0;
    }
    return 1;
}

int EBuffer::PushULong(unsigned long l) {
//    static unsigned long x = l;
    return PushUData(&l, sizeof(unsigned long));
}

int EBuffer::PushUChar(unsigned char ch) {
    return PushUData(&ch, sizeof(unsigned char));
}


int EBuffer::PushUData(const void *data, size_t len) {
    int N;
    int Order = 1;
    
//    printf("UPUSH: %d %c\n", len, *(char *)data); fflush(stdout);

    if (BFI(this, BFI_Undo) == 0) return 0;
    if (US.Record == 0) return 1;
    if (US.NextCmd || US.Num == 0 || US.Data == 0 || US.Top == 0) {
        N = US.Num;
        if ((BFI(this, BFI_UndoLimit) == -1) || (US.Undo) || (US.Num < BFI(this, BFI_UndoLimit))) {
            N++;
            US.Data = (void **) realloc(US.Data, sizeof(void *) * (N | 255));
            US.Top = (int *)   realloc(US.Top,  sizeof(int)    * (N | 255));
            if (US.Num == US.UndoPtr && !US.Undo)
                US.UndoPtr++;
            US.Num++;
        } else {
            N = US.Num;
            free(US.Data[0]);
            memmove(US.Data, US.Data + 1, (N - 1) * sizeof(US.Data[0]));
            memmove(US.Top,  US.Top  + 1, (N - 1) * sizeof(US.Top[0]));
        }
        assert(US.Data);
        assert(US.Top);
        N = US.Num - 1;
        US.Data[N] = 0;
        US.Top[N] = 0;
        if (US.NextCmd == 1) {
            US.NextCmd = 0;
//            puts("\x7");
            if (PushULong(CP.Col) == 0) return 0;
            if (PushULong(CP.Row) == 0) return 0;
            if (PushUChar(ucPosition) == 0) return 0;
//            puts("\x7");
        }
        US.NextCmd = 0;
    }
    
    N = US.Num - 1;
    assert(N >= 0);
    
    if (US.Undo == 0) US.UndoPtr = US.Num;

    while (Order < (US.Top[N] + len)) Order <<= 1;
    US.Data[N] = realloc(US.Data[N], Order);
    memcpy((char *) US.Data[N] + US.Top[N], data, len);
    US.Top[N] += len;
    return 1;
}

int EBuffer::GetUData(int No, int pos, void **data, size_t len) {
    int N;
    
    if (No == -1)
        N = US.Num - 1;
    else
        N = No;

    if (BFI(this, BFI_Undo) == 0) return 0;
    if (N < 0) return 0;
    if (US.Data[N] == 0) return 0;
    if (US.Top[N] == 0) return 0;
    
    if (pos == -1)
        pos = US.Top[N];
    
    
    if (pos == 0) 
        return 0;
//    printf("N,pos = %d,%d len = %d\n", N, pos, len);
    
    assert(pos >= len);
    *data = ((char *) US.Data[N]) + pos - len;
    return 1;
}

#define UGETC(rc,no,pos,what) \
    do { void *d; \
    rc = GetUData(no, pos, &d, sizeof(unsigned char)); \
    *(unsigned char *)&what = *(unsigned char *)d; \
    pos -= (int) sizeof(unsigned char); \
    } while (0)

#define UGET(rc,no,pos,what) \
    do { void *d; \
    rc = GetUData(no, pos, &d, sizeof(what)); \
    memcpy(&what, d, sizeof(what)); \
    pos -= (int) sizeof(what); \
    } while (0)

int EBuffer::Undo(int undo) {
    unsigned char UndoCmd;
    int rc;
unsigned    long Line;
unsigned    long Len;
unsigned    long ACount;
unsigned    long Col;
    void *data;
    
    int No;
    int Pos;
    
    if (BFI(this, BFI_Undo) == 0)
        return 0;
    
    if (undo)
        No = US.UndoPtr - 1;
    else
        No = US.Num - 1;
    
    Pos = US.Top[No];
    
    if (No == 0 && Pos == 0) {
        //puts("bottom");
        return 0;
    }
//    for (int i = 0; i < Pos; i++) {
//        printf("%d: %d\n", i, ((char *)US.Data[No])[i]);
//    }
    
//   printf("Undo %d %d,%d\n", undo, No, Pos); fflush(stdout);
    
//    fprintf(stderr, "\nNo = %d, Num = %d\n", No, US.Num);
    UGETC(rc, No, Pos, UndoCmd);
    while (rc == 1) {
//        printf("%d Undo %d %d,%d\n", UndoCmd, undo, No, Pos); fflush(stdout);
  //  for (int i = 0; i < Pos; i++) {
//        printf("%d: %d\n", i, ((char *)US.Data[No])[i]);
//    }
        switch (UndoCmd) {
        case ucInsLine:
            UGET(rc, No, Pos, Line); if (rc == 0) return 0;
//            printf("\tDelLine %d\n", Line);
            if (DelLine(Line) == 0) return 0;
            break;
            
        case ucDelLine:
            UGET(rc, No, Pos, Line); if (rc == 0) return 0;
            UGET(rc, No, Pos, Len); if (rc == 0) return 0;
            if (GetUData(No, Pos, &data, Len) == 0) return 0;
//            printf("\tInsLine %d\n", Line);
            if (InsLine(Line, 0) == 0) return 0;
//            printf("\tInsText %d - %d\n", Line, Len);
            if (InsText(Line, 0, Len, (char *) data) == 0) return 0;
            Pos -= Len;
            break;
            
        case ucInsChars:
            UGET(rc, No, Pos, ACount); if (rc == 0) return 0;
            UGET(rc, No, Pos, Col); if (rc == 0) return 0;
            UGET(rc, No, Pos, Line); if (rc == 0) return 0;
//            printf("\tDelChars %d %d %d\n", Line, Col, ACount);
            if (DelChars(Line, Col, ACount) == 0) return 0;
            break;
            
        case ucDelChars:
            UGET(rc, No, Pos, Line); if (rc == 0) return 0;
            UGET(rc, No, Pos, Col); if (rc == 0) return 0;
            UGET(rc, No, Pos, ACount); if (rc == 0) return 0;
            if (GetUData(No, Pos, &data, ACount) == 0) return 0;
//            printf("\tInsChars %d %d %d\n", Line, Col, ACount);
            if (InsChars(Line, Col, ACount, (char *) data) == 0) return 0;
            Pos -= ACount;
            break;
            
        case ucPosition:
            UGET(rc, No, Pos, Line); if (rc == 0) return 0;
            UGET(rc, No, Pos, Col); if (rc == 0) return 0;
//            printf("\tSetPos %d %d\n", Line, Col);
            if (SetPos(Col, Line) == 0) return 0;
            break;
          
        case ucBlock: 
            {
                EPoint P;
		unsigned long l;
                
//                printf("\tBlock\n");
                UGET(rc, No, Pos, l); if (rc == 0) return 0;
                if (BlockMode != (int)l) BlockRedraw();
		BlockMode = (int)l;
                UGET(rc, No, Pos, l); if (rc == 0) return 0;   P.Row = (int)l;
                UGET(rc, No, Pos, l); if (rc == 0) return 0;   P.Col = (int)l;
                if (SetBE(P) == 0) return 0;
                UGET(rc, No, Pos, l); if (rc == 0) return 0;   P.Row = (int)l;
                UGET(rc, No, Pos, l); if (rc == 0) return 0;   P.Col = (int)l;
                if (SetBB(P) == 0) return 0;
            }
            break;

        case ucFoldCreate:
            // puts("ucFoldCreate");
            UGET(rc, No, Pos, Line); if (rc == 0) return 0;
            if (FoldDestroy(Line) == 0) return 0;
            break;
        
        case ucFoldDestroy:
            // puts("ucFoldDestroy");
            {
unsigned                int level;
                int ff;
                
                UGET(rc, No, Pos, Line); if (rc == 0) return 0;
                UGET(rc, No, Pos, level); if (rc == 0) return 0;
                if (FoldCreate(Line) == 0) return 0;
                
                ff = FindFold(Line);
                assert(ff != -1);
                FF[ff].level = (unsigned char) level;
            }
            break;
        case ucFoldPromote:
            // puts("ucFoldPromote");
            UGET(rc, No, Pos, Line); if (rc == 0) return 0;
            if (FoldDemote(Line) == 0) return 0;
            break;
            
        case ucFoldDemote:
            // puts("ucFoldDemote");
            UGET(rc, No, Pos, Line); if (rc == 0) return 0;
            if (FoldPromote(Line) == 0) return 0;
            break;
            
        case ucFoldOpen:
            // puts("ucFoldOpen");
            UGET(rc, No, Pos, Line); if (rc == 0) return 0;
            if (FoldClose(Line) == 0) return 0;
            break;
            
        case ucFoldClose:
            // puts("ucFoldClose");
            UGET(rc, No, Pos, Line); if (rc == 0) return 0;
            if (FoldOpen(Line) == 0) return 0;
            break;
            
        case ucModified:
//            printf("\tModified\n");
            Modified = 0;
            break;

        case ucPlaceUserBookmark:
            //puts ("ucPlaceUserBookmark");
            UGET(rc, No, Pos, ACount); if (rc == 0) return 0;
            if (GetUData(No, Pos, &data, ACount) == 0) return 0;
            Pos -= ACount;
            UGET(rc, No, Pos, Col); if (rc == 0) return 0;
            UGET(rc, No, Pos, Line); if (rc == 0) return 0;
//            if (Col == -1 || Line == -1) {
            if (Col == (unsigned long)-1 || Line == (unsigned long)-1) {
                if (RemoveUserBookmark ((const char *)data)==0) return 0;
            } else {
                if (PlaceUserBookmark ((const char *)data,EPoint (Line,Col))==0) return 0;
            }
            break;

        case ucRemoveUserBookmark:
            //puts("ucRemoveUserBookmark");
            UGET(rc, No, Pos, ACount); if (rc == 0) return 0;
            if (GetUData(No, Pos, &data, ACount) == 0) return 0;
            Pos -= ACount;
            UGET(rc, No, Pos, Col); if (rc == 0) return 0;
            UGET(rc, No, Pos, Line); if (rc == 0) return 0;
            if (PlaceUserBookmark ((const char *)data,EPoint (Line,Col))==0) return 0;
            break;

        default:
            fprintf(stderr, "Oops: invalid undo command  %d.\n", UndoCmd);
            return 0;
            //assert(1 == "Oops: invalid undo command.\n"[0]);
        }
//        puts("\tok");
        
//        fprintf(stderr, "\nNo = %d, Num = %d\n", No, US.Num);
        UGETC(rc, No, Pos, UndoCmd);
    }
    
    if (undo)
        US.UndoPtr--;
    else {
        US.UndoPtr++;
        free(US.Data[No]);
        if (EndUndo() == 0) return 0;
    }
    
    return 1;
}

int EBuffer::Redo() {
    int rc;
    
    if (BFI(this, BFI_Undo) == 0) return 0;
    
//    US.NextCmd = 0; // disable auto push position
    
    if (US.Num == 0 || US.UndoPtr == US.Num) {
        Msg(S_INFO, "Nothing to redo.");
        return 0;
    }
    
    US.Record = 0;
    rc =  Undo(0);
    US.Record = 1;
    return rc;
}

int EBuffer::Undo() {
    int rc;
    
    if (BFI(this, BFI_Undo) == 0) return 0;
    
    assert(US.Num >= 0);
    assert(US.UndoPtr >= 0);
    if (US.Num == 0 || US.UndoPtr == 0) {
        Msg(S_INFO, "Nothing to undo.");
        return 0;
    }
    
    US.Undo = 1;
    rc = Undo(1);
    US.Undo = 0;
    return rc;
}
#endif
