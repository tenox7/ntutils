#ifndef E_MARK_H
#define E_MARK_H

#include "e_buffer.h"

#include <stdio.h> // FILE

class EMark {
public:
    EMark(const char *aName, const char *aFileName, EPoint aPoint, EBuffer *aBuffer = 0);
    ~EMark();

    int SetBuffer(EBuffer *aBuffer);
    int RemoveBuffer(EBuffer *aBuffer);

    const char *GetName() const { return Name.c_str(); }
    const char *GetFileName() const { return FileName.c_str(); }
    EPoint &GetPoint();
    EBuffer *GetBuffer() { return Buffer; }
private:
    /* bookmark */
    StlString Name;
    StlString FileName;
    EPoint Point;

    /* bookmark in file */
    EBuffer *Buffer;
};

class EMarkIndex {
public:
    EMarkIndex();
    ~EMarkIndex();

    EMark *insert(const char *aName, const char *aFileName, EPoint aPoint, EBuffer* aBuffer = 0);
    EMark *insert(const char *aName, EBuffer* aBuffer, EPoint aPoint);
    EMark *locate(const char *aName);
    int remove(const char *aName);
    int view(EView *aView, const char *aName);

//    int MarkPush(EBuffer *B, EPoint P);
//    int MarkPop(EView *V);
//    int MarkSwap(EView *V, EBuffer *B, EPoint P);
//    int MarkNext(EView *V);
    //    int MarkPrev(EView *V);
    EMark *pushMark(EBuffer *aBuffer, EPoint P);
    int popMark(EView *aView);

    int retrieveForBuffer(EBuffer *aBuffer);
    int storeForBuffer(EBuffer *aBuffer);

    int saveToDesktop(FILE *fp);

private:
    StlVector<EMark*> Marks;
};

extern EMarkIndex markIndex;

#endif // E_MARK_H
