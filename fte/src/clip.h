/*    clip.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef CLIP_H
#define CLIP_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ClipData {
    size_t fLen;
    char *fChar;
};

int GetClipText(ClipData *cd);
int PutClipText(ClipData *cd);

#ifdef __cplusplus
}
#endif

#endif // CLIP_H
