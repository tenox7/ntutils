#include "e_mark.h"
#include "s_util.h"
#include "sysdep.h"

#include <ctype.h>

EMarkIndex markIndex;

EMark::EMark(const char *aName, const char *aFileName,
	     EPoint aPoint, EBuffer *aBuffer) :
    Name(aName),
    FileName(aFileName),
    Point(aPoint),
    Buffer(0)
{
    if (!aBuffer)
        aBuffer = FindFile(aFileName);
    if (aBuffer && aBuffer->Loaded)
        SetBuffer(aBuffer);
}

EMark::~EMark() {
    if (Buffer)
        RemoveBuffer(Buffer);
}

int EMark::SetBuffer(EBuffer *aBuffer) {
    assert(aBuffer != 0);
    assert(filecmp(aBuffer->FileName, FileName.c_str()) == 0);

    if (Point.Row >= aBuffer->RCount)
        Point.Row = aBuffer->RCount - 1;
    if (Point.Row < 0)
        Point.Row = 0;

    if (aBuffer->PlaceBookmark(Name.c_str(), Point) == 1) {
        Buffer = aBuffer;
        return 1;
    }
    return 0;
}

int EMark::RemoveBuffer(EBuffer *aBuffer) {
    assert(aBuffer != 0);
    if (Buffer == 0 || Buffer != aBuffer)
        return 0;
    assert(filecmp(aBuffer->FileName, FileName.c_str()) == 0);

    if (!Buffer->GetBookmark(Name.c_str(), Point))
        return 0;
    if (!Buffer->RemoveBookmark(Name.c_str()))
        return 0;

    Buffer = 0;
    return 1;
}

EPoint &EMark::GetPoint() {
    if (Buffer) {
        assert(Buffer->GetBookmark(Name.c_str(), Point) != 0);
    }
    return Point;
}

EMarkIndex::EMarkIndex() {
}

EMarkIndex::~EMarkIndex() {
    vector_iterate(EMark*, Marks, it)
        delete (*it);
}

EMark *EMarkIndex::insert(const char *aName, const char *aFileName, EPoint aPoint, EBuffer *aBuffer) {

    assert(aName != 0 && aName[0] != 0);
    assert(aFileName != 0 && aFileName[0] != 0);

    size_t L = 0, R = Marks.size();

    while (L < R) {
        size_t M = (L + R) / 2;
        int cmp = strcmp(aName, Marks[M]->GetName());
        if (cmp == 0)
            return 0;
        if (cmp > 0)
            L = M + 1;
        else
            R = M;
    }

    EMark* m = new EMark(aName, aFileName, aPoint, aBuffer);
    Marks.insert(Marks.begin() + L, m);
    return m;
}

EMark *EMarkIndex::insert(const char *aName, EBuffer *aBuffer, EPoint aPoint) {
    assert(aName != 0 && aName[0] != 0);
    assert(aBuffer != 0);
    assert(aBuffer->FileName != 0);

    return insert(aName, aBuffer->FileName, aPoint, aBuffer);
}

EMark *EMarkIndex::locate(const char *aName) {
    assert(aName != 0 && aName[0] != 0);

    size_t L = 0, R = Marks.size();

    while (L < R) {
        size_t M = (L + R) / 2;
        int cmp = strcmp(aName, Marks[M]->GetName());
        if (cmp == 0)
            return Marks[M];
        else if (cmp > 0)
            L = M + 1;
        else
            R = M;
    }
    return 0;
}

int EMarkIndex::remove(const char *aName) {
    assert(aName != 0 && aName[0] != 0);

    size_t L = 0, R = Marks.size();

    while (L < R) {
        size_t M = (L + R) / 2;
        int cmp = strcmp(aName, Marks[M]->GetName());
        if (cmp == 0) {
            delete Marks[M];
            Marks.erase(Marks.begin() + M);
            return 1;
        } else if (cmp > 0)
            L = M + 1;
        else
            R = M;
    }
    return 0;
}

int EMarkIndex::view(EView *aView, const char *aName) {
    EMark *m = locate(aName);
    if (m) {
        EBuffer *b = m->GetBuffer();
        if (b == 0) {
            if (!FileLoad(0, m->GetFileName(), 0, aView))
                return 0;
            if (!retrieveForBuffer((EBuffer *)ActiveModel))
                return 0;
            b = (EBuffer *)ActiveModel;
        }
        aView->SwitchToModel(b);
        return b->GotoBookmark(m->GetName());
    }
    return 0;
}

int EMarkIndex::retrieveForBuffer(EBuffer *aBuffer) {
    vector_iterate(EMark*, Marks, it)
        if (((*it)->GetBuffer() == 0)
            && (filecmp(aBuffer->FileName, (*it)->GetFileName()) == 0))
            if (!(*it)->SetBuffer(aBuffer))
                return 0;
    return 1;
}

int EMarkIndex::storeForBuffer(EBuffer *aBuffer) {
    vector_iterate(EMark*, Marks, it)
        if (((*it)->GetBuffer() == aBuffer)
            && ((*it)->RemoveBuffer(aBuffer) == 0))
                return 0;
    return 1;
}

int EMarkIndex::saveToDesktop(FILE *fp) {
    vector_iterate(EMark*, Marks, it)
        // ??? file of buffer or of mark? (different if file renamed) ???
        // perhaps marks should be duplicated?
        if (fprintf(fp, "M|%d|%d|%s|%s\n",
                    (*it)->GetPoint().Row,
                    (*it)->GetPoint().Col,
                    (*it)->GetName(),
                    (*it)->GetFileName()) < 0)
            return 0;

    return 1;
}

// needs performance fixes (perhaps a redesign ?)

EMark *EMarkIndex::pushMark(EBuffer *aBuffer, EPoint P) {
    int stackTop = -1;

    vector_iterate(EMark*, Marks, it) {
        const char *name = (*it)->GetName();
        if (name && name[0] == '#' && isdigit(name[1])) {
            int no = atoi(name + 1);
            if (no > stackTop)
                stackTop = no;
        }
    }
    char name[20];
    sprintf(name, "#%d", stackTop + 1);
    return insert(name, aBuffer, P);
}

int EMarkIndex::popMark(EView *aView) {
    int stackTop = -1;

    vector_iterate(EMark*, Marks, it) {
        const char *name = (*it)->GetName();
        if (name && name[0] == '#' && isdigit(name[1])) {
            int no = atoi(name + 1);
            if (no > stackTop)
                stackTop = no;
        }
    }
    if (stackTop == -1)
        return 0;
    char name[20];
    sprintf(name, "#%d", stackTop);
    if (!view(aView, name))
        return 0;
    assert(remove(name) == 1);
    return 1;
}
