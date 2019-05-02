// Contributed by Markus F.X.J. Oberhumer <markus.oberhumer@jk.uni-linz.ac.at>

#include "fte.h"

#include <stddef.h>
#include <ctype.h>

#if defined(__DJGPP__) || defined(UNIX)

#ifdef __cplusplus
extern "C"
#endif
int memicmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *p1 = (const unsigned char *) s1;
    const unsigned char *p2 = (const unsigned char *) s2;

    for (; n-- > 0; p1++, p2++) {
        int c;
        if (*p1 != *p2
            && (c = (toupper(*p1) - toupper(*p2))))
            return c;
    }
    return 0;
}

#endif
