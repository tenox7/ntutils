/*    i_modelview.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef I_MODELVIEW_H
#define I_MODELVIEW_H

#include "i_oview.h"

class EView;

class ExModelView: public ExView {
    EView *View;

public:
    int MouseCaptured;
    int MouseMoved;

    ExModelView(EView *AView);
    virtual ~ExModelView();

    virtual void Activate(int gotfocus);

    virtual EEventMap *GetEventMap();
    virtual int ExecCommand(ExCommands Command, ExState &State);

    virtual int GetContext();
    virtual int BeginMacro();
    virtual void HandleEvent(TEvent &Event);
    virtual void UpdateView();
    virtual void RepaintView();
    virtual void UpdateStatus();
    virtual void RepaintStatus();
    virtual void Resize(int width, int height);
    virtual int IsModelView() { return 1; }

    EView* GetView() { return  View; }
};

#endif // I_MODELVIEW_H
