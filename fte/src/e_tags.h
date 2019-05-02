/*    e_tags.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef E_TAGS_H
#define E_TAGS_H

#include "fte.h"

#ifdef CONFIG_TAGS

#include <stdio.h> // FILE

class EView;
class EBuffer;

int TagsAdd(const char *FileName);
int TagsSave(FILE *fp);

int TagLoad(const char *FileName);
void TagClear();
int TagGoto(EView *V, const char *Tag);
int TagDefined(const char *Tag);
int TagFind(EBuffer *B, EView *V, const char *Tag);
int TagComplete(char **Words, int *WordsPos, int WordsMax, const char *Tag);
int TagNext(EView *V);
int TagPrev(EView *V);
int TagPop(EView *V);

#endif

#endif // E_TAGS_H
