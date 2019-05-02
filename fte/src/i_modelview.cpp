/*    i_modelview.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "i_modelview.h"
#include "o_routine.h"

//#include <stdio.h>

ExModelView::ExModelView(EView *AView) :
    View(AView),
    MouseCaptured(0),
    MouseMoved(0)
{
    //fprintf(stderr, "Create ExModel %p   View %p\n", this, View);
    View->MView = this;
}

ExModelView::~ExModelView() {
    //fprintf(stderr, "Delete ExModel %p   View %p\n", this, View);
    //if (View->MView == this)
    //    View->MView = 0;
    delete View;
}

int ExModelView::GetContext() {
    return View->GetContext();
}

void ExModelView::Activate(int gotfocus) {
    ExView::Activate(gotfocus);
    View->Activate(gotfocus);
}

EEventMap *ExModelView::GetEventMap() {
    return View->GetEventMap();
}

int ExModelView::ExecCommand(ExCommands Command, ExState &State) {
    return View->ExecCommand(Command, State);
}

int ExModelView::BeginMacro() {
    return View->BeginMacro();
}

void ExModelView::HandleEvent(TEvent &Event) {
    ExView::HandleEvent(Event);
    View->HandleEvent(Event);
}

void ExModelView::UpdateView() {
    View->UpdateView();
}

void ExModelView::RepaintView() {
    View->RepaintView();
}

void ExModelView::RepaintStatus() {
    View->RepaintStatus();
}

void ExModelView::UpdateStatus() {
    View->UpdateStatus();
}

void ExModelView::Resize(int width, int height) {
    View->Resize(width, height);
}
