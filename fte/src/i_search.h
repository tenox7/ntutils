/*    i_search.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef I_SEARCH_H
#define I_SEARCH_H

#include "i_oview.h"
#include "e_buffer.h"

class ExISearch: public ExViewNext
{
    enum IState { IOk, INoMatch, INoPrev, INoNext };
public:
    static const unsigned MAXISEARCH = 256;

    ExISearch(EBuffer *B);
    virtual ~ExISearch();

    virtual void HandleEvent(TEvent &Event);
    virtual void RepaintStatus();

    void SetState(IState state);

private:
    char ISearchStr[MAXISEARCH + 1];
    EPoint Orig;
    EPoint stack[MAXISEARCH];
    int len;
    int stacklen;
    EBuffer *Buffer;
    IState state;
    int Direction;
};

#endif // I_SEARCH_H
