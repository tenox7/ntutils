// used in text mode versions, should never be called (just for linking)

#include "gui.h"

#include <stdarg.h>
#include <assert.h>
#include <sys/types.h>

int DLGGetFile(GView * /*v*/, const char * /*Prompt*/, unsigned int /*BufLen*/, char * /*FileName*/, int /*Flags*/) {
    assert(1==0);
    return 0;
}

int DLGPickChoice(GView * /*v*/, const char * /*ATitle*/, int /*NSel*/, va_list /*ap*/, int /*Flags*/) {
    assert(1==0);
    return 0;
}

int DLGGetFind(GView * /*View*/, SearchReplaceOptions &/*sr*/) {
    assert(1==0);
    return 0;
}

int DLGGetFindReplace(GView * /*View*/, SearchReplaceOptions &/*sr*/) {
    assert(1==0);
    return 0;
}

int DLGGetStr(GView * /*View*/, const char * /*Prompt*/, unsigned int /*BufLen*/, char * /*Str*/, int /*HistId*/, int /*Flags*/) {
    assert(1 == 0);
    return 0;
}
