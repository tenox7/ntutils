/*    clip_os2.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "fte.h"
#include "e_buffer.h"
#include "clip.h"

int GetPMClip(int clipboard) {
    ClipData cd;
    unsigned int i, j, l, dx;
    EPoint P;

    if (clipboard != 0)
        // only one clipboard supported
        return 0;

    if (!GetClipText(&cd))
        return 0;

    SSBuffer->Clear();
    j = 0;
    l = 0;

    for (i = 0; i < cd.fLen; i++) {
        if (cd.fChar[i] == 0x0A) {
            SSBuffer->AssertLine(l);
            P.Col = 0; P.Row = l++;
            dx = 0;
            if ((i > 0) && (cd.fChar[i-1] == 0x0D)) dx++;
            SSBuffer->InsertLine(P, i - j - dx, cd.fChar + j);
            j = i + 1;
        }
    }

    if (j < cd.fLen) { // remainder
        i = cd.fLen;
        SSBuffer->AssertLine(l);
        P.Col = 0; P.Row = l++;
        dx = 0;
        if ((i > 0) && (cd.fChar[i-1] == 0x0D)) dx++;
        SSBuffer->InsText(P.Row, P.Col, i - j - dx, cd.fChar + j);
        j = i + 1;
    }

    // now that we don't need cd.fChar anymore, free it
    free(cd.fChar);

    return 1;
}

int PutPMClip(int clipboard) {
    char *p = NULL;
    int l = 0;
    PELine L;
    int Len;
    ClipData cd;
    int rc;

    if (clipboard != 0)
        // only one clipboard supported
        return 0;

    for (int i = 0; i < SSBuffer->RCount; i++) {
        L = SSBuffer->RLine(i);
        p = (char *)realloc(p, l + (Len = L->Count) + 2);
        memcpy(p + l, L->Chars, L->Count);
        l += Len;
        if (i < SSBuffer->RCount - 1) {
            p[l++] = 13;
            p[l++] = 10;
        }
    }
    p = (char *)realloc(p, l + 1);
    p[l++] = 0;
    cd.fChar = p;
    cd.fLen = l;
    rc = PutClipText(&cd);
    free(p);

    return rc;
}
